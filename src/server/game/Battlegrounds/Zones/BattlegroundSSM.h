#ifndef __BATTLEGROUNDSSM_H
#define __BATTLEGROUNDSSM_H

#include "Battleground.h"

enum BG_SSM_ObjectTypes
{
    BG_SSM_OBJECT_DOOR_H_1                 = 0,
    BG_SSM_OBJECT_DOOR_H_2                 = 1,
    BG_SSM_OBJECT_MINE_CART_END_POINT_1    = 2,
    BG_SSM_OBJECT_MINE_CART_END_POINT_2    = 3,
    BG_SSM_OBJECT_MINE_CART_END_POINT_3    = 4,
    BG_SSM_OBJECT_MINE_CART_END_POINT_4    = 5,
    BG_SSM_OBJECT_BUFF_1                   = 6,
    BG_SSM_OBJECT_BUFF_2                   = 7,
    BG_SSM_OBJECT_BUFF_3                   = 8,
    BG_SSM_OBJECT_BUFF_4                   = 9,
    BG_SSM_OBJECT_MAX                      = 10
};

enum BG_SSM_CreatureTypes
{
    BG_CREATURE_MINE_CART_1                = 0,
    BG_CREATURE_MINE_CART_2                = 1,
    BG_CREATURE_MINE_CART_3                = 2,
    BG_CREATURE_TRACK_SWITCH_1             = 3,
    BG_CREATURE_TRACK_SWITCH_2             = 4,
    BG_CREATURES_MAX_SSM                   = 5
};

enum BG_SSM_ObjectEntry
{
    BG_OBJECT_DOOR_H_1_SSM_ENTRY          = 212942,
    BG_OBJECT_DOOR_H_2_SSM_ENTRY          = 212941,
    BG_OBJECT_MINE_CART_EP_1_SSM_ENTRY    = 212081,
    BG_OBJECT_MINE_CART_EP_2_SSM_ENTRY    = 212080,
    BG_OBJECT_MINE_CART_EP_3_SSM_ENTRY    = 212083,
    BG_OBJECT_MINE_CART_EP_4_SSM_ENTRY    = 212082,
    BG_OBJECT_BUFF_1_SSM_ENTRY            = 179906,
    BG_OBJECT_BUFF_2_SSM_ENTRY            = 179904,
    BG_OBJECT_BUFF_3_SSM_ENTRY            = 179907,
    BG_OBJECT_BUFF_4_SSM_ENTRY            = 179907,
    BG_CREATURE_MINE_CART_ENTRY           = 60140,
    BG_CREATURE_TRACK_SWITCH_ENTRY        = 60283,
};

enum BG_SSM_Graveyard
{
    BG_SSM_GRAVEYARD_WATERFALL            = 4072,
    BG_SSM_GRAVEYARD_LAVA                 = 4073,
    BG_SSM_GRAVEYARD_DIAMOND              = 4074,
    BG_SSM_GRAVEYARD_TROLL                = 4075
};

enum BG_SSM_Defines
{
    BG_SSM_POINTS_FOR_WIN                 = 1600,
    BG_SSM_POINTS_FROM_MINE_CART          = 200,
};

class BattlegroundSSM : public Battleground
{
    public:
        BattlegroundSSM();
        ~BattlegroundSSM();

        bool SetupBattleground();
        virtual void Reset();
        
        void StartingEventCloseDoors();
        void StartingEventOpenDoors();
        WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);
        void PostUpdateImpl(uint32 diff);
        void RewardMineCart(uint8 team);

    private:
        uint32 m_Points[2];
};

#endif
