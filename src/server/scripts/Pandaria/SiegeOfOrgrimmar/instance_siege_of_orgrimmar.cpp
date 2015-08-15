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
 * Raid: Siege of Orgrimmar.
 * Description: Instance Script.
 */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Unit.h"
#include "Player.h"
#include "Map.h"
#include "PoolMgr.h"
#include "VMapFactory.h"
#include "AccountMgr.h"

#include "siege_of_orgrimmar.h"

DoorData const doorData[] =
{
    /*
    {GOB_STONE_GUARD_DOOR_ENTRANCE,          DATA_STONE_GUARD,          DOOR_TYPE_ROOM,       BOUNDARY_E   },
    {GOB_STONE_GUARD_DOOR_EXIT,              DATA_STONE_GUARD,          DOOR_TYPE_PASSAGE,    BOUNDARY_W   },
    {GOB_FENG_DOOR_FENCE,                    DATA_FENG,                 DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    {GOB_FENG_DOOR_EXIT,                     DATA_FENG,                 DOOR_TYPE_PASSAGE,    BOUNDARY_N   },
    {GOB_GARAJAL_FENCE,                      DATA_GARAJAL,              DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    {GOB_GARAJAL_EXIT,                       DATA_GARAJAL,              DOOR_TYPE_PASSAGE,    BOUNDARY_W   },
    {GOB_SPIRIT_KINGS_WIND_WALL,             DATA_SPIRIT_KINGS,         DOOR_TYPE_ROOM,       BOUNDARY_NONE},
    {GOB_SPIRIT_KINGS_EXIT,                  DATA_SPIRIT_KINGS,         DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GOB_CELESTIAL_DOOR,                     DATA_ELEGON,               DOOR_TYPE_ROOM,       BOUNDARY_E   },
    {GOB_ELEGON_DOOR_ENTRANCE,               DATA_SPIRIT_KINGS,         DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},
    {GOB_ELEGON_CELESTIAL_DOOR,              DATA_ELEGON,               DOOR_TYPE_ROOM,       BOUNDARY_E   },
    {GOB_WILL_OF_EMPEROR_ENTRANCE,           DATA_ELEGON,               DOOR_TYPE_PASSAGE,    BOUNDARY_NONE},*/
    {0,                                      0,                         DOOR_TYPE_ROOM,       BOUNDARY_NONE},// END
};

class instance_siege_of_orgrimmar : public InstanceMapScript
{
    public:
        instance_siege_of_orgrimmar() : InstanceMapScript("instance_siege_of_orgrimmar", 1008) { }

        struct instance_siege_of_orgrimmar_InstanceMapScript : public InstanceScript
        {
            instance_siege_of_orgrimmar_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                Initialize();
            }

            // First part
            uint64 immerseusGUID;
            uint64 fallenProtectorsGUID[MAX_FALLEN_PROTECTORS];
            uint64 norushenGUID;
            uint64 amalgamOfCorruptionGUID;
            uint64 shaOfPrideGUID;

            // Second part
            uint64 galakrasGUID;
            uint64 ironJuggernautGUID;
            uint64 korkronDarkShamansGUID[MAX_KORKRON_DARK_SHAMANS];
            uint64 generalNazgrimGUID;

            // Third part
            uint64 malkorokGUID;
            uint64 spoilsOfPandariaGUID;
            uint64 thokTheBloodthirstyGUID;

            // Last part
            uint64 siegecrafterBlackfuseGUID;
            uint64 paragonsOfTheKlaxxiGUID[MAX_PARAGONS_OF_THE_KLAXXI];
            uint64 garroshHellscreamGUID;

