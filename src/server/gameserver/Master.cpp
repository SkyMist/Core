/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
    \ingroup Trinityd
*/

#include <Common.h>
#include "SystemConfig.h"
#include "World.h"
#include "Configuration/Config.h"
#include "Database/DatabaseEnv.h"
#include "Database/DatabaseWorkerPool.h"
#include "PlayerDump.h"
#include "Player.h"
#include "ObjectMgr.h"

#include "CliRunnable.h"
#include "Log.h"
#include "Master.h"
#include "RARunnable.h"
#include "TCSoap.h"
#include "Timer.h"
#include "Util.h"
#include "AuthSocket.h"
#include "RealmList.h"

#include "BigNumber.h"

#ifdef _WIN32
#include "ServiceWin32.h"
extern int m_ServiceStatus;
#endif

void C_SignalCallback(uv_signal_t* handle, int signum)
{
    switch (signum)
    {
        case SIGINT:
            World::StopNow(RESTART_EXIT_CODE);
            break;
        case SIGTERM:
#ifdef _WIN32
        case SIGBREAK:
            if (m_ServiceStatus != 1)
#endif
                World::StopNow(SHUTDOWN_EXIT_CODE);
            break;
    }
}

void C_FreezeThread(void* data)
{
    uint32 DelayTime = *reinterpret_cast<uint32*>(data);
    
    if (!DelayTime)
        return;
    sLog->outInfo(LOG_FILTER_WORLDSERVER, "Starting up anti-freeze thread (%u seconds max stuck time)...", DelayTime/1000);
    uint32 w_loops = 0;
    uint32 w_lastchange = 0;
    while (!World::IsStopped())
    {
        sleep(1000);
        uint32 curtime = getMSTime();
        // normal work
        uint32 worldLoopCounter = World::m_worldLoopCounter;
        if (w_loops != worldLoopCounter)
        {
            w_lastchange = curtime;
            w_loops = worldLoopCounter;
        }
        // possible freeze
        else if (getMSTimeDiff(w_lastchange, curtime) > DelayTime)
        {
            sLog->outError(LOG_FILTER_WORLDSERVER, "World Thread hangs, kicking out server!");
            // ASSERT(false);
            exit(0);
        }
    }
    sLog->outInfo(LOG_FILTER_WORLDSERVER, "Anti-freeze thread exiting without problems.");
}

Master::Master()
{
    uv_loop_init(&_loop);
    
    uv_signal_init(&_loop, &_signalInt);
    uv_signal_init(&_loop, &_signalTrm);
    uv_signal_init(&_loop, &_signalBrk);
}

Master::~Master()
{
    uv_signal_stop(&_signalInt);
    uv_signal_stop(&_signalBrk);
    uv_signal_stop(&_signalTrm);
    
    uv_loop_close(&_loop);
}

