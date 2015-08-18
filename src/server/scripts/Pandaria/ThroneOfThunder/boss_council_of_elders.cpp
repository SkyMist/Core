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
 * Raid: Throne of Thunder.
 * Boss: Council of Elders.
 *
 * Wowpedia boss history:
 *
 * "The history of the troll tribes -- the Drakkari, the Farraki, the Amani, and the Gurubashi -- 
 *  is awash with millennia of betrayal and conflict, but the Zandalari's promise of a new, unstoppable empire has finally united these disparate troll leaders."
 */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "Spell.h"
#include "Vehicle.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CreatureTextMgr.h"
#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Map.h"
#include "VehicleDefines.h"
#include "SpellInfo.h"

#include "throne_of_thunder.h"

enum Yells
{
    /*** Bosses ***/

    // Spirit of Gara'jal (Gara'jal the Spiritbinder)

    SAY_INTRO                   = 0, // Witness the powa of the Spiritbinder!

    // Kazra'jin

    SAY_KAZ_AGGRO,                   // Dis is gonna hurt!
    SAY_KAZ_KILL,                    // 0 - Da Amani empire cannot be stopped! ; 1 - On ya knees!
    SAY_KAZ_DEATH,                   // Da thunder king... promised...
    SAY_KAZ_POSSESSED,               // Ya shouldn't be messin' wit da Zandalari!

    SAY_RECKLESS_CHARGE,             // 0 - Incoming! ; 1 - Out da way!
    SAY_DISCHARGE,                   // Shocking!

    // Sul the Sandcrawler

    SAY_SUL_AGGRO,                   // Da sands will consume everyting!
    SAY_SUL_KILL,                    // 0 - Da sands are endless. ; 1 - Da first of many!
    SAY_SUL_DEATH,                   // I return... to... da... sands...
    SAY_SUL_POSSESSED,               // I will bury ya all!

    SAY_SANDSTORM,                   // Da storm approaches!
    SAY_QUICKSAND,                   // Watch yer step!
    SAY_TREACHEROUS_GROUND,          // Dey tink dey can stop da Farraki?

    // Frost King Malakk

    SAY_MAL_AGGRO,                   // Ya have met your match, fools!
    SAY_MAL_KILL,                    // 0 - Death's cold embrace. ; 1 - Witness da Drakkari's might.
    SAY_MAL_DEATH,                   // Da... empire... can't... fall...
    SAY_MAL_POSSESSED,               // Winter is coming...

    SAY_FROSTBITE,                   // 0 - Freeze! ; 1 - Getting cold?
    SAY_BODY_HEAT,                   // Da' Drakkari will never fall to de' likes of you!

    // High Priestess Mar'li

    SAY_MAR_AGGRO,                   // Death ta all who appose da empire!
    SAY_MAR_KILL,                    // 0 - She will feast on yer soul! ; 1 - Another offering to da loa!
    SAY_MAR_DEATH,                   // Shadra... save... me...
    SAY_MAR_POSSESSED,               // Da spiritbinder reveals yer soul ta me!

    SAY_LOA_SPIRIT                   // 0 - Yer soul belongs ta me! ; 1 - Succumb ta her venom! ; 2 - Embrace yer demise! ; 3 - Shadra's rage will destroy you!
};

enum Spells
{
    /*** Bosses ***/

    // Spirit of Gara'jal (Gara'jal the Spiritbinder)

    SPELL_INTRO_VISUAL          = 144465,
    SPELL_WISP_FORM             = 90793,

    SPELL_POSSESS               = 136442, // Possess councilor spell. Generating Dark Energy, 25% size increase. Periodic dummy effect 1.
    SPELL_LINGERING_PRESENCE    = 136467, // Increases damage done by 10%, Dark Energy generation rate by 10%. Cancels Possessed aura - not working. Use by adding aura.

    SPELL_SOUL_FRAGMENT         = 137641, // HEROIC only. Mechanic abilities override aura + Shadowed Soul (137650) trigger each 5 seconds. Every second time (Normal) / Every time (25 Heroic) spirit changes possess target.
    SPELL_SOUL_FRAGMENT_PASS    = 137643, // On button click to pass SPELL_SOUL_FRAGMENT to another player (trigger spell from target - not working, needs script).

    // Kazra'jin - Mostly no tank needed, casts Reckless Charge all the time.

    SPELL_RECKLESS_CHARGE       = 137117, // Cast time and roll visual. 5 sec duration.
    SPELL_RECKLESS_CHARGE_VIS   = 138026, // Creates areatrigger to the location where he will charge and on path. 2 sec duration.
    SPELL_RECKLESS_CHARGE_T_DMG = 137122, // Target reach damage and knockback.
    SPELL_RECKLESS_CHARGE_DMG   = 137133, // On players encountered while moving to target.

    SPELL_OVERLOAD              = 137149, // Dummy duration aura, applies dummy aura on eff 0 for SPELL_OVERLOAD_REFLECT_DMG (40% dmg reflected). Boss should be "stunned" (passive). When Possesed.
    SPELL_OVERLOAD_REFLECT_DMG  = 137151, // Damage reflected spell.

    SPELL_DISCHARGE             = 137166, // HEROIC only. Replaces Overload. Dummy duration aura, applies dummy aura on eff 0 for SPELL_DISCHARGE_REFLECT_DMG (5% dmg reflected to raid). Boss "stunned" (passive). When Possesed.
    SPELL_DISCHARGE_REFLECT_DMG = 136935, // Damage reflected spell.

    // Sul the Sandcrawler - Mostly no tank needed, casts Sand Bolt all the time.

    SPELL_SAND_BOLT             = 136189, // Triggers 136190 damage in 5 yards.

    SPELL_QUICKSAND             = 136521, // Cast time + eff 0 triggering of SPELL_ENTRAPPED, stun 8 seconds on target player (who is at location where npc will spawn).

    SPELL_SANDSTORM             = 136894, // Replaces Quicksand. Cast time and periodic damage spell. Despawns Quicksand and creates NPC_LIVING_SAND. Possessed version.
    SPELL_SANDSTORM_VISUAL      = 136895, // Boss visual.

    // Frost King Malakk

    SPELL_FRIGID_ASSAULT        = 136904, // Aura for 75k frost damage / melee triggered on eff 1, and stack add SPELL_FRIGID_ASSAULT_STACKS on eff 0.
    SPELL_FRIGID_ASSAULT_STACKS = 136903, // Periodic dummy eff 0 for checking stacks. At 15 stacks player stunned by SPELL_FRIGID_ASSAULT_STUN.
    SPELL_FRIGID_ASSAULT_STUN   = 136910, // 15 second stun.

    SPELL_BITING_COLD           = 136992, // 94 k dmg, triggers periodic aura on player.
    SPELL_BITING_COLD_VISUAL    = 137579, // Visual dummy. Put on player with add aura.

    SPELL_FROSTBITE             = 136922, // Replaces Biting Cold. Added in 5 stacks to a player. Damage 22k / stack players in 4 yards, and removes 2 (1 in 25 - player) stacks for each. Can't remove fully. Possessed version.
    SPELL_BODY_HEAT             = 137084, // HEROIC only. On player in 4 yards when hit by frostbite. Applies SPELL_CHILLED_TO_THE_BONE aura after 8 sec, and player cannot help remove Frostbite.
    SPELL_CHILLED_TO_THE_BONE   = 137085, // Prevents players from removing Frostbite stacks for 8 sec.

    // High Priestess Mar'li

    SPELL_WRATH_OF_THE_LOA      = 137344, // 133000 to 147000 Holy damage.
    SPELL_BLESSED_LOA_SPIRIT    = 137203, // Main fake cast time spell. Eff 0 dummy for SPELL_BLESSED_LOA_SPIRIT_S.
    SPELL_BLESSED_LOA_SPIRIT_S  = 137200, // Summons NPC_BLESSED_LOA_SPIRIT.

    SPELL_WRATH_OF_THE_LOA_POSS = 137347, // 152625 to 177375 Shadow damage. Possessed version.
    SPELL_SHADOWED_LOA_SPIRIT   = 137350, // Replaces Blessed Loa Spirit. Main fake cast time spell. Eff 0 dummy for SPELL_SHADOWED_LOA_SPIRIT_S. Possessed version.
    SPELL_SHADOWED_LOA_SPIRIT_S = 137351, // Summons NPC_SHADOWED_LOA_SPIRIT. Possessed version.

    SPELL_TWISTED_FATE          = 137891, // HEROIC only. Replaces Shadowed Loa Spirit summoning. Cast time and eff 0 spell trigger (for SPELL_TWISTED_FATE_S_1). Possessed version.
    SPELL_TWISTED_FATE_S_1      = 137893, // Summons NPC_TWISTED_FATE_1.
    SPELL_TWISTED_FATE_S_2      = 137963, // Summons NPC_TWISTED_FATE_2.
    SPELL_TWISTED_FATE_VISUAL   = 137967, // Visual between the two npc's. Dummy eff 0 (maybe for checking distance between them).
    SPELL_TWISTED_FATE_DMG_A    = 137986, // Damage aura on each mob, triggers SPELL_TWISTED_FATE_DMG to the whole raid every 3 sec.
    SPELL_TWISTED_FATE_DMG      = 137972, // Calculation is done according to distance between npc's (further = less dmg to raid). If one dies, 100k dmg / 3 sec to all raid by remaning one.