            void Initialize()
            {
                SetBossNumber(MAX_ENCOUNTERS);
                LoadDoorData(doorData);

                immerseusGUID                   = 0;

                for (uint8 i = 0; i < MAX_FALLEN_PROTECTORS; ++i)
                    fallenProtectorsGUID[i]    = 0;
                
                norushenGUID                    = 0;
                amalgamOfCorruptionGUID         = 0;
                shaOfPrideGUID                  = 0;
                galakrasGUID                    = 0;
                ironJuggernautGUID              = 0;

                for (uint8 i = 0; i < MAX_KORKRON_DARK_SHAMANS; ++i)
                    korkronDarkShamansGUID[i]   = 0;

                malkorokGUID                    = 0;
                spoilsOfPandariaGUID            = 0;
                thokTheBloodthirstyGUID         = 0;
                siegecrafterBlackfuseGUID       = 0;

                for (uint8 i = 0; i < MAX_PARAGONS_OF_THE_KLAXXI; ++i)
                    paragonsOfTheKlaxxiGUID[i]  = 0;

                garroshHellscreamGUID           = 0;

                for (uint32 i = 0; i < MAX_ENCOUNTERS; ++i)
                    SetBossState(i, NOT_STARTED);
            }

            bool IsEncounterInProgress() const
            {
                for (uint32 i = 0; i < MAX_ENCOUNTERS; ++i)
                    if (GetBossState(i) == IN_PROGRESS)
                        return true;

                return false;
            }

            void OnCreatureCreate(Creature* creature)
            {
                switch (creature->GetEntry())
                {
                    case DATA_IMMERSEUS:
                        immerseusGUID = creature->GetGUID();
                        break;
                    case DATA_FALLEN_PROTECTOR_1:
                        fallenProtectorsGUID[ROOK_STONETOE] = creature->GetGUID();
                        break;
                    case DATA_FALLEN_PROTECTOR_2:
                        fallenProtectorsGUID[HE_SOFTFOOT] = creature->GetGUID();
                        break;
                    case DATA_FALLEN_PROTECTOR_3:
                        fallenProtectorsGUID[SUN_TENDERHEART] = creature->GetGUID();
                        break;
                    case DATA_NORUSHEN:
                        norushenGUID = creature->GetGUID();
                        break;
                    case DATA_AMALGAM_OF_CORRUPTION:
                        amalgamOfCorruptionGUID = creature->GetGUID();
                        break;
                    case DATA_SHA_OF_PRIDE:
                        shaOfPrideGUID = creature->GetGUID();
                        break;
                    case DATA_GALAKRAS:
                        galakrasGUID = creature->GetGUID();
                        break;
                    case DATA_IRON_JUGGERNAUT:
                        ironJuggernautGUID = creature->GetGUID();
                        break;
                    case DATA_KORKRON_DARK_SHAMAN_1:
                        korkronDarkShamansGUID[EARTHBREAKER_HAROMM] = creature->GetGUID();
                        break;
                    case DATA_KORKRON_DARK_SHAMAN_2:
                        korkronDarkShamansGUID[WAVEBINDER_KARDIS] = creature->GetGUID();
                        break;
                    case DATA_GENERAL_NAZGRIM:
                        generalNazgrimGUID = creature->GetGUID();
                        break;
                    case DATA_MALKOROK:
                        malkorokGUID = creature->GetGUID();
                        break;
                    case DATA_SPOILS_OF_PANDARIA:
                        spoilsOfPandariaGUID = creature->GetGUID();
                        break;
                    case DATA_THOK_THE_BLOODTHIRSTY:
                        thokTheBloodthirstyGUID = creature->GetGUID();
                        break;
                    case DATA_SIEGECRAFTER_BLACKFUSE:
                        siegecrafterBlackfuseGUID = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_1:
                        paragonsOfTheKlaxxiGUID[KILRUK_THE_WIND_REAVER] = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_2:
                        paragonsOfTheKlaxxiGUID[XARIL_THE_POISONED_MIND] = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_3:
                        paragonsOfTheKlaxxiGUID[KAZTIK_THE_MANIPULATOR] = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_4:
                        paragonsOfTheKlaxxiGUID[KORVEN_THE_PRIME] = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_5:
                        paragonsOfTheKlaxxiGUID[IYYOKUK_THE_LUCID] = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_6:
                        paragonsOfTheKlaxxiGUID[KAROZ_THE_LOCUST] = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_7:
                        paragonsOfTheKlaxxiGUID[SKEER_THE_BLOODSEEKER] = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_8:
                        paragonsOfTheKlaxxiGUID[RIKKAL_THE_DISSECTOR] = creature->GetGUID();
                        break;
                    case DATA_PARAGONS_OF_THE_KLAX_9:
                        paragonsOfTheKlaxxiGUID[HISEK_THE_SWARMKEEPER] = creature->GetGUID();
                        break;
                    case DATA_GARROSH_HELLSCREAM:
                        garroshHellscreamGUID = creature->GetGUID();
                        break;

                    default: break;
                }
            }

