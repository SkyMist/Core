#include "ScriptPCH.h"
#include "throne_of_the_four_winds.h"
#include "MoveSplineInit.h"

enum ScriptTextAnshal
{
    SAY_ANSHAL_AGGRO        = 0,  
    SAY_ANSHAL_KILL         = 1,
    SAY_ANSHAL_ENRAGE       = 2,
    SAY_CONCLAVE_ULTIMATE   = 3,
    SAY_ANSHAL_LOW          = 4,
};

enum ScriptTextNezir
{
    SAY_NEZIR_AGGRO         = 0,  
    SAY_NEZIR_KILL          = 1,
    SAY_NEZIR_ENRAGE        = 2,
    SAY_NEZIR_LOW           = 4,
};

enum ScriptTextRohash
{
    SAY_ROHASH_AGGRO        = 0,  
    SAY_ROHASH_KILL         = 1,
    SAY_ROHASH_ENRAGE       = 2,
    SAY_ROHASH_LOW          = 4,
};

enum Spells
{
    SPELL_NO_REGEN                  = 96301,

    // Anshal
    SPELL_PREFIGHT_VISUAL_WEST      = 85537,
    SPELL_SOOTHING_BREEZE           = 86205,
    SPELL_SOOTHING_BREEZE_SUMMON    = 86204,
    SPELL_SOOTHING_BREEZE_AURA_1    = 86206,
    SPELL_SOOTHING_BREEZE_AURA_2    = 86207,
    SPELL_SOOTHING_BREEZE_VISUAL    = 86208,
    SPELL_NURTURE                   = 85422,
    SPELL_NURTURE_SUMMON_TRIGGER    = 85425,
    SPELL_NURTURE_DUMMY             = 85428,
    SPELL_NURTURE_SUMMON            = 85429,
    SPELL_TOXIC_SPORES_FORCE        = 76290,
    SPELL_TOXIC_SPORES              = 86281,
    SPELL_TOXIC_SPORES_DMG          = 86282,
    SPELL_TOXIC_SPORES_DMG_25       = 93120,
    SPELL_TOXIC_SPORES_DMG_10H      = 93121,
    SPELL_TOXIC_SPORES_DMG_25H      = 93122,
    SPELL_ZEPHYR                    = 84638,
    SPELL_DUMMY_FOR_NURTURE         = 89813,
    SPELL_WITHERING_WINDS           = 85576,
    SPELL_WITHERING_WINDS_DAMAGE    = 93168,

    // Nezir
    SPELL_PREFIGHT_VISUAL_NORTH     = 85532,
    SPELL_ICE_PATCH                 = 86122,
    SPELL_ICE_PATCH_VISUAL          = 86107, 
    SPELL_ICE_PATCH_AURA            = 86111,
    SPELL_PERMAFROST                = 86082,
    SPELL_WIND_CHILL                = 84645,
    SPELL_SLEET_STORM               = 84644,
    SPELL_SLEET_STORM_DMG           = 86367,
    SPELL_SLEET_STORM_DMG_25        = 93135,
    SPELL_SLEET_STORM_DMG_10H       = 93136,
    SPELL_SLEET_STORM_DMG_25H       = 93137,
    SPELL_CHILLING_WINDS            = 85578,
    SPELL_CHILLING_WINDS_DAMAGE     = 93163,

    // Rohash
    SPELL_PREFIGHT_VISUAL_EAST      = 85538,
    SPELL_SLICING_GALE              = 86182,
    SPELL_WIND_BLAST                = 86193, //casts the other spell...
    SPELL_WIND_BLAST_EFFECT         = 85483, //casted by this 85480
    SPELL_TORNADO_AURA              = 86134, 
    SPELL_TORNADO_SUMMON            = 86192,
    SPELL_SLOW_SPIN                 = 99380,  //For Spinning and Wind Blast
    SPELL_STORM_SHIELD              = 93059,
    SPELL_DEAFENING_WINDS           = 85573,
    SPELL_DEAFENING_WINDS_DAMAGE    = 93166,
    SPELL_HURRICANE_ULTIMATE        = 84643,  
    SPELL_HURRICANE_DUMMY           = 86498, // triggered from main spell
    SPELL_HURRICANE_VEHICLE         = 86492, // casted by player
    SPELL_HURRICANE_DAMAGE          = 86487,

    // All
    SPELL_GATHER_STRENGHT           = 86307,
    SPELL_DEAD                      = 81238, // Dead look.
    //SPELL_CANNOT_TURN               = 95544,
    SPELL_TELEPORT_VISUAL           = 83369,