    // General / Misc

    SPELL_DARK_POWER            = 136507, // At 100 Dark Energy, raid damage and increase.
    SPELL_ZERO_POWER            = 72242,  // No Regen
    SPELL_BERSERK               = 144369, // Berserk, Enrage, Bye - Bye or, simply put, a wipe. :)

    /*** Adds ***/

    SPELL_QUICKSAND_VISUAL      = 136851, // Visual aura. Dummy aura on effect 0 for SPELL_QUICKSAND_DMG / SPELL_ENSNARED for players entering it.
    SPELL_QUICKSAND_DMG         = 136860, // Periodic damage aura on player.
    SPELL_QUICKSAND_SPAWN_EFF   = 137759, // Spawn effect made by npc with DoCast.
    SPELL_TREACHEROUS_GROUND_QS = 137629, // HEROIC only. 50% Quicksand increase size, it despawns older Quicksand in radius after spawn and adds this aura (they "merge").

    SPELL_ENSNARED              = 136878, // 15% speed decrease each sec for players in Quicksand. On 5 stacks causes SPELL_ENTRAPPED.
    SPELL_ENTRAPPED             = 136857, // At 5 stacks or if player is at Quicksand npc location when it spawns. Used with add aura for 30 sec stun.

    SPELL_FORTIFIED             = 136864, // Living Sand alive when Sandstorm hits cast this on them, full heal + size + dmg done increase.
    SPELL_TREACHEROUS_GROUND    = 137614, // HEROIC only. Living Sand + 125% health and damage done aura if Quicksands combine (if Quicksand has SPELL_TREACHEROUS_GROUND_QS aura when Sandstorm hits).

    SPELL_BLESSED_GIFT          = 137303, // Blessed Loa Spirit heal on councilor 5% hp.
    SPELL_SHADOWED_GIFT         = 137390, // Shadowed Loa Spirit 10 mil damage in 6 yards.
    SPELL_MARKED_SOUL           = 137359, // Fixated spell for Shadowed Loa Spirit aura. Pet control (used with add aura).

    SPELL_TWISTED_FATE_CLONE_1  = 137950, // Player target cast on NPC_TWISTED_FATE_1 when it spawns.
    SPELL_TWISTED_FATE_CLONE_2  = 137965  // Player target cast on NPC_TWISTED_FATE_2 when it spawns.
};

enum Events
{
    /*** Bosses ***/

    // Spirit of Gara'jal (Gara'jal the Spiritbinder)

    EVENT_KAZRAJIN_SAY_AGRRO    = 1,
    EVENT_SUL_SAY_AGRRO,
    EVENT_MALAKK_SAY_AGRRO,
    EVENT_MARLI_SAY_AGRRO,

    EVENT_POSSESS_NEXT,                   // When current possess target health dropped by 25%.

    // Kazra'jin

    EVENT_RECKLESS_CHARGE,
    EVENT_RECKLESS_CHARGE_MOVE,
    EVENT_SUMMON_RECKLESS_CHARGE_VIS,

    EVENT_OVERLOAD_DISCHARGE,             // Discharge is HEROIC only.
    EVENT_R_OVERLOAD_DISCHARGE,

    // Sul the Sandcrawler

    EVENT_SAND_BOLT,
    EVENT_QUICKSAND,

    // Frost King Malakk

    EVENT_FRIGID_ASSAULT,
    EVENT_BITING_COLD_FROSTBITE,

    // High Priestess Mar'li

    EVENT_WRATH_OF_THE_LOA,
    EVENT_BS_LOA_SPIRIT_FATE,             // Twisted Fate is HEROIC only.               

    // General / Misc

    EVENT_DARK_POWER,
    EVENT_BERSERK,

    /*** Adds ***/

    EVENT_ENSNARE_ENTRAPPED,
    EVENT_CHECK_TREACH_GROUND,
    EVENT_LOA_SPIRIT_JUMP_KILL
};

enum Timers
{
    /*** Bosses ***/

    // Spirit of Gara'jal (Gara'jal the Spiritbinder)

    TIMER_KAZRAJIN_SAY_AGRRO    = 4000,
    TIMER_SUL_SAY_AGRRO         = 4000,
    TIMER_MALAKK_SAY_AGRRO      = 4000,
    TIMER_MARLI_SAY_AGRRO       = 4000,

    TIMER_POSSESS_NEXT          = 2000,

    // Kazra'jin

    TIMER_RECKLESS_CHARGE_F     = 10000,
    TIMER_RECKLESS_CHARGE_S     = 6000,

    TIMER_RECKLESS_CHARGE_MOVE  = 1100,
    TIMER_SUMM_R_CHARGE_VIS     =  250,

    TIMER_OVERLOAD_DISCHARGE    = 1000,
    TIMER_R_OVERLOAD_DISCHARGE  = 20100,

    // Sul the Sandcrawler

    TIMER_SAND_BOLT             = 2500,

    TIMER_QUICKSAND_F           = 8000,
    TIMER_QUICKSAND_S           = 35000,

    // Frost King Malakk

    TIMER_FRIGID_ASSAULT        = 30000,

    TIMER_BITING_COLD_F         = 13000,
    TIMER_BITING_COLD_S         = 45000,

    // High Priestess Mar'li

    TIMER_WRATH_OF_THE_LOA      = 2500,

    TIMER_LOA_SPIRIT_F          = 25000,
    TIMER_LOA_SPIRIT_S          = 34000,

    // General / Misc

    TIMER_DARK_POWER            = 1000,
    TIMER_BERSERK               = 720000, // 12 minutes.

    /*** Adds ***/

    TIMER_ENSNARE_ENTRAPPED     = 1000,
	TIMER_CHECK_TREACH_GROUND   = 500,
    TIMER_LOA_SPIRIT_JUMP_KILL  = 20000
};

enum Actions
{
    ACTION_POSSESS_NEXT         = 1,
    ACTION_SUMMON_LIVING_SAND
};

enum Npcs
{
    NPC_SPIRIT_OF_GARAJAL       = 69135,

    NPC_RECKLESS_CHARGE         = 69453, // Boss summons this and charges towards it.

    NPC_QUICKSAND               = 69150, // Quicksand npc.
    NPC_LIVING_SAND             = 69153, // Spawns from Quicksand on Sandstorm.

    NPC_BLESSED_LOA_SPIRIT      = 69480, // Moves slowly to lowest councilor and heals him for 5%. If alive after 20 seconds jumps and heals him.
    NPC_SHADOWED_LOA_SPIRIT     = 69548, // Fixates to random player and moves to him. If within 6 yards jump and kills him.
    NPC_TWISTED_FATE_1          = 69740, // Twisted Fate npc 1.
    NPC_TWISTED_FATE_2          = 69746  // Twisted Fate npc 2.
};

/*** Spirit of Gara'jal ***/

// Gara'jal the Spiritbender 69135 - Acts as controller!
class boss_spirit_of_gara_jal_council : public CreatureScript
{
    public:
        boss_spirit_of_gara_jal_council() : CreatureScript("boss_spirit_of_gara_jal_council") { }

        struct boss_spirit_of_gara_jal_councilAI : public BossAI
        {
            boss_spirit_of_gara_jal_councilAI(Creature* creature) : BossAI(creature, DATA_COUNCIL_OF_ELDERS_EVENT)
            {
                instance  = creature->GetInstanceScript();
                creature->AddAura(SPELL_INTRO_VISUAL, creature);
            }

            InstanceScript* instance;
            EventMap events;
            bool kazPossessed, sulPossessed, malPossessed, marPossessed, eventEnded;
            uint8 possessedCount;
            uint8 bossesKilled;

            void Reset()
            {
                events.Reset();

                if (Creature* kaz = me->FindNearestCreature(BOSS_KAZRAJIN, 200.0f, true))
                    kaz->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                if (Creature* sul = me->FindNearestCreature(BOSS_SUL_THE_SANDCRAWLER, 200.0f, true))
                    sul->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                if (Creature* mal = me->FindNearestCreature(BOSS_FROST_KING_MALAKK, 200.0f, true))
                    mal->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                if (Creature* mar = me->FindNearestCreature(BOSS_HIGH_PRIESTESS_MARLI, 200.0f, true))
                    mar->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                kazPossessed = false;
                sulPossessed = false;
                malPossessed = false;
                marPossessed = false;

                possessedCount = 0;
                bossesKilled = 0;

                eventEnded = false;

                me->SetReactState(REACT_AGGRESSIVE);

                if (instance)
                    instance->SetData(DATA_COUNCIL_OF_ELDERS_EVENT, NOT_STARTED);

                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                Talk(SAY_INTRO);

                if (Creature* kaz = me->FindNearestCreature(BOSS_KAZRAJIN, 200.0f, true))
                {
                    kaz->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    kaz->AI()->DoZoneInCombat(kaz, 200.0f);
                }
                if (Creature* sul = me->FindNearestCreature(BOSS_SUL_THE_SANDCRAWLER, 200.0f, true))
                {
                    sul->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    sul->AI()->DoZoneInCombat(sul, 200.0f);
                }
                if (Creature* mal = me->FindNearestCreature(BOSS_FROST_KING_MALAKK, 200.0f, true))
                {
                    mal->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    mal->AI()->DoZoneInCombat(mal, 200.0f);
                }
                if (Creature* mar = me->FindNearestCreature(BOSS_HIGH_PRIESTESS_MARLI, 200.0f, true))
                {
                    mar->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    mar->AI()->DoZoneInCombat(mar, 200.0f);
                }

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                DoCast(me, SPELL_WISP_FORM);

				events.ScheduleEvent(EVENT_KAZRAJIN_SAY_AGRRO, TIMER_KAZRAJIN_SAY_AGRRO);
				events.ScheduleEvent(EVENT_SUL_SAY_AGRRO, TIMER_SUL_SAY_AGRRO);
				events.ScheduleEvent(EVENT_MALAKK_SAY_AGRRO, TIMER_MALAKK_SAY_AGRRO);
				events.ScheduleEvent(EVENT_MARLI_SAY_AGRRO, TIMER_MARLI_SAY_AGRRO);

				events.ScheduleEvent(EVENT_POSSESS_NEXT, TIMER_POSSESS_NEXT);

                if (instance)
                    instance->SetData(DATA_COUNCIL_OF_ELDERS_EVENT, IN_PROGRESS);

                _EnterCombat();
            }

