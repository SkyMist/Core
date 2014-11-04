#include "ScriptPCH.h"
#include "Battleground.h"
#include "BattlegroundSSM.h"

struct MovePoint
{
    uint32 id;
    float X, Y, Z;
};

MovePoint MineCartMovePoints1[25] = 
{
    { 0, 736.11f, 207.42f, 319.64f },
    { 1, 732.28f, 213.91f, 320.12f },
    { 2, 730.64f, 218.38f, 320.46f },
    { 3, 729.07f, 225.70f, 320.81f },
    { 4, 727.04f, 233.24f, 320.89f },
    { 5, 724.48f, 238.89f, 320.74f },
    { 6, 715.64f, 251.31f, 321.04f },
    { 7, 710.80f, 261.45f, 320.71f },
    { 8, 707.73f, 269.23f, 320.54f },
    { 9, 703.13f, 279.00f, 320.45f },
    {10, 696.00f, 290.83f, 320.58f },
    {11, 686.36f, 301.63f, 320.96f },
    {12, 673.09f, 312.74f, 322.01f },
    {13, 661.14f, 323.33f, 324.43f },
    {14, 650.37f, 331.95f, 327.56f },
    {15, 636.24f, 344.01f, 333.34f },
    {16, 630.91f, 345.65f, 335.58f },
    {17, 626.59f, 345.75f, 337.20f },
    {18, 610.42f, 341.75f, 342.32f },
    {19, 601.04f, 339.81f, 344.60f },
    {20, 592.96f, 339.32f, 345.89f },
    {21, 583.68f, 337.66f, 346.23f },
    {22, 574.50f, 336.92f, 346.49f },
    {23, 568.00f, 337.10f, 346.68f },
    {24, 564.80f, 337.08f, 347.14f } // BG_OBJECT_MINE_CART_EP_2_SSM_ENTRY
};

MovePoint MineCartMovePoints2[12] = 
{
    { 0, 765.10f, 202.04f, 319.74f },
    { 1, 770.91f, 210.38f, 321.36f },
    { 2, 776.79f, 222.71f, 323.25f },
    { 3, 781.01f, 235.79f, 326.21f },
    { 4, 785.73f, 245.87f, 329.02f },
    { 5, 790.19f, 253.86f, 332.96f },
    { 6, 795.75f, 261.00f, 337.37f },
    { 7, 803.42f, 269.66f, 341.15f },
    { 8, 813.19f, 279.42f, 344.04f },
    { 9, 822.44f, 288.50f, 345.65f },
    {10, 832.77f, 297.38f, 346.75f },
    {11, 837.98f, 303.08f, 346.86f }, // Точка разветвления
};

MovePoint MineCartMovePoints2_L[21] = 
{
    {12, 843.59f, 312.01f, 346.98f },
    {13, 847.61f, 320.71f, 347.12f },
    {14, 848.76f, 329.14f, 346.98f },
    {15, 845.31f, 346.37f, 347.10f },
    {16, 836.58f, 364.34f, 346.88f },
    {17, 833.09f, 369.80f, 346.84f },
    {18, 827.47f, 383.89f, 346.60f },
    {19, 821.04f, 401.75f, 346.54f },
    {20, 817.53f, 410.13f, 348.09f },
    {21, 816.77f, 417.41f, 349.57f },
    {22, 816.48f, 428.03f, 352.64f },
    {23, 813.95f, 434.97f, 354.71f },
    {24, 810.74f, 442.70f, 356.72f },
    {25, 808.21f, 453.84f, 357.94f },
    {26, 806.33f, 466.17f, 358.74f },
    {27, 803.36f, 473.31f, 359.12f },
    {28, 798.40f, 480.04f, 359.17f },
    {29, 793.79f, 485.49f, 359.31f },
    {30, 786.63f, 492.07f, 359.29f },
    {31, 779.68f, 499.65f, 359.33f },
    {32, 777.73f, 502.31f, 359.45f } // BG_OBJECT_MINE_CART_EP_3_SSM_ENTRY 
};

/*MovePoint MineCartMovePoints1_R[0] = 
{
};*/

MovePoint MineCartMovePoints3[8] = 
{
    { 0, 739.80f, 171.65f, 319.36f },
    { 1, 734.23f, 161.08f, 319.01f },
    { 2, 727.76f, 149.89f, 319.75f },
    { 3, 725.82f, 146.03f, 319.70f },
    { 4, 721.71f, 137.06f, 319.58f },
    { 5, 718.65f, 126.86f, 319.61f },
    { 6, 717.54f, 121.52f, 320.22f },
    { 7, 716.98f, 113.81f, 320.75f },
};