    // Instance
    SPELL_WINDDRAFT                 = 84576,
    SPELL_JETSTREAM_PREVENT         = 89771,
    SPELL_BERSERK                   = 82395,
    SPELL_WIND_PRE_EFFECT_WARNING   = 96508,

    // Achievements
    SPELL_ACHIEVEMENT_CONCLAVE      = 88835,
};

enum GameobjectIds
{
    GOB_WIND_DRAFTEFFECT_CENTER     = 207922,
    GOB_RAIDPLATFORM                = 207737,

    GOB_WIND_DRAFTEFFECT_1          = 207923,
    GOB_WIND_DRAFTEFFECT_2          = 207924,
    GOB_WIND_DRAFTEFFECT_3          = 207925,
    GOB_WIND_DRAFTEFFECT_4          = 207926,
    GOB_WIND_DRAFTEFFECT_5          = 207929,
    GOB_WIND_DRAFTEFFECT_6          = 207930,
    GOB_WIND_DRAFTEFFECT_7          = 207927,
    GOB_WIND_DRAFTEFFECT_8          = 207928,

    GOB_DEIJING_HEALING             = 206699,
    GOB_DEIJING_FROST               = 206700,
    GOB_DEIJING_TORNADO             = 206701
};

enum Adds
{
    NPC_TORNADO                     = 46207,
    NPC_HURRICANE                   = 46419,
    NPC_RAVENOUS_TRIGGER            = 45813,
    NPC_RAVENOUS_CREEPER            = 45812,
    NPC_SOOTHING_BREEZE             = 46246,
};

enum Events
{
    // Anshal
    EVENT_SOOTHING_BREEZE           = 1,
    EVENT_NURTURE,
    EVENT_ULTIMATE_ANSHAL,
    EVENT_TOXIC_SPORES,
    EVENT_ZEPHYR_1,
    EVENT_ZEPHYR_2,

    // Nezir
    EVENT_ICE_PATCH,
    EVENT_PERMAFROST,
    EVENT_WIND_CHILL,
    EVENT_SLEET_STORM_1,
    EVENT_SLEET_STORM_2,

    // Rohash
    EVENT_SLICING_GALE,
    EVENT_WIND_BLAST,
    EVENT_TORNADO,
    EVENT_STORM_SHIELD,
    EVENT_HURRICANE,
    EVENT_HURRICANE_1,
    EVENT_HURRICANE_2,
    
    //All
    EVENT_UPDATE_VICTIM,
    EVENT_REVIVE,
    EVENT_GATHER_STRENGHT,
    EVENT_START_MOVE,
    EVENT_UPDATE_ENERGY,
};

enum Others
{
    DATA_DEAD       = 1,
    DATA_ULTIMATE   = 2,
};

class CouncilGameObject
{
    public:
        CouncilGameObject() { }

        bool operator()(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GOB_WIND_DRAFTEFFECT_CENTER:
                    go->SetGoState(GO_STATE_READY);
                    break;
                case GOB_DEIJING_HEALING:
                case GOB_DEIJING_FROST:
                case GOB_DEIJING_TORNADO:
                    go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GOB_RAIDPLATFORM:
                    go->SetGoState(GO_STATE_READY);
                    break;
                case GOB_WIND_DRAFTEFFECT_1:
                case GOB_WIND_DRAFTEFFECT_2:
                case GOB_WIND_DRAFTEFFECT_3:
                case GOB_WIND_DRAFTEFFECT_4:
                case GOB_WIND_DRAFTEFFECT_5:
                case GOB_WIND_DRAFTEFFECT_6:
                case GOB_WIND_DRAFTEFFECT_7:
                case GOB_WIND_DRAFTEFFECT_8:
                    go->SetGoState(GO_STATE_READY);
                    break;
                default:
                    break;
            }

            return false;
        }
};


