/*
* FUCK CREDITS! (SkyMist Gaming © says just enjoy the script. It is not free to use and under copyright law so if you are an unauthorised third party we'll just sew your ass).
*
* Raid: Siege of Orgrimmar.
* Boss: Immerseus.
*
* Wowpedia boss history:
*
* "The ancient inhabitants of Pandaria recognized the vital importance of the lifegiving Pools of Power, 
*  building an underground system of aqueducts to safeguard the waters and nurture life in the Vale of Eternal Blossoms. 
*  The touch of corruption has animated and twisted these waters, and Immerseus stands as an unnatural embodiment of the Vale's sorrow."
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
#include "MoveSplineInit.h"
#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Map.h"
#include "VehicleDefines.h"
#include "SpellInfo.h"

#include "siege_of_orgrimmar.h"

/*
Intro:
Lorewalker Cho yells: Ah, we adventure together again, except this time I am afraid that the circumstances are much more dire.
Lorewalker Cho yells: What has become of the Vale?
Lorewalker Cho yells: The land is scarred! The ever blooming trees and plants wither and die, as the water from the pools drains away.
Lorewalker Cho yells: Come, let us see the other horrors Garrosh has unleashed upon our land.
Lorewalker Cho yells: Once, the Titans used these life-giving waters to create and shape all life in Pandaria.
Lorewalker Cho yells: It is these waters that kept the Vale in bloom. Their runoff into the Valley of the Four Winds created the most fertile farmland in the world!
Lorewalker Cho yells: And now, the malignance of the Old God has manifested itself within the waters.
Lorewalker Cho yells: Such a foul, foul thing - destroy it, before it seeps deep into the soil and corrupts all of Pandaria!

Outro:
Lorewalker Cho yells: Ah, you have done it. The waters are pure once more!
Lorewalker Cho yells: Can you feel their life-giving energies flow through you?
Lorewalker Cho yells: It will take much time for the Vale to heal, but you have given us hope!
*/

enum Yells
{
    /*** IMMERSEUS ***/

    ANN_SPLIT                  = 0, // Immerseus [Splits]!
    ANN_REFORM,                     // Immerseus [Reforms]!
    ANN_SWIRL                       // Immerseus begins to cast [Swirl]!
};

enum Spells
{
    /*** IMMERSEUS ***/

    // Misc - General.
    SPELL_ZERO_POWER            =  72242, // No Energy (Corruption) Regen.

    SPELL_SUBMERGE              = 121541, // Submerge visual (when casted makes boss submerge).
    SPELL_EMERGE                = 139832, // Submerged visual (when removed makes boss emerge).

    // Phase 1 - Tears of the Vale.

    // Sha Bolt
    // On all players, causes Sha Bolt (Sha Splash casting NPC's) to appear under their feet for the duration of the phase.
    /*
        Sha Bolt is an ability that Immerseus uses roughly every 10 seconds. 
        It deals moderate Shadow damage to every raid member (and to anyone within 5 yards of them), and places a small void zone at their location. 
        These void zones persist until the end of the phase. When the phase ends, the void zones will move towards Immerseus and disappear upon reaching him.
    */
    SPELL_SHA_BOLT_DUMMY        = 143290, // Dummy for SPELL_SHA_BOLT on eff 0 to cast it on all players in 500 yd.
    SPELL_SHA_BOLT              = 143293, // Triggers missile 143295 (summons NPC_SHA_BOLT).

    // Swirl
    // Huge ring of water, boss turns around with it, Channeled. On hit / touch deals damage. (IMMERSEUS). Also in the whole room NPC_WHIRL appear under players (ROOM).
    /*
        Swirl is an ability that has two components.
        Immerseus will summon a number of small void zones that move around the room, dealing damage to any players who come in contact with them, and knocking these players up. 
        Immediately afterwards, Immerseus will channel a very damaging jet of water in front of him, and he will spin clockwise for 10 seconds.
        After the 10 seconds are up, the void zones will disappear and Immerseus will resume his usual behaviour.
    */
    // Handled directly in the script, left here as reminder of spells connections.
    SPELL_SWIRL                 = 143309, // Applies Areatrigger 1018 (eff 1) on summoned NPC_SWIRL_TARGET - seems visual for water burst on boss. Eff 0 dummy aura.
    SPELL_SWIRL_DMG_IMMERSEUS   = 143412, // On players coming into contact with waters near Immerseus. Periodic damage aura every 0.25 seconds.
    SPELL_SWIRL_ROOM_SE         = 143415, // Script effect (eff 0) for casting SPELL_SWIRL_ROOM on all triggers in room (NPC_SWIRL - "cracks in the ground").
    SPELL_SWIRL_ROOM            = 143410, // Applies Areatrigger 1024 on all NPC_SWIRL ground crack trigger mobs (visual).
    SPELL_SWIRL_ROOM_DMG        = 143413, // On players coming into contact with waters on NPC_SWIRL "ground crack" trigger mobs. Periodic damage aura every 0.25 seconds.

    // Corrosive Blast
    // Similar to regular Cone Breath attacks.
    /*
        Corrosive Blast is a frontal cone attack that deals Shadow damage to affected players, and debuffs them to take 300% increased Shadow damage for 45 seconds.
        This effect stacks. Immerseus casts this ability every 35 seconds or so. This ability requires a tank switch.
    */
    SPELL_CORROSIVE_BLAST       = 143436, // Damage + Shadow damage taken increase 45 sec.

    // Out of melee range version.
    SPELL_CORROSIVE_BLAST_OUM   = 143437, // Each sec.

