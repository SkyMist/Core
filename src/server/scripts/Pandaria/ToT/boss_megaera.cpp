/*
*
* SkyMist Gaming © says just enjoy the script. It is not free to use and under copyright law so if you are an unauthorised third party we'll just sew your ass.
*
* Raid: Throne of Thunder.
* Boss: Megaera.
*
* Wowpedia boss history:
*
* "Even the most callous among the mogu quaver at the thought of the dark experiments performed at the Thunder King's behest.
*  Long ago, a young cloud serpent was twisted into a multi-headed hydra and left to languish beneath Lei Shen's throne.
*  Now the twisted creature lurks deep in the Forgotten Depths, awaiting the moment it may finally inflict its terrible agony upon others."
*
* ! 3 heads visible, 2 front + 1 back. 7 heads to kill -> win, 15% hp loss per head.
* ! TODO: Diffusion dummy for 10% healing redirection.
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
    // Boss
    ANN_RAMPAGE                 = 0,      // Megaera begins to [Rampage].
    ANN_SUBSIDE                           // Megaera's rage subsides.
};

enum Spells
{
    // Boss
    SPELL_SUBMERGE              = 121541, // Submerge visual (when casted makes head submerge).
    SPELL_EMERGE                = 139832, // Submerged visual (when removed makes head emerge).

    SPELL_CONCEALING_FOG        = 137973, // Dummy on eff 0 for attackable state (body + back head).

    SPELL_WATER_WALK            = 143333, // Spell used to let the heads stand above water.

    // Rampage - When a head dies.
    SPELL_RAMPAGE               = 139458, // Periodic dummy on eff 0 for damage increase on the specific spells (+25% for each extra available head of type).

    SPELL_RAMPAGE_FIRE_CAST     = 139433,
    SPELL_RAMPAGE_FROST_CAST    = 139440,
    SPELL_RAMPAGE_POISON_CAST   = 139504,
    SPELL_RAMPAGE_ARCANE_CAST   = 139513,

    SPELL_RAMPAGE_FIRE          = 139548, // Flaming Head.
    SPELL_RAMPAGE_FROST         = 139549, // Frozen Head.
    SPELL_RAMPAGE_POISON        = 139551, // Venomous Head.
    SPELL_RAMPAGE_ARCANE        = 139552, // Arcane Head - Heroic only!.

    // Hydra Frenzy.
    SPELL_HYDRA_FRENZY          = 139942, // Attack speed increase (others when a head dies, this + heal to full).

    // Elemental Bonds.
    SPELL_ELEMENTAL_BONDS_FIRE  = 139586, // 15% dmg increase aura for each additional Flaming Head, on each.
    SPELL_ELEMENTAL_BONDS_FROST = 139587, // 15% dmg increase aura for each additional Frozen Head, on each.
    SPELL_ELEMENTAL_BONDS_VENOM = 139588, // 15% dmg increase aura for each additional Venomous Head, on each.
    SPELL_ELEMENTAL_BONDS_ARCAN = 139589, // 15% dmg increase aura for each additional Arcane Head, on each.

    // Megaera's Rage - When there's no melee target.
    SPELL_MEGAERAS_RAGE_FIRE    = 139758, // Flaming Head.
    SPELL_MEGAERAS_RAGE_FROST   = 139816, // Frozen Head.
    SPELL_MEGAERAS_RAGE_POISON  = 139818, // Venomous Head.
    SPELL_MEGAERAS_RAGE_ARCANE  = 139820, // Arcane Head - Heroic only!.

    SPELL_BERSERK               = 144369, // Berserk, Enrage, Bye - Bye or, simply put, a wipe. :)

    // Flaming Head
    SPELL_CINDERS               = 139822, // When in Concealing Fog. Melts Icy Ground. Pool NPC summoned when removed / every 3 seconds on Heroic.
    SPELL_CINDERS_SUMMON        = 139834, // Summons NPC_CINDERS.
    SPELL_CINDERS_AURA          = 139835, // Pool npc aura (per. dmg. trigger for 139836).

    SPELL_IGNITE_FLESH          = 137729, // Triggers 137330 dmg each sec and SPELL_IGNITE_FLESH_AURA on targets.
    SPELL_IGNITE_FLESH_AURA     = 137731, // Periodic damage aura on players hit by main spell.

    // Frozen Head
    // Torrent of Ice : NPC_TORRENT_OF_ICE summoned at player location, follows him having SPELL_TORRENT_OF_ICE_NPC_A. SPELL_TORRENT_OF_ICE cast on it.
    SPELL_TORRENT_OF_ICE        = 139866, // When in Concealing Fog. Each sec triggers SPELL_TORRENT_OF_ICE_SUMMON.
    SPELL_TORRENT_OF_ICE_SUMMON = 139870, // Summons NPC_ICY_GROUND.
    SPELL_TORRENT_OF_ICE_TARGET = 139857, // Target aura ("Torrent of Ice is targeting you!").
    SPELL_TORRENT_OF_ICE_NPC_A  = 139890, // NPC aura (60k periodic damage).

    SPELL_ICY_GROUND_AURA       = 139909, // Periodic damage and slow aura on NPC_ICY_GROUND.
    SPELL_ICY_GROUND_GROW       = 140213, // 1% scale increase (Heroic only!). 1 min -> double size. Not good as-is!
    SPELL_ICY_GROUND_VISUAL     = 139875, // Triggers SPELL_ICY_GROUND_DUMMY.
    SPELL_ICY_GROUND_DUMMY      = 139877, // Check for target and add stacks (SPELL_ICY_GROUND_AURA).

    SPELL_ARCTIC_FREEZE         = 139841, // Triggers damage and SPELL_ARCTIC_FREEZE_DUMMY.
    SPELL_ARCTIC_FREEZE_DUMMY   = 139843, // Stack checker and adder - 5 stacks = SPELL_ARCTIC_FREEZE_STUN on player.
    SPELL_ARCTIC_FREEZE_STUN    = 139844, // 20 sec stun.

    // Venomous Head
    SPELL_ACID_RAIN             = 134378, // When in Concealing Fog. Each sec triggers SPELL_ACID_GLOB_DUMMY.
    SPELL_ACID_GLOB_DUMMY       = 134343, // Dummy on eff 0 for SPELL_ACID_GLOB_MISSILE.
    SPELL_ACID_GLOB_MISSILE     = 134344, // Triggers 1 (Heroic: 3) Acid Splash missiles.

    SPELL_ROT_ARMOR             = 139838, // Triggers 139839 per. dmg. + 139840 damage taken debuff. 

    // Arcane Head - Heroic only!
    SPELL_NETHER_TEAR           = 140138,

    SPELL_DIFFUSION             = 139991, // Triggers per. dmg. and SPELL_DIFFUSION_PER_DUMMY.
    SPELL_DIFFUSION_PER_DUMMY   = 139993, // Periodic dummy for 10% health redirect.

    // Nether Wyrm
    SPELL_NETHER_SPIKE          = 140178,
    SPELL_SUPPRESSION           = 140179
};

enum Npcs
{
    // Boss
    NPC_FLAMING_HEAD            = 70212,
    NPC_FROZEN_HEAD             = 70235,
    NPC_VENOMOUS_HEAD           = 70247,
    NPC_ARCANE_HEAD             = 70252, // After flaming head dies - Heroic only!

    // Flaming Head
    NPC_CINDERS                 = 70432,

    // Frozen Head
    NPC_TORRENT_OF_ICE          = 70439,
    NPC_ICY_GROUND              = 70446,

    // Arcane Head - Heroic only!
    NPC_NETHER_WYRM             = 70507  // From Nether Tear.
};

enum GOs
{
    GO_CACHE_OF_ANCIENT_TREAS   = 218805 // Loot chest.
};

enum Events
{
    // Boss
    EVENT_RAMPAGE               = 1,
    EVENT_MEGAERAS_RAGE,

    EVENT_SUBMERGE,
    EVENT_EMERGE,
    EVENT_BERSERK,

    // Heads - General
    EVENT_CHECK_MEGAERAS_RAGE,

    // Flaming Head
    EVENT_CINDERS,
    EVENT_IGNITE_FLESH,

    // Frozen Head
    EVENT_TORRENT_OF_ICE,
    EVENT_ARCTIC_FREEZE,

    // Venomous Head
    EVENT_ACID_RAIN,
    EVENT_ROT_ARMOR,

    // Arcane Head - Heroic only!
    EVENT_NETHER_TEAR,
    EVENT_DIFFUSION,

    // Icy Ground
    EVENT_CHECK_ICY_GROUND_AND_CINDERS,
    EVENT_ICY_GROUND_GROW,

    // Nether Wyrm - From Arcane Head - Heroic only! 
    EVENT_NETHER_SPIKE,
    EVENT_SUPPRESSION
};

enum Timers
{
    // Boss
    TIMER_BERSERK_H             = 600000, // 10 minutes (Heroic).
    TIMER_BERSERK               = 780000  // 13 minutes.
};

enum Actions
{
    ACTION_SET_IN_COMBAT         = 1,
    ACTION_MEGAERAS_RAGE
};

// Front Head positions.
Position const headPositions[2] = 
{
    { 6414.79f, 4511.80f, -210.245f }, // Right.
    { 6430.56f, 4538.10f, -210.234f }, // Left.
};

// Concealing Fog Head positions.
Position const concealingPositions[7] =
{
    { 6457.16f, 4489.78f, -210.032f }, // Middle.

    { 6447.39f, 4481.54f, -210.032f }, // Right.
    { 6436.35f, 4473.30f, -210.032f },
    { 6410.10f, 4471.53f, -210.032f },

    { 6468.27f, 4503.42f, -210.032f }, // Left.
    { 6472.02f, 4524.07f, -210.032f },
    { 6465.45f, 4545.15f, -210.032f },
};

// Front Head orientations.
float headOrientations[2] =
{
    2.29f,                             // Right.
    2.70f                              // Left.
};

// Concealing Fog Head orientations.
float concealingOrientations[7] =
{
    2.46f,                             // Middle.

    2.38f,                             // Right.
    1.79f,
    1.71f,

    2.58f,                             // Left.
    2.94f,
    3.13f
};

// Megaera spawn / original body position.
Position const bossSpawnPos = { 6442.61f, 4599.18f, -210.032f };

// Megaera 68065.
class boss_megaera : public CreatureScript
{
    public:
        boss_megaera() : CreatureScript("boss_megaera") { }

        struct boss_megaeraAI : public BossAI
        {
            boss_megaeraAI(Creature* creature) : BossAI(creature, DATA_MEGAERA_EVENT), summons(me)
            {
                instance  = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            SummonList summons;
            EventMap events;
            uint8 headKills, rampageCount;
            bool isRaging, isRampaging;

            /*** SPECIFIC AI FUNCTIONS ***/

            // Used to add Hydra Frenzy to all heads of a type and heal them.
            void AddHydraFrenzy(uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                {
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                    {
                        if ((*summs)->isAlive())
                        {
                            if (!(*summs)->HasAura(SPELL_CONCEALING_FOG))
                            {
                                if (!(*summs)->HasAura(SPELL_HYDRA_FRENZY))
                                    (*summs)->AddAura(SPELL_HYDRA_FRENZY, *summs);
                                else
                                {
                                    if (AuraPtr frenzy = (*summs)->GetAura(SPELL_HYDRA_FRENZY))
                                        frenzy->SetStackAmount(frenzy->GetStackAmount() + 1);
                                }
                                (*summs)->SetFullHealth();
                            }
                        }
                    }
                }
            }

            // Used to retrieve number of surplus heads of a certain type (for use in Elemental Bonds aura stacks addition and Rampage damage calculations).
            int32 GetExtraHeadsCount(uint32 entry)
            {
                int32 count = 0;

                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);

                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        if ((*summs)->isAlive())
                            count++;

                // Check if we have a single head of that type, no counting needed.
                if (count <= 1)
                    return 0; 

                // First head is excluded from the count.
                count -= 1;

                return count;
            }

            // Used to add Elemental Bonds to all extra heads of a type.
            void AddElementalBonds(uint32 entry)
            {
                int32 headCount = GetExtraHeadsCount(entry);

                // No extra heads, no aura addition needed.
                if (headCount == 0)
                    return;

                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);

                // Now add the aura properly.
                if (!summonsList.empty())
                {
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                    {
                        if ((*summs)->isAlive())
                        {
                            if (entry == NPC_FLAMING_HEAD)
                            {
                                if (!(*summs)->HasAura(SPELL_ELEMENTAL_BONDS_FIRE))
                                {
                                    if (AuraPtr elemFire = (*summs)->AddAura(SPELL_ELEMENTAL_BONDS_FIRE, *summs))
                                        elemFire->SetStackAmount(headCount);
                                }
                                else
                                {
                                    if (AuraPtr elemFire = (*summs)->GetAura(SPELL_ELEMENTAL_BONDS_FIRE))
                                        elemFire->SetStackAmount(headCount);
                                }
                            }
                            else if (entry == NPC_FROZEN_HEAD)
                            {
                                if (!(*summs)->HasAura(SPELL_ELEMENTAL_BONDS_FROST))
                                {
                                    if (AuraPtr elemFrost = (*summs)->AddAura(SPELL_ELEMENTAL_BONDS_FROST, *summs))
                                        elemFrost->SetStackAmount(headCount);
                                }
                                else
                                {
                                    if (AuraPtr elemFrost = (*summs)->GetAura(SPELL_ELEMENTAL_BONDS_FROST))
                                        elemFrost->SetStackAmount(headCount);
                                }
                            }
                            else if (entry == NPC_VENOMOUS_HEAD)
                            {
                                if (!(*summs)->HasAura(SPELL_ELEMENTAL_BONDS_VENOM))
                                {
                                    if (AuraPtr elemVenom = (*summs)->AddAura(SPELL_ELEMENTAL_BONDS_VENOM, *summs))
                                        elemVenom->SetStackAmount(headCount);
                                }
                                else
                                {
                                    if (AuraPtr elemVenom = (*summs)->GetAura(SPELL_ELEMENTAL_BONDS_VENOM))
                                        elemVenom->SetStackAmount(headCount);
                                }
                            }
                            else if (entry == NPC_ARCANE_HEAD)
                            {
                                if (!(*summs)->HasAura(SPELL_ELEMENTAL_BONDS_ARCAN))
                                {
                                    if (AuraPtr elemArcane = (*summs)->AddAura(SPELL_ELEMENTAL_BONDS_ARCAN, *summs))
                                        elemArcane->SetStackAmount(headCount);
                                }
                                else
                                {
                                    if (AuraPtr elemArcane = (*summs)->GetAura(SPELL_ELEMENTAL_BONDS_ARCAN))
                                        elemArcane->SetStackAmount(headCount);
                                }
                            }
                            else return; // Error! :))
                        }
                    }
                }
            }

            // Selection of the three starting heads (from those available according to difficulty).
            void SpawnStartingHeads()
            {
                uint32 concealingHead = 0;
                uint32 firstHead      = 0;
                uint32 secondHead     = 0;

                // Select the Concealing Fog head.
                concealingHead = (me->GetMap()->IsHeroic() ? NPC_ARCANE_HEAD : NPC_FLAMING_HEAD);

                // The Concealing Fog head is already selected, select the first front one based on what's left.
                firstHead = NPC_FROZEN_HEAD;

                // The Concealing Fog head and the first front one were already selected, select the third one based on what's left.
                secondHead = NPC_VENOMOUS_HEAD;

                // Now do the checks and spawn them.
                if (!concealingHead || !firstHead || !secondHead)
                {
                    // Throw some error or shit.
                    sLog->outError(LOG_FILTER_TSCR, "Cannot complete SpawnStartingHeads() function on Megaera encounter (TOT) - Head spawns are bad / missing!");
                    return;
                }

                // Summon the front heads.
                if (Creature* fiHead  = me->SummonCreature(firstHead, headPositions[0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000))
                {
                    fiHead->SetFacingTo(headOrientations[0]);
                    fiHead->AddAura(SPELL_WATER_WALK, fiHead);
                    fiHead->SetReactState(REACT_DEFENSIVE);
			    }

                if (Creature* secHead = me->SummonCreature(secondHead, headPositions[1], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000))
                {
                    secHead->SetFacingTo(headOrientations[1]);
                    secHead->AddAura(SPELL_WATER_WALK, secHead);
                    secHead->SetReactState(REACT_DEFENSIVE);
			    }

                // Summon the Concealing Fog head and add the aura to it.
                if (Creature* concealing = me->SummonCreature(concealingHead, concealingPositions[0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000))
                {
                    concealing->SetFacingTo(concealingOrientations[0]);
                    me->AddAura(SPELL_CONCEALING_FOG, concealing);
                    concealing->AddAura(SPELL_WATER_WALK, concealing);
                    concealing->SetReactState(REACT_DEFENSIVE);
                }
            }

            // Selection of the next two heads when one dies (from those available according to difficulty).
            void SpawnNextHeads(uint32 deadHead, uint8 frontHeadPosition)
            {
                // Just a sanity check.
                if (headKills == 0 || headKills == 7)
                    return;

                uint32 firstNewHead  = 0;
                uint32 secondNewHead = 0;

                // Select first head to spawn (different from dead one and the others - always having heads of all types).
                switch (headKills)
                {
                    case 1:
                        firstNewHead = NPC_FLAMING_HEAD;
                        break;

                    case 2:
                        firstNewHead = (me->GetMap()->IsHeroic() ? NPC_ARCANE_HEAD : NPC_FROZEN_HEAD);
                        break;

                    case 3:
                        firstNewHead = (me->GetMap()->IsHeroic() ? NPC_FROZEN_HEAD : NPC_VENOMOUS_HEAD);
                        break;

                    case 4:
                        firstNewHead = (me->GetMap()->IsHeroic() ? NPC_VENOMOUS_HEAD : NPC_FLAMING_HEAD);
                        break;

                    case 5:
                        firstNewHead = (me->GetMap()->IsHeroic() ? NPC_FLAMING_HEAD : NPC_FROZEN_HEAD);
                        break;

                    case 6:
                        firstNewHead = (me->GetMap()->IsHeroic() ? NPC_ARCANE_HEAD : NPC_VENOMOUS_HEAD);
                        break;

                    default: break;
                }

                // Second head to spawn is same as the dead one, but summoned in the fog.
                secondNewHead = deadHead;

                // Now do the checks and spawn them.
                if (!firstNewHead || !secondNewHead)
                {
                    // Throw some error or shit.
                    sLog->outError(LOG_FILTER_TSCR, "Cannot complete SpawnNextHeads() function on Megaera encounter (TOT) - Head spawns are bad / missing!");
                    return;
                }

                // Summon front head and add the Encounter frame for it.
                if (Creature* fiNewHead   = me->SummonCreature(firstNewHead, headPositions[frontHeadPosition], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000))
                {
                    fiNewHead->SetFacingTo(headOrientations[frontHeadPosition]);
                    if (instance) instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, fiNewHead);
                    fiNewHead->AddAura(SPELL_WATER_WALK, fiNewHead);
                    me->AddAura(SPELL_SUBMERGE, fiNewHead);
                }

                // Summon Concealing Fog head + add the aura to it.
                if (Creature* secNewHead  = me->SummonCreature(secondNewHead, concealingPositions[headKills], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000))
                {
                    secNewHead->SetFacingTo(concealingOrientations[headKills]);
                    me->AddAura(SPELL_CONCEALING_FOG, secNewHead);
                    secNewHead->AddAura(SPELL_WATER_WALK, secNewHead);
                    me->AddAura(SPELL_SUBMERGE, secNewHead);
                }

                // Add Elemental Bonds to the heads.
                AddElementalBonds(firstNewHead);
                AddElementalBonds(secondNewHead);
            }

            // Used to remove Encounter frames from all the heads.
            void RemoveSummonFrame(uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty() && instance)
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, (*summs)); // Remove
            }

            // Used to add auras to all heads.
            void AddSummonAura(uint32 spellId, uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        if ((*summs)->isAlive())
                            (*summs)->AddAura(spellId, *summs);
            }

            // Used to remove auras from all heads.
            void RemoveSummonAura(uint32 spellId, uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        if ((*summs)->isAlive())
                            (*summs)->RemoveAurasDueToSpell(spellId);
            }

            // Used to make all same-type heads cast certain spells.
            void HeadsCastSpell(uint32 spellId, uint32 entry, bool randomTarget = false, bool triggered = false)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                {
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                    {
                        if ((*summs)->isAlive())
                        {
                            if (randomTarget)
                            {
                                if (Unit* target = (*summs)->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                                    (*summs)->CastSpell(target, spellId, triggered);
                            }
                            else
                                (*summs)->CastSpell(*summs, spellId, triggered);
                        }
                    }
                }
            }

            // Used to despawn all summons having a specific entry.
            void DespawnSummon(uint32 entry)
            {
                std::list<Creature*> summonsList;
                GetCreatureListWithEntryInGrid(summonsList, me, entry, 200.0f);
                if (!summonsList.empty())
                    for (std::list<Creature*>::iterator summs = summonsList.begin(); summs != summonsList.end(); summs++)
                        if ((*summs)->isAlive())
                            (*summs)->DespawnOrUnsummon();
            }

            /*** GENERAL AI FUNCTIONS ***/

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                // Set Head kills count (at 7 encounter ends), rampage damage count to 0 (20 max) and Megaera's Rage scheduled to false (check used in head scripts for melee range).
                headKills = 0;
                rampageCount = 0;
                isRaging = false;
                isRampaging = false;

                // Main boss body doesn't move.
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                me->AddAura(SPELL_CONCEALING_FOG, me);
                me->SetReactState(REACT_DEFENSIVE);

                if (instance)
                    instance->SetData(DATA_MEGAERA_EVENT, NOT_STARTED);

                _Reset();

                // Select and spawn the 3 heads.
                SpawnStartingHeads();
            }

            void EnterCombat(Unit* who)
            {
                // Just Berserk scheduled here, the other events are handled by the specific heads / through happenings (ex. a head dies -> Rampage, etc).
				events.ScheduleEvent(EVENT_BERSERK, (me->GetMap()->IsHeroic() ? TIMER_BERSERK_H : TIMER_BERSERK));

                // Put all heads in combat and send the frames for the front ones.
                if (instance)
                {
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                    if (Creature* firstHead = me->FindNearestCreature(NPC_FROZEN_HEAD, 100.0f, true))
                    {
                        firstHead->SetReactState(REACT_AGGRESSIVE);
                        firstHead->AI()->DoZoneInCombat(firstHead, 150.0f);
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, firstHead);
                    }
                    if (Creature* secondHead = me->FindNearestCreature(NPC_VENOMOUS_HEAD, 100.0f, true))
                    {
                        secondHead->SetReactState(REACT_AGGRESSIVE);
                        secondHead->AI()->DoZoneInCombat(secondHead, 150.0f);
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, secondHead);
                    }
                    if (Creature* concealingHead = me->FindNearestCreature((me->GetMap()->IsHeroic() ? NPC_ARCANE_HEAD : NPC_FLAMING_HEAD), 100.0f, true))
                    {
                        concealingHead->SetReactState(REACT_AGGRESSIVE);
                        concealingHead->AI()->DoZoneInCombat(concealingHead, 150.0f);
                    }

                    instance->SetData(DATA_MEGAERA_EVENT, IN_PROGRESS);
                }

                _EnterCombat();
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_SET_IN_COMBAT:
                        me->SetReactState(REACT_AGGRESSIVE);
                        DoZoneInCombat(me, 150.0f);
                        break;

                    case ACTION_MEGAERAS_RAGE:
                        events.ScheduleEvent(EVENT_MEGAERAS_RAGE, 3000);
                        isRaging = true;
                        break;

                    default: break;
                }
            };

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (!instance)
                    return;

                if (instance->GetData(DATA_MEGAERA_EVENT) == DONE)
                    return;

                // Handle Emerge / Submerge mechanics.
                if (spell->Id == SPELL_SUBMERGE)
                {
                    AddSummonAura(SPELL_EMERGE, NPC_FLAMING_HEAD);
                    AddSummonAura(SPELL_EMERGE, NPC_FROZEN_HEAD);
                    AddSummonAura(SPELL_EMERGE, NPC_VENOMOUS_HEAD);
                    if (me->GetMap()->IsHeroic())
                        AddSummonAura(SPELL_EMERGE, NPC_ARCANE_HEAD);

                    RemoveSummonAura(SPELL_SUBMERGE, NPC_FLAMING_HEAD);
                    RemoveSummonAura(SPELL_SUBMERGE, NPC_FROZEN_HEAD);
                    RemoveSummonAura(SPELL_SUBMERGE, NPC_VENOMOUS_HEAD);
                    if (me->GetMap()->IsHeroic())
                        RemoveSummonAura(SPELL_SUBMERGE, NPC_ARCANE_HEAD);
                }
            }

			void EnterEvadeMode()
            {
                DespawnSummon(NPC_CINDERS);
                DespawnSummon(NPC_TORRENT_OF_ICE);
                DespawnSummon(NPC_ICY_GROUND);
                DespawnSummon(NPC_NETHER_WYRM);

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->AddUnitState(UNIT_STATE_EVADE);

                RemoveSummonFrame(NPC_FLAMING_HEAD);
                RemoveSummonFrame(NPC_FROZEN_HEAD);
                RemoveSummonFrame(NPC_VENOMOUS_HEAD);
                if (me->GetMap()->IsHeroic())
                    RemoveSummonFrame(NPC_ARCANE_HEAD);

                me->RemoveAllAuras();
                me->DeleteThreatList();
                me->CombatStop(true);
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->MoveTargetedHome();

                if (instance)
                {
                    instance->SetData(DATA_MEGAERA_EVENT, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove
                }

                _EnterEvadeMode();
            }

            void JustReachedHome()
            {
                me->ClearUnitState(UNIT_STATE_EVADE);

                // This should take care of everything needed.
                Reset();

                _JustReachedHome();
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
				summon->setActive(true);

                // The heads cannot move.
                if (summon->GetEntry() == NPC_FLAMING_HEAD || summon->GetEntry() == NPC_FROZEN_HEAD || summon->GetEntry() == NPC_VENOMOUS_HEAD || summon->GetEntry() == NPC_ARCANE_HEAD)
                    summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                // Take care of putting the newly - spawned heads to fight while in combat.
				if (me->isInCombat())
					summon->SetInCombatWithZone();
            }

            void SummonedCreatureDies(Creature* summon, Unit* killer)
            {
                uint8 newFrontHeadSpawnPos = 0;

                // Just a sanity check. If headKills = 6 -> gets increased to 7 -> Megaera dead.
                if (headKills < 6 && (summon->GetEntry() == NPC_FLAMING_HEAD || summon->GetEntry() == NPC_FROZEN_HEAD || 
                    summon->GetEntry() == NPC_VENOMOUS_HEAD || me->GetMap()->IsHeroic() && summon->GetEntry() == NPC_ARCANE_HEAD))
                {
                    if (instance)
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, summon); // Remove

                    // Submerge (Reappear 4 seconds later with all heads).
                    HeadsCastSpell(SPELL_SUBMERGE, NPC_FLAMING_HEAD);
                    HeadsCastSpell(SPELL_SUBMERGE, NPC_FROZEN_HEAD);
                    HeadsCastSpell(SPELL_SUBMERGE, NPC_VENOMOUS_HEAD);
                    if (me->GetMap()->IsHeroic())
                        HeadsCastSpell(SPELL_SUBMERGE, NPC_ARCANE_HEAD);

                    events.ScheduleEvent(EVENT_SUBMERGE, 100);
                    events.ScheduleEvent(EVENT_EMERGE, 3100);

                    // Increase killed heads count.
                    headKills++;

                    // Remove 15% health each time a head is killed.
                    me->SetHealth(me->GetMaxHealth() - ((0.15 * me->GetMaxHealth()) * headKills));

                    // Check the head positions and find the right position on which to summon the new ones.
                    uint8 summonPos = (summon->GetPositionX() == headPositions[0].GetPositionX() ? 0 : 1);
                    uint32 summonEntry = summon->GetEntry();

                    // Now we can safely despawn the head.
                    summons.Despawn(summon);

                    // And spawn the next ones.
                    SpawnNextHeads(summonEntry, summonPos);

                    // Hydra Frenzy - set remaining heads + 1 stacks + heal them.
                    AddHydraFrenzy(NPC_FLAMING_HEAD);
                    AddHydraFrenzy(NPC_FROZEN_HEAD);
                    AddHydraFrenzy(NPC_VENOMOUS_HEAD);
                    if (me->GetMap()->IsHeroic())
                        AddHydraFrenzy(NPC_ARCANE_HEAD);

                    // A head died - schedule Rampage (4 to emerge + 1).
                    events.ScheduleEvent(EVENT_RAMPAGE, 5000);
                    isRampaging = true;
                }
                else
                {
                    JustDied(killer);
                    me->DespawnOrUnsummon(1000);
                }
            }

            void JustDied(Unit* killer)
            {
                DespawnSummon(NPC_CINDERS);
                DespawnSummon(NPC_TORRENT_OF_ICE);
                DespawnSummon(NPC_ICY_GROUND);
                DespawnSummon(NPC_NETHER_WYRM);

                RemoveSummonFrame(NPC_FLAMING_HEAD);
                RemoveSummonFrame(NPC_FROZEN_HEAD);
                RemoveSummonFrame(NPC_VENOMOUS_HEAD);
                if (me->GetMap()->IsHeroic())
                    RemoveSummonFrame(NPC_ARCANE_HEAD);

                summons.DespawnAll();

                // Spawn the chest.
                killer->SummonGameObject(GO_CACHE_OF_ANCIENT_TREAS, 6418.52f, 4524.48f, -209.176f, 2.42f, 0.0f, 0.0f, 0.0f, 0.0f, RESPAWN_IMMEDIATELY, 0);

                if (instance)
                {
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    instance->SetData(DATA_MEGAERA_EVENT, DONE);
                }

                _JustDied();
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || !CheckInRoom() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (instance && instance->IsWipe())
                {
                    EnterEvadeMode();
                    return;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUBMERGE:
                            DoCast(me, SPELL_SUBMERGE);
                            break;

                        case EVENT_EMERGE:
                            me->RemoveAurasDueToSpell(SPELL_EMERGE);
                            RemoveSummonAura(SPELL_EMERGE, NPC_FLAMING_HEAD);
                            RemoveSummonAura(SPELL_EMERGE, NPC_FROZEN_HEAD);
                            RemoveSummonAura(SPELL_EMERGE, NPC_VENOMOUS_HEAD);
                            if (me->GetMap()->IsHeroic())
                                RemoveSummonAura(SPELL_EMERGE, NPC_ARCANE_HEAD);
                            break;

                        case EVENT_RAMPAGE:
                            if (rampageCount < 20)
                            {
                                // Check and add the auras.
                                if (!me->HasAura(SPELL_RAMPAGE))
                                {
                                    Talk(ANN_RAMPAGE);

                                    me->AddAura(SPELL_RAMPAGE, me);

                                    AddSummonAura(SPELL_RAMPAGE, NPC_FLAMING_HEAD);
                                    AddSummonAura(SPELL_RAMPAGE, NPC_FROZEN_HEAD);
                                    AddSummonAura(SPELL_RAMPAGE, NPC_VENOMOUS_HEAD);
                                    if (me->GetMap()->IsHeroic())
                                        AddSummonAura(SPELL_RAMPAGE, NPC_ARCANE_HEAD);
                                }

                                // Have each of the specific heads cast the spell (in order).
                                if (!me->GetMap()->IsHeroic())
                                {
                                    if (rampageCount == 0 || rampageCount == 3 || rampageCount == 6 || rampageCount == 9 || rampageCount == 12 || rampageCount == 15 || rampageCount == 18)
                                        HeadsCastSpell(SPELL_RAMPAGE_FIRE_CAST, NPC_FLAMING_HEAD, true);
                                    if (rampageCount == 1 || rampageCount == 4 || rampageCount == 7 || rampageCount == 10 || rampageCount == 13 || rampageCount == 16 || rampageCount == 19)
                                        HeadsCastSpell(SPELL_RAMPAGE_FROST_CAST, NPC_FROZEN_HEAD, true);
                                    if (rampageCount == 2 || rampageCount == 5 || rampageCount == 8 || rampageCount == 11 || rampageCount == 14 || rampageCount == 17)
                                        HeadsCastSpell(SPELL_RAMPAGE_POISON_CAST, NPC_VENOMOUS_HEAD, true);
                                }
                                else // Heroic!
                                {
                                    if (rampageCount == 0 || rampageCount == 4 || rampageCount == 8 || rampageCount == 12 || rampageCount == 16)
                                        HeadsCastSpell(SPELL_RAMPAGE_FIRE_CAST, NPC_FLAMING_HEAD, true);
                                    if (rampageCount == 1 || rampageCount == 5 || rampageCount == 9 || rampageCount == 13 || rampageCount == 17)
                                        HeadsCastSpell(SPELL_RAMPAGE_FROST_CAST, NPC_FROZEN_HEAD, true);
                                    if (rampageCount == 2 || rampageCount == 6 || rampageCount == 10 || rampageCount == 14 || rampageCount == 18)
                                        HeadsCastSpell(SPELL_RAMPAGE_POISON_CAST, NPC_VENOMOUS_HEAD, true);
                                    if (rampageCount == 3 || rampageCount == 7 || rampageCount == 11 || rampageCount == 15 || rampageCount == 19)
                                        HeadsCastSpell(SPELL_RAMPAGE_ARCANE_CAST, NPC_ARCANE_HEAD, true);
                                }

                                // Increase the count and reschedule the event.
                                rampageCount++;
                                events.ScheduleEvent(EVENT_RAMPAGE, 1000);
                            }
                            else
                            {
                                // Check and remove the auras.
                                if (me->HasAura(SPELL_RAMPAGE))
                                {
                                    Talk(ANN_SUBSIDE);

                                    me->RemoveAurasDueToSpell(SPELL_RAMPAGE);

                                    RemoveSummonAura(SPELL_RAMPAGE, NPC_FLAMING_HEAD);
                                    RemoveSummonAura(SPELL_RAMPAGE, NPC_FROZEN_HEAD);
                                    RemoveSummonAura(SPELL_RAMPAGE, NPC_VENOMOUS_HEAD);
                                    if (me->GetMap()->IsHeroic())
                                        RemoveSummonAura(SPELL_RAMPAGE, NPC_ARCANE_HEAD);
                                }

                                rampageCount = 0;
                                isRampaging = false;
                            }
                            break;

                        case EVENT_MEGAERAS_RAGE:
                            if (isRaging)
                            {
                                // Have each of the specific heads cast the spell (in order).
                                HeadsCastSpell(SPELL_MEGAERAS_RAGE_FIRE, NPC_FLAMING_HEAD, true);
                                HeadsCastSpell(SPELL_MEGAERAS_RAGE_FROST, NPC_FROZEN_HEAD, true);
                                HeadsCastSpell(SPELL_MEGAERAS_RAGE_POISON, NPC_VENOMOUS_HEAD, true);
                                if (me->GetMap()->IsHeroic())
                                    HeadsCastSpell(SPELL_MEGAERAS_RAGE_ARCANE, NPC_ARCANE_HEAD, true);

                                isRaging = false;
                            }
                            break;

                        case EVENT_BERSERK:
				            DoCast(me, SPELL_BERSERK);
                            HeadsCastSpell(SPELL_BERSERK, NPC_FLAMING_HEAD);
                            HeadsCastSpell(SPELL_BERSERK, NPC_FROZEN_HEAD);
                            HeadsCastSpell(SPELL_BERSERK, NPC_VENOMOUS_HEAD);
                            if (me->GetMap()->IsHeroic())
                                HeadsCastSpell(SPELL_BERSERK, NPC_ARCANE_HEAD);
                            break;

                        default: break;
                    }
                }

                // No melee from body.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_megaeraAI(creature);
        }
};

