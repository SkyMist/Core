/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#ifndef DEF_NEXUS_H
#define DEF_NEXUS_H

#define NexusScriptName     "instance_nexus"

#define MAX_ENCOUNTER      4

enum DataTypes
{
    DATA_GRAND_MAGUS_TELESTRA           = 0,
    DATA_ANOMALUS                       = 1,
    DATA_ORMOROK                        = 2,
    DATA_KERISTRASZA                    = 3,
    DATA_COMMANDER                      = 4,

    DATA_ANOMALUS_CONTAINMET_SPHERE     = 5,
    DATA_ORMOROKS_CONTAINMET_SPHERE     = 6,
    DATA_TELESTRAS_CONTAINMET_SPHERE    = 7,
    
    DATA_CHAOS_THEORY                   = 8
};

enum CreatureIds
{
    MOB_CHAOTIC_RIFT                    = 26918,
    MOB_CRAZED_MANA_WRAITH              = 26746,
    
    NPC_ANOMALUS                        = 26763,
    NPC_GRAND_MAGUS_TELESTRA            = 26731,
    NPC_ORMOROK                         = 26794,
    NPC_KERISTRASZA                     = 26723,
    NPC_ALLIANCE_BERSERKER              = 26800,
    NPC_HORDE_BERSERKER                 = 26799,
    NPC_ALLIANCE_RANGER                 = 26802,
    NPC_HORDE_RANGER                    = 26801,
    NPC_ALLIANCE_CLERIC                 = 26805,
    NPC_HORDE_CLERIC                    = 26803,
    NPC_ALLIANCE_COMMANDER              = 27949,
    NPC_HORDE_COMMANDER                 = 27947,
    NPC_COMMANDER_STOUTBEARD            = 26796,
    NPC_COMMANDER_KOLURG                = 26798,
};

enum GameObjectIds
{
    GO_TELESTRAS_CONTAINMENT_SPHERE     = 188526,
    GO_ANOMALUS_CONTAINMENT_SPHERE      = 188527,
    GO_ORMOROKS_CONTAINMENT_SPHERE      = 188528,
};

#endif
