/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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
 */

#include "Common.h"
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "SkillExtraItems.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "Pet.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Creature.h"
#include "Totem.h"
#include "CreatureAI.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "BattlegroundEY.h"
#include "BattlegroundWS.h"
#include "BattlegroundTP.h"
#include "OutdoorPvPMgr.h"
#include "Language.h"
#include "SocialMgr.h"
#include "Util.h"
#include "VMapFactory.h"
#include "TemporarySummon.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SkillDiscovery.h"
#include "Formulas.h"
#include "Vehicle.h"
#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "AccountMgr.h"
#include "InstanceScript.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Group.h"
#include "UpdateFieldFlags.h"

pEffect SpellEffects[TOTAL_SPELL_EFFECTS]=
{
    &Spell::EffectNULL,                                     //  0
    &Spell::EffectInstaKill,                                //  1 SPELL_EFFECT_INSTAKILL
    &Spell::EffectSchoolDMG,                                //  2 SPELL_EFFECT_SCHOOL_DAMAGE
    &Spell::EffectDummy,                                    //  3 SPELL_EFFECT_DUMMY
    &Spell::EffectUnused,                                   //  4 SPELL_EFFECT_PORTAL_TELEPORT          unused
    &Spell::EffectTeleportUnits,                            //  5 SPELL_EFFECT_TELEPORT_UNITS
    &Spell::EffectApplyAura,                                //  6 SPELL_EFFECT_APPLY_AURA
    &Spell::EffectEnvironmentalDMG,                         //  7 SPELL_EFFECT_ENVIRONMENTAL_DAMAGE
    &Spell::EffectPowerDrain,                               //  8 SPELL_EFFECT_POWER_DRAIN
    &Spell::EffectHealthLeech,                              //  9 SPELL_EFFECT_HEALTH_LEECH
    &Spell::EffectHeal,                                     // 10 SPELL_EFFECT_HEAL
    &Spell::EffectBind,                                     // 11 SPELL_EFFECT_BIND
    &Spell::EffectNULL,                                     // 12 SPELL_EFFECT_PORTAL
    &Spell::EffectUnused,                                   // 13 SPELL_EFFECT_RITUAL_BASE              unused
    &Spell::EffectUnused,                                   // 14 SPELL_EFFECT_RITUAL_SPECIALIZE        unused
    &Spell::EffectUnused,                                   // 15 SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL   unused
    &Spell::EffectQuestComplete,                            // 16 SPELL_EFFECT_QUEST_COMPLETE
    &Spell::EffectWeaponDmg,                                // 17 SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL
    &Spell::EffectResurrect,                                // 18 SPELL_EFFECT_RESURRECT
    &Spell::EffectAddExtraAttacks,                          // 19 SPELL_EFFECT_ADD_EXTRA_ATTACKS
    &Spell::EffectUnused,                                   // 20 SPELL_EFFECT_DODGE                    one spell: Dodge
    &Spell::EffectUnused,                                   // 21 SPELL_EFFECT_EVADE                    one spell: Evade (DND)
    &Spell::EffectParry,                                    // 22 SPELL_EFFECT_PARRY
    &Spell::EffectBlock,                                    // 23 SPELL_EFFECT_BLOCK                    one spell: Block
    &Spell::EffectCreateItem,                               // 24 SPELL_EFFECT_CREATE_ITEM
    &Spell::EffectUnused,                                   // 25 SPELL_EFFECT_WEAPON
    &Spell::EffectUnused,                                   // 26 SPELL_EFFECT_DEFENSE                  one spell: Defense
    &Spell::EffectPersistentAA,                             // 27 SPELL_EFFECT_PERSISTENT_AREA_AURA
    &Spell::EffectSummonType,                               // 28 SPELL_EFFECT_SUMMON
    &Spell::EffectLeap,                                     // 29 SPELL_EFFECT_LEAP
    &Spell::EffectEnergize,                                 // 30 SPELL_EFFECT_ENERGIZE
    &Spell::EffectWeaponDmg,                                // 31 SPELL_EFFECT_WEAPON_PERCENT_DAMAGE
    &Spell::EffectTriggerMissileSpell,                      // 32 SPELL_EFFECT_TRIGGER_MISSILE
    &Spell::EffectOpenLock,                                 // 33 SPELL_EFFECT_OPEN_LOCK
    &Spell::EffectSummonChangeItem,                         // 34 SPELL_EFFECT_SUMMON_CHANGE_ITEM
    &Spell::EffectApplyAreaAura,                            // 35 SPELL_EFFECT_APPLY_AREA_AURA_PARTY
    &Spell::EffectLearnSpell,                               // 36 SPELL_EFFECT_LEARN_SPELL
    &Spell::EffectUnused,                                   // 37 SPELL_EFFECT_SPELL_DEFENSE            one spell: SPELLDEFENSE (DND)
    &Spell::EffectDispel,                                   // 38 SPELL_EFFECT_DISPEL
    &Spell::EffectUnused,                                   // 39 SPELL_EFFECT_LANGUAGE
    &Spell::EffectDualWield,                                // 40 SPELL_EFFECT_DUAL_WIELD
    &Spell::EffectJump,                                     // 41 SPELL_EFFECT_JUMP
    &Spell::EffectJumpDest,                                 // 42 SPELL_EFFECT_JUMP_DEST
    &Spell::EffectTeleUnitsFaceCaster,                      // 43 SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER
    &Spell::EffectLearnSkill,                               // 44 SPELL_EFFECT_SKILL_STEP
    &Spell::EffectPlayMovie,                                // 45 SPELL_EFFECT_PLAY_MOVIE
    &Spell::EffectUnused,                                   // 46 SPELL_EFFECT_SPAWN clientside, unit appears as if it was just spawned
    &Spell::EffectTradeSkill,                               // 47 SPELL_EFFECT_TRADE_SKILL
    &Spell::EffectUnused,                                   // 48 SPELL_EFFECT_STEALTH                  one spell: Base Stealth
    &Spell::EffectUnused,                                   // 49 SPELL_EFFECT_DETECT                   one spell: Detect
    &Spell::EffectTransmitted,                              // 50 SPELL_EFFECT_TRANS_DOOR
    &Spell::EffectUnused,                                   // 51 SPELL_EFFECT_FORCE_CRITICAL_HIT       unused
    &Spell::EffectUnused,                                   // 52 SPELL_EFFECT_GUARANTEE_HIT            unused
    &Spell::EffectEnchantItemPerm,                          // 53 SPELL_EFFECT_ENCHANT_ITEM
    &Spell::EffectEnchantItemTmp,                           // 54 SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY
    &Spell::EffectTameCreature,                             // 55 SPELL_EFFECT_TAMECREATURE
    &Spell::EffectSummonPet,                                // 56 SPELL_EFFECT_SUMMON_PET
    &Spell::EffectLearnPetSpell,                            // 57 SPELL_EFFECT_LEARN_PET_SPELL
    &Spell::EffectWeaponDmg,                                // 58 SPELL_EFFECT_WEAPON_DAMAGE
    &Spell::EffectCreateRandomItem,                         // 59 SPELL_EFFECT_CREATE_RANDOM_ITEM       create item base at spell specific loot
    &Spell::EffectProficiency,                              // 60 SPELL_EFFECT_PROFICIENCY
    &Spell::EffectSendEvent,                                // 61 SPELL_EFFECT_SEND_EVENT
    &Spell::EffectPowerBurn,                                // 62 SPELL_EFFECT_POWER_BURN
    &Spell::EffectThreat,                                   // 63 SPELL_EFFECT_THREAT
    &Spell::EffectTriggerSpell,                             // 64 SPELL_EFFECT_TRIGGER_SPELL
    &Spell::EffectApplyAreaAura,                            // 65 SPELL_EFFECT_APPLY_AREA_AURA_RAID
    &Spell::EffectRechargeManaGem,                          // 66 SPELL_EFFECT_CREATE_MANA_GEM          (possibly recharge it, misc - is item ID)
    &Spell::EffectHealMaxHealth,                            // 67 SPELL_EFFECT_HEAL_MAX_HEALTH
    &Spell::EffectInterruptCast,                            // 68 SPELL_EFFECT_INTERRUPT_CAST
    &Spell::EffectDistract,                                 // 69 SPELL_EFFECT_DISTRACT
    &Spell::EffectPull,                                     // 70 SPELL_EFFECT_PULL                     one spell: Distract Move
    &Spell::EffectPickPocket,                               // 71 SPELL_EFFECT_PICKPOCKET
    &Spell::EffectAddFarsight,                              // 72 SPELL_EFFECT_ADD_FARSIGHT
    &Spell::EffectUntrainTalents,                           // 73 SPELL_EFFECT_UNTRAIN_TALENTS
    &Spell::EffectApplyGlyph,                               // 74 SPELL_EFFECT_APPLY_GLYPH
    &Spell::EffectHealMechanical,                           // 75 SPELL_EFFECT_HEAL_MECHANICAL          one spell: Mechanical Patch Kit
    &Spell::EffectSummonObjectWild,                         // 76 SPELL_EFFECT_SUMMON_OBJECT_WILD
    &Spell::EffectScriptEffect,                             // 77 SPELL_EFFECT_SCRIPT_EFFECT
    &Spell::EffectUnused,                                   // 78 SPELL_EFFECT_ATTACK
    &Spell::EffectSanctuary,                                // 79 SPELL_EFFECT_SANCTUARY
    &Spell::EffectAddComboPoints,                           // 80 SPELL_EFFECT_ADD_COMBO_POINTS
    &Spell::EffectUnused,                                   // 81 SPELL_EFFECT_CREATE_HOUSE             one spell: Create House (TEST)
    &Spell::EffectNULL,                                     // 82 SPELL_EFFECT_BIND_SIGHT
    &Spell::EffectDuel,                                     // 83 SPELL_EFFECT_DUEL
    &Spell::EffectStuck,                                    // 84 SPELL_EFFECT_STUCK
    &Spell::EffectSummonPlayer,                             // 85 SPELL_EFFECT_SUMMON_PLAYER
    &Spell::EffectActivateObject,                           // 86 SPELL_EFFECT_ACTIVATE_OBJECT
    &Spell::EffectGameObjectDamage,                         // 87 SPELL_EFFECT_GAMEOBJECT_DAMAGE
    &Spell::EffectGameObjectRepair,                         // 88 SPELL_EFFECT_GAMEOBJECT_REPAIR
    &Spell::EffectGameObjectSetDestructionState,            // 89 SPELL_EFFECT_GAMEOBJECT_SET_DESTRUCTION_STATE
    &Spell::EffectKillCreditPersonal,                       // 90 SPELL_EFFECT_KILL_CREDIT              Kill credit but only for single person
    &Spell::EffectUnused,                                   // 91 SPELL_EFFECT_THREAT_ALL               one spell: zzOLDBrainwash
    &Spell::EffectEnchantHeldItem,                          // 92 SPELL_EFFECT_ENCHANT_HELD_ITEM
    &Spell::EffectForceDeselect,                            // 93 SPELL_EFFECT_FORCE_DESELECT
    &Spell::EffectSelfResurrect,                            // 94 SPELL_EFFECT_SELF_RESURRECT
    &Spell::EffectSkinning,                                 // 95 SPELL_EFFECT_SKINNING
    &Spell::EffectCharge,                                   // 96 SPELL_EFFECT_CHARGE
    &Spell::EffectCastButtons,                              // 97 SPELL_EFFECT_CAST_BUTTON (totem bar since 3.2.2a)
    &Spell::EffectKnockBack,                                // 98 SPELL_EFFECT_KNOCK_BACK
    &Spell::EffectDisEnchant,                               // 99 SPELL_EFFECT_DISENCHANT
    &Spell::EffectInebriate,                                //100 SPELL_EFFECT_INEBRIATE
    &Spell::EffectFeedPet,                                  //101 SPELL_EFFECT_FEED_PET
    &Spell::EffectDismissPet,                               //102 SPELL_EFFECT_DISMISS_PET
    &Spell::EffectReputation,                               //103 SPELL_EFFECT_REPUTATION
    &Spell::EffectSummonObject,                             //104 SPELL_EFFECT_SUMMON_OBJECT_SLOT1
    &Spell::EffectSummonObject,                             //105 SPELL_EFFECT_SUMMON_OBJECT_SLOT2
    &Spell::EffectSummonRaidMarker,                         //106 SPELL_EFFECT_SUMMON_OBJECT_SLOT3
    &Spell::EffectSummonObject,                             //107 SPELL_EFFECT_SUMMON_OBJECT_SLOT4
    &Spell::EffectDispelMechanic,                           //108 SPELL_EFFECT_DISPEL_MECHANIC
    &Spell::EffectSummonDeadPet,                            //109 SPELL_EFFECT_SUMMON_DEAD_PET
    &Spell::EffectDestroyAllTotems,                         //110 SPELL_EFFECT_DESTROY_ALL_TOTEMS
    &Spell::EffectDurabilityDamage,                         //111 SPELL_EFFECT_DURABILITY_DAMAGE
    &Spell::EffectUnused,                                   //112 SPELL_EFFECT_112
    &Spell::EffectResurrectNew,                             //113 SPELL_EFFECT_RESURRECT_NEW
    &Spell::EffectTaunt,                                    //114 SPELL_EFFECT_ATTACK_ME
    &Spell::EffectDurabilityDamagePCT,                      //115 SPELL_EFFECT_DURABILITY_DAMAGE_PCT
    &Spell::EffectSkinPlayerCorpse,                         //116 SPELL_EFFECT_SKIN_PLAYER_CORPSE       one spell: Remove Insignia, bg usage, required special corpse flags...
    &Spell::EffectSpiritHeal,                               //117 SPELL_EFFECT_SPIRIT_HEAL              one spell: Spirit Heal
    &Spell::EffectSkill,                                    //118 SPELL_EFFECT_SKILL                    professions and more
    &Spell::EffectApplyAreaAura,                            //119 SPELL_EFFECT_APPLY_AREA_AURA_PET
    &Spell::EffectUnused,                                   //120 SPELL_EFFECT_TELEPORT_GRAVEYARD       one spell: Graveyard Teleport Test
    &Spell::EffectWeaponDmg,                                //121 SPELL_EFFECT_NORMALIZED_WEAPON_DMG
    &Spell::EffectUnused,                                   //122 SPELL_EFFECT_122                      unused
    &Spell::EffectSendTaxi,                                 //123 SPELL_EFFECT_SEND_TAXI                taxi/flight related (misc value is taxi path id)
    &Spell::EffectPullTowards,                              //124 SPELL_EFFECT_PULL_TOWARDS
    &Spell::EffectModifyThreatPercent,                      //125 SPELL_EFFECT_MODIFY_THREAT_PERCENT
    &Spell::EffectStealBeneficialBuff,                      //126 SPELL_EFFECT_STEAL_BENEFICIAL_BUFF    spell steal effect?
    &Spell::EffectProspecting,                              //127 SPELL_EFFECT_PROSPECTING              Prospecting spell
    &Spell::EffectApplyAreaAura,                            //128 SPELL_EFFECT_APPLY_AREA_AURA_FRIEND
    &Spell::EffectApplyAreaAura,                            //129 SPELL_EFFECT_APPLY_AREA_AURA_ENEMY
    &Spell::EffectRedirectThreat,                           //130 SPELL_EFFECT_REDIRECT_THREAT
    &Spell::EffectPlaySound,                                //131 SPELL_EFFECT_PLAY_SOUND               sound id in misc value (SoundEntries.dbc)
    &Spell::EffectPlayMusic,                                //132 SPELL_EFFECT_PLAY_MUSIC               sound id in misc value (SoundEntries.dbc)
    &Spell::EffectUnlearnSpecialization,                    //133 SPELL_EFFECT_UNLEARN_SPECIALIZATION   unlearn profession specialization
    &Spell::EffectKillCredit,                               //134 SPELL_EFFECT_KILL_CREDIT              misc value is creature entry
    &Spell::EffectNULL,                                     //135 SPELL_EFFECT_CALL_PET
    &Spell::EffectHealPct,                                  //136 SPELL_EFFECT_HEAL_PCT
    &Spell::EffectEnergizePct,                              //137 SPELL_EFFECT_ENERGIZE_PCT
    &Spell::EffectLeapBack,                                 //138 SPELL_EFFECT_LEAP_BACK                Leap back
    &Spell::EffectQuestClear,                               //139 SPELL_EFFECT_CLEAR_QUEST              Reset quest status (miscValue - quest ID)
    &Spell::EffectForceCast,                                //140 SPELL_EFFECT_FORCE_CAST
    &Spell::EffectForceCast,                                //141 SPELL_EFFECT_FORCE_CAST_WITH_VALUE
    &Spell::EffectTriggerSpell,                             //142 SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE
    &Spell::EffectApplyAreaAura,                            //143 SPELL_EFFECT_APPLY_AREA_AURA_OWNER
    &Spell::EffectKnockBack,                                //144 SPELL_EFFECT_KNOCK_BACK_DEST
    &Spell::EffectPullTowards,                              //145 SPELL_EFFECT_PULL_TOWARDS_DEST                      Black Hole Effect
    &Spell::EffectActivateRune,                             //146 SPELL_EFFECT_ACTIVATE_RUNE
    &Spell::EffectQuestFail,                                //147 SPELL_EFFECT_QUEST_FAIL               quest fail
    &Spell::EffectTriggerMissileSpell,                      //148 SPELL_EFFECT_TRIGGER_MISSILE_SPELL_WITH_VALUE
    &Spell::EffectChargeDest,                               //149 SPELL_EFFECT_CHARGE_DEST
    &Spell::EffectQuestStart,                               //150 SPELL_EFFECT_QUEST_START
    &Spell::EffectTriggerRitualOfSummoning,                 //151 SPELL_EFFECT_TRIGGER_SPELL_2
    &Spell::EffectSummonRaFFriend,                          //152 SPELL_EFFECT_SUMMON_RAF_FRIEND        summon Refer-a-Friend
    &Spell::EffectCreateTamedPet,                           //153 SPELL_EFFECT_CREATE_TAMED_PET         misc value is creature entry
    &Spell::EffectDiscoverTaxi,                             //154 SPELL_EFFECT_DISCOVER_TAXI
    &Spell::EffectTitanGrip,                                //155 SPELL_EFFECT_TITAN_GRIP Allows you to equip two-handed axes, maces and swords in one hand, but you attack $49152s1% slower than normal.
    &Spell::EffectEnchantItemPrismatic,                     //156 SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC
    &Spell::EffectCreateItem2,                              //157 SPELL_EFFECT_CREATE_ITEM_2            create item or create item template and replace by some randon spell loot item
    &Spell::EffectMilling,                                  //158 SPELL_EFFECT_MILLING                  milling
    &Spell::EffectRenamePet,                                //159 SPELL_EFFECT_ALLOW_RENAME_PET         allow rename pet once again
    &Spell::EffectNULL,                                     //160 SPELL_EFFECT_160                      1 spell - 45534
    &Spell::EffectSpecCount,                                //161 SPELL_EFFECT_TALENT_SPEC_COUNT        second talent spec (learn/revert)
    &Spell::EffectActivateSpec,                             //162 SPELL_EFFECT_TALENT_SPEC_SELECT       activate primary/secondary spec
    &Spell::EffectUnused,                                   //163 SPELL_EFFECT_163  unused
    &Spell::EffectRemoveAura,                               //164 SPELL_EFFECT_REMOVE_AURA
    &Spell::EffectDamageFromMaxHealthPCT,                   //165 SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT
    &Spell::EffectGiveCurrency,                             //166 SPELL_EFFECT_GIVE_CURRENCY
    &Spell::EffectNULL,                                     //167 SPELL_EFFECT_167
    &Spell::EffectNULL,                                     //168 SPELL_EFFECT_168
    &Spell::EffectDestroyItem,                              //169 SPELL_EFFECT_DESTROY_ITEM
    &Spell::EffectNULL,                                     //170 SPELL_EFFECT_UPDATE_ZONE_AURAS_AND_PHASES
    &Spell::EffectNULL,                                     //171 SPELL_EFFECT_171
    &Spell::EffectResurrectWithAura,                        //172 SPELL_EFFECT_RESURRECT_WITH_AURA
    &Spell::EffectUnlockGuildVaultTab,                      //173 SPELL_EFFECT_UNLOCK_GUILD_VAULT_TAB
    &Spell::EffectNULL,                                     //174 SPELL_EFFECT_APPLY_AURA_ON_PET
    &Spell::EffectUnused,                                   //175 SPELL_EFFECT_175
    &Spell::EffectNULL,                                     //176 SPELL_EFFECT_176
    &Spell::EffectNULL,                                     //177 SPELL_EFFECT_177
    &Spell::EffectUnused,                                   //178 SPELL_EFFECT_178 unused
    &Spell::EffectCreateAreatrigger,                        //179 SPELL_EFFECT_CREATE_AREATRIGGER
    &Spell::EffectUnused,                                   //180 SPELL_EFFECT_180 unused
    &Spell::EffectUnlearnTalent,                            //181 SPELL_EFFECT_UNLEARN_TALENT
    &Spell::EffectNULL,                                     //182 SPELL_EFFECT_182
    &Spell::EffectNULL,                                     //183 SPELL_EFFECT_183
    &Spell::EffectNULL,                                     //184 SPELL_EFFECT_REPUTATION_REWARD
    &Spell::EffectNULL,                                     //185 SPELL_EFFECT_185
    &Spell::EffectNULL,                                     //186 SPELL_EFFECT_186
    &Spell::EffectNULL,                                     //187 SPELL_EFFECT_187
    &Spell::EffectNULL,                                     //188 SPELL_EFFECT_188
    &Spell::EffectNULL,                                     //189 SPELL_EFFECT_189
    &Spell::EffectNULL,                                     //190 SPELL_EFFECT_190
    &Spell::EffectNULL,                                     //191 SPELL_EFFECT_191
    &Spell::EffectNULL,                                     //192 SPELL_EFFECT_192
    &Spell::EffectNULL,                                     //193 SPELL_EFFECT_193
    &Spell::EffectNULL,                                     //194 SPELL_EFFECT_194
    &Spell::EffectNULL,                                     //195 SPELL_EFFECT_195
    &Spell::EffectNULL,                                     //196 SPELL_EFFECT_196
    &Spell::EffectNULL,                                     //197 SPELL_EFFECT_197
    &Spell::EffectStartScene,                               //198 SPELL_EFFECT_START_SCENE
    &Spell::EffectNULL,                                     //199 SPELL_EFFECT_199
    &Spell::EffectNULL,                                     //200 SPELL_EFFECT_HEAL_BATTLEPET_PCT
    &Spell::EffectNULL,                                     //201 SPELL_EFFECT_201
    &Spell::EffectNULL,                                     //202 SPELL_EFFECT_202
    &Spell::EffectNULL,                                     //203 SPELL_EFFECT_203
    &Spell::EffectNULL,                                     //204 SPELL_EFFECT_204
    &Spell::EffectNULL,                                     //205 SPELL_EFFECT_205
    &Spell::EffectNULL,                                     //206 SPELL_EFFECT_206
    &Spell::EffectNULL,                                     //207 SPELL_EFFECT_207
    &Spell::EffectNULL,                                     //208 SPELL_EFFECT_208
};
void Spell::EffectNULL(SpellEffIndex /*effIndex*/)
{
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "WORLD: Spell Effect DUMMY");
}

void Spell::EffectUnused(SpellEffIndex /*effIndex*/)
{
    // NOT USED BY ANY SPELL OR USELESS OR IMPLEMENTED IN DIFFERENT WAY IN TRINITY
}

void Spell::EffectResurrectNew(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->isAlive())
        return;

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!unitTarget->IsInWorld())
        return;

    Player* target = unitTarget->ToPlayer();

    if (target->IsRessurectRequested())       // already have one active request
        return;

    uint32 health = damage;
    uint32 mana = m_spellInfo->Effects[effIndex].MiscValue;
    ExecuteLogEffectResurrect(effIndex, target);
    target->SetResurrectRequestData(m_caster, health, mana, 0);
    SendResurrectRequest(target);
}

void Spell::EffectInstaKill(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || (!unitTarget->isAlive() && m_spellInfo->Id != 108503))
        return;

    if (m_spellInfo->Id == 108503 && (!unitTarget->GetHealth() || !unitTarget->isAlive()))
    {
        unitTarget->ToPet()->Remove(PET_SLOT_ACTUAL_PET_SLOT, false, unitTarget->ToPet()->m_Stampeded);
        return;
    }

    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
        if (unitTarget->ToPlayer()->GetCommandStatus(CHEAT_GOD))
            return;

    if (m_caster == unitTarget)                              // prevent interrupt message
        finish();

    WorldPacket data(SMSG_SPELLINSTAKILLLOG, 8+8+4);

    ObjectGuid casterGUID = m_caster->GetGUID();
    ObjectGuid targetGUID = unitTarget->GetGUID();

    data.WriteBit(targetGUID[1]);
    data.WriteBit(casterGUID[0]);
    data.WriteBit(casterGUID[2]);
    data.WriteBit(targetGUID[7]);
    data.WriteBit(targetGUID[2]);
    data.WriteBit(casterGUID[3]);
    data.WriteBit(casterGUID[1]);
    data.WriteBit(targetGUID[3]);
    data.WriteBit(targetGUID[6]);
    data.WriteBit(targetGUID[4]);
    data.WriteBit(casterGUID[5]);
    data.WriteBit(targetGUID[0]);
    data.WriteBit(targetGUID[5]);
    data.WriteBit(casterGUID[7]);
    data.WriteBit(casterGUID[6]);
    data.WriteBit(casterGUID[4]);

    data.WriteByteSeq(targetGUID[2]);
    data.WriteByteSeq(casterGUID[0]);
    data << uint32(m_spellInfo->Id);
    data.WriteByteSeq(casterGUID[1]);
    data.WriteByteSeq(targetGUID[4]);
    data.WriteByteSeq(casterGUID[2]);
    data.WriteByteSeq(targetGUID[6]);
    data.WriteByteSeq(casterGUID[4]);
    data.WriteByteSeq(targetGUID[5]);
    data.WriteByteSeq(casterGUID[7]);
    data.WriteByteSeq(casterGUID[3]);
    data.WriteByteSeq(targetGUID[7]);
    data.WriteByteSeq(targetGUID[0]);
    data.WriteByteSeq(targetGUID[3]);
    data.WriteByteSeq(casterGUID[5]);
    data.WriteByteSeq(targetGUID[1]);
    data.WriteByteSeq(casterGUID[6]);

    m_caster->SendMessageToSet(&data, true);

    m_caster->DealDamage(unitTarget, unitTarget->GetHealth(), NULL, NODAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
}

void Spell::EffectEnvironmentalDMG(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive())
        return;

    uint32 absorb = 0;
    uint32 resist = 0;

    m_caster->CalcAbsorbResist(unitTarget, m_spellInfo->GetSchoolMask(), SPELL_DIRECT_DAMAGE, damage, &absorb, &resist, m_spellInfo);

    m_caster->SendSpellNonMeleeDamageLog(unitTarget, m_spellInfo->Id, damage, m_spellInfo->GetSchoolMask(), absorb, resist, false, 0, false);
    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
        unitTarget->ToPlayer()->EnvironmentalDamage(DAMAGE_FIRE, damage);
}

