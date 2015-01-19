/*
* Copyright (C) 2013 Skymist Project
*
* This file is NOT free software. You may NOT copy, redistribute it or modify it.
*/

/* ScriptData
SDName: Elwynn_Forest
SD%Complete: ??
SDComment: Quest support.
SDCategory: Elwynn Forest
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "World.h"
#include "PetAI.h"
#include "PassiveAI.h"
#include "CombatAI.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "SpellAuras.h"
#include "Vehicle.h"
#include "Player.h"
#include "SpellScript.h"
#include "Group.h"

enum Northshire
{
    NPC_BLACKROCK_BATTLE_WORG = 49871,
    NPC_STORMWIND_INFANTRY    = 49869,
    NPC_BROTHER_PAXTON        = 951,

    SAY_INFANTRY_NORMAL_1     = 0,
    SAY_INFANTRY_NORMAL_2     = 1,
    SAY_INFANTRY_NORMAL_3     = 2,
    SAY_INFANTRY_NORMAL_4     = 3,
    SAY_INFANTRY_NORMAL_5     = 4,

    SAY_INFANTRY_COMBAT_1     = 5,
    SAY_INFANTRY_COMBAT_2     = 6,
    SAY_INFANTRY_COMBAT_3     = 7,
    SAY_INFANTRY_COMBAT_4     = 8,

    SAY_PAXTON_NORMAL_1       = 0,
    SAY_PAXTON_NORMAL_2       = 1,
    SAY_PAXTON_NORMAL_3       = 2,
    SAY_PAXTON_NORMAL_4       = 3,
    SAY_PAXTON_NORMAL_5       = 4,

    SPELL_RENEW               = 93094,
    SPELL_PRAYER_OF_HEALING   = 93091,
    SPELL_FORTITUDE           = 13864,
    SPELL_PENANCE             = 47750,
    SPELL_FLASH_HEAL          = 17843,

    SPELL_RENEWEDLIFE         = 93097,

    SPELL_SPYING              = 92857,
    SPELL_SNEAKING            = 93046,
    SPELL_SPYGLASS            = 80676
};

/*######
## npc_stormwind_infantry
######*/