// Flaming Head 70212.
class npc_flaming_head_megaera : public CreatureScript
{
    public:
        npc_flaming_head_megaera() : CreatureScript("npc_flaming_head_megaera") { }

        struct npc_flaming_head_megaeraAI : public ScriptedAI
        {
            npc_flaming_head_megaeraAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_CHECK_MEGAERAS_RAGE, 18000);
                events.ScheduleEvent(EVENT_CINDERS, urand(10000, 15000));
                events.ScheduleEvent(EVENT_IGNITE_FLESH, urand(12000, 17000));

				if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                    if (!Megaera->isInCombat())
                        Megaera->AI()->DoAction(ACTION_SET_IN_COMBAT);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHECK_MEGAERAS_RAGE:
                            // Must be one of the heads in front.
                            if (!me->HasAura(SPELL_CONCEALING_FOG))
                            {
                                // Find boss and check for melee distance to victim + unscheduled boss action.
				                if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                                    if (!me->IsWithinDistInMap(me->getVictim(), me->GetAttackDistance(me->getVictim())))
                                        if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRaging == false)
                                            Megaera->AI()->DoAction(ACTION_MEGAERAS_RAGE);

                                events.ScheduleEvent(EVENT_CHECK_MEGAERAS_RAGE, 4000);
                            }
                            break;

                        case EVENT_CINDERS:
				            if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                            {
                                if (me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                                {
                                    if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                                        DoCast(me, SPELL_CINDERS);
                                    events.ScheduleEvent(EVENT_CINDERS, (me->GetMap()->IsHeroic() ? urand(54000, 60000) : urand(23000, 29000)));
                                }
                            }
                            break;

                        case EVENT_IGNITE_FLESH:
				            if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                            {
                                if (!me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                                {
                                    if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                                        DoCast(me, SPELL_IGNITE_FLESH);
                                    events.ScheduleEvent(EVENT_IGNITE_FLESH, urand(18000, 23000));
                                }
                            }
                            break;

                        default: break;
                    }
                }

                // Can't melee from the fog.
				if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                    if (!me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                        if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                            DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_flaming_head_megaeraAI(creature);
        }
};

