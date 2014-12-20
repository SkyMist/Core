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

        /* Inherited from Battleground class. */

        /* Players. */
        void AddPlayer(Player* player);
        void RemovePlayer(Player* player, uint64 guid, uint32 team);

        /* Doors. */
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();

        /* Areatriggers. */
        void HandleAreaTrigger(Player* Source, uint32 Trigger);

        /* Scorekeeping. */
        void UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor = true);
};

#endif