void Spell::EffectSchoolDMG(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH_TARGET)
        return;

    if (unitTarget && unitTarget->isAlive())
    {
        bool apply_direct_bonus = true;
        switch (m_spellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                // Meteor like spells (divided damage to targets)
                if (m_spellInfo->AttributesCu & SPELL_ATTR0_CU_SHARE_DAMAGE)
                {
                    uint32 count = 0;
                    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        if (ihit->effectMask & (1<<effIndex))
                            ++count;

                    switch (m_spellInfo->Id)
                    {
                        case 107439: // Twilight Barrage
                        case 109203: // Twilight Barrage
                        case 109204: // Twilight Barrage
                        case 109205: // Twilight Barrage
                        case 106401: // Twilight Onslaught
                        case 108862: // Twilight Onslaught
                        case 109226: // Twilight Onslaught
                        case 109227: // Twilight Onslaught
                            ++count; // + ship
                            break;
                    }

                    damage /= count;                    // divide to all targets
                }

                switch (m_spellInfo->Id)                     // better way to check unknown
                {
                    case 109721: // Lightning Strike, Vial of Shadows (lfr)
                        damage += int32(0.266f * m_caster->GetTotalAttackPowerValue(m_caster->getClass() == CLASS_HUNTER ? RANGED_ATTACK : BASE_ATTACK));
                        break;
                    case 107994: // Lightning Strike, Vial of Shadows (normal)
                        damage += int32(0.3f * m_caster->GetTotalAttackPowerValue(m_caster->getClass() == CLASS_HUNTER ? RANGED_ATTACK : BASE_ATTACK));
                        break;
                    case 109724: // Lightning Strike, Vial of Shadows (heroic)
                        damage += int32(0.339f * m_caster->GetTotalAttackPowerValue(m_caster->getClass() == CLASS_HUNTER ? RANGED_ATTACK : BASE_ATTACK));
                        break;
                    // Detonate Mana, Tyrande's Favorite Doll
                    case 92601:
                        if (constAuraEffectPtr aurEff = m_caster->GetAuraEffect(92596, EFFECT_0))
                            damage = aurEff->GetAmount();
                        break;
                    case 105033: // Searing Blood, Yor'sahj The Unsleeping, Dragon Soul
                    case 108356:
                    case 108357:
                    case 108358:
                    case 108218: // Searing Blood, Crimson Globule, Dragon Soul
                    case 108363:
                    {
                        if (!unitTarget)
                            break;

                        float dist = m_caster->GetDistance(unitTarget);

                        if (dist > 10.0f)
                            damage *= dist / 10.0f;
                        break;
                    }
                    // Resonating Crystal dmg, Morchok, Dragon Soul
                    case 103545:
                    case 108572:
                    case 110041:
                    case 110040:
                        if (!unitTarget)
                            break;

                        if (unitTarget->HasAura(103534))
                            damage *= 1.5f;
                        else if (unitTarget->HasAura(103536))
                            damage *= 0.7f;
                        else if (unitTarget->HasAura(103541))
                            damage *= 0.3f;

                        unitTarget->RemoveAura(103534);
                        unitTarget->RemoveAura(103536);
                        unitTarget->RemoveAura(103541);
                        break;
                    // Stomp, Morchok, Dragon Soul
                    case 103414:
                    case 108571:
                    case 109033:
                    case 109034:
                    {
                        if (!unitTarget)
                            break;

                        if (Creature* pMorchok = m_caster->ToCreature())
                        {
                            if ((pMorchok->GetEntry() == 57773) || pMorchok->AI()->GetData(3))
                                damage /= 2;

                            if ((unitTarget->GetGUID() == pMorchok->AI()->GetGUID(1)) || 
                                (unitTarget->GetGUID() == pMorchok->AI()->GetGUID(2)))
                                damage *= 2;
                        }
                        break;
                    }
                    // Ragnaros (Firelands), Molten Inferno
                    case 98518:
                    case 100252:
                    case 100253:
                    case 100254:
                    {
                        int32 min_dmg = 8000;
                        float distance = m_caster->GetDistance(unitTarget);
                        int32 new_dmg = damage - (int32(distance * 4000));
                        damage = std::max(min_dmg, new_dmg);
                        break;
                    }
                    // Mirror Image, Frost Bolt
                    case 59638:
                        if (m_caster->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
                            damage += int32(((Guardian*)m_caster)->GetBonusDamage() * 0.25f);
                        break;
                    // Consumption
                    case 28865:
                        damage = (((InstanceMap*)m_caster->GetMap())->GetDifficulty() == DUNGEON_DIFFICULTY_NORMAL ? 2750 : 4250);
                        break;
                    // percent from health with min
                    case 25599:                             // Thundercrash
                    {
                        damage = unitTarget->GetHealth() / 2;
                        if (damage < 200)
                            damage = 200;
                        break;
                    }
                    // arcane charge. must only affect demons (also undead?)
                    case 45072:
                    {
                        if (unitTarget->GetCreatureType() != CREATURE_TYPE_DEMON
                            && unitTarget->GetCreatureType() != CREATURE_TYPE_UNDEAD)
                            return;
                        break;
                    }
                    case 123199:// Toss Explosive Barrel
                        if (unitTarget->GetTypeId() == TYPEID_PLAYER ||
                            (unitTarget->GetTypeId() == TYPEID_UNIT && unitTarget->GetOwner() && unitTarget->GetOwner()->ToPlayer()))
                            return;
                        break;
                }
                break;
            }
            case SPELLFAMILY_WARRIOR:
            {
                switch (m_spellInfo->Id)
                {
                    case 34428: // Victory Rush
                    {
                        if (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_WARRIOR_ARMS)
                            damage = CalculatePct(m_caster->GetTotalAttackPowerValue(BASE_ATTACK), 67.2f);
                        else
                            damage = CalculatePct(m_caster->GetTotalAttackPowerValue(BASE_ATTACK), 56.0f);

                        break;
                    }
                    case 46968: // Shockwave
                    {
                        if (m_caster->GetTypeId() != TYPEID_PLAYER)
                            break;

                        int32 pct = 0;

                        switch (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()))
                        {
                            case SPEC_WARRIOR_ARMS:
                                pct = 110;
                                break;
                            case SPEC_WARRIOR_FURY:
                            case SPEC_WARRIOR_PROTECTION:
                                pct = 75;
                                break;
                            default:
                                break;
                        }

                        // If Shockwave hits more than 3 targets, its cooldown is reduced for 20 seconds
                        if (m_UniqueTargetInfo.size() > 3 && !m_caster->ToPlayer()->HasSpellCooldown(136848))
                        {
                            m_caster->ToPlayer()->ReduceSpellCooldown(46968, 20000);
                            // hack way to prevent multiple calls of this condition
                            m_caster->ToPlayer()->AddSpellCooldown(136848, 0, time(NULL) + 5);
                        }

                        damage = int32(CalculatePct(m_caster->GetTotalAttackPowerValue(BASE_ATTACK), pct));

                        break;
                    }
                    case 103840:// Impending Victory
                    {
                        if (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_WARRIOR_ARMS)
                            damage = CalculatePct(m_caster->GetTotalAttackPowerValue(BASE_ATTACK), 70.0f);
                        else
                            damage = CalculatePct(m_caster->GetTotalAttackPowerValue(BASE_ATTACK), 56.0f);

                        // Impending Victory heals you for 10% of your maximum health
                        m_caster->CastSpell(m_caster, 118340, true);

                        break;
                    }
                    case 118000:// Dragon Roar
                    {
                        if (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_WARRIOR_ARMS)
                            damage += CalculatePct(m_caster->GetTotalAttackPowerValue(BASE_ATTACK), 168);
                        else
                            damage += CalculatePct(m_caster->GetTotalAttackPowerValue(BASE_ATTACK), 140);

                        break;
                    }
                }

                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                switch (m_spellInfo->Id)
                {
                    // Ancient Fury
                    case 86704:
                    {
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        {
                            if (AuraPtr aura = m_caster->GetAura(86700))
                            {
                                uint8 stacks = aura->GetStackAmount();
                                damage = stacks * (damage + 0.1f * m_caster->SpellBaseDamageBonusDone(m_spellInfo->GetSchoolMask()));
                                damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, damage, SPELL_DIRECT_DAMAGE);
                                uint32 count = 0;
                                for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                                    ++count;
                                damage /= count;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            case SPELLFAMILY_WARLOCK:
            {
                switch (m_spellInfo->Id)
                {
                    case 29722: // Incinerate
                    case 114654:// Incinerate (Fire and Brimstone)
                    {
                        // Incinerate does more dmg (dmg/6) if the target have Immolate debuff.
                        // Check aura state for speed but aura state set not only for Immolate spell
                        if (unitTarget->HasAuraState(AURA_STATE_CONFLAGRATE))
                        {
                            if (unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARLOCK, 0x4, 0, 0))
                                damage += damage / 6;
                        }
                        break;
                    }
                    case 87385:// Soulburn : Seed of Corruption (AoE)
                    {
                        // Apply corruption to each target
                        if (m_caster->HasAura(86664))
                            m_caster->AddAura(172, unitTarget);

                        break;
                    }
                    default:
                        break;
                }

                break;
            }
            case SPELLFAMILY_PRIEST:
            {
                // Improved Mind Blast (Mind Blast in shadow form bonus)
                if (m_caster->GetShapeshiftForm() == FORM_SHADOW && (m_spellInfo->SpellFamilyFlags[0] & 0x00002000))
                {
                    Unit::AuraEffectList const& ImprMindBlast = m_caster->GetAuraEffectsByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                    for (Unit::AuraEffectList::const_iterator i = ImprMindBlast.begin(); i != ImprMindBlast.end(); ++i)
                    {
                        if ((*i)->GetSpellInfo()->SpellFamilyName == SPELLFAMILY_PRIEST &&
                            ((*i)->GetSpellInfo()->SpellIconID == 95))
                        {
                            int chance = (*i)->GetSpellInfo()->Effects[EFFECT_1].CalcValue(m_caster);
                            if (roll_chance_i(chance))
                                // Mind Trauma
                                m_caster->CastSpell(unitTarget, 48301, true, 0);
                            break;
                        }
                    }
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                switch (m_spellInfo->Id)
                {
                    case 770:   // Faerie Fire
                    case 102355:// Faerie Swarm
                        if (m_caster->GetShapeshiftForm() != FORM_BEAR)
                            return;

                        // Glyph of Fae Silence
                        if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->HasAura(114237))
                        {
                            m_caster->CastSpell(unitTarget, 114238, true);
                            m_caster->ToPlayer()->AddSpellCooldown(m_spellInfo->Id, 0, time(NULL) + 15);
                        }

                        break;
                    case 22568: // Ferocious Bite
                    {
                        if (m_caster->GetTypeId() != TYPEID_PLAYER)
                            return;

                        damage += int32(0.196f * m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * m_caster->ToPlayer()->GetComboPoints());

                        // converts each extra point of energy ( up to 25 energy ) into additional damage
                        int32 energy = -(m_caster->ModifyPower(POWER_ENERGY, -25));
                        // 25 energy = 100% more damage
                        AddPct(damage, energy * 4);

                        // if target is under 25% of life, also reset rake duration
                        if (unitTarget->GetHealthPct() <= 25.0f)
                            if (AuraPtr aura = unitTarget->GetAura(1822))
                                aura->RefreshDuration();

                        // Glyph of Ferocious Bite
                        if (m_caster->HasAura(67598))
                        {
                            int32 heal_pct = int32((25 + energy) / 10);
                            m_caster->CastCustomSpell(m_caster, 101024, &heal_pct, 0, 0, true);
                        }

                        break;
                    }
                    default:
                        break;
                }
            }
            case SPELLFAMILY_ROGUE:
            {
                switch (m_spellInfo->Id)
                {
                    case 2098:  // Eviscerate
                    {
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        {
                            if (uint32 combo = ((Player*)m_caster)->GetComboPoints())
                            {
                                float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);

                                switch (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()))
                                {
                                    case SPEC_ROGUE_ASSASSINATION:
                                    case SPEC_ROGUE_COMBAT:
                                        damage += int32(ap * combo * 0.18f);
                                        break;
                                    case SPEC_ROGUE_SUBTLETY:
                                    default:
                                        damage += int32(ap * combo * 0.223f);
                                        break;
                                }

                                // Eviscerate and Envenom Bonus Damage (item set effect)
                                if (AuraEffectPtr eviscerateBonus = m_caster->GetAuraEffect(37169, EFFECT_0))
                                    AddPct(damage, eviscerateBonus->GetAmount());
                            }
                        }

                        break;
                    }
                    case 26679: // Deadly Throw
                    {
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        {
                            if (uint32 combo = ((Player*)m_caster)->GetComboPoints())
                            {
                                float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);

                                if (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_ROGUE_ASSASSINATION
                                    || m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_ROGUE_COMBAT)
                                    damage += int32(ap * combo * 0.12f);
                                else if (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_ROGUE_SUBTLETY)
                                    damage += int32(ap * combo * 0.149f);
                            }
                        }

                        break;
                    }
                    case 32645: // Envenom
                    {
                        if (Player* player = m_caster->ToPlayer())
                        {
                            uint8 combo = player->GetComboPoints();

                            float ap = player->GetTotalAttackPowerValue(BASE_ATTACK);

                            if (combo)
                            {
                                damage += int32(0.112f * combo * ap + damage * combo);

                                // Eviscerate and Envenom Bonus Damage (item set effect)
                                if (m_caster->HasAura(37169))
                                    damage += combo * 40;

                                // 5.4.0 blizzard fix, now it deals 10% less damage
                                damage = int32(damage * 0.9f);
                            }
                        }

                        break;
                    }
                    case 51723: // Fan of Knives
                    {
                        if (m_caster->GetTypeId() != TYPEID_PLAYER)
                            break;

                        // Fan of Knives - Vile Poisons
                        if (AuraEffectPtr aur = m_caster->GetDummyAuraEffect(SPELLFAMILY_ROGUE, 857, 2))
                        {
                            if (roll_chance_i(aur->GetAmount()))
                            {
                                for (uint8 i = BASE_ATTACK; i < MAX_ATTACK; ++i)
                                    m_caster->ToPlayer()->CastItemCombatSpell(unitTarget, WeaponAttackType(i), PROC_FLAG_TAKEN_DAMAGE, PROC_EX_NORMAL_HIT);
                            }
                        }

                        // Glyph of Sharpened Knives
                        if (m_caster->HasAura(146628))
                            m_caster->CastSpell(unitTarget, 113746, true);

                        break;
                    }
                    case 121411:// Crimson Tempest
                    {
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        {
                            if (uint32 combo = ((Player*)m_caster)->GetComboPoints())
                            {
                                float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);

                                if (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_ROGUE_ASSASSINATION
                                    || m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_ROGUE_COMBAT)
                                    damage += int32(ap * combo * 0.028f);
                                else if (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_ROGUE_SUBTLETY)
                                    damage += int32(ap * combo * 0.034f);
                            }
                        }

                        break;
                    }
                }

                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                switch (m_spellInfo->Id)
                {
                    case 13812: // Explosive Trap
                        if (m_caster->HasAura(119403)) // Glyph of Explosive Trap
                            return;
                        break;
                    case 16827: // Claw
                    case 17253: // Bite
                    {
                        if (m_caster->GetOwner())
                        {
                            damage += int32(m_caster->GetOwner()->GetTotalAttackPowerValue(RANGED_ATTACK) * 0.2544f);

                            // Deals 100% more damage and costs 100% more Focus when your pet has 50 or more Focus.
                            if (m_caster->GetPower(POWER_FOCUS) + 25 > 50)
                            {
                                damage *= 2;
                                m_caster->EnergizeBySpell(m_caster, m_spellInfo->Id, -25, POWER_FOCUS);
                            }
                        }

                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_MAGE:
            {
                switch (m_spellInfo->Id)
                {
                    // Mirror Image, Fire Blast
                    case 59637:
                        if (m_caster->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
                            damage += int32(((Guardian*)m_caster)->GetBonusDamage() * 0.15f);
                        break;

                    default: break;
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                switch (m_spellInfo->Id)
                {
                    // Gargoyle Strike
                    case 51963:
                    {
                        // calculate owner pvp power
                        if (Unit* owner = m_caster->GetOwner())
                        {
                            float value = 1 + (owner->GetFloatValue(PLAYER_MASTERY) * 2.5f / 100.0f);
                            damage = int32(damage * value);
                        }
                        break;
                    }
                }
            }
            case SPELLFAMILY_MONK:
            {
                switch (m_spellInfo->Id)
                {
                    // Custom MoP script
                    case 123408:// Spinning Fire Blossom
                        damage = CalculateMonkMeleeAttacks(m_caster, 1.5f, 6);
                        break;
                    case 117418:// Fists of Fury
                    {
                        damage = CalculateMonkMeleeAttacks(m_caster, 7.5f, 14);

                        uint32 count = 0;
                        for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                            if (ihit->effectMask & (1<<effIndex))
                                ++count;

                        damage /= count;

                        break;
                    }
                    case 100780:// Jab
                    case 108557:// Jab (Staff)
                    case 115698:// Jab (Polearm)
                    case 115687:// Jab (Axes)
                    case 115693:// Jab (Maces)
                    case 115695:// Jab (Swords)
                        damage = CalculateMonkMeleeAttacks(m_caster, 1.5f, 14);
                        break;
                    case 115080:// Touch of Death
                        if (Unit* caster = GetCaster())
                        {
                            if (unitTarget)
                            {
                                uint32 damage = unitTarget->GetHealth();
                                m_caster->SendSpellNonMeleeDamageLog(unitTarget, m_spellInfo->Id, damage, m_spellInfo->GetSchoolMask(), 0, 0, false, 0, false);
                                m_caster->DealDamageMods(unitTarget, damage, NULL);
                                m_caster->DealDamage(unitTarget, damage, NULL, SPELL_DIRECT_DAMAGE, m_spellInfo->GetSchoolMask(), m_spellInfo, false);
                            }
                        }

                        return;
                    case 100787:// Tiger Palm
                        damage = CalculateMonkMeleeAttacks(m_caster, 3.0f, 14);
                        break;
                    case 107270:// Spinning Crane Kick
                        damage = CalculateMonkMeleeAttacks(m_caster, 1.59f, 14);
                        break;
                    case 107428:// Rising Sun Kick
                        damage = CalculateMonkMeleeAttacks(m_caster, 12.816f, 14);
                        m_caster->CastSpell(unitTarget, 130320, true);
                        break;
                    case 100784:// Blackout Kick
                        damage = CalculateMonkMeleeAttacks(m_caster, 7.12f, 14);
                        break;
                    case 148187:// Rushing Jade Wind
                        damage = CalculateMonkMeleeAttacks(m_caster, 1.47f, 14);
                        break;
                    case 124335:// Swift Reflexes
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        {
                            switch (m_caster->ToPlayer()->GetSpecializationId(m_caster->ToPlayer()->GetActiveSpec()))
                            {
                                case SPEC_MONK_BREWMASTER:
                                    damage = CalculateMonkMeleeAttacks(m_caster, 0.3f, 5);
                                    break;
                                case SPEC_MONK_MISTWEAVER:
                                    damage = CalculateMonkMeleeAttacks(m_caster, 0.3f, 6);
                                    break;
                                case SPEC_MONK_WINDWALKER:
                                    damage = CalculateMonkMeleeAttacks(m_caster, 0.3f, 6);
                                    // Sparring
                                    if (m_caster->HasAura(116033))
                                        m_caster->CastSpell(m_caster, 116033, true);
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    case 121253:// Keg Smash
                        damage = CalculateMonkMeleeAttacks(m_caster, 8.12f, 11);
                        break;
                    default:
                        break;
                }
                break;
            }
        }

        if (m_originalCaster && damage > 0 && apply_direct_bonus)
        {
            damage = m_originalCaster->SpellDamageBonusDone(unitTarget, m_spellInfo, (uint32)damage, SPELL_DIRECT_DAMAGE);
            damage = unitTarget->SpellDamageBonusTaken(m_originalCaster, m_spellInfo, (uint32)damage, SPELL_DIRECT_DAMAGE);
        }

        // Frost Bomb - aoe targets get only half dmg.
        if (m_spellInfo->Id == 113092 && effIndex == 1)
            damage /= 2;

        m_damage += damage;
    }
}

void Spell::EffectDummy(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget && !gameObjTarget && !itemTarget)
        return;

    uint32 spell_id = 0;
    int32 bp = 0;
    bool triggered = true;
    SpellCastTargets targets;

    // Fishing dummy
    if (m_spellInfo->Id == 131474)
        m_caster->CastSpell(m_caster, 131476, true);

    // selection by spell family
    switch (m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->Id)
            {
                // Flask of Enhancement
                case 79637:
                {
                    uint32 stat_str = m_caster->GetStat(STAT_STRENGTH);
                    uint32 stat_agi = m_caster->GetStat(STAT_AGILITY);
                    uint32 stat_int = m_caster->GetStat(STAT_INTELLECT);

                    if (stat_str > stat_agi && stat_str > stat_int)
                        m_caster->CastSpell(m_caster, 79638, true);
                    else if (stat_agi > stat_str && stat_agi > stat_int)
                        m_caster->CastSpell(m_caster, 79639, true);
                    else if (stat_int > stat_agi && stat_int > stat_str)
                        m_caster->CastSpell(m_caster, 79640, true);

                    break;
                }
                case 96934: // Blessing of Khaz'goroth
                case 97127: // Blessing of Khaz'goroth (H)
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (!m_caster->HasAura(96923))
                        return;

                    uint32 crit = m_caster->ToPlayer()->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_CRIT_MELEE);
                    uint32 mastery = m_caster->ToPlayer()->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_MASTERY);
                    uint32 haste = m_caster->ToPlayer()->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_HASTE_MELEE);
                    
                    uint8 stacks = 1;
                    if (AuraPtr aur = m_caster->GetAura(96923))
                        stacks = aur->GetStackAmount();

                    int32 bp0 = damage * stacks;

                    if (crit > mastery && crit > haste)
                        m_caster->CastCustomSpell(m_caster, 96928, &bp0, 0, 0, true);
                    else if (haste > mastery && haste > crit)
                        m_caster->CastCustomSpell(m_caster, 96927, &bp0, 0, 0, true);
                    else if (mastery > haste && mastery > crit)
                        m_caster->CastCustomSpell(m_caster, 96929, &bp0, 0, 0, true);
                    
                    m_caster->RemoveAurasDueToSpell(96923);

                    break;
                }
                case 25952: // Reindeer Dust Effect
                {
                    if (m_caster->GetTypeId() == TYPEID_UNIT)
                        m_caster->ToCreature()->DisappearAndDie();
                    return;
                }
                case 101992:// Put up Darkmoon Banner
                {
                    m_caster->CastSpell(m_caster, 102006, true);
                    break;
                }
                case 122180:// Tap to the Spirit World
                {
                    m_caster->CastSpell(unitTarget, 122181, true);
                    break;
                }
                case 47468: // Ghoul: Claw
                {
                    if (!unitTarget)
                        return;

                    // Dark Transformation - Replace spell
                    if (m_caster->HasAura(63560))
                        m_caster->CastSpell(unitTarget, 91778, true); // Sweeping Claws
                    else
                        m_caster->CastSpell(unitTarget, 91776, true); // Claw

                    break;
                }
                case 47484: // Ghoul: Huddle
                {
                    if (!unitTarget)
                        return;

                    // Dark Transformation - Replace spell
                    if (m_caster->HasAura(63560))
                        m_caster->CastSpell(unitTarget, 91837, true); // Putrid Bulwark
                    else
                        m_caster->CastSpell(unitTarget, 91838, true); // Huddle
                    break;
                }
                case 47482: // Ghoul: Leap
                {
                    if (!unitTarget)
                        return;

                    // Dark Transformation - Replace spell
                    if (m_caster->HasAura(63560))
                        m_caster->CastSpell(unitTarget, 91802, true); // Shambling Rush
                    else
                        m_caster->CastSpell(unitTarget, 91809, true); // Leap
                    break;
                }
                case 6203:  // Soulstone
                {
                    if (!unitTarget->isAlive())
                        unitTarget->CastSpell(unitTarget, 3026, true); // Self resurrect
                    break;
                }
                case 45206: // Copy Off-hand Weapon
                case 69892:
                {
                    m_caster->CastSpell(unitTarget, damage, true);
                    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
                        break;

                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (Item* offItem = m_caster->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                            unitTarget->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, offItem->GetEntry());
                    }
                    else
                        unitTarget->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, m_caster->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1));
                    break;
                }
                case 41055: // Copy Mainhand Weapon
                case 63416:
                case 69891:
                {
                    m_caster->CastSpell(unitTarget, damage, true);
                    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
                        break;

                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (Item* mainItem = m_caster->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
                            unitTarget->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, mainItem->GetEntry());
                    }
                    else
                        unitTarget->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, m_caster->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID));
                    break;
                }
                default:
                    break;
            }

            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            switch (m_spellInfo->Id)
            {
                case 136494:
                {
                    if (unitTarget)
                        m_caster->CastSpell(unitTarget, 130552, true);
                    break;
                }
                case 31789: // Righteous Defense (step 1)
                {
                    // Clear targets for eff 1
                    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        ihit->effectMask &= ~(1<<1);

                    // not empty (checked), copy
                    Unit::AttackerSet attackers = unitTarget->getAttackers();

                    // remove invalid attackers
                    for (Unit::AttackerSet::iterator aItr = attackers.begin(); aItr != attackers.end();)
                        if (!(*aItr)->IsValidAttackTarget(m_caster))
                            attackers.erase(aItr++);
                        else
                            ++aItr;

                    // selected from list 3
                    uint32 maxTargets = std::min<uint32>(3, attackers.size());
                    for (uint32 i = 0; i < maxTargets; ++i)
                    {
                        Unit* attacker = SkyMistCore::Containers::SelectRandomContainerElement(attackers);
                        AddUnitTarget(attacker, 1 << 1);
                        attackers.erase(attacker);
                    }

                    // now let next effect cast spell at each target.
                    return;
                }
                default:
                    break;
            }

            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            switch (m_spellInfo->Id)
            {
                case 53478: // Last Stand (Tenacity)
                    m_caster->CastSpell(m_caster, 53479, true);
                    break;
                case 51753: // Camouflage
                    m_caster->CastSpell(m_caster, 51755, true);
                    m_caster->CastSpell(m_caster, 80326, true);

                    if (m_caster->isInCombat())
                        if (AuraPtr camouflage = m_caster->GetAura(51755))
                            camouflage->SetDuration(6000);

                    if (Unit* pet = m_caster->GetGuardianPet())
                        pet->CastSpell(pet, 51753, true);

                    break;
                case 63487: // Ice Trap
                    if (Unit* owner = m_caster->GetOwner())
                        owner->CastSpell(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), 13810, true);
                    break;
                case 128997:// Spirit Beast Blessing
                {
                    m_caster->CastSpell(m_caster, 127830, true);
                    break;
                }
                default:
                    break;
            }

            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            switch (m_spellInfo->Id)
            {
                case 45529: // Blood Tap
                {
                    if (!m_caster || !m_caster->ToPlayer())
                        return;

                    auto player = m_caster->ToPlayer();

                    uint8 runeIndex = 255;
                    for (uint8 i = 0; i < MAX_RUNES; ++i)
                    {
                        uint32 cd = player->GetRuneCooldown(i);
                        uint32 base = player->GetRuneBaseCooldown(i);
                        if (cd != base)
                            continue;

                        runeIndex = i;
                        break;
                    }
 
                    if (runeIndex == 255)
                        return; // there is no deplated runes

                    player->SetRuneCooldown(runeIndex, 0);
                    if (player->GetCurrentRune(runeIndex) != RUNE_DEATH)
                        player->AddRuneBySpell(runeIndex, RUNE_DEATH, m_spellInfo->Id);

                    player->ResyncRunes(MAX_RUNES);
                    break;
                }
                // Death Grip
                case 49576:
                {
                    if (!unitTarget)
                        return;

                    Unit *caster = m_caster;
                    if (m_UniqueTargetInfo.size())
                    {
                        if (m_UniqueTargetInfo.front().missCondition == SPELL_MISS_REFLECT)
                            caster = m_targets.GetUnitTarget();
                    }
                    caster->CastSpell(unitTarget, 49560, true);
                    break;
                }
                case 47481: // Ghoul: Gnaw
                {
                    if (!unitTarget)
                        return;

                    // Dark Transformation - Replace spell
                    if (m_caster->HasAura(63560))
                        m_caster->CastSpell(unitTarget, 91797, true); // Monstrous Blow
                    else
                        m_caster->CastSpell(unitTarget, 91800, true); // Gnaw

                    break;
                }
                case 47541: // Death Coil
                {
                    if (m_caster->IsFriendlyTo(unitTarget))
                    {
                        int32 bp = (damage + m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.514f) * 3.5f;
                        m_caster->CastCustomSpell(unitTarget, 47633, &bp, NULL, NULL, true);

                        // Glyph of Death's Embrace
                        if (m_caster->HasAura(58677))
                            m_caster->CastSpell(m_caster, 58679, true);
                    }
                    else
                    {
                        int32 bp = damage + m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.514f;
                        m_caster->CastCustomSpell(unitTarget, 47632, &bp, NULL, NULL, true);
                    }

                    return;
                }
                case 49998: // Death Strike
                {
                    if ((m_caster->CountPctFromMaxHealth(7)) > (20 * m_caster->GetDamageTakenInPastSecs(5) / 100))
                        bp = m_caster->CountPctFromMaxHealth(7);
                    else
                        bp = (20 * m_caster->GetDamageTakenInPastSecs(5) / 100);

                    // Item - Death Knight T14 Blood 4P bonus
                    if (m_caster->HasAura(123080))
                        bp *= 1.1f;

                    // Glyph of Dark Succor
                    if (constAuraEffectPtr aurEff = m_caster->GetAuraEffect(101568, 0))
                        if (bp < int32(m_caster->CountPctFromMaxHealth(aurEff->GetAmount())))
                            if (m_caster->HasAura(48265) || m_caster->HasAura(48266)) // Only in frost/unholy presence
                                bp = m_caster->CountPctFromMaxHealth(aurEff->GetAmount());

                    m_caster->CastCustomSpell(m_caster, 45470, &bp, NULL, NULL, false);

                    return;
                }
                default:
                    break;
            }

            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            switch (m_spellInfo->Id)
            {
                case 119905:// Cauterize Master (Command Demon)
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        if (Pet* pet = m_caster->ToPlayer()->GetPet())
                        {
                            pet->CastSpell(unitTarget, 119899, true);
                            m_caster->ToPlayer()->AddSpellCooldown(119905, 0, time(NULL) + 30);
                        }

                    break;
                }
                case 119907:// Disarm (Command Demon)
                {
                    if (!m_caster->IsFriendlyTo(unitTarget))
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                            if (Pet* pet = m_caster->ToPlayer()->GetPet())
                            {
                                pet->CastSpell(unitTarget, damage, true);
                                m_caster->ToPlayer()->AddSpellCooldown(119907, 0, time(NULL) + 60);
                            }
                    break;
                }
                case 119910:// Spell Lock (Command Demon)
                {
                    if (!m_caster->IsFriendlyTo(unitTarget))
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                            if (Pet* pet = m_caster->ToPlayer()->GetPet())
                            {
                                pet->CastSpell(unitTarget, damage, true);
                                m_caster->ToPlayer()->AddSpellCooldown(119910, 0, time(NULL) + 24);
                            }
                    break;
                }
                case 119911:// Optical Blast (Command Demon)
                {
                    if (!m_caster->IsFriendlyTo(unitTarget))
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                            if (Pet* pet = m_caster->ToPlayer()->GetPet())
                            {
                                pet->CastSpell(unitTarget, damage, true);
                                m_caster->ToPlayer()->AddSpellCooldown(119911, 0, time(NULL) + 24);
                            }
                    break;
                }
                case 119909:// Whiplash (Command Demon)
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        if (targets.GetDstPos())
                            if (Pet* pet = m_caster->ToPlayer()->GetPet())
                            {
                                pet->CastSpell(targets.GetDstPos()->GetPositionX(), targets.GetDstPos()->GetPositionY(), targets.GetDstPos()->GetPositionZ(), damage, true);
                                m_caster->ToPlayer()->AddSpellCooldown(119909, 0, time(NULL) + 25);
                            }

                    break;
                }
                case 119913:// Fellash (Command Demon)
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        if (targets.GetDstPos())
                            if (Pet* pet = m_caster->ToPlayer()->GetPet())
                            {
                                pet->CastSpell(targets.GetDstPos()->GetPositionX(), targets.GetDstPos()->GetPositionY(), targets.GetDstPos()->GetPositionZ(), damage, true);
                                m_caster->ToPlayer()->AddSpellCooldown(119913, 0, time(NULL) + 25);
                            }

                    break;
                }
                default:
                    break;
            }

            break;
        }
        case SPELLFAMILY_MAGE:
        {
            switch (m_spellInfo->Id)
            {
                case 43987: // Conjure Refreshment Table
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        m_caster->ToPlayer()->RemoveSpellCooldown(120056, true); // Rank 1
                        m_caster->ToPlayer()->RemoveSpellCooldown(120055, true); // Rank 2
                        m_caster->ToPlayer()->RemoveSpellCooldown(120054, true); // Rank 3
                        m_caster->ToPlayer()->RemoveSpellCooldown(120053, true); // Rank 4

                        if (m_caster->getLevel() <= 70)
                            m_caster->CastSpell(m_caster, 120056, true);
                        else if (m_caster->getLevel() <= 80)
                            m_caster->CastSpell(m_caster, 120055, true);
                        else if (m_caster->getLevel() <= 85)
                            m_caster->CastSpell(m_caster, 120054, true);
                        else
                            m_caster->CastSpell(m_caster, 120053, true);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    switch (m_spellInfo->Id)
    {
        case 111397:// Bloody Fear
        {
            m_caster->DealDamage(m_caster, m_caster->CountPctFromMaxHealth(5), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            break;
        }
        case 122282:// Death Coil (Symbiosis)
        {
            if (m_caster->IsFriendlyTo(unitTarget))
            {
                int32 bp = (damage + m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.514f) * 3.5f;
                m_caster->CastCustomSpell(unitTarget, 47633, &bp, NULL, NULL, true);
            }
            else
            {
                int32 bp = damage + m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.514f;
                m_caster->CastCustomSpell(unitTarget, 47632, &bp, NULL, NULL, true);
            }
            return;
        }
        case 120165:// Conflagrate
        {
            UnitList friends;
            SkyMistCore::AnyFriendlyUnitInObjectRangeCheck u_check(m_caster, m_caster, 5.0f);
            SkyMistCore::UnitListSearcher<SkyMistCore::AnyFriendlyUnitInObjectRangeCheck> searcher(m_caster, friends, u_check);
            m_caster->VisitNearbyObject(5.0f, searcher);

            for (auto unit : friends)
            {
                if (m_caster->GetGUID() == unit->GetGUID())
                    continue;
                GetOriginalCaster()->CastSpell(unit, 120160, true);
                GetOriginalCaster()->CastSpell(unit, 120201, true);
            }

            break;
        }
        case 107045:// Jade Fire
            m_caster->CastSpell(unitTarget, 107098, false);
            break;
        case 106299:// Summon Living Air
        {
            TempSummon* enne = m_caster->SummonCreature(54631, m_caster->GetPositionX()+rand()%5, m_caster->GetPositionY()+2+rand()%5, m_caster->GetPositionZ()+1, 3.3f,TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
            enne->AddThreat(m_caster, 2000.f);
            break;
        }
        case 120202:// Gate of the Setting Sun | Boss 3 | Bombard
            spell_id = GetSpellInfo()->Effects[0].BasePoints;
            break;
    }

    // spells triggered by dummy effect should not miss
    if (spell_id)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell_id);

        if (!spellInfo)
            return;

        targets.SetUnitTarget(unitTarget);
        Spell* spell = new Spell(m_caster, spellInfo, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE, m_originalCasterGUID, true);
        if (bp) spell->SetSpellValue(SPELLVALUE_BASE_POINT0, bp);
        spell->prepare(&targets);
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr->GetPetAura(m_spellInfo->Id, effIndex))
    {
        m_caster->AddPetAura(petSpell);
        return;
    }

    // normal DB scripted effect
    m_caster->GetMap()->ScriptsStart(sSpellScripts, uint32(m_spellInfo->Id | (effIndex << 24)), m_caster, unitTarget);

    // Script based implementation. Must be used only for not good for implementation in core spell effects
    // So called only for not proccessed cases
    if (gameObjTarget)
        sScriptMgr->OnDummyEffect(m_caster, m_spellInfo->Id, effIndex, gameObjTarget);
    else if (unitTarget && unitTarget->GetTypeId() == TYPEID_UNIT)
        sScriptMgr->OnDummyEffect(m_caster, m_spellInfo->Id, effIndex, unitTarget->ToCreature());
    else if (itemTarget)
        sScriptMgr->OnDummyEffect(m_caster, m_spellInfo->Id, effIndex, itemTarget);
}

void Spell::EffectTriggerSpell(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH_TARGET
        && effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH)
        return;

    uint32 triggered_spell_id = m_spellInfo->Effects[effIndex].TriggerSpell;

    // todo: move those to spell scripts
    if (m_spellInfo->Effects[effIndex].Effect == SPELL_EFFECT_TRIGGER_SPELL
        && effectHandleMode == SPELL_EFFECT_HANDLE_LAUNCH_TARGET)
    {
        // special cases
        switch (triggered_spell_id)
        {
            // Vanish (not exist)
            case 131369:
            {
                unitTarget->RemoveMovementImpairingAuras();
                unitTarget->RemoveAurasByType(SPELL_AURA_MOD_STALKED);

                // If this spell is given to an NPC, it must handle the rest using its own AI
                if (unitTarget->GetTypeId() != TYPEID_PLAYER)
                    return;

                // See if we already are stealthed. If so, we're done.
                if (unitTarget->HasAura(1784) || unitTarget->HasAura(115191))
                    return;

                // Reset cooldown on stealth if needed
                if (unitTarget->ToPlayer()->HasSpellCooldown(1784))
                    unitTarget->ToPlayer()->RemoveSpellCooldown(1784, true);
                if (unitTarget->ToPlayer()->HasSpellCooldown(115191))
                    unitTarget->ToPlayer()->RemoveSpellCooldown(115191, true);
                
                if (!unitTarget->HasAura(108208))
                    unitTarget->CastSpell(unitTarget, 1784, true);
                else
                    unitTarget->CastSpell(unitTarget, 115191, true);
                return;
            }  
            // Demonic Empowerment -- succubus
            case 54437:
            {
                unitTarget->RemoveMovementImpairingAuras();
                unitTarget->RemoveAurasByType(SPELL_AURA_MOD_STALKED);
                unitTarget->RemoveAurasByType(SPELL_AURA_MOD_STUN);

                // Cast Lesser Invisibility
                unitTarget->CastSpell(unitTarget, 7870, true);
                return;
            }
            // Brittle Armor - (need add max stack of 24575 Brittle Armor)
            case 29284:
            {
                // Brittle Armor
                SpellInfo const* spell = sSpellMgr->GetSpellInfo(24575);
                if (!spell)
                    return;

                for (uint32 j = 0; j < spell->StackAmount; ++j)
                    m_caster->CastSpell(unitTarget, spell->Id, true);
                return;
            }
            // Mercurial Shield - (need add max stack of 26464 Mercurial Shield)
            case 29286:
            {
                // Mercurial Shield
                SpellInfo const* spell = sSpellMgr->GetSpellInfo(26464);
                if (!spell)
                    return;

                for (uint32 j = 0; j < spell->StackAmount; ++j)
                    m_caster->CastSpell(unitTarget, spell->Id, true);
                return;
            }
            // Righteous Defense
            case 31980:
            {
                m_caster->CastSpell(unitTarget, 31790, true);
                return;
            }
            // Cloak of Shadows
            case 35729:
            {
                uint32 dispelMask = SpellInfo::GetDispelMask(DISPEL_ALL);
                Unit::AuraApplicationMap& Auras = unitTarget->GetAppliedAuras();
                for (Unit::AuraApplicationMap::iterator iter = Auras.begin(); iter != Auras.end();)
                {
                    // remove all harmful spells on you...
                    SpellInfo const* spell = iter->second->GetBase()->GetSpellInfo();
                    if ((spell->DmgClass == SPELL_DAMAGE_CLASS_MAGIC // only affect magic spells
                        || (spell->GetDispelMask() & dispelMask) || (spell->GetSchoolMask() & SPELL_SCHOOL_MASK_MAGIC))
                        // ignore positive and passive auras
                        && !iter->second->IsPositive() && !iter->second->GetBase()->IsPassive() && m_spellInfo->CanDispelAura(spell))
                    {
                        m_caster->RemoveAura(iter);
                    }
                    else
                        ++iter;
                }
                return;
            }
        }
    }

    // normal case
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(triggered_spell_id);
    if (!spellInfo)
        return;

    SpellCastTargets targets;
    if (effectHandleMode == SPELL_EFFECT_HANDLE_LAUNCH_TARGET)
    {
        if (!spellInfo->NeedsToBeTriggeredByCaster())
            return;
        targets.SetUnitTarget(unitTarget);
    }
    else //if (effectHandleMode == SPELL_EFFECT_HANDLE_LAUNCH)
    {
        if (spellInfo->NeedsToBeTriggeredByCaster() && (m_spellInfo->Effects[effIndex].GetProvidedTargetMask() & TARGET_FLAG_UNIT_MASK))
            return;

        if (spellInfo->GetExplicitTargetMask() & TARGET_FLAG_DEST_LOCATION)
            targets.SetDst(m_targets);

        targets.SetUnitTarget(m_targets.GetUnitTarget());
    }

    CustomSpellValues values;
    // set basepoints for trigger with value effect
    if (m_spellInfo->Effects[effIndex].Effect == SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE)
    {
        // maybe need to set value only when basepoints == 0?
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, damage);
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, damage);
        values.AddSpellMod(SPELLVALUE_BASE_POINT2, damage);
    }

    // Remove spell cooldown (not category) if spell triggering spell with cooldown and same category
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->CategoryRecoveryTime && spellInfo->CategoryRecoveryTime
        && m_spellInfo->Category == spellInfo->Category)
        m_caster->ToPlayer()->RemoveSpellCooldown(spellInfo->Id);

    // original caster guid only for GO cast
    m_caster->CastSpell(targets, spellInfo, &values, TRIGGERED_FULL_MASK, NULL, NULLAURA_EFFECT, m_originalCasterGUID);
}

