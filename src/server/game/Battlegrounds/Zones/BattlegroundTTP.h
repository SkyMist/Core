/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/
 
#ifndef __BATTLEGROUNDTTP_H
#define __BATTLEGROUNDTTP_H

#include "Battleground.h"

/*
enum BattlegroundTTPObjectTypes
{
    BG_TTP_OBJECT_BUFF_1         = 0,
    BG_TTP_OBJECT_BUFF_2         = 1,
    BG_TTP_OBJECT_DOOR_1         = 2,
    BG_TTP_OBJECT_DOOR_2         = 3,

    BG_TTP_OBJECT_MAX            = 4
};

enum BattlegroundTTPObjects
{
    BG_TTP_OBJECT_TYPE_DOOR_1    = 185918,
    BG_TTP_OBJECT_TYPE_DOOR_2    = 185917,
    BG_TTP_OBJECT_TYPE_BUFF_1    = 184663,
    BG_TTP_OBJECT_TYPE_BUFF_2    = 184664
};
*/

class BattlegroundTTP : public Battleground
{
    public:
        BattlegroundTTP();
        ~BattlegroundTTP();

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
