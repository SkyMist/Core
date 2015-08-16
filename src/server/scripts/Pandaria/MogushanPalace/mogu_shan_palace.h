/*
 * Copyright (C) 2011-2015 SkyMist Gaming
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
 *
 * Dungeon: Mogu'shan Palace.
 * Description: Header Script.
 */

#ifndef MOGUSHAN_PALACE_H_
#define MOGUSHAN_PALACE_H_

#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"

#define MAX_ENCOUNTERS 3

enum DataTypes // Events / Encounters.
{
    DATA_TRIAL_OF_THE_KING_EVENT            = 0,
    DATA_GEKKAN_EVENT                       = 1,
    DATA_XIN_THE_WEAPONMASTER_EVENT         = 2,

    DATA_GEKKAN_ADDS
};

enum Data     // GUID handling.
{
	DATA_TRIAL_OF_THE_KING                  = 0,
    DATA_GEKKAN                             = 1,
    DATA_XIN_THE_WEAPONMASTER               = 2
};

enum SharedSpells
{
    // Xin the Weaponmaster.
    SPELL_THROW_DAMAGE                      = 119311,
    SPELL_THROW_AURA                        = 119315,
    SPELL_PERMANENT_FEIGN_DEATH             = 130966,
    SPELL_AXE_TOURBILOL                     = 119373,

    // Gurthan scrapper, harthak adept and kargesh grunt.
    SPELL_GRUNT_AURA                        = 121746
};

enum CreaturesIds
{
    //Boss
    BOSS_KUAI_THE_BRUTE                     = 61442,
    BOSS_MING_THE_CUNNING                   = 61444,
    BOSS_HAIYAN_THE_UNSTOPPABLE             = 61445,
    CREATURE_XIN_THE_WEAPONMASTER_TRIGGER   = 61884,
    BOSS_XIN_THE_WEAPONMASTER               = 61398,

    //Trash
    CREATURE_GURTHAN_SCRAPPER               = 61447,
    CREATURE_HARTHAK_ADEPT                  = 61449,
    CREATURE_KARGESH_GRUNT                  = 61450,
    //Trigger
    CREATURE_WHIRLING_DERVISH               = 61626,

    BOSS_GEKKAN                             = 61243,
    CREATURE_GLINTROK_IRONHIDE              = 61337,
    CREATURE_GLINTROK_SKULKER               = 61338,
    CREATURE_GLINTROK_ORACLE                = 61339,
    CREATURE_GLINTROK_HEXXER                = 61340,

    //XIN THE WEAPONMASTER
    CREATURE_ANIMATED_STAFF                 = 61433,
    CREATURE_ANIMATED_AXE                   = 61451,
    CREATURE_LAUNCH_SWORD                   = 63808
};

enum GameObjects
{
    GO_DOOR_BEFORE_TRIAL                    = 213594,
    GO_TRIAL_CHEST                          = 214520,
    GO_DOOR_AFTER_TRIAL                     = 213593,
    GO_DOOR_BEFORE_KING                     = 213596
};

enum Types
{
    TYPE_MING_ATTACK                        = 0,
    TYPE_KUAI_ATTACK,
    TYPE_HAIYAN_ATTACK,
    TYPE_ALL_ATTACK,

    TYPE_MING_RETIRED,
    TYPE_KUAI_RETIRED,
    TYPE_HAIYAN_RETIRED,

    TYPE_WIPE_FIRST_BOSS,

    TYPE_MING_INTRO,
    TYPE_OUTRO_01,
    TYPE_OUTRO_02,
    TYPE_OUTRO_03,
    TYPE_OUTRO_04,
    TYPE_OUTRO_05,

    TYPE_GET_ENTOURAGE_0,
    TYPE_GET_ENTOURAGE_1,
    TYPE_GET_ENTOURAGE_2,
    TYPE_GET_ENTOURAGE_3,
    
    TYPE_ACTIVATE_ANIMATED_STAFF,
    TYPE_ACTIVATE_ANIMATED_AXE,
    TYPE_ACTIVATE_SWORD
};

#endif // MOGUSHAN_PALACE_H_
