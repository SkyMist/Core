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
 * World Boss: Niuzao <The Black Ox>.
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
 * Niuzao Temple - ! Tank spec challenge.
 *
 * Wrathion says: Here we are, the home of Niuzao, the Black Ox. He is the patron spirit of Pandaren fortitude, the only celestial who chooses to live beyond the wall.
 * Wrathion says: Mighty Black Ox! Hear our plea.
 * Wrathion says: My champion and I seek the blessing of fortitude.
 * Niuzao says: So enters the dragon, and his mortal champion. You are welcome here.
 * Niuzao says: Tell me, Black Prince: What is the nature of fortitude?
 * Wrathion says: The strength to overcome any hardship! That is fortitude.
 * Niuzao says: You confuse strength with fortitude, young dragon. But power is worthless without spirit!
 * Niuzao says: I have seen humble slaves endure unimaginable torture. Only to rise up and overthrow their masters.
 * Niuzao says: And I have seen the mightiest of emperors laid low by the perseverance of the smallest of enemies.
 * Wrathion says: You mean to say - physical strength is developed without, but fortitude comes from within?
 * Niuzao says: Precisely!
 * Wrathion says: ...Sure, I knew that.
 * Niuzao says: Of course you did. We will put your understanding to the test.
 * Niuzao says: My challenge, if you accept it, will test your champion's ability to defend and protect while enduring terrible hardship. 
 *
 * - Tank Challenge -
 *
 * Niuzao says: Let the challenge begin! Hero, you must defend the Black Prince while he faces his inner turmoil.
 * Wrathion says: I'm a black dragon. I won't need any help.
 * Niuzao says: Is that so? You are filled with doubts and fears, young Prince.
 * Niuzao says: Face your inner demons. Now is the time.
 * Wrathion says: What - wait - father!?
 * Vision of Deathwing yells: I shall tear this world apart!
 * Wrathion says: Please - don't make me do this.
 * Niuzao yells: Your champion will defend you. Begin!
 * Vision of Deathwing yells: Your efforts are insignificant!
 * 
 * Niuzao yells: The elements will not die! Make use of your time before they rise again!
 * Niuzao yells: Control the combat. Pay attention to ALL your foes.
 * Niuzao yells: Use the battlefield to regenerate yourself!
 * Niuzao yells: Protect the Black Prince! Distract his enemies!
 * Niuzao yells: Do not allow your foe to complete his attack!
 * Niuzao yells: You must sacrifice yourself to keep the Prince from falling!
 * 
 * Vision of Deathwing yells: Your tenacity is admirable, but pointless!
 * Vision of Deathwing yells: There's no shelter from my fury!
 * Vision of Deathwing yells: Your armor means nothing! Your faith - even less!
 * 
 * Failure
 * Wrathion yells: Enough! Make it stop!
 * Niuzao yells: So be it. The test is over.
 * Niuzao says: Remember: You are never defeated until you decide to remain so. Those with true fortitude always rise whenever they fall.
 * Niuzao says: Try again - I know you can defeat this challenge!
 * 
 * Victory
 * Niuzao yells: Well done! You have triumphant!
 * Wrathion says: Such... madness...
 * Niuzao says: You are stronger than your father, young prince.
 * Niuzao says: You have friends. Your champion did not allow you to fall. Take this lesson to heart.
 * Wrathion says: I understand. Thank you, Mighty Ox. 
 *
 * ===================================================================================================================
 *
 *[90] The Emperor's Way - Actual Boss fight.
 *
 * Intro
 * Emperor Shaohao yells: In the face of great fear, the black ox taught me fortitude, Through terror and darkness it persevered. The fear was vanquished.
 * Niuzao yells: Can you stand on the tallest peak? Winds and sleet buffeting your skin, until the trees wither and the mountains fall into the sea?
 *
 * Aggro
 * We shall see.
 *
 * Charge
 * The winds may be strong, and the sleets may sting.
 * You are the mountain unmovable by all but time!
 *
 * Massive Quake
 * Be vigilant in your stand or you will never achieve your goals!
 *
 * Kills player
 * You must persevere!
 *
 * Death
 * Niuzao yells: Though you will be surrounded by foes greater than you can imagine, your fortitude shall allow you to endure. Remember this in the times ahead.
 * Emperor Shaohao yells: You have walked the trial of fortitude, and learned of the path of the black ox. May it bless your passage.
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
    // Niuzao
    SAY_INTRO               = 0, // Can you stand on the tallest peak? Winds and sleet buffeting your skin, until the trees wither and the mountains fall into the sea?
    SAY_AGGRO               = 1, // We shall see.
    SAY_DEATH               = 2, // Though you will be surrounded by foes greater than you can imagine, your fortitude shall allow you to endure. Remember this in the times ahead.
    SAY_SLAY                = 3, // You must persevere!
    SAY_MASSIVE_QUAKE       = 4, // Be vigilant in your stand or you will never achieve your goals!
    SAY_CHARGE              = 5  // 0 - The winds may be strong, and the sleets may sting. ; 1 - You are the mountain unmovable by all but time!
};