    // Swelling Corruption - Heroic only!
    // Cast in the beginning of the normal phase, see below for details.
    /*
        Once per Immerseus Phase, the boss gains a buff called Swelling Corruption. 
        Swelling Corruption has an unlimited duration, and it has a certain number of stacks. 
        Specifically, the buff has one stack for every 2 points of Corruption that the boss has. 
        The very first Swelling Corruption has 50 stacks, and then fewer stacks as goes on.
        Every attack made against the boss removes a stack of the buff, and causes the attacker to 
        gain a stack of a dispellable 6-second Shadow-damage DoT, and to spawn an add called Congealed Sha.
        The DoT's damage increases over the course of its 6-second duration, and the Congealed Sha adds do not do anything except melee their main aggro target.
        Swelling Corruption has no other effects, and once all of its stacks are gone, it disappears.
    */
    // Handled directly in the script, left here as reminder of spells connections.
    SPELL_SWELLING_CORRUPTION   = 143574, // Heroic only! Boss aura for checking player ability attacks (Summons Congealed Sha). Stacks = boss Corruption level. Dummy on eff 0 + 1.
    SPELL_SWELLING_CORRUPTION_D = 143578, // Eff 0 script effect for SPELL_SHA_CORRUPTION_DMG.
    SPELL_SHA_CORRUPTION_DMG    = 143579, // Periodic damage aura on players when they strike Immerseus with abilities. Stacks.
    SPELL_SHA_CORRUPTION_SUMM   = 143580, // Triggers SPELL_SWELLING_CORRUPTION_S missile when players strike Immerseus with abilities. Eff 1 on SPELL_SWELLING_CORUPTION.
    SPELL_SWELLING_CORRUPTION_S = 143581, // Summons NPC_CONGEALED_SHA.

    // Phase 2 - Split.

    // Split.
    // Actual "Split" spell cast at phase change.
    /*
        Split causes Sha Puddles and Contaminated Puddles to erupt from Immerseus that will try to reform in the center of the room.
        For each Sha Puddle killed and each Contaminated Puddle healed to full, Immerseus' energy is reduced by one.
    */
    SPELL_SPLIT                 = 143020, // On eff. 0 stun, eff. 1 Script Effect for add - summoning missiles.
    SPELL_REFORM                = 143469, // On eff. 0 Script Effect for setting Health and Corruption points.

    // Missiles and corresponding add - summoning spells.
    SPELL_SPLIT_SHA_MISSILE     = 143022, // Triggers SPELL_SPLIT_SHA.
    SPELL_SPLIT_SHA             = 143453, // Summons Sha Puddle at location.

    SPELL_SPLIT_CONTAM_MISSILE  = 143024, // Triggers SPELL_SPLIT_CONTAM.
    SPELL_SPLIT_CONTAM          = 143454, // Summons Contaminated Puddle at location.

    // When any Puddle reaches Immerseus, it triggers an eruption, inflicting:
    SPELL_ERRUPTING_SHA         = 143498, // 73125 to 76875 Shadow damage to all players for a Sha or Contaminated Puddle;
    SPELL_ERRUPTING_WATER       = 145377, // 29250 to 30750 Frost damage to all players for a Purified Puddle.

    // When the puddles are destroyed (Sha) / healed fully (Contaminated) before they reach Immerseus, the give to players:
    SPELL_SHA_RESIDUE           = 143459, // Sha Puddle. Applies Sha Residue to enemies within 10 yards. Increases damage dealt to Sha Puddles by 25%. Stacks.
    SPELL_PURIFIED_RESIDUE      = 143524, // Contaminated Puddle. Restores 25% of allies within 10 yards mana. Increases healing done by 75% (also 10 yd).

    // Sha Pool - Heroic only!.
    // The Sha Pool grows over time (+ when a puddle reaches it), inflicting 4000 Shadow damage over 1 sec to enemies who touch it.  Touching the Sha Puddle causes it to shrink.
    // This is mostly cast by the boss on himself.
    /*
        During the Split Phase, Immerseus gains an ability called Sha Pool.
        This causes the center of the room (the area where your raid cannot go without being damaged and knocked back, 
        and the same area where all the adds converge) to become a large void zone.
        Any players standing inside the void zone take a small amount of Shadow damage every second.
        The void zone grows slowly over the course of the Split Phase, and it also grows each time a Sha Puddle, Contaminated Puddle or Purified Puddle reaches it.
        As it increases in size, its damage is also increased. The void zone shrinks in size whenever it damages a raid member, and it disappears as soon as the phase ends.
    */
    SPELL_SHA_POOL_AURA         = 143462, // Triggers 143460 Periodic damage aura on eff 0, SPELL_SHA_POOL_CHECK_GROW script effect 100 yd on eff 1 (check players and grow / shrink).
    SPELL_SHA_POOL_CHECK_GROW   = 143461,

    // Phase 1 + Phase 2 (General).

    // Seeping Sha
    // Cast basically by the boss on himself.Visual areatrigger and damage to players in contact. HAPPENS IN PHASE 2 ALSO!
    /*
        Seeping Sha is essentially a protective barrier that surrounds Immerseus (displayed as a ring of water). 
        Players who get too close to the boss and enter this water take damage and are knocked back.
        This prevents players from walking through the boss, but since his hitbox is very large, it does not interfere with melee players' ability to attack.
    */
    SPELL_SEEPING_SHA           = 143281, // Create Areatrigger 1016 (around Immerseus). 40 yards diameter, base water visual.
    SPELL_SEEPING_SHA_DAMAGE    = 143286, // Coming into contact with the Seeping Sha that surrounds Immerseus inflicts 97500 to 102500 Shadow damage and knocks players back. 

    SPELL_BERSERK               =  64238, // Berserk, Enrage, Bye - Bye or, simply put, a wipe. :)

    /*** SHA BOLT (NPC_SHA_BOLT) ***/

    SPELL_SHA_SPLASH            = 143298, // Creates Areatrigger 1017.
    SPELL_SHA_SPLASH_VISUAL     = 119542, // 3 yard visual.
    SPELL_SHA_SPLASH_DMG        = 143297, // Periodic damage aura on player when splash occurs.

    /*** CONTAMINATED PUDDLE (NPC_CONTAMINATED_PUDDLE) ***/

    SPELL_CONGEALING_AURA       = 143538, // Check health every 500 ms (Periodic Dummy eff 0) and add SPELL_CONGEALING stacks.
    SPELL_CONGEALING            = 143540, // 5% Size mod. Movement speed reduced by 10%. Speed wanes as it increases in health (1 stack / 10% hp).
    SPELL_PURIFIED              = 143523  // Applied when it's healed to full (Dummy visual aura).
};

enum Events
{
    /*** IMMERSEUS ***/

    // First phase (Normal).
    EVENT_SHA_BOLT              = 1,
    EVENT_SWIRL,
    EVENT_CORROSIVE_BLAST,
    EVENT_CORROSIVE_BLAST_OUM, // Melee check.
    EVENT_SWELLING_CORRUPTION,

