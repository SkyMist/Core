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
 * Dungeon: Stormstout Brewery.
 * Boss: Yan-Zhu the Uncasked.
 */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "MoveSplineInit.h"

#include "stormstout_brewery.h"

/*
Intro
    Uncle Gao yells: Yes yes yes! So close! Just a pinch of...oops...
    Uncle Gao yells: Too bitter! Just a drop of honey, and a cane of sugar, and maybe some corn? Yes, more corn!
    Uncle Gao yells: Yes, yes, yes...no, no, no no no! Yes! No! Peppers!
    Uncle Gao yells: Ahh! Help! What is that thing? It doesn't matter, nothing will stop me now, not when I'm so close! Maybe if I don't look at it...?

Outro
    Uncle Gao says: Is it... can it be???
    * Gao runs over and kneels down by Yan-Zhu's remains and speaks reverently. *
    Uncle Gao says: Such harmony of flavor, such heady aroma! It is...the perfect brew!
    Uncle Gao yells: The name of Stormstout will be sung once again throughout the hills!
    * As he proclaims this, Chen runs into the Tasting Room and over to Yan-Zhu. *
    Chen Stormstout says: Uncle Gao, this brewery was left in your care! What have you done?
    Uncle Gao yells: You again! Don't you see? I have made a name for myself at last: I have brewed perfection!
    * Chen is dismayed with Gao's attitude. *
    Chen Stormstout says: At what cost? The Brewery is trashed! Infested!
    Uncle Gao says: Details, details.
    Chen Stormstout says: And there are Virmen in the main store!
    * Gao walks over to stand behind the stove as Chen follows to stand in front of it. *
    Uncle Gao says: Look, "Chen Stormstout", we can't all be heroes, running from our responsibilities, tromping around the Dread Wastes, saving the world. Some of us are "Artists".
    Chen Stormstout says: I think the brewery might be on fire.
    * Although Chen points this out, Gao's tone is dismissive, even bored. *
    Uncle Gao says: Yes, yes. That happens. 

    Mastery stuff depends on npc's summoned in intro.
    Just a beer thought:
    Weirdly, Bloat mastery should go with Stout abilities, as the adds summoned are Stout / Bloated, and Wheat with Sudsy / Fizzy (top fermented).
    Thus, we will handle them correctly and not how sites / the dungeon journal does, to have a correct selection.

    1. For Brewmastery: Stout
    - First wave of adds selected counts.
        - Bloated Alementals : Bloat.
        - Stout Alementals : Blackout Brew.

    2. For Brewmastery: Ale
    - Second wave of adds selected counts.
        - Yeasty Alementals : Yeasty Brew.
        - Bubbling Alementals : Bubble Shield.

    3. For Brewmastery: Wheat
    - Third wave of adds selected counts.
        - Fizzy Alementals : Carbonation.
        - Sudsy Alementals : Wall of Suds.
*/

enum Yells
{
    // Intro / Outro.
};

enum Spells
{
    // Boss
    SPELL_BREW_BOLT           = 114548,
    SPELL_ROOT                = 42716,  // Self root forever, used when Bubble Shield is up.

    // One of two from each category of the following abilities:

    // Brewmastery: Stout.
    SPELL_BLOAT               = 106546, // (1).
    SPELL_BLOAT_DUMMY         = 114929, // Tooltip says "Can use the Bloat ability."
    SPELL_BLOATED             = 106549, // Player aura triggering 106560 Gushing Brew - damage.
    SPELL_BLACKOUT_BREW       = 106851, // (2).
    SPELL_BLACKOUT_BREW_DUMMY = 114930, // Tooltip says "Can use the Blackout Brew ability."
    SPELL_BLACKOUT_DRUNK      = 106857, // At 10 stacks of SPELL_BLACKOUT_BREW.

    // Bremastery: Ale.
    SPELL_BUBBLE_SHIELD       = 106563, // (1).
    SPELL_BUBBLE_SHIELD_DUMMY = 114931, // Tooltip says "Can use the Bubble Shield ability."
    // Summons multiple NPC_YEASTY_BREW_ALEMENTAL_Y (2).
    SPELL_YEASTY_BREW_DUMMY   = 114932, // Tooltip says "Can summon Yeasty Brew minions."