// Frozen Head 70235.
class npc_frozen_head_megaera : public CreatureScript
{
    public:
        npc_frozen_head_megaera() : CreatureScript("npc_frozen_head_megaera") { }

        struct npc_frozen_head_megaeraAI : public ScriptedAI
        {
            npc_frozen_head_megaeraAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_CHECK_MEGAERAS_RAGE, 18900);
                events.ScheduleEvent(EVENT_TORRENT_OF_ICE, urand(14000, 19000));
                events.ScheduleEvent(EVENT_ARCTIC_FREEZE, urand(7000, 12000));

				if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                    if (!Megaera->isInCombat())
                        Megaera->AI()->DoAction(ACTION_SET_IN_COMBAT);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHECK_MEGAERAS_RAGE:
                            // Must be one of the heads in front.
                            if (!me->HasAura(SPELL_CONCEALING_FOG))
                            {
                                // Find boss and check for melee distance to victim + unscheduled boss action.
				                if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                                    if (!me->IsWithinDistInMap(me->getVictim(), me->GetAttackDistance(me->getVictim())))
                                        if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRaging == false)
                                            Megaera->AI()->DoAction(ACTION_MEGAERAS_RAGE);

                                events.ScheduleEvent(EVENT_CHECK_MEGAERAS_RAGE, 4000);
                            }
                            break;

                        case EVENT_TORRENT_OF_ICE:
				            if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                            {
                                if (me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                                {
                                    if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                                    {
                                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                        {
                                            me->AddAura(SPELL_TORRENT_OF_ICE_TARGET, target);
                                            if (Creature* torrent = me->SummonCreature(NPC_TORRENT_OF_ICE, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 9500))
                                            {
                                                torrent->SetSpeed(MOVE_WALK, 0.8f);
                                                torrent->SetSpeed(MOVE_RUN, 0.7f);
                                                torrent->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                                                torrent->Attack(target, false);
                                                torrent->GetMotionMaster()->MoveChase(target, 2.0f);
                                                torrent->AddAura(SPELL_TORRENT_OF_ICE_NPC_A, torrent);
                                                torrent->SetReactState(REACT_PASSIVE);
                                                DoCast(torrent, SPELL_TORRENT_OF_ICE);
                                            }
                                        }
                                    }
                                    events.ScheduleEvent(EVENT_TORRENT_OF_ICE, urand(26000, 30000));
                                }
                            }
                            break;

                        case EVENT_ARCTIC_FREEZE:
				            if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                            {
                                if (!me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                                {
                                    if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                                        DoCast(me, SPELL_ARCTIC_FREEZE);
                                    events.ScheduleEvent(EVENT_ARCTIC_FREEZE, urand(18000, 23000));
                                }
                            }
                            break;

                        default: break;
                    }
                }

                // Can't melee from the fog.
				if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                    if (!me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                        if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                            DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_frozen_head_megaeraAI(creature);
        }
};