    // Second phase (Split).
    EVENT_SPLIT,
    EVENT_PUDDLES_MOVE_CENTER,

    // Both phases.
    EVENT_SEEPING_SHA,
    EVENT_BERSERK,

    /*** SHA BOLT (NPC_SHA_BOLT) ***/

    EVENT_SHA_BOLT_ROOM_CHECK,

    /*** SWIRL (NPC_SWIRL) ***/

    EVENT_SWIRL_ROOM_CHECK,

    /*** SWIRL (NPC_SWIRL_TARGET) ***/

    EVENT_MOVE_CIRCLE,
    EVENT_SWIRL_TARGET_CHECK,

    /*** SHA PUDDLE (NPC_SHA_PUDDLE) / CONTAMINATED PUDDLE (NPC_CONTAMINATED_PUDDLE) ***/

    EVENT_MOVE_CENTER
};

enum Actions
{
    /*** SHA PUDDLE / CONTAMINATED PUDDLE ***/

    ACTION_PUDDLES_MOVE_CENTER   = 1,
    ACTION_STOP_SPLASH_CHECK
};

enum Phases
{
    /*** IMMERSEUS ***/

    PHASE_IMMERSEUS_NORMAL       = 1,
    PHASE_IMMERSEUS_SPLIT        = 2
};

enum Npcs
{
    /*** IMMERSEUS ***/

    NPC_SHA_BOLT                 = 71544, // Sha Bolt NPC - goes with Sha Splash.
    NPC_SWIRL                    = 71548, // Swirl NPC (many, in room).
    NPC_SWIRL_TARGET             = 71550, // For the boss channel spell.
    NPC_CONGEALED_SHA            = 71642, // From Swelling Corruption aura, Heroic only!.

    // Always 25 mobs summoned for p2, different types are selected according to Corruption level.
    // Already declared in the header as main mobs.
    // NPC_SHA_PUDDLE               = 71603, // Sha Puddle phase 2 (1 for every 4 Corruption +).
    // NPC_CONTAMINATED_PUDDLE      = 71604  // Contaminated Puddle phase 2 (1 for every 4 Corruption -).
};

enum GOs
{
    GO_TEARS_OF_THE_VALE         = 221776 // Loot chest.
};

#define MAX_SPLIT_PUDDLES        25       // Maximum number of puddles summoned each Split phase.

/*** IMMERSEUS - 71543. ***/
class boss_immerseus : public CreatureScript
{
    public:
        boss_immerseus() : CreatureScript("boss_immerseus") { }

        struct boss_immerseusAI : public BossAI
        {
            boss_immerseusAI(Creature* creature) : BossAI(creature, DATA_IMMERSEUS_EVENT), vehicle(creature->GetVehicleKit()), summons(me)
            {
                instance = creature->GetInstanceScript();
                ASSERT(vehicle);                // Purple energy bar. Vehicle Id 2116.
            }

            InstanceScript* instance;
            Vehicle* vehicle;
            EventMap events;
            SummonList summons;
            uint8 phase;

            uint8 corruptionToRemove;           // Sha Puddles killed / Contaminated Puddles healed in Split phase each increase it by 1. Used to set boss Corruption @ Phase 1 RR.
            bool secondPhaseInProgress;         // Used as an UpdateAI diff check.

            uint8 shaPuddlesToSummon;           // For Split phase. 1 / each 4 Corruption points boss has.
            uint8 contaminatedPuddlesToSummon;  // For Split phase. 1 / each 4 Corruption points boss lacks.

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                me->SetPower(POWER_ENERGY, 100);

                DoCast(me, SPELL_ZERO_POWER);
                if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                phase = PHASE_IMMERSEUS_NORMAL;

                corruptionToRemove = 0;
                secondPhaseInProgress = false;

                shaPuddlesToSummon = 0;
                contaminatedPuddlesToSummon = 0;

                if (instance)
                    instance->SetData(DATA_IMMERSEUS_EVENT, NOT_STARTED);

                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                events.ScheduleEvent(EVENT_SEEPING_SHA,       100);
                events.ScheduleEvent(EVENT_SHA_BOLT,         5000);
                events.ScheduleEvent(EVENT_CORROSIVE_BLAST, 10000);
                events.ScheduleEvent(EVENT_SWIRL,           20000);

                if (me->GetMap()->IsHeroic())
                    events.ScheduleEvent(EVENT_SWELLING_CORRUPTION, 3000);

                events.ScheduleEvent(EVENT_CORROSIVE_BLAST_OUM, 4000); // Melee check (tank).
                events.ScheduleEvent(EVENT_BERSERK,        605000); // 6 minutes, 5 seconds according to logs.

                if (instance)
                {
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add.
                    instance->SetData(DATA_IMMERSEUS_EVENT, IN_PROGRESS);
                }

                _EnterCombat();
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                // Handle Split phase entrance and event ending.
                if (phase == PHASE_IMMERSEUS_NORMAL && damage >= me->GetHealth())
                {
                    damage = 0;

                    if (me->GetPower(POWER_ENERGY) > 10) // Split phase.
                        ChangePhase(PHASE_IMMERSEUS_SPLIT);
                    else // Boss is done.
                    {
                        JustDied(attacker);

                        me->setFaction(35);
                        me->RemoveAllAuras();
                        me->DeleteThreatList();
                        me->CombatStop(true);
                        me->SetFullHealth();
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                        attacker->SummonGameObject(GO_TEARS_OF_THE_VALE, 1458.48f, 716.72f, 246.84f, 5.22f, 0.0f, 0.0f, 0.0f, 0.0f, RESPAWN_IMMEDIATELY, 0);
                    }
                }
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (!instance)
                    return;

                if (instance->GetData(DATA_IMMERSEUS_EVENT) == DONE)
                    return;

                // Handle Emerge / Submerge mechanics.
                if (spell->Id == SPELL_SUBMERGE)
                {
                    me->AddAura(SPELL_EMERGE, me);
                    me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                }

                // Swelling Corruption eff. 0 + 1 handling - player ability cast on boss. Aura addition includes stack increase. Summon NPCs.
                // A bit of a workaround, but done here because there are no periodic check dummy triggers on the spell itself.
                if (me->HasAura(SPELL_SWELLING_CORRUPTION) && caster->GetTypeId() == TYPEID_PLAYER)
                {
                    me->AddAura(SPELL_SHA_CORRUPTION_DMG, caster);
                    DoCast(caster, SPELL_SHA_CORRUPTION_SUMM);
                }
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                if (instance && summon->GetEntry() == NPC_SHA_BOLT)
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHA_SPLASH_DMG);

