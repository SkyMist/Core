/*
    Dungeon : Stormstout Brewery 85-87
    Instance General Script
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "stormstout_brewery.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Unit.h"
#include "Player.h"

class instance_stormstout_brewery : public InstanceMapScript
{
    public:
        instance_stormstout_brewery() : InstanceMapScript("instance_stormstout_brewery", 961) { }

        struct instance_stormstout_brewery_InstanceMapScript : public InstanceScript
        {
            instance_stormstout_brewery_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                Initialize();
            }

            // Bosses.
            uint64 uiOokOok;
            uint64 uiHoptallus;
            uint64 uiYanzhuTheUncasked;
            bool OokOokSummoned;
            bool HoptallusSummoned;

            void Initialize()
            {
                SetBossNumber(MAX_ENCOUNTERS);

                // Bosses.
                uiOokOok = 0;
                uiHoptallus = 0;
                uiYanzhuTheUncasked = 0;
                OokOokSummoned = false;
                HoptallusSummoned = false;

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
                switch(creature->GetEntry())
                {
                    // Bosses.
                    case BOSS_OOKOOK:
                        uiOokOok = creature->GetGUID();
                        break;
                    case BOSS_HOPTALLUS:
                        uiHoptallus = creature->GetGUID();
                        break;
                    case BOSS_YANZHU_THE_UNCASKED:
                        uiYanzhuTheUncasked = creature->GetGUID();
                        break;

                    // NPCs
                    case NPC_ANCESTRAL_BREWMASTER:
                        creature->AddAura(SPELL_ANCESTRAL_BREWM_V, creature);
                        break;

                    default: break;
                }
            }

            void OnUnitDeath(Unit* killed)
            {
                if (killed->GetTypeId() == TYPEID_PLAYER) return;

                switch(killed->ToCreature()->GetEntry())
                {
                    // Script for Ook-ook summon bar.
                    case NPC_SODDEN_HOZEN_BRAWLER:
                    case NPC_INFLAMED_HOZEN_BRAWLER:
                    case NPC_SLEEPY_HOZEN_BRAWLER:
                    case NPC_DRUNKEN_HOZEN_BRAWLER:
                    case NPC_HOZEN_BOUNCER:
                    case NPC_HOZEN_PARTY_ANIMAL:
						if (Player* player = killed->FindNearestPlayer(20.0f))
                        {
                            if (player->GetGroup())
                            {
                                if (Player* Leader = ObjectAccessor::FindPlayer(player->GetGroup()->GetLeaderGUID()))
                                {
                                    if (Aura* bananas = Leader->GetAura(SPELL_BANANA_BAR))
                                    {
                                        if (Leader->GetPower(POWER_ALTERNATE_POWER) + 1 < 40) // We check the leader's power in advance to summon Ook-Ook at right value.
                                        {
                                            // Update Leader power.
                                            Leader->SetPower(POWER_ALTERNATE_POWER, Leader->GetPower(POWER_ALTERNATE_POWER) + 1);

                                            // Update group members.
                                            for (GroupReference* itr = Leader->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next())
                                                if (Player* member = itr->GetSource())
                                                    if (member != Leader)
                                                        member->SetPower(POWER_ALTERNATE_POWER, Leader->GetPower(POWER_ALTERNATE_POWER));
                                        }
                                        else
                                        {
                                            if (!OokOokSummoned)
                                            {
                                                Leader->SummonCreature(BOSS_OOKOOK, ookOokSummonPosition, TEMPSUMMON_MANUAL_DESPAWN);

                                                // Update group members and remove aura.
                                                for (GroupReference* itr = Leader->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next())
                                                    if (Player* member = itr->GetSource())
                                                        member->RemoveAurasDueToSpell(SPELL_BANANA_BAR);

                                                OokOokSummoned = true;
                                            }
                                        }
                                    }
                                }
                            }
                            else // Solo.
                            {
                                if (Aura* bananas = player->GetAura(SPELL_BANANA_BAR))
                                {
                                    if (player->GetPower(POWER_ALTERNATE_POWER) + 1 < 40) // We check the player's power in advance to summon Ook-Ook at right value.
                                        player->SetPower(POWER_ALTERNATE_POWER, player->GetPower(POWER_ALTERNATE_POWER) + 1); // // Update player power.
                                    else
                                    {
                                        if (!OokOokSummoned)
                                        {
                                            player->SummonCreature(BOSS_OOKOOK, ookOokSummonPosition, TEMPSUMMON_MANUAL_DESPAWN);
                                            player->RemoveAurasDueToSpell(SPELL_BANANA_BAR);

                                            OokOokSummoned = true;
                                        }
                                    }
                                }
                            }
                        }
                        break;

                        // Script for summoning Hoptallus.
                        case NPC_HOPPER:
                            if (!HoptallusSummoned && killed->GetPositionX() > -707.0f && killed->GetPositionY() < 1280.0f) // Check outside Hopper.
                            {
                                if (Player* player = killed->FindNearestPlayer(20.0f))
                                    player->SummonCreature(BOSS_HOPTALLUS, HoptallusSummonPosition, TEMPSUMMON_MANUAL_DESPAWN);
                                HoptallusSummoned = true;
                            }
                            break;

                    default: break;
                }
            }

            /*
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

            uint32 GetData(uint32 type) const OVERRIDE
            {
                return GetBossState(type);
            }

            uint64 GetData64(uint32 data) const OVERRIDE
            {
                switch(data)
                {
                    // Bosses.
                    case DATA_OOKOOK:               return uiOokOok;             break;
                    case DATA_HOPTALLUS:            return uiHoptallus;          break;
                    case DATA_YANZHU_THE_UNCASKED:  return uiYanzhuTheUncasked;  break;

                    default:                        return 0;                    break;
                }
            }

            bool SetBossState(uint32 data, EncounterState state)
            {
                if (!InstanceScript::SetBossState(data, state))
                    return false;

                if (state == DONE)
                {
                    switch(data)
                    {
                        case DATA_OOKOOK_EVENT:
                        case DATA_HOPTALLUS_EVENT:
                        case DATA_YANZHU_THE_UNCASKED_EVENT:
                        break;
                    }
                }

                return true;
            }

            std::string GetSaveData()
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "S B " << GetBossSaveData();

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

                if (dataHead1 == 'S' && dataHead2 == 'B')
                {
                    for (uint32 i = 0; i < MAX_ENCOUNTERS; ++i)
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
                }
                else OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_stormstout_brewery_InstanceMapScript(map);
        }
};

void AddSC_instance_stormstout_brewery()
{
    new instance_stormstout_brewery();
}