			void EnterEvadeMode()
            {
                if (Creature* kaz = me->FindNearestCreature(BOSS_KAZRAJIN, 200.0f, false))
                {
                    kaz->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    if (kaz->isDead())
                        kaz->Respawn();
                }
                if (Creature* sul = me->FindNearestCreature(BOSS_SUL_THE_SANDCRAWLER, 200.0f, false))
                {
                    sul->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    if (sul->isDead())
                        sul->Respawn();
                }
                if (Creature* mal = me->FindNearestCreature(BOSS_FROST_KING_MALAKK, 200.0f, false))
                {
                    mal->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    if (mal->isDead())
                        mal->Respawn();
                }
                if (Creature* mar = me->FindNearestCreature(BOSS_HIGH_PRIESTESS_MARLI, 200.0f, false))
                {
                    mar->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    if (mar->isDead())
                        mar->Respawn();
                }

                DespawnSummon(NPC_LIVING_SAND);

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                    instance->SetData(DATA_COUNCIL_OF_ELDERS_EVENT, FAIL);

                _EnterEvadeMode();
            }

            void JustReachedHome()
            {
                if (!me->HasAura(SPELL_INTRO_VISUAL))
                    me->AddAura(SPELL_INTRO_VISUAL, me);

                _JustReachedHome();
            }

            void JustDied(Unit* killer)
            {
                DespawnSummon(NPC_LIVING_SAND);

                if (instance)
                    instance->SetData(DATA_COUNCIL_OF_ELDERS_EVENT, DONE);

                _JustDied();
            }

            void DespawnSummon(uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        (*summs)->DespawnOrUnsummon();
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_POSSESS_NEXT:
                        events.ScheduleEvent(EVENT_POSSESS_NEXT, TIMER_POSSESS_NEXT);
                        break;

                    default: break;
                }
            };

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || !CheckInRoom() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    EnterEvadeMode();
                    return;
                }

                if (bossesKilled >= 4)
                {
                    // Act as loot controller.
                    if (Player* player = me->FindNearestPlayer(100.0f, true))
                    {
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->LowerPlayerDamageReq(me->GetMaxHealth());
						player->DealDamage(me, 1000);
                        player->ToUnit()->Kill(me);
                    }
                    eventEnded = true;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_KAZRAJIN_SAY_AGRRO:
                            if (Creature* kaz = me->FindNearestCreature(BOSS_KAZRAJIN, 200.0f, true))
                                kaz->AI()->Talk(SAY_KAZ_AGGRO);
                            break;

                        case EVENT_SUL_SAY_AGRRO:
                            if (Creature* sul = me->FindNearestCreature(BOSS_SUL_THE_SANDCRAWLER, 200.0f, true))
                                sul->AI()->Talk(SAY_SUL_AGGRO);
                            break;

                        case EVENT_MALAKK_SAY_AGRRO:
                            if (Creature* mal = me->FindNearestCreature(BOSS_FROST_KING_MALAKK, 200.0f, true))
                                mal->AI()->Talk(SAY_MAL_AGGRO);
                            break;

                        case EVENT_MARLI_SAY_AGRRO:
                            if (Creature* mar = me->FindNearestCreature(BOSS_HIGH_PRIESTESS_MARLI, 200.0f, true))
                                mar->AI()->Talk(SAY_MAR_AGGRO);
                            break;

                        case EVENT_POSSESS_NEXT:
                        {
                            uint32 councilorToPossess = 0;

                            if (kazPossessed && sulPossessed && malPossessed && marPossessed)
                            {
                                // First round done. Select highest health of them.
                                uint32 kazHealth = 0;
                                uint32 sulHealth = 0;
                                uint32 malHealth = 0;
                                uint32 marHealth = 0;

                                uint32 healthToCompare1 = 0;
                                uint32 healthToCompare2 = 0;

                                if (Creature* kaz = me->FindNearestCreature(BOSS_KAZRAJIN, 200.0f, true))
                                    kazHealth = kaz->GetHealth();
                                if (Creature* sul = me->FindNearestCreature(BOSS_SUL_THE_SANDCRAWLER, 200.0f, true))
                                    sulHealth = sul->GetHealth();
                                if (Creature* mal = me->FindNearestCreature(BOSS_FROST_KING_MALAKK, 200.0f, true))
                                    malHealth = mal->GetHealth();
                                if (Creature* mar = me->FindNearestCreature(BOSS_HIGH_PRIESTESS_MARLI, 200.0f, true))
                                    marHealth = mar->GetHealth();

                                if (kazHealth && sulHealth || kazHealth && !sulHealth || !kazHealth && sulHealth)
                                {
                                    if (kazHealth >= sulHealth || kazHealth && !sulHealth)
                                        healthToCompare1 = kazHealth;
                                    else
                                        healthToCompare1 = sulHealth;
                                }

                                if (malHealth && marHealth || malHealth && !marHealth || !malHealth && marHealth)
                                {
                                    if (malHealth >= marHealth || malHealth && !marHealth)
                                        healthToCompare2 = malHealth;
                                    else
                                        healthToCompare2 = marHealth;
                                }

                                if (healthToCompare1 && healthToCompare2)
                                {
                                    if (healthToCompare1 >= healthToCompare2)
                                    {
                                        if (healthToCompare1 == kazHealth)
                                            councilorToPossess = BOSS_KAZRAJIN;
                                        else
                                            councilorToPossess = BOSS_SUL_THE_SANDCRAWLER;
                                    }
                                    else
                                    {
                                        if (healthToCompare2 == malHealth)
                                            councilorToPossess = BOSS_FROST_KING_MALAKK;
                                        else
                                            councilorToPossess = BOSS_HIGH_PRIESTESS_MARLI;
                                    }
                                }
                            }
                            else
                            {
                                // First round not done. Frost King Malakk is first, goes to all 4, then to highest health of them.
                                if (!malPossessed)
                                {
                                    councilorToPossess = BOSS_FROST_KING_MALAKK;
                                    malPossessed = true;
                                }
								else if (malPossessed && !marPossessed)
                                {
                                    councilorToPossess = BOSS_HIGH_PRIESTESS_MARLI;
                                    marPossessed = true;
                                }
								else if (malPossessed && marPossessed && !kazPossessed)
                                {
                                    councilorToPossess = BOSS_KAZRAJIN;
                                    kazPossessed = true;
                                }
								else if (malPossessed && marPossessed && kazPossessed && !sulPossessed)
                                {
                                    councilorToPossess = BOSS_SUL_THE_SANDCRAWLER;
                                    sulPossessed = true;
                                }
                            }

                            // Failsafe. If selection failed, simply select a random one.
                            if (!councilorToPossess)
                            {
                                bool kazE = false;
                                bool sulE = false;
                                bool malE = false;
                                bool marE = false;

                                if (Creature* kaz = me->FindNearestCreature(BOSS_KAZRAJIN, 200.0f, true))
                                    kazE = true;
                                if (Creature* sul = me->FindNearestCreature(BOSS_SUL_THE_SANDCRAWLER, 200.0f, true))
                                    sulE = true;
                                if (Creature* mal = me->FindNearestCreature(BOSS_FROST_KING_MALAKK, 200.0f, true))
                                    malE = true;
                                if (Creature* mar = me->FindNearestCreature(BOSS_HIGH_PRIESTESS_MARLI, 200.0f, true))
                                    marE = true;

                                if (kazE && sulE && malE && marE)
                                    councilorToPossess = RAND(BOSS_KAZRAJIN, BOSS_SUL_THE_SANDCRAWLER, BOSS_FROST_KING_MALAKK, BOSS_HIGH_PRIESTESS_MARLI);
                                else if (!kazE)
                                {
                                    if (sulE && malE && marE)
                                        councilorToPossess = RAND(BOSS_SUL_THE_SANDCRAWLER, BOSS_FROST_KING_MALAKK, BOSS_HIGH_PRIESTESS_MARLI);
                                    else if (!sulE)
                                    {
                                        if (malE && marE)
                                            councilorToPossess = RAND(BOSS_FROST_KING_MALAKK, BOSS_HIGH_PRIESTESS_MARLI);
                                        else if (!malE && marE)
                                            councilorToPossess = BOSS_HIGH_PRIESTESS_MARLI;
                                        else if (malE && !marE)
                                            councilorToPossess = BOSS_FROST_KING_MALAKK;
                                    }
                                }
                            }

                            // Impossible to fail now, check encounter status.
                            if (councilorToPossess)
                            {
                                // Handle Soul Fragment.
                                possessedCount++;
                                if (me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC || me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC && possessedCount % 2 == 0)
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                        me->AddAura(SPELL_SOUL_FRAGMENT, target);

                                if (Creature* councilor = me->FindNearestCreature(councilorToPossess, 200.0f, true))
                                {
                                    me->GetMotionMaster()->MovementExpired();
                                    me->GetMotionMaster()->MoveFollow(councilor, 2.0f, 0.0f);
                                    DoCast(councilor, SPELL_POSSESS);
                                }
                            }
                            break;
                        }

                        default: break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_spirit_of_gara_jal_councilAI(creature);
        }
};