            /*
            void OnUnitDeath(Unit* killed)
            {
                if (killed->GetTypeId() == TYPEID_PLAYER) return;

                switch(killed->ToCreature()->GetEntry())
                {
                    default: break;
                }
            }


            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    default: break;
                }
            }

            void OnGameObjectRemove(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    default: break;
                }
            }
            */

            void SetData(uint32 type, uint32 data)
            {
                SetBossState(type, EncounterState(data));

                if (data == DONE)
                    SaveToDB();
            }

            bool SetBossState(uint32 id, EncounterState state)
            {
                if (!InstanceScript::SetBossState(id, state))
                    return false;

                return true;
            }

            uint32 GetData(uint32 type)
            {
                return GetBossState(type);
            }

            uint64 GetData64(uint32 type)
            {
                switch (type)
                {
                    case DATA_IMMERSEUS:
                        return immerseusGUID;
                    case DATA_FALLEN_PROTECTOR_1:
                        return fallenProtectorsGUID[ROOK_STONETOE];
                    case DATA_FALLEN_PROTECTOR_2:
                        return fallenProtectorsGUID[HE_SOFTFOOT];
                    case DATA_FALLEN_PROTECTOR_3:
                        return fallenProtectorsGUID[SUN_TENDERHEART];
                    case DATA_NORUSHEN:
                        return norushenGUID;
                    case DATA_AMALGAM_OF_CORRUPTION:
                        return amalgamOfCorruptionGUID;
                    case DATA_SHA_OF_PRIDE:
                        return shaOfPrideGUID;
                    case DATA_GALAKRAS:
                        return galakrasGUID;
                    case DATA_IRON_JUGGERNAUT:
                        return ironJuggernautGUID;
                    case DATA_KORKRON_DARK_SHAMAN_1:
                        return korkronDarkShamansGUID[EARTHBREAKER_HAROMM];
                    case DATA_KORKRON_DARK_SHAMAN_2:
                        return korkronDarkShamansGUID[WAVEBINDER_KARDIS];
                    case DATA_GENERAL_NAZGRIM:
                        return generalNazgrimGUID;
                    case DATA_MALKOROK:
                        return malkorokGUID;
                    case DATA_SPOILS_OF_PANDARIA:
                        return spoilsOfPandariaGUID;
                    case DATA_THOK_THE_BLOODTHIRSTY:
                        return thokTheBloodthirstyGUID;
                    case DATA_SIEGECRAFTER_BLACKFUSE:
                        return siegecrafterBlackfuseGUID;
                    case DATA_PARAGONS_OF_THE_KLAX_1:
                        return paragonsOfTheKlaxxiGUID[KILRUK_THE_WIND_REAVER];
                    case DATA_PARAGONS_OF_THE_KLAX_2:
                        return paragonsOfTheKlaxxiGUID[XARIL_THE_POISONED_MIND];
                    case DATA_PARAGONS_OF_THE_KLAX_3:
                        return paragonsOfTheKlaxxiGUID[KAZTIK_THE_MANIPULATOR];
                    case DATA_PARAGONS_OF_THE_KLAX_4:
                        return paragonsOfTheKlaxxiGUID[KORVEN_THE_PRIME];
                    case DATA_PARAGONS_OF_THE_KLAX_5:
                        return paragonsOfTheKlaxxiGUID[IYYOKUK_THE_LUCID];
                    case DATA_PARAGONS_OF_THE_KLAX_6:
                        return paragonsOfTheKlaxxiGUID[KAROZ_THE_LOCUST];
                    case DATA_PARAGONS_OF_THE_KLAX_7:
                        return paragonsOfTheKlaxxiGUID[SKEER_THE_BLOODSEEKER];
                    case DATA_PARAGONS_OF_THE_KLAX_8:
                        return paragonsOfTheKlaxxiGUID[RIKKAL_THE_DISSECTOR];
                    case DATA_PARAGONS_OF_THE_KLAX_9:
                        return paragonsOfTheKlaxxiGUID[HISEK_THE_SWARMKEEPER];
                    case DATA_GARROSH_HELLSCREAM:
                        return garroshHellscreamGUID;

                    default: break;
                }

                return 0;
            }