    // Bremastery: Wheat.
    SPELL_CARBONATION         = 115003, // Triggers 114386 damage (1).
    SPELL_CARBONATION_DUMMY   = 114934, // Tooltip says "Can use the Carbonation ability."
    // Summons multiple NPC_FIZZY_BUBBLE (if uses Carbonation).
    SPELL_FIZZY_BUBBLE_VISUAL = 114458, // Dummy visual for NPC.
    SPELL_FIZZY_BUBBLE        = 114459, // Player fly aura on spellclick.
    // Summons multiple NPC_WALL_OF_SUDS (if uses Wall of Suds) (2).
    SPELL_SUDSY               = 114468, // If uses Wall of Suds. Triggers 114470 multiple Jump height at Jumping. Player aura.
    SPELL_SUDSY_JUMP          = 114470, // Triggered by above.
    SPELL_WALL_OF_SUDS_DUMMY  = 114933, // Tooltip says "Can summon walls of suds."

    // NPCs
    SPELL_YEASTY_BREW_BOLT    = 116155, // Yeasty Brew Alementals can cast a less powerful version of Brew Bolt, inflicting 9750 to 10250 Frost damage.
    SPELL_YEASTY_SUMMON_VIS   = 116259, // Yeasty Brew Alemental spawn visual.
    SPELL_FERMENT             = 106859, // Channeled beam, triggers 114451 - 1% Hp / Mana restore on target.
    SPELL_FERMENT_HEAL        = 114451, // Heal.
    SPELL_WALL_OF_SUDS        = 114467, // Triggers 114466 damage and stun.
    SPELL_WALL_OF_SUDS_DAMAGE = 114466  // Triggered by above.
};

enum Events
{
    // Boss
    EVENT_BREW_BOLT           = 1,

    // Bremastery: Stout.
    EVENT_BLOAT,
    EVENT_BLACKOUT_BREW,

    // Bremastery: Ale.
    EVENT_BUBBLE_SHIELD,
    EVENT_SET_BUBBLE_SHIELD_STACKS,
    EVENT_YEASTY_BREW_ELEMENTALS,

    // Brewmastery: Wheat.
    EVENT_CARBONATION,
    EVENT_WALL_OF_SUDS,

    // NPCs

    // Yeasty Brew Alemental
    EVENT_YEASTY_BREW_BOLT,
    EVENT_FERMENT
};

enum Abilities
{
    // Brewmastery: Stout.
    ABILITY_BLOAT              = 0,
    ABILITY_BLACKOUT_BREW,

    // Bremastery: Ale.
    ABILITY_BUBBLE_SHIELD,
    ABILITY_YEASTY_BREW_ELEMENTALS,

    // Bremastery: Wheat.
    ABILITY_CARBONATION,
    ABILITY_WALL_OF_SUDS
};

// NPC summon positions.
Position const SudsPositions[4] =
{
    {-734.301f, 1130.030f, 166.660f, 1.843f}, // Top
    {-670.259f, 1190.673f, 166.749f, 3.241f}, // Right
    {-734.301f, 1130.030f, 166.660f, 0.276f}, // Left
    {-670.259f, 1190.673f, 166.749f, 4.988f}, // Bottom
};

// NPC move positions, inversed from summon ones. L->R and B->U + the other way around.
Position const SudsMovePositions[2] =
{
    {-662.180f, 1150.694f, 166.710f}, // Right and Top go here.
    {-745.371f, 1169.491f, 166.752f}, // Left and Bottom go here.
};

class boss_yan_zhu_the_uncasked : public CreatureScript
{
    public :
        boss_yan_zhu_the_uncasked() : CreatureScript("boss_yan_zhu_the_uncasked") { }

        struct boss_yan_zhu_the_uncasked_AI : public BossAI
        {
            boss_yan_zhu_the_uncasked_AI(Creature* creature) : BossAI(creature, DATA_YANZHU_THE_UNCASKED_EVENT), summons(me)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
            SummonList summons;
            uint8 RandomStoutAbility;
            uint8 RandomAleAbility;
            uint8 RandomWheatAbility;
            uint8 sudWallsCasts;