class boss_anshal : public CreatureScript
{
    public:
        boss_anshal() : CreatureScript("boss_anshal") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_anshalAI (creature);
        }

        struct boss_anshalAI : public ScriptedAI
        {
            boss_anshalAI(Creature* pCreature) : ScriptedAI(pCreature), summons(me)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);

                me->setActive(true);
                me->SetFloatValue(UNIT_FIELD_COMBATREACH, 5.0f);

                pInstance = pCreature->GetInstanceScript();

                m_isDead = false;
                hasNoTarget = false;
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();
                DespawnCreatures(NPC_SOOTHING_BREEZE);
                
                me->RemoveAura(SPELL_DEAD); // Dead Look

                DoCast(me, SPELL_NO_REGEN, true);
                
                me->SetMaxPower(POWER_MANA, 90);
                me->SetPower(POWER_MANA, 0);

                me->SetReactState(REACT_AGGRESSIVE);

                m_isDead = false;
                hasNoTarget = false;

                if (pInstance)
                    pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, NOT_STARTED);
            }

            void EnterCombat(Unit* who)
            {
                me->RemoveAura(SPELL_PREFIGHT_VISUAL_WEST);

                events.ScheduleEvent(EVENT_NURTURE, 30000);
                events.ScheduleEvent(EVENT_SOOTHING_BREEZE, 16000);
                events.ScheduleEvent(EVENT_UPDATE_ENERGY, 1000);
                events.ScheduleEvent(EVENT_UPDATE_VICTIM, 10000);

                Talk(SAY_ANSHAL_AGGRO);

                me->SetMaxPower(POWER_MANA, 90);
                me->SetPower(POWER_MANA, 0);

                m_isDead = false;
                hasNoTarget = false;

                if (pInstance)
                {
                    if (Creature* pNezir = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_NEZIR)))
                        if (!pNezir->isInCombat())
                            DoZoneInCombat(pNezir, 600.0f);

                    if (Creature* pRohash = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ROHASH)))
                        if (!pRohash->isInCombat())
                            DoZoneInCombat(pRohash, 600.0f);

                    pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, IN_PROGRESS);
                }
            }

            void DamageTaken(Unit* dealer, uint32 &damage)
            {
                if (dealer->GetGUID() == me->GetGUID())
                    return;
                
                /*if (me->GetHealth() < damage)
                {
                    damage = me->GetHealth() - 1;

                    if (!m_isDead)
                    {
                        m_isDead = true;
                        if (pInstance)
                        {
                            Creature* pNezir = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_NEZIR));
                            Creature* pRohash = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ROHASH));
                            if (pNezir->AI()->GetData(DATA_DEAD) > 0 && pRohash->AI()->GetData(DATA_DEAD) > 0)
                            {
                                pNezir->Kill(pNezir);
                                pRohash->Kill(pRohash);
                                me->Kill(me);
                            }
                            else
                            {
                                events.Reset();
                                me->SetReactState(REACT_PASSIVE);
                                me->AttackStop();
                                DoCast(me, SPELL_GATHER_STRENGHT);
                                Talk(SAY_ANSHAL_LOW);
                            }
                        }
                    }
                }*/
            }

            void JustSummoned(Creature *summon)
            {
                summons.Summon(summon);
                if (me->isInCombat())
                    DoZoneInCombat(summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void JustDied(Unit* killer)
            {
                events.Reset();
                summons.DespawnAll();

                pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, DONE);
            }
            
            void KilledUnit(Unit* victim)
            {
                Talk(SAY_ANSHAL_KILL);
            }

            uint32 GetData(uint32 type)
            {
                if (type == DATA_DEAD)
                    return (m_isDead ? 1 : 0);

                return 0;
            }
            
            void UpdateAI(const uint32 diff)
            {                    
                if (!UpdateVictim() || !pInstance)
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                
                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_UPDATE_VICTIM:
                            if (Unit* victim = me->getVictim())
                            {
                                if (me->GetHomePosition().GetExactDist2d(victim) > 60.0f)
                                {
                                    Unit* pTarget = NULL;
                                    pTarget = SelectTarget(SELECT_TARGET_NEAREST, 0, 55.0f, true);
                                    if (pTarget && me->GetHomePosition().GetExactDist2d(pTarget) <= 60.0f)
                                    {
                                        AttackStart(pTarget);
                                    }
                                    else if (!hasNoTarget)
                                    {
                                        hasNoTarget = true;
                                        Talk(SAY_ANSHAL_ENRAGE);
                                        //DoCast(me, SPELL_WITHERING_WINDS, true);
                                    }
                                }
                            }

                            events.ScheduleEvent(EVENT_UPDATE_VICTIM, 5000);
                            break;
                        case EVENT_SOOTHING_BREEZE:
                            if (Creature* creeper = me->FindNearestCreature(NPC_RAVENOUS_CREEPER, 90.0f, true))
                                DoCast(creeper, SPELL_SOOTHING_BREEZE);
                            else
                                DoCast(me, SPELL_SOOTHING_BREEZE);
                            events.ScheduleEvent(EVENT_SOOTHING_BREEZE, 32000);
                            break;
                        case EVENT_NURTURE:
                            DoCast(me, SPELL_NURTURE, true);
                            me->AddAura(SPELL_DUMMY_FOR_NURTURE, me);
                            events.ScheduleEvent(EVENT_TOXIC_SPORES, urand(5000, 6000));
                            break;
                        case EVENT_TOXIC_SPORES:
                            DoCastAOE(SPELL_TOXIC_SPORES_FORCE, true);
                            events.ScheduleEvent(EVENT_TOXIC_SPORES, urand(16000, 20000));
                            break;
                        case EVENT_UPDATE_ENERGY:
                            if (me->GetPower(POWER_MANA) < 90)
                            {
                                int32 power = me->GetPower(POWER_MANA);
                                power++;
                                me->SetPower(POWER_MANA, power);
                                if (Creature* pNezir = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_NEZIR)))
                                    pNezir->SetPower(POWER_MANA, power);
                                if (Creature* pRohash = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ROHASH)))
                                    pRohash->SetPower(POWER_MANA, power);

                                events.ScheduleEvent(EVENT_UPDATE_ENERGY, 1000);
                            }
                            else
                            {
                                events.CancelEvent(EVENT_SOOTHING_BREEZE);
                                events.CancelEvent(EVENT_NURTURE);
                                events.CancelEvent(EVENT_TOXIC_SPORES);
                                events.CancelEvent(EVENT_UPDATE_VICTIM);

                                summons.DespawnEntry(NPC_SOOTHING_BREEZE);

                                me->NearTeleportTo(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), me->GetHomePosition().GetOrientation());
                                me->UpdateObjectVisibility(true);

                                if (Creature* pNezir = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_NEZIR)))
                                    pNezir->AI()->DoAction(DATA_ULTIMATE);
                                if (Creature* pRohash = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_NEZIR)))
                                    pRohash->AI()->DoAction(DATA_ULTIMATE);

                                Talk(SAY_CONCLAVE_ULTIMATE);

                                events.ScheduleEvent(EVENT_ZEPHYR_1, 1000);                                
                            }
                            break;
                        case EVENT_ZEPHYR_1:
                            DoCast(SPELL_ZEPHYR);
                            events.ScheduleEvent(EVENT_ZEPHYR_2, 16000);
                            break;
                        case EVENT_ZEPHYR_2:
                            me->SetPower(POWER_MANA, 0);

                            events.ScheduleEvent(EVENT_SOOTHING_BREEZE, 16000);
                            events.ScheduleEvent(EVENT_NURTURE, 35000);
                            events.ScheduleEvent(EVENT_UPDATE_ENERGY, 1000);
                            events.ScheduleEvent(EVENT_UPDATE_VICTIM, 10000);

                            me->SetReactState(REACT_AGGRESSIVE);
                            break;
                        default:
                            break;
                    }
                }        

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* pInstance;
            EventMap events;
            SummonList summons;
            bool m_isDead;
            bool hasNoTarget;

            void DespawnCreatures(uint32 entry)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 1000.0f);

                if (creatures.empty())
                   return;

                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                     (*iter)->DespawnOrUnsummon(2000);
            }
        };
};