class npc_stormwind_infantry : public CreatureScript
{
public:
    npc_stormwind_infantry() : CreatureScript("npc_stormwind_infantry") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_stormwind_infantryAI (creature);
    }

    struct npc_stormwind_infantryAI : public ScriptedAI
    {
        npc_stormwind_infantryAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 uiSayNormalTimer;
        uint32 uiSayCombatTimer;
        uint32 DamageCount;

        void Reset()
        {
            if (me->HasUnitState(UNIT_STATE_ROOT))
                me->ClearUnitState(UNIT_STATE_ROOT);

            uiSayNormalTimer = urand(20000, 90000);
            uiSayCombatTimer = urand(10000, 80000);

            DamageCount = 0;
        }

        void EnterCombat(Unit* /*who*/)
        {
            if (!me->HasUnitState(UNIT_STATE_ROOT))
                me->AddUnitState(UNIT_STATE_ROOT);
        }

        void DamageTaken(Unit* who, uint32 &uiDamage)
        {
            if (who->GetTypeId() == TYPEID_UNIT && who->GetEntry() == NPC_BLACKROCK_BATTLE_WORG && !me->isInCombat())
                me->AI()->AttackStart(who);
        }

        void PaxtonCast(uint32 type)
        {
            if (Creature* Paxton = me->FindNearestCreature(NPC_BROTHER_PAXTON, 20.0f, true))
            {
                Paxton->SetOrientation(me->GetAngle(Paxton));
                Paxton->AI()->Talk(RAND(SAY_PAXTON_NORMAL_1, SAY_PAXTON_NORMAL_2, SAY_PAXTON_NORMAL_3, SAY_PAXTON_NORMAL_4, SAY_PAXTON_NORMAL_5));

                switch(type)
                {
                    case 1:
                        Paxton->AI()->DoCast(me, SPELL_PRAYER_OF_HEALING);
                        break;
                    case 2:
                        Paxton->AI()->DoCast(me, SPELL_PENANCE);
                        break;
                    case 3:
                        Paxton->AI()->DoCast(me, SPELL_RENEW);
                        break;
                    case 4:
                        Paxton->AI()->DoCast(me, SPELL_FLASH_HEAL);
                        break;
                    default: break;
                }
            }
        }

        void DamageDealt(Unit* target, uint32& damage, DamageEffectType damageType)
        {
            if (target->GetEntry() == NPC_BLACKROCK_BATTLE_WORG)
                ++DamageCount;

            if (DamageCount >= 2)
            {
                if (target->GetEntry() == NPC_BLACKROCK_BATTLE_WORG && target->HealthBelowPct(100))
                    damage = 0;
                else
                    DamageCount = 0;
            }
        }

        void EnterEvadeMode()
        {
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            me->GetMotionMaster()->MoveTargetedHome();
        }

        void UpdateAI(uint32 const diff)
        {
            if (!UpdateVictim())
                return;

            if (uiSayNormalTimer <= diff)
            {
                Talk(RAND(SAY_INFANTRY_NORMAL_1, SAY_INFANTRY_NORMAL_2, SAY_INFANTRY_NORMAL_3, SAY_INFANTRY_NORMAL_4, SAY_INFANTRY_NORMAL_5));
                uiSayNormalTimer = urand(30000, 120000);
            }else uiSayNormalTimer -= diff;

            if (me->HealthBelowPct(100))
            {
                if (uiSayCombatTimer <= diff)
                {
                    if (Creature* Paxton = me->FindNearestCreature(NPC_BROTHER_PAXTON, 20.0f, true))
                    {
                        Talk(RAND(SAY_INFANTRY_COMBAT_1, SAY_INFANTRY_COMBAT_2, SAY_INFANTRY_COMBAT_3, SAY_INFANTRY_COMBAT_4));
                        PaxtonCast(urand(1, 4));
                    }
                    uiSayCombatTimer = urand(10000, 100000);
                }else uiSayCombatTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

/*######
## npc_brother_paxton
######*/

class npc_brother_paxton : public CreatureScript
{
public:
    npc_brother_paxton() : CreatureScript("npc_brother_paxton") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_brother_paxtonAI (creature);
    }

    struct npc_brother_paxtonAI : public ScriptedAI
    {
        npc_brother_paxtonAI(Creature* creature) : ScriptedAI(creature)
        {
            creature->GetMotionMaster()->MovePath(951, true);
        }

        void Reset()
        {
            if (!me->HasAura(SPELL_FORTITUDE))
                DoCast(me, SPELL_FORTITUDE);
            me->SetReactState(REACT_PASSIVE);
        }

        void EnterEvadeMode()
        {
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            me->GetMotionMaster()->MoveTargetedHome();
        }

        void UpdateAI(uint32 const diff)
        {
            if (!me->HasAura(SPELL_FORTITUDE) && !me->HasUnitState(UNIT_STATE_CASTING))
                DoCast(me, SPELL_FORTITUDE);

            if (me->HasUnitState(UNIT_STATE_CASTING) && me->isMoving())
                me->StopMoving();
            else if (!me->HasUnitState(UNIT_STATE_CASTING) && !me->isMoving())
                me->GetMotionMaster()->MovePath(951, true);

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

/*######
## npc_blackrock_battle_worg
######*/

class npc_blackrock_battle_worg : public CreatureScript
{
public:
    npc_blackrock_battle_worg() : CreatureScript("npc_blackrock_battle_worg") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_blackrock_battle_worgAI (creature);
    }

    struct npc_blackrock_battle_worgAI : public ScriptedAI
    {
        npc_blackrock_battle_worgAI(Creature* creature) : ScriptedAI(creature)  { }

        uint32 DamageCount;
        uint32 AttackTimer;
        bool isMovingHome;

        void Reset()
        {
            DamageCount = 0;
            AttackTimer = 1000;
            isMovingHome = false;
        }

        void EnterEvadeMode()
        {
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            me->GetMotionMaster()->MoveTargetedHome();
            isMovingHome = true;
        }

        void JustReachedHome()
        {
            isMovingHome = false;
        }

        void DamageDealt(Unit* target, uint32& damage, DamageEffectType damageType)
        {
            if (target->GetEntry() == NPC_STORMWIND_INFANTRY)
                ++DamageCount;

            if (DamageCount >= 2)
            {
                if (target->GetEntry() == NPC_STORMWIND_INFANTRY && target->HealthBelowPct(100))
                    damage = 0;
                else
                    DamageCount = 0;
            }
        }

        void DamageTaken(Unit* who, uint32 &uiDamage)
        {
            if (who->GetTypeId() == TYPEID_PLAYER || who->isPet())
            {
                me->getThreatManager().resetAllAggro();
                me->AddThreat(who, 100.0f);
                me->AI()->AttackStart(who);
                DamageCount = 0;
            }
        }

        void UpdateAI(uint32 const diff)
        {
            if (AttackTimer <= diff)
            {
                if (!me->isInCombat() && !isMovingHome && me->FindNearestCreature(NPC_STORMWIND_INFANTRY, 7.0f))
                if (Creature* infantry = me->FindNearestCreature(NPC_STORMWIND_INFANTRY, 7.0f))
                    me->AI()->AttackStart(infantry);
                AttackTimer = 2000;
            } else AttackTimer -= diff;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_injured_soldier : public CreatureScript
{
public:
    npc_injured_soldier() : CreatureScript("npc_injured_soldier") { }

    struct npc_injured_soldierAI : public ScriptedAI
    {
        npc_injured_soldierAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 RunTimer;
        uint8 Phase;
        bool IsHealed;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_15);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_14);
            me->SetStandState(UNIT_STAND_STATE_DEAD);
            IsHealed = false;
            RunTimer = -1;
            Phase = 0;
        }

        void OnSpellClick(Unit* clicker, bool& result)
        {
            if (!IsHealed && result)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_15);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_14);
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->SetReactState(REACT_PASSIVE);
                IsHealed = true;
                RunTimer = 2000;
            }
        }

        void UpdateAI(uint32 const diff)
        {
            if (IsHealed)
            {
                if (RunTimer <= diff)
                {
                    switch(Phase)
                    {
                        case 0:
                        {
                            switch(urand(0, 3))
                            {
                                case 0: me->MonsterSay("Bless you, hero!", LANG_UNIVERSAL, 0); break;
                                case 1: me->MonsterSay("I will fear no evil!", LANG_UNIVERSAL, 0); break;
                                case 2: me->MonsterSay("Thank the Light!", LANG_UNIVERSAL, 0); break;
                                case 3: me->MonsterSay("You're the hero that everyone has been talking about! Thank you!", LANG_UNIVERSAL, 0); break;
                                default: break;
                            }

                            me->HandleEmote(EMOTE_ONESHOT_WAVE);
                            RunTimer = 2000;
                            Phase++;
                            break;
                        }
                        case 1:
                        {
                            float x, y, z;
                            me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 8.0f);
                            me->GetMotionMaster()->MovePoint(1, x, y, z);
                            RunTimer = 6000;
                            Phase++;
                            break;
                        }
                        case 2: me->DespawnOrUnsummon(); break;
                        default: break;
                    }
                } else RunTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_injured_soldierAI (creature);
    }
};

enum Hogger
{
    SPELL_TELEPORT_VIS  = 87459,

    SPELL_EATING        = 87351,
    SPELL_UPSET_STOMACH = 87352,
    SPELL_VICIOUS_SLICE = 87337,

    NPC_GENERAL_MARCUS  = 46942,
    NPC_MAGINOR_DUMAS   = 46940, // His left
    NPC_ANDROMATH       = 46941  // His right
};

#define SAY_HOGGER_A  "Grrrr... fresh meat!" 
#define SAY_HOGGER_E  "Yipe! Help Hogger!" 
#define SAY_EAT       "Hogger is eating! Stop him!" // Announce
#define SAY_HOGGER_HP "No hurt Hogger!"
#define SAY_HOGGER_K  "More bones to gnaw on..."

class npc_hogger : public CreatureScript
{
public:
    npc_hogger() : CreatureScript("npc_hogger") { }

    struct npc_hoggerAI : public ScriptedAI
    {
        npc_hoggerAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 ViciousSliceTimer;
        uint32 EatTimer;
        uint32 MarcusSayTimer1; // Hold your blade, adventurer!
        uint32 HoggerSayTimer1; // Grrr... - above + 5s.
        uint32 MarcusSayTimer2; // This beast leads the Riverpaw Gnoll gang and may be the key to ending gnoll aggression in Elwynn. - above + 4s
        uint32 MarcusSayTimer3; // We're taking him into custody in the name of King Varian Wrynn. - above + 8s
        uint32 HoggerSayTimer2; // Nooooo... - above + 6s.
        uint32 MarcusSayTimer4; // Take us to the Stockades, Andromath. - above + 4s.
        uint32 DespawnTimer;
        Creature* Marcus;
        Creature* Maginor;
        Creature* Andromath;
        bool Eaten, MovedHome, MarcusSummoned;

        void Reset()
        {
            me->SetReactState(REACT_AGGRESSIVE);

            ViciousSliceTimer = urand(2000, 3000);
            EatTimer          = -1;
            MarcusSayTimer1   = -1;
            HoggerSayTimer1   = -1;
            MarcusSayTimer2   = -1;
            MarcusSayTimer3   = -1;
            HoggerSayTimer2   = -1;
            MarcusSayTimer4   = -1;
            DespawnTimer      = -1;
            Marcus            = NULL;
            Maginor           = NULL;
            Andromath         = NULL;
            Eaten             = false;
            MovedHome         = false;
            MarcusSummoned    = false;
        }

        void EnterEvadeMode()
        {
            me->RemoveAllAuras();
            Reset();
            me->DeleteThreatList();
            me->CombatStop(false);
            if (Eaten)
            {
                me->GetMotionMaster()->MovementExpired();
                me->GetMotionMaster()->Clear();
            }
            me->GetMotionMaster()->MoveTargetedHome();
        }

        void EnterCombat(Unit* who)
        {
            if (who->GetTypeId() == TYPEID_PLAYER || who->isPet())
                me->MonsterYell(SAY_HOGGER_A, LANG_UNIVERSAL, 0);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                me->MonsterYell(SAY_HOGGER_K, LANG_UNIVERSAL, 0);
        }

        void DamageTaken(Unit* who, uint32 &damage)
        {
            if (who->GetTypeId() == TYPEID_PLAYER || who->isPet())
            {
                if (me->HealthBelowPct(51) && !Eaten) // Eat phase
                {
                    me->MonsterYell(SAY_HOGGER_E, LANG_UNIVERSAL, 0);
                    me->MonsterTextEmote(SAY_EAT, NULL, true);
                    me->GetMotionMaster()->MovementExpired();
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MovePoint(1, -10144.489f, 668.569f, 35.971f);
                    EatTimer = 3000;
                    Eaten = true;
                }

                if (damage >= me->GetHealth()) // Final phase
                {
                    damage = me->GetHealth() - 1;

                    if (!MovedHome && Eaten)
                    {
                        if (Player* player = (who->GetTypeId() == TYPEID_PLAYER) ? who->ToPlayer() : who->GetOwner()->ToPlayer())
						{
                            if (player->GetGroup())
                            {
                                if (Group* group = player->GetGroup())
                                    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                                        if (Player* member = itr->getSource())
                                            member->KilledMonsterCredit(448, 0);
                            }
                            else player->KilledMonsterCredit(448, 0);
						}

                        me->MonsterYell(SAY_HOGGER_HP, LANG_UNIVERSAL, 0);
                        if (me->HasAura(SPELL_EATING))
                            me->RemoveAurasDueToSpell(SPELL_EATING);
                        if (me->HasAura(SPELL_UPSET_STOMACH))
                            me->RemoveAurasDueToSpell(SPELL_UPSET_STOMACH);
                        me->SetReactState(REACT_PASSIVE);
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MoveTargetedHome();
                        MovedHome = true;
                    }
                }
            }
        }

        void JustReachedHome()
        {
            if (!MarcusSummoned && MovedHome)
            {
                float x, y, z;
                me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 12.0f);

                if (Marcus = me->SummonCreature(NPC_GENERAL_MARCUS, x, y, z, 0, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    Maginor = me->SummonCreature(NPC_MAGINOR_DUMAS, x, y + 5, z, Marcus->GetAngle(me), TEMPSUMMON_MANUAL_DESPAWN);
                    Andromath = me->SummonCreature(NPC_ANDROMATH, x, y - 5, z, Marcus->GetAngle(me), TEMPSUMMON_MANUAL_DESPAWN);
                    Maginor->SetReactState(REACT_PASSIVE);
                    Andromath->SetReactState(REACT_PASSIVE);
                    Maginor->CastSpell(Maginor, SPELL_TELEPORT_VIS, true);
                    Andromath->CastSpell(Andromath, SPELL_TELEPORT_VIS, true);

                    me->SetFacingTo(me->GetAngle(Marcus));
                    Marcus->SetReactState(REACT_PASSIVE);
                    Marcus->SetFacingTo(Marcus->GetAngle(me));
                    Marcus->CastSpell(Marcus, SPELL_TELEPORT_VIS, true);
                    Marcus->Mount(2410);
                    MarcusSayTimer1 = 3000;

                    if (Creature* Ragamuffin1 = me->SummonCreature(46943, x - 3, y - 6, z, Marcus->GetAngle(me), TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        Ragamuffin1->SetReactState(REACT_PASSIVE);
                        Ragamuffin1->MonsterSay("General Marcus Jonathan!", LANG_UNIVERSAL, 0);
                        Ragamuffin1->DespawnOrUnsummon(6000);
                    }

                    if (Creature* Ragamuffin2 = me->SummonCreature(46943, x - 4, y + 6, z, Marcus->GetAngle(me), TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        Ragamuffin2->SetReactState(REACT_PASSIVE);
                        Ragamuffin2->MonsterSay("Wow!", LANG_UNIVERSAL, 0);
                        Ragamuffin2->DespawnOrUnsummon(6000);
                    }
                }

                MarcusSummoned = true;
            }
        }

        void UpdateAI(uint32 const diff)
        {
            if (Eaten)
            {
                if (EatTimer <= diff)
                {
                    if (me->isMoving()) me->StopMoving();
                    DoCast(me, SPELL_EATING);
                    EatTimer = -1;
                } else EatTimer -= diff;

                if (!me->HasAura(SPELL_EATING) && !me->HasAura(SPELL_UPSET_STOMACH) && !MovedHome && !me->isMoving())
                {
                    me->GetMotionMaster()->MovementExpired();
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveChase(me->getVictim());
                }

                if (MarcusSummoned)
                {
                    if (MarcusSayTimer1 <= diff)
                    {
                        Marcus->MonsterYell("Hold your blade, adventurer!", LANG_UNIVERSAL, 0);
                        Marcus->HandleEmote(EMOTE_ONESHOT_SHOUT);
                        MarcusSayTimer1 = -1;
                        HoggerSayTimer1 = 5000;
                    } else MarcusSayTimer1 -= diff;

                    if (HoggerSayTimer1 <= diff)
                    {
                        me->MonsterSay("Grrr...", LANG_UNIVERSAL, 0);
                        Marcus->Dismount();
                        float x, y, z;
                        me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 5.0f);
                        Marcus->GetMotionMaster()->MovePoint(1, x, y, z);
                        HoggerSayTimer1 = -1;
                        MarcusSayTimer2 = 4000;
                    } else HoggerSayTimer1 -= diff;

                    if (MarcusSayTimer2 <= diff)
                    {
                        Marcus->MonsterSay("This beast leads the Riverpaw Gnoll gang and may be the key to ending gnoll aggression in Elwynn.", LANG_UNIVERSAL, 0);
                        Marcus->HandleEmote(EMOTE_ONESHOT_POINT);
                        MarcusSayTimer2 = -1;
                        MarcusSayTimer3 = 8000;
                    } else MarcusSayTimer2 -= diff;

                    if (MarcusSayTimer3 <= diff)
                    {
                        Marcus->MonsterSay("We're taking him into custody in the name of King Varian Wrynn.", LANG_UNIVERSAL, 0);
                        Marcus->HandleEmote(EMOTE_ONESHOT_EXCLAMATION);
                        MarcusSayTimer3 = -1;
                        HoggerSayTimer2 = 6000;
                    } else MarcusSayTimer3 -= diff;

                    if (HoggerSayTimer2 <= diff)
                    {
                        me->MonsterSay("Nooooo...", LANG_UNIVERSAL, 0);
                        HoggerSayTimer2 = -1;
                        MarcusSayTimer4 = 4000;
                    } else HoggerSayTimer2 -= diff;

                    if (MarcusSayTimer4 <= diff)
                    {
                        Marcus->MonsterSay("Take us to the Stockades, Andromath.", LANG_UNIVERSAL, 0);
                        Marcus->SetFacingTo(Marcus->GetAngle(Andromath));
                        MarcusSayTimer4 = -1;
                        DespawnTimer = 3000;
                    } else MarcusSayTimer4 -= diff;

                    if (DespawnTimer <= diff)
                    {
                        Andromath->MonsterSay("Right away, General!", LANG_UNIVERSAL, 0);
                        Andromath->HandleEmote(EMOTE_ONESHOT_SALUTE);
                        Maginor->CastSpell(Maginor, SPELL_TELEPORT_VIS, true);
                        Andromath->CastSpell(Andromath, SPELL_TELEPORT_VIS, true);
                        Marcus->CastSpell(Marcus, SPELL_TELEPORT_VIS, true);
                        me->CastSpell(me, SPELL_TELEPORT_VIS, true);
                        Maginor->DespawnOrUnsummon(1000);
                        Andromath->DespawnOrUnsummon(1000);
                        me->DespawnOrUnsummon(1000);
                        Marcus->DespawnOrUnsummon(1000);
                        DespawnTimer = -1;
                    } else DespawnTimer -= diff;
                }
            }

            if (!UpdateVictim())
                return;

            if (ViciousSliceTimer <= diff && !MarcusSummoned)
            {
                DoCastVictim(SPELL_VICIOUS_SLICE);
                ViciousSliceTimer = urand(7000, 11000);
            } else ViciousSliceTimer -= diff;

            if (!MarcusSummoned)
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_hoggerAI (creature);
    }
};

// Spray Water 80199.

#define SPELL_VISUAL_EXTINGUISHER   96028

class spell_spray_water : public SpellScriptLoader
{
    public:
        spell_spray_water() : SpellScriptLoader("spell_spray_water") { }

        class spell_spray_water_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_spray_water_AuraScript);

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster())
                    GetCaster()->AddAura(SPELL_VISUAL_EXTINGUISHER, GetCaster());
            }

            void OnPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                if (!GetCaster())
                    return;

                if (Creature* fire = GetCaster()->FindNearestCreature(42940, 5.0f, true))
                {
                    if (Player* player = GetCaster()->ToPlayer())
                        player->KilledMonsterCredit(42940, 0);

                    fire->DespawnOrUnsummon();
                }
            }

            void OnRemove(constAuraEffectPtr, AuraEffectHandleModes)
            {
                if (GetCaster())
                    GetCaster()->RemoveAurasDueToSpell(SPELL_VISUAL_EXTINGUISHER);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_spray_water_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_spray_water_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                OnEffectRemove += AuraEffectRemoveFn(spell_spray_water_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_spray_water_AuraScript();
        }
};

void AddSC_elwyn_forest()
{
    new npc_stormwind_infantry();
    new npc_brother_paxton();
    new npc_blackrock_battle_worg();
    new npc_injured_soldier();
    new npc_hogger();
    new spell_spray_water();
}