/// Main function
int Master::Run()
{
    BigNumber seed1;
    seed1.SetRand(16 * 8);

    sLog->outInfo(LOG_FILTER_WORLDSERVER, "%s (worldserver-daemon)", _FULLVERSION);
    sLog->outInfo(LOG_FILTER_WORLDSERVER, "<Ctrl-C> to stop.\n");

    sLog->outInfo(LOG_FILTER_WORLDSERVER, " #########  ##  ##  #    #  ###    ###  ##  ##########  ##########");
    sLog->outInfo(LOG_FILTER_WORLDSERVER, " ###        ## ##    #  #   ## #  # ##      ###             ##");
    sLog->outInfo(LOG_FILTER_WORLDSERVER, " #########  ###       ##    ##  ##  ##  ##  ##########      ##");
    sLog->outInfo(LOG_FILTER_WORLDSERVER, "       ###  ## ##     ##    ##  ##  ##  ##         ###      ##");
    sLog->outInfo(LOG_FILTER_WORLDSERVER, " #########  ##  ##    ##    ##  ##  ##  ##  ##########      ##");
    sLog->outInfo(LOG_FILTER_WORLDSERVER, "     v. 5.4.7                         C O R E ");
    sLog->outInfo(LOG_FILTER_WORLDSERVER, "  Project SkyMist 2014(c) Game Emulation <http://www.sky-mist.net/> \n");

    /// worldserver PID file creation
    std::string pidfile = ConfigMgr::GetStringDefault("PidFile", "");
    if (!pidfile.empty())
    {
        uint32 pid = CreatePIDFile(pidfile);
        if (!pid)
        {
            sLog->outError(LOG_FILTER_WORLDSERVER, "Cannot create PID file %s.\n", pidfile.c_str());
            return 1;
        }

        sLog->outInfo(LOG_FILTER_WORLDSERVER, "Daemon PID: %u\n", pid);
    }

    ///- Start the databases
    if (!_StartDB())
        return 1;

    ///- Initialize the World
    sWorld->SetInitialWorldSettings();

    ///- Register worldserver's signal handlers
    uv_signal_start(&_signalInt, C_SignalCallback, SIGINT);

#ifdef _WIN32
    uv_signal_start(&_signalBrk, C_SignalCallback, SIGBREAK);
#endif
    
    uv_signal_start(&_signalTrm, C_SignalCallback, SIGTERM);
    
    /*ACE_Based::Thread world_thread(new WorldRunnable);
    world_thread.setPriority(ACE_Based::Highest);*/

    ACE_Based::Thread* cliThread = NULL;

#ifdef _WIN32
    if (ConfigMgr::GetBoolDefault("Console.Enable", true) && (m_ServiceStatus == -1)/* need disable console in service mode*/)
#else
    if (ConfigMgr::GetBoolDefault("Console.Enable", true))
#endif
    {
        ///- Launch CliRunnable thread
        cliThread = new ACE_Based::Thread(new CliRunnable);
    }

    ACE_Based::Thread rar_thread(new RARunnable);

    ///- Handle affinity for multiple processors and process priority on Windows
    #ifdef _WIN32
    {
        HANDLE hProcess = GetCurrentProcess();

        uint32 Aff = ConfigMgr::GetIntDefault("UseProcessors", 0);
        if (Aff > 0)
        {
            ULONG_PTR appAff;
            ULONG_PTR sysAff;

            if (GetProcessAffinityMask(hProcess, &appAff, &sysAff))
            {
                ULONG_PTR curAff = Aff & appAff;            // remove non accessible processors

                if (!curAff)
                {
                    sLog->outError(LOG_FILTER_WORLDSERVER, "Processors marked in UseProcessors bitmask (hex) %x are not accessible for the worldserver. Accessible processors bitmask (hex): %x", Aff, appAff);
                }
                else
                {
                    if (SetProcessAffinityMask(hProcess, curAff))
                        sLog->outInfo(LOG_FILTER_WORLDSERVER, "Using processors (bitmask, hex): %x", curAff);
                    else
                        sLog->outError(LOG_FILTER_WORLDSERVER, "Can't set used processors (hex): %x", curAff);
                }
            }
        }

        bool Prio = ConfigMgr::GetBoolDefault("ProcessPriority", false);

        //if (Prio && (m_ServiceStatus == -1)  /* need set to default process priority class in service mode*/)
        if (Prio)
        {
            if (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
                sLog->outInfo(LOG_FILTER_WORLDSERVER, "worldserver process priority class set to HIGH");
            else
                sLog->outError(LOG_FILTER_WORLDSERVER, "Can't set worldserver process priority class.");
        }
    }
    #endif
    //Start soap serving thread
    ACE_Based::Thread* soap_thread = NULL;

    if (ConfigMgr::GetBoolDefault("SOAP.Enabled", false))
    {
        TCSoapRunnable* runnable = new TCSoapRunnable();
        runnable->setListenArguments(ConfigMgr::GetStringDefault("SOAP.IP", "127.0.0.1"), uint16(ConfigMgr::GetIntDefault("SOAP.Port", 7878)));
        soap_thread = new ACE_Based::Thread(runnable);
    }
    
    ///- Setup up freeze catcher thread
    uint32* DelayTime = new uint32;
    *DelayTime = ConfigMgr::GetIntDefault("MaxCoreStuckTime", 0);
    uv_thread_t FreezeThread;
    uv_thread_create(&FreezeThread, C_FreezeThread, reinterpret_cast<void*>(DelayTime));

    ///- Launch the world listener socket
    StartNetworking(ConfigMgr::GetStringDefault("BindIP", "0.0.0.0"), sWorld->getIntConfig(CONFIG_PORT_WORLD));
    
    sLog->outInfo(LOG_FILTER_WORLDSERVER, "%s (worldserver-daemon) ready...", _FULLVERSION);
    
    uv_run(&_loop, UV_RUN_DEFAULT);
    // when the main thread closes the singletons get unloaded
    // since worldrunnable uses them, it will crash if unloaded after master
    uv_thread_join(&FreezeThread);
    delete DelayTime;
    
    rar_thread.wait();

    if (soap_thread)
    {
        soap_thread->wait();
        soap_thread->destroy();
        delete soap_thread;
    }

    sLog->outInfo(LOG_FILTER_WORLDSERVER, "Halting process...");

    if (cliThread)
    {
        #ifdef _WIN32

        // this only way to terminate CLI thread exist at Win32 (alt. way exist only in Windows Vista API)
        //_exit(1);
        // send keyboard input to safely unblock the CLI thread
        INPUT_RECORD b[5];
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
        b[0].EventType = KEY_EVENT;
        b[0].Event.KeyEvent.bKeyDown = TRUE;
        b[0].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[0].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[0].Event.KeyEvent.wRepeatCount = 1;

        b[1].EventType = KEY_EVENT;
        b[1].Event.KeyEvent.bKeyDown = FALSE;
        b[1].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[1].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[1].Event.KeyEvent.wRepeatCount = 1;

        b[2].EventType = KEY_EVENT;
        b[2].Event.KeyEvent.bKeyDown = TRUE;
        b[2].Event.KeyEvent.dwControlKeyState = 0;
        b[2].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[2].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[2].Event.KeyEvent.wRepeatCount = 1;
        b[2].Event.KeyEvent.wVirtualScanCode = 0x1c;

        b[3].EventType = KEY_EVENT;
        b[3].Event.KeyEvent.bKeyDown = FALSE;
        b[3].Event.KeyEvent.dwControlKeyState = 0;
        b[3].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[3].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[3].Event.KeyEvent.wVirtualScanCode = 0x1c;
        b[3].Event.KeyEvent.wRepeatCount = 1;
        DWORD numb;
        WriteConsoleInput(hStdIn, b, 4, &numb);

        cliThread->wait();

        #else

        cliThread->destroy();

        #endif

        delete cliThread;
    }

    // for some unknown reason, unloading scripts here and not in worldrunnable
    // fixes a memory leak related to detaching threads from the module
    //UnloadScriptingModule();

    // Exit the process with specified return value
    return World::GetExitCode();
}

/// Stop the event loop, kills the server
void Master::Stop()
{
    //stop the accepting socket
    //save everything to the DB
    //disconnect all the connected players
    uv_stop(&_loop);
    ClearOnlineAccounts();
    _StopDB();
}

/// Initialize connection to the databases
bool Master::_StartDB()
{
    MySQL::Library_Init();

    std::string dbstring;
    uint8 async_threads, synch_threads;

    dbstring = ConfigMgr::GetStringDefault("WorldDatabaseInfo", "");
    if (dbstring.empty())
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "World database not specified in configuration file");
        return false;
    }

    async_threads = ConfigMgr::GetIntDefault("WorldDatabase.WorkerThreads", 1);
    if (async_threads < 1 || async_threads > 32)
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "World database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = ConfigMgr::GetIntDefault("WorldDatabase.SynchThreads", 1);
    ///- Initialise the world database
    if (!WorldDatabase.Open(dbstring, async_threads, synch_threads))
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "Cannot connect to world database %s", dbstring.c_str());
        return false;
    }

    ///- Get character database info from configuration file
    dbstring = ConfigMgr::GetStringDefault("CharacterDatabaseInfo", "");
    if (dbstring.empty())
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "Character database not specified in configuration file");
        return false;
    }

    async_threads = ConfigMgr::GetIntDefault("CharacterDatabase.WorkerThreads", 1);
    if (async_threads < 1 || async_threads > 32)
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "Character database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = ConfigMgr::GetIntDefault("CharacterDatabase.SynchThreads", 2);

    ///- Initialise the Character database
    if (!CharacterDatabase.Open(dbstring, async_threads, synch_threads))
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "Cannot connect to Character database %s", dbstring.c_str());
        return false;
    }

    ///- Get login database info from configuration file
    dbstring = ConfigMgr::GetStringDefault("LoginDatabaseInfo", "");
    if (dbstring.empty())
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "Login database not specified in configuration file");
        return false;
    }

    async_threads = ConfigMgr::GetIntDefault("LoginDatabase.WorkerThreads", 1);
    if (async_threads < 1 || async_threads > 32)
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "Login database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = ConfigMgr::GetIntDefault("LoginDatabase.SynchThreads", 1);
    ///- Initialise the login database
    if (!LoginDatabase.Open(dbstring, async_threads, synch_threads))
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "Cannot connect to login database %s", dbstring.c_str());
        return false;
    }

    ///- Get the realm Id from the configuration file
    realmID = ConfigMgr::GetIntDefault("RealmID", 0);
    if (!realmID)
    {
        sLog->outError(LOG_FILTER_WORLDSERVER, "Realm ID not defined in configuration file");
        return false;
    }
    sLog->outInfo(LOG_FILTER_WORLDSERVER, "Realm running as realm ID %d", realmID);

    sLog->SetRealmID(realmID);

    ///- Clean the database before starting
    ClearOnlineAccounts();

    ///- Insert version info into DB
    WorldDatabase.PExecute("UPDATE version SET core_version = '%s', core_revision = '%s'", _FULLVERSION, _HASH);        // One-time query

    sWorld->LoadDBVersion();

    sLog->outInfo(LOG_FILTER_WORLDSERVER, "Using World DB: %s", sWorld->GetDBVersion());
    return true;
}

void Master::_StopDB()
{
    CharacterDatabase.Close();
    WorldDatabase.Close();
    LoginDatabase.Close();

    MySQL::Library_End();
}

/// Clear 'online' status for all accounts with characters in this realm
void Master::ClearOnlineAccounts()
{
    // Reset online status for all accounts with characters on the current realm
    LoginDatabase.DirectPExecute("UPDATE account SET online = 0 WHERE online > 0 AND id IN (SELECT acctid FROM realmcharacters WHERE realmid = %d)", realmID);

    // Reset online status for all characters
    CharacterDatabase.DirectExecute("UPDATE characters SET online = 0 WHERE online <> 0");

    // Battleground instance ids reset at server restart
    CharacterDatabase.DirectExecute("UPDATE character_battleground_data SET instanceId = 0");
}