                if (summon->GetEntry() == NPC_SWIRL_TARGET)
                {
                    if (instance)
                    {
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SWIRL_DMG_IMMERSEUS);
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SWIRL_ROOM_DMG);
                    }
                }

                summons.Despawn(summon);
            }

            // Used to despawn all summons having a specific entry.
            void DespawnSummon(uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        (*summs)->DespawnOrUnsummon();
            }

            // Used to move and despawn all Sha Bolt Puddles.
            void MoveAndDespawnPuddles()
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, NPC_SHA_BOLT, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        (*summs)->AI()->DoAction(ACTION_STOP_SPLASH_CHECK);
            }

            // Used as an internal function for calling Boss Phase changes.
            void ChangePhase(uint8 bossPhase)
            {
                // Just preventing processing of any unwanted calls.
                if (bossPhase > PHASE_IMMERSEUS_SPLIT)
                    return;

                switch (bossPhase)
                {
                    case PHASE_IMMERSEUS_NORMAL: // When all adds are killed / healed or reached boss.
                        phase = PHASE_IMMERSEUS_NORMAL;
                        if (int32 powersRemaining = me->GetPower(POWER_ENERGY) - corruptionToRemove)
                        {
                            if (uint32 healthRemaining = me->GetMaxHealth() * (0.01 * powersRemaining))
                                me->SetHealth(healthRemaining < 1 ? 1 : healthRemaining); // Works like percentage calculation (MaxHealth * (100% - (1% * KilledHealedAdds))).
                            me->SetPower(POWER_ENERGY, powersRemaining);
                        }
                        me->RemoveAurasDueToSpell(SPELL_SPLIT);
                        me->RemoveAurasDueToSpell(SPELL_EMERGE); // Emerge.
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        events.ScheduleEvent(EVENT_SHA_BOLT,         5000);
                        events.ScheduleEvent(EVENT_CORROSIVE_BLAST, 10000);
                        events.ScheduleEvent(EVENT_SWIRL,           20000);
                        events.ScheduleEvent(EVENT_CORROSIVE_BLAST_OUM, 4000);
                        if (me->GetMap()->IsHeroic())
                        {
                            me->RemoveAurasDueToSpell(SPELL_SHA_POOL_AURA);
                            events.ScheduleEvent(EVENT_SWELLING_CORRUPTION, 3000);
                        }
                        if (instance)
                            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEEPING_SHA_DAMAGE);
                        corruptionToRemove = 0;
                        shaPuddlesToSummon = 0;
                        contaminatedPuddlesToSummon = 0;
                        break;

                    case PHASE_IMMERSEUS_SPLIT: // When boss "dies" (reaches < 1 health).
                        phase = PHASE_IMMERSEUS_SPLIT;
                        MoveAndDespawnPuddles();
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        events.CancelEvent(EVENT_SHA_BOLT);
                        events.CancelEvent(EVENT_CORROSIVE_BLAST);
                        events.CancelEvent(EVENT_SWIRL);
                        events.CancelEvent(EVENT_CORROSIVE_BLAST_OUM);
                        if (me->GetMap()->IsHeroic())
                        {
                            me->RemoveAurasDueToSpell(SPELL_SWELLING_CORRUPTION);
                            events.CancelEvent(EVENT_SWELLING_CORRUPTION);
                            me->AddAura(SPELL_SHA_POOL_AURA, me);
                        }
                        if (instance)
                            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEEPING_SHA_DAMAGE);
                        events.ScheduleEvent(EVENT_SPLIT, 200);
                        break;

                    default: break;
                }
            }

            // Used as an Add counter function for SPELL_SPLIT calculations.
            void CalculateShaContaminatedPuddles()
            {
                // Generate some local variables, calculate the correct boss ones and make any necessary extra checks.
                uint8 myEnergy = me->GetPower(POWER_ENERGY);
                uint8 myEnergyMissing = 100 - myEnergy;

                shaPuddlesToSummon = ((myEnergy % 4) == 0 ? (myEnergy / 4) : ((myEnergy - (myEnergy % 4)) / 4)); // Select the lowest number.
                if (shaPuddlesToSummon < 1)
                    shaPuddlesToSummon = 1;
                if (shaPuddlesToSummon > 24)
                    shaPuddlesToSummon = 24;

                contaminatedPuddlesToSummon = ((myEnergyMissing % 4) == 0 ? (myEnergyMissing / 4) : ((myEnergyMissing + (myEnergyMissing % 4)) / 4)); // Select the highest number.
                if (contaminatedPuddlesToSummon < 1)
                    contaminatedPuddlesToSummon = 1;
                if (contaminatedPuddlesToSummon > 24)
                    contaminatedPuddlesToSummon = 24;

                uint8 totalAddsToSummon = shaPuddlesToSummon + contaminatedPuddlesToSummon;

                // Far fetched to believe we get here, but worth to check.
                if (totalAddsToSummon < MAX_SPLIT_PUDDLES)
                {
                    // Add counts to a reasonable add entry, depending on Corruption points level.
                    if (myEnergy > 50 && contaminatedPuddlesToSummon < shaPuddlesToSummon)
                        contaminatedPuddlesToSummon += MAX_SPLIT_PUDDLES - totalAddsToSummon;
                    else
                        shaPuddlesToSummon += MAX_SPLIT_PUDDLES - totalAddsToSummon;
                }
                else if (totalAddsToSummon > MAX_SPLIT_PUDDLES)
                {
                    // Substract counts from a reasonable add entry, depending on Corruption points level.
                    if (myEnergy > 50 && contaminatedPuddlesToSummon < shaPuddlesToSummon)
                        shaPuddlesToSummon -= totalAddsToSummon - MAX_SPLIT_PUDDLES;
                    else
                        contaminatedPuddlesToSummon -= totalAddsToSummon - MAX_SPLIT_PUDDLES;
                }
            }

            void KilledUnit(Unit* who) { } // He does nothing :(.

			void EnterEvadeMode()
            {
                DespawnSummon(NPC_SHA_BOLT);

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->AddUnitState(UNIT_STATE_EVADE);

                me->RemoveAllAuras();
                Reset();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEEPING_SHA_DAMAGE);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHA_SPLASH_DMG);
                    instance->SetData(DATA_IMMERSEUS_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove.
                }

                _EnterEvadeMode();
            }

            void JustReachedHome()
            {
                // Prevent calling this if boss is not in Evade mode.
                if (!me->HasUnitState(UNIT_STATE_EVADE))
                    return;

                me->ClearUnitState(UNIT_STATE_EVADE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                DoCast(me, SPELL_ZERO_POWER);
                me->SetPower(POWER_ENERGY, 100);

                _JustReachedHome();
            }

            void JustDied(Unit* killer)
            {
                summons.DespawnAll();
                DespawnSummon(NPC_SHA_BOLT);

                if (instance)
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEEPING_SHA_DAMAGE);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHA_SPLASH_DMG);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove.
                    instance->SetData(DATA_IMMERSEUS_EVENT, DONE);
                }

                _JustDied();
            }

            void UpdateAI(uint32 const diff)
            {
                // Handle Normal phase entrance.
                if (!me->FindNearestCreature(NPC_SHA_PUDDLE, 200.0f, true) && !me->FindNearestCreature(NPC_CONTAMINATED_PUDDLE, 200.0f, true) && secondPhaseInProgress)
                {
                    secondPhaseInProgress = false;
                    Talk(ANN_REFORM);
                    DoCast(me, SPELL_REFORM);
                }

                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                // if (instance && instance->IsWipe())
                // {
                //     EnterEvadeMode();
                //     return;
                // }

                events.Update(diff);

                // Handle Seeping Sha (Normally handled as an Areatrigger check, but can be just as good checked here for periodic damage aura addition).
                if (me->HasAura(SPELL_SEEPING_SHA))
                {
                    Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                    if (!PlayerList.isEmpty())
                    {
                        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        {
                            if (Player* player = i->getSource())
                            {
                                if (player->GetDistance(me->GetHomePosition()) <= 20.0f) // Check if the player is too close and doesn't have the aura.
                                {
                                    if (!player->HasAura(SPELL_SEEPING_SHA_DAMAGE))
                                        DoCast(player, SPELL_SEEPING_SHA_DAMAGE);
                                }
                                else                                            // If the player is further and has the aura, remove it.
                                {
                                    if (player->HasAura(SPELL_SEEPING_SHA_DAMAGE))
                                        player->RemoveAurasDueToSpell(SPELL_SEEPING_SHA_DAMAGE);
                                }
                            }
                        }
                    }
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        // Phase 1 - Tears of the Vale.

                        case EVENT_SHA_BOLT:
                        {
                            if (phase == PHASE_IMMERSEUS_NORMAL)
                                DoCast(me, SPELL_SHA_BOLT_DUMMY);
                            events.ScheduleEvent(EVENT_SHA_BOLT, urand(11000, 15000)); // 6 - 20 seconds in logs.
                            break;
                        }

                        case EVENT_SWIRL:
                        {
                            if (phase == PHASE_IMMERSEUS_NORMAL)
                            {
                                Talk(ANN_SWIRL);

                                // Summon Swirl NPC's.
                                Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                                if (!PlayerList.isEmpty())
                                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                        if (Player* player = i->getSource())
                                            me->SummonCreature(NPC_SWIRL, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 13100);

                                // Select one of the 4 cardinal points - 1.67f, 3.14f, 4.81f, 6.28f and set the boss orientation to it.
                                float angle = RAND(0.5f, 1.0f, 1.5f, 2.0f) * M_PI;
                                me->SetFacingTo(angle);

                                // Now get a close point at 20 yards in front of the boss, summon the target NPC there and cast the spell on it.
                                float x, y, z;
                                me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 20.0f);

                                if (Creature* swirlTarget = me->SummonCreature(NPC_SWIRL_TARGET, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 13100))
                                    DoCast(swirlTarget, SPELL_SWIRL);
                            }
                            events.ScheduleEvent(EVENT_SWIRL, urand(46500, 50500)); // 48.5 seconds in logs.
                            break;
                        }

                        case EVENT_CORROSIVE_BLAST:
                        {
                            if (phase == PHASE_IMMERSEUS_NORMAL)
                                if (Unit* mainTarget = me->getVictim())
                                    DoCast(mainTarget, SPELL_CORROSIVE_BLAST);
                            events.ScheduleEvent(EVENT_CORROSIVE_BLAST, urand(33000, 37000)); // 35 seconds in logs.
                            break;
                        }

                        case EVENT_SWELLING_CORRUPTION:
                        {
                            // Add the correct amount of stacks, considering Even / Odd Corruption power points remaining.
                            if (phase == PHASE_IMMERSEUS_NORMAL)
                            {
                                uint8 stackAmount = (me->GetPower(POWER_ENERGY) % 2 == 0) ? (me->GetPower(POWER_ENERGY) / 2) : ((me->GetPower(POWER_ENERGY) - 1) / 2);

                                if (!me->HasAura(SPELL_SWELLING_CORRUPTION))
                                {
                                    if (AuraPtr swellingCorruption = me->AddAura(SPELL_SWELLING_CORRUPTION, me))
                                        swellingCorruption->SetStackAmount(stackAmount);
                                }
                                else
                                {
                                    if (AuraPtr swellingCorruption = me->GetAura(SPELL_SWELLING_CORRUPTION))
                                        swellingCorruption->SetStackAmount(stackAmount);
                                }
                            }
                            events.ScheduleEvent(EVENT_SWELLING_CORRUPTION, 75000); // 75 seconds in logs.
                            break;
                        }

                        // Phase 2 - Split.

                        case EVENT_SPLIT:
                        {
                            Talk(ANN_SPLIT);
                            CalculateShaContaminatedPuddles();
                            DoCast(me, SPELL_SPLIT);
                            events.ScheduleEvent(EVENT_PUDDLES_MOVE_CENTER, 10000);
                            break;
                        }

                        case EVENT_PUDDLES_MOVE_CENTER:
                        {
                            std::list<Creature*> shaContaminatedPuddleList;
                            GetCreatureListWithEntryInGrid(shaContaminatedPuddleList, me, NPC_SHA_PUDDLE, 150.0f);
                            GetCreatureListWithEntryInGrid(shaContaminatedPuddleList, me, NPC_CONTAMINATED_PUDDLE, 150.0f);
                            if (!shaContaminatedPuddleList.empty())
                                for (std::list<Creature*>::iterator shaContaminatedPuddle = shaContaminatedPuddleList.begin(); shaContaminatedPuddle != shaContaminatedPuddleList.end(); shaContaminatedPuddle++)
                                    (*shaContaminatedPuddle)->AI()->DoAction(ACTION_PUDDLES_MOVE_CENTER);
                            secondPhaseInProgress = true;
                            break;
                        }

                        // Phase 1 + Phase 2 (General).

                        case EVENT_SEEPING_SHA:
                        {
                            // This works like a check, basically the boss should always have this aura (in both phases).
                            if (!me->HasAura(SPELL_SEEPING_SHA))
                                DoCast(me, SPELL_SEEPING_SHA);
                            events.ScheduleEvent(EVENT_SEEPING_SHA, 1000); // Just a check, no logs needed / indications.
                            break;
                        }

                        case EVENT_CORROSIVE_BLAST_OUM:
                            if (phase == PHASE_IMMERSEUS_NORMAL)
                                if (me->getVictim()->GetDistance(me->GetHomePosition()) > 40.0f)
                                    DoCast(me->getVictim(), SPELL_CORROSIVE_BLAST_OUM);
                            events.ScheduleEvent(EVENT_CORROSIVE_BLAST_OUM, 2000);
                            break;

                        case EVENT_BERSERK:
                            DoCast(me, SPELL_BERSERK);
                            break;

                        default: break;
                    }
                }

                if (phase == PHASE_IMMERSEUS_NORMAL)
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_immerseusAI(creature);
        }
};