/*** Elders ***/

// Kazra'jin 69134
class boss_kazra_jin : public CreatureScript
{
    public:
        boss_kazra_jin() : CreatureScript("boss_kazra_jin") { }

        struct boss_kazra_jinAI : public ScriptedAI
        {
            boss_kazra_jinAI(Creature* creature) : ScriptedAI(creature), summons(me), vehicle(creature->GetVehicleKit())
            {
                instance = creature->GetInstanceScript();
                ASSERT(vehicle);
            }

            InstanceScript* instance;
            Vehicle* vehicle;
            SummonList summons;
            EventMap events;
            uint32 damageTakenPossessed;
            bool darkPowerScheduled;
            Unit* recklessTarget;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);

                damageTakenPossessed = 0;
                darkPowerScheduled = false;

                recklessTarget = NULL;
            }

            void EnterCombat(Unit* who)
            {
				events.ScheduleEvent(EVENT_RECKLESS_CHARGE, TIMER_RECKLESS_CHARGE_F);

				events.ScheduleEvent(EVENT_BERSERK, TIMER_BERSERK);

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KAZ_KILL);
            }

			void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void JustDied(Unit* killer)
            {
                Talk(SAY_KAZ_DEATH);
                summons.DespawnAll();

                // Check event state.
                if (Creature* Garajal = me->FindNearestCreature(NPC_SPIRIT_OF_GARAJAL, 200.0f, true))
                {
                    CAST_AI(boss_spirit_of_gara_jal_council::boss_spirit_of_gara_jal_councilAI, Garajal->ToCreature()->AI())->bossesKilled++;

                    if (me->HasAura(SPELL_POSSESS))
                        Garajal->AI()->DoAction(ACTION_POSSESS_NEXT);
                }

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            // Reset damage taken while Possessed.
            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_POSSESS)
                {
                    Talk(SAY_KAZ_POSSESSED);
                    damageTakenPossessed = 0;
                }
            }

            // Check damage taken and remove Possessed aura.
            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (me->HasAura(SPELL_POSSESS))
                {
                    // Handle Overload / Discharge.
                    if (me->HasAura(SPELL_OVERLOAD))
                    {
                        const SpellInfo* overloadSpell = sSpellMgr->GetSpellInfo(SPELL_OVERLOAD_REFLECT_DMG, me->GetMap()->GetDifficulty());
                        me->DealDamage(doneBy, (uiDamage * 4) / 10, NULL, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NATURE, overloadSpell);
                    }
                    else if (me->HasAura(SPELL_DISCHARGE))
                    {
                        const SpellInfo* dischargeSpell = sSpellMgr->GetSpellInfo(SPELL_DISCHARGE_REFLECT_DMG, me->GetMap()->GetDifficulty());
                        Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                        if (!PlayerList.isEmpty())
                            for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                if (Player* player = i->getSource())
                                    me->DealDamage(player, uiDamage / 20, NULL, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NATURE, dischargeSpell);
                    }

                    // Increase Possessed damage taken.
                    damageTakenPossessed += uiDamage;

                    if (damageTakenPossessed >= me->GetMaxHealth() / 4)
                    {
                        me->RemoveAurasDueToSpell(SPELL_POSSESS);
                        me->SetPower(POWER_ENERGY, 0);
                        me->AddAura(SPELL_LINGERING_PRESENCE, me);

                        if (Creature* Garajal = me->FindNearestCreature(NPC_SPIRIT_OF_GARAJAL, 200.0f, true))
                            Garajal->AI()->DoAction(ACTION_POSSESS_NEXT);
                    }
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    EnterEvadeMode();
                    return;
                }

                if (me->GetPower(POWER_ENERGY) == 100 && !darkPowerScheduled && !me->HasAura(SPELL_RECKLESS_CHARGE))
                {
                    events.ScheduleEvent(EVENT_DARK_POWER, TIMER_DARK_POWER);
                    darkPowerScheduled = true;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_RECKLESS_CHARGE:
                            Talk(SAY_RECKLESS_CHARGE);
                            if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 40.0f, true))
                                if (Creature* trigger = me->SummonCreature(NPC_RECKLESS_CHARGE, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN))
                                    recklessTarget = trigger;
                            DoCast(me, SPELL_RECKLESS_CHARGE);
				            events.ScheduleEvent(EVENT_RECKLESS_CHARGE_MOVE, TIMER_RECKLESS_CHARGE_MOVE);
                            break;

                        case EVENT_RECKLESS_CHARGE_MOVE:
                            if (recklessTarget)
                                me->GetMotionMaster()->MoveCharge(recklessTarget->GetPositionX(), recklessTarget->GetPositionY(), recklessTarget->GetPositionZ(), 12.0f);
                            me->CastSpell(me, SPELL_RECKLESS_CHARGE_VIS, true);
				            events.ScheduleEvent(EVENT_SUMMON_RECKLESS_CHARGE_VIS, TIMER_SUMM_R_CHARGE_VIS);
                            break;

                        case EVENT_SUMMON_RECKLESS_CHARGE_VIS:
                            if (recklessTarget && me->HasAura(SPELL_RECKLESS_CHARGE))
                            {
                                if (!me->IsWithinDistInMap(recklessTarget, 1.5f, true)) // Not reached yet.
                                {
                                    me->CastSpell(me, SPELL_RECKLESS_CHARGE_VIS, true);

                                    if (Player* player = me->FindNearestPlayer(2.0f, true))
                                        if (player->IsInBetween(me, recklessTarget, 1.0f))
                                            me->CastSpell(player, SPELL_RECKLESS_CHARGE_DMG, true);

				                    events.ScheduleEvent(EVENT_SUMMON_RECKLESS_CHARGE_VIS, TIMER_SUMM_R_CHARGE_VIS);
                                }
                                else // target reached, remove aura, cast damage and schedule Overload / Discharge.
                                {
                                    me->RemoveAurasDueToSpell(SPELL_RECKLESS_CHARGE);
                                    summons.DespawnAll();
                                    recklessTarget = NULL;
                                    DoCast(me, SPELL_RECKLESS_CHARGE_T_DMG);

				                    events.ScheduleEvent(EVENT_OVERLOAD_DISCHARGE, TIMER_OVERLOAD_DISCHARGE);
                                }
                            }
                            else // Failsafe for target reached but not in range etc.
                            {
                                if (recklessTarget) // Mob is summoned, but aura expired because left removal range.
                                {
                                    summons.DespawnAll();
                                    recklessTarget = NULL;
                                }
                                DoCast(me, SPELL_RECKLESS_CHARGE_T_DMG);
				                events.ScheduleEvent(EVENT_OVERLOAD_DISCHARGE, TIMER_OVERLOAD_DISCHARGE);
                            }
                            break;

                        case EVENT_OVERLOAD_DISCHARGE:
                            if (me->HasAura(SPELL_POSSESS))
                            {
                                Talk(SAY_DISCHARGE);
                                me->SetReactState(REACT_PASSIVE);
                                if (me->GetMap()->IsHeroic())
                                    DoCast(me, SPELL_DISCHARGE);
                                else
                                    DoCast(me, SPELL_OVERLOAD);

				                events.ScheduleEvent(EVENT_R_OVERLOAD_DISCHARGE, TIMER_R_OVERLOAD_DISCHARGE);
                            }
                            else
				                events.ScheduleEvent(EVENT_RECKLESS_CHARGE, TIMER_RECKLESS_CHARGE_S);
                            break;

                        case EVENT_R_OVERLOAD_DISCHARGE:
                            me->RemoveAurasDueToSpell(me->GetMap()->IsHeroic() ? SPELL_DISCHARGE : SPELL_OVERLOAD);
                            me->SetReactState(REACT_AGGRESSIVE);
				            events.ScheduleEvent(EVENT_RECKLESS_CHARGE, TIMER_RECKLESS_CHARGE_S);
                            break;

                        case EVENT_DARK_POWER:
                            DoCast(me, SPELL_DARK_POWER);
                            if (me->GetPower(POWER_ENERGY) == 100)
                                events.ScheduleEvent(EVENT_DARK_POWER, TIMER_DARK_POWER);
                            else
                                darkPowerScheduled = false;
                            break;

                        case EVENT_BERSERK:
				            DoCast(me, SPELL_BERSERK);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_kazra_jinAI(creature);
        }
};

