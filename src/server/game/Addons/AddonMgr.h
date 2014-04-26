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

#ifndef _ADDONMGR_H
#define _ADDONMGR_H

#include "Define.h"
#include <string>
#include <list>

enum AddonStates // Needs a check.
{
    ADDON_STATE_NONE     = 0,

    ADDON_STATE_DISABLED = 1,
    ADDON_STATE_ENABLED  = 2
};

struct AddonInfo
{
    AddonInfo(const std::string& name, uint8 hasPublicKey, uint32 publicKeyCRC, uint32 urlCRC, uint8 state)
        : Name(name), HasPublicKey(hasPublicKey), PublicKeyCRC(publicKeyCRC), UrlCRC(urlCRC), State(state) { }

    std::string Name;
    uint8 HasPublicKey;
    uint32 PublicKeyCRC;
    uint32 UrlCRC;
    uint8 State;
};

struct SavedAddon
{
    SavedAddon(std::string const& name, uint32 crc) : Name(name)
    {
        CRC = crc;
    }

    std::string Name;
    uint32 CRC;
};

struct BannedAddon
{
    uint32 Id;
    uint8 NameMD5[16];
    uint8 VersionMD5[16];
    uint32 Timestamp;
};

#define STANDARD_ADDON_CRC 0x60687BAB

namespace AddonMgr
{
    void LoadFromDB();
    void SaveAddon(AddonInfo const& addon);
    SavedAddon const* GetAddonInfo(const std::string& name);

    typedef std::list<BannedAddon> BannedAddonList;
    BannedAddonList const* GetBannedAddons();
}

#endif
