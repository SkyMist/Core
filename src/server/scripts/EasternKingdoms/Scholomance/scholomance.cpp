#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scholomance.h"

enum ScholoSpells
{
    //Gandling Event Spells
    SPELL_GANDLING_CHANNEL    = 114201,
    SPELL_SKULL_GANDLING_DEAD = 126343,
    SPELL_HOVER_IDLE          = 127603, // maybe not the correct hover for Gandling
    SPELL_BONE_ARMOR_VISUAL   = 113996, // find the correct visual for Gandling
};

class mob_gandling_event : public CreatureScript
{
public:
    mob_gandling_event() : CreatureScript("mob_gandling_event") { }

    struct mob_gandling_eventAI : public ScriptedAI
    {
        mob_gandling_eventAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_DEFENSIVE);
        }

        void DoAction(int32 const action)
        {
        // Texts and Actions will be included soon
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_gandling_eventAI (creature);
    }
};

void AddSC_scholomance()
{
    new mob_gandling_event();
}