void Spell::EffectTriggerMissileSpell(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET
        && effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    uint32 triggered_spell_id = m_spellInfo->Effects[effIndex].TriggerSpell;

    // normal case
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(triggered_spell_id);
    if (!spellInfo)
        return;

    SpellCastTargets targets;
    if (effectHandleMode == SPELL_EFFECT_HANDLE_HIT_TARGET)
    {
        if (!spellInfo->NeedsToBeTriggeredByCaster())
            return;
        targets.SetUnitTarget(unitTarget);
    }
    else //if (effectHandleMode == SPELL_EFFECT_HANDLE_HIT)
    {
        if (spellInfo->NeedsToBeTriggeredByCaster() && (m_spellInfo->Effects[effIndex].GetProvidedTargetMask() & TARGET_FLAG_UNIT_MASK))
            return;

        if (spellInfo->GetExplicitTargetMask() & TARGET_FLAG_DEST_LOCATION)
            targets.SetDst(m_targets);

        targets.SetUnitTarget(m_caster);
    }

    CustomSpellValues values;
    // set basepoints for trigger with value effect
    if (m_spellInfo->Effects[effIndex].Effect == SPELL_EFFECT_TRIGGER_MISSILE_SPELL_WITH_VALUE)
    {
        // maybe need to set value only when basepoints == 0?
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, damage);
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, damage);
        values.AddSpellMod(SPELLVALUE_BASE_POINT2, damage);
    }

    // Remove spell cooldown (not category) if spell triggering spell with cooldown and same category
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->CategoryRecoveryTime && spellInfo->CategoryRecoveryTime
        && m_spellInfo->Category == spellInfo->Category)
        m_caster->ToPlayer()->RemoveSpellCooldown(spellInfo->Id);

    // original caster guid only for GO cast
    m_caster->CastSpell(targets, spellInfo, &values, TRIGGERED_FULL_MASK, NULL, NULLAURA_EFFECT, m_originalCasterGUID);
}

void Spell::EffectForceCast(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    switch (m_spellInfo->Id)
    {
        case 66548: // teleports outside (Isle of Conquest)
        case 66549: // teleports inside (Isle of Conquest)
        {
            if (Creature* teleportTarget = m_caster->FindNearestCreature((m_spellInfo->Id == 66548 ? 23472 : 22515), 60.0f, true))
            {
                float x, y, z, o;
                teleportTarget->GetPosition(x, y, z, o);

                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    m_caster->ToPlayer()->TeleportTo(628, x, y, z, o);
            }
            break;
        }
    }

    if (!unitTarget)
        return;

    uint32 triggered_spell_id = m_spellInfo->Effects[effIndex].TriggerSpell;

    // normal case
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!spellInfo)
        return;

    if (m_spellInfo->Effects[effIndex].Effect == SPELL_EFFECT_FORCE_CAST && damage)
    {
        switch (m_spellInfo->Id)
        {
            case 52588: // Skeletal Gryphon Escape
            case 48598: // Ride Flamebringer Cue
                unitTarget->RemoveAura(damage);
                break;
            case 52463: // Hide In Mine Car
            case 52349: // Overtake
                unitTarget->CastCustomSpell(unitTarget, spellInfo->Id, &damage, NULL, NULL, true, NULL, NULLAURA_EFFECT, m_originalCasterGUID);
                return;
        }
    }

    CustomSpellValues values;
    // set basepoints for trigger with value effect
    if (m_spellInfo->Effects[effIndex].Effect == SPELL_EFFECT_FORCE_CAST_WITH_VALUE)
    {
        // maybe need to set value only when basepoints == 0?
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, damage);
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, damage);
        values.AddSpellMod(SPELLVALUE_BASE_POINT2, damage);
    }

    SpellCastTargets targets;
    targets.SetUnitTarget(m_caster);

    unitTarget->CastSpell(targets, spellInfo, &values, TRIGGERED_FULL_MASK);
}

void Spell::EffectTriggerRitualOfSummoning(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    uint32 triggered_spell_id = m_spellInfo->Effects[effIndex].TriggerSpell;
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!spellInfo)
        return;

    finish();

    m_caster->CastSpell((Unit*)NULL, spellInfo, false);
}

void Spell::EffectJump(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH_TARGET)
        return;

    if (m_caster->isInFlight())
        return;

    if (!unitTarget)
        return;

    float x, y, z;
    unitTarget->GetContactPoint(m_caster, x, y, z, CONTACT_DISTANCE);

    float speedXY, speedZ;
    CalculateJumpSpeeds(effIndex, m_caster->GetExactDist2d(x, y), speedXY, speedZ);
    m_caster->GetMotionMaster()->MoveJump(x, y, z, speedXY, speedZ);
}

void Spell::EffectJumpDest(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH)
        return;

    if (m_caster->isInFlight())
        return;

    if (!m_targets.HasDst())
        return;

    // Init dest coordinates
    float x, y, z;
    destTarget->GetPosition(x, y, z);

    float speedXY, speedZ;
    CalculateJumpSpeeds(effIndex, m_caster->GetExactDist2d(x, y), speedXY, speedZ);

    switch (m_spellInfo->Id)
    {
        case 49575: // Death Grip 
        case 92832: // Leap of Faith
        case 110726: // Leap of Faith
            m_caster->GetMotionMaster()->CustomJump(x, y, z, speedXY, speedZ);
            break;
        case 49376:
            m_caster->GetMotionMaster()->MoveJump(x, y, z, speedXY, speedZ, destTarget->GetOrientation());
            break;
        default:
            m_caster->GetMotionMaster()->MoveJump(x, y, z, speedXY, speedZ);
    }
}

void Spell::CalculateJumpSpeeds(uint8 i, float dist, float & speedXY, float & speedZ)
{
    if (m_spellInfo->Effects[i].MiscValue)
        speedZ = float(m_spellInfo->Effects[i].MiscValue)/10;
    else if (m_spellInfo->Effects[i].MiscValueB)
        speedZ = float(m_spellInfo->Effects[i].MiscValueB)/10;
    else
        speedZ = 10.0f;

    speedXY = dist * 10.0f / speedZ;

    if (m_spellInfo->Id == 49575)
        speedXY = 38;
}

void Spell::EffectTeleportUnits(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->isInFlight())
        return;

    // Pre effects
    uint8 uiMaxSafeLevel = 0;
    switch (m_spellInfo->Id)
    {
        case 48129:  // Scroll of Recall
            uiMaxSafeLevel = 40;
        case 60320:  // Scroll of Recall II
            if (!uiMaxSafeLevel)
                uiMaxSafeLevel = 70;
        case 60321:  // Scroll of Recal III
            if (!uiMaxSafeLevel)
                uiMaxSafeLevel = 80;

            if (unitTarget->getLevel() > uiMaxSafeLevel)
            {
                unitTarget->AddAura(60444, unitTarget); //Apply Lost! Aura

                // ALLIANCE from 60323 to 60330 - HORDE from 60328 to 60335

                uint32 spellId = 60323;
                if (m_caster->ToPlayer()->GetTeam() == HORDE)
                    spellId += 5;
                spellId += urand(0, 7);
                m_caster->CastSpell(m_caster, spellId, true);
                return;
            }
            break;
        case 66550: // teleport outside (Isle of Conquest)
            if (Player* target = unitTarget->ToPlayer())
            {
                if (target->GetTeamId() == TEAM_ALLIANCE)
                    m_targets.SetDst(442.24f, -835.25f, 44.30f, 0.06f, 628);
                else
                    m_targets.SetDst(1120.43f, -762.11f, 47.92f, 2.94f, 628);
            }
            break;
        case 66551: // teleport inside (Isle of Conquest)
            if (Player* target = unitTarget->ToPlayer())
            {
                if (target->GetTeamId() == TEAM_ALLIANCE)
                    m_targets.SetDst(389.57f, -832.38f, 48.65f, 3.00f, 628);
                else
                    m_targets.SetDst(1174.85f, -763.24f, 48.72f, 6.26f, 628);
            }
            break;
    }

    // If not exist data for dest location - return
    if (!m_targets.HasDst())
        return;

    // Init dest coordinates
    uint32 mapid = destTarget->GetMapId();
    if (mapid == MAPID_INVALID)
        mapid = unitTarget->GetMapId();
    float x, y, z, orientation;
    destTarget->GetPosition(x, y, z, orientation);
    if (!orientation && m_targets.GetUnitTarget())
        orientation = m_targets.GetUnitTarget()->GetOrientation();

    if (m_spellInfo->Id == 130393) // Blink Strikes
    {
        m_caster->NearTeleportTo(x, y, z, orientation);
    }
    else
    {
        if (mapid == unitTarget->GetMapId())
            unitTarget->NearTeleportTo(x, y, z, orientation, unitTarget == m_caster);
        else if (unitTarget->GetTypeId() == TYPEID_PLAYER)
            unitTarget->ToPlayer()->TeleportTo(mapid, x, y, z, orientation, unitTarget == m_caster ? TELE_TO_SPELL : 0);
    }
    // post effects for TARGET_DEST_DB
    switch (m_spellInfo->Id)
    {
        // Dimensional Ripper - Everlook
        case 23442:
        {
            int32 r = irand(0, 119);
            if (r >= 70)                                  // 7/12 success
            {
                if (r < 100)                              // 4/12 evil twin
                    m_caster->CastSpell(m_caster, 23445, true);
                else                                        // 1/12 fire
                    m_caster->CastSpell(m_caster, 23449, true);
            }
            return;
        }
        // Ultra safe Transporter: Toshley's Station
        case 36941:
        {
            if (roll_chance_i(50))                        // 50% success
            {
                int32 rand_eff = urand(1, 7);
                switch (rand_eff)
                {
                    case 1:
                        // soul split - evil
                        m_caster->CastSpell(m_caster, 36900, true);
                        break;
                    case 2:
                        // soul split - good
                        m_caster->CastSpell(m_caster, 36901, true);
                        break;
                    case 3:
                        // Increase the size
                        m_caster->CastSpell(m_caster, 36895, true);
                        break;
                    case 4:
                        // Decrease the size
                        m_caster->CastSpell(m_caster, 36893, true);
                        break;
                    case 5:
                    // Transform
                    {
                        if (m_caster->ToPlayer()->GetTeam() == ALLIANCE)
                            m_caster->CastSpell(m_caster, 36897, true);
                        else
                            m_caster->CastSpell(m_caster, 36899, true);
                        break;
                    }
                    case 6:
                        // chicken
                        m_caster->CastSpell(m_caster, 36940, true);
                        break;
                    case 7:
                        // evil twin
                        m_caster->CastSpell(m_caster, 23445, true);
                        break;
                }
            }
            return;
        }
        // Dimensional Ripper - Area 52
        case 36890:
        {
            if (roll_chance_i(50))                        // 50% success
            {
                int32 rand_eff = urand(1, 4);
                switch (rand_eff)
                {
                    case 1:
                        // soul split - evil
                        m_caster->CastSpell(m_caster, 36900, true);
                        break;
                    case 2:
                        // soul split - good
                        m_caster->CastSpell(m_caster, 36901, true);
                        break;
                    case 3:
                        // Increase the size
                        m_caster->CastSpell(m_caster, 36895, true);
                        break;
                    case 4:
                        // Transform
                    {
                        if (m_caster->ToPlayer()->GetTeam() == ALLIANCE)
                            m_caster->CastSpell(m_caster, 36897, true);
                        else
                            m_caster->CastSpell(m_caster, 36899, true);
                        break;
                    }
                }
            }
            return;
        }
    }
}

void Spell::EffectApplyAura(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!m_spellAura || !unitTarget)
        return;

    ASSERT(unitTarget == m_spellAura->GetOwner());

    // Divine Aegis - 47753,  Mastery: Illuminated Healing - 76669, Spirit Shell - 114908, Anti-Magic Shell - 48707 must ignore Battle Fatigue
    if (m_spellAura->GetId() == 47753 || m_spellAura->GetId() == 86273 || m_spellAura->GetId() == 114908 || m_spellAura->GetId() != 48707)
    {
        for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            if (m_spellAura->GetEffect(i) && m_spellAura->GetEffect(i)->GetAuraType() == SPELL_AURA_SCHOOL_ABSORB)
            {
                float AbsorbMod2 = 0.0f;

                float minval = (float)unitTarget->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_ABSORPTION_PCT);
                float maxval = (float)unitTarget->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_ABSORPTION_PCT);

                AbsorbMod2 = minval + maxval;

                int currentValue = m_spellAura->GetEffect(i)->GetAmount();
                AddPct(currentValue, AbsorbMod2);
                m_spellAura->GetEffect(i)->SetAmount(currentValue);
            }
        }
    }

    m_spellAura->_ApplyEffectForTargets(effIndex);
}

void Spell::EffectApplyAreaAura(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!m_spellAura || !unitTarget)
        return;
    ASSERT (unitTarget == m_spellAura->GetOwner());
    m_spellAura->_ApplyEffectForTargets(effIndex);
}

void Spell::EffectUnlearnSpecialization(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();
    uint32 spellToUnlearn = m_spellInfo->Effects[effIndex].TriggerSpell;

    player->removeSpell(spellToUnlearn);
}

void Spell::EffectPowerDrain(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_spellInfo->Effects[effIndex].MiscValue < 0 || m_spellInfo->Effects[effIndex].MiscValue >= int8(MAX_POWERS))
        return;

    Powers powerType = Powers(m_spellInfo->Effects[effIndex].MiscValue);

    if (!unitTarget || !unitTarget->isAlive() || unitTarget->getPowerType() != powerType || damage < 0)
        return;

    // add spell damage bonus
    damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);
    damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);

    int32 newDamage = -(unitTarget->ModifyPower(powerType, -damage));

    float gainMultiplier = 0.0f;

    // Don't restore from self drain
    if (m_caster != unitTarget)
    {
        gainMultiplier = m_spellInfo->Effects[effIndex].CalcValueMultiplier(m_originalCaster, this);

        int32 gain = int32(newDamage* gainMultiplier);

        m_caster->EnergizeBySpell(m_caster, m_spellInfo->Id, gain, powerType);
    }
    ExecuteLogEffectTakeTargetPower(effIndex, unitTarget, powerType, newDamage, gainMultiplier);
}

void Spell::EffectSendEvent(SpellEffIndex effIndex)
{
    // we do not handle a flag dropping or clicking on flag in battleground by sendevent system
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET
        && effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    WorldObject* target = NULL;

    // call events for object target if present
    if (effectHandleMode == SPELL_EFFECT_HANDLE_HIT_TARGET)
    {
        if (unitTarget)
            target = unitTarget;
        else if (gameObjTarget)
            target = gameObjTarget;
    }
    else // if (effectHandleMode == SPELL_EFFECT_HANDLE_HIT)
    {
        // let's prevent executing effect handler twice in case when spell effect is capable of targeting an object
        // this check was requested by scripters, but it has some downsides:
        // now it's impossible to script (using sEventScripts) a cast which misses all targets
        // or to have an ability to script the moment spell hits dest (in a case when there are object targets present)
        if (m_spellInfo->Effects[effIndex].GetProvidedTargetMask() & (TARGET_FLAG_UNIT_MASK | TARGET_FLAG_GAMEOBJECT_MASK))
            return;
        // some spells have no target entries in dbc and they use focus target
        if (focusObject)
            target = focusObject;
        // TODO: there should be a possibility to pass dest target to event script
    }

    if (ZoneScript* zoneScript = m_caster->GetZoneScript())
        zoneScript->ProcessEvent(target, m_spellInfo->Effects[effIndex].MiscValue);
    else if (InstanceScript* instanceScript = m_caster->GetInstanceScript())    // needed in case Player is the caster
        instanceScript->ProcessEvent(target, m_spellInfo->Effects[effIndex].MiscValue);

    m_caster->GetMap()->ScriptsStart(sEventScripts, m_spellInfo->Effects[effIndex].MiscValue, m_caster, target);
}

void Spell::EffectPowerBurn(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_spellInfo->Effects[effIndex].MiscValue < 0 || m_spellInfo->Effects[effIndex].MiscValue >= int8(MAX_POWERS))
        return;

    Powers powerType = Powers(m_spellInfo->Effects[effIndex].MiscValue);

    if (!unitTarget || !unitTarget->isAlive() || unitTarget->getPowerType() != powerType || damage < 0)
        return;

    // burn x% of target's mana, up to maximum of 2x% of caster's mana (Mana Burn)
    if (m_spellInfo->Id == 8129)
    {
        switch (unitTarget->GetEntry())
        {
            case 52498: // Beth'tilac
            case 52530: // Alysrazor
            case 45870: // Anshal
            case 45871: // Nezir
            case 45872: // Rohash
                return;
        }

        int32 maxDamage = int32(CalculatePct(m_caster->GetMaxPower(powerType), damage * 2));
        damage = int32(CalculatePct(unitTarget->GetMaxPower(powerType), damage));
        damage = std::min(damage, maxDamage);
    }

    int32 newDamage = -(unitTarget->ModifyPower(powerType, -damage));

    // NO - Not a typo - EffectPowerBurn uses effect value multiplier - not effect damage multiplier
    float dmgMultiplier = m_spellInfo->Effects[effIndex].CalcValueMultiplier(m_originalCaster, this);

    // add log data before multiplication (need power amount, not damage)
    ExecuteLogEffectTakeTargetPower(effIndex, unitTarget, powerType, newDamage, 0.0f);

    newDamage = int32(newDamage* dmgMultiplier);

    m_damage += newDamage;
}

void Spell::EffectHeal(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH_TARGET)
        return;

    if (unitTarget && unitTarget->isAlive() && damage >= 0)
    {
        // Try to get original caster
        Unit* caster = m_originalCasterGUID ? m_originalCaster : m_caster;

        // Skip if m_originalCaster not available
        if (!caster)
            return;

        int32 addhealth = damage;

        switch (m_spellInfo->Id)
        {
            case 105996: // Essence of Dreams, Ultraxion, Dragon Soul
            {
                uint32 count = 0;
                for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                    if (ihit->effectMask & (1<<effIndex))
                        ++count;

                addhealth /= count; 
                break;
            }
            // Tipping of the Scales, Scales of Life
            case 96880:
            {
                if (constAuraEffectPtr aurEff = m_caster->GetAuraEffect(96881, EFFECT_0))
                {
                    addhealth = aurEff->GetAmount();
                    m_caster->RemoveAurasDueToSpell(96881);
                }
                else
                    return;
                break;
            }
            case 45064: // Vessel of the Naaru (Vial of the Sunwell trinket)
            {
                if (!caster)
                    break;

                // Amount of heal - depends from stacked Holy Energy
                int damageAmount = 0;
                if (constAuraEffectPtr aurEff = caster->GetAuraEffect(45062, 0))
                {
                    damageAmount+= aurEff->GetAmount();
                    caster->RemoveAurasDueToSpell(45062);
                }

                addhealth += damageAmount;
                addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);

                break;
            }
            case 48743: // Death Pact - return pct of max health to caster
            {
                if (!caster)
                    break;

                addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, int32(caster->CountPctFromMaxHealth(damage)), HEAL);

                // PvP Power effect for Death Pact, twice because we calculate pet + dk pvp power
                float PvPPower = 1 + (caster->GetFloatValue(PLAYER_FIELD_PVP_POWER_HEALING) / 100);
                addhealth = int32((addhealth * PvPPower) * PvPPower);
                break;
            }
            case 67489: // Runic Healing Injector (heal increased by 25% for engineers - 3.2.0 patch change)
                if (!caster)
                    break;

                if (Player* player = caster->ToPlayer())
                    if (player->HasSkill(SKILL_ENGINEERING))
                        AddPct(addhealth, 25);
                break;
            case 85222: // Light of Dawn
                addhealth *= GetPowerCost();

                if (!caster)
                    break;

                if (caster->HasAura(54940))
                    AddPct(addhealth, 25);

                addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);

                break;
            case 86961: // Cleansing Waters
            {
                addhealth = m_caster->CountPctFromMaxHealth(4);
                break;
            }
            case 90361: // Spirit Mend
            {
                if (!unitTarget || !caster)
                    return;

                if (!caster->GetOwner())
                    return;

                Player* m_owner = caster->GetOwner()->ToPlayer();
                if (!m_owner)
                    return;

                addhealth += int32(m_owner->GetTotalAttackPowerValue(RANGED_ATTACK) * 0.35f * 0.5f);

                break;
            }
            case 114163:// Eternal Flame
            case 130551:// Word of Glory
            {
                if (!caster || !unitTarget)
                    return;

                if (caster->GetTypeId() != TYPEID_PLAYER)
                    return;

                addhealth += int32(0.49f * m_caster->SpellBaseDamageBonusDone(SpellSchoolMask(m_spellInfo->SchoolMask)));

                int32 holyPower = caster->GetPower(POWER_HOLY_POWER) + 1;

                if (holyPower > 3)
                    holyPower = 3;

                // Divine Purpose
                if (caster->HasAura(90174))
                    holyPower = 3;

                addhealth *= holyPower;

                // Bastion of Glory : +10% of power per application if target is caster
                if (unitTarget->GetGUID() == caster->GetGUID() && caster->HasAura(114637))
                {
                    AuraPtr bastionOfGlory = caster->GetAura(114637);
                    if (!bastionOfGlory)
                        break;

                    AddPct(addhealth, (10 * bastionOfGlory->GetStackAmount()));

                    caster->RemoveAurasDueToSpell(114637);
                }

                addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);

                if (caster->HasAura(86172) && roll_chance_i(25))
                    caster->AddAura(90174, caster);

                break;
            }
            case 19750: // Flash heal bastion of glory
            {
                if (!caster || !unitTarget)
                    break;

                if (caster->HasAura(114637) && caster->HasAura(114250) && unitTarget->GetGUID() == caster->GetGUID())
                {
                    int32 amount = caster->GetAura(114637)->GetStackAmount() * 20;
                    AddPct(addhealth, (amount * caster->GetAura(114250)->GetStackAmount()));
                    caster->RemoveAurasDueToSpell(114637);
                    caster->RemoveAurasDueToSpell(114250);
                }

                addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);
                break;
            }
            case 115072:// Expel Harm
            case 147489:// Expel Harm with glyph of Targeted Expulsion
            {
                if (caster && caster->getClass() == CLASS_MONK && addhealth && (m_spellInfo->Id == 115072 || m_spellInfo->Id == 147489))
                {
                    addhealth = Spell::CalculateMonkMeleeAttacks(m_caster, 7, 14);
                    addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);
                }

                break;
            }
            case 121129:// Daybreak
            {
                uint32 count = 0;
                for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                    if (ihit->effectMask & (1 << effIndex))
                        ++count;

                count--; // Remove main target

                if (count > 0)
                    addhealth /= count;

                addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);

                break;
            }
            default:
                if (!caster)
                    break;

                addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);
                break;
        }

        addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL);

        // Remove Grievous bite if fully healed
        if (unitTarget->HasAura(48920) && (unitTarget->GetHealth() + addhealth >= unitTarget->GetMaxHealth()))
            unitTarget->RemoveAura(48920);

        // Custom MoP Script
        // 77495 - Mastery : Harmony
        if (caster && caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_DRUID && m_spellInfo->Id != 81269 && m_spellInfo->Id != 33778 && m_spellInfo->Id != 145109)
        {
            if (caster->HasAura(77495))
            {
                if (addhealth)
                {
                    float Mastery = caster->GetFloatValue(PLAYER_MASTERY) * 1.25 / 100.0f;

                    if (m_spellInfo->HasEffect(SPELL_EFFECT_HEAL))
                    {
                        addhealth *= (1 + Mastery);

                        int32 bp = int32(100.0f * Mastery);

                        caster->CastCustomSpell(caster, 100977, &bp, NULL, NULL, true);
                    }
                }
            }
        }
        // 77226 - Mastery : Deep Healing
        if (caster && caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_SHAMAN)
        {
            if (caster->HasAura(77226))
            {
                if (addhealth)
                {
                    float Mastery = caster->GetFloatValue(PLAYER_MASTERY) * 3.0f / 100.0f;
                    float healthpct = unitTarget->GetHealthPct();

                    float bonus = 0;
                    bonus = CalculatePct((1 + (100.0f - healthpct)), Mastery);

                    addhealth *= 1 + bonus;
                }
            }
        }
        // Chakra : Serenity - 81208
        if (caster && addhealth && m_spellInfo->Effects[0].TargetA.GetTarget() == TARGET_UNIT_TARGET_ALLY && caster->HasAura(81208)) // Single heal target
            if (AuraPtr renew = unitTarget->GetAura(139, caster->GetGUID()))
                renew->RefreshDuration();

        // Mogu'Shan Vault
        if (caster && (caster->HasAura(116161) || unitTarget->HasAura(116161))) // SPELL_CROSSED_OVER
        {
            // http://fr.wowhead.com/spell=117549#english-comments
            // uint32 targetSpec = unitTarget->ToPlayer()->GetSpecializationId(unitTarget->ToPlayer()->GetActiveSpec());
            uint32 innervationId = 0;

            if (unitTarget == caster)
            {
                int32 bp1 = addhealth/2;
                int32 bp2 = 15;

                caster->CastCustomSpell(unitTarget, 117543, &bp1, &bp2, NULL, NULL, NULL, NULL, true); // Mana regen bonus
            }
            else
            {
                int32 bp1 = 10;
                int32 bp2 = 15;
                int32 bp3 = 20;
                int32 bp4 = 25;
                int32 bp5 = 30;
                int32 bp6 = 35;

                caster->CastCustomSpell(unitTarget, 117549, &bp1, &bp2, &bp3, &bp4, &bp5, &bp6, true);
            }

            if (unitTarget->GetHealth() + addhealth >= unitTarget->GetMaxHealth())
                unitTarget->CastSpell(unitTarget, 120717, true);  // Revitalized Spirit
        }

        if (m_spellInfo->Id == 27827)
            addhealth = m_caster->GetMaxHealth();

        m_damage -= addhealth;
    }
}

void Spell::EffectHealPct(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive() || damage < 0)
        return;

    // Skip if m_originalCaster not available
    if (!m_originalCaster)
        return;

    switch (m_spellInfo->Id)
    {
        case 6262:  // Healthstone
            if (m_caster->HasAura(56224)) // Glyph of Healthstone
                return;
            break;
        case 59754: // Rune Tap - Party
            if (unitTarget == m_caster)
                return;
            break;
        case 53353: // Chimera Shot - Heal
            if (m_caster->HasAura(119447)) // Glyph of Chimera Shot
                damage += 2;
            break;
        case 115450: // Detox
            if (!m_caster->HasAura(146954))
                return;
            break;
        case 118779: // Victory rush heal
            // Glyph of victory rush
            if (m_caster->HasAura(58382))
                damage += 50;
            break;
        case 118340:// Impending Victory - Heal
            // Victorious State causes your next Impending Victory to heal for 20% of your maximum health.
            if (m_caster->HasAura(32216))
            {
                damage = 20;
                m_caster->RemoveAurasDueToSpell(32216);
            }
            break;
        case 137562:// Nimble Brew
            if (!m_caster->HasAura(146952)) // Glyph of Nimble Brew
                return;
            break;
        default:
            break;
    }

    uint32 heal = m_originalCaster->SpellHealingBonusDone(unitTarget, m_spellInfo, unitTarget->CountPctFromMaxHealth(damage), HEAL);
    heal = unitTarget->SpellHealingBonusTaken(m_originalCaster, m_spellInfo, heal, HEAL);

    // Unholy Strength with pvp power
    if (m_spellInfo->Id == 53365)
    {
        float PvPPower = 1 + (m_originalCaster->GetFloatValue(PLAYER_FIELD_PVP_POWER_HEALING) / 100);
        heal = int32(heal * PvPPower);
    }

    m_healing += heal;
}

void Spell::EffectHealMechanical(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive() || damage < 0)
        return;

    // Skip if m_originalCaster not available
    if (!m_originalCaster)
        return;

    uint32 heal = m_originalCaster->SpellHealingBonusDone(unitTarget, m_spellInfo, uint32(damage), HEAL);

    m_healing += unitTarget->SpellHealingBonusTaken(m_originalCaster, m_spellInfo, heal, HEAL);
}

void Spell::EffectHealthLeech(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive() || damage < 0)
        return;

    switch (m_spellInfo->Id)
    {
        // Siphon Vitality, Warmaster Blackhorn, Dragon Soul
        case 110312:
            damage = unitTarget->CountPctFromCurHealth(20);
            break;
        // Drain Life, Item - Dragon Soul - Proc - Str Tank Sword, Souldrinker
        case 108022:
        case 109828:
        case 109831:
            damage = int32(0.001f * damage * m_caster->GetMaxHealth());
            break;  
        default:
            break;
    }

    damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);
    damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);

    float healMultiplier = m_spellInfo->Effects[effIndex].CalcValueMultiplier(m_originalCaster, this);

    m_damage += damage;
    // get max possible damage, don't count overkill for heal
    uint32 healthGain = uint32(-unitTarget->GetHealthGain(-damage) * healMultiplier);

    if (m_caster->isAlive())
    {
        healthGain = m_caster->SpellHealingBonusDone(m_caster, m_spellInfo, healthGain, HEAL);
        healthGain = m_caster->SpellHealingBonusTaken(m_caster, m_spellInfo, healthGain, HEAL);

        m_caster->HealBySpell(m_caster, m_spellInfo, uint32(healthGain));
    }
}

void Spell::DoCreateItem(uint32 /*i*/, uint32 itemtype)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();

    uint32 newitemid = itemtype;
    ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(newitemid);
    if (!pProto)
    {
        player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    // bg reward have some special in code work
    uint32 bgType = 0;
    switch (m_spellInfo->Id)
    {
        case SPELL_AV_MARK_WINNER:
        case SPELL_AV_MARK_LOSER:
            bgType = BATTLEGROUND_AV;
            break;
        case SPELL_WS_MARK_WINNER:
        case SPELL_WS_MARK_LOSER:
            bgType = BATTLEGROUND_WS;
            break;
        case SPELL_AB_MARK_WINNER:
        case SPELL_AB_MARK_LOSER:
            bgType = BATTLEGROUND_AB;
            break;
        default:
            break;
    }

    uint32 num_to_add = damage;

    if (num_to_add < 1)
        num_to_add = 1;
    if (num_to_add > pProto->GetMaxStackSize())
        num_to_add = pProto->GetMaxStackSize();

    // init items_count to 1, since 1 item will be created regardless of specialization
    int items_count=1;
    // the chance to create additional items
    float additionalCreateChance=0.0f;
    // the maximum number of created additional items
    uint8 additionalMaxNum=0;
    // get the chance and maximum number for creating extra items
    if (canCreateExtraItems(player, m_spellInfo->Id, additionalCreateChance, additionalMaxNum))
    {
        // roll with this chance till we roll not to create or we create the max num
        while (roll_chance_f(additionalCreateChance) && items_count <= additionalMaxNum)
            ++items_count;
    }

    // really will be created more items
    num_to_add *= items_count;

    // can the player store the new item?
    ItemPosCountVec dest;
    uint32 no_space = 0;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, newitemid, num_to_add, &no_space);
    if (msg != EQUIP_ERR_OK)
    {
        // convert to possible store amount
        if (msg == EQUIP_ERR_INV_FULL || msg == EQUIP_ERR_ITEM_MAX_COUNT)
            num_to_add -= no_space;
        else
        {
            // if not created by another reason from full inventory or unique items amount limitation
            player->SendEquipError(msg, NULL, NULL, newitemid);
            return;
        }
    }

    if (num_to_add)
    {
        // create the new item and store it
        Item* pItem = player->StoreNewItem(dest, newitemid, true, Item::GenerateItemRandomPropertyId(newitemid));

        if (pProto->Quality > ITEM_QUALITY_EPIC || (pProto->Quality == ITEM_QUALITY_EPIC && pProto->ItemLevel >= MinNewsItemLevel[sWorld->getIntConfig(CONFIG_EXPANSION)]))
            if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId()))
                guild->GetNewsLog().AddNewEvent(GUILD_NEWS_ITEM_CRAFTED, time(NULL), player->GetGUID(), 0, pProto->ItemId);

        // was it successful? return error if not
        if (!pItem)
        {
            player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
            return;
        }

        // set the "Crafted by ..." property of the item
        if (pItem->GetTemplate()->Class != ITEM_CLASS_CONSUMABLE && pItem->GetTemplate()->Class != ITEM_CLASS_QUEST && newitemid != 6265 && newitemid != 6948)
            pItem->SetUInt32Value(ITEM_FIELD_CREATOR, player->GetGUIDLow());

        // send info to the client
        player->SendNewItem(pItem, num_to_add, true, bgType == 0);

        // we succeeded in creating at least one item, so a level up is possible
        if (bgType == 0)
            player->UpdateCraftSkill(m_spellInfo->Id);
    }

/*
    // for battleground marks send by mail if not add all expected
    if (no_space > 0 && bgType)
    {
        if (Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(bgType)))
            bg->SendRewardMarkByMail(player, newitemid, no_space);
    }
*/
}

void Spell::EffectCreateItem(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->IsAbilityOfSkillType(SKILL_ARCHAEOLOGY))
        if (!m_caster->ToPlayer()->GetArchaeologyMgr().SolveResearchProject(m_spellInfo->ResearchProject))
            return;

    DoCreateItem(effIndex, m_spellInfo->Effects[effIndex].ItemType);
    ExecuteLogEffectCreateItem(effIndex, m_spellInfo->Effects[effIndex].ItemType);
}

void Spell::EffectCreateItem2(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();

    uint32 item_id = m_spellInfo->Effects[effIndex].ItemType;

    // Some recipes can proc for more valueable items (cataclysm)
    uint32 new_id = 0;
    switch (item_id)
    {
        case 52309: new_id = 52314; break; // Nightstone Choker
        case 52308: new_id = 52316; break; // Hessonite Band
        case 52307: new_id = 52312; break; // Alicite Pendant  
        case 52306: new_id = 52310; break; // Jasper Ring
        default: break;
    }

    if (new_id && roll_chance_i(10))
        item_id = new_id;

    if (item_id)
        DoCreateItem(effIndex, item_id);

    // special case: fake item replaced by generate using spell_loot_template
    if (m_spellInfo->IsLootCrafting())
    {
        if (item_id)
        {
            if (!player->HasItemCount(item_id))
                return;

            // remove reagent
            uint32 count = 1;
            player->DestroyItemCount(item_id, count, true);

            // create some random items
            player->AutoStoreLoot(m_spellInfo->Id, LootTemplates_Spell);
        }
        else
            player->AutoStoreLoot(m_spellInfo->Id, LootTemplates_Spell);    // create some random items
    }
    // TODO: ExecuteLogEffectCreateItem(i, m_spellInfo->Effects[i].ItemType);
}