// Sul the Sandcrawler 69078
class boss_sul_the_sandcrawler : public CreatureScript
{
    public:
        boss_sul_the_sandcrawler() : CreatureScript("boss_sul_the_sandcrawler") { }

        struct boss_sul_the_sandcrawlerAI : public ScriptedAI
        {
            boss_sul_the_sandcrawlerAI(Creature* creature) : ScriptedAI(creature), summons(me), vehicle(creature->GetVehicleKit())
            {
                instance = creature->GetInstanceScript();
                ASSERT(vehicle);
            }

            InstanceScript* instance;
            Vehicle* vehicle;
            SummonList summons;
            EventMap events;
            uint32 damageTakenPossessed;
            bool darkPowerScheduled;
            uint8 quicksandCastCount;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);

                damageTakenPossessed = 0;
                quicksandCastCount = 0;
                darkPowerScheduled = false;
            }

            void EnterCombat(Unit* who)
            {
				events.ScheduleEvent(EVENT_SAND_BOLT, TIMER_SAND_BOLT);
				events.ScheduleEvent(EVENT_QUICKSAND, TIMER_QUICKSAND_F);

				events.ScheduleEvent(EVENT_BERSERK, TIMER_BERSERK);

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SUL_KILL);
            }

			void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void JustDied(Unit* killer)
            {
                Talk(SAY_SUL_DEATH);
                summons.DespawnAll();

                // Check event state.
                if (Creature* Garajal = me->FindNearestCreature(NPC_SPIRIT_OF_GARAJAL, 200.0f, true))
                {
                    CAST_AI(boss_spirit_of_gara_jal_council::boss_spirit_of_gara_jal_councilAI, Garajal->ToCreature()->AI())->bossesKilled++;

                    if (me->HasAura(SPELL_POSSESS))
                        Garajal->AI()->DoAction(ACTION_POSSESS_NEXT);
                }

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            // Reset damage taken while Possessed.
            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_POSSESS)
                {
                    Talk(SAY_SUL_POSSESSED);
                    damageTakenPossessed = 0;
                }
            }

            // Check damage taken and remove Possessed aura.
            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (me->HasAura(SPELL_POSSESS))
                {
                    damageTakenPossessed += uiDamage;

                    if (damageTakenPossessed >= me->GetMaxHealth() / 4)
                    {
                        me->RemoveAurasDueToSpell(SPELL_POSSESS);
                        me->SetPower(POWER_ENERGY, 0);
                        me->AddAura(SPELL_LINGERING_PRESENCE, me);

                        if (Creature* Garajal = me->FindNearestCreature(NPC_SPIRIT_OF_GARAJAL, 200.0f, true))
                            Garajal->AI()->DoAction(ACTION_POSSESS_NEXT);
                    }
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    EnterEvadeMode();
                    return;
                }

                if (me->GetPower(POWER_ENERGY) == 100 && !darkPowerScheduled)
                {
                    events.ScheduleEvent(EVENT_DARK_POWER, TIMER_DARK_POWER);
                    darkPowerScheduled = true;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SAND_BOLT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_SAND_BOLT);
				            events.ScheduleEvent(EVENT_SAND_BOLT, TIMER_SAND_BOLT);
                            break;

                        case EVENT_QUICKSAND:
                            if (me->HasAura(SPELL_POSSESS))
                            {
                                Talk(SAY_SANDSTORM);

                                std::list<Creature*> sandsList;
                                GetCreatureListWithEntryInGrid(sandsList, me, NPC_QUICKSAND, 200.0f);
                                if (!sandsList.empty())
                                    for (std::list<Creature*>::iterator sand = sandsList.begin(); sand != sandsList.end(); sand++)
                                        (*sand)->AI()->DoAction(ACTION_SUMMON_LIVING_SAND);

                                std::list<Creature*> livingList;
                                GetCreatureListWithEntryInGrid(livingList, me, NPC_LIVING_SAND, 200.0f);
                                if (!livingList.empty())
                                    for (std::list<Creature*>::iterator living = livingList.begin(); living != livingList.end(); living++)
                                        (*living)->AddAura(SPELL_FORTIFIED, *living);

                                DoCast(me, SPELL_SANDSTORM);
                                me->AddAura(SPELL_SANDSTORM_VISUAL, me);
                            }
                            else
                            {
                                Talk(SAY_QUICKSAND);
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
								{
                                    DoCast(target, SPELL_QUICKSAND);
                                    me->SummonCreature(NPC_QUICKSAND, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN);
                                    quicksandCastCount++;
								}
                            }
				            events.ScheduleEvent(EVENT_QUICKSAND, TIMER_QUICKSAND_F);
                            break;

                        case EVENT_DARK_POWER:
                            DoCast(me, SPELL_DARK_POWER);
                            if (me->GetPower(POWER_ENERGY) == 100)
                                events.ScheduleEvent(EVENT_DARK_POWER, TIMER_DARK_POWER);
                            else
                                darkPowerScheduled = false;
                            break;

                        case EVENT_BERSERK:
				            DoCast(me, SPELL_BERSERK);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_sul_the_sandcrawlerAI(creature);
        }
};

// Frost King Malakk 69131
class boss_frost_king_malakk : public CreatureScript
{
    public:
        boss_frost_king_malakk() : CreatureScript("boss_frost_king_malakk") { }

        struct boss_frost_king_malakkAI : public ScriptedAI
        {
            boss_frost_king_malakkAI(Creature* creature) : ScriptedAI(creature), summons(me), vehicle(creature->GetVehicleKit())
            {
                instance  = creature->GetInstanceScript();
                ASSERT(vehicle);
            }

            InstanceScript* instance;
            Vehicle* vehicle;
            SummonList summons;
            EventMap events;
            uint32 damageTakenPossessed;
            bool darkPowerScheduled;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);

                damageTakenPossessed = 0;
                darkPowerScheduled = false;
            }

            void EnterCombat(Unit* who)
            {
				events.ScheduleEvent(EVENT_FRIGID_ASSAULT, TIMER_FRIGID_ASSAULT);
				events.ScheduleEvent(EVENT_BITING_COLD_FROSTBITE, TIMER_BITING_COLD_F);

				events.ScheduleEvent(EVENT_BERSERK, TIMER_BERSERK);

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_MAL_KILL);
            }

			void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void JustDied(Unit* killer)
            {
                Talk(SAY_MAL_DEATH);
                summons.DespawnAll();

                // Check event state.
                if (Creature* Garajal = me->FindNearestCreature(NPC_SPIRIT_OF_GARAJAL, 200.0f, true))
                {
                    CAST_AI(boss_spirit_of_gara_jal_council::boss_spirit_of_gara_jal_councilAI, Garajal->ToCreature()->AI())->bossesKilled++;

                    if (me->HasAura(SPELL_POSSESS))
                        Garajal->AI()->DoAction(ACTION_POSSESS_NEXT);
                }

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            // Reset damage taken while Possessed.
            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_POSSESS)
                {
                    Talk(SAY_MAL_POSSESSED);
                    damageTakenPossessed = 0;
                }
            }

            // Check damage taken and remove Possessed aura.
            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (me->HasAura(SPELL_POSSESS))
                {
                    damageTakenPossessed += uiDamage;

                    if (damageTakenPossessed >= me->GetMaxHealth() / 4)
                    {
                        me->RemoveAurasDueToSpell(SPELL_POSSESS);
                        me->SetPower(POWER_ENERGY, 0);
                        me->AddAura(SPELL_LINGERING_PRESENCE, me);

                        if (Creature* Garajal = me->FindNearestCreature(NPC_SPIRIT_OF_GARAJAL, 200.0f, true))
                            Garajal->AI()->DoAction(ACTION_POSSESS_NEXT);
                    }
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    EnterEvadeMode();
                    return;
                }

                if (me->GetPower(POWER_ENERGY) == 100 && !darkPowerScheduled)
                {
                    events.ScheduleEvent(EVENT_DARK_POWER, TIMER_DARK_POWER);
                    darkPowerScheduled = true;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FRIGID_ASSAULT:
                            DoCast(me, SPELL_FRIGID_ASSAULT);
				            events.ScheduleEvent(EVENT_FRIGID_ASSAULT, TIMER_FRIGID_ASSAULT);
                            break;

                        case EVENT_BITING_COLD_FROSTBITE:
                            Talk(SAY_FROSTBITE);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            {
                                me->AddAura(SPELL_BITING_COLD_VISUAL, target);
                                if (me->HasAura(SPELL_POSSESS))
                                    me->CastCustomSpell(SPELL_FROSTBITE, SPELLVALUE_AURA_STACK, 5, target);
                                else DoCast(target, SPELL_BITING_COLD);
                            }
				            events.ScheduleEvent(EVENT_BITING_COLD_FROSTBITE, TIMER_BITING_COLD_F);
                            break;

                        case EVENT_DARK_POWER:
                            DoCast(me, SPELL_DARK_POWER);
                            if (me->GetPower(POWER_ENERGY) == 100)
                                events.ScheduleEvent(EVENT_DARK_POWER, TIMER_DARK_POWER);
                            else
                                darkPowerScheduled = false;
                            break;

                        case EVENT_BERSERK:
				            DoCast(me, SPELL_BERSERK);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_frost_king_malakkAI(creature);
        }
};

