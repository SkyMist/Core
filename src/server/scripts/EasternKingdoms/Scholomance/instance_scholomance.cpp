#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "scholomance.h"

// placeholder for chillheart: on death (113685) | boss back to box (111256), icechill dmg (105265) | channel before combat 122499

class instance_scholomance : public InstanceMapScript
{
public:
    instance_scholomance() : InstanceMapScript("instance_scholomance", 1007) { }

    struct instance_scholomance_InstanceMapScript: public InstanceScript
    {
        instance_scholomance_InstanceMapScript(InstanceMap* map): InstanceScript(map)  { }

        void Initialize()
        {
            SetBossNumber(MAX_ENCOUNTER);

            JandiceBarovGUID    = 0;
            RattlegoreGUID      = 0;
            LillianVossGUID     = 0;
            LillianVossSoulGUID = 0;
            GandlingGUID        = 0;
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case BOSS_JANDICE_BAROV:
                    JandiceBarovGUID = creature->GetGUID();
                    break;
                case BOSS_RATTLEGORE:
                    RattlegoreGUID = creature->GetGUID();
                    break;
                case BOSS_LILLIAN_VOSS:
                    LillianVossGUID = creature->GetGUID();
                    break;
                case BOSS_LILLIAN_VOSS_SOUL:
                    LillianVossSoulGUID = creature->GetGUID();
                    break;
                case BOSS_DARKMASTER_GANDLING:
                    GandlingGUID = creature->GetGUID();
                    break;
            }
        }

        uint64 GetData64(uint32 type) const
        {
            switch (type)
            {
                case DATA_JANDICE_BAROV:
                    return JandiceBarovGUID;
                case DATA_RATTLEGORE:
                    return RattlegoreGUID;
                case DATA_LILLIAN_VOSS:
                    return LillianVossGUID;
                case DATA_LILLIAN_VOSS_SOUL:
                    return LillianVossSoulGUID;
                case DATA_GANDLING:
                    return GandlingGUID;
            }
            return 0;
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch(type)
            {
                case DATA_JANDICE_BAROV:
                    break;
                case DATA_RATTLEGORE:
                    break;
                case DATA_LILLIAN_VOSS:
                case DATA_LILLIAN_VOSS_SOUL:
                    break;
                case DATA_GANDLING:
                    break;
                default:
                    break;
            }

            return true;
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << "S O " << GetBossSaveData();

            OUT_SAVE_INST_DATA_COMPLETE;
            return saveStream.str();
        }

        void Load(const char* str)
        {
            if (!str)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(str);

            char dataHead1, dataHead2;

            std::istringstream loadStream(str);
            loadStream >> dataHead1 >> dataHead2;

            if (dataHead1 == 'S' && dataHead2 == 'O')
            {
                for (uint32 i = 0; i < MAX_ENCOUNTER; ++i)
                {
                    uint32 tmpState;
                    loadStream >> tmpState;
                    if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                        tmpState = NOT_STARTED;
                    SetBossState(i, EncounterState(tmpState));
                }

            }
            else
                OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }
        protected:

            uint64 JandiceBarovGUID;
            uint64 RattlegoreGUID;
            uint64 LillianVossGUID;
            uint64 LillianVossSoulGUID;
            uint64 GandlingGUID;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_scholomance_InstanceMapScript(map);
    }
};

void AddSC_instance_scholomance()
{
    new instance_scholomance();
}
