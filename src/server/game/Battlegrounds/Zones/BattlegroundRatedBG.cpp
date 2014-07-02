/*
 * Copyright (C) 2013 SkyMist Project.
 *
 * This program is NOT free software; you can NOT redistribute it and/or modify it.
 */

#include "Player.h"
#include "Battleground.h"
#include "BattlegroundRatedBG.h"
#include "Language.h"

BattlegroundRatedBG::BattlegroundRatedBG()
{
    //TODO FIX ME!
    StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_WS_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_WS_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_WS_HAS_BEGUN;
}

BattlegroundRatedBG::~BattlegroundRatedBG()
{

}

void BattlegroundRatedBG::StartingEventCloseDoors()
{
}

void BattlegroundRatedBG::StartingEventOpenDoors()
{
}

void BattlegroundRatedBG::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    //create score and add it to map, default values are set in constructor
    BattlegroundRatedBGScore* sc = new BattlegroundRatedBGScore;

    PlayerScores[player->GetGUID()] = sc;
}

void BattlegroundRatedBG::RemovePlayer(Player* /*player*/, uint64 /*guid*/, uint32 /*team*/)
{
}

void BattlegroundRatedBG::HandleAreaTrigger(Player* /*Source*/, uint32 /*Trigger*/)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;
}

void BattlegroundRatedBG::UpdatePlayerScore(Player* Source, uint32 type, uint32 value, bool doAddHonor)
{
    std::map<uint64, BattlegroundScore*>::iterator itr = PlayerScores.find(Source->GetGUID());

    if (itr == PlayerScores.end())                         // player not found...
        return;

    Battleground::UpdatePlayerScore(Source, type, value, doAddHonor);
}