            void InitializeAI()
            {
                if (!me->isDead())
                    Reset();
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                if (instance)
                    instance->SetData(DATA_YANZHU_THE_UNCASKED_EVENT, NOT_STARTED);

                RandomStoutAbility = RAND(ABILITY_BLOAT, ABILITY_BLACKOUT_BREW);
                RandomAleAbility   = RAND(ABILITY_BUBBLE_SHIELD, ABILITY_YEASTY_BREW_ELEMENTALS);
                RandomWheatAbility = RAND(ABILITY_CARBONATION, ABILITY_WALL_OF_SUDS);

                sudWallsCasts = 0;

                _Reset();

                // Bremastery: Stout.
                if (RandomStoutAbility == ABILITY_BLOAT)
                    me->AddAura(SPELL_BLOAT_DUMMY, me);         // Add the visual tooltip.
                else
                    me->AddAura(SPELL_BLACKOUT_BREW_DUMMY, me); // Add the visual tooltip.

                // Bremastery: Ale.
                if (RandomAleAbility == ABILITY_BUBBLE_SHIELD)
                    me->AddAura(SPELL_BUBBLE_SHIELD_DUMMY, me); // Add the visual tooltip.
                else
                    me->AddAura(SPELL_YEASTY_BREW_DUMMY, me);   // Add the visual tooltip.

                // Brewmastery: Wheat.
                if (RandomWheatAbility == ABILITY_CARBONATION)
                    me->AddAura(SPELL_CARBONATION_DUMMY, me);   // Add the visual tooltip.
                else
                    me->AddAura(SPELL_WALL_OF_SUDS_DUMMY, me);  // Add the visual tooltip.
            }

            void EnterCombat(Unit* /*who*/)
			{
                if (instance)
                {
                    instance->SetData(DATA_YANZHU_THE_UNCASKED_EVENT, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
                }

                // Melee range check.
                events.ScheduleEvent(EVENT_BREW_BOLT, 5000);

                // Bremastery: Stout.
                if (RandomStoutAbility == ABILITY_BLOAT)
                    events.ScheduleEvent(EVENT_BLOAT, urand(11000, 13000));
                else
                    events.ScheduleEvent(EVENT_BLACKOUT_BREW, urand(9000, 11000));

                // Bremastery: Ale.
                if (RandomAleAbility == ABILITY_BUBBLE_SHIELD)
                    events.ScheduleEvent(EVENT_BUBBLE_SHIELD, urand(16000, 19000));
                else
                    events.ScheduleEvent(EVENT_YEASTY_BREW_ELEMENTALS, urand(18000, 20000));

                // Brewmastery: Wheat.
                if (RandomWheatAbility == ABILITY_CARBONATION)
                    events.ScheduleEvent(EVENT_CARBONATION, urand(22500, 24500));
                else
                    events.ScheduleEvent(EVENT_WALL_OF_SUDS, urand(25000, 27000));

                _EnterCombat();
            }

            void EnterEvadeMode()
            {
                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                {
                    instance->SetData(DATA_YANZHU_THE_UNCASKED_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove

                    if (me->HasAura(SPELL_BLOAT_DUMMY))
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOATED);
                    if (me->HasAura(SPELL_BLACKOUT_BREW_DUMMY))
                    {
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACKOUT_BREW);
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACKOUT_DRUNK);
                    }
                    if (me->HasAura(SPELL_WALL_OF_SUDS_DUMMY))
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SUDSY);
                }

