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
 * Boss: Ook-Ook.
 */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "Vehicle.h"

#include "stormstout_brewery.h"

enum Yells
{
    SAY_INTRO                    = 0, // Who crashing Ook party!? Ook ook ook...
    SAY_AGGRO                    = 1, // Me gonna ook you in the ooker!
    SAY_KILL                     = 2, // In the ooker!
    SAY_BANANAS                  = 3, // 0 - Get Ooking party started! ; 1 - Come on and get your Ook on! ; 2 - We're gonna Ook all night!
    SAY_DEATH                    = 4  // Ook ! Oooook !
};

#define ANN_BANANAS "Ook-Ook is Going Bananas! More barrels are coming!"

enum Spells
{
    // Boss
    SPELL_GROUND_POUND           = 106807, // Aura.
    SPELL_GOING_BANANAS          = 106651, // Aura.
    SPELL_GOING_BANANAS_DUMMY    = 115978, // Throws bananas around :).

    // NPCs
    SPELL_BARREL_COSMETIC        = 106647, // Visual spell, triggers 106672 dummy each 300 ms.

    SPELL_BARREL_EXPLOSION       = 107016, // Explosion on players. Triggered by 115868 periodic aura.
    SPELL_BARREL_EXPLOSION_O     = 106784, // Explosion on Ook-Ook. Triggered by 115875, triggered by 115907 periodic aura.

    SPELL_BARREL_EXPLOSION_M_O   = 106769, // Explosion on monkeys (eff 0) && Ook-Ook (eff 1). Triggered by 106768 periodic aura.
    SPELL_BARREL_RIDE            = 106614, // Vehicle ride spell, triggers 106768, triggering 106769.
    SPELL_FORCECAST_BARREL_DROP  = 122385, // Triggers SPELL_BARREL_DROP.
    SPELL_BARREL_DROP            = 122376  // Jump spell for Barrel NPC.
};

enum Events
{
    // Boss
    EVENT_GROUND_POUND           = 1,
    EVENT_GOING_BANANAS,

    // NPCs
    EVENT_EXPLODE
};

enum GoingBananasStates
{
    DONE_NONE              = 0, // No casts done.
    DONE_90                = 1, // First cast done.
    DONE_60                = 2, // Second cast done.
    DONE_30                = 3  // All casts done.
};

// Barrel summon positions.
Position const barrelPos[6] = 
{
    { -783.203f, 1365.502f, 146.727f },     // South right.
    { -734.282f, 1378.475f, 146.714f },     // South left.
    { -779.212f, 1355.871f, 146.773f },     // North right.
    { -730.420f, 1360.788f, 146.708f },     // North left.
    { -736.140f, 1337.725f, 146.722f },     // Up right.
    { -765.841f, 1331.986f, 146.724f },     // Up left.
};

class boss_ook_ook : public CreatureScript
{
    public:
        boss_ook_ook() : CreatureScript("boss_ook_ook") { }

        struct boss_ook_ook_AI : public BossAI
        {
            boss_ook_ook_AI(Creature* creature) : BossAI(creature, DATA_OOKOOK_EVENT), summons(me)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
            SummonList summons;
            GameObject* OokDoor;
            uint8 goingBananasDone;
            bool summonedBarrels; // For Going Bananas phases.

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Talk(SAY_INTRO);
                Reset();
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();
                summonedBarrels = false;
                OokDoor = NULL;

                if (instance)
                    instance->SetData(DATA_OOKOOK_EVENT, NOT_STARTED);

                goingBananasDone = DONE_NONE;

                _Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);