// NPCs.

/*** SHA BOLT - 71544. ***/
class npc_sha_splash_bolt_immerseus : public CreatureScript
{
    public:
        npc_sha_splash_bolt_immerseus() : CreatureScript("npc_sha_splash_bolt_immerseus") { }

        struct npc_sha_splash_bolt_immerseusAI : public ScriptedAI
        {
            npc_sha_splash_bolt_immerseusAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                DoCast(me, SPELL_SHA_SPLASH);
                me->AddAura(SPELL_SHA_SPLASH_VISUAL, me);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);

                events.ScheduleEvent(EVENT_SHA_BOLT_ROOM_CHECK, 200);
            }

            void Reset()
            {
                events.Reset();
            }

            void DoAction(int32 const action)
            {
                // Just preventing processing of any unwanted calls.
                if (action != ACTION_STOP_SPLASH_CHECK)
                    return;

                switch (action)
                {
                    case ACTION_STOP_SPLASH_CHECK:
                        events.CancelEvent(EVENT_SHA_BOLT_ROOM_CHECK);
                        if (instance)
                            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SHA_SPLASH_DMG);
                        me->SetSpeed(MOVE_WALK, 3.0f);
                        me->SetSpeed(MOVE_RUN, 3.0f);
                        if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 150.0f, true))
                            me->GetMotionMaster()->MovePoint(1, Immerseus->GetHomePosition().GetPositionX(), Immerseus->GetHomePosition().GetPositionY(), Immerseus->GetHomePosition().GetPositionZ());
                        me->DespawnOrUnsummon(3000);
                        break;

                    default: break;
                }
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHA_BOLT_ROOM_CHECK:
                        {
                            // Check for players.
                            if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 300.0f, true))
				            {
                                Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                                if (!PlayerList.isEmpty())
                                {
                                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                    {
                                        if (Player* player = i->getSource())
                                        {
                                            if (me->GetDistance(player) <= 3.0f) // Check if the player is too close and doesn't have the aura.
                                            {
                                                if (!player->HasAura(SPELL_SHA_SPLASH_DMG))
                                                    me->AddAura(SPELL_SHA_SPLASH_DMG, player);
                                            }
                                            else
                                            {
                                                if (!player->FindNearestCreature(NPC_SHA_BOLT, 3.0f, true)) // Else if the player is further and has the aura, remove it.
                                                    if (player->HasAura(SPELL_SHA_SPLASH_DMG))
                                                        player->RemoveAurasDueToSpell(SPELL_SHA_SPLASH_DMG);
                                            }
                                        }
                                    }
                                }
				            }
                            events.ScheduleEvent(EVENT_SHA_BOLT_ROOM_CHECK, 200);
                            break;
                        }

                        default: break;
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sha_splash_bolt_immerseusAI(creature);
        }
};

