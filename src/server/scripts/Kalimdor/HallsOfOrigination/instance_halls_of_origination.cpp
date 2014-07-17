/*Copyright (C) 2013 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "Map.h"
#include "PoolMgr.h"
#include "AccountMgr.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Unit.h"
#include "Player.h"

#include "halls_of_origination.h"

class instance_halls_of_origination : public InstanceMapScript
{
public:
    instance_halls_of_origination() : InstanceMapScript("instance_halls_of_origination", 644) { }

    struct instance_halls_of_origination_InstanceMapScript: public InstanceScript
    {
        instance_halls_of_origination_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            Initialize();
        }

        // Bosses.
        uint64 uiTempleGuardianAnhuur;
        uint64 uiEarthragerPtah;
        uint64 uiAnraphet;
        uint64 uiIsiset;
        uint64 uiAmmunae;
        uint64 uiSetesh;
        uint64 uiRajh;

        // GameObjects.
        uint64 OriginationElevatorGUID;
        uint64 uiAnhuurBridgeGUID;

        void Initialize()
        {
            SetBossNumber(MAX_ENCOUNTER);

            // Bosses.
            uiTempleGuardianAnhuur  = 0;
            uiEarthragerPtah        = 0;
            uiAnraphet              = 0;
            uiIsiset                = 0;
            uiAmmunae               = 0;
            uiSetesh                = 0;
            uiRajh                  = 0;

            // GameObjects.
            uiAnhuurBridgeGUID      = 0;
            // OriginationElevatorGUID = 0;

            for (uint32 i = 0; i < MAX_ENCOUNTER; ++i)
                SetBossState(i, NOT_STARTED);
        }

        bool IsEncounterInProgress() const
        {
            for (uint32 i = 0; i < MAX_ENCOUNTER; ++i)
                if (GetBossState(i) == IN_PROGRESS)
                    return true;

            return false;
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch(creature->GetEntry())
            {
                case BOSS_TEMPLE_GUARDIAN_ANHUUR:
                    uiTempleGuardianAnhuur = creature->GetGUID();
                    break;
                case BOSS_EARTHRAGER_PTAH:
                    uiEarthragerPtah = creature->GetGUID();
                    break;
                case BOSS_ANRAPHET:
                    uiAnraphet = creature->GetGUID();
                    break;
                case BOSS_ISISET:
                    uiIsiset = creature->GetGUID();
                    break;
                case BOSS_AMMUNAE:
                    uiAmmunae = creature->GetGUID();
                    break;
                case BOSS_SETESH:
                    uiSetesh = creature->GetGUID();
                    break;
                case BOSS_RAJH:
                    uiRajh = creature->GetGUID();
                    break;

                default: break;
            }
        }

        void OnUnitDeath(Unit* killed)
        {
            Creature* creature = killed->ToCreature();
            if (!creature)
                return;

            switch (creature->GetEntry())
            {
                case NPC_FLAME_WARDEN:
                    if (Creature* brann = creature->FindNearestCreature(39908, 500.0f, true))
                        brann->AI()->DoAction(2);
                    if (GameObject* laser1 = creature->FindNearestGameObject(207663, 1000.0f))
                        laser1->SetGoState(GO_STATE_ACTIVE);
                    if (GameObject* machine1 = creature->FindNearestGameObject(207375, 1000.0f))
                        machine1->SetGoState(GO_STATE_ACTIVE);
                    break;
                case NPC_EARTH_WARDEN:
                    if (Creature* brann = creature->FindNearestCreature(39908, 500.0f, true))
                        brann->AI()->DoAction(2);
                    if (GameObject* laser3 = creature->FindNearestGameObject(207665, 1000.0f))
                        laser3->SetGoState(GO_STATE_ACTIVE);
                    if (GameObject* machine3 = creature->FindNearestGameObject(207377, 1000.0f))
                        machine3->SetGoState(GO_STATE_ACTIVE);
                    break;
                case NPC_WATER_WARDEN:
                    if (Creature* brann = creature->FindNearestCreature(39908, 500.0f, true))
                        brann->AI()->DoAction(2);
                    if (GameObject* laser4 = creature->FindNearestGameObject(207664, 1000.0f))
                        laser4->SetGoState(GO_STATE_ACTIVE);
                    if (GameObject* machine4 = creature->FindNearestGameObject(207376, 1000.0f))
                        machine4->SetGoState(GO_STATE_ACTIVE);
                    break;
                case NPC_AIR_WARDEN:
                    if (Creature* brann = creature->FindNearestCreature(39908, 500.0f, true))
                        brann->AI()->DoAction(2);
                    if (GameObject* laser2 = creature->FindNearestGameObject(207662, 1000.0f))
                        laser2->SetGoState(GO_STATE_ACTIVE);
                    if (GameObject* machine2 = creature->FindNearestGameObject(207374, 1000.0f))
                        machine2->SetGoState(GO_STATE_ACTIVE);
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_ANHUUR_BRIDGE:
                    uiAnhuurBridgeGUID = go->GetGUID();
                    break;
                case GO_ORIGINATION_ELEVATOR:
                    OriginationElevatorGUID = go->GetGUID();
                    break;

                default: break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            SetBossState(type, EncounterState(data));

            if (data == DONE)
                SaveToDB();
        }

        uint32 GetData(uint32 type) const OVERRIDE
        {
            return GetBossState(type);
        }

        uint64 GetData64(uint32 identifier) const OVERRIDE
        {
            switch(identifier)
            {
                // Bosses.
                case DATA_TEMPLE_GUARDIAN_ANHUUR: return uiTempleGuardianAnhuur;  break;
                case DATA_EARTHRAGER_PTAH:        return uiEarthragerPtah;        break;
                case DATA_ANRAPHET:               return uiAnraphet;              break;
                case DATA_ISISET:                 return uiIsiset;                break;
                case DATA_AMMUNAE:                return uiAmmunae;               break;
                case DATA_SETESH:                 return uiSetesh;                break;
                case DATA_RAJH:                   return uiRajh;                  break;

                // GameObjects.
                case DATA_ANHUUR_BRIDGE:          return uiAnhuurBridgeGUID;      break;
                case DATA_ORIGINATION_ELEVATOR:   return OriginationElevatorGUID; break;

                default:                          return 0;                       break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            if (state == DONE)
            {
                switch(type)
                {
                    case DATA_TEMPLE_GUARDIAN_ANHUUR_EVENT:
                    case DATA_EARTHRAGER_PTAH_EVENT:
                    case DATA_ANRAPHET_EVENT:
                    case DATA_ISISET_EVENT:
                    case DATA_AMMUNAE_EVENT:
                    case DATA_SETESH_EVENT:
                    case DATA_RAJH_EVENT:
                    break;
                }
            }

            return true;
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << "H O " << GetBossSaveData();

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

            if (dataHead1 == 'H' && dataHead2 == 'O')
            {
                for (uint32 i = 0; i < MAX_ENCOUNTER; ++i)
                {
                    uint32 tmpState;
                    loadStream >> tmpState;

                    if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                        tmpState = NOT_STARTED;

                    // Below makes the player on-instance-entry display of bosses killed shit work (SMSG_RAID_INSTANCE_INFO).
                    // Like, say an unbound player joins the party and he tries to enter the dungeon / raid.
                    // This makes sure binding-to-instance-on-entrance confirmation box will properly display bosses defeated / available.
                    SetBossState(i, EncounterState(tmpState));
                }

            } else OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_halls_of_origination_InstanceMapScript(map);
    }
};

void AddSC_instance_halls_of_origination()
{
    new instance_halls_of_origination();
}
