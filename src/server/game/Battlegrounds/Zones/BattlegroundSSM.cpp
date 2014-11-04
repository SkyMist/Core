#include "Battleground.h"
#include "BattlegroundSSM.h"
#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

BattlegroundSSM::BattlegroundSSM()
{
    BgObjects.resize(BG_SSM_OBJECT_MAX);
    BgCreatures.resize(BG_CREATURES_MAX_SSM);
    
    m_Points[BG_TEAM_ALLIANCE] = 0;
    m_Points[BG_TEAM_HORDE] = 0;
}

BattlegroundSSM::~BattlegroundSSM()
{
}

void BattlegroundSSM::StartingEventCloseDoors()
{
    for (uint32 i = BG_SSM_OBJECT_DOOR_H_1; i <= BG_SSM_OBJECT_DOOR_H_2; ++i)
    {
        DoorClose(i);
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
    }

    for (uint32 i = BG_SSM_OBJECT_BUFF_1; i <= BG_SSM_OBJECT_BUFF_4; ++i)
        SpawnBGObject(i, RESPAWN_ONE_DAY);
}

void BattlegroundSSM::StartingEventOpenDoors()
{
    for (uint32 i = BG_SSM_OBJECT_DOOR_H_1; i <= BG_SSM_OBJECT_DOOR_H_2; ++i)
        DoorOpen(i);
}

WorldSafeLocsEntry const* BattlegroundSSM::GetClosestGraveYard(Player* player)
{
    if (player->GetBGTeam() == ALLIANCE)
    {
        return sWorldSafeLocsStore.LookupEntry(BG_SSM_GRAVEYARD_WATERFALL);
    }
    else
    {
        return sWorldSafeLocsStore.LookupEntry(BG_SSM_GRAVEYARD_LAVA);
    }
}

void BattlegroundSSM::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        if (m_Points[BG_TEAM_ALLIANCE] >= BG_SSM_POINTS_FOR_WIN)
            EndBattleground(1);
        else if (m_Points[BG_TEAM_HORDE] >= BG_SSM_POINTS_FOR_WIN)
            EndBattleground(2);
    }
}

void BattlegroundSSM::RewardMineCart(uint8 team)
{
    m_Points[team] += BG_SSM_POINTS_FROM_MINE_CART;
}

bool BattlegroundSSM::SetupBattleground()
{
    // Horde doors
    if (!AddObject(BG_SSM_OBJECT_DOOR_H_1, BG_OBJECT_DOOR_H_1_SSM_ENTRY, 652.8557f, 227.6726f, 329.0039f, 3.524531f, 0.03816891f, 0.0473032f, 1.020438f, -0.212105f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_DOOR_H_2, BG_OBJECT_DOOR_H_2_SSM_ENTRY, 635.2554f, 208.1812f, 327.9662f, 3.49866f, 1.962523f, 1.976324f, 1.016984f, -2.791346f, RESPAWN_IMMEDIATELY)
        // Mine cart end points
        || !AddObject(BG_SSM_OBJECT_MINE_CART_END_POINT_1, BG_OBJECT_MINE_CART_EP_1_SSM_ENTRY, 615.5099f, 79.41812f, 298.2867f, 1.654061f, 1.96592f, 1.973535f, 0.735445f, -2.701955f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_MINE_CART_END_POINT_2, BG_OBJECT_MINE_CART_EP_2_SSM_ENTRY, 564.8048f, 337.0873f, 347.1427f, 1.571254f, 0.01127672f, 0.006951332f, 0.7072344f, 0.7068551f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_MINE_CART_END_POINT_3, BG_OBJECT_MINE_CART_EP_3_SSM_ENTRY, 777.7377f, 502.3111f, 359.4537f, 0.719448f, 1.981547f, 1.982702f, 0.3516912f, -2.64222f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_MINE_CART_END_POINT_4, BG_OBJECT_MINE_CART_EP_4_SSM_ENTRY, 896.7936f, 25.29027f, 364.1431f, 3.539995f, 1.990183f, 0.03659725f, 1.020461f, -2.000877f, RESPAWN_IMMEDIATELY)
        // Buffs
        || !AddObject(BG_SSM_OBJECT_BUFF_1, BG_OBJECT_BUFF_1_SSM_ENTRY, 614.1354f, 120.0972f, 294.6562f, 4.409839f, 0, 0, 1.194408f, -0.653155f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_BUFF_2, BG_OBJECT_BUFF_2_SSM_ENTRY, 538.5104f, 396.9306f, 345.7396f, 4.059507f, 0, 0, 1.103485f, -0.4665612f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_BUFF_3, BG_OBJECT_BUFF_3_SSM_ENTRY, 789.8958f, 282.7639f, 355.0665f, 0.8819928f, 0, 0, 0.4268408f, 0.9043268f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_SSM_OBJECT_BUFF_4, BG_OBJECT_BUFF_4_SSM_ENTRY, 789.8958f, 282.7639f, 355.0665f, 0.8819928f, 0, 0, 0.4268408f, 0.9043268f, RESPAWN_IMMEDIATELY)
    )
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundSSM: Failed to spawn some object. Battleground not created!");
        return false;
    }

    // Mine cart
    if (!AddCreature(BG_CREATURE_MINE_CART_ENTRY, BG_CREATURE_MINE_CART_1, 0, 738.5997f, 204.5219f, 319.6078f, 2.248367f, 20 * 1000)
        || !AddCreature(BG_CREATURE_MINE_CART_ENTRY, BG_CREATURE_MINE_CART_2, 0, 759.8108f, 198.5496f, 319.5401f, 0.4215083f, 20 * 1000)
        || !AddCreature(BG_CREATURE_MINE_CART_ENTRY, BG_CREATURE_MINE_CART_3, 0, 744.472f, 183.0825f, 319.5455f, 4.338116f, 20 * 1000)
        // Track switch
        || !AddCreature(BG_CREATURE_TRACK_SWITCH_ENTRY, BG_CREATURE_TRACK_SWITCH_1, 0, 715.6424f, 100.1649f, 320.2845f, 4.592556f, RESPAWN_IMMEDIATELY)
        || !AddCreature(BG_CREATURE_TRACK_SWITCH_ENTRY, BG_CREATURE_TRACK_SWITCH_2, 0, 845.5573f, 307.5521f, 347.0379f, 0.6224777f, RESPAWN_IMMEDIATELY)
    )
    {
        sLog->outError(LOG_FILTER_SQL, "BatteGroundSSM: Failed to spawn some creatures. Battleground not created!");
        return false;
    }

    return true;
}

void BattlegroundSSM::Reset()
{
    //call parent's class reset
    Battleground::Reset();
}