                // _EnterEvadeMode();
            }

            void JustDied(Unit* /*killer*/)
            {
                summons.DespawnAll();

                if (instance)
                {
                    instance->SetData(DATA_YANZHU_THE_UNCASKED_EVENT, DONE);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove

                    if (me->HasAura(SPELL_BLOAT_DUMMY))
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOATED);
                    if (me->HasAura(SPELL_BLACKOUT_BREW_DUMMY))
                    {
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACKOUT_BREW);
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACKOUT_DRUNK);
                    }
                    if (me->HasAura(SPELL_WALL_OF_SUDS_DUMMY))
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SUDSY);
                }

                _JustDied();
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                summon->setActive(true);

		        if (me->isInCombat())
                    summon->SetInCombatWithZone();

                if (summon->GetEntry() == NPC_FIZZY_BUBBLE)
                {
                    summon->SetReactState(REACT_PASSIVE);
                    summon->AddAura(SPELL_FIZZY_BUBBLE_VISUAL, summon);
                    summon->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);

                    summon->SetCanFly(true);
                    summon->SetSpeed(MOVE_WALK, 0.3f);
                    summon->SetSpeed(MOVE_RUN, 0.3f);
                    summon->SetSpeed(MOVE_FLIGHT, 0.3f);

                    float x, y, z;
                    summon->GetClosePoint(x, y, z, summon->GetObjectSize() / 3, 2.0f);
                    summon->GetMotionMaster()->MovePoint(1, x, y, z + 30.0f); // Move up 30y slowly, and disappear.
                    summon->DespawnOrUnsummon(20000);
                }

                if (summon->GetEntry() == NPC_WALL_OF_SUDS)
                {
                    summon->SetReactState(REACT_PASSIVE);
                    summon->AddAura(SPELL_WALL_OF_SUDS, summon);
                    summon->SetFlag(UNIT_NPC_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    summon->SetSpeed(MOVE_WALK, 1.2f);
                    summon->SetSpeed(MOVE_RUN, 1.2f);
                    summon->DespawnOrUnsummon(8000);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        // Melee range check.
                        case EVENT_BREW_BOLT:
                            if (!HasHostileInMeleeRange())
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                    DoCast(target, SPELL_BREW_BOLT);
                            events.ScheduleEvent(EVENT_BREW_BOLT, 2500);
                            break;

                        // Bremastery: Stout.
                        case EVENT_BLOAT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_BLOAT);
                            events.ScheduleEvent(EVENT_BLOAT, urand(15500, 17500)); // every 14.5 seconds + 2c.
                            break;
                        case EVENT_BLACKOUT_BREW:
                            DoCast(me, SPELL_BLACKOUT_BREW);
                            events.ScheduleEvent(EVENT_BLACKOUT_BREW, urand(10500, 12500)); // every 10.5 seconds + 1c.
                            break;

                        // Bremastery: Ale.
                        case EVENT_BUBBLE_SHIELD:
                            DoCast(me, SPELL_BUBBLE_SHIELD);
                            events.ScheduleEvent(EVENT_SET_BUBBLE_SHIELD_STACKS, 2100); // 2s cast time.
                            events.ScheduleEvent(EVENT_BUBBLE_SHIELD, urand(43000, 45000)); // every 42 seconds + 2c.
                            break;
                        case EVENT_SET_BUBBLE_SHIELD_STACKS:
                            me->AddAura(SPELL_ROOT, me);
                            SpawnInCircle(1.0f, 8, NPC_BUBBLE_SHIELD, TEMPSUMMON_MANUAL_DESPAWN);
                            if (AuraPtr aura = me->GetAura(SPELL_BUBBLE_SHIELD))
					            me->SetAuraStack(SPELL_BUBBLE_SHIELD, me, 8);
                            break;
                        case EVENT_YEASTY_BREW_ELEMENTALS:
                            for (uint8 i = 0; i < 4; i++)
                                me->SummonCreature(NPC_YEASTY_BREW_ALEMENTAL_Y, me->GetPositionX() + frand(-4.0f, 4.0f), me->GetPositionY() + frand(-4.0f, 4.0f), me->GetPositionZ() + 1.0f, me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN);
                            events.ScheduleEvent(EVENT_YEASTY_BREW_ELEMENTALS, urand(69000, 73000)); // every 71 seconds.
                            break;

                        // Brewmastery: Wheat.
                        case EVENT_CARBONATION:
                            DoCast(me, SPELL_CARBONATION);
                            events.ScheduleEvent(EVENT_CARBONATION, urand(66000, 68000)); // every 64 seconds + 3c.
                            break;
                        case EVENT_WALL_OF_SUDS:
                            SummonSuds(sudWallsCasts % 2 == 0 ? true : false);
                            sudWallsCasts++;
                            events.ScheduleEvent(EVENT_WALL_OF_SUDS, urand(70000, 75000)); // every 72.5 seconds.
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }

    // Particular AI functions.
    private:
        // Check for pets or players in Melee range.
        bool HasHostileInMeleeRange()
        {
            // Check for tank.
            if (Unit* tank = me->getVictim())
                if (tank->IsWithinDistInMap(me, MELEE_RANGE))
                    return true;

            // Check for pets.
            if (Unit* unit = me->SelectNearbyTarget(NULL, MELEE_RANGE))
                if (unit->isPet())
                    return true;

            // Check for players.
            if (Player* nearPlayer = me->FindNearestPlayer(MELEE_RANGE))
                if (nearPlayer->IsWithinDistInMap(me, MELEE_RANGE))
                    if (!nearPlayer->isGameMaster())
                        return true;

            return false;
        }

        // Used for spawning NPC's in a circle around the caster.
        void SpawnInCircle(float row, uint8 summonNumber, uint32 unitEntry, TempSummonType summonType = TEMPSUMMON_MANUAL_DESPAWN, uint32 despawnTimer = 30000)
        {
            float teta = (2 * M_PI) / summonNumber;

            for (uint8 i = 0 ; i < summonNumber ; i++)
            {
                float x = cos(i * teta) * row + me->GetPositionX();
                float y = sin(teta * i) * row + me->GetPositionY();

                if (summonType = TEMPSUMMON_MANUAL_DESPAWN)
                    me->SummonCreature(unitEntry, x, y, me->GetPositionZ() + 2.0f, 0, summonType);
                else
                    me->SummonCreature(unitEntry, x, y, me->GetPositionZ() + 2.0f, 0, summonType, despawnTimer);
            }
        }

        // Used for moving the Wall of Suds.
        void SudsMove(Creature* sudsWall, float X, float Y, float Z)
        {
            Movement::MoveSplineInit init(*sudsWall);
            init.MoveTo(X, Y, Z);
            init.SetOrientationFixed(true);
            init.Launch();
        }

        // Used for summoning the Wall of Suds.
        void SummonSuds(bool right)
        {
            if (right) // Right->Left.
            {
                if (urand(0, 1) == 0) // Bottom->Top.
                {
                    // Need Bottom and Right.
                    if (Creature* sudsBottom = me->SummonCreature(NPC_WALL_OF_SUDS, SudsPositions[3], TEMPSUMMON_MANUAL_DESPAWN))
                    if (Creature* sudsRight  = me->SummonCreature(NPC_WALL_OF_SUDS, SudsPositions[1], TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        SudsMove(sudsBottom, SudsMovePositions[1].GetPositionX(), SudsMovePositions[1].GetPositionY(), SudsMovePositions[1].GetPositionZ());
                        SudsMove(sudsRight, SudsMovePositions[0].GetPositionX(), SudsMovePositions[0].GetPositionY(), SudsMovePositions[0].GetPositionZ());
                    }
				}
                else                  // Top->Bottom.
                {
                    // Need Top and Right.
                    if (Creature* sudsTop = me->SummonCreature(NPC_WALL_OF_SUDS, SudsPositions[0], TEMPSUMMON_MANUAL_DESPAWN))
                    if (Creature* sudsRight  = me->SummonCreature(NPC_WALL_OF_SUDS, SudsPositions[1], TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        SudsMove(sudsTop, SudsMovePositions[0].GetPositionX(), SudsMovePositions[0].GetPositionY(), SudsMovePositions[0].GetPositionZ());
                        SudsMove(sudsRight, SudsMovePositions[0].GetPositionX(), SudsMovePositions[0].GetPositionY(), SudsMovePositions[0].GetPositionZ());
                    }
				}
			}
            else       // Left->Right.
            {
                if (urand(0, 1) == 0) // Bottom->Top.
                {
                    // Need Bottom and Left.
                    if (Creature* sudsBottom = me->SummonCreature(NPC_WALL_OF_SUDS, SudsPositions[3], TEMPSUMMON_MANUAL_DESPAWN))
                    if (Creature* sudsLeft  = me->SummonCreature(NPC_WALL_OF_SUDS, SudsPositions[2], TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        SudsMove(sudsBottom, SudsMovePositions[1].GetPositionX(), SudsMovePositions[1].GetPositionY(), SudsMovePositions[1].GetPositionZ());
                        SudsMove(sudsLeft, SudsMovePositions[1].GetPositionX(), SudsMovePositions[1].GetPositionY(), SudsMovePositions[1].GetPositionZ());
                    }
                }
                else                  // Top->Bottom.
                {
                    // Need Top and Left.
                    if (Creature* sudsTop = me->SummonCreature(NPC_WALL_OF_SUDS, SudsPositions[0], TEMPSUMMON_MANUAL_DESPAWN))
                    if (Creature* sudsLeft  = me->SummonCreature(NPC_WALL_OF_SUDS, SudsPositions[2], TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        SudsMove(sudsTop, SudsMovePositions[0].GetPositionX(), SudsMovePositions[0].GetPositionY(), SudsMovePositions[0].GetPositionZ());
                        SudsMove(sudsLeft, SudsMovePositions[1].GetPositionX(), SudsMovePositions[1].GetPositionY(), SudsMovePositions[1].GetPositionZ());
                    }
                }
			}

            // Add Sudsy aura to players.
            DoCast(me, SPELL_SUDSY);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_yan_zhu_the_uncasked_AI(creature);
    }
};

// Bubble Shield 65522.
class npc_bubble_shield_yanzhu : public CreatureScript
{
    public :
        npc_bubble_shield_yanzhu() : CreatureScript("npc_bubble_shield_yanzhu") { }

        struct npc_bubble_shield_yanzhu_AI : public ScriptedAI
        {
            npc_bubble_shield_yanzhu_AI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            Creature* bossYanzhu;

            void IsSummonedBy(Unit* summoner)
            {
                if (summoner)
                {
                    bossYanzhu = summoner->ToCreature();
                    me->SetFacingTo(summoner->GetOrientation());
                }
                else
                    bossYanzhu = NULL;

                Reset();
            }

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (bossYanzhu)
                {
                    if (AuraPtr bubble = bossYanzhu->GetAura(SPELL_BUBBLE_SHIELD))
                    {
                        if (bubble->GetStackAmount() > 1)
                            bubble->SetStackAmount(bubble->GetStackAmount() - 1);
                        else
                        {
                            bossYanzhu->RemoveAurasDueToSpell(SPELL_BUBBLE_SHIELD);
                            bossYanzhu->RemoveAurasDueToSpell(SPELL_ROOT);
                        }
                    }
                }
            }

            void UpdateAI(const uint32 diff) { } // No melee.
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_bubble_shield_yanzhu_AI(creature);
        }
};

// Fizzy Bubble 59799.
class npc_fizzy_bubble_yanzhu : public CreatureScript
{
    public:
        npc_fizzy_bubble_yanzhu() : CreatureScript("npc_fizzy_bubble_yanzhu") { }

        struct npc_fizzy_bubble_yanzhuAI : public PassiveAI
        {
            npc_fizzy_bubble_yanzhuAI(Creature* creature) : PassiveAI(creature) { }

            void OnSpellClick(Unit* clicker)
            {
                clicker->AddAura(SPELL_FIZZY_BUBBLE, clicker);
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                me->DespawnOrUnsummon(100);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_fizzy_bubble_yanzhuAI(creature);
        }
};

// Yeasty Brew Alemental 66413.
class npc_yeasty_brew_elemental_yanzhu : public CreatureScript
{
    public :
        npc_yeasty_brew_elemental_yanzhu() : CreatureScript("npc_yeasty_brew_elemental_yanzhu") { }

        struct npc_yeasty_brew_elemental_yanzhu_AI : public ScriptedAI
        {
            npc_yeasty_brew_elemental_yanzhu_AI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            EventMap events;
            InstanceScript* instance;

            void IsSummonedBy(Unit* summoner)
            {
                DoCast(me, SPELL_YEASTY_SUMMON_VIS);
                Reset();
            }

            void Reset()
            {
                events.Reset();
                events.ScheduleEvent(EVENT_YEASTY_BREW_BOLT, urand(2000, 5500));
                events.ScheduleEvent(EVENT_FERMENT, urand(8000, 19000));
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_YEASTY_BREW_BOLT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_YEASTY_BREW_BOLT);
                            events.ScheduleEvent(EVENT_YEASTY_BREW_BOLT, 3000);
                            break;

                        case EVENT_FERMENT:
                            if (Creature* yanzhu = me->FindNearestCreature(BOSS_YANZHU_THE_UNCASKED, 100.0f, true))
                                DoCast(yanzhu, SPELL_FERMENT);
                            events.ScheduleEvent(EVENT_FERMENT, urand(21000, 32000));
                            break;
                    }
                }
                // No melee.
            }
        };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_yeasty_brew_elemental_yanzhu_AI(creature);
    }
};