void Spell::EffectCreateRandomItem(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    Player* player = unitTarget->ToPlayer();

    // create some random items
    player->AutoStoreLoot(m_spellInfo->Id, LootTemplates_Spell);
    // TODO: ExecuteLogEffectCreateItem(i, m_spellInfo->Effects[i].ItemType);
}

void Spell::EffectPersistentAA(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (!m_spellAura)
    {
        Unit* caster = m_caster->GetEntry() == WORLD_TRIGGER ? m_originalCaster : m_caster;
        float radius = m_spellInfo->Effects[effIndex].CalcRadius(caster);

        // Caster not in world, might be spell triggered from aura removal
        if (!caster->IsInWorld())
            return;

        DynamicObject* dynObj = new DynamicObject(false);
        if (!dynObj->CreateDynamicObject(sObjectMgr->GenerateLowGuid(HIGHGUID_DYNAMICOBJECT), caster, m_spellInfo, *destTarget, radius, DYNAMIC_OBJECT_AREA_SPELL))
        {
            delete dynObj;
            return;
        }

        if (AuraPtr aura = Aura::TryCreate(m_spellInfo, MAX_EFFECT_MASK, dynObj, caster, m_spellPowerData, &m_spellValue->EffectBasePoints[0]))
        {
            m_spellAura = aura;
            m_spellAura->_RegisterForTargets();
        }
        else
            return;
    }

    ASSERT(m_spellAura->GetDynobjOwner());
    m_spellAura->_ApplyEffectForTargets(effIndex);
}

void Spell::EffectEnergize(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;
    if (!unitTarget->isAlive())
        return;

    if (m_spellInfo->Effects[effIndex].MiscValue < 0 || m_spellInfo->Effects[effIndex].MiscValue >= int8(MAX_POWERS))
        return;

    Powers power = Powers(m_spellInfo->Effects[effIndex].MiscValue);

    // Some level depends spells
    int level_multiplier = 0;
    int level_diff = 0;
    switch (m_spellInfo->Id)
    {
        case 9512:                                          // Restore Energy
            level_diff = m_caster->getLevel() - 40;
            level_multiplier = 2;
            break;
        case 24571:                                         // Blood Fury
            level_diff = m_caster->getLevel() - 60;
            level_multiplier = 10;
            break;
        case 24532:                                         // Burst of Energy
            level_diff = m_caster->getLevel() - 60;
            level_multiplier = 4;
            break;
        case 63375:                                         // Primal Wisdom
            damage = int32(CalculatePct(unitTarget->GetCreateMana(), damage));
            break;
        case 67490:                                         // Runic Mana Injector (mana gain increased by 25% for engineers - 3.2.0 patch change)
        {
            if (Player* player = m_caster->ToPlayer())
                if (player->HasSkill(SKILL_ENGINEERING))
                    AddPct(damage, 25);
            break;
        }
        case 92601: // Detonate Mana, Tyrande's Favorite Doll
            if (constAuraEffectPtr aurEff = m_caster->GetAuraEffect(92596, EFFECT_0))
            {
                damage = aurEff->GetAmount();
                m_caster->RemoveAurasDueToSpell(92596);
            }
            break;
        case 99069: // Fires of Heaven, Item - Paladin T12 Holy 2P Bonus
        case 99007: // Heartfire, Item - Druid T12 Restoration 2P Bonus
        case 99131: // Divine Fire, Item - Mage T12 2P Bonus
        case 99189: // Flametide, Item - Shaman T12 Restoration 2P Bonus 
            damage = int32(CalculatePct(unitTarget->GetCreateMana(), damage));
            break;
        case 35395: // Crusader Strike
            if (uint64 targetGUID = m_targets.GetUnitTargetGUID())
            {
                for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                {
                    if (ihit->targetGUID == targetGUID)
                    {
                        if (ihit->missCondition != SPELL_MISS_NONE)
                            return;
                    }
                }
            }
            break;
        case 77443: // concentration
            if (m_caster->HasAura(53220))
                damage += 3;
            break;

        default: break;
    }

    // Master Mana Potion - 105709
    if (m_spellInfo->Id == 105709)
        damage = urand(28500,31500);

    // Replenish Mana - 5405
    if (m_spellInfo->Id == 119313)
        damage = urand(42750, 47250);

    if (level_diff > 0)
        damage -= level_multiplier * level_diff;

    if (!damage)
        return;

    if (unitTarget->GetMaxPower(power) == 0)
        return;

    m_caster->EnergizeBySpell(unitTarget, m_spellInfo->Id, damage, power);

    // Mad Alchemist's Potion
    if (m_spellInfo->Id == 45051)
    {
        // find elixirs on target
        bool guardianFound = false;
        bool battleFound = false;
        Unit::AuraApplicationMap& Auras = unitTarget->GetAppliedAuras();
        for (Unit::AuraApplicationMap::iterator itr = Auras.begin(); itr != Auras.end(); ++itr)
        {
            uint32 spell_id = itr->second->GetBase()->GetId();
            if (!guardianFound)
                if (sSpellMgr->IsSpellMemberOfSpellGroup(spell_id, SPELL_GROUP_ELIXIR_GUARDIAN))
                    guardianFound = true;
            if (!battleFound)
                if (sSpellMgr->IsSpellMemberOfSpellGroup(spell_id, SPELL_GROUP_ELIXIR_BATTLE))
                    battleFound = true;
            if (battleFound && guardianFound)
                break;
        }

        // get all available elixirs by mask and spell level
        std::set<uint32> avalibleElixirs;
        if (!guardianFound)
            sSpellMgr->GetSetOfSpellsInSpellGroup(SPELL_GROUP_ELIXIR_GUARDIAN, avalibleElixirs);
        if (!battleFound)
            sSpellMgr->GetSetOfSpellsInSpellGroup(SPELL_GROUP_ELIXIR_BATTLE, avalibleElixirs);
        for (std::set<uint32>::iterator itr = avalibleElixirs.begin(); itr != avalibleElixirs.end();)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(*itr);
            if (spellInfo->SpellLevel < m_spellInfo->SpellLevel || spellInfo->SpellLevel > unitTarget->getLevel())
                avalibleElixirs.erase(itr++);
            else if (sSpellMgr->IsSpellMemberOfSpellGroup(*itr, SPELL_GROUP_ELIXIR_SHATTRATH))
                avalibleElixirs.erase(itr++);
            else if (sSpellMgr->IsSpellMemberOfSpellGroup(*itr, SPELL_GROUP_ELIXIR_UNSTABLE))
                avalibleElixirs.erase(itr++);
            else
                ++itr;
        }

        if (!avalibleElixirs.empty())
        {
            // cast random elixir on target
            m_caster->CastSpell(unitTarget, SkyMistCore::Containers::SelectRandomContainerElement(avalibleElixirs), true, m_CastItem);
        }
    }
}

void Spell::EffectEnergizePct(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;
    if (!unitTarget->isAlive())
        return;

    if (m_spellInfo->Effects[effIndex].MiscValue < 0 || m_spellInfo->Effects[effIndex].MiscValue >= int8(MAX_POWERS))
        return;

    Powers power = Powers(m_spellInfo->Effects[effIndex].MiscValue);

    uint32 maxPower = unitTarget->GetMaxPower(power);
    if (maxPower == 0)
        return;

    uint32 gain = CalculatePct(maxPower, damage);
    m_caster->EnergizeBySpell(unitTarget, m_spellInfo->Id, gain, power);
}

void Spell::SendLoot(uint64 guid, LootType loottype)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = m_caster->ToPlayer();
    if (!player)
        return;

    if (gameObjTarget)
    {
        // Players shouldn't be able to loot gameobjects that are currently despawned
        if (!gameObjTarget->isSpawned() && !player->isGameMaster())
        {
            sLog->outError(LOG_FILTER_SPELLS_AURAS, "Possible hacking attempt: Player %s [guid: %u] tried to loot a gameobject [entry: %u id: %u] which is on respawn time without being in GM mode!",
                            player->GetName(), player->GetGUIDLow(), gameObjTarget->GetEntry(), gameObjTarget->GetGUIDLow());
            return;
        }
        // special case, already has GossipHello inside so return and avoid calling twice
        if (gameObjTarget->GetGoType() == GAMEOBJECT_TYPE_GOOBER)
        {
            gameObjTarget->Use(m_caster);
            return;
        }

        if (sScriptMgr->OnGossipHello(player, gameObjTarget))
            return;

        if (gameObjTarget->AI()->GossipHello(player))
            return;

        switch (gameObjTarget->GetGoType())
        {
            case GAMEOBJECT_TYPE_DOOR:
            case GAMEOBJECT_TYPE_BUTTON:
                gameObjTarget->UseDoorOrButton(0, false, player);
                player->GetMap()->ScriptsStart(sGameObjectScripts, gameObjTarget->GetDBTableGUIDLow(), player, gameObjTarget);
                return;

            case GAMEOBJECT_TYPE_QUESTGIVER:
                player->PrepareGossipMenu(gameObjTarget, gameObjTarget->GetGOInfo()->questgiver.gossipID, true);
                player->SendPreparedGossip(gameObjTarget);
                return;

            case GAMEOBJECT_TYPE_SPELL_FOCUS:
                // triggering linked GO
                if (uint32 trapEntry = gameObjTarget->GetGOInfo()->spellFocus.linkedTrapId)
                    gameObjTarget->TriggeringLinkedGameObject(trapEntry, m_caster);
                return;

            case GAMEOBJECT_TYPE_CHEST:
                // TODO: possible must be moved to loot release (in different from linked triggering)
                if (gameObjTarget->GetGOInfo()->chest.eventId)
                    player->GetMap()->ScriptsStart(sEventScripts, gameObjTarget->GetGOInfo()->chest.eventId, player, gameObjTarget);

                // triggering linked GO
                if (uint32 trapEntry = gameObjTarget->GetGOInfo()->chest.linkedTrapId)
                    gameObjTarget->TriggeringLinkedGameObject(trapEntry, m_caster);

                // Don't return, let loots been taken
            default:
                break;
        }
    }

    // Send loot
    player->SendLoot(guid, loottype);
}

void Spell::EffectOpenLock(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = m_caster->ToPlayer();

    uint32 lockId = 0;
    uint64 guid = 0;

    // Get lockId
    if (gameObjTarget)
    {
        GameObjectTemplate const* goInfo = gameObjTarget->GetGOInfo();
        // Arathi Basin banner opening. // TODO: Verify correctness of this check
        if ((goInfo->type == GAMEOBJECT_TYPE_BUTTON && goInfo->button.noDamageImmune) ||
            (goInfo->type == GAMEOBJECT_TYPE_GOOBER && goInfo->goober.losOK))
        {
            //CanUseBattlegroundObject() already called in CheckCast()
            // in battleground check
            if (Battleground* bg = player->GetBattleground())
            {
                bg->EventPlayerClickedOnFlag(player, gameObjTarget);
                return;
            }
        }
        else if (goInfo->type == GAMEOBJECT_TYPE_FLAGSTAND || goInfo->type == GAMEOBJECT_TYPE_FLAGSTAND_2)
        {
            //CanUseBattlegroundObject() already called in CheckCast()
            // in battleground check
            if (Battleground* bg = player->GetBattleground())
            {
                if (bg->GetTypeID(true) == BATTLEGROUND_EY || bg->GetTypeID(true) == BATTLEGROUND_EYR)
                    bg->EventPlayerClickedOnFlag(player, gameObjTarget);
                return;
            }
        }
        else if (m_spellInfo->Id == 1842 && gameObjTarget->GetGOInfo()->type == GAMEOBJECT_TYPE_TRAP && gameObjTarget->GetOwner())
        {
            gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
            return;
        }
        // TODO: Add script for spell 41920 - Filling, becouse server it freze when use this spell
        // handle outdoor pvp object opening, return true if go was registered for handling
        // these objects must have been spawned by outdoorpvp!
        else if (gameObjTarget->GetGOInfo()->type == GAMEOBJECT_TYPE_GOOBER && sOutdoorPvPMgr->HandleOpenGo(player, gameObjTarget->GetGUID()))
            return;
        lockId = goInfo->GetLockId();
        guid = gameObjTarget->GetGUID();
    }
    else if (itemTarget)
    {
        lockId = itemTarget->GetTemplate()->LockID;
        guid = itemTarget->GetGUID();
    }
    else
        return;

    SkillType skillId = SKILL_NONE;
    int32 reqSkillValue = 0;
    int32 skillValue;

    SpellCastResult res = CanOpenLock(effIndex, lockId, skillId, reqSkillValue, skillValue);
    if (res != SPELL_CAST_OK)
    {
        SendCastResult(res);
        return;
    }

    if (gameObjTarget)
        SendLoot(guid, LOOT_SKINNING);
    else if (itemTarget)
        itemTarget->SetFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_UNLOCKED);

    // not allow use skill grow at item base open
    if (!m_CastItem && skillId != SKILL_NONE)
    {
        // update skill if really known
        if (uint32 pureSkillValue = player->GetPureSkillValue(skillId))
        {
            if (gameObjTarget)
            {
                // Allow one skill-up until respawned
                if (!gameObjTarget->IsInSkillupList(player->GetGUIDLow()) &&
                    player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue))
                {
                    gameObjTarget->AddToSkillupList(player->GetGUIDLow());

                    // Update player XP
                    // Patch 4.0.1 (2010-10-12): Gathering herbs and Mining will give XP
                    if (skillId == SKILL_MINING || skillId == SKILL_HERBALISM)
                        player->GiveGatheringXP();
                }
            }
            else if (itemTarget)
            {
                // Do one skill-up
                player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue);
            }
        }
    }
    ExecuteLogEffectOpenLock(effIndex, gameObjTarget ? (Object*)gameObjTarget : (Object*)itemTarget);
}

void Spell::EffectSummonChangeItem(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = m_caster->ToPlayer();

    // applied only to using item
    if (!m_CastItem)
        return;

    // ... only to item in own inventory/bank/equip_slot
    if (m_CastItem->GetOwnerGUID() != player->GetGUID())
        return;

    uint32 newitemid = m_spellInfo->Effects[effIndex].ItemType;
    if (!newitemid)
        return;

    uint16 pos = m_CastItem->GetPos();

    Item* pNewItem = Item::CreateItem(newitemid, 1, player);
    if (!pNewItem)
        return;

    for (uint8 j = PERM_ENCHANTMENT_SLOT; j <= TEMP_ENCHANTMENT_SLOT; ++j)
        if (m_CastItem->GetEnchantmentId(EnchantmentSlot(j)))
            pNewItem->SetEnchantment(EnchantmentSlot(j), m_CastItem->GetEnchantmentId(EnchantmentSlot(j)), m_CastItem->GetEnchantmentDuration(EnchantmentSlot(j)), m_CastItem->GetEnchantmentCharges(EnchantmentSlot(j)));

    if (m_CastItem->GetUInt32Value(ITEM_FIELD_DURABILITY) < m_CastItem->GetUInt32Value(ITEM_FIELD_MAXDURABILITY))
    {
        double lossPercent = 1 - m_CastItem->GetUInt32Value(ITEM_FIELD_DURABILITY) / double(m_CastItem->GetUInt32Value(ITEM_FIELD_MAXDURABILITY));
        player->DurabilityLoss(pNewItem, lossPercent);
    }

    if (player->IsInventoryPos(pos))
    {
        ItemPosCountVec dest;
        InventoryResult msg = player->CanStoreItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), dest, pNewItem, true);
        if (msg == EQUIP_ERR_OK)
        {
            player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

            // prevent crash at access and unexpected charges counting with item update queue corrupt
            if (m_CastItem == m_targets.GetItemTarget())
                m_targets.SetItemTarget(NULL);

            m_CastItem = NULL;

            player->StoreItem(dest, pNewItem, true);
            return;
        }
    }
    else if (player->IsBankPos(pos))
    {
        ItemPosCountVec dest;
        uint8 msg = player->CanBankItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), dest, pNewItem, true);
        if (msg == EQUIP_ERR_OK)
        {
            player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

            // prevent crash at access and unexpected charges counting with item update queue corrupt
            if (m_CastItem == m_targets.GetItemTarget())
                m_targets.SetItemTarget(NULL);

            m_CastItem = NULL;

            player->BankItem(dest, pNewItem, true);
            return;
        }
    }
    else if (player->IsEquipmentPos(pos))
    {
        uint16 dest;

        player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

        uint8 msg = player->CanEquipItem(m_CastItem->GetSlot(), dest, pNewItem, true);

        if (msg == EQUIP_ERR_OK || msg == EQUIP_ERR_CLIENT_LOCKED_OUT)
        {
            if (msg == EQUIP_ERR_CLIENT_LOCKED_OUT) dest = EQUIPMENT_SLOT_MAINHAND;

            // prevent crash at access and unexpected charges counting with item update queue corrupt
            if (m_CastItem == m_targets.GetItemTarget())
                m_targets.SetItemTarget(NULL);

            m_CastItem = NULL;

            player->EquipItem(dest, pNewItem, true);
            player->AutoUnequipOffhandIfNeed();
            return;
        }
    }

    // fail
    delete pNewItem;
}

void Spell::EffectProficiency(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    Player* p_target = m_caster->ToPlayer();

    uint32 subClassMask = m_spellInfo->EquippedItemSubClassMask;
    if (m_spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON && !(p_target->GetWeaponProficiency() & subClassMask))
    {
        p_target->AddWeaponProficiency(subClassMask);
        p_target->SendProficiency(ITEM_CLASS_WEAPON, p_target->GetWeaponProficiency());
    }
    if (m_spellInfo->EquippedItemClass == ITEM_CLASS_ARMOR && !(p_target->GetArmorProficiency() & subClassMask))
    {
        p_target->AddArmorProficiency(subClassMask);
        p_target->SendProficiency(ITEM_CLASS_ARMOR, p_target->GetArmorProficiency());
    }
}

void Spell::EffectSummonType(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    uint32 entry = m_spellInfo->Effects[effIndex].MiscValue;
    if (!entry)
        return;

    SummonPropertiesEntry const* properties = sSummonPropertiesStore.LookupEntry(m_spellInfo->Effects[effIndex].MiscValueB);
    if (!properties)
        return;

    if (!m_originalCaster)
        return;

    // Fix Mindbender : Pet entry update function of weapon (sha)
    if (m_spellInfo->Id == 123040 && m_originalCaster->ToPlayer())
    {
        Item* mainItem = m_originalCaster->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

        if (mainItem && (mainItem->GetEntry() == 86335 || mainItem->GetEntry() == 86893 || mainItem->GetEntry() == 87170 ||
            mainItem->GetEntry() == 86227 || mainItem->GetEntry() == 86990 || mainItem->GetEntry() == 86865))
        {
            entry = sSpellMgr->GetSpellInfo(132604)->Effects[effIndex].MiscValue;

            SummonPropertiesEntry const* newProperties = sSummonPropertiesStore.LookupEntry(sSpellMgr->GetSpellInfo(132604)->Effects[effIndex].MiscValueB);
            if (newProperties)
                properties = newProperties;
        }
    }

    // Demonic Gateway : Remove old summon when cast an other gate
    if (m_spellInfo->Id == 113890 || m_spellInfo->Id == 113886)
    {
        if (m_spellInfo->Id == 113890)
        {
            std::list<Creature*> tempList;
            std::list<Creature*> gatewayList;

            m_caster->GetCreatureListWithEntryInGrid(tempList, 59271, 500.0f);

            if (!tempList.empty())
            {
                for (auto itr : tempList)
                    gatewayList.push_back(itr);

                // Remove other players mushrooms
                for (std::list<Creature*>::iterator i = tempList.begin(); i != tempList.end(); ++i)
                {
                    Unit* owner = (*i)->GetOwner();
                    if (owner && owner == m_caster && (*i)->isSummon())
                        continue;

                    gatewayList.remove((*i));
                }

                // 1 gateway max
                if ((int32)gatewayList.size() >= 1)
                    gatewayList.back()->ToTempSummon()->UnSummon();
            }
        }
        else
        {
            std::list<Creature*> tempList;
            std::list<Creature*> gatewayList;

            m_caster->GetCreatureListWithEntryInGrid(tempList, 59262, 500.0f);

            if (!tempList.empty())
            {
                for (auto itr : tempList)
                    gatewayList.push_back(itr);

                // Remove other players mushrooms
                for (std::list<Creature*>::iterator i = tempList.begin(); i != tempList.end(); ++i)
                {
                    Unit* owner = (*i)->GetOwner();
                    if (owner && owner == m_caster && (*i)->isSummon())
                        continue;

                    gatewayList.remove((*i));
                }

                // 1 gateway max
                if ((int32)gatewayList.size() >= 1)
                    gatewayList.back()->ToTempSummon()->UnSummon();
            }

        }
    }

    // Primal Elementalist
    if (m_spellInfo->Id == 33663 || m_spellInfo->Id == 117663)
    {
        if (m_originalCaster->GetTypeId() == TYPEID_UNIT)
        {
            if (m_originalCaster->isTotem() && m_originalCaster->GetOwner())
            {
                if (m_originalCaster->GetOwner()->HasAura(117013))
                {
                    m_originalCaster->CastSpell(m_originalCaster, m_spellInfo->Id == 33663 ? 118323 : 118291, true);
                    return;
                }
            }
        }
    }

    int32 duration = m_spellInfo->GetDuration();
    if (Player* modOwner = m_originalCaster->GetSpellModOwner())
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_DURATION, duration);

    // Item - Warlock T13 2P Bonus (Doomguard and Infernal)
    if (entry == 11859 || entry == 89)
        if (constAuraEffectPtr aurEff = m_originalCaster->GetAuraEffect(105888, EFFECT_1))
            duration += aurEff->GetAmount() * 1000;

    TempSummon* summon = NULL;

    // determine how many units should be summoned
    uint32 numSummons;

    // some spells need to summon many units, for those spells number of summons is stored in effect value
    // however so far noone found a generic check to find all of those (there's no related data in summonproperties.dbc
    // and in spell attributes, possibly we need to add a table for those)
    // so here's a list of MiscValueB values, which is currently most generic check
    switch (properties->Id)
    {
        case 61:
        case 64:
        case 66:
        case 181:
        case 629:
        case 648:
        case 715:
        case 833:
        case 2301:
        case 1061:
        case 1101:
        case 1161:
        case 1261:
        case 1562:
        case 2929: // Summon Unbound Flamesparks, Flameseer's Staff
        case 3097:// Force of Nature
        case 3245:
            numSummons = (damage > 0) ? damage : 1;
            break;
        default:
            numSummons = 1;
            break;
    }

    switch (properties->Category)
    {
        case SUMMON_CATEGORY_WILD:
        case SUMMON_CATEGORY_ALLY:
        case SUMMON_CATEGORY_UNK:
            if ((properties->Flags & 512) || m_spellInfo->Id == 114192 || m_spellInfo->Id == 114203 || m_spellInfo->Id == 114207)
            {
                SummonGuardian(effIndex, entry, properties, numSummons);
                break;
            }
            switch (properties->Type)
            {
                case SUMMON_TYPE_PET:
                case SUMMON_TYPE_GUARDIAN:
                case SUMMON_TYPE_GUARDIAN2:
                case SUMMON_TYPE_MINION:
                    SummonGuardian(effIndex, entry, properties, numSummons);
                    break;
                // Summons a vehicle, but doesn't force anyone to enter it (see SUMMON_CATEGORY_VEHICLE)
                case SUMMON_TYPE_VEHICLE:
                case SUMMON_TYPE_VEHICLE2:
                    summon = m_caster->GetMap()->SummonCreature(entry, *destTarget, properties, duration, m_originalCaster, m_spellInfo->Id);
                    break;
                case SUMMON_TYPE_TOTEM:
                {
                    summon = m_caster->GetMap()->SummonCreature(entry, *destTarget, properties, duration, m_originalCaster, m_spellInfo->Id);
                    if (!summon || !summon->isTotem())
                        return;

                    if (damage)                                            // if not spell info, DB values used
                    {
                        summon->SetMaxHealth(damage != 10 ? (m_caster->HasAura(63298) ? m_caster->CountPctFromMaxHealth(5) : damage) : m_caster->CountPctFromMaxHealth(m_caster->HasAura(63298) ? 15 : 10));
                        summon->SetHealth(damage != 10 ? (m_caster->HasAura(63298) ? m_caster->CountPctFromMaxHealth(5) : damage) : m_caster->CountPctFromMaxHealth(m_caster->HasAura(63298) ? 15 : 10));
                    }
                    break;
                }
                case SUMMON_TYPE_MINIPET:
                {
                    summon = m_caster->GetMap()->SummonCreature(entry, *destTarget, properties, duration, m_originalCaster, m_spellInfo->Id);
                    if (!summon || !summon->HasUnitTypeMask(UNIT_MASK_MINION))
                        return;

                    summon->SelectLevel(summon->GetCreatureTemplate());       // some summoned creaters have different from 1 DB data for level/hp
                    summon->SetUInt32Value(UNIT_NPC_FLAGS, summon->GetCreatureTemplate()->npcflag);
                    summon->SetUInt32Value(UNIT_NPC_FLAGS + 1, summon->GetCreatureTemplate()->npcflag2);

                    summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);

                    summon->AI()->EnterEvadeMode();
                    break;
                }
                default:
                {
                    float radius = m_spellInfo->Effects[effIndex].CalcRadius();

                    TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;

                    for (uint32 count = 0; count < numSummons; ++count)
                    {
                        Position pos;
                        if (count == 0)
                            pos = *destTarget;
                        else
                            // randomize position for multiple summons
                            m_caster->GetRandomPoint(*destTarget, radius, pos);

                        summon = m_originalCaster->SummonCreature(entry, *destTarget, summonType, duration);
                        if (!summon)
                            continue;

                        switch (properties->Id)
                        {
                            case 3347: // Orphelins
                            {
                                if (uint32 slot = properties->Slot)
                                {
                                    if (m_caster->m_SummonSlot[slot] && m_caster->m_SummonSlot[slot] != summon->GetGUID())
                                    {
                                        Creature* oldSummon = m_caster->GetMap()->GetCreature(m_caster->m_SummonSlot[slot]);
                                        if (oldSummon && oldSummon->isSummon())
                                            oldSummon->ToTempSummon()->UnSummon();
                                    }
                                    m_caster->m_SummonSlot[slot] = summon->GetGUID();
                                }
                            }
                            default:
                                break;
                        }

                        if (properties->Category == SUMMON_CATEGORY_ALLY)
                        {
                            summon->SetOwnerGUID(m_originalCaster->GetGUID());
                            summon->setFaction(m_originalCaster->getFaction());
                            summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);
                        }

                        // Explosive Decoy and Explosive Decoy 2.0
                        if (m_spellInfo->Id == 54359 || m_spellInfo->Id == 62405)
                        {
                            summon->SetMaxHealth(damage);
                            summon->SetHealth(damage);
                            summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        }

                        // Wild Mushroom : Plague
                        if (summon && m_spellInfo->Id == 113516)
                            m_originalCaster->CastSpell(m_originalCaster, 113517, true); // Wild Mushroom : Plague (periodic dummy)

                        ExecuteLogEffectSummonObject(effIndex, summon);
                    }
                    return;
                }
            }//switch
            break;
        case SUMMON_CATEGORY_PET:
            SummonGuardian(effIndex, entry, properties, numSummons);
            break;
        case SUMMON_CATEGORY_PUPPET:
            summon = m_caster->GetMap()->SummonCreature(entry, *destTarget, properties, duration, m_originalCaster, m_spellInfo->Id);
            break;
        case SUMMON_CATEGORY_VEHICLE:
            // Summoning spells (usually triggered by npc_spellclick) that spawn a vehicle and that cause the clicker
            // to cast a ride vehicle spell on the summoned unit.
            float x, y, z;
            m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);
            summon = m_originalCaster->GetMap()->SummonCreature(entry, *destTarget, properties, duration, m_caster, m_spellInfo->Id);
            if (!summon || !summon->IsVehicle())
                return;

            // The spell that this effect will trigger. It has SPELL_AURA_CONTROL_VEHICLE
            uint32 spellId = VEHICLE_SPELL_RIDE_HARDCODED;
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(m_spellInfo->Effects[effIndex].CalcValue());
            if (spellInfo && spellInfo->HasAura(SPELL_AURA_CONTROL_VEHICLE))
                spellId = spellInfo->Id;

            // Hard coded enter vehicle spell
            m_originalCaster->CastSpell(summon, spellId, true);

            uint32 faction = properties->Faction;
            if (!faction)
                faction = m_originalCaster->getFaction();

            summon->setFaction(faction);
            break;
    }

    if (summon)
    {
        summon->SetCreatorGUID(m_originalCaster->GetGUID());
        ExecuteLogEffectSummonObject(effIndex, summon);
    }
}

void Spell::EffectLearnSpell(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        if (unitTarget->ToPet())
            EffectLearnPetSpell(effIndex);
        return;
    }

    Player* player = unitTarget->ToPlayer();

    uint32 spellToLearn = (m_spellInfo->Id == 483 || m_spellInfo->Id == 55884) ? damage : m_spellInfo->Effects[effIndex].TriggerSpell;
    player->learnSpell(spellToLearn, false);
}