MovePoint MineCartMovePoints3_R[14] = 
{
    { 8, 715.26f, 108.20f, 320.37f },
    { 9, 713.08f, 104.02f, 319.99f },
    {10, 709.19f, 100.63f, 319.20f },
    {11, 702.68f,  97.74f, 317.74f },
    {12, 694.57f,  96.27f, 315.91f },
    {13, 683.12f,  95.04f, 313.47f },
    {14, 674.62f,  92.18f, 310.52f },
    {15, 670.67f,  90.20f, 309.05f },
    {16, 662.19f,  85.00f, 305.79f },
    {17, 653.73f,  82.42f, 303.43f },
    {18, 645.54f,  81.56f, 300.96f },
    {19, 639.35f,  81.36f, 298.98f },
    {20, 632.26f,  80.62f, 298.61f },
    {21, 615.50f,  79.41f, 298.28f } // BG_OBJECT_MINE_CART_EP_1_SSM_ENTRY
};

MovePoint MineCartMovePoints3_L[32] = 
{
    { 8, 716.94f, 108.34f, 320.55f },
    { 9, 717.88f, 104.36f, 320.65f },
    {10, 719.91f, 101.24f, 320.87f },
    {11, 723.26f,  98.51f, 321.47f },
    {12, 729.41f,  95.75f, 322.58f },
    {13, 737.59f,  93.68f, 325.29f },
    {14, 751.28f,  91.34f, 329.88f },
    {15, 757.37f,  89.70f, 331.96f },
    {16, 767.40f,  85.78f, 335.89f },
    {17, 773.97f,  82.17f, 338.77f },
    {18, 779.23f,  80.52f, 340.96f },
    {19, 791.22f,  78.69f, 344.95f },
    {20, 799.53f,  76.49f, 348.26f },
    {21, 803.22f,  74.74f, 349.58f },
    {22, 809.72f,  70.38f, 351.41f },
    {23, 815.24f,  66.36f, 353.20f },
    {24, 820.96f,  63.41f, 354.73f },
    {25, 824.09f,  62.78f, 355.54f },
    {26, 828.78f,  62.42f, 356.71f },
    {27, 833.49f,  63.98f, 357.74f },
    {28, 838.71f,  68.12f, 358.83f },
    {29, 843.19f,  70.43f, 359.81f },
    {30, 847.36f,  71.51f, 360.73f },
    {31, 852.01f,  71.37f, 361.49f },
    {32, 856.20f,  70.13f, 362.16f },
    {33, 864.15f,  66.11f, 363.33f },
    {34, 872.41f,  58.86f, 364.36f },
    {35, 879.77f,  51.07f, 364.24f },
    {36, 885.33f,  43.34f, 363.70f },
    {37, 889.53f,  37.57f, 363.59f },
    {38, 893.14f,  33.49f, 363.84f },
    {39, 896.79f,  25.29f, 364.14f }, // BG_OBJECT_MINE_CART_EP_4_SSM_ENTRY
};

enum eDefines
{
    SPELL_ALLIANCE_MINE_CART        = 140876,
    SPELL_HORDE_MINE_CART           = 141210,
    
    WAYPOINTS_PATH_MINE_CART_1      = 100500,
    WAYPOINTS_PATH_MINE_CART_2      = 100501,
    WAYPOINTS_PATH_MINE_CART_2_L    = 100502,
    WAYPOINTS_PATH_MINE_CART_3      = 100503,
    WAYPOINTS_PATH_MINE_CART_3_R    = 100504,
    WAYPOINTS_PATH_MINE_CART_3_L    = 100505,
};

