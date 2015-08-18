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
 * World Boss: Yu'lon <The Jade Serpent>.
 *
 * ======= QUESTS =======
 *
 * [90] Celestial Blessings
 *
 * Quest accept:
 * 
 * Wrathion says: I will meet you at each of the four shrines, champion.
 * Wrathion says: Remember, we must visit all four, but we only need to complete ONE of the four challenges.
 * Wrathion says: Choose the challenge most appropriate for your unique talents.
 * // Wrathion transforms into his whelp form.
 * Wrathion says: Good luck! 
 *
 * Quest complete:
 *
 * Wrathion says: We have done it, hero! We have the blessings of the celestials, and we have completed their challenge.
 * Wrathion says: I know our next step. Meet me back atop Mason's folly - you'll be pleasantly surprised at what I have in store for you! 
 *
 * Temple of the Jade Serpent - ! Ranged DPS spec challenge.
 *
 * Here we are - the ancient seat of pandaren wisdom.
 * Shall we begin?
 * (Gossip) Yu'lon awaits!
 *
 * Wrathion says: The Jade Serpent is the incarnation of wisdom of Pandaria. Let's see what she has to say.
 * Wrathion says: Great Jade Serpent. Hear our plea.
 * Wrathion says: My champion and I seek the blessing of your wisdom.
 * Yu'lon says: Ah, the dragon-child and the hero. Together you will achieve great things.
 * Yu'lon says: Tell me, dragon-child. What is the nature of wisdom?
 * Wrathion says: To do what is right, no matter the cost. That is wisdom.
 * Yu'lon says: Spoken like a true child of the Aspects. Always seeking to change the world.
 * Yu'lon says: But true wisdom comes from KNOWING what is right. And sometimes DOING nothing at all.
 * Wrathion says: When is it ever wise not to act for the greater good?
 * Yu'lon says: To learn THAT is to learn true wisdom.
 * Yu'lon says: Come forward! I will give you my blessing.
 * Yu'lon says: My challenge, if you accept it, will test your cunning and footwork even in the midst of life-threatening danger. 
 *
 * - Ranged DPS Challenge -
 *
 * Yu'lon says: Let the challenge begin! Hero, you must defeat the Black Prince in combat.
 * Wrathion says: Wait - that's absurd. I would annihilate my own champion!
 * Yu'lon says: You, my dark prince, will be blindfolded.
 * Wrathion says: ...interesting. But the outcome is unlikely to change.
 * Yu'lon says: We will demonstrate the limits of power without vision to guide it.
 * Wrathion says: Very well. I am sorry, champion - But I will not be the one learning a lesson today.
 * // Wrathion mutters to himself as he ties the blindfold.
 * Wrathion says: A blind dragon. Ridiculous!
 * Yu'lon yells: Begin!
 *
 * Yu'lon yells: The elements are undying! Make use of their time before they rise again!
 * Yu'lon yells: Use movement to your advantage. Do not allow the dragon-child to get close!
 * Yu'lon yells: Be aware of your surroundings!
 * Yu'lon yells: Do not allow the elements to engage you!
 * Yu'lon yells: Seek out your true enemy! Do not be fooled of his false images!
 *
 * Wrathion says: The legendary power of the Titans is mine to command!
 * Wrathion says: Where did you go? Come back!
 * Wrathion says: Stand still, I am trying to kill you here!
 * Wrathion says: Ahhh... Hah!
 * Wrathion yells: Your lucky that I'm not those Green Dragons that can see with their eyes closed!
 * Wrathion yells: Your eyes fail you yet!
 * Wrathion yells: But can you find me again?
 * Wrathion yells: One final time!
 * Wrathion says: Well done.
 *
 * Defeated
 * Yu'lon yells: That is enough!
 * Wrathion says: Just as I expected. Did you want to give it another go?
 * Yu'lon says: Champion, predict your foes movements and always stay a step ahead. Victory is within you.
 *
 * Victory
 * Yu'lon yells: Very good - that is enough!
 * Wrathion says: PAH! A lopsided challenge.
 * Yu'lon says: Do you see? Power without vision causes only random chaos and destruction. Please, young prince, take this lesson to heart.
 * Wrathion says: I am please that our hero is still alive. Thank you for your blessing, Great Serpent. 
 *
 * ===================================================================================================================
 *
 *[90] The Emperor's Way - Actual Boss fight.
 *
 * Intro
 * Emperor Shaohao yells: Long ago, the jade serpent instructed me to purify my spirit and become one with the land. My doubts were vanquished.
 * Yu'lon yells: The lesson of true wisdom lies within you, dear heroes, and in the choices you make. When faced with overwhelming darkness, will you make the right decisions?
 *
 * Aggro
 * The trial begins!
 *
 * Jadefire Wall
 * Listen to your inner voice, and seek out the truth.
 * Do not let your judgement be clouded in trying times.
 * Always consider the consequences of your actions.
 *
 * Kills player
 * Learn from your mistakes.
 *
 * Death
 * Yu'lon yells: Your wisdom has seen you through this trial. May it ever light your way out of dark places.
 * Emperor Shaohao yells: You have walked the trial of wisdom, and learned of the path of the jade serpent. May you learn from this lesson as I did.
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Player.h"

