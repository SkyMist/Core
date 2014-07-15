/*
 * Copyright (C) 2011-2014 Project SkyFire <http://www.projectskyfire.org/>
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2014 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

/* ScriptData
Name: arena_commandscript
%Complete: 100
Comment: All arena team related commands
Category: commandscripts
EndScriptData */

#include "ObjectMgr.h"
#include "Chat.h"
#include "Language.h"
#include "Arena.h"
#include "Player.h"
#include "ScriptMgr.h"

class arena_commandscript : public CommandScript
{
public:
    arena_commandscript() : CommandScript("arena_commandscript") { }

    ChatCommand* GetCommands() const OVERRIDE
    {
        static ChatCommand arenaCommandTable[] =
        {
            { "create",         rbac::RBAC_PERM_COMMAND_ARENA_CREATE,   true, &HandleArenaCreateCommand,   "", NULL },
            { "disband",        rbac::RBAC_PERM_COMMAND_ARENA_DISBAND,  true, &HandleArenaDisbandCommand,  "", NULL },
            { "rename",         rbac::RBAC_PERM_COMMAND_ARENA_RENAME,   true, &HandleArenaRenameCommand,   "", NULL },
            { "captain",        rbac::RBAC_PERM_COMMAND_ARENA_CAPTAIN, false, &HandleArenaCaptainCommand,  "", NULL },
            { "info",           rbac::RBAC_PERM_COMMAND_ARENA_INFO,     true, &HandleArenaInfoCommand,     "", NULL },
            { "lookup",         rbac::RBAC_PERM_COMMAND_ARENA_LOOKUP,  false, &HandleArenaLookupCommand,   "", NULL },
            { NULL, 0, false, NULL, "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "arena",          rbac::RBAC_PERM_COMMAND_ARENA,     false, NULL,                       "", arenaCommandTable },
            { NULL, 0, false, NULL, "", NULL }
        };
        return commandTable;
    }

    static bool HandleArenaCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // Deprecated.
        return false;
    }

    static bool HandleArenaDisbandCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // Deprecated.
        return false;
    }

    static bool HandleArenaRenameCommand(ChatHandler* handler, char const* _args)
    {
        if (!*_args)
            return false;

        // Deprecated.
        return false;
    }

    static bool HandleArenaCaptainCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // Deprecated.
        return false;
    }

    static bool HandleArenaInfoCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // Deprecated.
        return false;
    }

    static bool HandleArenaLookupCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // Deprecated.
        return false;
    }
};

void AddSC_arena_commandscript()
{
    new arena_commandscript();
}
