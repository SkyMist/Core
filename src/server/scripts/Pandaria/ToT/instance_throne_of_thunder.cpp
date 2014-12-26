/*
*
* SkyMist Gaming © says just enjoy the script. It is not free to use and under copyright law so if you are an unauthorised third party we'll just sew your ass.
*
* Raid: Throne of Thunder.
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
#include "AccountMgr.h"

#include "throne_of_thunder.h"

class instance_throne_of_thunder : public InstanceMapScript
{
    public:
        instance_throne_of_thunder() : InstanceMapScript("instance_throne_of_thunder", 1098) { }

        struct instance_throne_of_thunder_InstanceMapScript : public InstanceScript
        {
            instance_throne_of_thunder_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                Initialize();
            }

            // creatures
            uint64 jinrokhBreakerGUID;
            uint64 horridonGUID;
            uint64 kazrajinGUID;
            uint64 sulTheSandCrawlerGUID;
            uint64 frostKingMalakkGUID;
            uint64 highPriestessMarliGUID;
            uint64 tortosGUID;
            uint64 megaeraGUID;
            uint64 jiKunGUID;
            uint64 durumuTheForgottenGUID;
            uint64 primordiusGUID;
            uint64 darkAnimusGUID;
            uint64 ironQonGUID;
            uint64 luLinGUID;
            uint64 suenGUID;
            uint64 leiShenGUID;
            uint64 raDenGUID;

            // gameObjects
            uint64 firstDoorguid;
            uint64 moguFountainNEguid;
            uint64 moguFountainNWguid;
            uint64 moguFountainSEguid;
            uint64 moguFountainSWguid;
            uint64 jinRokhfrontDoor;
            uint64 jinRokhbackDoor;

            void Initialize()
            {
                SetBossNumber(MAX_ENCOUNTERS);

                // creatures
                jinrokhBreakerGUID     = 0;
                horridonGUID           = 0;
                kazrajinGUID           = 0;
                sulTheSandCrawlerGUID  = 0;
                frostKingMalakkGUID    = 0;
                highPriestessMarliGUID = 0;
                tortosGUID             = 0;
                megaeraGUID            = 0;
                jiKunGUID              = 0;
                durumuTheForgottenGUID = 0;
                primordiusGUID         = 0;
                darkAnimusGUID         = 0;
                ironQonGUID            = 0;
                luLinGUID              = 0;
                suenGUID               = 0;
                leiShenGUID            = 0;
                raDenGUID              = 0;

               // gameObjects
               firstDoorguid           = 0;
               moguFountainNEguid      = 0;
               moguFountainNWguid      = 0;
               moguFountainSEguid      = 0;
               moguFountainSWguid      = 0;
               jinRokhfrontDoor        = 0;
               jinRokhbackDoor         = 0;

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
                    case DATA_JIN_ROKH_BREAKER:
                        jinrokhBreakerGUID = creature->GetGUID();
                        break;
                    case DATA_HORRIDON:
                        horridonGUID = creature->GetGUID();
                        break;
                    case DATA_KAZRAJIN:
                        kazrajinGUID = creature->GetGUID();
                        break;
                    case DATA_SUL_THE_SANDCRAWLER:
                        sulTheSandCrawlerGUID = creature->GetGUID();
                        break;
                    case DATA_FROST_KING_MALAKK:
                        frostKingMalakkGUID = creature->GetGUID();
                        break;
                    case DATA_HIGH_PRIESTESS_MARLI:
                        highPriestessMarliGUID = creature->GetGUID();
                        break;
                    case DATA_TORTOS:
                        tortosGUID = creature->GetGUID();
                        break;
                    case DATA_MEGAERA:
                        megaeraGUID = creature->GetGUID();
                        break;
                    case DATA_JI_KUN:
                        jiKunGUID = creature->GetGUID();
                        break;
                    case DATA_DURUMU_THE_FORGOTTEN:
                        durumuTheForgottenGUID = creature->GetGUID();
                        break;
                    case DATA_PRIMORDIUS:
                        primordiusGUID = creature->GetGUID();
                        break;
                    case DATA_DARK_ANIMUS:
                        darkAnimusGUID = creature->GetGUID();
                        break;
                    case DATA_IRON_QON:
                        ironQonGUID = creature->GetGUID();
                        break;
                    case DATA_LU_LIN:
                        luLinGUID = creature->GetGUID();
                        break;
                    case DATA_SUEN:
                        suenGUID = creature->GetGUID();
                        break;
                    case DATA_LEI_SHEN:
                        leiShenGUID = creature->GetGUID();
                        break;
                    case DATA_RA_DEN:
                        raDenGUID = creature->GetGUID();
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
            */

            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case DATA_FIRST_DOOR:
                        firstDoorguid = go->GetGUID();
                        break;
                    case DATA_MOGU_FOUNTAIN_NE:
                        moguFountainNEguid = go->GetGUID();
                        break;
                    case DATA_MOGU_FOUNTAIN_NW:
                        moguFountainNWguid = go->GetGUID();
                        break;
                    case DATA_MOGU_FOUNTAIN_SE:
                        moguFountainSEguid = go->GetGUID();
                        break;
                    case DATA_MOGU_FOUNTAIN_SW:
                        moguFountainSWguid = go->GetGUID();
                        break;
                    case DATA_JIN_ROKH_FRONT_DOOR:
                        jinRokhfrontDoor = go->GetGUID();
                        break;
                    case DATA_JIN_ROKH_BACK_DOOR:
                        jinRokhbackDoor = go->GetGUID();
						break;

                    default: break;
                }
            }

            /*
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
                        case DATA_JIN_ROKH_BREAKER_EVENT:
                        case DATA_HORRIDON_EVENT:
                        case DATA_COUNCIL_OF_ELDERS_EVENT:
                        case DATA_TORTOS_EVENT:
                        case DATA_MEGAERA_EVENT:
                        case DATA_JI_KUN_EVENT:
                        case DATA_DURUMU_THE_FORGOTTEN_EVENT:
                        case DATA_PRIMORDIUS_EVENT:
                        case DATA_DARK_ANIMUS_EVENT:
                        case DATA_IRON_QON_EVENT:
                        case DATA_TWIN_CONSORTS_EVENT:
                        case DATA_LEI_SHEN_EVENT:
                        case DATA_RA_DEN_EVENT:
                        break;
                    }
                }

                return true;
            }

		    uint32 GetData(uint32 type)
            {
		    	return GetBossState(type);
		    }

            uint64 GetData64(uint32 type) const
            {
                switch (type)
                {
                    case DATA_JIN_ROKH_BREAKER:     return jinrokhBreakerGUID;     break;
                    case DATA_HORRIDON:             return horridonGUID;           break;
                    case DATA_KAZRAJIN:             return kazrajinGUID;           break;
                    case DATA_SUL_THE_SANDCRAWLER:  return sulTheSandCrawlerGUID;  break;
                    case DATA_FROST_KING_MALAKK:    return frostKingMalakkGUID;    break;
                    case DATA_HIGH_PRIESTESS_MARLI: return highPriestessMarliGUID; break;
                    case DATA_TORTOS:               return tortosGUID;             break;
                    case DATA_MEGAERA:              return megaeraGUID;            break;
                    case DATA_JI_KUN:               return jiKunGUID;              break;
                    case DATA_DURUMU_THE_FORGOTTEN: return durumuTheForgottenGUID; break;
                    case DATA_PRIMORDIUS:           return primordiusGUID;         break;
                    case DATA_DARK_ANIMUS:          return darkAnimusGUID;         break;
                    case DATA_IRON_QON:             return ironQonGUID;            break;
                    case DATA_LU_LIN:               return luLinGUID;              break;
                    case DATA_SUEN:                 return suenGUID;               break;
                    case DATA_LEI_SHEN:             return leiShenGUID;            break;
                    case DATA_RA_DEN:               return raDenGUID;              break;

                    case DATA_FIRST_DOOR:           return firstDoorguid;          break;
                    case DATA_MOGU_FOUNTAIN_NE:     return moguFountainNEguid;     break;
                    case DATA_MOGU_FOUNTAIN_NW:     return moguFountainNWguid;     break;
                    case DATA_MOGU_FOUNTAIN_SE:     return moguFountainSEguid;     break;
                    case DATA_MOGU_FOUNTAIN_SW:     return moguFountainSWguid;     break;
                    case DATA_JIN_ROKH_FRONT_DOOR:  return jinRokhfrontDoor;       break;
                    case DATA_JIN_ROKH_BACK_DOOR:   return jinRokhbackDoor;        break;

                    default:                        return 0;                      break;
                }
            }

		    std::string GetSaveData() 
            {
		    	OUT_SAVE_INST_DATA;

		    	std::ostringstream saveStream;
		    	saveStream << "T T " << GetBossSaveData();

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

		    	if (dataHead1 == 'T' && dataHead2 == 'T') 
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
            return new instance_throne_of_thunder_InstanceMapScript(map);
        }
};

void AddSC_instance_throne_of_thunder()
{
    new instance_throne_of_thunder();
}