// High Priestess Mar'li 69132
class boss_high_priestess_mar_li : public CreatureScript
{
    public:
        boss_high_priestess_mar_li() : CreatureScript("boss_high_priestess_mar_li") { }

        struct boss_high_priestess_mar_liAI : public ScriptedAI
        {
            boss_high_priestess_mar_liAI(Creature* creature) : ScriptedAI(creature), summons(me), vehicle(creature->GetVehicleKit())
            {
                instance  = creature->GetInstanceScript();
                ASSERT(vehicle);
            }

            InstanceScript* instance;
            Vehicle* vehicle;
            SummonList summons;
            EventMap events;
            uint32 damageTakenPossessed;
            bool darkPowerScheduled;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);

                damageTakenPossessed = 0;
                darkPowerScheduled = false;
            }

            void EnterCombat(Unit* who)
            {
				events.ScheduleEvent(EVENT_WRATH_OF_THE_LOA, TIMER_WRATH_OF_THE_LOA);
				events.ScheduleEvent(EVENT_BS_LOA_SPIRIT_FATE, TIMER_LOA_SPIRIT_F);

				events.ScheduleEvent(EVENT_BERSERK, TIMER_BERSERK);

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_MAR_KILL);
            }

			void EnterEvadeMode()
            {
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);
                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 0);
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void JustDied(Unit* killer)
            {
                Talk(SAY_MAR_DEATH);
                summons.DespawnAll();

                // Check event state.
                if (Creature* Garajal = me->FindNearestCreature(NPC_SPIRIT_OF_GARAJAL, 200.0f, true))
                {
                    CAST_AI(boss_spirit_of_gara_jal_council::boss_spirit_of_gara_jal_councilAI, Garajal->ToCreature()->AI())->bossesKilled++;

                    if (me->HasAura(SPELL_POSSESS))
                        Garajal->AI()->DoAction(ACTION_POSSESS_NEXT);
                }

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            // Reset damage taken while Possessed.
            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_POSSESS)
                {
                    Talk(SAY_MAR_POSSESSED);
                    damageTakenPossessed = 0;
                }
            }

            // Check damage taken and remove Possessed aura.
            void DamageTaken(Unit* doneBy, uint32 &uiDamage)
            {
                if (me->HasAura(SPELL_POSSESS))
                {
                    damageTakenPossessed += uiDamage;

                    if (damageTakenPossessed >= me->GetMaxHealth() / 4)
                    {
                        me->RemoveAurasDueToSpell(SPELL_POSSESS);
                        me->SetPower(POWER_ENERGY, 0);
                        me->AddAura(SPELL_LINGERING_PRESENCE, me);

                        if (Creature* Garajal = me->FindNearestCreature(NPC_SPIRIT_OF_GARAJAL, 200.0f, true))
                            Garajal->AI()->DoAction(ACTION_POSSESS_NEXT);
                    }
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    EnterEvadeMode();
                    return;
                }

                if (me->GetPower(POWER_ENERGY) == 100 && !darkPowerScheduled)
                {
                    events.ScheduleEvent(EVENT_DARK_POWER, TIMER_DARK_POWER);
                    darkPowerScheduled = true;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_WRATH_OF_THE_LOA:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, me->HasAura(SPELL_POSSESS) ? SPELL_WRATH_OF_THE_LOA_POSS : SPELL_WRATH_OF_THE_LOA);
				            events.ScheduleEvent(EVENT_WRATH_OF_THE_LOA, TIMER_WRATH_OF_THE_LOA);
                            break;

                        case EVENT_BS_LOA_SPIRIT_FATE:
                            if (me->HasAura(SPELL_POSSESS))
                            {
                                Talk(SAY_LOA_SPIRIT);
                                if (me->GetMap()->IsHeroic())
                                    DoCast(me, SPELL_TWISTED_FATE);
                                else
                                    DoCast(me, SPELL_SHADOWED_LOA_SPIRIT);
                            }
                            else DoCast(me, SPELL_BLESSED_LOA_SPIRIT);
				            events.ScheduleEvent(EVENT_BS_LOA_SPIRIT_FATE, TIMER_LOA_SPIRIT_F);
                            break;

                        case EVENT_DARK_POWER:
                            DoCast(me, SPELL_DARK_POWER);
                            if (me->GetPower(POWER_ENERGY) == 100)
                                events.ScheduleEvent(EVENT_DARK_POWER, TIMER_DARK_POWER);
                            else
                                darkPowerScheduled = false;
                            break;

                        case EVENT_BERSERK:
				            DoCast(me, SPELL_BERSERK);
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_high_priestess_mar_liAI(creature);
        }
};

// Reckless Charge 69453
class npc_zabrajin_reckless_charge : public CreatureScript
{
    public:
        npc_zabrajin_reckless_charge() : CreatureScript("npc_zabrajin_reckless_charge") { }

        struct npc_zabrajin_reckless_charge_AI : public ScriptedAI
        {
            npc_zabrajin_reckless_charge_AI(Creature* creature) : ScriptedAI(creature) { }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
            }

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_PASSIVE);
            }

            void UpdateAI(uint32 const diff) { }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_zabrajin_reckless_charge_AI(creature);
        }
};

// Quicksand 69150
class npc_sul_quicksand : public CreatureScript
{
    public:
        npc_sul_quicksand() : CreatureScript("npc_sul_quicksand") { }

        struct npc_sul_quicksandAI : public ScriptedAI
        {
            npc_sul_quicksandAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;
            bool despawnable;

            void IsSummonedBy(Unit* summoner)
            {
                events.Reset();
                me->AddAura(SPELL_QUICKSAND_VISUAL, me);
                DoCast(me, SPELL_QUICKSAND_SPAWN_EFF);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_PASSIVE);

                if (summoner->ToCreature())
                {
                    if (CAST_AI(boss_sul_the_sandcrawler::boss_sul_the_sandcrawlerAI, summoner->ToCreature()->AI())->quicksandCastCount % 2 == 0) // first and each 2 casts.
                        despawnable = false;
                    else
                        despawnable = true;
                }
                else despawnable = false;

                events.ScheduleEvent(EVENT_ENSNARE_ENTRAPPED, TIMER_ENSNARE_ENTRAPPED);
                events.ScheduleEvent(EVENT_CHECK_TREACH_GROUND, TIMER_CHECK_TREACH_GROUND);
            }