typedef std::list< std::pair<uint32, uint64> > DispelList;
void Spell::EffectDispel(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    // Create dispel mask by dispel type
    uint32 dispel_type = m_spellInfo->Effects[effIndex].MiscValue;
    uint32 dispelMask  = SpellInfo::GetDispelMask(DispelType(dispel_type));

    // fix Glyph of Mass Dispel
    if (m_spellInfo->Id == 32375 && m_caster->HasAura(55691))
        m_caster->CastSpell(unitTarget, 39897, true);

    // Check if player has Execution Sentence - 114916 damage aura
    bool hasExecutionSentenceDamage = false;
    bool hasExecutionSentenceHeal = false;
    int32 executionDamage = 0;
    Unit* executionCaster;
    if (AuraPtr executionSentenceDamage = unitTarget->GetAura(114916))
    {
        executionCaster = executionSentenceDamage->GetCaster();
        if (executionCaster)
        {
            hasExecutionSentenceDamage = true;

            // calculate damage from last tick
            int32 spellPowerBonus = int32(5.936 * executionCaster->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_HOLY));
            executionDamage = 12989 + spellPowerBonus;

            float PvPPower = (1 + executionCaster->GetFloatValue(PLAYER_FIELD_PVP_POWER_DAMAGE) / 100);
            // We need to calculate damage with PvPPower
            if (PvPPower > 1.0f)
                executionDamage *= PvPPower;
            // Inquisition
            if (executionCaster->HasAura(84963))
                executionDamage *= 1.3f;
            // Avenging Wrath
            if (executionCaster->HasAura(31884))
                executionDamage *= 1.2f;
            // Berserking (bg buff)
            if (executionCaster->HasAura(23505))
                executionDamage *= 1.3f;
            uint32 lastTickNumber = int32(floor((executionSentenceDamage->GetMaxDuration() - executionSentenceDamage->GetDuration()) / 1000.0f));
            if (lastTickNumber == 1 || lastTickNumber == 0)
                executionDamage = int32(executionDamage * 0.04f); // First:   4.0%
            else if (lastTickNumber == 2)
                executionDamage = int32(executionDamage * 0.044f);// Second:  4.4%
            else if (lastTickNumber == 3)
                executionDamage = int32(executionDamage * 0.05f); // Third:   5.0%
            else if (lastTickNumber == 4)
                executionDamage = int32(executionDamage * 0.055f);// Fourth:  5.5%
            else if (lastTickNumber == 5)
                executionDamage = int32(executionDamage * 0.06f); // Fifth:   6.0%
            else if (lastTickNumber == 6)
                executionDamage = int32(executionDamage * 0.066f);// Sixth:   6.6%
            else if (lastTickNumber == 7)
                executionDamage = int32(executionDamage * 0.073f);// Seventh: 7.3%
            else if (lastTickNumber == 8)
                executionDamage = int32(executionDamage * 0.08f); // Eight:   8.0%
            else if (lastTickNumber == 9)
                executionDamage = int32(executionDamage * 0.088f);// Ninth:   8.8%

            // Last damage should be 500% from last tick damage.
            executionDamage *= 5;
        }
    }
   // Check if player has Execution Sentence - 114917 heal aura
    else if (AuraPtr executionSentenceDamage = unitTarget->GetAura(114917))
    {
        executionCaster = executionSentenceDamage->GetCaster();
        if (executionCaster)
        {
            hasExecutionSentenceHeal = true;
            int32 spellPowerBonus = int32(5.936 * executionCaster->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_HOLY));
            executionDamage = 12989 + spellPowerBonus;
            float PvPPower = (1 + executionCaster->GetFloatValue(PLAYER_FIELD_PVP_POWER_HEALING) / 100);
            // We need to calculate executionDamage with PvPPower
            if (PvPPower > 1.0f)
                executionDamage *= PvPPower;
            // Seal of Insight
            if (executionCaster->HasAura(20165))
                executionDamage *= 1.05f;
            // Avenging Wrath
            if (executionCaster->HasAura(31884))
                executionDamage *= 1.2f;
            // Holy Insight
            if (executionCaster->HasAura(112859))
                executionDamage *= 1.25f;
            // Battle Fatigue
            if (unitTarget->HasAura(134735))
                executionDamage *= 0.4f;
            uint32 lastTickNumber = int32(floor((executionSentenceDamage->GetMaxDuration() - executionSentenceDamage->GetDuration()) / 1000.0f));
            if (lastTickNumber == 1 || lastTickNumber == 0)
                executionDamage = int32(executionDamage * 0.04f); // First:   4.0%
            else if (lastTickNumber == 2)
               executionDamage = int32(executionDamage * 0.044f); // Second:  4.4%
            else if (lastTickNumber == 3)
                executionDamage = int32(executionDamage * 0.05f); // Third:   5.0%
            else if (lastTickNumber == 4)
                executionDamage = int32(executionDamage * 0.055f); // Fourth:  5.5%
            else if (lastTickNumber == 5)
                executionDamage = int32(executionDamage * 0.06f); // Fifth:   6.0%
            else if (lastTickNumber == 6)
                executionDamage = int32(executionDamage * 0.066f); // Sixth:   6.6%
            else if (lastTickNumber == 7)
                executionDamage = int32(executionDamage * 0.073f); // Seventh: 7.3%
            else if (lastTickNumber == 8)
                executionDamage = int32(executionDamage * 0.08f); // Eight:   8.0%
            else if (lastTickNumber == 9)
                executionDamage = int32(executionDamage * 0.088f); // Ninth:   8.8%

            //Last heal should be 500% from last heal tick.
            executionDamage *= 5;
        }
    }

    // Before dispel
    switch (m_spellInfo->Id)
    {
        case 88423: // Nature's Cure
            dispelMask = ((1<<DISPEL_MAGIC) | (1<<DISPEL_CURSE) | (1<<DISPEL_POISON));
            break;
        case 115450:// Detox
        {
            if (effIndex > 1)
                if (Player* plr = m_caster->ToPlayer())
                    if (plr->GetSpecializationId(plr->GetActiveSpec()) != SPEC_MONK_MISTWEAVER)
                        return;

            break;
        }
        default:
            break;
    }

    // Mass Dispel Inivisiblity removal
    if (m_spellInfo->Id == 32592)
    {
        DispelChargesList inv_dispel_list;
        unitTarget->GetDispellableAuraList(m_caster, 1 << DISPEL_INVISIBILITY, inv_dispel_list);
        // will not call scripted dispel hook
        for (DispelChargesList::iterator itr = inv_dispel_list.begin(); itr != inv_dispel_list.end(); ++itr)
        {
            if (AuraPtr aur = itr->first)
                aur->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
        }
        
    }

    DispelChargesList dispel_list;
    unitTarget->GetDispellableAuraList(m_caster, dispelMask, dispel_list);
    if (dispel_list.empty())
        return;

    // Ok if exist some buffs for dispel try dispel it
    uint32 failCount = 0;
    DispelChargesList success_list;
    WorldPacket dataFail(SMSG_DISPEL_FAILED, 8+8+4+4+damage*4);
    // dispel N = damage buffs (or while exist buffs for dispel)
    int32 count;
    for (count = 0; count < damage && !dispel_list.empty();)
    {
        // Random select buff for dispel
        DispelChargesList::iterator itr = dispel_list.begin();
        std::advance(itr, urand(0, dispel_list.size() - 1));

        int32 chance = itr->first->CalcDispelChance(unitTarget, !unitTarget->IsFriendlyTo(m_caster));
        // 2.4.3 Patch Notes: "Dispel effects will no longer attempt to remove effects that have 100% dispel resistance."
        if (!chance)
        {
            dispel_list.erase(itr);
            continue;
        }
        else
        {
            if (roll_chance_i(chance))
            {
                bool alreadyListed = false;
                for (DispelChargesList::iterator successItr = success_list.begin(); successItr != success_list.end(); ++successItr)
                {
                    if (successItr->first->GetId() == itr->first->GetId())
                    {
                        ++successItr->second;
                        alreadyListed = true;
                    }
                }
                if (!alreadyListed)
                    success_list.push_back(std::make_pair(itr->first, 1));
                --itr->second;
                if (itr->second <= 0)
                    dispel_list.erase(itr);
            }
            else
            {
                if (!failCount)
                {
                    // Failed to dispell
                    dataFail << uint64(m_caster->GetGUID());            // Caster GUID
                    dataFail << uint64(unitTarget->GetGUID());          // Victim GUID
                    dataFail << uint32(m_spellInfo->Id);                // dispel spell id
                }
                ++failCount;
                dataFail << uint32(itr->first->GetId());                         // Spell Id
            }
            ++count;
        }
    }

    if (failCount)
        m_caster->SendMessageToSet(&dataFail, true);

    if (success_list.empty())
        return;

    // Custom effects
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        // Glyph of Dispel Magic (discipline and priest) -- Purify
        if (m_spellInfo->Id == 97960)
        {
            if (AuraEffectPtr aurEff = m_caster->GetAuraEffect(55677, 0))
            {
                if (m_caster->IsFriendlyTo(unitTarget))
                {
                    int32 bp = int32(unitTarget->CountPctFromMaxHealth(aurEff->GetAmount() * count));
                    m_caster->CastCustomSpell(unitTarget, 56131, &bp, 0, 0, true); 
                }
            }
        }
        // Glyph of Dispel Magic
        if (m_spellInfo->Id == 528)
        {
            if (AuraEffectPtr aurEff = m_caster->GetAuraEffect(119864, 0))
                m_caster->CastSpell(unitTarget, 119856, true);
        }
        // Remove Curse
        if (m_spellInfo->Id == 475)
        {
            if (AuraEffectPtr aurEff = m_caster->GetAuraEffect(115700, 0))
                m_caster->AddAura(115701, m_caster);
        }
    }
    
    WorldPacket dataSuccess(SMSG_SPELLDISPELLOG, 8+8+4+1+4+success_list.size()*5);

    ObjectGuid casterGuid = unitTarget->GetGUID();
    ObjectGuid targetGuid = m_caster->GetGUID();

    dataSuccess.WriteBit(targetGuid[1]);
    dataSuccess.WriteBit(casterGuid[5]);
    dataSuccess.WriteBit(targetGuid[5]);
    dataSuccess.WriteBit(casterGuid[4]);
    dataSuccess.WriteBit(targetGuid[6]);
    dataSuccess.WriteBit(casterGuid[2]);
    dataSuccess.WriteBit(casterGuid[6]);
    dataSuccess.WriteBit(targetGuid[4]);
    dataSuccess.WriteBit(casterGuid[0]);
    dataSuccess.WriteBit(targetGuid[2]);
    dataSuccess.WriteBit(0); // unk
    dataSuccess.WriteBit(casterGuid[1]);
    dataSuccess.WriteBits(success_list.size(), 22); 
    dataSuccess.WriteBit(casterGuid[3]);
    dataSuccess.WriteBit(targetGuid[0]);
    dataSuccess.WriteBit(targetGuid[3]);
    for (DispelChargesList::iterator itr = success_list.begin(); itr != success_list.end(); ++itr)
    {
        dataSuccess.WriteBit(1); // unk bit 1
        dataSuccess.WriteBit(0); // unk bit 2
        dataSuccess.WriteBit(0); // unk bit 3
    }
    dataSuccess.WriteBit(targetGuid[7]);
    dataSuccess.WriteBit(casterGuid[7]);
    dataSuccess.WriteBit(1); // unk // hastarget ?
    dataSuccess.WriteByteSeq(casterGuid[6]);
    dataSuccess.WriteByteSeq(casterGuid[5]);
    dataSuccess.WriteByteSeq(targetGuid[2]);
    dataSuccess.WriteByteSeq(casterGuid[4]);
    dataSuccess << uint32(0); // unk
    dataSuccess.WriteByteSeq(targetGuid[4]);
    dataSuccess.WriteByteSeq(targetGuid[7]);
    dataSuccess.WriteByteSeq(targetGuid[5]);
    dataSuccess.WriteByteSeq(casterGuid[7]);
    dataSuccess.WriteByteSeq(targetGuid[0]);
    dataSuccess.WriteByteSeq(targetGuid[3]);
    dataSuccess.WriteByteSeq(casterGuid[3]);
    dataSuccess.WriteByteSeq(casterGuid[0]);
    dataSuccess.WriteByteSeq(casterGuid[1]);
    dataSuccess.WriteByteSeq(targetGuid[6]);
    dataSuccess.WriteByteSeq(casterGuid[2]);
    for (DispelChargesList::iterator itr = success_list.begin(); itr != success_list.end(); ++itr)
    {
        //if (unk bit 2)
        //    dataSuccess << uint32
        dataSuccess << uint32(itr->first->GetId());
        unitTarget->RemoveAurasDueToSpellByDispel(itr->first->GetId(), m_spellInfo->Id, itr->first->GetCasterGUID(), m_caster, itr->second);
        //if (unk bit 2)
        //    dataSuccess << uint32
    }
    dataSuccess.WriteByteSeq(targetGuid[1]);
    m_caster->SendMessageToSet(&dataSuccess, true);

    // On success dispel
    switch (m_spellInfo->Id)
    {
        case 527:   // Purify
            // Glyph of Purify
            if (m_caster->HasAura(55677))
                m_caster->CastSpell(unitTarget, 56131, true);
            break;
        case 19505: // Devour Magic
        {
            m_caster->CastSpell(m_caster, 19658, true);
            // Glyph of Felhunter
            if (Unit* owner = m_caster->GetOwner())
                if (owner->HasAura(56249))
                    owner->CastSpell(owner, 19658, true);
            break;
        }
        case 51886: // Cleanse Spirit
        case 77130: // Purify Spirit
            if (m_caster->HasAura(86959)) // Glyph of Cleansing Waters
                m_caster->CastSpell(unitTarget, 86961, true);
            break;

        default: break;
    }

    // If we dispelled Execution Sentence - 114916 damage aura, victim must receive 500% damage of last tick.
    if (hasExecutionSentenceDamage && !hasExecutionSentenceHeal && !unitTarget->HasAura(114916))
    {
        executionCaster->CastCustomSpell(unitTarget, 146585, &executionDamage, NULL, NULL, true);
    }
    // If we dispelled Execution Sentence - 114917 heal aura, victim must receive 500% heal of last tick.
    else if (hasExecutionSentenceHeal && !hasExecutionSentenceDamage && !unitTarget->HasAura(114917))
    {
        executionCaster->CastCustomSpell(unitTarget, 146586, &executionDamage, NULL, NULL, true);
    }
}

void Spell::EffectDualWield(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    // Mistweaver monks cannot dual wield
    if (unitTarget->ToPlayer())
        if (unitTarget->ToPlayer()->GetSpecializationId(unitTarget->ToPlayer()->GetActiveSpec()) == SPEC_MONK_MISTWEAVER)
            return;

    unitTarget->SetCanDualWield(true);

    if (unitTarget->GetTypeId() == TYPEID_UNIT)
        unitTarget->ToCreature()->UpdateDamagePhysical(OFF_ATTACK);
}

void Spell::EffectPull(SpellEffIndex effIndex)
{
    // TODO: create a proper pull towards distract spell center for distract
    EffectNULL(effIndex);
}

void Spell::EffectDistract(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    // Check for possible target
    if (!unitTarget || unitTarget->isInCombat())
        return;

    // target must be OK to do this
    if (unitTarget->HasUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_STUNNED | UNIT_STATE_FLEEING))
        return;

    unitTarget->SetFacingTo(unitTarget->GetAngle(destTarget));
    unitTarget->ClearUnitState(UNIT_STATE_MOVING);

    if (unitTarget->GetTypeId() == TYPEID_UNIT)
        unitTarget->GetMotionMaster()->MoveDistract(damage * IN_MILLISECONDS);
}

void Spell::EffectPickPocket(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    // victim must be creature and attackable
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->IsFriendlyTo(unitTarget))
        return;

    // victim have to be alive and humanoid or undead
    if (unitTarget->isAlive() && (unitTarget->GetCreatureTypeMask() & CREATURE_TYPE_HUMANOID) != 0)
    {
        if (m_caster->HasAura(63268)) // Glyph of Disguise
            unitTarget->AddAura(121308, m_caster);

        m_caster->ToPlayer()->SendLoot(unitTarget->GetGUID(), LOOT_PICKPOCKETING);
    }
}

void Spell::EffectAddFarsight(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    float radius = m_spellInfo->Effects[effIndex].CalcRadius();
    int32 duration = m_spellInfo->GetDuration();
    // Caster not in world, might be spell triggered from aura removal
    if (!m_caster->IsInWorld())
        return;

    DynamicObject* dynObj = new DynamicObject(true);
    if (!dynObj->CreateDynamicObject(sObjectMgr->GenerateLowGuid(HIGHGUID_DYNAMICOBJECT), m_caster, m_spellInfo, *destTarget, radius, DYNAMIC_OBJECT_FARSIGHT_FOCUS))
    {
        delete dynObj;
        return;
    }

    dynObj->SetDuration(duration);
    dynObj->SetCasterViewpoint();
}

void Spell::EffectUntrainTalents(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || m_caster->GetTypeId() == TYPEID_PLAYER)
        return;

    if (uint64 guid = m_caster->GetGUID()) // the trainer is the caster
        unitTarget->ToPlayer()->SendTalentWipeConfirm(guid, false);
}

void Spell::EffectTeleUnitsFaceCaster(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    if (unitTarget->isInFlight())
        return;

    float dis = m_spellInfo->Effects[effIndex].CalcRadius(m_caster);

    //float fx, fy, fz;
    //m_caster->GetClosePoint(fx, fy, fz, unitTarget->GetObjectSize(), dis);
    Position pos;
    m_caster->GetNearPosition(pos, m_caster->GetObjectSize(), m_caster->GetAngle(unitTarget));
    
    // Earthen Vortex, Morchok, Dragon Soul
    // Prevent dropping into textures
    switch (m_spellInfo->Id)
    {
        case 103821:
        case 110045:
        case 110046:
        case 110047:
            pos.m_positionX += 8.0f;
            break;
        default:
            break;
    }
    
    unitTarget->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), -m_caster->GetOrientation(), unitTarget == m_caster);
}

void Spell::EffectLearnSkill(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    if (damage < 0)
        return;

    uint32 skillid = m_spellInfo->Effects[effIndex].MiscValue;
    uint16 skillval = unitTarget->ToPlayer()->GetPureSkillValue(skillid);
    unitTarget->ToPlayer()->SetSkill(skillid, m_spellInfo->Effects[effIndex].CalcValue(), skillval?skillval:1, damage*75);

    // Archaeology
    if (skillid == SKILL_ARCHAEOLOGY)
    {
        unitTarget->ToPlayer()->GetArchaeologyMgr().GenerateResearchSites();
        unitTarget->ToPlayer()->GetArchaeologyMgr().GenerateResearchProjects();
    }
}

void Spell::EffectPlayMovie(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 movieId = GetSpellInfo()->Effects[effIndex].MiscValue;
     if (!sMovieStore.LookupEntry(movieId))
         return;

      unitTarget->ToPlayer()->SendMovieStart(movieId);
}

void Spell::EffectTradeSkill(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    // uint32 skillid =  m_spellInfo->Effects[i].MiscValue;
    // uint16 skillmax = unitTarget->ToPlayer()->(skillid);
    // m_caster->ToPlayer()->SetSkill(skillid, skillval?skillval:1, skillmax+75);
}

void Spell::EffectEnchantItemPerm(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    if (!itemTarget)
        return;

    Player* p_caster = (Player*)m_caster;

    // Handle vellums
    if (itemTarget->IsVellum())
    {
        // destroy one vellum from stack
        uint32 count = 1;
        p_caster->DestroyItemCount(itemTarget, count, true);
        unitTarget=p_caster;
        // and add a scroll
        DoCreateItem(effIndex, m_spellInfo->Effects[effIndex].ItemType);
        itemTarget=NULL;
        m_targets.SetItemTarget(NULL);
    }
    else
    {
        // do not increase skill if vellum used
        if (!(m_CastItem && m_CastItem->GetTemplate()->Flags & ITEM_PROTO_FLAG_TRIGGERED_CAST))
            p_caster->UpdateCraftSkill(m_spellInfo->Id);

        uint32 enchant_id = m_spellInfo->Effects[effIndex].MiscValue;
        if (!enchant_id)
            return;

        SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!pEnchant)
            return;

        // item can be in trade slot and have owner diff. from caster
        Player* item_owner = itemTarget->GetOwner();
        if (!item_owner)
            return;

        /*if (item_owner != p_caster && !AccountMgr::IsPlayerAccount(p_caster->GetSession()->GetSecurity()) && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
        {
            sLog->outCommand(p_caster->GetSession()->GetAccountId(), "GM %s (Account: %u) enchanting(perm): %s (Entry: %d) for player: %s (Account: %u)",
                p_caster->GetName(), p_caster->GetSession()->GetAccountId(),
                itemTarget->GetTemplate()->Name1.c_str(), itemTarget->GetEntry(),
                item_owner->GetName(), item_owner->GetSession()->GetAccountId());
        }*/

        // remove old enchanting before applying new if equipped
        item_owner->ApplyEnchantment(itemTarget, PERM_ENCHANTMENT_SLOT, false);

        itemTarget->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchant_id, 0, 0);

        // add new enchanting if equipped
        item_owner->ApplyEnchantment(itemTarget, PERM_ENCHANTMENT_SLOT, true);

        item_owner->RemoveTradeableItem(itemTarget);
        itemTarget->ClearSoulboundTradeable(item_owner);
    }
}

void Spell::EffectEnchantItemPrismatic(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    if (!itemTarget)
        return;

    Player* p_caster = (Player*)m_caster;

    uint32 enchant_id = m_spellInfo->Effects[effIndex].MiscValue;
    if (!enchant_id)
        return;

    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
        return;

    // support only enchantings with add socket in this slot
    {
        bool add_socket = false;
        for (uint8 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
        {
            if (pEnchant->type[i] == ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET)
            {
                add_socket = true;
                break;
            }
        }
        if (!add_socket)
        {
            sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::EffectEnchantItemPrismatic: attempt apply enchant spell %u with SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC (%u) but without ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET (%u), not suppoted yet.",
                m_spellInfo->Id, SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC, ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET);
            return;
        }
    }

    // item can be in trade slot and have owner diff. from caster
    Player* item_owner = itemTarget->GetOwner();
    if (!item_owner)
        return;

    /*if (item_owner != p_caster && !AccountMgr::IsPlayerAccount(p_caster->GetSession()->GetSecurity()) && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        sLog->outCommand(p_caster->GetSession()->GetAccountId(), "GM %s (Account: %u) enchanting(perm): %s (Entry: %d) for player: %s (Account: %u)",
            p_caster->GetName(), p_caster->GetSession()->GetAccountId(),
            itemTarget->GetTemplate()->Name1.c_str(), itemTarget->GetEntry(),
            item_owner->GetName(), item_owner->GetSession()->GetAccountId());
    }*/

    // remove old enchanting before applying new if equipped
    item_owner->ApplyEnchantment(itemTarget, PRISMATIC_ENCHANTMENT_SLOT, false);

    itemTarget->SetEnchantment(PRISMATIC_ENCHANTMENT_SLOT, enchant_id, 0, 0);

    // add new enchanting if equipped
    item_owner->ApplyEnchantment(itemTarget, PRISMATIC_ENCHANTMENT_SLOT, true);

    item_owner->RemoveTradeableItem(itemTarget);
    itemTarget->ClearSoulboundTradeable(item_owner);
}

void Spell::EffectEnchantItemTmp(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)m_caster;

    if (!itemTarget)
        return;

    uint32 enchant_id = m_spellInfo->Effects[effIndex].MiscValue;

    if (!enchant_id)
    {
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell %u Effect %u (SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) have 0 as enchanting id", m_spellInfo->Id, effIndex);
        return;
    }

    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
    {
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell %u Effect %u (SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) have not existed enchanting id %u ", m_spellInfo->Id, effIndex, enchant_id);
        return;
    }

    // select enchantment duration
    uint32 duration;

    // rogue family enchantments exception by duration
    if (m_spellInfo->Id == 38615)
        duration = 1800;                                    // 30 mins
    // other rogue family enchantments always 1 hour (some have spell damage=0, but some have wrong data in EffBasePoints)
    else if (m_spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE)
        duration = 3600;                                    // 1 hour
    // shaman family enchantments
    else if (m_spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN)
        duration = 3600;                                    // 1 hour
    // other cases with this SpellVisual already selected
    else if (m_spellInfo->SpellVisual[0] == 215)
        duration = 1800;                                    // 30 mins
    // some fishing pole bonuses except Glow Worm which lasts full hour
    else if (m_spellInfo->SpellVisual[0] == 563 && m_spellInfo->Id != 64401)
        duration = 600;                                     // 10 mins
    // shaman rockbiter enchantments
    else if (m_spellInfo->SpellVisual[0] == 0)
        duration = 1800;                                    // 30 mins
    else if (m_spellInfo->Id == 29702)
        duration = 300;                                     // 5 mins
    else if (m_spellInfo->Id == 37360)
        duration = 300;                                     // 5 mins
    // default case
    else
        duration = 3600;                                    // 1 hour

    // item can be in trade slot and have owner diff. from caster
    Player* item_owner = itemTarget->GetOwner();
    if (!item_owner)
        return;

    // remove old enchanting before applying new if equipped
    item_owner->ApplyEnchantment(itemTarget, TEMP_ENCHANTMENT_SLOT, false);

    itemTarget->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, duration * 1000, 0);

    // add new enchanting if equipped
    item_owner->ApplyEnchantment(itemTarget, TEMP_ENCHANTMENT_SLOT, true);
}

void Spell::EffectTameCreature(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetPetGUID())
        return;

    if (!unitTarget)
        return;

    if (unitTarget->GetTypeId() != TYPEID_UNIT)
        return;

    Creature* creatureTarget = unitTarget->ToCreature();

    if (creatureTarget->isPet())
        return;

    if (m_caster->getClass() != CLASS_HUNTER)
        return;

    // If we have a full list we shoulden't be able to create a new one.
    if (m_caster->ToPlayer()->getSlotForNewPet() == PET_SLOT_FULL_LIST)
    {
        m_caster->ToPlayer()->SendPetTameResult(PET_TAME_ERROR_TOO_MANY_PETS);
        return;
    }

    // cast finish successfully
    //SendChannelUpdate(0);
    finish();

    Pet* pet = m_caster->CreateTamedPetFrom(creatureTarget, m_spellInfo->Id);
    if (!pet)                                               // in very specific state like near world end/etc.
        return;

    // "kill" original creature
    creatureTarget->DespawnOrUnsummon();

    uint8 level = m_caster->getLevel();

    // prepare visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL, level - 1);

    // add to world
    pet->GetMap()->AddToMap(pet->ToCreature());

    // visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL, level);

    // caster have pet now
    m_caster->SetMinion(pet, true, m_caster->ToPlayer()->getSlotForNewPet());

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        pet->SavePetToDB(PET_SLOT_ACTUAL_PET_SLOT);
        m_caster->ToPlayer()->PetSpellInitialize();
        m_caster->ToPlayer()->GetSession()->SendStablePet(0);
    }
}

void Spell::EffectSummonPet(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    Player* owner = NULL;
    if (m_originalCaster)
    {
        owner = m_originalCaster->ToPlayer();
        if (!owner && m_originalCaster->ToCreature()->isTotem())
            owner = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself();
    }

    uint32 petentry = m_spellInfo->Effects[effIndex].MiscValue;

    PetSlot slot = PetSlot(m_spellInfo->Effects[effIndex].BasePoints);
    if (petentry)
        slot = PET_SLOT_UNK_SLOT;

    if (!owner)
    {
        SummonPropertiesEntry const* properties = sSummonPropertiesStore.LookupEntry(67);
        if (properties)
            SummonGuardian(effIndex, petentry, properties, 1);
        return;
    }

    Pet* OldSummon = owner->GetPet();

    // if pet requested type already exist
    if (OldSummon)
    {
        if (petentry == 0 || OldSummon->GetEntry() == petentry)
        {
            // pet in corpse state can't be summoned
            if (OldSummon->isDead())
                return;

            ASSERT(OldSummon->GetMap() == owner->GetMap());

            //OldSummon->GetMap()->Remove(OldSummon->ToCreature(), false);

            float px, py, pz;
            owner->GetClosePoint(px, py, pz, OldSummon->GetObjectSize());

            OldSummon->NearTeleportTo(px, py, pz, OldSummon->GetOrientation());
            //OldSummon->Relocate(px, py, pz, OldSummon->GetOrientation());
            //OldSummon->SetMap(owner->GetMap());
            //owner->GetMap()->Add(OldSummon->ToCreature());

            if (owner->GetTypeId() == TYPEID_PLAYER && OldSummon->isControlled())
                owner->ToPlayer()->PetSpellInitialize();

            return;
        }

        if (owner->GetTypeId() == TYPEID_PLAYER)
            owner->ToPlayer()->RemovePet(OldSummon, (OldSummon->getPetType() == HUNTER_PET ? PET_SLOT_DELETED : PET_SLOT_OTHER_PET), false, OldSummon->m_Stampeded);
        else
            return;
    }

    float x, y, z;
    owner->GetClosePoint(x, y, z, owner->GetObjectSize());
    Pet* pet = owner->SummonPet(petentry, x, y, z, owner->GetOrientation(), SUMMON_PET, 0, PetSlot(slot));
    if (!pet)
        return;

    if (m_caster->GetTypeId() == TYPEID_UNIT)
    {
        if (m_caster->ToCreature()->isTotem())
            pet->SetReactState(REACT_AGGRESSIVE);
        else
            pet->SetReactState(REACT_DEFENSIVE);
    }

    pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

    if (m_caster->GetTypeId() == TYPEID_UNIT)
    {
        if (m_caster->ToCreature()->isTotem())
        {
            if (pet->GetEntry() == 61029 || pet->GetEntry() == 61056)
            {
                if (Unit* owner = m_caster->GetOwner())
                {
                    if (Player* _plr = owner->ToPlayer())
                    {
                        if (pet->GetEntry() == 61029)
                        {
                            pet->addSpell(118297);  // Immolate
                            pet->addSpell(118350);  // Empower
                        }
                        else
                        {
                            pet->addSpell(118337);  // Harden Skin
                            pet->addSpell(118345);  // Pulverize
                            pet->addSpell(118347);  // Reinforce
                        }

                        _plr->PetSpellInitialize();
                    }
                }
            }
        }
    }

    // Fel energy, must be added only for visual on pet.
    if (owner->GetTypeId() == TYPEID_PLAYER)
        if (owner->ToPlayer()->getClass() == CLASS_WARLOCK)
            if (pet->GetEntry() != ENTRY_INFERNAL && pet->GetEntry() != ENTRY_ABYSSAL)
                if (!pet->HasSpell(123746))
                    pet->learnSpell(123746);

    if (pet->GetEntry() == ENTRY_VOIDWALKER || pet->GetEntry() == ENTRY_VOIDLORD || pet->GetEntry() == ENTRY_WRATHGUARD)
    {
        // Threatening Presence
        if (!pet->HasSpell(112042))
            pet->learnSpell(112042);

        // Void Reflexes
        if (!pet->HasSpell(117225))
            pet->learnSpell(117225);
    }

    // Grimoire of Supremacy increase damage
    if (pet->GetEntry() == ENTRY_FEL_IMP  || pet->GetEntry() == ENTRY_VOIDLORD   || pet->GetEntry() == ENTRY_SHIVARRA || 
        pet->GetEntry() == ENTRY_OBSERVER || pet->GetEntry() == ENTRY_WRATHGUARD || pet->GetEntry() == ENTRY_ABYSSAL  || pet->GetEntry() == ENTRY_TERRORGUARD)
        if (!pet->HasSpell(115578))
            pet->learnSpell(115578);

    // Soul burn: summon pet
    if (m_caster->HasAura(74434))
        m_caster->RemoveAurasDueToSpell(74434);

    // generate new name for summon pet
    if (petentry)
    {
        std::string new_name = sObjectMgr->GeneratePetName(petentry);
        if (!new_name.empty())
            pet->SetName(new_name);
    }

    ExecuteLogEffectSummonObject(effIndex, pet);
}

void Spell::EffectLearnPetSpell(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    if (unitTarget->ToPlayer())
    {
        EffectLearnSpell(effIndex);
        return;
    }
    Pet* pet = unitTarget->ToPet();
    if (!pet)
        return;

    SpellInfo const* learn_spellproto = sSpellMgr->GetSpellInfo(m_spellInfo->Effects[effIndex].TriggerSpell);
    if (!learn_spellproto)
        return;

    pet->learnSpell(learn_spellproto->Id);
    pet->SavePetToDB(PET_SLOT_ACTUAL_PET_SLOT);
    pet->GetOwner()->PetSpellInitialize();
}

void Spell::EffectTaunt(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    if (m_spellInfo->Id == 49560)
    {
        if (m_caster->HasAura(63335))
            return;
    }

    // this effect use before aura Taunt apply for prevent taunt already attacking target
    // for spell as marked "non effective at already attacking target"
    if (!unitTarget || !unitTarget->CanHaveThreatList()
        || unitTarget->getVictim() == m_caster)
    {
        SendCastResult(SPELL_FAILED_DONT_REPORT);
        return;
    }

    // Also use this effect to set the taunter's threat to the taunted creature's highest value
    if (unitTarget->getThreatManager().getCurrentVictim())
    {
        float myThreat = unitTarget->getThreatManager().getThreat(m_caster);
        float itsThreat = unitTarget->getThreatManager().getCurrentVictim()->getThreat();
        if (itsThreat > myThreat)
            unitTarget->getThreatManager().addThreat(m_caster, itsThreat - myThreat);
    }

    //Set aggro victim to caster
    if (!unitTarget->getThreatManager().getOnlineContainer().empty())
        if (HostileReference* forcedVictim = unitTarget->getThreatManager().getOnlineContainer().getReferenceByTarget(m_caster))
            unitTarget->getThreatManager().setCurrentVictim(forcedVictim);

    if (unitTarget->GetTypeId() == TYPEID_UNIT && unitTarget->ToCreature()->IsAIEnabled 
    && (!unitTarget->ToCreature()->HasReactState(REACT_PASSIVE) || (unitTarget->IsPetGuardianStuff() && IS_PLAYER_GUID(unitTarget->GetCharmerOrOwnerGUID()))))
    {
        // taken from case COMMAND_ATTACK:                        //spellid=1792  //ATTACK PetHandler.cpp
        if (CharmInfo* charmInfo = unitTarget->GetCharmInfo())
        {
            unitTarget->AttackStop();
            charmInfo->SetIsCommandAttack(true);
            charmInfo->SetIsAtStay(false);
            charmInfo->SetIsFollowing(false);
            charmInfo->SetIsReturning(false);
        }
        unitTarget->ToCreature()->AI()->AttackStart(m_caster);
    }
}