class boss_nezir : public CreatureScript
{
    public:
        boss_nezir() : CreatureScript("boss_nezir") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_nezirAI (creature);
        }

        struct boss_nezirAI : public ScriptedAI
        {
            boss_nezirAI(Creature* pCreature) : ScriptedAI(pCreature), summons(me)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);

                me->setActive(true);
                me->SetFloatValue(UNIT_FIELD_COMBATREACH, 5.0f);

                pInstance = pCreature->GetInstanceScript();

                m_isDead = false;
                hasNoTarget = false;
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();
                
                me->RemoveAura(SPELL_DEAD); // Dead Look

                DoCast(me, SPELL_NO_REGEN, true);
                
                me->SetMaxPower(POWER_MANA, 90);
                me->SetPower(POWER_MANA, 0);

                me->SetReactState(REACT_AGGRESSIVE);

                m_isDead = false;
                hasNoTarget = false;

                if (pInstance)
                    pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, NOT_STARTED);
            }

            void EnterCombat(Unit* who)
            {
                me->RemoveAura(SPELL_PREFIGHT_VISUAL_NORTH);

                me->SetMaxPower(POWER_MANA, 90);
                me->SetPower(POWER_MANA, 0);

                events.ScheduleEvent(EVENT_WIND_CHILL, urand(5000, 10500));
                events.ScheduleEvent(EVENT_PERMAFROST, 10000);
                events.ScheduleEvent(EVENT_UPDATE_VICTIM, 10000);

                m_isDead = false;
                hasNoTarget = false;

                if (pInstance)
                {
                    if (Creature* pAnshal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ANSHAL)))
                        if (!pAnshal->isInCombat())
                            DoZoneInCombat(pAnshal, 600.0f);

                    if (Creature* pRohash = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ROHASH)))
                        if (!pRohash->isInCombat())
                            DoZoneInCombat(pRohash, 600.0f);

                    pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, IN_PROGRESS);
                }
            }

            void DoAction(const int32 action)
            {
                if (action == DATA_ULTIMATE)
                {
                    events.CancelEvent(EVENT_WIND_CHILL);
                    events.CancelEvent(EVENT_PERMAFROST);
                    events.CancelEvent(EVENT_UPDATE_VICTIM);

                    me->NearTeleportTo(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), me->GetHomePosition().GetOrientation());
                    me->UpdateObjectVisibility(true);

                    Talk(SAY_CONCLAVE_ULTIMATE);

                    events.ScheduleEvent(EVENT_SLEET_STORM_1, 1000);
                }
            }

            void DamageTaken(Unit* dealer, uint32 &damage)
            {
                if (dealer->GetGUID() == me->GetGUID())
                    return;
                
                /*if (me->GetHealth() < damage)
                {
                    damage = me->GetHealth() - 1;

                    if (!m_isDead)
                    {
                        m_isDead = true;
                        if (pInstance)
                        {
                            Creature* pAnshal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ANSHAL));
                            Creature* pRohash = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ROHASH));
                            if (pAnshal->AI()->GetData(DATA_DEAD) > 0 && pRohash->AI()->GetData(DATA_DEAD) > 0)
                            {
                                pAnshal->Kill(pAnshal);
                                pRohash->Kill(pRohash);
                                me->Kill(me);
                            }
                            else
                            {
                                events.Reset();
                                me->SetReactState(REACT_PASSIVE);
                                me->AttackStop();
                                DoCast(me, SPELL_GATHER_STRENGHT);
                            }
                        }
                    }
                }*/
            }

            void JustSummoned(Creature *summon)
            {
                summons.Summon(summon);
                if (me->isInCombat())
                    DoZoneInCombat(summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void JustDied(Unit* killer)
            {
                events.Reset();
                summons.DespawnAll();

                pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, DONE);
            }
            
            void KilledUnit(Unit* victim)
            {

            }

            uint32 GetData(uint32 type)
            {
                if (type == DATA_DEAD)
                    return (m_isDead ? 1 : 0);

                return 0;
            }
            
            void UpdateAI(const uint32 diff)
            {                    
                if (!UpdateVictim() || !pInstance)
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                
                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_UPDATE_VICTIM:
                            if (Unit* victim = me->getVictim())
                            {
                                if (me->GetHomePosition().GetExactDist2d(victim) > 60.0f)
                                {
                                    Unit* pTarget = NULL;
                                    pTarget = SelectTarget(SELECT_TARGET_NEAREST, 0, 55.0f, true);
                                    if (pTarget && me->GetHomePosition().GetExactDist2d(pTarget) <= 60.0f)
                                    {
                                        AttackStart(pTarget);
                                    }
                                    else if (!hasNoTarget)
                                    {
                                        hasNoTarget = true;
                                        Talk(SAY_NEZIR_ENRAGE);
                                        //DoCast(me, SPELL_CHILLING_WINDS, true);
                                    }
                                }
                            }

                            events.ScheduleEvent(EVENT_UPDATE_VICTIM, 10000);
                            break;
                        case EVENT_WIND_CHILL:
                            DoCastAOE(SPELL_WIND_CHILL);
                            events.ScheduleEvent(EVENT_WIND_CHILL, 10500);
                            break;
                        case EVENT_PERMAFROST:
                            DoCastVictim(SPELL_PERMAFROST);
                            events.ScheduleEvent(EVENT_PERMAFROST, 15000);
                            break;
                        case EVENT_SLEET_STORM_1:
                            DoCast(me, SPELL_SLEET_STORM);
                            events.ScheduleEvent(EVENT_SLEET_STORM_2, 16000);
                            break;
                        case EVENT_SLEET_STORM_2:
                            me->SetReactState(REACT_AGGRESSIVE);

                            events.ScheduleEvent(EVENT_WIND_CHILL, urand(7000, 10500));
                            events.ScheduleEvent(EVENT_PERMAFROST, 3000);
                            events.ScheduleEvent(EVENT_UPDATE_VICTIM, 10000);
                            break;
                        default:
                            break;
                    }
                }        

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* pInstance;
            EventMap events;
            SummonList summons;
            bool m_isDead;
            bool hasNoTarget;

            void DespawnCreatures(uint32 entry)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 1000.0f);

                if (creatures.empty())
                   return;

                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                     (*iter)->DespawnOrUnsummon(2000);
            }
        };
};

