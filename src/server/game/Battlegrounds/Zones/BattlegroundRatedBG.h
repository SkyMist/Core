/*
 * Copyright (C) 2013 SkyMist Project.
 *
 * This program is NOT free software; you can NOT redistribute it and/or modify it.
 */

#ifndef __BATTLEGROUNDRATEDBG_H
#define __BATTLEGROUNDRATEDBG_H

#include "Battleground.h"

class BattlegroundRatedBGScore : public BattlegroundScore
{
    public:
        BattlegroundRatedBGScore() {};
        virtual ~BattlegroundRatedBGScore() {};
};

class BattlegroundRatedBG : public Battleground
{
    public:
        BattlegroundRatedBG();
        ~BattlegroundRatedBG();

        void AddPlayer(Player* player);
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();

        void RemovePlayer(Player* player, uint64 guid, uint32 team);
        void HandleAreaTrigger(Player* Source, uint32 Trigger);

        /* Scorekeeping */
        void UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor = true);

    private:
};

#endif