void Spell::EffectWeaponDmg(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive())
        return;

    // Multiple weapon dmg effect fix: Execute only the last weapon damage and handle all effects at once.
    for (uint32 j = effIndex + 1; j < MAX_SPELL_EFFECTS; ++j)
    {
        switch (m_spellInfo->Effects[j].Effect)
        {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                return;     // we must calculate only at last weapon effect
            break;
        }
    }

    // some spell specific modifiers
    float totalDamagePercentMod  = 1.0f;                    // applied to final bonus+weapon damage
    int32 fixed_bonus = 0;
    int32 spell_bonus = 0;                                  // bonus specific for spell
    float final_bonus = 0;

    switch (m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->Id)
            {
                case 69055: // Saber Lash
                case 70814: // Saber Lash
                {
                    uint32 count = 0;
                    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        if (ihit->effectMask & (1 << effIndex))
                            ++count;

                    totalDamagePercentMod /= count;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch (m_spellInfo->Id)
            {
                case 8676:  // Ambush
                {
                    // 44.7% more damage with daggers
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        if (Item* item = m_caster->ToPlayer()->GetWeaponForAttack(m_attackType, true))
                            if (item->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                                totalDamagePercentMod *= 1.447f;
                    break;
                }
                case 16511: // Hemorrhage
                {
                    // 45% more damage with daggers
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        if (Item* item = m_caster->ToPlayer()->GetWeaponForAttack(m_attackType, true))
                            if (item->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                                totalDamagePercentMod *= 1.45f;
                    break;
                }
                default:
                    break;
            }

            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Skyshatter Harness item set bonus
            // Stormstrike
            if (AuraEffectPtr aurEff = m_caster->IsScriptOverriden(m_spellInfo, 5634))
                m_caster->CastSpell(m_caster, 38430, true, NULL, aurEff);
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch (m_spellInfo->Id)
            {
                case 5221:  // Shred
                case 114236:// Shred (Glyph of Shred)
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        if (unitTarget->HasAuraState(AURA_STATE_BLEEDING))
                            totalDamagePercentMod *= 1.2f;
                    break;
                case 33876: // Mangle (Cat)
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        m_caster->ToPlayer()->AddComboPoints(unitTarget, 1, this);
                    break;
                default:
                    break;
            }

            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            float shotMod = 0;
            switch(m_spellInfo->Id)
            {
                case 53351: // Kill Shot
                {
                    shotMod = 0.375f;
                    break;
                }
                case 56641: // Steady Shot
                {
                    shotMod = 0.021f;
                    break;
                }
                case 53209: // Chimera Shot
                {
                    shotMod = 0.398f;
                    break;
                }
                case 3044: // Arcane Shot
                {
                    shotMod = 0.0483f;
                    break;
                }
                case 75:
                {
                    m_caster->Attack(unitTarget, false);
                    break;
                }
                default:
                  break;
            }
            spell_bonus += int32((shotMod*m_caster->GetTotalAttackPowerValue(RANGED_ATTACK)));
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            switch (m_spellInfo->Id)
            {
                case 45902: // Blood Strike
                {
                    float bonusPct = m_spellInfo->Effects[EFFECT_3].BasePoints * unitTarget->GetDiseasesByCaster(m_caster->GetGUID()) / 10.0f;
                    // Death Knight T8 Melee 4P Bonus
                    if (constAuraEffectPtr aurEff = m_caster->GetAuraEffect(64736, EFFECT_0))
                        AddPct(bonusPct, aurEff->GetAmount());
                    AddPct(totalDamagePercentMod, bonusPct);

                    break;
                }
                case 49020: // Obliterate
                case 66198: // Obliterate Off-Hand
                {
                    // 12.5% more damage per disease
                    float bonusPct = m_spellInfo->Effects[EFFECT_2].CalcValue(m_caster) * unitTarget->GetDiseasesByCaster(m_caster->GetGUID(), false) / 2.0f;
                    // Death Knight T8 Melee 4P Bonus
                    if (constAuraEffectPtr aurEff = m_caster->GetAuraEffect(64736, EFFECT_0))
                        AddPct(bonusPct, aurEff->GetAmount());
                    AddPct(totalDamagePercentMod, bonusPct);
                    break;
                }
                case 55050: // Heart Strike
                {
                    float bonusPct = m_spellInfo->Effects[EFFECT_2].CalcValue(m_caster) * unitTarget->GetDiseasesByCaster(m_caster->GetGUID());
                    // Death Knight T8 Melee 4P Bonus
                    if (constAuraEffectPtr aurEff = m_caster->GetAuraEffect(64736, EFFECT_0))
                        AddPct(bonusPct, aurEff->GetAmount());

                    AddPct(totalDamagePercentMod, bonusPct);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    bool normalized = false;
    float weaponDamagePercentMod = 1.0f;
    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        switch (m_spellInfo->Effects[j].Effect)
        {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                fixed_bonus += CalculateDamage(j, unitTarget);
                break;
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                fixed_bonus += CalculateDamage(j, unitTarget);
                normalized = true;
                break;
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                ApplyPct(weaponDamagePercentMod, CalculateDamage(j, unitTarget));
                break;
            default:
                break;                                      // not weapon damage effect, just skip
        }
    }

    // apply to non-weapon bonus weapon total pct effect, weapon total flat effect included in weapon damage
    if (fixed_bonus || spell_bonus)
    {
        UnitMods unitMod;
        switch (m_attackType)
        {
            default:
            case BASE_ATTACK:   unitMod = UNIT_MOD_DAMAGE_MAINHAND; break;
            case OFF_ATTACK:    unitMod = UNIT_MOD_DAMAGE_OFFHAND;  break;
            case RANGED_ATTACK: unitMod = UNIT_MOD_DAMAGE_RANGED;   break;
        }

        float weapon_total_pct = 1.0f;
        if (m_spellInfo->SchoolMask & SPELL_SCHOOL_MASK_NORMAL)
             weapon_total_pct = m_caster->GetModifierValue(unitMod, TOTAL_PCT);

        if (fixed_bonus)
            fixed_bonus = int32(fixed_bonus * weapon_total_pct);
        if (spell_bonus)
            spell_bonus = int32(spell_bonus * weapon_total_pct);
    }

    int32 weaponDamage = m_caster->CalculateDamage(m_attackType, normalized, true);

    // Sequence is important
    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        // We assume that a spell have at most one fixed_bonus
        // and at most one weaponDamagePercentMod
        switch (m_spellInfo->Effects[j].Effect)
        {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                weaponDamage += fixed_bonus;
                break;
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                weaponDamage = int32(weaponDamage* weaponDamagePercentMod);
                break;

            default: break; // Not weapon damage effect, skip.
        }
    }

    if (spell_bonus)
        weaponDamage += spell_bonus;

    if (final_bonus)
        weaponDamage *= final_bonus;

    if (totalDamagePercentMod != 1.0f)
        weaponDamage = int32(weaponDamage* totalDamagePercentMod);

    // prevent negative damage
    uint32 eff_damage(std::max(weaponDamage, 0));

    // Add melee damage bonuses (also check for negative)
    uint32 damage = m_caster->MeleeDamageBonusDone(unitTarget, eff_damage, m_attackType, m_spellInfo);

    m_damage += unitTarget->MeleeDamageBonusTaken(m_caster, damage, m_attackType, m_spellInfo);

    // Legion Strike and Mortal Cleave 
    if (m_spellInfo->Id == 30213 || m_spellInfo->Id == 115625)
    {
        uint32 count = 0;
        for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            if (ihit->effectMask & (1<<effIndex))
                ++count;

        m_damage /= count;
    }
}

void Spell::EffectThreat(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive() || !m_caster->isAlive())
        return;

    if (!unitTarget->CanHaveThreatList())
        return;

    unitTarget->AddThreat(m_caster, float(damage));
}

void Spell::EffectHealMaxHealth(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive())
        return;

    int32 addhealth = 0;

    // damage == 0 - heal for caster max health
    if (damage == 0)
        addhealth = m_caster->GetMaxHealth();
    else
        addhealth = unitTarget->GetMaxHealth() - unitTarget->GetHealth();

    m_healing += addhealth;
    m_healing = unitTarget->SpellHealingBonusTaken(m_caster, m_spellInfo, m_healing, HEAL);
}

void Spell::EffectInterruptCast(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive())
        return;

    // Deadly Throw - Interrupt spell only if used with 5 combo points
    if (m_spellInfo->Id == 26679)
        if (m_originalCaster && m_originalCaster->GetTypeId() == TYPEID_PLAYER)
            if (m_originalCaster->ToPlayer()->GetComboPoints() < 5)
                return;

    // TODO: not all spells that used this effect apply cooldown at school spells
    // also exist case: apply cooldown to interrupted cast only and to all spells
    // there is no CURRENT_AUTOREPEAT_SPELL spells that can be interrupted
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_AUTOREPEAT_SPELL; ++i)
    {
        if (Spell* spell = unitTarget->GetCurrentSpell(CurrentSpellTypes(i)))
        {
            SpellInfo const* curSpellInfo = spell->m_spellInfo;
            // check if we can interrupt spell
            if ((spell->getState() == SPELL_STATE_CASTING
                || (spell->getState() == SPELL_STATE_PREPARING && spell->GetCastTime() > 0.0f))
                && (curSpellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE || curSpellInfo->PreventionType == SPELL_PREVENTION_TYPE_UNK1)
                && ((i == CURRENT_GENERIC_SPELL && curSpellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_INTERRUPT)
                || (i == CURRENT_CHANNELED_SPELL && curSpellInfo->ChannelInterruptFlags & CHANNEL_INTERRUPT_FLAG_INTERRUPT)))
            {
                if (m_originalCaster)
                {
                    int32 duration = m_spellInfo->GetDuration();
                    unitTarget->ProhibitSpellSchool(curSpellInfo->GetSchoolMask(), unitTarget->ModSpellDuration(m_spellInfo, unitTarget, duration, false, 1 << effIndex));

                    WorldPacket interrupt(SMSG_SPELLINTERRUPTLOG);
                    ObjectGuid targetGUID = unitTarget->GetGUID();
                    ObjectGuid casterGUID = m_originalCasterGUID;

                    interrupt.WriteBit(targetGUID[5]);
                    interrupt.WriteBit(casterGUID[4]);
                    interrupt.WriteBit(casterGUID[6]);
                    interrupt.WriteBit(targetGUID[4]);
                    interrupt.WriteBit(casterGUID[1]);
                    interrupt.WriteBit(targetGUID[7]);
                    interrupt.WriteBit(targetGUID[3]);
                    interrupt.WriteBit(casterGUID[0]);
                    interrupt.WriteBit(targetGUID[1]);
                    interrupt.WriteBit(casterGUID[2]);
                    interrupt.WriteBit(casterGUID[3]);
                    interrupt.WriteBit(casterGUID[7]);
                    interrupt.WriteBit(targetGUID[2]);
                    interrupt.WriteBit(targetGUID[6]);
                    interrupt.WriteBit(casterGUID[5]);
                    interrupt.WriteBit(targetGUID[0]);

                    interrupt.FlushBits();

                    interrupt.WriteByteSeq(targetGUID[2]);
                    interrupt.WriteByteSeq(targetGUID[1]);
                    interrupt.WriteByteSeq(targetGUID[0]);
                    interrupt.WriteByteSeq(targetGUID[7]);
                    interrupt.WriteByteSeq(casterGUID[5]);
                    interrupt.WriteByteSeq(casterGUID[3]);
                    interrupt.WriteByteSeq(casterGUID[2]);
                    interrupt.WriteByteSeq(targetGUID[6]);
                    interrupt.WriteByteSeq(casterGUID[7]);
                    interrupt.WriteByteSeq(casterGUID[1]);
                    interrupt.WriteByteSeq(targetGUID[3]);
                    interrupt.WriteByteSeq(targetGUID[4]);
                    interrupt.WriteByteSeq(casterGUID[6]);

                    interrupt << uint32(m_spellInfo->Id);

                    interrupt.WriteByteSeq(casterGUID[0]);
                    interrupt.WriteByteSeq(targetGUID[5]);

                    interrupt << uint32(curSpellInfo->Id);

                    interrupt.WriteByteSeq(casterGUID[4]);

                    m_originalCaster->SendMessageToSet(&interrupt, true);
                }

                ExecuteLogEffectInterruptCast(effIndex, unitTarget, curSpellInfo->Id);
                unitTarget->InterruptSpell(CurrentSpellTypes(i), false);

                // Glyph of Rude Interruption - 58372
                if (m_spellInfo->Id == 6552 && m_originalCaster->HasAura(58372))
                    m_originalCaster->AddAura(86663, m_originalCaster);
            }
        }
    }
}

void Spell::EffectSummonObjectWild(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    uint32 gameobject_id = m_spellInfo->Effects[effIndex].MiscValue;

    GameObject* pGameObj = new GameObject;

    WorldObject* target = focusObject;
    if (!target)
        target = m_caster;

    float x, y, z;
    if (m_targets.HasDst())
        destTarget->GetPosition(x, y, z);
    else
        m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);

    Map* map = target->GetMap();

    if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), gameobject_id, map,
        m_caster->GetPhaseMask(), x, y, z, target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
    {
        delete pGameObj;
        return;
    }

    int32 duration = m_spellInfo->GetDuration();

    pGameObj->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);

    ExecuteLogEffectSummonObject(effIndex, pGameObj);

    // Wild object not have owner and check clickable by players
    map->AddToMap(pGameObj);

    if (pGameObj->GetGoType() == GAMEOBJECT_TYPE_FLAGDROP && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = m_caster->ToPlayer();
        Battleground* bg = player->GetBattleground();

        switch (pGameObj->GetMapId())
        {
            case 489:                                       //WS
            {
                if (bg && bg->GetTypeID(true) == BATTLEGROUND_WS && bg->GetStatus() == STATUS_IN_PROGRESS)
                {
                    uint32 team = ALLIANCE;

                    if (player->GetTeam() == team)
                        team = HORDE;

                    ((BattlegroundWS*)bg)->SetDroppedFlagGUID(pGameObj->GetGUID(), team);
                }
                break;
            }
            case 566:                                       //EY
            {
                if (bg && (bg->GetTypeID(true) == BATTLEGROUND_EY || bg->GetTypeID(true) == BATTLEGROUND_EYR) && bg->GetStatus() == STATUS_IN_PROGRESS)
                {
                    ((BattlegroundEY*)bg)->SetDroppedFlagGUID(pGameObj->GetGUID());
                }
                break;
            }
            case 726:                                       //TP
            {
                if (bg && bg->GetTypeID(true) == BATTLEGROUND_TP && bg->GetStatus() == STATUS_IN_PROGRESS)
                {
                    uint32 team = TEAM_ALLIANCE;

                    if (player->GetTeamId() == team)
                        team = TEAM_HORDE;

                    ((BattlegroundTP*)bg)->SetDroppedFlagGUID(pGameObj->GetGUID(), team);
                }
                break;
            }
        }
    }

    if (uint32 linkedEntry = pGameObj->GetGOInfo()->GetLinkedGameObjectEntry())
    {
        GameObject* linkedGO = new GameObject;
        if (linkedGO->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), linkedEntry, map,
            m_caster->GetPhaseMask(), x, y, z, target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
        {
            linkedGO->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
            linkedGO->SetSpellId(m_spellInfo->Id);

            ExecuteLogEffectSummonObject(effIndex, linkedGO);

            // Wild object not have owner and check clickable by players
            map->AddToMap(linkedGO);
        }
        else
        {
            delete linkedGO;
            linkedGO = NULL;
            return;
        }
    }
}

void Spell::EffectScriptEffect(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    // TODO: we must implement hunter pet summon at login there (spell 6962)
    switch (m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->Id)
            {
                case 59077: // Remove Seaforium
                    if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER)
                        m_caster->ToPlayer()->DestroyItemCount(39213, 1, true);
                    break;
                case 111922:// Fortitude (Runescroll of Fortitude III)
                    if (!unitTarget)
                        m_caster->CastSpell(m_caster, 111923, true);
                    else
                        m_caster->CastSpell(unitTarget, 111923, true);
                    break;
                case 86958: // Mobile Banking
                    m_caster->CastSpell(m_caster, 88304, true);
                    break;
                case 63975: // Glyph of Backstab
                {
                    if (!unitTarget)
                        break;

                    // search our Rupture aura on target
                    if (constAuraEffectPtr aurEff = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x00100000, 0, 0, m_caster->GetGUID()))
                    {
                        uint32 countMin = aurEff->GetBase()->GetMaxDuration();
                        uint32 countMax = 12000; // this can be wrong, duration should be based on combo-points
                        countMax += m_caster->HasAura(56801) ? 4000 : 0;

                        if (countMin < countMax)
                        {
                            aurEff->GetBase()->SetDuration(uint32(aurEff->GetBase()->GetDuration() + 3000));
                            aurEff->GetBase()->SetMaxDuration(countMin + 2000);
                        }

                    }
                    return;
                }
                case 69961: // Glyph of Scourge Strike
                {
                    if (!unitTarget)
                        break;

                    Unit::AuraEffectList const &mPeriodic = unitTarget->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraEffectList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
                    {
                        constAuraEffectPtr aurEff = *i;
                        SpellInfo const* spellInfo = aurEff->GetSpellInfo();
                        // search our Blood Plague and Frost Fever on target
                        if (spellInfo->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && spellInfo->SpellFamilyFlags[2] & 0x2 &&
                            aurEff->GetCasterGUID() == m_caster->GetGUID())
                        {
                            uint32 countMin = aurEff->GetBase()->GetMaxDuration();
                            uint32 countMax = spellInfo->GetMaxDuration();

                            // this Glyph
                            countMax += 9000;
                            // talent Epidemic
                            if (constAuraEffectPtr epidemic = m_caster->GetAuraEffect(SPELL_AURA_ADD_FLAT_MODIFIER, SPELLFAMILY_DEATHKNIGHT, 234, EFFECT_0))
                                countMax += epidemic->GetAmount();

                            if (countMin < countMax)
                            {
                                aurEff->GetBase()->SetDuration(aurEff->GetBase()->GetDuration() + 3000);
                                aurEff->GetBase()->SetMaxDuration(countMin + 3000);
                            }
                        }
                    }
                    return;
                }
                case 45204: // Clone Me!
                    if (unitTarget)
                        m_caster->CastSpell(unitTarget, damage, true);
                    break;
                case 55693:                                 // Remove Collapsing Cave Aura
                    if (!unitTarget)
                        return;
                    unitTarget->RemoveAurasDueToSpell(m_spellInfo->Effects[effIndex].CalcValue());
                    break;
                case 26275: // PX-238 Winter Wondervolt TRAP
                {
                    uint32 spells[4] = { 26272, 26157, 26273, 26274 };

                    // check presence
                    for (uint8 j = 0; j < 4; ++j)
                        if (unitTarget->HasAuraEffect(spells[j], 0))
                            return;

                    // select spell
                    uint32 iTmpSpellId = spells[urand(0, 3)];

                    // cast
                    unitTarget->CastSpell(unitTarget, iTmpSpellId, true);
                    return;
                }
                case 8856:  // Bending Shinbone
                {
                    if (!itemTarget && m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    uint32 spell_id = roll_chance_i(20) ? 8854 : 8855;

                    m_caster->CastSpell(m_caster, spell_id, true, NULL);
                    return;
                }
                case 24590: // Brittle Armor - need remove one 24575 Brittle Armor aura
                    if (unitTarget)
                        unitTarget->RemoveAuraFromStack(24575);
                    return;
                case 26465: // Mercurial Shield - need remove one 26464 Mercurial Shield aura
                    if (unitTarget)
                        unitTarget->RemoveAuraFromStack(26464);
                    return;
                case 22539:
                case 22972:
                case 22975:
                case 22976:
                case 22977:
                case 22978:
                case 22979:
                case 22980:
                case 22981:
                case 22982:
                case 22983:
                case 22984:
                case 22985: // Shadow Flame (All script effects, not just end ones to prevent player from dodging the last triggered spell)
                {
                    if (!unitTarget || !unitTarget->isAlive())
                        return;

                    // Onyxia Scale Cloak
                    if (unitTarget->HasAura(22683))
                        return;

                    // Shadow Flame
                    m_caster->CastSpell(unitTarget, 22682, true);
                    return;
                }
                case 17512: // Piccolo of the Flaming Fire
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;
                    unitTarget->HandleEmote(EMOTE_STATE_DANCE);
                    return;
                }
                case 28374: // Decimate
                case 54426:
                    if (unitTarget)
                    {
                        int32 damage = int32(unitTarget->GetHealth()) - int32(unitTarget->CountPctFromMaxHealth(5));
                        if (damage > 0)
                            m_caster->CastCustomSpell(28375, SPELLVALUE_BASE_POINT0, damage, unitTarget);
                    }
                    return;
                case 29830: // Mirren's Drinking Hat
                {
                    uint32 item = 0;
                    switch (urand(1, 6))
                    {
                        case 1:
                        case 2:
                        case 3:
                            item = 23584; break;            // Loch Modan Lager
                        case 4:
                        case 5:
                            item = 23585; break;            // Stouthammer Lite
                        case 6:
                            item = 23586; break;            // Aerie Peak Pale Ale
                    }
                    if (item)
                        DoCreateItem(effIndex, item);
                    break;
                }
                case 20589: // Escape artist
                case 30918: // Improved Sprint
                {
                    // Removes snares and roots.
                    unitTarget->RemoveMovementImpairingAuras();
                    break;
                }
                case 32307: // Plant Warmaul Ogre Banner
                    if (Player* caster = m_caster->ToPlayer())
                    {
                        caster->RewardPlayerAndGroupAtEvent(18388, unitTarget);
                        if (Creature* target = unitTarget->ToCreature())
                        {
                            target->setDeathState(CORPSE);
                            target->RemoveCorpse();
                        }
                    }
                    break;
                case 41931: // Mug Transformation
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    uint8 bag = 19;
                    uint8 slot = 0;
                    Item* item = NULL;

                    while (bag) // 256 = 0 due to var type
                    {
                        item = m_caster->ToPlayer()->GetItemByPos(bag, slot);
                        if (item && item->GetEntry() == 38587)
                            break;

                        ++slot;
                        if (slot == 39)
                        {
                            slot = 0;
                            ++bag;
                        }
                    }
                    if (bag)
                    {
                        if (m_caster->ToPlayer()->GetItemByPos(bag, slot)->GetCount() == 1) m_caster->ToPlayer()->RemoveItem(bag, slot, true);
                        else m_caster->ToPlayer()->GetItemByPos(bag, slot)->SetCount(m_caster->ToPlayer()->GetItemByPos(bag, slot)->GetCount()-1);
                        // Spell 42518 (Braufest - Gratisprobe des Braufest herstellen)
                        m_caster->CastSpell(m_caster, 42518, true);
                        return;
                    }
                    break;
                }
                case 45141: // Brutallus - Burn
                case 45151:
                {
                    //Workaround for Range ... should be global for every ScriptEffect
                    float radius = m_spellInfo->Effects[effIndex].CalcRadius();
                    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER && unitTarget->GetDistance(m_caster) >= radius && !unitTarget->HasAura(46394) && unitTarget != m_caster)
                        unitTarget->CastSpell(unitTarget, 46394, true);

                    break;
                }
                case 46203: // Goblin Weather Machine
                {
                    if (!unitTarget)
                        return;

                    uint32 spellId = 0;
                    switch (rand() % 4)
                    {
                        case 0: spellId = 46740; break;
                        case 1: spellId = 46739; break;
                        case 2: spellId = 46738; break;
                        case 3: spellId = 46736; break;
                    }
                    unitTarget->CastSpell(unitTarget, spellId, true);
                    break;
                }
                case 46642: // 5, 000 Gold
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    unitTarget->ToPlayer()->ModifyMoney(5000 * GOLD);

                    break;
                }
                case 47770: // Roll Dice - Decahedral Dwarven Dice
                {
                    char buf[128];
                    const char *gender = "his";
                    if (m_caster->getGender() > 0)
                        gender = "her";
                    sprintf(buf, "%s rubs %s [Decahedral Dwarven Dice] between %s hands and rolls. One %u and one %u.", m_caster->GetName(), gender, gender, urand(1, 10), urand(1, 10));
                    m_caster->MonsterTextEmote(buf, 0);
                    break;
                }
                case 47776: // Roll 'dem Bones - Worn Troll Dice
                {
                    char buf[128];
                    const char *gender = "his";
                    if (m_caster->getGender() > 0)
                        gender = "her";
                    sprintf(buf, "%s causually tosses %s [Worn Troll Dice]. One %u and one %u.", m_caster->GetName(), gender, urand(1, 6), urand(1, 6));
                    m_caster->MonsterTextEmote(buf, 0);
                    break;
                }
                case 50725: // Vigilance
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Remove Taunt cooldown
                    unitTarget->ToPlayer()->RemoveSpellCooldown(355, true);

                    return;
                }
                case 51519: // Death Knight Initiate Visual
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                        return;

                    uint32 iTmpSpellId = 0;
                    switch (unitTarget->GetDisplayId())
                    {
                        case 25369:
                            iTmpSpellId = 51552;
                            break; // bloodelf female
                        case 25373:
                            iTmpSpellId = 51551;
                            break; // bloodelf male
                        case 25363:
                            iTmpSpellId = 51542;
                            break; // draenei female
                        case 25357:
                            iTmpSpellId = 51541;
                            break; // draenei male
                        case 25361:
                            iTmpSpellId = 51537;
                            break; // dwarf female
                        case 25356:
                            iTmpSpellId = 51538;
                            break; // dwarf male
                        case 25372:
                            iTmpSpellId = 51550;
                            break; // forsaken female
                        case 25367:
                            iTmpSpellId = 51549;
                            break; // forsaken male
                        case 25362:
                            iTmpSpellId = 51540;
                            break; // gnome female
                        case 25359:
                            iTmpSpellId = 51539;
                            break; // gnome male
                        case 25355:
                            iTmpSpellId = 51534;
                            break; // human female
                        case 25354:
                            iTmpSpellId = 51520;
                            break; // human male
                        case 25360:
                            iTmpSpellId = 51536;
                            break; // nightelf female
                        case 25358:
                            iTmpSpellId = 51535;
                            break; // nightelf male
                        case 25368:
                            iTmpSpellId = 51544;
                            break; // orc female
                        case 25364:
                            iTmpSpellId = 51543;
                            break; // orc male
                        case 25371:
                            iTmpSpellId = 51548;
                            break; // tauren female
                        case 25366:
                            iTmpSpellId = 51547;
                            break; // tauren male
                        case 25370:
                            iTmpSpellId = 51545;
                            break; // troll female
                        case 25365:
                            iTmpSpellId = 51546;
                            break; // troll male
                        default:
                            return;
                    }

                    unitTarget->CastSpell(unitTarget, iTmpSpellId, true);
                    Creature* npc = unitTarget->ToCreature();
                    npc->LoadEquipment(npc->GetEquipmentId());
                    return;
                }
                case 51770: // Emblazon Runeblade
                {
                    if (!m_originalCaster)
                        return;

                    m_originalCaster->CastSpell(m_originalCaster, damage, false);
                    break;
                }
                // Deathbolt from Thalgran Blightbringer
                // reflected by Freya's Ward
                // Retribution by Sevenfold Retribution
                case 51854:
                {
                    if (!unitTarget)
                        return;
                    if (unitTarget->HasAura(51845))
                        unitTarget->CastSpell(m_caster, 51856, true);
                    else
                        m_caster->CastSpell(unitTarget, 51855, true);
                    break;
                }
                case 51904: // Summon Ghouls On Scarlet Crusade
                {
                    if (!m_targets.HasDst())
                        return;

                    float x, y, z;
                    float radius = m_spellInfo->Effects[effIndex].CalcRadius();
                    for (uint8 i = 0; i < 15; ++i)
                    {
                        m_caster->GetRandomPoint(*destTarget, radius, x, y, z);
                        m_caster->CastSpell(x, y, z, 54522, true);
                    }
                    break;
                }
                case 52173: // Coyote Spirit Despawn
                case 60243: // Blood Parrot Despawn
                    if (unitTarget->GetTypeId() == TYPEID_UNIT && unitTarget->ToCreature()->isSummon())
                        unitTarget->ToTempSummon()->UnSummon();
                    return;
                case 52479: // Gift of the Harvester
                    if (unitTarget && m_originalCaster)
                        m_originalCaster->CastSpell(unitTarget, urand(0, 1) ? damage : 52505, true);
                    return;
                case 53110: // Devour Humanoid
                    if (unitTarget)
                        unitTarget->CastSpell(m_caster, damage, true);
                    return;
                case 57347: // Retrieving (Wintergrasp RP-GG pickup spell)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    unitTarget->ToCreature()->DespawnOrUnsummon();

                    return;
                }
                case 57349: // Drop RP-GG (Wintergrasp RP-GG at death drop spell)
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Delete item from inventory at death
                    m_caster->ToPlayer()->DestroyItemCount(damage, 5, true);

                    return;
                }
                case 58418:                                 // Portal to Orgrimmar
                case 58420:                                 // Portal to Stormwind
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || effIndex != 0)
                        return;

                    uint32 spellID = m_spellInfo->Effects[EFFECT_0].CalcValue();
                    uint32 questID = m_spellInfo->Effects[EFFECT_1].CalcValue();

                    if (unitTarget->ToPlayer()->GetQuestStatus(questID) == QUEST_STATUS_COMPLETE)
                        unitTarget->CastSpell(unitTarget, spellID, true);

                    return;
                }
                case 58941:                                 // Rock Shards
                    if (unitTarget && m_originalCaster)
                    {
                        for (uint32 i = 0; i < 3; ++i)
                        {
                            m_originalCaster->CastSpell(unitTarget, 58689, true);
                            m_originalCaster->CastSpell(unitTarget, 58692, true);
                        }
                        if (((InstanceMap*)m_originalCaster->GetMap())->GetDifficulty() == DUNGEON_DIFFICULTY_NORMAL)
                        {
                            m_originalCaster->CastSpell(unitTarget, 58695, true);
                            m_originalCaster->CastSpell(unitTarget, 58696, true);
                        }
                        else
                        {
                            m_originalCaster->CastSpell(unitTarget, 60883, true);
                            m_originalCaster->CastSpell(unitTarget, 60884, true);
                        }
                    }
                    return;
                case 58983: // Big Blizzard Bear
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts and client crashes upon dismounting
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill
                    if (uint16 skillval = unitTarget->ToPlayer()->GetSkillValue(SKILL_RIDING))
                    {
                        if (skillval >= 150)
                            unitTarget->CastSpell(unitTarget, 58999, true);
                        else
                            unitTarget->CastSpell(unitTarget, 58997, true);
                    }
                    return;
                }
                case 63845: // Create Lance
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (m_caster->ToPlayer()->GetTeam() == ALLIANCE)
                        m_caster->CastSpell(m_caster, 63914, true);
                    else
                        m_caster->CastSpell(m_caster, 63919, true);
                    return;
                }
                case 59317:                                 // Teleporting
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // return from top
                    if (unitTarget->ToPlayer()->GetAreaId() == 4637)
                        unitTarget->CastSpell(unitTarget, 59316, true);
                    // teleport atop
                    else
                        unitTarget->CastSpell(unitTarget, 59314, true);

                    return;
                // random spell learn instead placeholder
                case 60893:                                 // Northrend Alchemy Research
                case 61177:                                 // Northrend Inscription Research
                case 61288:                                 // Minor Inscription Research
                case 61756:                                 // Northrend Inscription Research (FAST QA VERSION)
                case 64323:                                 // Book of Glyph Mastery
                case 112996:                                // Scroll of Wisdom
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // learn random explicit discovery recipe (if any)
                    if (uint32 discoveredSpell = GetExplicitDiscoverySpell(m_spellInfo->Id, m_caster->ToPlayer()))
                        m_caster->ToPlayer()->learnSpell(discoveredSpell, false);
                    return;
                }
                case 62482: // Grab Crate
                {
                    if (unitTarget)
                    {
                        if (Unit* seat = m_caster->GetVehicleBase())
                        {
                            if (Unit* parent = seat->GetVehicleBase())
                            {
                                // TODO: a hack, range = 11, should after some time cast, otherwise too far
                                m_caster->CastSpell(parent, 62496, true);
                                unitTarget->CastSpell(parent, m_spellInfo->Effects[EFFECT_0].CalcValue());
                            }
                        }
                    }
                    return;
                }
                // Stoneclaw Totem
                case 55328: // Rank 1
                case 55329: // Rank 2
                case 55330: // Rank 3
                case 55332: // Rank 4
                case 55333: // Rank 5
                case 55335: // Rank 6
                case 55278: // Rank 7
                case 58589: // Rank 8
                case 58590: // Rank 9
                case 58591: // Rank 10
                {
                    int32 basepoints0 = damage;
                    // Cast Absorb on totems
                    for (uint8 slot = SUMMON_SLOT_TOTEM; slot < MAX_TOTEM_SLOT; ++slot)
                    {
                        if (!unitTarget->m_SummonSlot[slot])
                            continue;

                        Creature* totem = unitTarget->GetMap()->GetCreature(unitTarget->m_SummonSlot[slot]);
                        if (totem && totem->isTotem())
                        {
                            m_caster->CastCustomSpell(totem, 55277, &basepoints0, NULL, NULL, true);
                        }
                    }
                    // Glyph of Stoneclaw Totem
                    if (AuraEffectPtr aur=unitTarget->GetAuraEffect(63298, 0))
                    {
                        basepoints0 *= aur->GetAmount();
                        m_caster->CastCustomSpell(unitTarget, 55277, &basepoints0, NULL, NULL, true);
                    }
                    break;
                }
                case 66545: //Summon Memory
                {
                    uint8 uiRandom = urand(0, 25);
                    uint32 uiSpells[26] = {66704, 66705, 66706, 66707, 66709, 66710, 66711, 66712, 66713, 66714, 66715, 66708, 66708, 66691, 66692, 66694, 66695, 66696, 66697, 66698, 66699, 66700, 66701, 66702, 66703, 66543};

                    m_caster->CastSpell(m_caster, uiSpells[uiRandom], true);
                    break;
                }
                case 45668:                                 // Ultra-Advanced Proto-Typical Shortening Blaster
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                        return;

                    if (roll_chance_i(50))                  // chance unknown, using 50
                        return;

                    static uint32 const spellPlayer[5] =
                    {
                        45674,                            // Bigger!
                        45675,                            // Shrunk
                        45678,                            // Yellow
                        45682,                            // Ghost
                        45684                             // Polymorph
                    };

                    static uint32 const spellTarget[5] =
                    {
                        45673,                            // Bigger!
                        45672,                            // Shrunk
                        45677,                            // Yellow
                        45681,                            // Ghost
                        45683                             // Polymorph
                    };

                    m_caster->CastSpell(m_caster, spellPlayer[urand(0, 4)], true);
                    unitTarget->CastSpell(unitTarget, spellTarget[urand(0, 4)], true);
                    break;
                }
                // Fortune Cookie
                case 87604:
                {
                    DoCreateItem(effIndex, urand(62552, 62591));
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_POTION:
        {
            switch (m_spellInfo->Id)
            {
                // Netherbloom
                case 28702:
                {
                    if (!unitTarget)
                        return;
                    // 25% chance of casting a random buff
                    if (roll_chance_i(75))
                        return;

                    // triggered spells are 28703 to 28707
                    // Note: some sources say, that there was the possibility of
                    //       receiving a debuff. However, this seems to be removed by a patch.
                    const uint32 spellid = 28703;

                    // don't overwrite an existing aura
                    for (uint8 i = 0; i < 5; ++i)
                        if (unitTarget->HasAura(spellid + i))
                            return;
                    unitTarget->CastSpell(unitTarget, spellid+urand(0, 4), true);
                    break;
                }

                // Nightmare Vine
                case 28720:
                {
                    if (!unitTarget)
                        return;
                    // 25% chance of casting Nightmare Pollen
                    if (roll_chance_i(75))
                        return;
                    unitTarget->CastSpell(unitTarget, 28721, true);
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            switch (m_spellInfo->Id)
            {
                case 125048:// Fetch
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (!unitTarget)
                        return;

                    Player* player = m_caster->ToPlayer();
                    Pet* pet = player->GetPet();
                    if (!pet)
                        return;

                    pet->GetMotionMaster()->MoveCharge(unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ());
                    player->SendLoot(unitTarget->GetGUID(), LOOT_CORPSE, true);

                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Pestilence
            if (m_spellInfo->SpellFamilyFlags[1]&0x10000)
            {
                // Get diseases on target of spell
                if (m_targets.GetUnitTarget() && m_targets.GetUnitTarget() != unitTarget)
                {
                    uint8 pestilcenceModifier = /*m_spellInfo->Effects[EFFECT_1].CalcValue(m_caster)*/ 100;
                    // And spread them on target
                    // Blood Plague
                    if (m_targets.GetUnitTarget()->GetAura(55078))
                    {
                        m_caster->CastSpell(unitTarget, 55078, true, NULL, NULL, 0, float(pestilcenceModifier / 100.0f));
                        m_caster->AddAura(63687, unitTarget);                           // Cosmetic - Pestilence State
                        m_targets.GetUnitTarget()->CastSpell(unitTarget, 91939, true);  // Cosmetic - Send Diseases on target
                    }
                    // Frost Fever
                    if (m_targets.GetUnitTarget()->GetAura(55095))
                    {
                        m_caster->CastSpell(unitTarget, 55095, true, NULL, NULL, 0, float(pestilcenceModifier / 100.0f));
                        m_caster->AddAura(63687, unitTarget);                           // Cosmetic - Pestilence State
                        m_targets.GetUnitTarget()->CastSpell(unitTarget, 91939, true);  // Cosmetic - Send Diseases on target
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Shattering Throw
            if (m_spellInfo->SpellFamilyFlags[1] & 0x00400000)
            {
                if (!unitTarget)
                    return;
                // remove shields, will still display immune to damage part
                unitTarget->RemoveAurasWithMechanic(1<<MECHANIC_IMMUNE_SHIELD, AURA_REMOVE_BY_ENEMY_SPELL);
                return;
            }
            // Bladestorm
            if (m_spellInfo->Id == 46924)
            {
                m_caster->RemoveAurasWithMechanic((1<<MECHANIC_DISARM)|(1<<MECHANIC_SNARE)|(1<<MECHANIC_ROOT));
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            switch (m_spellInfo->Id)
            {
                // Earthquake (stun effect)
                case 77478:
                    if (roll_chance_i(10))
                        m_caster->CastSpell(unitTarget, 77505, true);
                    break;
                // Taming the Flames, Item - Shaman T12 Elemental 2P Bonus
                case 99202:
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    m_caster->ToPlayer()->ReduceSpellCooldown(2894, 4000); 
                    break;
            }
        }
    }

    // normal DB scripted effect
    m_caster->GetMap()->ScriptsStart(sSpellScripts, uint32(m_spellInfo->Id | (effIndex << 24)), m_caster, unitTarget);
}

void Spell::EffectSanctuary(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    unitTarget->getHostileRefManager().UpdateVisibility();

    unitTarget->CombatStop(false);

    // Vanish allows to remove all threat and cast regular stealth so other spells can be used
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->Id == 131369)
    {
        m_caster->ToPlayer()->RemoveAurasByType(SPELL_AURA_MOD_ROOT);
        // Overkill
        if (m_caster->ToPlayer()->HasSpell(58426))
           m_caster->CastSpell(m_caster, 58427, true);
    }
    else if (!IsTriggered())
        unitTarget->m_lastSanctuaryTime = getMSTime();
}

void Spell::EffectAddComboPoints(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    if (!m_caster->m_movedPlayer)
        return;

    if (damage <= 0)
        return;

    switch (m_spellInfo->Id)
    {
        case 51723: // Fan of Knives
            if (!m_caster->ToPlayer())
                break;

            if (m_caster->ToPlayer()->GetSelectedUnit() != unitTarget)
                return;
            break;
        case 62078: // Swipe (cat)
            if (m_caster->m_movedPlayer->GetComboTarget() != unitTarget->GetGUID())
                return;
            break;
        default:
            break;
    }

    m_caster->m_movedPlayer->AddComboPoints(unitTarget, damage, this);
}

void Spell::EffectDuel(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* caster = m_caster->ToPlayer();
    Player* target = unitTarget->ToPlayer();

    // caster or target already have requested duel
    if (caster->duel || target->duel || !target->GetSocial() || target->GetSocial()->HasIgnore(caster->GetGUIDLow()))
        return;

    // Players can only fight a duel in zones with this flag
    AreaTableEntry const* casterAreaEntry = GetAreaEntryByAreaID(caster->GetAreaId());
    if (casterAreaEntry && !(casterAreaEntry->flags & AREA_FLAG_ALLOW_DUELS))
    {
        SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
        return;
    }

    AreaTableEntry const* targetAreaEntry = GetAreaEntryByAreaID(target->GetAreaId());
    if (targetAreaEntry && !(targetAreaEntry->flags & AREA_FLAG_ALLOW_DUELS))
    {
        SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
        return;
    }

    //CREATE DUEL FLAG OBJECT
    GameObject* pGameObj = new GameObject;

    uint32 gameobject_id = m_spellInfo->Effects[effIndex].MiscValue;

    Map* map = m_caster->GetMap();
    if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), gameobject_id,
        map, m_caster->GetPhaseMask(),
        m_caster->GetPositionX()+(unitTarget->GetPositionX()-m_caster->GetPositionX())/2,
        m_caster->GetPositionY()+(unitTarget->GetPositionY()-m_caster->GetPositionY())/2,
        m_caster->GetPositionZ(),
        m_caster->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 0, GO_STATE_READY))
    {
        delete pGameObj;
        return;
    }

    pGameObj->SetUInt32Value(GAMEOBJECT_FACTION, m_caster->getFaction());
    pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel()+1);
    int32 duration = m_spellInfo->GetDuration();
    pGameObj->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);

    ExecuteLogEffectSummonObject(effIndex, pGameObj);

    m_caster->AddGameObject(pGameObj);
    map->AddToMap(pGameObj);
    //END

    // Send request
    WorldPacket data(SMSG_DUEL_REQUESTED);

    ObjectGuid guid2 = pGameObj->GetGUID();
    ObjectGuid guid1 = caster->GetGUID();

    data.WriteBit(guid1[2]);
    data.WriteBit(guid2[7]);
    data.WriteBit(guid2[1]);
    data.WriteBit(guid2[4]);
    data.WriteBit(guid1[6]);
    data.WriteBit(guid2[0]);
    data.WriteBit(guid2[3]);
    data.WriteBit(guid2[5]);
    data.WriteBit(guid2[6]);
    data.WriteBit(guid2[2]);
    data.WriteBit(guid1[7]);
    data.WriteBit(guid1[3]);
    data.WriteBit(guid1[0]);
    data.WriteBit(guid1[5]);
    data.WriteBit(guid1[4]);
    data.WriteBit(guid1[1]);

    data.WriteByteSeq(guid2[2]);
    data.WriteByteSeq(guid2[7]);
    data.WriteByteSeq(guid1[7]);
    data.WriteByteSeq(guid1[2]);
    data.WriteByteSeq(guid2[4]);
    data.WriteByteSeq(guid2[5]);
    data.WriteByteSeq(guid2[3]);
    data.WriteByteSeq(guid2[0]);
    data.WriteByteSeq(guid1[6]);
    data.WriteByteSeq(guid1[5]);
    data.WriteByteSeq(guid2[1]);
    data.WriteByteSeq(guid1[4]);
    data.WriteByteSeq(guid2[6]);
    data.WriteByteSeq(guid1[3]);
    data.WriteByteSeq(guid1[0]);
    data.WriteByteSeq(guid1[1]);


    caster->GetSession()->SendPacket(&data);
    target->GetSession()->SendPacket(&data);

    // create duel-info
    DuelInfo* duel   = new DuelInfo;
    duel->initiator  = caster;
    duel->opponent   = target;
    duel->startTime  = 0;
    duel->startTimer = 0;
    duel->isMounted  = (GetSpellInfo()->Id == 62875); // Mounted Duel
    caster->duel     = duel;

    DuelInfo* duel2   = new DuelInfo;
    duel2->initiator  = caster;
    duel2->opponent   = caster;
    duel2->startTime  = 0;
    duel2->startTimer = 0;
    duel2->isMounted  = (GetSpellInfo()->Id == 62875); // Mounted Duel
    target->duel      = duel2;

    caster->SetUInt64Value(PLAYER_DUEL_ARBITER, pGameObj->GetGUID());
    target->SetUInt64Value(PLAYER_DUEL_ARBITER, pGameObj->GetGUID());

    sScriptMgr->OnPlayerDuelRequest(target, caster);
}

