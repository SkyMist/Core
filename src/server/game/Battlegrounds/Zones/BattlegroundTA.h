/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/
 
#ifndef __BATTLEGROUNDTA_H
#define __BATTLEGROUNDTA_H

#include "Battleground.h"

enum BattlegroundTAObjectTypes
{
    BG_TA_OBJECT_DOOR_1         = 0,
    BG_TA_OBJECT_DOOR_2         = 1,

    BG_TA_OBJECT_MAX            = 2
};

enum BattlegroundTAObjects
{
    BG_TA_OBJECT_TYPE_DOOR_1    = 185918,
    BG_TA_OBJECT_TYPE_DOOR_2    = 185917
};

class BattlegroundTA : public Battleground
{
    public:
        BattlegroundTA();
        ~BattlegroundTA();

        /* inherited from BattlegroundClass */
        void AddPlayer(Player* player);
        void Reset();
        void FillInitialWorldStates(ByteBuffer &d);
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();

        void RemovePlayer(Player* player, uint64 guid, uint32 team);
        void HandleAreaTrigger(Player* Source, uint32 Trigger);
        bool SetupBattleground();
        void HandleKillPlayer(Player* player, Player* killer);
};
#endif
