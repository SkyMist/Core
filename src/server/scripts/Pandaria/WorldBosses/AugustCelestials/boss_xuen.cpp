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
 * World Boss: Xuen <The White Tiger>.
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
 * Temple of the White Tiger - ! Melee DPS spec challenge.
 *
 * Wrathion says: I am looking forward to this. The White Tiger is the embodiment of strength in Pandaria!
 * Wrathion says: Great White Tiger! Hear our plea.
 * Wrathion says: My champion and I seek the blessing of your strength.
 * Xuen says: Ah, the young black dragon graces my temple. The deeds of you and your champion are quickly becoming Pandaren legend.
 * Wrathion says: I take great pride in the company I keep.
 * Xuen says: Tell me, young prince: What is the nature of strength?
 * Wrathion says: The power to crush one's foe. That is strength.
 * Xuen says: You speak only in terms of one's enemies.
 * Wrathion says: Certainly, strength can benefit one's allies.
 * Xuen says: How so?
 * Wrathion says: ...by using it to crush their enemies!
 * Xuen says: A black dragon, through and through. Very well, I will give you my blessing, but also some advice:
 * Xuen says: Strength used in the service of others is twice as powerful as strength spent on one's foes.
 * Xuen says: Now. My challenge, should you accept it, will test your ability to stand right in the face of danger and persevere. 
 *
 * - Melee DPS Challenge -
 *
 * Xuen says: Let the challenge begin! Hero, you must defeat the Black Prince in close quarters combat.
 * Wrathion says: Wait - that's absurd. I would annihilate my own champion!
 * Xuen says: You, Wrathion, will be blindfolded.
 * Wrathion says: ...interesting. But the outcome is unlikely to change.
 * Xuen says: Black Prince. You are very strong, but what good is strength without the vision to guide it?
 * Wrathion says: Very well. I am sorry, champion - But I will not be the one learning a lesson today.
 * // Wrathion mutters to himself as he ties the blindfold.
 * Wrathion says: A blind dragon. Ridiculous!
 * Xuen yells: Begin!
 * 
 * Xuen yells: Seek out your true enemy! Do not be fooled of his false images!
 * Xuen yells: The dragon is stronger than you, hero! Do not let him finish his attacks!
 * Xuen yells: Disarm him if you can! Use every advantage!
 * Xuen yells: Stay behind your foe! He cannot see you! Move quickly!
 * Xuen yells: Dodge his attacks! Use his strength against him!
 * Xuen yells: The elements will not die! Make use of your time before they rise again!
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
 * Xuen yells: That is enough!
 * Wrathion says: Just as I expected. Did you want to give it another go?
 * Xuen says: Champion, turn your foes strength and confidence against him. I believe in you.
 * 
 * Victory
 * Xuen yells: Very good - lay down your weapons!
 * Wrathion says: PAH! A lopsided challenge.
 * Xuen says: I hope now you understand limits of strength without vision. It is one thing to change the world, but quite another to do what is right.
 * Xuen says: Young prince, I hope you take this lesson to heart.
 * Wrathion says: I am pleased that our hero is still alive. Thank you for your blessing, Mighty Tiger. 
 *
 * ===================================================================================================================
 *
 *[90] The Emperor's Way - Actual Boss fight.
 *
 * Intro
 * Emperor Shaohao yells: When my journey began, I was reckless with my strength. The white tiger taught me control. My hatred, anger, and violence were vanquished.
 * Xuen yells: Strength is far more than simple physical prowess. When you are truly tested, will you be able to tell the difference between strength and power?
 *
 * Aggro
 * Haha! The trial commences.
 *
 * Agility
 * Believe in your strength.
 * You have the power to change your destiny.
 * Do not mistake the power that darkness offers for true strength.
 *
 * Kills player
 * Return twice as powerful.
 *
 * Death
 * Xuen yells: You are strong, stronger even than you realize. Carry this thought with you into the darkness ahead. Let it shield you.
 * Emperor Shaohao yells: You have walked the trial of strength, and learned of the path of the white tiger. May you remember this lesson always.
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
    // Xuen
    SAY_INTRO               = 0, // Strength is far more than simple physical prowess. When you are truly tested, will you be able to tell the difference between strength and power?
    SAY_AGGRO               = 1, // Haha! The trial commences.
    SAY_DEATH               = 2, // You are strong, stronger even than you realize. Carry this thought with you into the darkness ahead. Let it shield you.
    SAY_SLAY                = 3, // Return twice as powerful.
    SAY_AGILITY             = 4  // 0 - Believe in your strength. ; 1 - You have the power to change your destiny. ; 2 - Do not mistake the power that darkness offers for true strength.
};

enum Spells
{
    // Xuen
    SPELL_LEAP              = 144640, // Jump spell.
    SPELL_SPECTRAL_SWIPE    = 144652, // Damage and armor reduction. Triggered by above.
    SPELL_CRACK_LIGHTNING   = 144635, // Cast time and aura.
    SPELL_CRACK_LIGHTNING_S = 144634, // Script effect to cast each sec in 60y. Triggered by above.
    SPELL_CRACK_LIGHTNING_D = 144633, // Damage, on above SE. Jumps to 5 targets.
    SPELL_CHI_BARRAGE       = 144642, // Cast time and aura. Script effect to cast in 100y.
    SPELL_CHI_BARRAGE_M     = 144643, // Triggers missiles.
    SPELL_CHI_BARRAGE_D     = 144644, // Damage spell for impact zone, 3y. Triggered by above.
    SPELL_AGILITY           = 144631  // Boss buff spell.
};

enum Events
{
    // Xuen
    EVENT_LEAP              = 1, // 20s from aggro, 30s after.
    EVENT_CRACK_LIGHTNING   = 2, // 12s from aggro, 45s after.
    EVENT_CHI_BARRAGE       = 3, // 45s from aggro, 65s after.
    EVENT_AGILITY           = 4
};

// ToDo : Script Crackling Lightning + Chi Barrage, fix timers.
class boss_xuen : public CreatureScript
{
    public:
        boss_xuen() : CreatureScript("boss_xuen") { }

        struct boss_xuenAI : public ScriptedAI
        {
            boss_xuenAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;

            void InitializeAI()
            {
                if (!me->isDead())
                    Reset();
            }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_CRACK_LIGHTNING, urand(18000, 23000));
                events.ScheduleEvent(EVENT_LEAP, urand(12000, 14000));
                events.ScheduleEvent(EVENT_CHI_BARRAGE, urand(44000, 48000));
                events.ScheduleEvent(EVENT_AGILITY, urand(60000, 70000));
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

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_LEAP:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                                DoCast(target, SPELL_LEAP);
                            events.ScheduleEvent(EVENT_LEAP, urand(35000, 40000));
                            break;

                        case EVENT_CRACK_LIGHTNING:
                            DoCast(me, SPELL_CRACK_LIGHTNING);
                            events.ScheduleEvent(EVENT_CRACK_LIGHTNING, urand(35000, 40000));
                            break;

                        case EVENT_CHI_BARRAGE:
                            DoCast(me, SPELL_CHI_BARRAGE);
                            events.ScheduleEvent(EVENT_CHI_BARRAGE, urand(43000, 47000));
                            break;

                        case EVENT_AGILITY:
                            Talk(SAY_AGILITY);
                            DoCast(me, SPELL_AGILITY);
                            events.ScheduleEvent(EVENT_AGILITY, urand(113000, 127000));
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_xuenAI(creature);
        }
};

void AddSC_boss_xuen()
{
    new boss_xuen();
}