/*** SWIRL - 71548. ***/
class npc_swirl_immerseus : public CreatureScript
{
    public:
        npc_swirl_immerseus() : CreatureScript("npc_swirl_immerseus") { }

        struct npc_swirl_immerseusAI : public ScriptedAI
        {
            npc_swirl_immerseusAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                DoCast(me, SPELL_SWIRL_ROOM); // 3 sec cast, after kicks in periodic damage aura check.
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                me->GetMotionMaster()->MoveRandom(10.0f); // Move random in a 10y radius.
                events.ScheduleEvent(EVENT_SWIRL_ROOM_CHECK, 3100);
            }

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(uint32 const diff)
            {
                // Ensure the npc always has the visual aura.
                if (!me->HasUnitState(UNIT_STATE_CASTING))
                    DoCast(me, SPELL_SWIRL_ROOM);

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SWIRL_ROOM_CHECK:
                        {
                            if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 300.0f, true))
                            {
                                Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                                if (!PlayerList.isEmpty())
                                {
                                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                    {
                                        if (Player* player = i->getSource())
                                        {
                                            if (me->GetDistance(player) <= 2.0f) // Check if the player is too close and doesn't have the aura.
                                            {
                                                if (!player->HasAura(SPELL_SWIRL_ROOM_DMG))
                                                    player->CastSpell(player, SPELL_SWIRL_ROOM_DMG, true);
                                            }
                                            else
                                            {
                                                if (!player->FindNearestCreature(NPC_SWIRL, 2.0f, true)) // Else if the player is further and has the aura, remove it.
                                                    if (player->HasAura(SPELL_SWIRL_ROOM_DMG))
                                                        player->RemoveAurasDueToSpell(SPELL_SWIRL_ROOM_DMG);
                                            }
                                        }
                                    }
                                }
                            }
                            events.ScheduleEvent(EVENT_SWIRL_ROOM_CHECK, 100);
                            break;
                        }

                        default: break;
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_swirl_immerseusAI(creature);
        }
};