class boss_rohash : public CreatureScript
{
    public:
        boss_rohash() : CreatureScript("boss_rohash") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_rohashAI (creature);
        }

        struct boss_rohashAI : public ScriptedAI
        {
            boss_rohashAI(Creature* pCreature) : ScriptedAI(pCreature), summons(me)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);

                me->setActive(true);
                me->SetFloatValue(UNIT_FIELD_COMBATREACH, 5.0f);

                pInstance = pCreature->GetInstanceScript();

                m_isDead = false;
                hasNoTarget = false;
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();
                
                me->RemoveAura(SPELL_DEAD); // Dead Look

                DoCast(me, SPELL_NO_REGEN, true);
                
                me->SetMaxPower(POWER_MANA, 90);
                me->SetPower(POWER_MANA, 0);

                me->SetReactState(REACT_AGGRESSIVE);

                m_isDead = false;
                hasNoTarget = false;

                if (pInstance)
                    pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, NOT_STARTED);
            }

            void EnterCombat(Unit* who)
            {
                me->RemoveAura(SPELL_PREFIGHT_VISUAL_EAST);

                me->SetMaxPower(POWER_MANA, 90);
                me->SetPower(POWER_MANA, 0);

                m_isDead = false;
                hasNoTarget = false;

                events.ScheduleEvent(EVENT_SLICING_GALE, 2000);
                events.ScheduleEvent(EVENT_HURRICANE, 10000);
                events.ScheduleEvent(EVENT_TORNADO, 7000);
                events.ScheduleEvent(EVENT_UPDATE_VICTIM, 10000);

                Talk(SAY_ROHASH_AGGRO);

                if (pInstance)
                {
                    if (Creature* pAnshal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ANSHAL)))
                        if (!pAnshal->isInCombat())
                            DoZoneInCombat(pAnshal, 600.0f);

                    if (Creature* pNezir = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_NEZIR)))
                        if (!pNezir->isInCombat())
                            DoZoneInCombat(pNezir, 600.0f);

                    pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, IN_PROGRESS);
                }
            }

            void AttackStart(Unit* who)
            {
                if (!who)
                    return;

                if (me->Attack(who, false))
                    DoStartNoMovement(who);
            }

            void DoAction(const int32 action)
            {
                if (action == DATA_ULTIMATE)
                {
                    events.CancelEvent(EVENT_SLICING_GALE);
                    events.CancelEvent(EVENT_UPDATE_VICTIM);

                    me->NearTeleportTo(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), me->GetHomePosition().GetOrientation());
                    me->UpdateObjectVisibility(true);

                    Talk(SAY_CONCLAVE_ULTIMATE);

                    events.ScheduleEvent(EVENT_HURRICANE_1, 1000);
                }
            }

            void DamageTaken(Unit* dealer, uint32 &damage)
            {
                if (dealer->GetGUID() == me->GetGUID())
                    return;
                
                /*if (me->GetHealth() < damage)
                {
                    damage = me->GetHealth() - 1;

                    if (!m_isDead)
                    {
                        m_isDead = true;
                        if (pInstance)
                        {
                            Creature* pAnshal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ANSHAL));
                            Creature* pNezir = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ROHASH));
                            if (pAnshal->AI()->GetData(DATA_DEAD) > 0 && pNezir->AI()->GetData(DATA_DEAD) > 0)
                            {
                                pAnshal->Kill(pAnshal);
                                pNezir->Kill(pNezir);
                                me->Kill(me);
                            }
                            else
                            {
                                events.Reset();
                                me->SetReactState(REACT_PASSIVE);
                                me->AttackStop();
                                DoCast(me, SPELL_GATHER_STRENGHT);
                            }
                        }
                    }
                }*/
            }

            void JustSummoned(Creature *summon)
            {
                summons.Summon(summon);
                if (me->isInCombat())
                    DoZoneInCombat(summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void JustDied(Unit* killer)
            {
                events.Reset();
                summons.DespawnAll();

                pInstance->SetBossState(DATA_CONCLAVE_OF_WIND, DONE);
            }
            
            void KilledUnit(Unit* victim)
            {

            }

            uint32 GetData(uint32 type)
            {
                if (type == DATA_DEAD)
                    return (m_isDead ? 1 : 0);

                return 0;
            }
            
            void UpdateAI(const uint32 diff)
            {                    
                if (!UpdateVictim() || !pInstance)
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                
                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_UPDATE_VICTIM:
                            if (Unit* victim = me->getVictim())
                            {
                                if (me->GetHomePosition().GetExactDist2d(victim) > 60.0f)
                                {
                                    Unit* pTarget = NULL;
                                    pTarget = SelectTarget(SELECT_TARGET_NEAREST, 0, 55.0f, true);
                                    if (pTarget && me->GetHomePosition().GetExactDist2d(pTarget) <= 60.0f)
                                    {
                                        AttackStart(pTarget);
                                    }
                                    else if (!hasNoTarget)
                                    {
                                        hasNoTarget = true;
                                        Talk(SAY_ROHASH_ENRAGE);
                                        //DoCast(me, SPELL_DEAFENING_WINDS, true);
                                    }
                                }
                            }

                            events.ScheduleEvent(EVENT_UPDATE_VICTIM, 10000);
                            break;
                        case EVENT_SLICING_GALE:
                            DoCastVictim(SPELL_SLICING_GALE);
                            events.ScheduleEvent(EVENT_SLICING_GALE, 2500);
                            break;
                        case EVENT_HURRICANE_1:
                            events.ScheduleEvent(EVENT_HURRICANE_2, 16000);

                            me->SummonCreature(NPC_HURRICANE, me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 14000);

                            DoCast(me, SPELL_HURRICANE_ULTIMATE);
                            break;
                        case EVENT_HURRICANE_2:
                            me->SetReactState(REACT_AGGRESSIVE);

                            events.ScheduleEvent(EVENT_SLICING_GALE, 3000);
                            events.ScheduleEvent(EVENT_UPDATE_VICTIM, 10000);
                            break;
                        case EVENT_TORNADO:
                            DoCast(me, SPELL_TORNADO_SUMMON);
                            break;
                        default:
                            break;
                    }
                }        
            }

        private:
            InstanceScript* pInstance;
            EventMap events;
            SummonList summons;
            bool m_isDead;
            bool hasNoTarget;

            void DespawnCreatures(uint32 entry)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 1000.0f);

                if (creatures.empty())
                   return;

                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                     (*iter)->DespawnOrUnsummon(2000);
            }
        };
};

