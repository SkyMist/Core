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

/// \addtogroup Trinityd
/// @{
/// \file

#ifndef _MASTER_H
#define _MASTER_H

#include <Common.h>
#include <Networking/Networking.h>

/// Start the server
class Master : public Networking::Server
{
    public:
        Master();
        ~Master();
        int Run();
        void Stop();

    private:
        bool _StartDB();
        void _StopDB();

        void ClearOnlineAccounts();
    
    private:
        uv_signal_t _signalInt;
        uv_signal_t _signalBrk;
        uv_signal_t _signalTrm;
        uv_loop_t   _loop;
};

#define sMaster ACE_Singleton<Master, ACE_Null_Mutex>::instance()
#endif
/// @}
