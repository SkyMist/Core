#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scholomance.h"

enum Spells
{
    SPELL_RUSTING      = 113765,
    SPELL_BONE_SPIKE   = 113999,
    SPELL_SOULFLAME    = 114007,
    SPELL_BONE_ARMOR   = 113996
};

enum Talk
{
    SAY_INTRO          = 0,
    SAY_AGGRO          = 1,
    EMOTE_PL_HIT_BONE  = 3,
    SAY_KILLER         = 4,
};

enum Events
{
    EVENT_RUSTING,
    EVENT_SOULFLAME,
    EVENT_BONE_SPIKE
};

enum Actions
{
    ACTION_INTRO_RATTLEGORE = 1
};

class boss_rattlegore : public CreatureScript
{
    public:
        boss_rattlegore() : CreatureScript("boss_rattlegore") { }

        struct boss_rattlegoreAI : public BossAI
        {
            boss_rattlegoreAI(Creature* creature) : BossAI(creature, DATA_RATTLEGORE) { }

            void Reset()
            {
                _Reset();
                intro = false;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
                events.ScheduleEvent(EVENT_RUSTING, 1000);
                events.ScheduleEvent(EVENT_BONE_SPIKE, 10000);
            }

            void RespawnCreatures(uint32 entry)
            {
                std::list<Creature*> creatures;
                    GetCreatureListWithEntryInGrid(creatures, me, entry, 1000.0f);

                if (creatures.empty())
                    return;

                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                    (*iter)->Respawn();
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_INTRO_RATTLEGORE: // there is a special summon spell, needs to be sniffed from retail
                        intro = true;
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        Talk(SAY_INTRO);
                        break;
                    default:
                        break;
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILLER);
            }

            void UpdateAI(uint32 const diff)
            {
                if(!UpdateVictim())
                    return;

                events.Update(diff);

                if(me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_RUSTING:
                            DoCast(SPELL_RUSTING);
                            events.ScheduleEvent(EVENT_RUSTING, urand(5000, 7000));
                            break;
                        case EVENT_BONE_SPIKE:
                            Talk(EMOTE_PL_HIT_BONE);
                            RespawnCreatures(CREATURE_BONE_PILE);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                                DoCast(target, SPELL_BONE_SPIKE);
                            events.ScheduleEvent(EVENT_BONE_SPIKE, urand(10000, 15000));
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        private:
            bool intro;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_rattlegoreAI(creature);
        }
};

class mob_bone_pile : public CreatureScript
{
public:
    mob_bone_pile() : CreatureScript("mob_bone_pile") { }

    struct mob_bone_pileAI : public ScriptedAI
    {
        mob_bone_pileAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        void OnSpellClick(Unit* /*clicker*/)
        {
            me->DespawnOrUnsummon(3000);
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_bone_pileAI (creature);
    }
};

class at_intro_rattlegore : public AreaTriggerScript
{
    public:
        at_intro_rattlegore() : AreaTriggerScript("at_intro_rattlegore") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (InstanceScript* instance = player->GetInstanceScript())
            {
                if (Creature* rattlegore = ObjectAccessor::GetCreature(*player, instance->GetData64(DATA_RATTLEGORE)))
                    rattlegore->AI()->DoAction(ACTION_INTRO_RATTLEGORE);
            }

            return true;
        }
};

void AddSC_boss_rattlegore()
{
    new boss_rattlegore();
    new mob_bone_pile();
    new at_intro_rattlegore();
}