                if (instance)
                {
                    instance->SetData(DATA_OOKOOK_EVENT, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
                }

                if (GameObject* door = me->SummonGameObject(GAMEOBJECT_OOK_OOK_DOOR, -767.094f, 1392.12f, 146.742f, 0.298132f, 0.0f, 0.0f, 0.148514f, 0.98891f, 0))
                {
                    OokDoor = door;
                    door->SetGoState(GO_STATE_READY);
                }

                events.ScheduleEvent(EVENT_GROUND_POUND, 10000);

                _EnterCombat();
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void EnterEvadeMode()
            {
                if (OokDoor)
                {
                    OokDoor->RemoveFromWorld();
                    OokDoor = NULL;
                }

                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                {
                    instance->SetData(DATA_OOKOOK_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }

                _EnterEvadeMode();
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                summons.DespawnAll();

                if (OokDoor)
                {
                    OokDoor->RemoveFromWorld();
                    OokDoor = NULL;
                }

                if (instance)
                {
                    instance->SetData(DATA_OOKOOK_EVENT, DONE);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }

                _JustDied();
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                summon->setActive(true);

		        if (me->isInCombat())
                    summon->SetInCombatWithZone();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                // Set Going Bananas phases execution.
                if (me->HealthBelowPct(91) && goingBananasDone == DONE_NONE || me->HealthBelowPct(61) && goingBananasDone == DONE_90 || me->HealthBelowPct(31) && goingBananasDone == DONE_60)
                {
                    events.ScheduleEvent(EVENT_GOING_BANANAS, 1000);
                    goingBananasDone++;
                }

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_GROUND_POUND:
                            DoCast(me, SPELL_GROUND_POUND);
                            events.ScheduleEvent(EVENT_GROUND_POUND, DUNGEON_MODE(10000, 7000));
                            break;

                        case EVENT_GOING_BANANAS:
                            me->MonsterTextEmote(ANN_BANANAS, NULL, true);
                            Talk(SAY_BANANAS);
                            summonedBarrels = false;
                            DoCast(me, SPELL_GOING_BANANAS);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_ook_ook_AI(creature);
        }
};

// Brew Barrel 56682.
class npc_ook_barrel : public CreatureScript
{
    public:
        npc_ook_barrel() : CreatureScript("npc_ook_barrel") { }

        struct npc_ook_barrel_AI : public ScriptedAI
        {
            npc_ook_barrel_AI(Creature* creature) : ScriptedAI(creature), vehicle(creature->GetVehicleKit())
            {
                ASSERT(vehicle);
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            Vehicle* vehicle;
            EventMap events;
            bool exploded, hasPassenger;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
            }

            void Reset()
            {
                events.Reset();
                me->SetReactState(REACT_PASSIVE);
                me->AddAura(SPELL_BARREL_COSMETIC, me);
                me->SetSpeed(MOVE_WALK, 0.7f);
                me->SetSpeed(MOVE_RUN, 0.7f);

                exploded = false;
                hasPassenger = false;

                float x, y, z;
                me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 50.0f);
                me->GetMotionMaster()->MovePoint(1, x, y, z);
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (!me->isAlive() || type != POINT_MOTION_TYPE || id != 1)
                    return;

                // We call a move stop and inform once a player boards it, so let's prevent it from exploding.
                if (hasPassenger)
                    return;

                events.ScheduleEvent(EVENT_EXPLODE, 500);
            }

            void OnCharmed(bool apply)
            {
                if (apply)
                {
                    hasPassenger = true;
                    me->StopMoving();
                }
                else events.ScheduleEvent(EVENT_EXPLODE, 500);
            }

            void UpdateAI(const uint32 diff)
            {
                if (CheckIfAgainstWall() || CheckIfAgainstPlayer())
                    DoExplode(true);
                else if (CheckIfAgainstOokOok())
                    DoExplode(false);

                events.Update(diff);

                while(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_EXPLODE:
                            DoExplode(true);
                            break;

                        default: break;
                    }
                }
            }

        // Particular functions here.
        private:
            bool CheckIfAgainstWall()
            {
                // ToDo : Fix this.
                // float x, y, z;
                // me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 5.0f);
                // if (!me->IsWithinLOS(x, y, z))
                //     return true;

                return false;
            }

            bool CheckIfAgainstOokOok()
            {
                if (hasPassenger && me->FindNearestCreature(BOSS_OOKOOK, 1.0f, true))
                    return true;

                return false;
            }

            bool CheckIfAgainstPlayer()
            {
                if (!hasPassenger)
                    if (Player* nearPlayer = me->FindNearestPlayer(1.0f))
                        if (nearPlayer->IsWithinDistInMap(me, 1.0f) && me->isInFront(nearPlayer, M_PI / 3))
                            return true;

                return false;
            }

            void DoExplode(bool onPlayer = true)
            {
                if (!exploded)
                {
                    if (onPlayer) // On players.
                        DoCast(me, SPELL_BARREL_EXPLOSION);
                    else // On Ook-Ook.
                    {
                        if (Vehicle* barrel = me->GetVehicleKit())
                            barrel->RemoveAllPassengers();

                        DoCast(me, SPELL_BARREL_EXPLOSION_O);
                    }

                    exploded = true;
                    hasPassenger = false;
                    me->DespawnOrUnsummon(200);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ook_barrel_AI(creature);
        }
};

// Going Bananas 115978.
class spell_ook_ook_going_bananas_summon : public SpellScriptLoader
{
public :
    spell_ook_ook_going_bananas_summon() : SpellScriptLoader("spell_ook_ook_going_bananas_summon") { }

    class spell_ook_ook_going_bananas_summon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ook_ook_going_bananas_summon_SpellScript)

        bool Validate(const SpellInfo* spellInfo)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_GOING_BANANAS_DUMMY))
                return false;

            return true;
        }

        void HandleDummy(SpellEffIndex effIndex) // This comes periodic, so only do it once per phase.
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (CAST_AI(boss_ook_ook::boss_ook_ook_AI, GetCaster()->ToCreature()->AI())->summonedBarrels == false)
                {
                    for (uint8 i = 0; i < 6; i++)
                        if (Creature* OokOok = GetCaster()->ToCreature())
                            OokOok->SummonCreature(NPC_OOK_BARREL, barrelPos[i], TEMPSUMMON_MANUAL_DESPAWN);
                    CAST_AI(boss_ook_ook::boss_ook_ook_AI, GetCaster()->ToCreature()->AI())->summonedBarrels = true;
                }
            }
        }

        void Register()
        {
            OnEffectLaunch += SpellEffectFn(spell_ook_ook_going_bananas_summon_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ook_ook_going_bananas_summon_SpellScript();
    }
};