// Bloat 106546.
class spell_yanzhu_bloat : public SpellScriptLoader
{
    public:
        spell_yanzhu_bloat() :  SpellScriptLoader("spell_yanzhu_bloat") { }

        class spell_yanzhu_bloat_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_yanzhu_bloat_AuraScript);

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster() && GetTarget())
                {
                    if (Unit* target = GetTarget())
                    {
                        // Summon 2 Bloated Stalkers that enter the player turned into a vehicle by the aura, and cast on the sides.
                        for (uint8 i = 0; i < 2; i++)
                        {
                            if (Creature* bloatStalker = GetCaster()->SummonCreature(NPC_BLOATED_STALKER, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                if (AuraPtr bloat = GetCaster()->AddAura(SPELL_BLOATED, bloatStalker))
                                {
                                    bloat->SetMaxDuration(GetMaxDuration());
                                    bloat->SetDuration(GetDuration());
                                }
                                bloatStalker->SetReactState(REACT_PASSIVE);
                                bloatStalker->DespawnOrUnsummon(31000);
                                bloatStalker->EnterVehicle(target); // Enter player.
                                bloatStalker->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_yanzhu_bloat_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_SCALE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_yanzhu_bloat_AuraScript();
        }
};

// Gushing Brew (Bloated trigger spell) 106560.
class spell_yanzhu_gushing_brew : public SpellScriptLoader
{
    public:
        spell_yanzhu_gushing_brew() : SpellScriptLoader("spell_yanzhu_gushing_brew") { }

        class spell_yanzhu_gushing_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_yanzhu_gushing_brew_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                Map* map = GetCaster()->GetMap();
                if (map && map->IsDungeon())
                {
                    targets.clear();
                    std::list<Player*> playerList;
                    Map::PlayerList const& players = map->GetPlayers();
                    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                        if (Player* player = itr->getSource())
                            if (GetCaster()->isInFront(player, M_PI / 3))
                                targets.push_back(player);

                    if (targets.empty())
                        return;
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_yanzhu_gushing_brew_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENTRY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_yanzhu_gushing_brew_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_CONE_ENTRY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_yanzhu_gushing_brew_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_CONE_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_yanzhu_gushing_brew_SpellScript();
        }
};

// Blackout Brew 106851.
class spell_yanzhu_blackout_brew : public SpellScriptLoader
{
    public :
        spell_yanzhu_blackout_brew() : SpellScriptLoader("spell_yanzhu_blackout_brew") { }

        class spell_yanzhu_blackout_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_yanzhu_blackout_brew_SpellScript)

            void HandleOnHit()
            {
                if (!GetHitUnit())
                    return;

                if (AuraPtr blackoutBrew = GetHitUnit()->GetAura(SPELL_BLACKOUT_BREW)) // Get the aura.
                {
                    if (blackoutBrew->GetStackAmount() < 7) // Adds 3 so must check for 6 or less.
                        blackoutBrew->SetStackAmount(blackoutBrew->GetStackAmount() + 2); // Add 3 stacks. 1 is already added from the aura so we add 2 more.
                    else
                    {
                        GetHitUnit()->AddAura(SPELL_BLACKOUT_DRUNK, GetHitUnit()); // Stun the player.
                        GetHitUnit()->RemoveAurasDueToSpell(SPELL_BLACKOUT_BREW); // Remove all the aura stacks.
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_yanzhu_blackout_brew_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_yanzhu_blackout_brew_SpellScript();
        }

        class spell_yanzhu_blackout_brew_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_yanzhu_blackout_brew_AuraScript)

            void OnPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                // Remove a stack if the player is moving or jumping (Jumping checked by MOVEMENTFLAG_FALLING and PositionZ compared to boss one plus a small margin).
                if (GetTarget()->isMoving() || GetTarget()->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) || GetTarget()->GetPositionZ() > GetCaster()->GetPositionZ() + 0.1f)
                    if (AuraPtr blackoutBrew = GetTarget()->GetAura(SPELL_BLACKOUT_BREW)) // Get the aura.
                        if (blackoutBrew->GetStackAmount() > 1)
                            blackoutBrew->SetStackAmount(blackoutBrew->GetStackAmount() - 1); // Remove a stack till the base one is left.
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_yanzhu_blackout_brew_AuraScript::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_yanzhu_blackout_brew_AuraScript();
        }
};

// Ferment 106859.
class spell_yeasty_alemental_ferment : public SpellScriptLoader
{
    public:
        spell_yeasty_alemental_ferment() : SpellScriptLoader("spell_yeasty_alemental_ferment") { }

        class spell_yeasty_alemental_ferment_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_yeasty_alemental_ferment_AuraScript);

            void HandlePeriodicTick(constAuraEffectPtr /*aurEff*/)
            {
                PreventDefaultAction();

                if (Unit* caster = GetCaster())
                {
	                if (Creature* Yanzhu = caster->FindNearestCreature(BOSS_YANZHU_THE_UNCASKED, 100.0f, true))
                    {
                        std::list<Player*> playerTargetsList;
                        Map::PlayerList const &PlayerList = caster->GetMap()->GetPlayers();
                        if (!PlayerList.isEmpty())
                            for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                if (Player* player = i->getSource())
                                    if (player->IsInBetween(caster, Yanzhu, 1.5f)) // Check for players between the npc and the boss.
                                        playerTargetsList.push_back(player);

                        if (!playerTargetsList.empty()) // If the list is not empty select only the nearest player in line with the boss.
                        {
                            playerTargetsList.sort(SkyMistCore::ObjectDistanceOrderPred(caster));
                            playerTargetsList.resize(1);
                            caster->CastSpell(playerTargetsList.front(), SPELL_FERMENT_HEAL, true);
                        }
                        else
                            caster->CastSpell(Yanzhu, SPELL_FERMENT_HEAL, true);
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_yeasty_alemental_ferment_AuraScript::HandlePeriodicTick, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_yeasty_alemental_ferment_AuraScript();
        }
};

// Carbonation 115003 - Summon Fizzy Bubbles script.
class spell_yanzhu_carbonation : public SpellScriptLoader
{
    public:
        spell_yanzhu_carbonation() : SpellScriptLoader("spell_yanzhu_carbonation") { }

        class spell_yanzhu_carbonation_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_yanzhu_carbonation_AuraScript)

            void PeriodicTick(constAuraEffectPtr /*aurEff*/)
            {
                for (uint8 i = 0; i < 2; i++)
                    if (Unit* caster = GetCaster())
                        caster->SummonCreature(NPC_FIZZY_BUBBLE, caster->GetPositionX() + frand(-13.0f, 13.0f), caster->GetPositionY() + frand(-13.0f, 13.0f), caster->GetPositionZ() + 1.0f,  caster->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_yanzhu_carbonation_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_yanzhu_carbonation_AuraScript();
        }
};

// Wall of Suds 114467.
class spell_wall_of_suds : public SpellScriptLoader
{
    public:
        spell_wall_of_suds() : SpellScriptLoader("spell_wall_of_suds") { }

        class spell_wall_of_suds_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_wall_of_suds_AuraScript);

            void HandlePeriodicTick(constAuraEffectPtr /*aurEff*/)
            {
                PreventDefaultAction();

                if (Unit* caster = GetCaster())
                {
                    Map::PlayerList const &PlayerList = caster->GetMap()->GetPlayers();
                    if (!PlayerList.isEmpty())
                        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                            if (Player* player = i->getSource())
                                if (caster->isInFront(player, M_PI / 5) && !player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) && player->GetPositionZ() <= caster->GetPositionZ() + 0.1f)
                                    caster->CastSpell(player, SPELL_WALL_OF_SUDS_DAMAGE, true);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_wall_of_suds_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_wall_of_suds_AuraScript();
        }
};

// Sudsy 114468 - Player Jump aura script.
class spell_yanzhu_sudsy : public SpellScriptLoader
{
    public:
        spell_yanzhu_sudsy() : SpellScriptLoader("spell_yanzhu_sudsy") { }

        class spell_yanzhu_sudsy_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_yanzhu_sudsy_AuraScript);

            bool jumpedHigh;

            bool Load()
            {
                jumpedHigh = false;
                return true;
            }

            void HandlePeriodicTick(constAuraEffectPtr /*aurEff*/)
            {
                PreventDefaultAction();

                Unit* caster = GetCaster();
                Unit* target = GetTarget();
                if (!caster || !target)
                    return;

                if (target->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) && !jumpedHigh)
                {
                    target->CastSpell(target, SPELL_SUDSY_JUMP, true);
                    jumpedHigh = true;
			    }

                if (!target->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) && target->isMoving() && jumpedHigh)
                    jumpedHigh = false;
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_yanzhu_sudsy_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_yanzhu_sudsy_AuraScript();
        }
};

void AddSC_boss_yan_zhu_the_uncasked()
{
    new boss_yan_zhu_the_uncasked();
    new npc_bubble_shield_yanzhu();
    new npc_fizzy_bubble_yanzhu();
    new npc_yeasty_brew_elemental_yanzhu();
    new spell_yanzhu_bloat();
    new spell_yanzhu_gushing_brew();
    new spell_yanzhu_blackout_brew();
    new spell_yeasty_alemental_ferment();
    new spell_yanzhu_carbonation();
    new spell_wall_of_suds();
    new spell_yanzhu_sudsy();
}