// Venomous Head 70247.
class npc_venomous_head_megaera : public CreatureScript
{
    public:
        npc_venomous_head_megaera() : CreatureScript("npc_venomous_head_megaera") { }

        struct npc_venomous_head_megaeraAI : public ScriptedAI
        {
            npc_venomous_head_megaeraAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_CHECK_MEGAERAS_RAGE, 19800);
                events.ScheduleEvent(EVENT_ACID_RAIN, urand(12000, 17000));
                events.ScheduleEvent(EVENT_ROT_ARMOR, urand(10000, 15000));

				if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                    if (!Megaera->isInCombat())
                        Megaera->AI()->DoAction(ACTION_SET_IN_COMBAT);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHECK_MEGAERAS_RAGE:
                            // Must be one of the heads in front.
                            if (!me->HasAura(SPELL_CONCEALING_FOG))
                            {
                                // Find boss and check for melee distance to victim + unscheduled boss action.
				                if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                                    if (!me->IsWithinDistInMap(me->getVictim(), me->GetAttackDistance(me->getVictim())))
                                        if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRaging == false)
                                            Megaera->AI()->DoAction(ACTION_MEGAERAS_RAGE);

                                events.ScheduleEvent(EVENT_CHECK_MEGAERAS_RAGE, 4000);
                            }
                            break;

                        case EVENT_ACID_RAIN:
				            if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                            {
                                if (me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                                {
                                    if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                                        DoCast(me, SPELL_ACID_RAIN);
                                    events.ScheduleEvent(EVENT_ACID_RAIN, (me->GetMap()->IsHeroic() ? urand(54000, 60000) : urand(23000, 29000)));
                                }
                            }
                            break;

                        case EVENT_ROT_ARMOR:
				            if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                            {
                                if (!me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                                {
                                    if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                                        DoCast(me, SPELL_ROT_ARMOR);
                                    events.ScheduleEvent(EVENT_ROT_ARMOR, urand(18000, 23000));
                                }
                            }
                            break;

                        default: break;
                    }
                }

                // Can't melee from the fog.
				if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                    if (!me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                        if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                            DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_venomous_head_megaeraAI(creature);
        }
};

