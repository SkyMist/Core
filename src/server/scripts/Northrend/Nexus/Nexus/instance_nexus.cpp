/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#include "ScriptPCH.h"
#include "nexus.h"

class instance_nexus : public InstanceMapScript
{
    public:
        instance_nexus() : InstanceMapScript(NexusScriptName, 576) { }

        struct instance_nexus_InstanceScript : public InstanceScript
        {
            instance_nexus_InstanceScript(Map* map) : InstanceScript(map)
            {
                SetBossNumber(MAX_ENCOUNTER);
                _anomalusGUID = 0;
                _magusTelestraGUID = 0;
                _ormorokGUID = 0;
                _keristraszaGUID = 0;
                _anomalusContainmentSphereGUID = 0;
                _ormoroksContainmentSphereGUID = 0;
                _telestrasContainmentSphereGUID = 0;
                _teamInInstance = 0;
            }

            void OnPlayerEnter(Player* player)
            {
                if (!_teamInInstance)
                    _teamInInstance = player->GetTeam();
            }

            void OnCreatureCreate(Creature* creature)
            {
                if (!_teamInInstance)
                {
                    Map::PlayerList const &players = instance->GetPlayers();
                    if (!players.isEmpty())
                        if (Player* player = players.begin()->getSource())
                            _teamInInstance = player->GetTeam();
                }

                switch (creature->GetEntry())
                {
                    case NPC_GRAND_MAGUS_TELESTRA:
                        _magusTelestraGUID = creature->GetGUID();
                        break;
                    case NPC_ANOMALUS:
                        _anomalusGUID = creature->GetGUID();
                        break;
                    case NPC_ORMOROK:
                        _ormorokGUID = creature->GetGUID();
                        break;
                    case NPC_KERISTRASZA:
                        _keristraszaGUID = creature->GetGUID();
                        break;
                    case NPC_ALLIANCE_BERSERKER:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_HORDE_BERSERKER, HORDE);
                        break;
                    case NPC_ALLIANCE_RANGER:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_HORDE_RANGER, HORDE);
                        break;
                    case NPC_ALLIANCE_CLERIC:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_HORDE_CLERIC, HORDE);
                        break;
                    case NPC_ALLIANCE_COMMANDER:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_HORDE_COMMANDER, HORDE);
                        break;
                    case NPC_COMMANDER_STOUTBEARD:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_COMMANDER_KOLURG, HORDE);
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GO_TELESTRAS_CONTAINMENT_SPHERE:
                        _telestrasContainmentSphereGUID = go->GetGUID();
                        if (GetBossState(DATA_GRAND_MAGUS_TELESTRA) == DONE)
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    case GO_ANOMALUS_CONTAINMENT_SPHERE:
                        _anomalusContainmentSphereGUID = go->GetGUID();
                        if (GetBossState(DATA_ANOMALUS) == DONE)
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    case GO_ORMOROKS_CONTAINMENT_SPHERE:
                        _ormoroksContainmentSphereGUID = go->GetGUID();
                        if (GetBossState(DATA_ORMOROK) == DONE)
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    default:
                        break;
                }
            }

            bool SetBossState(uint32 type, EncounterState state)
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
                    case DATA_GRAND_MAGUS_TELESTRA:
                        if (state == DONE)
                            if (GameObject* sphere = instance->GetGameObject(_telestrasContainmentSphereGUID))
                                sphere->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    case DATA_ANOMALUS:
                        if (state == DONE)
                            if (GameObject* sphere = instance->GetGameObject(_anomalusContainmentSphereGUID))
                                sphere->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    case DATA_ORMOROK:
                        if (state == DONE)
                            if (GameObject* sphere = instance->GetGameObject(_ormoroksContainmentSphereGUID))
                                sphere->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        break;
                    default:
                        break;
                }

                return true;
            }

            uint64 GetData64(uint32 type)
            {
                switch (type)
                {
                    case DATA_ANOMALUS:
                        return _anomalusGUID;
                    case DATA_GRAND_MAGUS_TELESTRA:
                        return _magusTelestraGUID;
                    case DATA_ORMOROK:
                        return _ormorokGUID;
                    case DATA_KERISTRASZA:
                        return _keristraszaGUID;
                    case DATA_ANOMALUS_CONTAINMET_SPHERE:
                        return _anomalusContainmentSphereGUID;
                    case DATA_ORMOROKS_CONTAINMET_SPHERE:
                        return _ormoroksContainmentSphereGUID;
                    case DATA_TELESTRAS_CONTAINMET_SPHERE:
                        return _telestrasContainmentSphereGUID;
                    default:
                        break;
                }

                return 0;
            }

            std::string GetSaveData()
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "T N " << GetBossSaveData();

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

                if (dataHead1 == 'T' && dataHead2 == 'N')
                {
                    for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                    {
                        uint32 tmpState;
                        loadStream >> tmpState;
                        if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                            tmpState = NOT_STARTED;

                        SetBossState(i, EncounterState(tmpState));
                    }
                }
                else
                    OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }

        private:
            uint64 _anomalusGUID;
            uint64 _magusTelestraGUID;
            uint64 _ormorokGUID;
            uint64 _keristraszaGUID;
            uint64 _anomalusContainmentSphereGUID;
            uint64 _ormoroksContainmentSphereGUID;
            uint64 _telestrasContainmentSphereGUID;

            uint32 _teamInInstance;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_nexus_InstanceScript(map);
        }
};

void AddSC_instance_nexus()
{
    new instance_nexus();
}