/*** SWIRL (Boss spell target) - 71550. ***/
class npc_swirl_target_immerseus : public CreatureScript
{
    public:
        npc_swirl_target_immerseus() : CreatureScript("npc_swirl_target_immerseus") { }

        struct npc_swirl_target_immerseusAI : public ScriptedAI
        {
            npc_swirl_target_immerseusAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            // Function for creating a circle around the boss to move around.
            void FillCirclePath(Position const& centerPos, float radius, float z, Movement::PointsArray& path)
            {
                float step = -M_PI / 8.0f; // Clockwise.
                float angle = centerPos.GetAngle(me->GetPositionX(), me->GetPositionY());
            
                for (uint8 i = 0; i < 16; angle += step, ++i)
                {
                    G3D::Vector3 point;
                    point.x = centerPos.GetPositionX() + radius * cosf(angle);
                    point.y = centerPos.GetPositionY() + radius * sinf(angle);
                    point.z = z;
                    path.push_back(point);
                }
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                events.ScheduleEvent(EVENT_MOVE_CIRCLE, 3100);
                events.ScheduleEvent(EVENT_SWIRL_TARGET_CHECK, 3200);
            }

            void Reset()
            {
                events.Reset();
            }

            void UpdateAI(uint32 const diff)
            {
                // Handle Immerseus Swirl rotation according to NPC position.
                if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 150.0f, true))
                    if (Immerseus->HasAura(SPELL_SWIRL))
						Immerseus->SetFacingToObject(me);

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOVE_CIRCLE:
                        {
                            if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 150.0f, true))
                            {
                                Movement::MoveSplineInit init(*me);
                                FillCirclePath(Immerseus->GetHomePosition(), me->GetDistance(Immerseus), me->GetPositionZ(), init.Path());
                                init.SetWalk(true);
                                init.SetCyclic();
                                init.SetSmooth();
                                init.Launch();
                                me->SetWalk(false);
                            }
                            break;
                        }

                        case EVENT_SWIRL_TARGET_CHECK:
                        {
                            if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 200.0f, true))
                            {
                                Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                                if (!PlayerList.isEmpty())
                                {
                                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                    {
                                        if (Player* player = i->getSource())
                                        {
                                            if (me->GetDistance(player) <= 10.0f) // Check if the player is too close and doesn't have the aura.
                                            {
                                                if (!player->HasAura(SPELL_SWIRL_DMG_IMMERSEUS))
                                                    player->CastSpell(player, SPELL_SWIRL_DMG_IMMERSEUS, true);
                                            }
                                            else
                                            {
                                                if (player->HasAura(SPELL_SWIRL_DMG_IMMERSEUS)) // Else if the player is further and has the aura, remove it.
                                                    player->RemoveAurasDueToSpell(SPELL_SWIRL_DMG_IMMERSEUS);
                                            }
                                        }
                                    }
                                }
                            }
                            events.ScheduleEvent(EVENT_SWIRL_TARGET_CHECK, 200);
                            break;
                        }

                        default: break;
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_swirl_target_immerseusAI(creature);
        }
};

/*** SHA PUDDLE - 71603. ***/
class npc_sha_puddle_immerseus : public CreatureScript
{
    public:
        npc_sha_puddle_immerseus() : CreatureScript("npc_sha_puddle_immerseus") { }

        struct npc_sha_puddle_immerseusAI : public ScriptedAI
        {
            npc_sha_puddle_immerseusAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                reachedBoss = false;
            }

            InstanceScript* instance;
            EventMap events;
            bool reachedBoss;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
				me->SetInCombatWithZone();
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset()
            {
                events.Reset();
            }

            void DoAction(int32 const action)
            {
                // Just preventing processing of any unwanted calls.
                if (action > ACTION_PUDDLES_MOVE_CENTER)
                    return;

                switch (action)
                {
                    case ACTION_PUDDLES_MOVE_CENTER:
                        events.ScheduleEvent(EVENT_MOVE_CENTER, 500);
                        break;

                    default: break;
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 1.0f, true))
                {
                    if (me->GetDistance(Immerseus->GetHomePosition()) <= 5.0f && !reachedBoss)
                    {
                        reachedBoss = true;
                        me->GetMotionMaster()->MovementExpired();
                        me->DespawnOrUnsummon(200);
                        DoCast(me, SPELL_ERRUPTING_SHA, true);
                    }
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOVE_CENTER:
                            if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 150.0f, true))
                                me->GetMotionMaster()->MovePoint(1, Immerseus->GetHomePosition().GetPositionX(), Immerseus->GetHomePosition().GetPositionY(), Immerseus->GetHomePosition().GetPositionZ());
                            break;

                        default: break;
                    }
                }

                // No melee.
            }

            void JustDied(Unit* killer)
            {
                // Set boss corruption to remove to +1.
                if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 200.0f, true))
                    CAST_AI(boss_immerseus::boss_immerseusAI, Immerseus->AI())->corruptionToRemove += 1;

                // Cast the buffs.
                DoCast(me, SPELL_SHA_RESIDUE, true);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sha_puddle_immerseusAI(creature);
        }
};

/*** CONTAMINATED PUDDLE - 71604. ***/
class npc_contaminated_puddle_immerseus : public CreatureScript
{
    public:
        npc_contaminated_puddle_immerseus() : CreatureScript("npc_contaminated_puddle_immerseus") { }

        struct npc_contaminated_puddle_immerseusAI : public ScriptedAI
        {
            npc_contaminated_puddle_immerseusAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
                reachedBoss = false;
            }

            InstanceScript* instance;
            EventMap events;
            bool reachedBoss;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
				me->SetInCombatWithZone();
                me->SetHealth(me->CountPctFromMaxHealth(1));
                me->AddAura(SPELL_CONGEALING_AURA, me);
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset()
            {
                events.Reset();
            }