// Arcane Head 70252 - Heroic only!
class npc_arcane_head_megaera : public CreatureScript
{
    public:
        npc_arcane_head_megaera() : CreatureScript("npc_arcane_head_megaera") { }

        struct npc_arcane_head_megaeraAI : public ScriptedAI
        {
            npc_arcane_head_megaeraAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_CHECK_MEGAERAS_RAGE, 20600);
                events.ScheduleEvent(EVENT_NETHER_TEAR, urand(12000, 17000));
                events.ScheduleEvent(EVENT_SUPPRESSION, urand(7000, 12000));

				if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                    if (!Megaera->isInCombat())
                        Megaera->AI()->DoAction(ACTION_SET_IN_COMBAT);
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHECK_MEGAERAS_RAGE:
                            // Must be one of the heads in front.
                            if (!me->HasAura(SPELL_CONCEALING_FOG))
                            {
                                // Find boss and check for melee distance to victim + unscheduled boss action.
				                if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                                    if (!me->IsWithinDistInMap(me->getVictim(), me->GetAttackDistance(me->getVictim())))
                                        if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRaging == false)
                                            Megaera->AI()->DoAction(ACTION_MEGAERAS_RAGE);

                                events.ScheduleEvent(EVENT_CHECK_MEGAERAS_RAGE, 4000);
                            }
                            break;

                        case EVENT_NETHER_TEAR:
				            if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                            {
                                if (me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                                {
                                    if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                                        DoCast(me, SPELL_NETHER_TEAR);
                                    events.ScheduleEvent(EVENT_NETHER_TEAR, urand(22000, 27000));
                                }
                            }
                            break;

                        case EVENT_DIFFUSION:
				            if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                            {
                                if (!me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                                {
                                    if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                                        DoCast(me, SPELL_DIFFUSION);
                                    events.ScheduleEvent(EVENT_DIFFUSION, urand(18000, 23000));
                                }
                            }
                            break;

                        default: break;
                    }
                }

                // Can't melee from the fog.
				if (Creature* Megaera = me->FindNearestCreature(BOSS_MEGAERA, 200.0f, true))
                    if (!me->HasAura(SPELL_CONCEALING_FOG) && !me->HasAura(SPELL_EMERGE))
                        if (CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->isRampaging == false)
                            DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_arcane_head_megaeraAI(creature);
        }
};