enum Texts
{
    SAY_INTRO               = 0, // The lesson of true wisdom lies within you, dear heroes, and in the choices you make. When faced with overwhelming darkness, will you make the right decisions?
    SAY_AGGRO               = 1, // The trial begins!
    SAY_DEATH               = 2, // Your wisdom has seen you through this trial. May it ever light your way out of dark places.
    SAY_SLAY                = 3, // Learn from your mistakes.
    SAY_JADEFIRE_WALL       = 4  // 0 - Listen to your inner voice, and seek out the truth. ; 1 - Do not let your judgement be clouded in trying times. ; 2 - Always consider the consequences of your actions.
};

enum Spells
{
    // Boss
    SPELL_JADEFLAME_BUFFET  = 144630, // Damage spell + increase % dmg taken.
    SPELL_JADEFIRE_BREATH   = 144530, // Damage spell.
    SPELL_JADEFIRE_BOLT     = 144545, // Cast time + spell. Script Effect to cast in 50y.
    SPELL_JADEFIRE_BOLT_S   = 144532, // On above SE. Triggers missiles in 50y.
    SPELL_JADEFIRE_BOLT_M   = 144541, // Triggered by above. Spawns NPC_JADEFIRE_BOLT on impact point.
    SPELL_JADEFIRE_WALL     = 144533, // Creates Areatrigger 1087.
    SPELL_JADEFIRE_WALL_DMG = 144539, // Damage from above Areatrigger.

    // NPCs
    SPELL_JADEFIRE_BOLT_D   = 147454  // Damage from standing within 11y of NPC_JADEFIRE_BOLT spawns.
};

enum Events
{
    EVENT_JADEFLAME_BUFFET  = 1, // 20s from aggro, 30s after.
    EVENT_JADEFIRE_BREATH   = 2, // 12s from aggro, 45s after.
    EVENT_JADEFIRE_BOLT     = 3, // 45s from aggro, 65s after.
    EVENT_JADEFIRE_WALL     = 4
};

enum Npcs
{
    NPC_JADEFIRE_BOLT       = 72016
};

// ToDo: Script Jadefire Bolt + NPC_JADEFIRE_BOLT, Jadefire Wall Areatrigger, fix timers.
class boss_yu_lon : public CreatureScript
{
    public:
        boss_yu_lon() : CreatureScript("boss_yu_lon") { }

        struct boss_yu_lonAI : public ScriptedAI
        {
            boss_yu_lonAI(Creature* creature) : ScriptedAI(creature), summons(me) { }

            EventMap events;
            SummonList summons;

            void InitializeAI()
            {
                if (!me->isDead())
                    Reset();
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();
            }
    
            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_JADEFLAME_BUFFET, urand(18000, 23000)); // 18-23
                events.ScheduleEvent(EVENT_JADEFIRE_BREATH, urand(12000, 14000)); // 12-14
                events.ScheduleEvent(EVENT_JADEFIRE_BOLT, urand(24000, 30000)); // 44-48
                events.ScheduleEvent(EVENT_JADEFIRE_WALL, urand(44000, 48000)); // 44-48
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void EnterEvadeMode()
            {
                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MoveTargetedHome();
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                summons.DespawnAll();
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                summon->setActive(true);

		        if (me->isInCombat())
                    summon->SetInCombatWithZone();

                switch (summon->GetEntry())
                {
                    case NPC_JADEFIRE_BOLT:
                        summon->SetReactState(REACT_PASSIVE);
						summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        break;

                    default: break;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_JADEFLAME_BUFFET:
                            DoCast(me, SPELL_JADEFLAME_BUFFET);
                            events.ScheduleEvent(EVENT_JADEFLAME_BUFFET, urand(30000, 40000));
                            break;

                        case EVENT_JADEFIRE_BREATH:
                            DoCast(me, SPELL_JADEFIRE_BREATH);
                            events.ScheduleEvent(EVENT_JADEFIRE_BREATH, urand(20000, 25000));
                            break;

                        case EVENT_JADEFIRE_BOLT:
                            DoCast(me, SPELL_JADEFIRE_BOLT);
                            events.ScheduleEvent(EVENT_JADEFIRE_BOLT, urand(50000, 60000));
                            break;

                        case EVENT_JADEFIRE_WALL:
                            Talk(SAY_JADEFIRE_WALL);
                            DoCast(me, SPELL_JADEFIRE_WALL);
                            events.ScheduleEvent(EVENT_JADEFIRE_WALL, urand(80000, 90000));
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_yu_lonAI(creature);
        }
};

void AddSC_boss_yu_lon()
{
    new boss_yu_lon();
}
