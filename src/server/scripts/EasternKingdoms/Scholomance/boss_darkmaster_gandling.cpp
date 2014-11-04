#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "ScriptedCreature.h"
#include "scholomance.h"

enum Spells
{
    SPELL_HARSH_LESSON      = 113395,
    SPELL_IMMOLATE          = 113141,
    SPELL_RISE              = 113143,
    SPELL_INCINERATE        = 113136,

    // Trash
    SPELL_EXPLOSIVE_PAIN    = 113312
};

enum Events
{
    EVENT_INCINERATE,
    EVENT_LESSON,
    EVENT_RISE,
    EVENT_IMMOLATE
};

enum Actions
{
    ACTION_GANDLING_INTRO = 1
};

enum Texts
{
    SAY_INTRO    = 0, // If you haven't come to study, I'll use you to teach a lesson. (27496)
    SAY_AGGRO    = 1, // School is in session! (27477)
    SAY_KILLER   = 2, // Dunce. (27497)
    SAY_DEATH    = 3, // Class...dismissed. (27478)
    EMOTE_LESSON = 4, //
};

class boss_darkmaster_gandling : public CreatureScript
{
    public:
        boss_darkmaster_gandling() : CreatureScript("boss_darkmaster_gandling") { }

        struct boss_darkmaster_gandlingAI : public BossAI
        {
            boss_darkmaster_gandlingAI(Creature* creature) : BossAI(creature, DATA_GANDLING)
            {
                pInstance = creature->GetInstanceScript();
            }

            InstanceScript* pInstance;

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
                events.ScheduleEvent(EVENT_INCINERATE, 500);
                events.ScheduleEvent(EVENT_RISE, 17000);
            }

            void DoAction(const int32 action)
            {
                if (action == ACTION_GANDLING_INTRO) // intro will also remove his channeling cosmetic spell (needs retail sniff)
                {
                    Talk(SAY_INTRO);
                }
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILLER);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
                Talk(SAY_DEATH);
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
                        case EVENT_INCINERATE:
                            DoCastVictim(SPELL_INCINERATE);
                            events.ScheduleEvent(EVENT_INCINERATE, 2*IN_MILLISECONDS);
                            break;
                        case EVENT_RISE:
                            DoCast(SPELL_RISE);
                            events.ScheduleEvent(EVENT_RISE, 60*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_IMMOLATE, 29*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_LESSON, 27*IN_MILLISECONDS);
                            break;
                        case EVENT_LESSON:
                            DoCast(SPELL_HARSH_LESSON);
                            break;
                        case EVENT_IMMOLATE:
                            DoCastVictim(SPELL_IMMOLATE);
                            break;
                        default:
                            break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_darkmaster_gandlingAI(creature);
        }
};

class AreaTrigger_at_gandling_intro : public AreaTriggerScript
{
    public:
        AreaTrigger_at_gandling_intro() : AreaTriggerScript("at_gandling_intro") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/)
        {
            if (InstanceScript* pInstance = player->GetInstanceScript())
                if (Creature* Gandling = ObjectAccessor::GetCreature(*player, pInstance->GetData64(DATA_GANDLING)))
                    Gandling->AI()->DoAction(ACTION_GANDLING_INTRO);

            return false;
        }
};

void AddSC_boss_darkmaster_gandling()
{
    new boss_darkmaster_gandling();
    new AreaTrigger_at_gandling_intro();
}