// Cinders 70432.
class npc_cinders_megaera : public CreatureScript
{
    public:
        npc_cinders_megaera() : CreatureScript("npc_cinders_megaera") { }

        struct npc_cinders_megaeraAI : public ScriptedAI
        {
            npc_cinders_megaeraAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            bool despawned;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->AddAura(SPELL_CINDERS_AURA, me);
                Reset();
            }

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetInCombatWithZone();
                me->SetReactState(REACT_PASSIVE);
                // me->DespawnOrUnsummon(60000); - Summon spell already has 1 minute duration.

                despawned = false;
            }

            void UpdateAI(uint32 const diff)
            {
                if (me->FindNearestCreature(NPC_TORRENT_OF_ICE, 5.0f, true) && !despawned)
                {
                    me->DespawnOrUnsummon(200);
                    despawned = true;
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_cinders_megaeraAI(creature);
        }
};

// Icy Ground 70446.
class npc_icy_ground_megaera : public CreatureScript
{
    public:
        npc_icy_ground_megaera() : CreatureScript("npc_icy_ground_megaera") { }

        struct npc_icy_ground_megaeraAI : public ScriptedAI
        {
            npc_icy_ground_megaeraAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;
            uint8 growTicks;
            bool despawned;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
                me->AddAura(SPELL_ICY_GROUND_VISUAL, me);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                me->SetInCombatWithZone();
                me->SetReactState(REACT_PASSIVE);