// Ground Pound triggered spell 106808.
class spell_ook_ook_ground_pound_dmg : public SpellScriptLoader
{
public:
    spell_ook_ook_ground_pound_dmg() : SpellScriptLoader("spell_ook_ook_ground_pound_dmg") { }

    class spell_ook_ook_ground_pound_dmgSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ook_ook_ground_pound_dmgSpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
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
            }
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ook_ook_ground_pound_dmgSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ook_ook_ground_pound_dmgSpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_CONE_ENEMY_104);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ook_ook_ground_pound_dmgSpellScript();
    }
};

// Brew Explosion Ook-Ook 106784.
class spell_brew_explosion : public SpellScriptLoader
{
    public:
        spell_brew_explosion() : SpellScriptLoader("spell_brew_explosion") { }

        class spell_brew_explosion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_brew_explosion_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.clear();
                if (Creature* Ook = GetCaster()->FindNearestCreature(BOSS_OOKOOK, 3.0f, true))
                    targets.push_back(Ook);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brew_explosion_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brew_explosion_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_brew_explosion_SpellScript();
        }
};

void AddSC_boss_ook_ook()
{
	new boss_ook_ook();
    new npc_ook_barrel();
    new spell_ook_ook_going_bananas_summon();
    new spell_ook_ook_ground_pound_dmg();
    new spell_brew_explosion();
}
