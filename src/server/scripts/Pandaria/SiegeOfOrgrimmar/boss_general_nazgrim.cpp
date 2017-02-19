#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "siege_of_orgrimmar.h"

enum eSpells
{
    SPELL_BATTLE_STANCE     = 143589,
    SPELL_BERSERK           = 47008,
    SPELL_BERSERKER_STANCE  = 143594,
    SPELL_BONECRACKER       = 143638,
    SPELL_COOLING_OFF       = 143484,
    SPELL_DEFENSIVE_STANCE  = 143593,
    SPELL_EXECUTE           = 143502,
    SPELL_HEROIC_SHOCKWAVE  = 143716,
    SPELL_RAVAGER           = 143872,
    SPELL_SUNDERING_BLOW    = 143494,
    SPELL_WAR_SONG          = 143503,
    SPELL_AFRTERSHOCK       = 143712,
    SPELL_KORKRON_BANNER    = 143501
};

enum eEvents
{
	EVENT_BERSERK                   = 1,
	EVENT_BERSERKER_STANCE          = 2,
	EVENT_BONECRACKER               = 3,
	EVENT_COOLING_OFF               = 4,
	EVENT_DEFENSIVE_STANCE          = 5,
	EVENT_EXECUTE                   = 6,
	EVENT_HEROIC_SHOCKWAVE          = 7,
	EVENT_RAVAGER                   = 8,
	EVENT_WAR_SONG                  = 9,
	EVENT_AFRTERSHOCK               = 10,
	EVENT_KORKRON_BANNER            = 11,
	EVENT_SUNDERING_BLOW            = 12,
	EVENT_BATTLE_STANCE             = 13
};

enum eSays
{
    SAY_AGGRO   = 1,
    SAY_KILL    = 2,
    SAY_DEATH   = 3
};

enum eAdds
{
	mob_orgrimmar_faithful = 71715,
	mob_korkron_ironblade  = 71516,
	mob_korkron_arcweaver  = 71517,
	mob_korkron_assassin   = 71518,
	mob_korkron_warshaman  = 71519
}

class boss_general_nazgrim : public CreatureScript //71515
{
	public:
		boss_general_nazgrim() : CreatureScript("boss_general_nazgrim") { }

		struct boss_general_nazgrimAI : public BossAI
		{
			boss_general_nazgrimAI(Creature* creature) : BossAI(creature, DATA_GENERAL_NAZGRIM)
			{
				pInstance = creature->GetInstanceScript();
			}
			
			EventMap events;
			InstanceScript* pInstance;
			
			void Reset()
			{
				Reset();
				
				events.Reset();
				
				summons.DespawnAll();
				
				if (pInstance)
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
			}
			
			void JustReachedHome()
            {
                _JustReachedHome();

                if (pInstance)
                    pInstance->SetBossState(DATA_GENERAL_NAZGRIM, FAIL);
            }
			
			void EnterCombat(Unit* attacker)
            {
                if (pInstance)
                {
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    pInstance->SetBossState(DATA_GENERAL_NAZGRIM, IN_PROGRESS);
                }
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_SUNDERING_BLOW, urand(5000, 6000));
                events.ScheduleEvent(EVENT_BONECRACKER, urand(16000, 16000));
                events.ScheduleEvent(EVENT_BERSERK, urand(600000, 600000));
                events.ScheduleEvent(EVENT_BATTLE_STANCE, urand(0, 0));
                events.ScheduleEvent(EVENT_BERSERKER_STANCE, urand(120000, 120000));
                events.ScheduleEvent(EVENT_DEFENSIVE_STANCE, urand(180000, 180000));
            }
			
			void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
            }
            
            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }
			
			void KilledUnit(Unit* who)
            {
            }
			
			void JustDied(Unit* killer)
            {
                _JustDied();

                if (pInstance)
                {
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    pInstance->SetBossState(DATA_GENERAL_NAZGRIM, DONE);
                }
            }
			
			void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);
			}
		};

        CreatureAI* GetAI(Creature* pCreature) const
		{
			return new boss_general_nazgrimAI(pCreature);
		}
};

class mob_orgrimmar_faithful : public CreatureScript //71715
{
    public:
        mob_orgrimmar_faithful() : CreatureScript("mob_orgrimmar_faithful") { }

        struct mob_orgrimmar_faithfulAI: public ScriptedAI
        { 
            mob_orgrimmar_faithfulAI(Creature* creature) :  ScriptedAI(creature)
            { 
                pInstance = creature->GetInstanceScript(); 
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_orgrimmar_faithfulAI(creature);
        }
};

class mob_korkron_ironblade : public CreatureScript //71516
{
    public:
        mob_korkron_ironblade() : CreatureScript("mob_korkron_ironblade") { }

        struct mob_korkron_ironbladeAI : public ScriptedAI
        {
            mob_korkron_ironbladeAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_korkron_ironbladeAI(creature);
        }
};

class mob_korkron_arcweaver : public CreatureScript //71517
{
    public:
        mob_korkron_arcweaver() : CreatureScript("mob_korkron_arcweaver") { }

        struct mob_korkron_arcweaverAI : public ScriptedAI
        {
            mob_korkron_arcweaverAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_korkron_arcweaverAI(creature);
        }
};

class mob_korkron_assassin : public CreatureScript //71518
{
    public:
        mob_korkron_assassin() : CreatureScript("mob_korkron_assassin") { }

        struct mob_korkron_assassinAI : public ScriptedAI
        {
            mob_korkron_assassinAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                return;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_korkron_assassinAI(creature);
        }
};


class mob_korkron_warshaman : public CreatureScript //71519
{
    public:
        mob_korkron_warshaman() : CreatureScript("mob_korkron_warshaman") { }

        struct mob_korkron_warshamanAI : public ScriptedAI
        {
            mob_korkron_warshamanAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_korkron_warshamanAI(creature);
        }
};

void AddSC_general_nazgrim()
{
    new boss_general_nazgrim();
    new mob_orgrimmar_faithful();
    new mob_korkron_ironblade();
    new mob_korkron_arcweaver();
    new mob_korkron_assassin();
    new mob_korkron_warshaman();
};