                events.ScheduleEvent(EVENT_CHECK_ICY_GROUND_AND_CINDERS, 300);
                if (me->GetMap()->IsHeroic())
                    events.ScheduleEvent(EVENT_ICY_GROUND_GROW, 1000);
            }

            void Reset()
            {
                events.Reset();

                growTicks = 0;
                despawned = false;
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHECK_ICY_GROUND_AND_CINDERS:
                        {
                            Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();
                            if (!PlayerList.isEmpty())
                            {
                                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                {
                                    if (Player* player = i->getSource())
                                    {
                                        if (me->GetDistance(player) <= 5.0f)
                                        {
                                            // Handle Icy Ground addition.
                                            if (!player->HasAura(SPELL_ICY_GROUND_AURA))
                                                me->AddAura(SPELL_ICY_GROUND_AURA, player);

                                            // Handle Cinders touching.
                                            if (player->HasAura(SPELL_CINDERS) && !despawned)
                                            {
                                                if (me->GetMap()->IsHeroic())
                                                    events.CancelEvent(EVENT_ICY_GROUND_GROW);

                                                me->DespawnOrUnsummon(200);
                                                despawned = true;
                                            }
                                        }
                                        else
                                        {
                                            // Handle Icy Ground removal.
                                            if (!player->FindNearestCreature(NPC_ICY_GROUND, 5.0f, true))
                                                if (player->HasAura(SPELL_ICY_GROUND_AURA))
                                                    player->RemoveAurasDueToSpell(SPELL_ICY_GROUND_AURA);
                                        }
                                    }
                                }
                            }

                            events.ScheduleEvent(EVENT_CHECK_ICY_GROUND_AND_CINDERS, 300);
                            break;
                        }

                        case EVENT_ICY_GROUND_GROW:
                            me->SetObjectScale(1.0f + (0.017f * growTicks));
                            events.ScheduleEvent(EVENT_ICY_GROUND_GROW, 1000);
                            break;

                        default: break;
                    }
                }

                // No melee.
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_icy_ground_megaeraAI(creature);
        }
};

// Nether Wyrm 70507 - From Arcane Head - Heroic only!
class npc_nether_wyrm_megaera : public CreatureScript
{
    public:
        npc_nether_wyrm_megaera() : CreatureScript("npc_nether_wyrm_megaera") { }

        struct npc_nether_wyrm_megaeraAI : public ScriptedAI
        {
            npc_nether_wyrm_megaeraAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            EventMap events;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                Reset();
            }

            void Reset()
            {
                events.Reset();
                DoZoneInCombat(me, 150.0f);

                events.ScheduleEvent(EVENT_NETHER_SPIKE, urand(3000, 7000));
                events.ScheduleEvent(EVENT_SUPPRESSION, urand(9000, 14000));
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_NETHER_SPIKE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_NETHER_SPIKE);
                            events.ScheduleEvent(EVENT_NETHER_SPIKE, urand(7000, 11000));
                            break;

                        case EVENT_SUPPRESSION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_SUPPRESSION);
                            events.ScheduleEvent(EVENT_SUPPRESSION, urand(18000, 23000));
                            break;

                        default: break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_nether_wyrm_megaeraAI(creature);
        }
};

// Concealing Fog - 137973.
class spell_concealing_fog_megaera : public SpellScriptLoader
{
    public:
        spell_concealing_fog_megaera() : SpellScriptLoader("spell_concealing_fog_megaera") { }

        class spell_concealing_fog_megaera_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_concealing_fog_megaera_AuraScript);

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();
                if (!caster || !target)
                    return;

                target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();
                if (!caster || !target)
                    return;

                target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_concealing_fog_megaera_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_concealing_fog_megaera_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_concealing_fog_megaera_AuraScript();
        }
};

// Rampage - Flaming Head 139548 (damage).
class spell_rampage_flaming_head_megaera : public SpellScriptLoader
{
    public:
        spell_rampage_flaming_head_megaera() : SpellScriptLoader("spell_rampage_flaming_head_megaera") { }