enum Spells
{
    // Niuzao
    SPELL_HEADBUTT          = 144610, // Damage, knockback and threat removal.
    SPELL_OXEN_FORTITUDE    = 144606, // Player health + boss damage increase.
    SPELL_OXEN_FORTITUDE_T  = 144607, // Triggered on players by above.
    SPELL_MASSIVE_QUAKE     = 144611, // Damage each sec.
    SPELL_MASSIVE_QUAKE_D   = 144612, // Triggered by above.
    SPELL_CHARGE            = 144608, // Charge cast time and aura.
    SPELL_CHARGE_D          = 144609, // Per. dmg., triggered by above each sec.
};

enum Events
{
    // Niuzao
    EVENT_HEADBUTT          = 1, // 20s from aggro, 30s after.
    EVENT_OXEN_FORTITUDE    = 2, // 12s from aggro, 45s after.
    EVENT_MASSIVE_QUAKE     = 3, // 45s from aggro, 65s after.
    EVENT_NIUZAO_CHARGE     = 4  // 66 and 33%.
};

enum ChargeStates
{
    DONE_NONE               = 0, // No casts done.
    DONE_66                 = 1, // First cast done.
    DONE_33                 = 2  // Both casts done.
};

// ToDo: Script Charge, fix timers.
class boss_niuzao : public CreatureScript
{
    public:
        boss_niuzao() : CreatureScript("boss_niuzao") { }

        struct boss_niuzaoAI : public ScriptedAI
        {
            boss_niuzaoAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;
            uint8 chargeDone;

            void InitializeAI()
            {
                if (!me->isDead())
                    Reset();
            }

            void Reset()
            {
                events.Reset();

                chargeDone = DONE_NONE;
            }
    
            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_HEADBUTT, urand(18000, 23000)); // 18-23
                events.ScheduleEvent(EVENT_OXEN_FORTITUDE, urand(12000, 14000)); // 12-14
                events.ScheduleEvent(EVENT_MASSIVE_QUAKE, urand(44000, 48000)); // 44-48
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
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                // Set Crane Rush phases execution.
                if (me->HealthBelowPct(67) && chargeDone == DONE_NONE || me->HealthBelowPct(34) && chargeDone == DONE_66)
                {
                    events.ScheduleEvent(EVENT_NIUZAO_CHARGE, 2000);
                    chargeDone++;
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_HEADBUTT:
                            DoCastVictim(SPELL_HEADBUTT);
                            events.ScheduleEvent(EVENT_HEADBUTT, urand(35000, 40000));
                            break;

                        case EVENT_OXEN_FORTITUDE:
                            DoCast(me, SPELL_OXEN_FORTITUDE);
                            events.ScheduleEvent(EVENT_OXEN_FORTITUDE, urand(43000, 47000));
                            break;

                        case EVENT_MASSIVE_QUAKE:
                            Talk(SAY_MASSIVE_QUAKE);
                            DoCast(me, SPELL_MASSIVE_QUAKE);
                            events.ScheduleEvent(EVENT_MASSIVE_QUAKE, urand(70000, 75000));
                            break;

                        case EVENT_NIUZAO_CHARGE:
                            Talk(SAY_CHARGE);
                            DoCast(me, SPELL_CHARGE);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_niuzaoAI(creature);
        }
};

void AddSC_boss_niuzao()
{
    new boss_niuzao();
}
