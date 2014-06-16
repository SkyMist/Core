/*Copyright (C) 2014 Buli.
*
* This file is NOT free software. Third-party users may NOT redistribute it or modify it :).
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
#include "AccountMgr.h"

#include "heart_of_fear.h"

class instance_heart_of_fear: public InstanceMapScript 
{
public:
	instance_heart_of_fear() : InstanceMapScript("instance_heart_of_fear", 1009) { }

	struct instance_heart_of_fear_InstanceMapScript: public InstanceScript
    {
		instance_heart_of_fear_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            Initialize();
        }

        // Bosses.
		uint64 uiGrandVizierZorlok;
		uint64 uiBladeLordTayak;
		uint64 uiGaralon;
		uint64 uiWindLordMeljarak;
		uint64 uiAmberShaperUnsok;
        uint64 uiGrandEmpressShekzeer;

		void Initialize() 
        {
            SetBossNumber(MAX_ENCOUNTERS);

            // Bosses.
			uiGrandVizierZorlok     = 0;
			uiBladeLordTayak        = 0;
			uiGaralon               = 0;
			uiWindLordMeljarak      = 0;
			uiAmberShaperUnsok      = 0;
            uiGrandEmpressShekzeer  = 0;

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
                // Bosses.
			    case BOSS_GRAND_VIZIER_ZORLOK:
				    uiGrandVizierZorlok = creature->GetGUID();
				    break;
			    case BOSS_BLADE_LORD_TAYAK:
				    uiBladeLordTayak = creature->GetGUID();
				    break;
			    case BOSS_GARALON:
				    uiGaralon = creature->GetGUID();
				    break;
			    case BOSS_WIND_LORD_MELJARAK:
				    uiWindLordMeljarak = creature->GetGUID();
				    break;
			    case BOSS_AMBER_SHAPER_UNSOK:
				    uiAmberShaperUnsok = creature->GetGUID();
				    break;
			    case BOSS_GRAND_EMPRESS_SHEKZEER:
				    uiGrandEmpressShekzeer = creature->GetGUID();
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
            }
        }

        void OnGameObjectCreate(GameObject* go) 
        {
            switch (go->GetEntry())
            {
            }
        }

        void OnGameObjectRemove(GameObject* go)
        {
            switch (go->GetEntry())
            {
            }
        }
*/

		void SetData(uint32 type, uint32 data)
        {
            SetBossState(type, EncounterState(data));

			if (data == DONE)
				SaveToDB();
		}

        bool SetBossState(uint32 data, EncounterState state)
        {
            if (!InstanceScript::SetBossState(data, state))
                return false;

            if (state == DONE)
            {
                switch(data)
                {
                    case DATA_VIZIER_ZORLOK_EVENT:
                    case DATA_BLADE_LORD_TAYAK_EVENT:
                    case DATA_GARALON_EVENT:
                    case DATA_WIND_LORD_MELJARAK_EVENT:
                    case DATA_AMBER_SHAPER_UNSOK_EVENT:
                    case DATA_EMPRESS_SHEKZEER_EVENT:
                    break;
                }
            }

            return true;
        }

		uint32 GetData(uint32 type) const OVERRIDE
        {
			return GetBossState(type);
		}

		uint64 getData64(uint32 data) const OVERRIDE
        {
			switch (data)
            {
                // Bosses.
			    case DATA_VIZIER_ZORLOK:      return uiGrandVizierZorlok;     break;
			    case DATA_BLADE_LORD_TAYAK:   return uiBladeLordTayak;        break;
			    case DATA_GARALON:            return uiGaralon;               break;
			    case DATA_WIND_LORD_MELJARAK: return uiWindLordMeljarak;      break;
			    case DATA_AMBER_SHAPER_UNSOK: return uiAmberShaperUnsok;      break;
			    case DATA_EMPRESS_SHEKZEER:   return uiGrandEmpressShekzeer;  break;

                default:                      return 0;                       break;
			}
		}

		std::string GetSaveData() 
        {
			OUT_SAVE_INST_DATA;

			std::ostringstream saveStream;
			saveStream << "H F " << GetBossSaveData();

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

			if (dataHead1 == 'H' && dataHead2 == 'F') 
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
		return new instance_heart_of_fear_InstanceMapScript(map);
	}
};

void AddSC_instance_heart_of_fear()
{
	new instance_heart_of_fear();
}