        class spell_rampage_flaming_head_megaera_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rampage_flaming_head_megaera_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                if (caster->ToCreature()->GetEntry() != NPC_FLAMING_HEAD)
                    return;

                Creature* Megaera = caster->FindNearestCreature(BOSS_MEGAERA, 200.0f, true);
                if (!Megaera)
                    return;

                int32 extraHeads = CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->GetExtraHeadsCount(NPC_FLAMING_HEAD);
                SetHitDamage(int32(GetHitDamage() + ((GetHitDamage() / 4) * extraHeads))); // Increases by 25% for each extra head.
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rampage_flaming_head_megaera_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rampage_flaming_head_megaera_SpellScript();
        }
};

// Rampage - Frozen Head 139549 (damage).
class spell_rampage_frozen_head_megaera : public SpellScriptLoader
{
    public:
        spell_rampage_frozen_head_megaera() : SpellScriptLoader("spell_rampage_frozen_head_megaera") { }

        class spell_rampage_frozen_head_megaera_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rampage_frozen_head_megaera_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                if (caster->ToCreature()->GetEntry() != NPC_FROZEN_HEAD)
                    return;

                Creature* Megaera = caster->FindNearestCreature(BOSS_MEGAERA, 200.0f, true);
                if (!Megaera)
                    return;

                int32 extraHeads = CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->GetExtraHeadsCount(NPC_FROZEN_HEAD);
                SetHitDamage(int32(GetHitDamage() + ((GetHitDamage() / 4) * extraHeads))); // Increases by 25% for each extra head.
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rampage_frozen_head_megaera_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rampage_frozen_head_megaera_SpellScript();
        }
};

// Rampage - Venomous Head 139551 (damage).
class spell_rampage_venomous_head_megaera : public SpellScriptLoader
{
    public:
        spell_rampage_venomous_head_megaera() : SpellScriptLoader("spell_rampage_venomous_head_megaera") { }

        class spell_rampage_venomous_head_megaera_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rampage_venomous_head_megaera_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                if (caster->ToCreature()->GetEntry() != NPC_VENOMOUS_HEAD)
                    return;

                Creature* Megaera = caster->FindNearestCreature(BOSS_MEGAERA, 200.0f, true);
                if (!Megaera)
                    return;

                int32 extraHeads = CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->GetExtraHeadsCount(NPC_VENOMOUS_HEAD);
                SetHitDamage(int32(GetHitDamage() + ((GetHitDamage() / 4) * extraHeads))); // Increases by 25% for each extra head.
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rampage_venomous_head_megaera_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rampage_venomous_head_megaera_SpellScript();
        }
};

// Rampage - Arcane Head 139552 (damage) - Heroic only!
class spell_rampage_arcane_head_megaera : public SpellScriptLoader
{
    public:
        spell_rampage_arcane_head_megaera() : SpellScriptLoader("spell_rampage_arcane_head_megaera") { }

        class spell_rampage_arcane_head_megaera_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rampage_arcane_head_megaera_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                if (caster->ToCreature()->GetEntry() != NPC_ARCANE_HEAD)
                    return;

                Creature* Megaera = caster->FindNearestCreature(BOSS_MEGAERA, 200.0f, true);
                if (!Megaera)
                    return;

                int32 extraHeads = CAST_AI(boss_megaera::boss_megaeraAI, Megaera->AI())->GetExtraHeadsCount(NPC_ARCANE_HEAD);
                SetHitDamage(int32(GetHitDamage() + ((GetHitDamage() / 4) * extraHeads))); // Increases by 25% for each extra head.
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rampage_arcane_head_megaera_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rampage_arcane_head_megaera_SpellScript();
        }
};

// Cinders - 139822.
class spell_cinders_megaera : public SpellScriptLoader
{
    public:
        spell_cinders_megaera() : SpellScriptLoader("spell_cinders_megaera") { }

        class spell_cinders_megaera_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_cinders_megaera_AuraScript);

            void OnTick(constAuraEffectPtr aurEff)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                // NPC is spawned every 3 seconds (so 3 ticks, 1 ticks / sec) on Heroic diff.
                if (!caster->GetMap()->IsHeroic())
                    return;

                if (aurEff->GetTickNumber() % 3 == 0)
                    target->CastSpell(target, SPELL_CINDERS_SUMMON, true);
            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                // NPC is spawned on aura removal on Normal diff.
                if (caster->GetMap()->IsHeroic())
                    return;

                target->CastSpell(target, SPELL_CINDERS_SUMMON, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_cinders_megaera_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
                OnEffectRemove += AuraEffectRemoveFn(spell_cinders_megaera_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_cinders_megaera_AuraScript();
        }
};

// Arctic Freeze (stack checker) - 139843.
class spell_arctic_freeze_megaera : public SpellScriptLoader
{
    public:
        spell_arctic_freeze_megaera() : SpellScriptLoader("spell_arctic_freeze_megaera") { }

        class spell_arctic_freeze_megaera_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_arctic_freeze_megaera_AuraScript)

            void OnPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();

                if (!caster || !target)
                    return;

                // Check aura stacks for stun apply.
                if (AuraPtr arcticFreeze = target->GetAura(SPELL_ARCTIC_FREEZE_DUMMY))
                {
                    int32 stacks = arcticFreeze->GetStackAmount();

                    // Stack handling.
                    if (stacks >= 5 && !target->HasAura(SPELL_ARCTIC_FREEZE_STUN))
                    {
                        target->RemoveAurasDueToSpell(SPELL_ARCTIC_FREEZE_DUMMY);
                        caster->AddAura(SPELL_ARCTIC_FREEZE_STUN, target);
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_arctic_freeze_megaera_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_arctic_freeze_megaera_AuraScript();
        }
};

// Acid Glob (Acid Rain dummy) - 134343.
class spell_acid_glob_megaera : public SpellScriptLoader
{
    public:
        spell_acid_glob_megaera() : SpellScriptLoader("spell_acid_glob_megaera") { }

        class spell_acid_glob_megaera_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_acid_glob_megaera_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();

                if (!caster)
                    return;

                if (!caster->ToCreature())
                    return;

                // The aura is 3 seconds long, and this is triggered each sec. So if not heroic, we should remove the main aura after this first cast.
                if (Unit* target = caster->ToCreature()->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    caster->CastSpell(target, SPELL_ACID_GLOB_MISSILE, true);
                if (!caster->GetMap()->IsHeroic())
                    caster->RemoveAurasDueToSpell(SPELL_ACID_RAIN);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_acid_glob_megaera_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_acid_glob_megaera_SpellScript();
        }
};

void AddSC_boss_megaera()
{
    new boss_megaera();
    new npc_flaming_head_megaera();
    new npc_frozen_head_megaera();
    new npc_venomous_head_megaera();
    new npc_arcane_head_megaera();
    new npc_cinders_megaera();
    new npc_icy_ground_megaera();
    new npc_nether_wyrm_megaera();
    new spell_concealing_fog_megaera();
    new spell_rampage_flaming_head_megaera();
    new spell_rampage_frozen_head_megaera();
    new spell_rampage_venomous_head_megaera();
    new spell_rampage_arcane_head_megaera();
    new spell_cinders_megaera();
    new spell_arctic_freeze_megaera();
    new spell_acid_glob_megaera();
}