void Spell::EffectStuck(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!sWorld->getBoolConfig(CONFIG_CAST_UNSTUCK))
        return;

    Player* target = (Player*)m_caster;

    if (target->isInFlight())
        return;

    if (target->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        target->RepopAtGraveyard();
    else
        target->TeleportTo(target->GetStartPosition(), TELE_TO_SPELL);
    // homebind location is loaded always
    // target->TeleportTo(target->m_homebindMapId, target->m_homebindX, target->m_homebindY, target->m_homebindZ, target->GetOrientation(), (m_caster == m_caster ? TELE_TO_SPELL : 0));

    // Stuck spell trigger Hearthstone cooldown
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(8690);
    if (!spellInfo)
        return;
    Spell spell(target, spellInfo, TRIGGERED_FULL_MASK);
    spell.SendSpellCooldown();
}

void Spell::EffectSummonPlayer(SpellEffIndex /*effIndex*/)
{
    // workaround - this effect should not use target map
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    // Evil Twin (ignore player summon, but hide this for summoner)
    if (unitTarget->HasAura(23445))
        return;

    float x, y, z;
    m_caster->GetPosition(x, y, z);

    unitTarget->ToPlayer()->SetSummonPoint(m_caster->GetMapId(), x, y, z);

    WorldPacket data(SMSG_SUMMON_REQUEST, 8+4+4);
    ObjectGuid summonerGuid = m_caster->GetGUID();

    uint8 bitsOrder[8] = { 2, 4, 6, 3, 0, 7, 1, 5 };
    data.WriteBitInOrder(summonerGuid, bitsOrder);

    data.WriteByteSeq(summonerGuid[1]);
    data.WriteByteSeq(summonerGuid[6]);
    data << uint32(m_caster->GetZoneId());                      // summoner zone
    data.WriteByteSeq(summonerGuid[0]);
    data.WriteByteSeq(summonerGuid[3]);
    data.WriteByteSeq(summonerGuid[2]);
    data.WriteByteSeq(summonerGuid[4]);
    data << uint32(MAX_PLAYER_SUMMON_DELAY * IN_MILLISECONDS);  // auto decline after msecs
    data.WriteByteSeq(summonerGuid[5]);
    
    data.WriteByteSeq(summonerGuid[7]);
    
    
    unitTarget->ToPlayer()->GetSession()->SendPacket(&data);
}

void Spell::EffectActivateObject(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!gameObjTarget)
        return;

    ScriptInfo activateCommand;
    activateCommand.command = SCRIPT_COMMAND_ACTIVATE_OBJECT;

    // int32 unk = m_spellInfo->Effects[effIndex].MiscValue; // This is set for EffectActivateObject spells; needs research

    /*switch (m_spellInfo->Id)
    {
        case 105847: // Seal Armor Breach (left), Spine of Deathwing, Dragon Soul
        case 105848: // Seal Armor Breach (right), Spine of Deathwing, Dragon Soul
        case 105363: // Breach Armor (left), Spine of Deathwing, Dragon Soul
        case 105385: // Breach Armor (right), Spine of Deathwing, Dragon Soul
        case 105366: // Plate Fly Off (left), Spine of Deathwing, Dragon Soul
        case 105384: // Plate Fly Off (right), Spine of Deathwing, Dragon Soul
            if (gameObjTarget->GetEntry() == 209623 || gameObjTarget->GetEntry() == 209631 || gameObjTarget->GetEntry() == 209632)
            {
                // Send anim kit
                gameObjTarget->SendActivateAnim(m_spellInfo->Effects[effIndex].MiscValueB);
                return;
            }
            break;
    }*/

    gameObjTarget->GetMap()->ScriptCommandStart(activateCommand, 0, m_caster, gameObjTarget);
}

void Spell::EffectApplyGlyph(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER || m_glyphIndex >= MAX_GLYPH_SLOT_INDEX)
        return;

    Player* player = (Player*)m_caster;

    // glyph sockets level requirement
    uint8 minLevel = 0;
    switch (m_glyphIndex)
    {
        case 0:
        case 1:
        case 6: minLevel = 25; break;
        case 2:
        case 3:
        case 7: minLevel = 50; break;
        case 4:
        case 5:
        case 8: minLevel = 75; break;
    }

    if (minLevel && m_caster->getLevel() < minLevel)
    {
        SendCastResult(SPELL_FAILED_GLYPH_SOCKET_LOCKED);
        return;
    }

    // apply new one
    if (uint32 glyph = m_spellInfo->Effects[effIndex].MiscValue)
    {
        if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            if (GlyphSlotEntry const* gs = sGlyphSlotStore.LookupEntry(player->GetGlyphSlot(m_glyphIndex)))
            {
                if (gp->TypeFlags != gs->TypeFlags)
                {
                    SendCastResult(SPELL_FAILED_INVALID_GLYPH);
                    return;                                 // glyph slot mismatch
                }
            }

            // remove old glyph
            if (uint32 oldglyph = player->GetGlyph(player->GetActiveSpec(), m_glyphIndex))
            {
                if (GlyphPropertiesEntry const* old_gp = sGlyphPropertiesStore.LookupEntry(oldglyph))
                {
                    player->RemoveAurasDueToSpell(old_gp->SpellId);
                    player->SetGlyph(m_glyphIndex, 0);
                }
            }

            player->CastSpell(m_caster, gp->SpellId, true);
            player->SetGlyph(m_glyphIndex, glyph);
            player->SendTalentsInfoData(false);
        }
    }
    else
    {
        // remove old glyph
        if (uint32 oldglyph = player->GetGlyph(player->GetActiveSpec(), m_glyphIndex))
        {
            if (GlyphPropertiesEntry const* old_gp = sGlyphPropertiesStore.LookupEntry(oldglyph))
            {
                player->RemoveAurasDueToSpell(old_gp->SpellId);
                player->SetGlyph(m_glyphIndex, 0);
                player->SendTalentsInfoData(false);
            }
        }
    }
}

void Spell::EffectEnchantHeldItem(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    // this is only item spell effect applied to main-hand weapon of target player (players in area)
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* item_owner = (Player*)unitTarget;
    Item* item = item_owner->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

    if (!item)
        return;

    // must be equipped
    if (!item->IsEquipped())
        return;

    if (m_spellInfo->Effects[effIndex].MiscValue)
    {
        uint32 enchant_id = m_spellInfo->Effects[effIndex].MiscValue;
        int32 duration = m_spellInfo->GetDuration();          //Try duration index first ..
        if (!duration)
            duration = damage;//+1;            //Base points after ..
        if (!duration)
            duration = 10;                                  //10 seconds for enchants which don't have listed duration

        SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!pEnchant)
            return;

        // Always go to temp enchantment slot
        EnchantmentSlot slot = TEMP_ENCHANTMENT_SLOT;

        // Enchantment will not be applied if a different one already exists
        if (item->GetEnchantmentId(slot) && item->GetEnchantmentId(slot) != enchant_id)
            return;

        // Apply the temporary enchantment
        item->SetEnchantment(slot, enchant_id, duration*IN_MILLISECONDS, 0);
        item_owner->ApplyEnchantment(item, slot, true);
    }
}

void Spell::EffectDisEnchant(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!itemTarget || !itemTarget->GetTemplate()->DisenchantID)
        return;

    if (Player* caster = m_caster->ToPlayer())
    {
        caster->UpdateCraftSkill(m_spellInfo->Id);
        caster->SendLoot(itemTarget->GetGUID(), LOOT_DISENCHANTING);
        caster->SendDisenchantCredit(itemTarget); // Here he gets the SMSG.
    }

    // item will be removed at disenchanting end
}

void Spell::EffectInebriate(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();
    uint8 currentDrunk = player->GetDrunkValue();
    uint8 drunkMod = damage;
    if (currentDrunk + drunkMod > 100)
    {
        currentDrunk = 100;
        if (rand_chance() < 25.0f)
            player->CastSpell(player, 67468, false);    // Drunken Vomit
    }
    else
        currentDrunk += drunkMod;

    player->SetDrunkValue(currentDrunk, m_CastItem ? m_CastItem->GetEntry() : 0);
}

void Spell::EffectFeedPet(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    Player* player = m_caster->ToPlayer();
    if (!player)
        return;

    Item* foodItem = itemTarget;
    if (!foodItem)
        return;

    Pet* pet = player->GetPet();
    if (!pet)
        return;

    if (!pet->isAlive())
        return;

    ExecuteLogEffectDestroyItem(effIndex, foodItem->GetEntry());

    uint32 count = 1;
    player->DestroyItemCount(foodItem, count, true);
    // TODO: fix crash when a spell has two effects, both pointed at the same item target

    m_caster->CastSpell(pet, m_spellInfo->Effects[effIndex].TriggerSpell, true);
}

void Spell::EffectDismissPet(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isPet())
        return;

    Pet* pet = unitTarget->ToPet();

    ExecuteLogEffectUnsummonObject(effIndex, pet);
    pet->GetOwner()->RemovePet(pet, PET_SLOT_ACTUAL_PET_SLOT, false, pet->m_Stampeded);
}

void Spell::EffectSummonObject(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    uint32 go_id = m_spellInfo->Effects[effIndex].MiscValue;

    float o = m_caster->GetOrientation();

    int32 duration = 0;

    // Archaeology
    if (m_spellInfo->Id == 80451)
    {
        if (Player* player = m_caster->ToPlayer())
            go_id = player->GetArchaeologyMgr().GetSurveyBotEntry(o);

        duration = 15000;
    }

    if (go_id == 0)
        return;

    uint8 slot = 0;

    switch (m_spellInfo->Effects[effIndex].Effect)
    {
        case SPELL_EFFECT_SUMMON_OBJECT_SLOT1:
            slot = m_spellInfo->Effects[effIndex].MiscValueB;
            break;
        case SPELL_EFFECT_SUMMON_OBJECT_SLOT2:
            slot = 1;
            break;
        case SPELL_EFFECT_SUMMON_OBJECT_SLOT3:
            slot = 2;
            break;
        case SPELL_EFFECT_SUMMON_OBJECT_SLOT4:
            slot = 3;
            break;
        default:
            return;
    }

    uint64 guid = m_caster->m_ObjectSlot[slot];
    if (guid != 0)
    {
        GameObject* obj = NULL;
        if (m_caster)
            obj = m_caster->GetMap()->GetGameObject(guid);

        if (obj)
        {
            // Recast case - null spell id to make auras not be removed on object remove from world
            if (m_spellInfo->Id == obj->GetSpellId())
                obj->SetSpellId(0);
            m_caster->RemoveGameObject(obj, true);
        }
        m_caster->m_ObjectSlot[slot] = 0;
    }

    GameObject* pGameObj = new GameObject;

    float x, y, z;
    // If dest location if present
    if (m_targets.HasDst())
        destTarget->GetPosition(x, y, z);
    // Summon in random point all other units if location present
    else
        m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);

    Map* map = m_caster->GetMap();
    if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), go_id, map,
        m_caster->GetPhaseMask(), x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, 0, GO_STATE_READY))
    {
        delete pGameObj;
        return;
    }

    //pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
    if (!duration)
        duration = m_spellInfo->GetDuration();
    pGameObj->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);
    m_caster->AddGameObject(pGameObj);

    ExecuteLogEffectSummonObject(effIndex, pGameObj);

    map->AddToMap(pGameObj);

    m_caster->m_ObjectSlot[slot] = pGameObj->GetGUID();
}

void Spell::EffectSummonRaidMarker(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    Player* player = m_caster->ToPlayer();
    if (!player)
        return;

    Group* group = player->GetGroup();
    if (!group)
        return;

    if (group->isRaidGroup() && (!group->IsAssistant(player->GetGUID()) && !group->IsLeader(player->GetGUID()) && !(group->GetGroupType() & GROUPTYPE_EVERYONE_IS_ASSISTANT)))
        return;

    Group::RaidMarkerList& markers = group->GetRaidMarkers();
    for (Group::RaidMarkerList::iterator itr = markers.begin(); itr != markers.end();)
    {
        if ((*itr).spellId == m_spellInfo->Id)
        {
            if (DynamicObject* obj = sObjectAccessor->FindDynamicObject((*itr).guid))
                obj->Remove();

            itr = markers.erase(itr);
        }
        else itr++;
    }

    // We could, but we do not use the ID or slot directly :).
    // uint32 go_id = m_spellInfo->Effects[effIndex].MiscValue;
    // uint32 slot = m_spellInfo->Effects[effIndex].BasePoints;
    // uint32 slotMask = 1 << m_spellInfo->Effects[effIndex].BasePoints;
    float radius = m_spellInfo->Effects[effIndex].CalcRadius(m_caster);

    DynamicObject* dynObj = new DynamicObject(false);
    if (!dynObj->CreateDynamicObject(sObjectMgr->GenerateLowGuid(HIGHGUID_DYNAMICOBJECT), m_caster, m_spellInfo, *destTarget, radius, DYNAMIC_OBJECT_RAID_MARKER))
    {
        delete dynObj;
        return;
    }

    dynObj->UpdateObjectVisibility(false);

    int32 duration = m_spellInfo->GetDuration();
    dynObj->SetDuration(duration);

    dynObj->SetFieldNotifyFlag(UF_FLAG_PARTY_MEMBER);

    // Calculate the position.
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (m_targets.HasDst())
        destTarget->GetPosition(x, y, z);

    group->AddRaidMarker(dynObj->GetGUID(), m_spellInfo->Id, m_caster->GetMapId(), x, y, z);
}

void Spell::EffectResurrect(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    if (unitTarget->isAlive())
        return;
    if (!unitTarget->IsInWorld())
        return;

    switch (m_spellInfo->Id)
    {
        // Defibrillate (Goblin Jumper Cables) have 33% chance on success
        case 8342:
            if (roll_chance_i(67))
            {
                m_caster->CastSpell(m_caster, 8338, true, m_CastItem);
                return;
            }
            break;
        // Defibrillate (Goblin Jumper Cables XL) have 50% chance on success
        case 22999:
            if (roll_chance_i(50))
            {
                m_caster->CastSpell(m_caster, 23055, true, m_CastItem);
                return;
            }
            break;
        // Defibrillate (Gnomish Army Knife) have 67% chance on success_list
        case 54732:
            if (roll_chance_i(33))
            {
                return;
            }
            break;
        default:
            break;
    }

    Player* target = unitTarget->ToPlayer();

    if (target->IsRessurectRequested())       // already have one active request
        return;

    uint32 health = target->CountPctFromMaxHealth(damage);
    uint32 mana   = CalculatePct(target->GetMaxPower(POWER_MANA), damage);

    // Rebirth, soulstone ...
    if (m_spellInfo->Id == 20484 || m_spellInfo->Id == 3026)
        health = target->CountPctFromMaxHealth(60);

    if (m_spellInfo->Id == 61999) // Raise Ally
        mana = target->CountPctFromMaxMana(60);

    // Rebirth (Symbiosis)
    if (m_spellInfo->Id == 113269)
    {
        health = target->CountPctFromMaxHealth(60);
        mana = target->CountPctFromMaxMana(20);
    }

    ExecuteLogEffectResurrect(effIndex, target);

    target->SetResurrectRequestData(m_caster, health, mana, 0);
    SendResurrectRequest(target);
}

void Spell::EffectAddExtraAttacks(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->isAlive() || !unitTarget->getVictim())
        return;

    if (unitTarget->m_extraAttacks)
        return;

    unitTarget->m_extraAttacks = damage;

    ExecuteLogEffectExtraAttacks(effIndex, unitTarget->getVictim(), damage);
}

void Spell::EffectParry(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetCanParry(true);
}

void Spell::EffectBlock(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetCanBlock(true);
}

void Spell::EffectLeap(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->isInFlight())
        return;

    if (!m_targets.HasDst())
        return;

    Position pos;
    destTarget->GetPosition(&pos);
    unitTarget->GetFirstCollisionPosition(pos, unitTarget->GetDistance(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ() + 2.0f), 0.0f);
    unitTarget->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), unitTarget == m_caster);
}

void Spell::EffectReputation(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();

    int32  rep_change = damage;

    uint32 faction_id = m_spellInfo->Effects[effIndex].MiscValue;

    FactionEntry const* factionEntry = sFactionStore.LookupEntry(faction_id);

    if (!factionEntry)
        return;

    if (RepRewardRate const* repData = sObjectMgr->GetRepRewardRate(faction_id))
    {
        rep_change = int32((float)rep_change * repData->spell_rate);
    }

    // Bonus from spells that increase reputation gain
    float bonus = rep_change * player->GetTotalAuraModifier(SPELL_AURA_MOD_REPUTATION_GAIN) / 100.0f; // 10%
    rep_change += (int32)bonus;

    player->GetReputationMgr().ModifyReputation(factionEntry, rep_change);
}

void Spell::EffectQuestComplete(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    Player* player = unitTarget->ToPlayer();

    uint32 questId = m_spellInfo->Effects[effIndex].MiscValue;
    if (questId)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return;

        uint16 logSlot = player->FindQuestSlot(questId);
        if (logSlot < MAX_QUEST_LOG_SIZE)
            player->AreaExploredOrEventHappens(questId);
        else if (player->CanTakeQuest(quest, false))    // never rewarded before
            player->CompleteQuest(questId);             // quest not in log - for internal use
    }
}

void Spell::EffectForceDeselect(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    WorldPacket data(SMSG_CLEAR_TARGET, 8);
    ObjectGuid casterGuid = m_caster->GetGUID();

    uint8 bitsOrder[8] = { 0, 5, 7, 2, 1, 6, 3, 4 };
    data.WriteBitInOrder(casterGuid, bitsOrder);

    uint8 bytesOrder[8] = { 2, 1, 5, 7, 4, 0, 6, 3 };
    data.WriteBytesSeq(casterGuid, bytesOrder);

    m_caster->SendMessageToSet(&data, true);
}

void Spell::EffectSelfResurrect(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (!m_caster || m_caster->isAlive())
        return;
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    if (!m_caster->IsInWorld())
        return;

    uint32 health = 0;
    uint32 mana = 0;

    // flat case
    if (damage < 0)
    {
        health = uint32(-damage);
        mana = m_spellInfo->Effects[effIndex].MiscValue;
    }
    // percent case
    else
    {
        if (m_spellInfo->Id == 3026) // Soulstone resurrect
        {
            health = m_caster->CountPctFromMaxHealth(60);
            if (m_caster->GetMaxPower(POWER_MANA) > 0)
                mana = CalculatePct(m_caster->GetMaxPower(POWER_MANA), damage);
        }
        else
        {
            health = m_caster->CountPctFromMaxHealth(damage);
            if (m_caster->GetMaxPower(POWER_MANA) > 0)
                mana = CalculatePct(m_caster->GetMaxPower(POWER_MANA), damage);
        }
    }

    Player* player = m_caster->ToPlayer();
    player->ResurrectPlayer(0.0f);

    player->SetHealth(health);
    player->SetPower(POWER_MANA, mana);
    player->SetPower(POWER_RAGE, 0);
    player->SetPower(POWER_ENERGY, player->GetMaxPower(POWER_ENERGY));
    player->SetPower(POWER_FOCUS, 0);

    player->SpawnCorpseBones();
}

void Spell::EffectSkinning(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (unitTarget->GetTypeId() != TYPEID_UNIT)
        return;
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Creature* creature = unitTarget->ToCreature();
    int32 targetLevel = creature->getLevel();

    uint32 skill = creature->GetCreatureTemplate()->GetRequiredLootSkill();

    m_caster->ToPlayer()->SendLoot(creature->GetGUID(), LOOT_SKINNING);
    creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

    int32 reqValue = 0;

    if (targetLevel < 10)
        reqValue = 0;
    else if (targetLevel < 20)
        reqValue = (targetLevel - 10) * 10;
    else if (targetLevel < 74)
        reqValue = targetLevel * 5;
    else if (targetLevel < 81)
        reqValue = (targetLevel * 5) + (targetLevel - 73) * 5;
    else if (targetLevel < 85)
        reqValue = (targetLevel * 5) + 35;
    else if (targetLevel == 85) // @TODO find a generic formula
        reqValue = 470;
    else if (targetLevel == 86)
        reqValue = 485;
    else if (targetLevel == 87)
        reqValue = 495;
    else if (targetLevel == 88)
        reqValue = 520;
    else if (targetLevel == 89)
        reqValue = 540;
    else
        reqValue = 560;

    int32 skillValue = m_caster->ToPlayer()->GetPureSkillValue(skill);

    // Double chances for elites
    m_caster->ToPlayer()->UpdateGatherSkill(skill, skillValue, reqValue, creature->isElite() ? 2 : 1);
}

void Spell::EffectCharge(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget)
        return;

    if (effectHandleMode == SPELL_EFFECT_HANDLE_LAUNCH_TARGET)
    {
        float angle = unitTarget->GetRelativeAngle(m_caster);
        Position pos;

        unitTarget->GetContactPoint(m_caster, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
        unitTarget->GetFirstCollisionPosition(pos, unitTarget->GetObjectSize(), angle);

        m_caster->GetMotionMaster()->MoveCharge(pos.m_positionX, pos.m_positionY, pos.m_positionZ + unitTarget->GetObjectSize());

        if (m_caster->GetTypeId() == TYPEID_PLAYER)
            m_caster->ToPlayer()->SetFallInformation(0, m_caster->GetPositionZ());
    }

    if (effectHandleMode == SPELL_EFFECT_HANDLE_HIT_TARGET)
    {
        // not all charge effects used in negative spells
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            m_caster->ToPlayer()->SetFallInformation(0, m_caster->GetPositionZ());

            if (!m_spellInfo->IsPositive())
                m_caster->Attack(unitTarget, true);
        }
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetFallInformation(0, unitTarget->GetPositionZ());
}

void Spell::EffectChargeDest(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH)
        return;

    if (m_targets.HasDst())
    {
        Position pos;
        destTarget->GetPosition(&pos);

        if (m_caster->GetEntry() != 36609)       // hack fix for Valkyr Shadowguard, ignore first collision
        {
            float angle = m_caster->GetRelativeAngle(pos.GetPositionX(), pos.GetPositionY());
            float dist = m_caster->GetDistance(pos);

            // Custom MoP Script
            // Hack Fix - Collision on charge for Clash
            if (m_spellInfo->Id == 126452)
                dist /= 2;

            m_caster->GetFirstCollisionPosition(pos, dist, angle);
        }

        // Racer Slam Hit Destination
        if (m_spellInfo->Id == 49302)
        {
            if (urand(0, 100) < 20)
            {
                m_caster->CastSpell(m_caster, 49336, false);
                m_caster->CastSpell((Unit*)NULL, 49444, false); // achievement counter
            }
        }

        m_caster->GetMotionMaster()->MoveCharge(pos.m_positionX, pos.m_positionY, pos.m_positionZ, 42.0f, m_spellValue->EffectBasePoints[0] > 0 ? m_spellInfo->Id : 1003);
    }
}

void Spell::EffectKnockBack(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    if (Creature* creatureTarget = unitTarget->ToCreature())
        if (creatureTarget->isWorldBoss() || creatureTarget->IsDungeonBoss())
            return;

    // Spells with SPELL_EFFECT_KNOCK_BACK can't knoback target if target has ROOT
    if (unitTarget->HasUnitState(UNIT_STATE_ROOT))
        return;

    switch (m_spellInfo->Id)
    {
        case 13812: // Explosive Trap
            if (!m_caster->HasAura(119403)) // Glyph of Explosive Trap
                return;
            break;
        case 51490: // Thunderstorm
            if (m_caster->HasAura(62132)) // Glyph of Thunderstorm
                return;
            break;
        default:
            break;
    }

    // Instantly interrupt non melee spells being casted
    if (unitTarget->IsNonMeleeSpellCasted(true))
        unitTarget->InterruptNonMeleeSpells(true);

    float ratio = 0.1f;
    float speedxy = float(m_spellInfo->Effects[effIndex].MiscValue) * ratio;
    float speedz = float(damage) * ratio;
    if (speedxy < 0.1f && speedz < 0.1f)
        return;

    float x, y;
    if (m_spellInfo->Effects[effIndex].Effect == SPELL_EFFECT_KNOCK_BACK_DEST)
    {
        if (m_targets.HasDst())
            destTarget->GetPosition(x, y);
        else
            return;
    }
    else //if (m_spellInfo->Effects[i].Effect == SPELL_EFFECT_KNOCK_BACK)
    {
        m_caster->GetPosition(x, y);
    }

    unitTarget->KnockbackFrom(x, y, speedxy, speedz);

    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
        unitTarget->ToPlayer()->SetKnockBackTime(getMSTime());
}

void Spell::EffectLeapBack(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH_TARGET)
        return;

    if (!unitTarget)
        return;

    float speedxy = float(m_spellInfo->Effects[effIndex].MiscValue)/10;
    float speedz = float(damage/10);
    bool back = true;

    // Fix Glyph of Disengage
    if (m_caster->HasAura(56844))
    {
        speedxy *= 1.5f;
        speedz = float(75 / 10);
    }

    // Wild Charge (Moonkin) and Disengage
    if (m_spellInfo->Id == 102383 || m_spellInfo->SpellIconID == 1891)
        back = false;

    switch (m_spellInfo->Id)
    {
        case 98928: // Lava Wave dmg, Ragnaros, Firelands
        case 100292:
        case 100293:
        case 100294:
        case 103684: // Wave of Virtue, Archbishop Benedictus, Hour of Twilight
        case 103781: // Wave of Twilight, Archbishop Benedictus, Hour of Twilight
            back = false;
            break;
        default:
            break;
    }    

    m_caster->JumpTo(speedxy, speedz, back);
}

void Spell::EffectQuestClear(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    Player* player = unitTarget->ToPlayer();

    uint32 quest_id = m_spellInfo->Effects[effIndex].MiscValue;

    Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);

    if (!quest)
        return;

    // Player has never done this quest
    if (player->GetQuestStatus(quest_id) == QUEST_STATUS_NONE)
        return;

    // remove all quest entries for 'entry' from quest log
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 logQuest = player->GetQuestSlotQuestId(slot);
        if (logQuest == quest_id)
        {
            player->SetQuestSlot(slot, 0);

            // we ignore unequippable quest items in this case, it's still be equipped
            player->TakeQuestSourceItem(logQuest, false);
        }
    }

    player->RemoveActiveQuest(quest_id);
    player->RemoveRewardedQuest(quest_id);
}

void Spell::EffectSendTaxi(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->ActivateTaxiPathTo(m_spellInfo->Effects[effIndex].MiscValue, m_spellInfo->Id);
}

void Spell::EffectPullTowards(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    Position pos;
    if (m_spellInfo->Effects[effIndex].Effect == SPELL_EFFECT_PULL_TOWARDS_DEST)
    {
        if (m_targets.HasDst())
            pos.Relocate(*destTarget);
        else
            return;
    }
    else //if (m_spellInfo->Effects[i].Effect == SPELL_EFFECT_PULL_TOWARDS)
        pos.Relocate(m_caster);

    float speedXY = float(m_spellInfo->Effects[effIndex].MiscValue) * 0.1f;
    float speedZ = unitTarget->GetDistance(pos) / speedXY * 0.5f * 19.2911; // Movement::gravity

    unitTarget->GetMotionMaster()->MoveJump(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), speedXY, speedZ);
}

void Spell::EffectDispelMechanic(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    uint32 mechanic = m_spellInfo->Effects[effIndex].MiscValue;

    std::queue < std::pair < uint32, uint64 > > dispel_list;

    Unit::AuraMap const& auras = unitTarget->GetOwnedAuras();
    for (Unit::AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        AuraPtr aura = itr->second;
        if (!aura->GetApplicationOfTarget(unitTarget->GetGUID()))
            continue;
        if (roll_chance_i(aura->CalcDispelChance(unitTarget, !unitTarget->IsFriendlyTo(m_caster))))
            if ((aura->GetSpellInfo()->GetAllEffectsMechanicMask() & (1 << mechanic)))
                dispel_list.push(std::make_pair(aura->GetId(), aura->GetCasterGUID()));
    }

    for (; dispel_list.size(); dispel_list.pop())
    {
        unitTarget->RemoveAura(dispel_list.front().first, dispel_list.front().second, 0, AURA_REMOVE_BY_ENEMY_SPELL);
    }
}

void Spell::EffectSummonDeadPet(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    Player* player = m_caster->ToPlayer();
    if (!player)
        return;

    Pet* pet = player->GetPet();
    if (!pet)
        return;

    if (pet->isAlive())
        return;

    if (damage < 0)
        return;

    float x, y, z;
    player->GetPosition(x, y, z);

    player->GetMap()->CreatureRelocation(pet, x, y, z, player->GetOrientation());

    pet->SetUInt32Value(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
    pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
    pet->setDeathState(ALIVE);
    pet->ClearUnitState(uint32(UNIT_STATE_ALL_STATE));
    pet->SetHealth(pet->CountPctFromMaxHealth(damage));

    //pet->AIM_Initialize();
    //player->PetSpellInitialize();
    pet->SavePetToDB(PET_SLOT_ACTUAL_PET_SLOT);
}

void Spell::EffectDestroyAllTotems(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    int32 mana = 0;
    float manaCostPercentage = 0.00f;
    for (uint8 slot = SUMMON_SLOT_TOTEM; slot < MAX_TOTEM_SLOT; ++slot)
    {
        if (!m_caster->m_SummonSlot[slot])
            continue;

        Creature* totem = m_caster->GetMap()->GetCreature(m_caster->m_SummonSlot[slot]);
        if (totem && totem->isTotem())
        {
            uint32 spell_id = totem->GetUInt32Value(UNIT_CREATED_BY_SPELL);
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell_id);
            if (spellInfo)
            {
                manaCostPercentage = spellInfo->ManaCostPercentage;
                mana += m_caster->CountPctFromMaxMana(int32(manaCostPercentage));
            }
            totem->ToTotem()->UnSummon();
        }
    }
    ApplyPct(mana, damage);
    if (mana)
        m_caster->CastCustomSpell(m_caster, 39104, &mana, NULL, NULL, true);
}