class npc_anshal_ravenous_trigger : public CreatureScript
{
    public:
        npc_anshal_ravenous_trigger() : CreatureScript("npc_anshal_ravenous_trigger") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_anshal_ravenous_triggerAI(pCreature);
        }

        struct npc_anshal_ravenous_triggerAI : public Scripted_NoMovementAI
        {
            npc_anshal_ravenous_triggerAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_PASSIVE);
            }

            void IsSummonedBy(Unit* /*owner*/)
            {
                
                DoCast(me, SPELL_NURTURE_SUMMON, true);
                DoCast(me, SPELL_NURTURE_DUMMY, true);
            }
        };
};

class npc_anshal_ravenous_creeper : public CreatureScript
{
    public:
        npc_anshal_ravenous_creeper() : CreatureScript("npc_anshal_ravenous_creeper") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_anshal_ravenous_creeperAI(pCreature);
        }

        struct npc_anshal_ravenous_creeperAI : public ScriptedAI
        {
            npc_anshal_ravenous_creeperAI(Creature* pCreature) : ScriptedAI(pCreature)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            void IsSummonedBy(Unit* /*owner*/)
            {
                events.ScheduleEvent(EVENT_START_MOVE, 2000);
            }

            void UpdateAI(const uint32 diff)
            {                    
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                
                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_START_MOVE:
                            me->SetReactState(REACT_AGGRESSIVE);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
        };
};