            bool IsWipe()
            {
                Map::PlayerList const& PlayerList = instance->GetPlayers();

                if (PlayerList.isEmpty())
                    return true;

                for (Map::PlayerList::const_iterator Itr = PlayerList.begin(); Itr != PlayerList.end(); ++Itr)
                {
                    Player* player = Itr->getSource();

                    if (!player)
                        continue;

                    if (player->isAlive() && !player->isGameMaster())
                        return false;
                }

                return true;
            }

            /*
            void Update(uint32 diff)
            {
            }
            */

            bool CheckRequiredBosses(uint32 bossId, Player const* player = NULL) const
            {
                if (!InstanceScript::CheckRequiredBosses(bossId, player))
                    return false;

                switch (bossId)
                {
                    case DATA_GARROSH_HELLSCREAM_EVENT:
                    case DATA_PARAGONS_OF_THE_KLAXXI_EVENT:
                    case DATA_SIEGECRAFTER_BLACKFUSE_EVENT:
                    case DATA_THOK_THE_BLOODTHIRSTY_EVENT:
                    case DATA_SPOILS_OF_PANDARIA_EVENT:
                    case DATA_MALKOROK_EVENT:
                    case DATA_GENERAL_NAZGRIM_EVENT:
                    case DATA_KORKRON_DARK_SHAMANS_EVENT:
                    case DATA_IRON_JUGGERNAUT_EVENT:
                    case DATA_GALAKRAS_EVENT:
                    case DATA_SHA_OF_PRIDE_EVENT:
                    case DATA_NORUSHEN_EVENT:
                    case DATA_FALLEN_PROTECTORS_EVENT:
                        if (GetBossState(bossId - 1) != DONE)
                            return false;
                    default:
                        break;
                }

                return true;
            }

            std::string GetSaveData()
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "S O " << GetBossSaveData();

                OUT_SAVE_INST_DATA_COMPLETE;
                return saveStream.str();
            }

            void Load(const char* in)
            {
                if (!in)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(in);

                char dataHead1, dataHead2;

                std::istringstream loadStream(in);
                loadStream >> dataHead1 >> dataHead2;

                if (dataHead1 == 'S' && dataHead2 == 'O')
                {
                    for (uint32 i = 0; i < MAX_ENCOUNTERS; ++i)
                    {
                        uint32 tmpState;
                        loadStream >> tmpState;

                        if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                            tmpState = NOT_STARTED;

                        SetBossState(i, EncounterState(tmpState));
                    }

                } else OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_siege_of_orgrimmar_InstanceMapScript(map);
        }
};

void AddSC_instance_siege_of_orgrimmar()
{
    new instance_siege_of_orgrimmar();
}