void Spell::EffectDurabilityDamage(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    int32 slot = m_spellInfo->Effects[effIndex].MiscValue;

    // FIXME: some spells effects have value -1/-2
    // Possibly its mean -1 all player equipped items and -2 all items
    if (slot < 0)
    {
        unitTarget->ToPlayer()->DurabilityPointsLossAll(damage, (slot < -1));
        return;
    }

    // invalid slot value
    if (slot >= INVENTORY_SLOT_BAG_END)
        return;

    if (Item* item = unitTarget->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        unitTarget->ToPlayer()->DurabilityPointsLoss(item, damage);

    ExecuteLogEffectDurabilityDamage(effIndex, unitTarget, slot, damage);
}

void Spell::EffectDurabilityDamagePCT(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    int32 slot = m_spellInfo->Effects[effIndex].MiscValue;

    // FIXME: some spells effects have value -1/-2
    // Possibly its mean -1 all player equipped items and -2 all items
    if (slot < 0)
    {
        unitTarget->ToPlayer()->DurabilityLossAll(float(damage) / 100.0f, (slot < -1));
        return;
    }

    // invalid slot value
    if (slot >= INVENTORY_SLOT_BAG_END)
        return;

    if (damage <= 0)
        return;

    if (Item* item = unitTarget->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        unitTarget->ToPlayer()->DurabilityLoss(item, float(damage) / 100.0f);
}

void Spell::EffectModifyThreatPercent(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    unitTarget->getThreatManager().modifyThreatPercent(m_caster, damage);
}

void Spell::EffectTransmitted(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    uint32 name_id = m_spellInfo->Effects[effIndex].MiscValue;

    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(name_id);

    if (!goinfo)
    {
        sLog->outError(LOG_FILTER_SQL, "Gameobject (Entry: %u) not exist and not created at spell (ID: %u) cast", name_id, m_spellInfo->Id);
        return;
    }

    float fx, fy, fz;

    if (m_targets.HasDst())
        destTarget->GetPosition(fx, fy, fz);
    //FIXME: this can be better check for most objects but still hack
    else if (m_spellInfo->Effects[effIndex].HasRadius() && m_spellInfo->Speed == 0)
    {
        float dis = m_spellInfo->Effects[effIndex].CalcRadius(m_originalCaster);
        m_caster->GetClosePoint(fx, fy, fz, DEFAULT_WORLD_OBJECT_SIZE, dis);
    }
    else
    {
        //GO is always friendly to it's creator, get range for friends
        float min_dis = m_spellInfo->GetMinRange(true);
        float max_dis = m_spellInfo->GetMaxRange(true);
        float dis = (float)rand_norm() * (max_dis - min_dis) + min_dis;

        m_caster->GetClosePoint(fx, fy, fz, DEFAULT_WORLD_OBJECT_SIZE, dis);
    }

    Map* cMap = m_caster->GetMap();
    if (goinfo->type == GAMEOBJECT_TYPE_FISHINGNODE || goinfo->type == GAMEOBJECT_TYPE_FISHINGHOLE)
    {
        LiquidData liqData;
        if (!cMap->IsInWater(fx, fy, fz + 1.f/* -0.5f */, &liqData))             // Hack to prevent fishing bobber from failing to land on fishing hole
        { // but this is not proper, we really need to ignore not materialized objects
            SendCastResult(SPELL_FAILED_NOT_HERE);
            SendChannelUpdate(0);
            return;
        }

        // replace by water level in this case
        //fz = cMap->GetWaterLevel(fx, fy);
        fz = liqData.level;
    }
    // if gameobject is summoning object, it should be spawned right on caster's position
    else if (goinfo->type == GAMEOBJECT_TYPE_SUMMONING_RITUAL)
    {
        m_caster->GetPosition(fx, fy, fz);
    }

    GameObject* pGameObj = new GameObject;

    if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), name_id, cMap,
        m_caster->GetPhaseMask(), fx, fy, fz, m_caster->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
    {
        delete pGameObj;
        return;
    }

    int32 duration = m_spellInfo->GetDuration();

    switch (goinfo->type)
    {
        case GAMEOBJECT_TYPE_FISHINGNODE:
        {
            m_caster->SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT, pGameObj->GetGUID());
            m_caster->AddGameObject(pGameObj);              // will removed at spell cancel

            // end time of range when possible catch fish (FISHING_BOBBER_READY_TIME..GetDuration(m_spellInfo))
            // start time == fish-FISHING_BOBBER_READY_TIME (0..GetDuration(m_spellInfo)-FISHING_BOBBER_READY_TIME)
            int32 lastSec = 0;
            switch (urand(0, 3))
            {
                case 0: lastSec =  3; break;
                case 1: lastSec =  7; break;
                case 2: lastSec = 13; break;
                case 3: lastSec = 17; break;
            }

            duration = duration - lastSec*IN_MILLISECONDS + FISHING_BOBBER_READY_TIME*IN_MILLISECONDS;
            break;
        }
        case GAMEOBJECT_TYPE_SUMMONING_RITUAL:
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                pGameObj->AddUniqueUse(m_caster->ToPlayer());
                m_caster->AddGameObject(pGameObj);      // will be removed at spell cancel
            }
            break;
        }
        case GAMEOBJECT_TYPE_DUEL_ARBITER: // 52991
            m_caster->AddGameObject(pGameObj);
            break;
        case GAMEOBJECT_TYPE_FISHINGHOLE:
        case GAMEOBJECT_TYPE_CHEST:
        default:
            break;
    }

    pGameObj->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);

    pGameObj->SetOwnerGUID(m_caster->GetGUID());

    //pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
    pGameObj->SetSpellId(m_spellInfo->Id);

    ExecuteLogEffectSummonObject(effIndex, pGameObj);
    //m_caster->AddGameObject(pGameObj);
    //m_ObjToDel.push_back(pGameObj);

    cMap->AddToMap(pGameObj);

    if (uint32 linkedEntry = pGameObj->GetGOInfo()->GetLinkedGameObjectEntry())
    {
        GameObject* linkedGO = new GameObject;
        if (linkedGO->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), linkedEntry, cMap,
            m_caster->GetPhaseMask(), fx, fy, fz, m_caster->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
        {
            linkedGO->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
            //linkedGO->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
            linkedGO->SetSpellId(m_spellInfo->Id);
            linkedGO->SetOwnerGUID(m_caster->GetGUID());

            ExecuteLogEffectSummonObject(effIndex, linkedGO);

            linkedGO->GetMap()->AddToMap(linkedGO);
        }
        else
        {
            delete linkedGO;
            linkedGO = NULL;
            return;
        }
    }
}

void Spell::EffectProspecting(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)m_caster;
    if (!itemTarget || !(itemTarget->GetTemplate()->Flags & ITEM_PROTO_FLAG_PROSPECTABLE))
        return;

    if (itemTarget->GetCount() < 5)
        return;

    if (sWorld->getBoolConfig(CONFIG_SKILL_PROSPECTING))
    {
        uint32 SkillValue = p_caster->GetPureSkillValue(SKILL_JEWELCRAFTING);
        uint32 reqSkillValue = itemTarget->GetTemplate()->RequiredSkillRank;
        p_caster->UpdateGatherSkill(SKILL_JEWELCRAFTING, SkillValue, reqSkillValue);
    }

    m_caster->ToPlayer()->SendLoot(itemTarget->GetGUID(), LOOT_PROSPECTING);
}

void Spell::EffectMilling(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)m_caster;
    if (!itemTarget || !(itemTarget->GetTemplate()->Flags & ITEM_PROTO_FLAG_MILLABLE))
        return;

    if (itemTarget->GetCount() < 5)
        return;

    if (sWorld->getBoolConfig(CONFIG_SKILL_MILLING))
    {
        uint32 SkillValue = p_caster->GetPureSkillValue(SKILL_INSCRIPTION);
        uint32 reqSkillValue = itemTarget->GetTemplate()->RequiredSkillRank;
        p_caster->UpdateGatherSkill(SKILL_INSCRIPTION, SkillValue, reqSkillValue);
    }

    m_caster->ToPlayer()->SendLoot(itemTarget->GetGUID(), LOOT_MILLING);
}

void Spell::EffectSkill(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;
}

/* There is currently no need for this effect. We handle it in Battleground.cpp
   If we would handle the resurrection here, the spiritguide would instantly disappear as the
   player revives, and so we wouldn't see the spirit heal visual effect on the npc.
   This is why we use a half sec delay between the visual effect and the resurrection itself */
void Spell::EffectSpiritHeal(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    /*
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    if (!unitTarget->IsInWorld())
        return;

    //m_spellInfo->Effects[i].BasePoints; == 99 (percent?)
    //unitTarget->ToPlayer()->setResurrect(m_caster->GetGUID(), unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), unitTarget->GetMaxHealth(), unitTarget->GetMaxPower(POWER_MANA));
    unitTarget->ToPlayer()->ResurrectPlayer(1.0f);
    unitTarget->ToPlayer()->SpawnCorpseBones();
    */
}

// remove insignia spell effect
void Spell::EffectSkinPlayerCorpse(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if ((m_caster->GetTypeId() != TYPEID_PLAYER) || (unitTarget->GetTypeId() != TYPEID_PLAYER) || (unitTarget->isAlive()))
        return;

    unitTarget->ToPlayer()->RemovedInsignia((Player*)m_caster);
}

void Spell::EffectStealBeneficialBuff(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget == m_caster)                 // can't steal from self
        return;

    DispelChargesList steal_list;

    // Create dispel mask by dispel type
    uint32 dispelMask  = SpellInfo::GetDispelMask(DispelType(m_spellInfo->Effects[effIndex].MiscValue));
    Unit::AuraMap const& auras = unitTarget->GetOwnedAuras();
    for (Unit::AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        AuraPtr aura = itr->second;
        AuraApplication * aurApp = aura->GetApplicationOfTarget(unitTarget->GetGUID());
        if (!aurApp)
            continue;

        if ((aura->GetSpellInfo()->GetDispelMask()) & dispelMask)
        {
            // Need check for passive? this
            if (!aurApp->IsPositive() || aura->IsPassive() || aura->GetSpellInfo()->AttributesEx4 & SPELL_ATTR4_NOT_STEALABLE)
                continue;

            // The charges / stack amounts don't count towards the total number of auras that can be dispelled.
            // Ie: A dispel on a target with 5 stacks of Winters Chill and a Polymorph has 1 / (1 + 1) -> 50% chance to dispell
            // Polymorph instead of 1 / (5 + 1) -> 16%.
            bool dispel_charges = aura->GetSpellInfo()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES;
            uint8 charges = dispel_charges ? aura->GetCharges() : aura->GetStackAmount();
            if (charges > 0)
                steal_list.push_back(std::make_pair(aura, charges));
        }
    }

    if (steal_list.empty())
        return;

    // Ok if exist some buffs for dispel try dispel it
    uint32 failCount = 0;
    DispelList success_list;
    WorldPacket dataFail(SMSG_DISPEL_FAILED, 8+8+4+4+damage*4);
    // dispel N = damage buffs (or while exist buffs for dispel)
    for (int32 count = 0; count < damage && !steal_list.empty();)
    {
        // Random select buff for dispel
        DispelChargesList::iterator itr = steal_list.begin();
        std::advance(itr, urand(0, steal_list.size() - 1));

        int32 chance = itr->first->CalcDispelChance(unitTarget, !unitTarget->IsFriendlyTo(m_caster));
        // 2.4.3 Patch Notes: "Dispel effects will no longer attempt to remove effects that have 100% dispel resistance."
        if (!chance)
        {
            steal_list.erase(itr);
            continue;
        }
        else
        {
            if (roll_chance_i(chance))
            {
                success_list.push_back(std::make_pair(itr->first->GetId(), itr->first->GetCasterGUID()));
                --itr->second;
                if (itr->second <= 0)
                    steal_list.erase(itr);
            }
            else
            {
                if (!failCount)
                {
                    // Failed to dispell
                    dataFail << uint64(m_caster->GetGUID());            // Caster GUID
                    dataFail << uint64(unitTarget->GetGUID());          // Victim GUID
                    dataFail << uint32(m_spellInfo->Id);                // dispel spell id
                }
                ++failCount;
                dataFail << uint32(itr->first->GetId());                         // Spell Id
            }
            ++count;
        }
    }

    if (failCount)
        m_caster->SendMessageToSet(&dataFail, true);

    if (success_list.empty())
        return;

    WorldPacket dataSuccess(SMSG_SPELLSTEALLOG, 8+8+4+1+4+damage*5);
    dataSuccess.append(unitTarget->GetPackGUID());  // Victim GUID
    dataSuccess.append(m_caster->GetPackGUID());    // Caster GUID
    dataSuccess << uint32(m_spellInfo->Id);         // dispel spell id
    dataSuccess << uint8(0);                        // not used
    dataSuccess << uint32(success_list.size());     // count
    for (DispelList::iterator itr = success_list.begin(); itr!=success_list.end(); ++itr)
    {
        dataSuccess << uint32(itr->first);          // Spell Id
        dataSuccess << uint8(0);                    // 0 - steals !=0 transfers
        unitTarget->RemoveAurasDueToSpellBySteal(itr->first, itr->second, m_caster);
    }
    m_caster->SendMessageToSet(&dataSuccess, true);

    // Glyph of SpellSteal
    if (m_caster->HasAura(115713))
        m_caster->HealBySpell(m_caster, m_spellInfo, m_caster->CountPctFromMaxHealth(5));

    // Observer: Clone magic
    if (m_spellInfo->Id == 115284)
    {
        m_caster->CastSpell(m_caster, 19658, true);
        // Glyph of Felhunter
        if (Unit* owner = m_caster->GetOwner())
            if (owner->HasAura(56249))
                owner->CastSpell(owner, 19658, true);
    }
}

void Spell::EffectKillCreditPersonal(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->KilledMonsterCredit(m_spellInfo->Effects[effIndex].MiscValue, 0);
}

void Spell::EffectKillCredit(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    int32 creatureEntry = m_spellInfo->Effects[effIndex].MiscValue;
    if (!creatureEntry)
    {
        if (m_spellInfo->Id == 42793) // Burn Body
            creatureEntry = 24008; // Fallen Combatant
    }

    if (creatureEntry)
        unitTarget->ToPlayer()->RewardPlayerAndGroupAtEvent(creatureEntry, unitTarget);
}

void Spell::EffectQuestFail(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->FailQuest(m_spellInfo->Effects[effIndex].MiscValue);
}

void Spell::EffectQuestStart(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();
    if (Quest const* qInfo = sObjectMgr->GetQuestTemplate(m_spellInfo->Effects[effIndex].MiscValue))
        if (player->CanTakeQuest(qInfo, false) && player->CanAddQuest(qInfo, false))
            player->AddQuestAndCheckCompletion(qInfo, NULL);
}

void Spell::EffectActivateRune(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_LAUNCH)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = m_caster->ToPlayer();

    if (player->getClass() != CLASS_DEATH_KNIGHT)
        return;

    // needed later
    m_runesState = m_caster->ToPlayer()->GetRunesState();

    uint32 count = std::max<uint32>(damage, 1);
    for (uint32 j = 0; j < MAX_RUNES && count > 0; ++j)
    {
        if (player->GetRuneCooldown(j) && player->GetCurrentRune(j) == RuneType(m_spellInfo->Effects[effIndex].MiscValue))
        {
            if (m_spellInfo->Id == 45529)
                if (player->GetBaseRune(j) != RuneType(m_spellInfo->Effects[effIndex].MiscValueB))
                    continue;

            player->SetRuneCooldown(j, 0);
            player->ResyncRunes(MAX_RUNES);
            --count;
        }
    }

    // Empower rune weapon
    if (m_spellInfo->Id == 47568)
    {
        // Need to do this just once
        if (effIndex != 0)
            return;

        for (uint32 i = 0; i < MAX_RUNES; ++i)
        {
            if (player->GetRuneCooldown(i))
                player->SetRuneCooldown(i, 0);
        }
    }
}

void Spell::EffectCreateTamedPet(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || unitTarget->GetPetGUID() || unitTarget->getClass() != CLASS_HUNTER)
        return;

    uint32 creatureEntry = m_spellInfo->Effects[effIndex].MiscValue;
    Pet* pet = unitTarget->CreateTamedPetFrom(creatureEntry, m_spellInfo->Id);
    if (!pet)
        return;

    // add to world
    pet->GetMap()->AddToMap(pet->ToCreature());

    // unitTarget has pet now
    unitTarget->SetMinion(pet, true, PET_SLOT_ACTUAL_PET_SLOT);

    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        m_caster->ToPlayer()->m_currentPetSlot = m_caster->ToPlayer()->getSlotForNewPet();
        pet->SavePetToDB(m_caster->ToPlayer()->m_currentPetSlot);
        unitTarget->ToPlayer()->PetSpellInitialize();
    }
}

void Spell::EffectDiscoverTaxi(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    uint32 nodeid = m_spellInfo->Effects[effIndex].MiscValue;
    if (sTaxiNodesStore.LookupEntry(nodeid))
        unitTarget->ToPlayer()->GetSession()->SendDiscoverNewTaxiNode(nodeid);
}

void Spell::EffectTitanGrip(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetCanTitanGrip(true);
}

void Spell::EffectRedirectThreat(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (unitTarget)
        m_caster->SetReducedThreatPercent((uint32)damage, unitTarget->GetGUID());
}

void Spell::EffectGameObjectDamage(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!gameObjTarget)
        return;

    Unit* caster = m_originalCaster;
    if (!caster)
        return;

    FactionTemplateEntry const* casterFaction = caster->getFactionTemplateEntry();
    FactionTemplateEntry const* targetFaction = sFactionTemplateStore.LookupEntry(gameObjTarget->GetUInt32Value(GAMEOBJECT_FACTION));
    // Do not allow to damage GO's of friendly factions (ie: Wintergrasp Walls/Ulduar Storm Beacons)
    if ((casterFaction && targetFaction && !casterFaction->IsFriendlyTo(*targetFaction)) || !targetFaction)
        gameObjTarget->ModifyHealth(-damage, caster, GetSpellInfo()->Id);
}

void Spell::EffectGameObjectRepair(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!gameObjTarget)
        return;

    gameObjTarget->ModifyHealth(damage, m_caster);
}

void Spell::EffectGameObjectSetDestructionState(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!gameObjTarget || !m_originalCaster)
        return;

    Player* player = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself();
    gameObjTarget->SetDestructibleState(GameObjectDestructibleState(m_spellInfo->Effects[effIndex].MiscValue), player, true);
}

void Spell::SummonGuardian(uint32 i, uint32 entry, SummonPropertiesEntry const* properties, uint32 numGuardians)
{
    Unit* caster = m_originalCaster;
    if (!caster)
        return;

    if (caster->isTotem())
        caster = caster->ToTotem()->GetOwner();

    // in another case summon new
    uint8 level = caster->getLevel();

    // level of pet summoned using engineering item based at engineering skill level
    if (m_CastItem && caster->GetTypeId() == TYPEID_PLAYER)
        if (ItemTemplate const* proto = m_CastItem->GetTemplate())
            if (proto->RequiredSkill == SKILL_ENGINEERING)
                if (uint16 skill202 = caster->ToPlayer()->GetSkillValue(SKILL_ENGINEERING))
                    level = skill202 / 5;

    float radius = 5.0f;
    int32 duration = m_spellInfo->GetDuration();

    switch (m_spellInfo->Id)
    {
        case 81283: // Fungal Growth
            numGuardians = 1;
            break;
        case 49028: // Dancing Rune Weapon
            // 20% Parry
            m_originalCaster->CastSpell(m_originalCaster, 81256, true);
            break;
        default:
            break;
    }

    if (Player* modOwner = m_originalCaster->GetSpellModOwner())
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_DURATION, duration);

    //TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;
    Map* map = caster->GetMap();

    for (uint32 count = 0; count < numGuardians; ++count)
    {
        Position pos;
        if (count == 0)
            pos = *destTarget;
        else
            // randomize position for multiple summons
            m_caster->GetRandomPoint(*destTarget, radius, pos);

        TempSummon* summon = map->SummonCreature(entry, pos, properties, duration, caster, m_spellInfo->Id);
        if (!summon)
            return;

        // summon gargoyle shouldn't be initialized twice
        if (summon->GetEntry() == 27829)
        {
            summon->setFaction(caster->getFaction());
            ExecuteLogEffectSummonObject(i, summon);
            return;
        }

        if (summon->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
            ((Guardian*)summon)->InitStatsForLevel(level);

        if (properties && properties->Category == SUMMON_CATEGORY_ALLY)
            summon->setFaction(caster->getFaction());

        if (summon->HasUnitTypeMask(UNIT_MASK_MINION) && m_targets.HasDst())
            ((Minion*)summon)->SetFollowAngle(m_caster->GetAngle(summon));

        if (summon->GetEntry() == 27893)
        {
            if (uint32 weapon = m_caster->GetUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID))
            {
                summon->SetDisplayId(11686);
                summon->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, weapon);
            }
            else
                summon->SetDisplayId(1126);
        }

        summon->AI()->EnterEvadeMode();

        ExecuteLogEffectSummonObject(i, summon);
    }
}

void Spell::EffectRenamePet(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT ||
        !unitTarget->ToCreature()->isPet() || ((Pet*)unitTarget)->getPetType() != HUNTER_PET)
        return;

    unitTarget->SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);
}

void Spell::EffectPlayMusic(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 soundId = m_spellInfo->Effects[effIndex].MiscValue;

    if (!sSoundEntriesStore.LookupEntry(soundId))
        return;

    WorldPacket data(SMSG_PLAY_MUSIC, 4);
    data << uint32(soundId);
    unitTarget->ToPlayer()->GetSession()->SendPacket(&data);
}

void Spell::EffectSpecCount(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->UpdateSpecCount(damage);
}

void Spell::EffectActivateSpec(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->ActivateSpec(damage - 1);  // damage is 1 or 2, spec is 0 or 1
}

void Spell::EffectPlaySound(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    switch (m_spellInfo->Id)
    {
        case 58730: // Restricted Flight Area
        case 58600: // Restricted Flight Area
        case 91604: // Restricted Flight Area
            unitTarget->ToPlayer()->GetSession()->SendNotification(LANG_ZONE_NOFLYZONE);
            break;
        default:
            break;
    }

    uint32 soundId = m_spellInfo->Effects[effIndex].MiscValue;

    if (!sSoundEntriesStore.LookupEntry(soundId))
        return;

    unitTarget->SendPlaySound(soundId, true);
}

void Spell::EffectRemoveAura(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;
    // there may be need of specifying casterguid of removed auras
    unitTarget->RemoveAurasDueToSpell(m_spellInfo->Effects[effIndex].TriggerSpell);
}

void Spell::EffectDamageFromMaxHealthPCT(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget)
        return;

    m_damage += unitTarget->CountPctFromMaxHealth(damage);
}

void Spell::EffectGiveCurrency(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->ModifyCurrency(m_spellInfo->Effects[effIndex].MiscValue, damage);
}

void Spell::EffectDestroyItem(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();
    uint32 itemId = m_spellInfo->Effects[effIndex].ItemType;

    if (Item* item = player->GetItemByEntry(itemId))
        player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
}

void Spell::EffectCastButtons(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = m_caster->ToPlayer();
    uint32 button_id = m_spellInfo->Effects[effIndex].MiscValue + 132;
    uint32 n_buttons = m_spellInfo->Effects[effIndex].MiscValueB;

    for (; n_buttons; --n_buttons, ++button_id)
    {
        ActionButton const* ab = p_caster->GetActionButton(button_id);
        if (!ab || ab->GetType() != ACTION_BUTTON_SPELL)
            continue;

        //! Action button data is unverified when it's set so it can be "hacked"
        //! to contain invalid spells, so filter here.
        uint32 spell_id = ab->GetAction();
        if (!spell_id)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell_id);
        if (!spellInfo)
            continue;

        if (!p_caster->HasSpell(spell_id) || p_caster->HasSpellCooldown(spell_id))
            continue;

        if (!(spellInfo->AttributesEx7 & SPELL_ATTR7_SUMMON_TOTEM))
            continue;

        int32 cost = spellInfo->CalcPowerCost(m_caster, spellInfo->GetSchoolMask(), m_spellPowerData);
        if (m_caster->GetPower(POWER_MANA) < cost)
            continue;

        TriggerCastFlags triggerFlags = TriggerCastFlags(TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_CAST_DIRECTLY);
        m_caster->CastSpell(m_caster, spell_id, triggerFlags);
    }
}

void Spell::EffectRechargeManaGem(SpellEffIndex /*effIndex*/)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = m_caster->ToPlayer();

    if (!player)
        return;

    uint32 item_id = m_spellInfo->Effects[EFFECT_0].ItemType;

    ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(item_id);
    if (!pProto)
    {
        player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    if (Item* pItem = player->GetItemByEntry(item_id))
    {
        for (int x = 0; x < MAX_ITEM_PROTO_SPELLS; ++x)
            pItem->SetSpellCharges(x, pProto->Spells[x].SpellCharges);
        pItem->SetState(ITEM_CHANGED, player);
    }
}

void Spell::EffectBind(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();

    uint32 area_id;
    WorldLocation loc;
    if (m_spellInfo->Effects[effIndex].TargetA.GetTarget() == TARGET_DEST_DB || m_spellInfo->Effects[effIndex].TargetB.GetTarget() == TARGET_DEST_DB)
    {
        SpellTargetPosition const* st = sSpellMgr->GetSpellTargetPosition(m_spellInfo->Id, effIndex);
        if (!st)
        {
            sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::EffectBind - unknown teleport coordinates for spell ID %u", m_spellInfo->Id);
            return;
        }

        loc.m_mapId       = st->target_mapId;
        loc.m_positionX   = st->target_X;
        loc.m_positionY   = st->target_Y;
        loc.m_positionZ   = st->target_Z;
        loc.SetOrientation(st->target_Orientation);
        area_id = player->GetAreaId();
    }
    else
    {
        player->GetPosition(&loc);
        loc.m_mapId = player->GetMapId();
        area_id = player->GetAreaId();
    }

    player->SetHomebind(loc, area_id);

    // binding
    WorldPacket data(SMSG_BIND_POINT_UPDATE, 4 + 4 + 4 + 4 + 4);
    data << uint32(area_id);
    data << float(loc.m_positionX);
    data << float(loc.m_positionZ);
    data << float(loc.m_positionY);
    data << uint32(loc.m_mapId);
    player->SendDirectMessage(&data);

    // zone update
    data.Initialize(SMSG_PLAYER_BOUND, 8 + 4);
    ObjectGuid playerGuid = player->GetGUID();

    uint8 bitsOrder[8] = { 5, 3, 1, 2, 0, 7, 6, 4 };
    data.WriteBitInOrder(playerGuid, bitsOrder);

    uint8 bytesOrder[8] = { 6, 7, 5, 4, 1, 3, 2, 0 };
    data.WriteBytesSeq(playerGuid, bytesOrder);

    data << uint32(area_id);

    player->SendDirectMessage(&data);
}

void Spell::EffectSummonRaFFriend(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER || !unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    m_caster->CastSpell(unitTarget, m_spellInfo->Effects[effIndex].TriggerSpell, true);
}

void Spell::EffectUnlearnTalent(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* plr = m_caster->ToPlayer();

    for (auto itr : *plr->GetTalentMap(plr->GetActiveSpec()))
    {
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(itr.first);
        if (!spell)
            continue;

        TalentEntry const* talent = sTalentStore.LookupEntry(spell->talentId);
        if (!talent)
            continue;

        if (spell->talentId != m_glyphIndex)
            continue;

        plr->removeSpell(itr.first, true);
        // search for spells that the talent teaches and unlearn them
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (spell->Effects[i].TriggerSpell > 0 && spell->Effects[i].Effect == SPELL_EFFECT_LEARN_SPELL)
                plr->removeSpell(spell->Effects[i].TriggerSpell, true);

        itr.second->state = PLAYERSPELL_REMOVED;

        plr->SetUsedTalentCount(plr->GetUsedTalentCount()-1);
        plr->SetFreeTalentPoints(plr->GetFreeTalentPoints()+1);
        break;
    }

    plr->SaveToDB();
    plr->SendTalentsInfoData(false);
}

void Spell::EffectCreateAreatrigger(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    Position pos;
    if (!m_targets.HasDst())
        GetCaster()->GetPosition(&pos);
    else
        destTarget->GetPosition(&pos);

    // trigger entry/miscvalue relation is currently unknown, for now use MiscValue as trigger entry
    uint32 triggerEntry = GetSpellInfo()->Effects[effIndex].MiscValue;

    AreaTrigger* areaTrigger = new AreaTrigger;
    if (!areaTrigger->CreateAreaTrigger(sObjectMgr->GenerateLowGuid(HIGHGUID_AREATRIGGER), triggerEntry, m_caster, GetSpellInfo(), pos))
        delete areaTrigger;

    // Custom MoP Script
    switch (m_spellInfo->Id)
    {
        case 121536:// Angelic Feather
        {
            int32 count = m_caster->CountAreaTrigger(m_spellInfo->Id);

            if (count > 3)
            {
                std::list<AreaTrigger*> angelicFeatherList;
                m_caster->GetAreaTriggerList(angelicFeatherList, m_spellInfo->Id);

                if (!angelicFeatherList.empty())
                {
                    angelicFeatherList.sort(SkyMistCore::AreaTriggerDurationPctOrderPred());

                    for (auto itr : angelicFeatherList)
                    {
                        AreaTrigger* angelicFeather = itr;
                        angelicFeather->SetDuration(0);
                        break;
                    }
                }
            }

            break;
        }
        case 115460:// Healing Sphere
        {
            int32 count = m_caster->CountAreaTrigger(m_spellInfo->Id);

            if (count > 3)
            {
                std::list<AreaTrigger*> healingSphereList;
                m_caster->GetAreaTriggerList(healingSphereList, m_spellInfo->Id);

                if (!healingSphereList.empty())
                {
                    healingSphereList.sort(SkyMistCore::AreaTriggerDurationPctOrderPred());

                    for (auto itr : healingSphereList)
                    {
                        AreaTrigger* healingSphere = itr;
                        healingSphere->SetDuration(0);
                        break;
                    }
                }
            }

            break;
        }
        case 116011:// Rune of Power
        {
            int32 count = m_caster->CountAreaTrigger(m_spellInfo->Id);

            if (count > 2)
            {
                std::list<AreaTrigger*> runeOfPowerList;
                m_caster->GetAreaTriggerList(runeOfPowerList, m_spellInfo->Id);

                if (!runeOfPowerList.empty())
                {
                    runeOfPowerList.sort(SkyMistCore::AreaTriggerDurationPctOrderPred());

                    for (auto itr : runeOfPowerList)
                    {
                        AreaTrigger* runeOfPower = itr;
                        runeOfPower->SetDuration(0);
                        break;
                    }
                }
            }

            break;
        }
        default:
            break;
    }
}

int32 Spell::CalculateMonkMeleeAttacks(Unit* caster, float coeff, int32 APmultiplier)
{
    float minDamage = 0;
    float maxDamage = 0;
    bool dualwield  = false;
    int32 AP = caster->GetTotalAttackPowerValue(BASE_ATTACK);

    if (Player* plr = caster->ToPlayer())
    {
        Item* mainItem = plr->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        Item* offItem = plr->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

        dualwield = (mainItem && offItem) ? 1 : 0;

        if (coeff < 0)
            coeff = 0.0f;

        // Main Hand
        if (mainItem && !caster->HasAuraType(SPELL_AURA_MOD_DISARM))
        {
            minDamage += mainItem->GetTemplate()->DamageMin;
            maxDamage += mainItem->GetTemplate()->DamageMax;

            minDamage /= float(m_caster->GetAttackTime(BASE_ATTACK) / 1000.0f);
            maxDamage /= float(m_caster->GetAttackTime(BASE_ATTACK) / 1000.0f);
        }

        // Off Hand
        if (offItem && !caster->HasAuraType(SPELL_AURA_MOD_DISARM))
        {
            float offMinDamage = (offItem->GetTemplate()->DamageMin) / float(m_caster->GetAttackTime(OFF_ATTACK) / 1000.0f);
            float offMaxDamage = (offItem->GetTemplate()->DamageMax)  / float(m_caster->GetAttackTime(OFF_ATTACK) / 1000.0f);

            minDamage += offMinDamage / 2.0f;
            maxDamage += offMaxDamage / 2.0f;
        }

        // DualWield coefficient
        if (dualwield)
        {
            minDamage *= 0.898882275f;
            maxDamage *= 0.898882275f;
        }

        minDamage += float(AP / APmultiplier);
        maxDamage += float(AP / APmultiplier);

        // Off Hand penalty reapplied if only equiped by an off hand weapon
        if (offItem && !mainItem)
        {
            minDamage /= 2.0f;
            maxDamage /= 2.0f;
        }
    }
    else
    {
        if (caster->GetEntry() != 69792) // Earth Spirit
            dualwield = true;

        if (dualwield)
        {
            minDamage += caster->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
            minDamage += caster->GetWeaponDamageRange(OFF_ATTACK, MINDAMAGE) / 2;
            maxDamage += caster->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
            maxDamage += caster->GetWeaponDamageRange(OFF_ATTACK, MAXDAMAGE) / 2;

            minDamage *= 0.898882275f;
            maxDamage *= 0.898882275f;
        }
        else
        {
            minDamage += caster->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
            maxDamage += caster->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
        }

        minDamage /= float(m_caster->GetAttackTime(BASE_ATTACK) / 1000.0f);
        maxDamage /= float(m_caster->GetAttackTime(BASE_ATTACK) / 1000.0f);

        minDamage += float(AP / APmultiplier);
        maxDamage += float(AP / APmultiplier);
    }

    return irand(int32(minDamage * coeff), int32(maxDamage * coeff));
}

void Spell::EffectUnlockGuildVaultTab(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT)
        return;

    // Safety checks done in Spell::CheckCast
    Player* caster = m_caster->ToPlayer();
    if (Guild* guild = caster->GetGuild())
        guild->HandleBuyBankTab(caster->GetSession(), m_spellInfo->Effects[effIndex].BasePoints - 1); // Bank tabs start at zero internally
}

void Spell::EffectResurrectWithAura(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || !unitTarget->IsInWorld())
        return;

    Player* target = unitTarget->ToPlayer();
    if (!target)
        return;
    if (target->isAlive())
        return;

    if (target->IsRessurectRequested())       // already have one active request
        return;

    uint32 health = target->CountPctFromMaxHealth(damage);
    uint32 mana   = CalculatePct(target->GetMaxPower(POWER_MANA), damage);

    uint32 resurrectAura = 0;
    if (sSpellMgr->GetSpellInfo(GetSpellInfo()->Effects[effIndex].TriggerSpell))
        resurrectAura = GetSpellInfo()->Effects[effIndex].TriggerSpell;

    if (resurrectAura && target->HasAura(resurrectAura))
        return;

    ExecuteLogEffectResurrect(effIndex, target);
    target->SetResurrectRequestData(m_caster, health, mana, resurrectAura);
    SendResurrectRequest(target);
}

void Spell::EffectStartScene(SpellEffIndex effIndex)
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET)
        return;

    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 sceneId = m_spellInfo->Effects[effIndex].MiscValue;

    unitTarget->ToPlayer()->GetSceneMgr().PlayScene(sceneId);
}