class npc_rohash_hurricane : public CreatureScript
{
    public:
        npc_rohash_hurricane() : CreatureScript("npc_rohash_hurricane") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_rohash_hurricaneAI(pCreature);
        }

        struct npc_rohash_hurricaneAI : public Scripted_NoMovementAI
        {
            npc_rohash_hurricaneAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_PASSIVE);
            }
        };
};

class npc_rohash_tornado : public CreatureScript
{
    public:
        npc_rohash_tornado() : CreatureScript("npc_rohash_tornado") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_rohash_tornadoAI(pCreature);
        }

        struct npc_rohash_tornadoAI : public Scripted_NoMovementAI
        {
            npc_rohash_tornadoAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_PASSIVE);

                moveTimer = 1000;
                isMoving = false;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!isMoving)
                    if (moveTimer <= diff)
                    {
                        me->SetSpeed(MOVE_RUN, 4.0f, true);
                        Movement::MoveSplineInit init(*me);
                        FillCirclePath(rohashPos, frand(15.f, 20.f), me->GetPositionZ(), init.Path(), true);
                        init.SetCyclic();
                        init.SetWalk(false);
                        init.SetVelocity(4.0f);
                        init.Launch();
                    }
                    else
                        moveTimer -= diff;

            }

        private:

            uint32 moveTimer;
            bool isMoving;

            void FillCirclePath(Position const& centerPos, float radius, float z, Movement::PointsArray& path, bool clockwise)
            {
                float step = clockwise ? -M_PI / 8.0f : M_PI / 8.0f;
                float angle = centerPos.GetAngle(me->GetPositionX(), me->GetPositionY());

                for (uint8 i = 0; i < 16; angle += step, ++i)
                {
                    G3D::Vector3 point;
                    point.x = centerPos.GetPositionX() + radius * cosf(angle);
                    point.y = centerPos.GetPositionY() + radius * sinf(angle);
                    z = me->GetMap()->GetHeight(point.x, point.y, z);
                    point.z = z;
                    path.push_back(point);
                }
            }
        };
};