            void Reset() { }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_SUMMON_LIVING_SAND:
                        if (Creature* living = me->SummonCreature(NPC_LIVING_SAND, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            if (me->HasAura(SPELL_TREACHEROUS_GROUND_QS))
                                living->AddAura(SPELL_TREACHEROUS_GROUND, living);

                            events.CancelEvent(EVENT_ENSNARE_ENTRAPPED);
                            events.CancelEvent(EVENT_CHECK_TREACH_GROUND);
                            me->RemoveAllAuras();
                            me->DespawnOrUnsummon(5000);
                        }
                        break;

                    default: break;
                }
            };

            void JustSummoned(Creature* summon)
            {
				summon->setActive(true);
				summon->AI()->DoZoneInCombat(summon, 100.0f);
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ENSNARE_ENTRAPPED:
                        {
                            Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                            if (!PlayerList.isEmpty())
                            {
                                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                {
                                    if (Player* player = i->getSource())
                                    {
                                        if (me->IsWithinDistInMap(player, 4.0f, true))
                                        {
                                            if (!player->HasAura(SPELL_QUICKSAND_DMG))
                                                me->AddAura(SPELL_QUICKSAND_DMG, player);

                                            if (!player->HasAura(SPELL_ENSNARED))
                                            {
                                                if (!player->HasAura(SPELL_ENTRAPPED))
                                                    me->AddAura(SPELL_ENSNARED, player);
                                            }
                                            else
                                            {
                                                if (AuraPtr ensnared = player->GetAura(SPELL_ENSNARED))
                                                {
                                                    if (int32 stacks = ensnared->GetStackAmount())
                                                    {
                                                        if (stacks >= 5 && !player->HasAura(SPELL_ENTRAPPED))
                                                        {
                                                            me->AddAura(SPELL_ENTRAPPED, player);
                                                            player->RemoveAurasDueToSpell(SPELL_ENSNARED);
                                                        }
                                                        else me->AddAura(SPELL_ENSNARED, player);
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (!player->FindNearestCreature(NPC_QUICKSAND, 4.0f, true) && player->HasAura(SPELL_QUICKSAND_DMG))
                                                player->RemoveAurasDueToSpell(SPELL_QUICKSAND_DMG);
                                        }
                                    }
                                }
                            }

                            events.ScheduleEvent(EVENT_ENSNARE_ENTRAPPED, TIMER_ENSNARE_ENTRAPPED);
                            break;
						}

                        case EVENT_CHECK_TREACH_GROUND:
			            {
                            if (!despawnable)
                            {
                                if (Creature* Quicksand = me->FindNearestCreature(NPC_QUICKSAND, 4.0f * me->GetObjectScale(), true))
                                {
                                    if (CAST_AI(npc_sul_quicksand::npc_sul_quicksandAI, Quicksand->ToCreature()->AI())->despawnable)
                                    {
                                        if (Creature* boss = me->FindNearestCreature(BOSS_SUL_THE_SANDCRAWLER, 200.0f, true))
                                            boss->AI()->Talk(SAY_TREACHEROUS_GROUND);
                                        Quicksand->DespawnOrUnsummon();
                                        me->AddAura(SPELL_TREACHEROUS_GROUND_QS, me);
                                    }
                                }

                                events.ScheduleEvent(EVENT_CHECK_TREACH_GROUND, TIMER_CHECK_TREACH_GROUND);
                            }
                            break;
						}

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sul_quicksandAI(creature);
        }
};

// Blessed Loa Spirit 69480
class npc_blessed_loa_spirit : public CreatureScript
{
    public:
        npc_blessed_loa_spirit() : CreatureScript("npc_blessed_loa_spirit") { }

        struct npc_blessed_loa_spiritAI : public ScriptedAI
        {
            npc_blessed_loa_spiritAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;
            Unit* councilorTarget;
            bool healed;

            void IsSummonedBy(Unit* summoner)
            {
                Reset();
                me->SetReactState(REACT_PASSIVE);

                if (!councilorTarget)
                {
                    Creature* councilor = me->FindNearestCreature(RAND(BOSS_KAZRAJIN, BOSS_SUL_THE_SANDCRAWLER, BOSS_FROST_KING_MALAKK), 200.0f, true);

                    if (!councilor)
                        councilor = me->FindNearestCreature(BOSS_KAZRAJIN, 200.0f, true);

                    if (!councilor)
                        councilor = me->FindNearestCreature(BOSS_SUL_THE_SANDCRAWLER, 200.0f, true);

                    if (!councilor)
                        councilor = me->FindNearestCreature(BOSS_FROST_KING_MALAKK, 200.0f, true);

                    if (!councilor)
                    {
                        me->DespawnOrUnsummon();
                        return;
                    }

                    me->GetMotionMaster()->MoveFollow(councilor, 0.0f, 0.0f);
                    councilorTarget = councilor;
                }

                events.ScheduleEvent(EVENT_LOA_SPIRIT_JUMP_KILL, TIMER_LOA_SPIRIT_JUMP_KILL);
            }

            void Reset()
            {
                councilorTarget = NULL;
                healed = false;

                events.Reset();
            }

            void JustSummoned(Creature* summon)
            {
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void UpdateAI(uint32 const diff)
            {
                // Check for player in range for normal Detonation.
                if (!healed && councilorTarget)
                {
                    if (me->IsWithinDistInMap(councilorTarget, 2.0f, true))
                    {
                        DoCast(councilorTarget, SPELL_BLESSED_GIFT);
                        me->DespawnOrUnsummon(500);
                        healed = true;
                    }
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_LOA_SPIRIT_JUMP_KILL:
                            if (!healed && councilorTarget)
                                me->GetMotionMaster()->MoveJump(councilorTarget->GetPositionX(), councilorTarget->GetPositionY(), councilorTarget->GetPositionZ(), 15.0f, 15.0f);
                            break;

                        default: break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_blessed_loa_spiritAI(creature);
        }
};

// Shadowed Loa Spirit 69548
class npc_shadowed_loa_spirit : public CreatureScript
{
    public:
        npc_shadowed_loa_spirit() : CreatureScript("npc_shadowed_loa_spirit") { }

        struct npc_shadowed_loa_spiritAI : public ScriptedAI
        {
            npc_shadowed_loa_spiritAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap events;
            Unit* playerTarget;
            bool exploded;

            void IsSummonedBy(Unit* summoner)
            {
                Reset();
                me->SetReactState(REACT_PASSIVE);

                if (!playerTarget)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 100.0f, true))
                    {
                        target->AddAura(SPELL_MARKED_SOUL, target);
                        me->Attack(target, false);
                        me->GetMotionMaster()->MoveChase(target);
                        playerTarget = target;
                    }
                }

                events.ScheduleEvent(EVENT_LOA_SPIRIT_JUMP_KILL, TIMER_LOA_SPIRIT_JUMP_KILL);
            }

            void Reset()
            {
                playerTarget = NULL;
                exploded = false;

                events.Reset();
            }

            void JustSummoned(Creature* summon)
            {
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void UpdateAI(uint32 const diff)
            {
                // Check for player in range for normal Detonation.
                if (!exploded && playerTarget)
                {
                    if (me->IsWithinDistInMap(playerTarget, 2.0f, true))
                    {
                        playerTarget->RemoveAurasDueToSpell(SPELL_MARKED_SOUL);
                        me->CastSpell(playerTarget, SPELL_SHADOWED_GIFT, true);
                        me->DespawnOrUnsummon(500);
                        exploded = true;
                    }
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_LOA_SPIRIT_JUMP_KILL:
                            if (!exploded && playerTarget)
                                me->GetMotionMaster()->MoveJump(playerTarget->GetPositionX(), playerTarget->GetPositionY(), playerTarget->GetPositionZ(), 15.0f, 15.0f);
                            break;

                        default: break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_shadowed_loa_spiritAI(creature);
        }
};

// Twisted Fate 1 69740 - acts as controller for Twisted Fate spell
class npc_marli_twisted_fate : public CreatureScript
{
    public:
        npc_marli_twisted_fate() : CreatureScript("npc_marli_twisted_fate") { }

        struct npc_marli_twisted_fateAI : public ScriptedAI
        {
            npc_marli_twisted_fateAI(Creature* creature) : ScriptedAI(creature) { }

            void IsSummonedBy(Unit* summoner)
            {
                Reset();
                me->SetReactState(REACT_PASSIVE);

                if (Unit* target = SelectTarget(SELECT_TARGET_NEAREST, 0, 100.0f, true))
                {
                    target->CastSpell(me, SPELL_TWISTED_FATE_CLONE_1, false);
                    me->AddAura(SPELL_TWISTED_FATE_DMG_A, me);

                    if (Unit* otherTarget = SelectTarget(SELECT_TARGET_FARTHEST, 0, 150.0f, true))
                    {
                        if (Creature* otherFate = summoner->SummonCreature(NPC_TWISTED_FATE_2, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN))
                        {
                            otherFate->SetReactState(REACT_PASSIVE);
                            otherTarget->CastSpell(otherFate, SPELL_TWISTED_FATE_CLONE_2, false);
                            otherFate->AddAura(SPELL_TWISTED_FATE_DMG_A, otherFate);
                            otherFate->GetMotionMaster()->MoveFollow(otherTarget, 2.0f, 0.0f);

                            me->CastSpell(otherFate, SPELL_TWISTED_FATE_VISUAL, true);
                            me->GetMotionMaster()->MoveFollow(otherFate, 0.0f, 0.0f);
                        }
                    }
                }

                events.ScheduleEvent(EVENT_LOA_SPIRIT_JUMP_KILL, TIMER_LOA_SPIRIT_JUMP_KILL);
            }

            void Reset() { }

            void UpdateAI(uint32 const diff) { }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_marli_twisted_fateAI(creature);
        }
};

// Possessed 136442
class spell_garajal_councilor_possessed : public SpellScriptLoader
{
    public:
        spell_garajal_councilor_possessed() : SpellScriptLoader("spell_garajal_councilor_possessed") { }

        class spell_garajal_councilor_possessed_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_garajal_councilor_possessed_AuraScript)

            void OnTick(constAuraEffectPtr aurEff)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                if (!target->ToCreature())
                    return;

                if (target->ToCreature()->GetEntry() != BOSS_KAZRAJIN && target->ToCreature()->GetEntry() != BOSS_SUL_THE_SANDCRAWLER &&
                    target->ToCreature()->GetEntry() != BOSS_FROST_KING_MALAKK && target->ToCreature()->GetEntry() != BOSS_HIGH_PRIESTESS_MARLI)
                    return;

                if (target->GetPower(POWER_ENERGY) < 100)
                    target->SetPower(POWER_ENERGY, 0 + (aurEff->GetTickNumber() * (aurEff->GetTickNumber() <= 32 ? 2 : 1))); // Ticks are 32 * 2 + 36 * 1 = 68 seconds to 100.
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_garajal_councilor_possessed_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_garajal_councilor_possessed_AuraScript();
        }
};

// Soul Fragment 137643
class spell_council_soul_fragment_transfer : public SpellScriptLoader
{
    public:
        spell_council_soul_fragment_transfer() : SpellScriptLoader("spell_council_soul_fragment_transfer") { }

        class spell_council_soul_fragment_transfer_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_council_soul_fragment_transfer_SpellScript);

            void HandleTrigger()
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                if (!caster->ToPlayer() || !target->ToPlayer())
                    return;

                caster->AddAura(SPELL_SOUL_FRAGMENT, target);
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_council_soul_fragment_transfer_SpellScript::HandleTrigger);
            }

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_council_soul_fragment_transfer_SpellScript();
        }
};

// Frigid Assault 136903
class spell_malakk_frigid_assault : public SpellScriptLoader
{
    public:
        spell_malakk_frigid_assault() : SpellScriptLoader("spell_malakk_frigid_assault") { }

        class spell_malakk_frigid_assault_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_malakk_frigid_assault_AuraScript)

            void OnPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                // Check aura stacks for stun apply.
                if (AuraPtr frigidAssault = target->GetAura(SPELL_FRIGID_ASSAULT_STACKS))
                {
                    int32 stacks = frigidAssault->GetStackAmount();

                    // Stack handling.
                    if (stacks >= 15 && !target->HasAura(SPELL_FRIGID_ASSAULT_STUN))
                    {
                        target->RemoveAurasDueToSpell(SPELL_FRIGID_ASSAULT_STACKS);
                        caster->AddAura(SPELL_FRIGID_ASSAULT_STUN, target);
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_malakk_frigid_assault_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_malakk_frigid_assault_AuraScript();
        }
};

