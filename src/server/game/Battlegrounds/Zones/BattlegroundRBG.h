#ifndef __BATTLEGROUNDRBG_H
#define __BATTLEGROUNDRBG_H

class Battleground;

class BattlegroundRBGScore : public BattlegroundScore
{
    public:
        BattlegroundRBGScore() { };
        virtual ~BattlegroundRBGScore() { };
};

class BattlegroundRBG : public Battleground
{
    public:
        BattlegroundRBG();
        ~BattlegroundRBG();

        virtual void AddPlayer(Player* player);
        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();

        virtual void RemovePlayer(Player* player, uint64 guid, uint32 team);
        virtual void HandleAreaTrigger(Player* Source, uint32 Trigger);

        /* Scorekeeping */
        virtual void UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor = true);
};

#endif