class spell_anshal_nurture_summon_trigger : public SpellScriptLoader
{
    public:
        spell_anshal_nurture_summon_trigger() : SpellScriptLoader("spell_anshal_nurture_summon_trigger") { }

        class spell_anshal_nurture_summon_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_anshal_nurture_summon_trigger_SpellScript);

            void ChangeSummonPos(SpellEffIndex effIndex)
            {
                if (!GetCaster() || !GetCaster()->ToCreature())
                    return;

                PreventHitDefaultEffect(effIndex);
                PreventHitEffect(effIndex);

                Position pos = GetCaster()->ToCreature()->GetHomePosition();
                pos.Relocate(pos.GetPositionX() + frand(-5.0f, 5.0f), pos.GetPositionY() + frand(-5.0f, 5.0f), pos.GetPositionZ() + 1.0f);
                GetCaster()->SummonCreature(NPC_RAVENOUS_CREEPER, pos);

                /*WorldLocation loc;
                loc.Relocate(pos);
                SetExplTargetDest(loc);
                GetHitDest()->Relocate(pos);*/
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_anshal_nurture_summon_trigger_SpellScript::ChangeSummonPos, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_anshal_nurture_summon_trigger_SpellScript();
        }
};

class spell_anshal_withering_winds : public SpellScriptLoader
{
    public:
        spell_anshal_withering_winds() : SpellScriptLoader("spell_anshal_withering_winds") { }

        class spell_anshal_withering_winds_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_anshal_withering_winds_AuraScript);

            void HandlePeriodic(constAuraEffectPtr aurEff)
            {
                if (!GetTarget())
                    return;

                int32 bp0 = aurEff->GetAmount() * aurEff->GetTickNumber();
                GetTarget()->CastCustomSpell(GetTarget(), SPELL_WITHERING_WINDS_DAMAGE, &bp0, NULL, NULL, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_anshal_withering_winds_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY); 
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_anshal_withering_winds_AuraScript();
        }
};

class spell_nezir_chilling_winds : public SpellScriptLoader
{
    public:
        spell_nezir_chilling_winds() : SpellScriptLoader("spell_nezir_chilling_winds") { }

        class spell_nezir_chilling_winds_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_nezir_chilling_winds_AuraScript);

            void HandlePeriodic(constAuraEffectPtr aurEff)
            {
                if (!GetTarget())
                    return;

                int32 bp0 = aurEff->GetAmount() * aurEff->GetTickNumber();
                GetTarget()->CastCustomSpell(GetTarget(), SPELL_CHILLING_WINDS_DAMAGE, &bp0, NULL, NULL, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_nezir_chilling_winds_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY); 
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_nezir_chilling_winds_AuraScript();
        }
};

class spell_rohash_deafening_winds : public SpellScriptLoader
{
    public:
        spell_rohash_deafening_winds() : SpellScriptLoader("spell_rohash_deafening_winds") { }

        class spell_rohash_deafening_winds_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rohash_deafening_winds_AuraScript);

            void HandlePeriodic(constAuraEffectPtr aurEff)
            {
                if (!GetTarget())
                    return;

                int32 bp0 = aurEff->GetAmount() * aurEff->GetTickNumber();
                GetTarget()->CastCustomSpell(GetTarget(), SPELL_DEAFENING_WINDS_DAMAGE, &bp0, NULL, NULL, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rohash_deafening_winds_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY); 
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_rohash_deafening_winds_AuraScript();
        }
};

class spell_rohash_hurricane_dummy : public SpellScriptLoader
{
    public:
        spell_rohash_hurricane_dummy() : SpellScriptLoader("spell_rohash_hurricane_dummy") { }

        class spell_rohash_hurricane_dummy_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rohash_hurricane_dummy_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster() || !GetHitUnit())
                    return;

                //GetHitUnit()->CastSpell(GetHitUnit(), SPELL_HURRICANE_VEHICLE, true);

            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rohash_hurricane_dummy_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rohash_hurricane_dummy_SpellScript();
        }
};

void AddSC_boss_conclave_of_wind()
{
    new boss_anshal();
    new boss_nezir();
    new boss_rohash();
    new npc_anshal_ravenous_trigger();
    new npc_anshal_ravenous_creeper();
    new npc_rohash_hurricane();
    new npc_rohash_tornado();
    new spell_anshal_nurture_summon_trigger();
    new spell_anshal_withering_winds();
    new spell_nezir_chilling_winds();
    new spell_rohash_deafening_winds();
    new spell_rohash_hurricane_dummy();
}