// Frostbite 136922
class spell_malakk_frostbite : public SpellScriptLoader
{
    public:
        spell_malakk_frostbite() : SpellScriptLoader("spell_malakk_frostbite") { }

        class spell_malakk_frostbite_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_malakk_frostbite_AuraScript);

            void HandleScript(constAuraEffectPtr aurEff)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                if (!caster->ToCreature())
                    return;

                if (AuraPtr dot = target->GetAura(SPELL_FROSTBITE))
                {
                    int32 stacks = dot->GetStackAmount();

                    uint8 playersNear = 0;

                    Map::PlayerList const &PlayerList = caster->GetMap()->GetPlayers();
                    if (!PlayerList.isEmpty())
                    {
                        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        {
                            if (Player* player = i->getSource())
                            {
                                if (target->IsWithinDistInMap(player, 4.0f, true) && !player->HasAura(SPELL_CHILLED_TO_THE_BONE))
                                {
                                    playersNear++;

                                    // Add Body Heat.
                                    if (caster->GetMap()->IsHeroic())
                                        caster->AddAura(SPELL_BODY_HEAT, player);
                                }
                            }
                        }
                    }

                    // Stack handling - cannot remove aura completely (below 1 stack).
                    if (stacks > 1 && playersNear > 0)
                    {
                        caster->ToCreature()->AI()->Talk(SAY_BODY_HEAT);

                        int32 stacksToRemove = (stacks - ((caster->GetMap()->Is25ManRaid() ? 1 : 2) * playersNear)) >= 1 ? (stacks - ((caster->GetMap()->Is25ManRaid() ? 1 : 2) * playersNear)) : (stacks - (stacks - 1));
                        dot->SetStackAmount(stacks - stacksToRemove);
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_malakk_frostbite_AuraScript::HandleScript, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_malakk_frostbite_AuraScript();
        }
};

// Body Heat 137084
class spell_malakk_body_heat : public SpellScriptLoader
{
    public:
        spell_malakk_body_heat() : SpellScriptLoader("spell_malakk_body_heat") { }

        class spell_malakk_body_heat_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_malakk_body_heat_AuraScript);

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                caster->AddAura(SPELL_CHILLED_TO_THE_BONE, target);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_malakk_body_heat_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_malakk_body_heat_AuraScript();
        }
};

// Blessed Loa Spirit 137203
class spell_marli_blessed_loa_spirit : public SpellScriptLoader
{
    public:
        spell_marli_blessed_loa_spirit() : SpellScriptLoader("spell_marli_blessed_loa_spirit") { }

        class spell_marli_blessed_loa_spirit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_marli_blessed_loa_spirit_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                // caster->CastSpell(caster, SPELL_BLESSED_LOA_SPIRIT_S, true);
                caster->SummonCreature(NPC_BLESSED_LOA_SPIRIT, caster->GetPositionX() + 3.0f, caster->GetPositionY(), caster->GetPositionZ() + 0.1f, 0, TEMPSUMMON_MANUAL_DESPAWN);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_marli_blessed_loa_spirit_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_marli_blessed_loa_spirit_SpellScript();
        }
};

// Shadowed Loa Spirit 137350
class spell_marli_shadowed_loa_spirit : public SpellScriptLoader
{
    public:
        spell_marli_shadowed_loa_spirit() : SpellScriptLoader("spell_marli_shadowed_loa_spirit") { }

        class spell_marli_shadowed_loa_spirit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_marli_shadowed_loa_spirit_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                // caster->CastSpell(caster, SPELL_SHADOWED_LOA_SPIRIT_S, true);
                caster->SummonCreature(NPC_SHADOWED_LOA_SPIRIT, caster->GetPositionX() + 3.0f, caster->GetPositionY(), caster->GetPositionZ() + 0.1f, 0, TEMPSUMMON_MANUAL_DESPAWN);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_marli_shadowed_loa_spirit_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_marli_shadowed_loa_spirit_SpellScript();
        }
};

// Twisted Fate 137972 (damage)
class spell_marli_twisted_fate_damage : public SpellScriptLoader
{
    public:
        spell_marli_twisted_fate_damage() : SpellScriptLoader("spell_marli_twisted_fate_damage") { }

        class spell_marli_twisted_fate_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_marli_twisted_fate_damage_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                Unit* otherNpc = NULL;
                if (caster->ToCreature()->GetEntry() == NPC_TWISTED_FATE_1)
                    otherNpc = caster->FindNearestCreature(NPC_TWISTED_FATE_2, 300.0f, true);
                else
                    otherNpc = caster->FindNearestCreature(NPC_TWISTED_FATE_1, 300.0f, true);

                if (!otherNpc) // Single.
                {
                    SetHitDamage(50000);
                    return;
                }

                float distance = caster->GetDistance2d(otherNpc);

                if (distance > 1.0f)
                    SetHitDamage(int32(GetHitDamage() - uint32(2500 * distance))); // Drops by 2500 for every yard the npc is away.

                if (GetHitDamage() < 10000) // Don't let it go lower then 10000.
                    SetHitDamage(10000);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_marli_twisted_fate_damage_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_marli_twisted_fate_damage_SpellScript();
        }
};


/*** INTRO ***/

enum IntroYells
{
    SAY_INTRO_GARAJAL_1 = 0, // Des fools thought they beat me once before. Dey only make me stronger. Now we show dem da true power of the Zandalari! - sound 35404.
    SAY_INTRO_GARAJAL_2,     // Let me share dis gift with ya! - sound 35395.
    SAY_INTRO_GARAJAL_3,     // Da Zandalari cannot be stopped! - sound 35400.
    SAY_INTRO_GARAJAL_4,     // Wit de Thunder King's power de Zandalari will be reborn! - sound 35401.
    SAY_INTRO_GARAJAL_5,     // De Thunder King will reward us for stoppin' ya! - sound 35397.
    SAY_INTRO_GARAJAL_6,     // Lei Shen, let us prove ta ya the might of the Zandalari. We will crush des intruders where dey stand! - sound 35405.
};

// Garajal council intro 70056
class npc_garajal_council_intro : public CreatureScript
{
    public:
        npc_garajal_council_intro() : CreatureScript("npc_garajal_council_intro") { }

        struct npc_garajal_council_introAI : public ScriptedAI
        {
            npc_garajal_council_introAI(Creature* creature) : ScriptedAI(creature) 
            {
                introDone = false;
			}

            bool introDone;

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetDisplayId(11686); // invisible.
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (!introDone && me->IsWithinDistInMap(who, 25.0f, true) && who->GetTypeId() == TYPEID_PLAYER)
                {
                    me->SetReactState(REACT_PASSIVE);
                    me->SetDisplayId(47230);

                    if (me->GetPositionX() > 5681.0f && me->GetPositionX() < 5682.0f)
                    {
                        Talk(SAY_INTRO_GARAJAL_1);
                        me->DespawnOrUnsummon(11000);
                    }

                    if (me->GetPositionX() > 5729.0f && me->GetPositionX() < 5730.0f)
                    {
                        Talk(SAY_INTRO_GARAJAL_2);
                        me->DespawnOrUnsummon(3000);
                    }

                    if (me->GetPositionX() > 5769.0f && me->GetPositionX() < 5770.0f)
                    {
                        Talk(SAY_INTRO_GARAJAL_3);
                        me->DespawnOrUnsummon(3000);
                    }

                    if (me->GetPositionX() > 5863.0f && me->GetPositionX() < 5864.0f)
                    {
                        Talk(SAY_INTRO_GARAJAL_4);
                        me->DespawnOrUnsummon(5000);
                    }

                    if (me->GetPositionX() > 6038.0f && me->GetPositionX() < 6039.0f)
                    {
                        Talk(SAY_INTRO_GARAJAL_5);
                        me->DespawnOrUnsummon(4000);
                    }

                    if (me->GetPositionX() > 6045.0f && me->GetPositionX() < 6046.0f)
                    {
                        Talk(SAY_INTRO_GARAJAL_6);
                        me->DespawnOrUnsummon(8000);
                    }

                    introDone = true;
                }
            }

            void UpdateAI(uint32 const diff) { }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_garajal_council_introAI(creature);
        }
};

void AddSC_boss_council_of_elders()
{
    new boss_spirit_of_gara_jal_council();
    new boss_kazra_jin();
    new boss_sul_the_sandcrawler();
    new boss_frost_king_malakk();
    new boss_high_priestess_mar_li();
    new npc_zabrajin_reckless_charge();
    new npc_sul_quicksand();
    new npc_blessed_loa_spirit();
    new npc_shadowed_loa_spirit();
    new npc_marli_twisted_fate();
    new spell_garajal_councilor_possessed();
    new spell_council_soul_fragment_transfer();
    new spell_malakk_frigid_assault();
    new spell_malakk_frostbite();
    new spell_malakk_body_heat();
    new spell_marli_blessed_loa_spirit();
    new spell_marli_shadowed_loa_spirit();
    new spell_marli_twisted_fate_damage();
    new npc_garajal_council_intro();
}