class npc_mine_cart : public CreatureScript
{
public:
    npc_mine_cart() : CreatureScript("npc_mine_cart") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mine_cartAI(creature);
    }

    struct npc_mine_cartAI : public ScriptedAI
    {
        npc_mine_cartAI(Creature* creature) : ScriptedAI(creature) {}
        
        uint8 mine_cart_id;
        uint32 updateTimer;
        bool second_path;

        void Reset()
        {
            second_path = false;

            if (Battleground* bg = me->GetBattleground())
            {
                if (bg->GetTypeID() == BATTLEGROUND_SSM)
                {
                    for (uint8 i = BG_CREATURE_MINE_CART_1; i < BG_CREATURE_TRACK_SWITCH_1; i++)
                    {
                        if (bg->BgCreatures[i] == me->GetGUID())
                        {
                            mine_cart_id = i;
                            break;
                        }
                    }
                }
            }

            switch (mine_cart_id)
            {
                case BG_CREATURE_MINE_CART_1:
                    me->GetMotionMaster()->MovePath(WAYPOINTS_PATH_MINE_CART_1, false);
                    //me->GetMotionMaster()->MovePoint(MineCartMovePoints1[0].id, MineCartMovePoints1[0].X, MineCartMovePoints1[0].Y, MineCartMovePoints1[0].Z);
                    break;
                case BG_CREATURE_MINE_CART_2:
                    me->GetMotionMaster()->MovePath(WAYPOINTS_PATH_MINE_CART_2, false);
                    //me->GetMotionMaster()->MovePoint(MineCartMovePoints2[0].id, MineCartMovePoints2[0].X, MineCartMovePoints2[0].Y, MineCartMovePoints2[0].Z);
                    break;
                case BG_CREATURE_MINE_CART_3:
                    me->GetMotionMaster()->MovePath(WAYPOINTS_PATH_MINE_CART_3, false);
                    //me->GetMotionMaster()->MovePoint(MineCartMovePoints3[0].id, MineCartMovePoints3[0].X, MineCartMovePoints3[0].Y, MineCartMovePoints3[0].Z);
                    break;
            }

            updateTimer = 1000;
        }
        
        void RewardMineCart()
        {
            if (BattlegroundSSM* bg = (BattlegroundSSM*)me->GetBattleground())
            {
                // ToDo: Find Auras for control
                if (me->HasAura(SPELL_ALLIANCE_MINE_CART)) // Alliance control aura
                    bg->RewardMineCart(BG_TEAM_ALLIANCE);
                else if (me->HasAura(SPELL_HORDE_MINE_CART)) // Horde control aura
                    bg->RewardMineCart(BG_TEAM_ALLIANCE);
            }
                
            me->DisappearAndDie();
        }

        void MovementInform(uint32 Type, uint32 PointId)
        {
            if (Type != WAYPOINT_MOTION_TYPE)
                return;

            switch (mine_cart_id)
            {
                case BG_CREATURE_MINE_CART_1:
                    if (PointId == 24)
                        RewardMineCart();
                    break;
                case BG_CREATURE_MINE_CART_2:
                    if (!second_path && PointId == 11)
                    {
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MovePath(WAYPOINTS_PATH_MINE_CART_2_L, false);
                        second_path = true;
                    }
                    else if (second_path && PointId == 20)
                    {
                        RewardMineCart();
                    }
                    break;
                case BG_CREATURE_MINE_CART_3:
                    if (!second_path && PointId == 7)
                    {
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MovePath(WAYPOINTS_PATH_MINE_CART_3_L, false);
                        second_path = true;
                    }
                    else if (second_path && PointId == 31)
                    {
                        RewardMineCart();
                    }
                    break;
            }
                
                
            /*if (Type != POINT_MOTION_TYPE)
                return;

            switch (mine_cart_id)
            {
                case BG_CREATURE_MINE_CART_1:
                    if (PointId < 25)
                        me->GetMotionMaster()->MovePoint(MineCartMovePoints1[PointId + 1].id, MineCartMovePoints1[PointId + 1].X, MineCartMovePoints1[PointId + 1].Y, MineCartMovePoints1[PointId + 1].Z);
                    else
                        RewardMineCart();
                    break;
                case BG_CREATURE_MINE_CART_2:
                    if (PointId < 11)
                        me->GetMotionMaster()->MovePoint(MineCartMovePoints2[PointId + 1].id, MineCartMovePoints2[PointId + 1].X, MineCartMovePoints2[PointId + 1].Y, MineCartMovePoints2[PointId + 1].Z);
                    else if (PointId < 32)
                        me->GetMotionMaster()->MovePoint(MineCartMovePoints2_L[PointId + 1].id, MineCartMovePoints2_L[PointId + 1].X, MineCartMovePoints2_L[PointId + 1].Y, MineCartMovePoints2_L[PointId + 1].Z);
                    else
                        RewardMineCart();
                    break;
                case BG_CREATURE_MINE_CART_3:
                    if (PointId < 8)
                        me->GetMotionMaster()->MovePoint(MineCartMovePoints3[PointId + 1].id, MineCartMovePoints3[PointId + 1].X, MineCartMovePoints3[PointId + 1].Y, MineCartMovePoints3[PointId + 1].Z);
                    else if (PointId < 40)
                        me->GetMotionMaster()->MovePoint(MineCartMovePoints2_L[PointId + 1].id, MineCartMovePoints2_L[PointId + 1].X, MineCartMovePoints2_L[PointId + 1].Y, MineCartMovePoints2_L[PointId + 1].Z);
                    else
                        RewardMineCart();
                    break;
            }*/
        }

        void UpdateAI(const uint32 diff)
        {
            if (updateTimer < diff)
            {
                std::list<Unit*> targetList;
                float radius = 20.0f;

                JadeCore::AnyUnitInObjectRangeCheck u_check(me, radius);
                JadeCore::UnitListSearcher<JadeCore::AnyUnitInObjectRangeCheck> searcher(me, targetList, u_check);
                me->VisitNearbyObject(radius, searcher);
                
                uint8 alliance_count = 0;
                uint8 horde_count = 0;

                if (!targetList.empty())
                {
                    for (auto itr : targetList)
                    {
                        if (Player* player = itr->ToPlayer())
                        {
                            if (player->GetBGTeam() == ALLIANCE)
                                alliance_count++;
                            else
                                horde_count++;
                        }
                    }
                }

                if (alliance_count > horde_count)
                {
                    me->RemoveAurasDueToSpell(SPELL_HORDE_MINE_CART);
                    me->CastSpell(me, SPELL_ALLIANCE_MINE_CART, false);
                }
                else if (alliance_count < horde_count)
                {
                    me->RemoveAurasDueToSpell(SPELL_ALLIANCE_MINE_CART);
                    me->CastSpell(me, SPELL_HORDE_MINE_CART, false);
                }

                updateTimer = 1000;
            }
            else
                updateTimer -= diff;
        }
    };
};

void AddSC_mine_cart()
{
    new npc_mine_cart();
}