            void DoAction(int32 const action)
            {
                // Just preventing processing of any unwanted calls.
                if (action > ACTION_PUDDLES_MOVE_CENTER)
                    return;

                switch (action)
                {
                    case ACTION_PUDDLES_MOVE_CENTER:
                        events.ScheduleEvent(EVENT_MOVE_CENTER, 500);
                        break;

                    default: break;
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 1.0f, true))
                {
                    if (me->GetDistance(Immerseus->GetHomePosition()) <= 5.0f && !reachedBoss)
                    {
                        reachedBoss = true;
                        me->GetMotionMaster()->MovementExpired();
                        me->DespawnOrUnsummon(200);
                        if (!me->HasAura(SPELL_PURIFIED))
                            DoCast(me, SPELL_ERRUPTING_SHA, true);
                        else
                            DoCast(me, SPELL_ERRUPTING_WATER, true);
                    }
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOVE_CENTER:
                            if (Creature* Immerseus = me->FindNearestCreature(BOSS_IMMERSEUS, 150.0f, true))
                                me->GetMotionMaster()->MovePoint(1, Immerseus->GetHomePosition().GetPositionX(), Immerseus->GetHomePosition().GetPositionY(), Immerseus->GetHomePosition().GetPositionZ());
                            break;

                        default: break;
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_contaminated_puddle_immerseusAI(creature);
        }
};

// SPELLS.

/*** SHA BOLT (Dummy) - 143290. ***/
class spell_immerseus_sha_bolt : public SpellScriptLoader
{
    public:
        spell_immerseus_sha_bolt() : SpellScriptLoader("spell_immerseus_sha_bolt") { }

        class spell_immerseus_sha_bolt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_immerseus_sha_bolt_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                if (caster->ToCreature()->GetEntry() != BOSS_IMMERSEUS)
                    return;

                Map::PlayerList const &PlayerList = caster->GetMap()->GetPlayers();
                if (!PlayerList.isEmpty())
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        if (Player* player = i->getSource())
                            if (caster->IsWithinDistInMap(player, 150.0f, true))
                                caster->CastSpell(player, SPELL_SHA_BOLT, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_immerseus_sha_bolt_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_immerseus_sha_bolt_SpellScript();
        }
};

/*** SPLIT - 143020. ***/
class spell_immerseus_split : public SpellScriptLoader
{
    public:
        spell_immerseus_split() : SpellScriptLoader("spell_immerseus_split") { }

        class spell_immerseus_split_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_immerseus_split_SpellScript);

            void PreventStun()
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                // No need to stun the boss.
                PreventHitDefaultEffect(EFFECT_0);
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                if (!caster->ToCreature())
                    return;

                if (caster->ToCreature()->GetEntry() != BOSS_IMMERSEUS)
                    return;

                // Trigger the mob summon missiles (85 yard radius).
                uint8 shaPuddles = CAST_AI(boss_immerseus::boss_immerseusAI, caster->ToCreature()->AI())->shaPuddlesToSummon;
                uint8 conPuddles = CAST_AI(boss_immerseus::boss_immerseusAI, caster->ToCreature()->AI())->contaminatedPuddlesToSummon;

                for (uint8 i = 0; i < shaPuddles; i++)
                    caster->CastSpell(caster, SPELL_SPLIT_SHA_MISSILE, true);

                for (uint8 i = 0; i < conPuddles; i++)
                    caster->CastSpell(caster, SPELL_SPLIT_CONTAM_MISSILE, true);

                // Handle Submerging.
                caster->CastSpell(caster, SPELL_SUBMERGE, false);
            }

            void Register()
            {
                BeforeHit += SpellHitFn(spell_immerseus_split_SpellScript::PreventStun);
                OnEffectHitTarget += SpellEffectFn(spell_immerseus_split_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_immerseus_split_SpellScript();
        }
};

/*** REFORM - 143469. ***/
class spell_immerseus_reform : public SpellScriptLoader
{
    public:
        spell_immerseus_reform() : SpellScriptLoader("spell_immerseus_reform") { }

        class spell_immerseus_reform_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_immerseus_reform_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                if (!caster->ToCreature())
                    return;

                if (caster->ToCreature()->GetEntry() != BOSS_IMMERSEUS)
                    return;

                CAST_AI(boss_immerseus::boss_immerseusAI, caster->ToCreature()->AI())->ChangePhase(PHASE_IMMERSEUS_NORMAL);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_immerseus_reform_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_immerseus_reform_SpellScript();
        }
};

/*** CONGEALING (Aura) - 143538. ***/
class spell_immerseus_congealing : public SpellScriptLoader
{
    public:
        spell_immerseus_congealing() : SpellScriptLoader("spell_immerseus_congealing") { }

        class spell_immerseus_congealing_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_immerseus_congealing_AuraScript);

            void OnPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                // Calculate correct stacks count (+1 for every 10% health healed).
                uint8 stackCount = floor(caster->GetHealthPct() / 10);

                // Should not add anything if stacks should be 0. 
                if (stackCount < 1)
                    return;

                // Add the aura and set the stack count.
                if (stackCount >= 1 && !caster->GetAura(SPELL_CONGEALING))
                {
                    if (AuraPtr congealing = caster->AddAura(SPELL_CONGEALING, caster))
                        congealing->SetStackAmount(stackCount);
                }
                else
                {
                    if (AuraPtr congealing = caster->GetAura(SPELL_CONGEALING))
                        congealing->SetStackAmount(stackCount);
                }

                // Handle Purified aura addition and cast the buffs.
                if (stackCount == 10 && caster->HealthAbovePct(99) && !caster->HasAura(SPELL_PURIFIED))
                {
                    caster->AddAura(SPELL_PURIFIED, caster);

                    // Set boss corruption to remove to +1.
                    if (Creature* Immerseus = caster->FindNearestCreature(BOSS_IMMERSEUS, 150.0f, true))
                        CAST_AI(boss_immerseus::boss_immerseusAI, Immerseus->AI())->corruptionToRemove += 1;

                    caster->CastSpell(caster, SPELL_PURIFIED_RESIDUE, true);
                    caster->ToCreature()->DespawnOrUnsummon(200);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_immerseus_congealing_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_immerseus_congealing_AuraScript();
        }
};

void AddSC_immerseus()
{
    new boss_immerseus();
    new npc_sha_splash_bolt_immerseus();
    new npc_swirl_immerseus();
    new npc_swirl_target_immerseus();
    new npc_sha_puddle_immerseus();
    new npc_contaminated_puddle_immerseus();
    new spell_immerseus_sha_bolt();
    new spell_immerseus_split();
    new spell_immerseus_reform();
    new spell_immerseus_congealing();
}
