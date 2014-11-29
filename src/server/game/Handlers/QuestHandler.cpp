/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "QuestDef.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Battleground.h"
#include "BattlegroundAV.h"
#include "ScriptMgr.h"
#include "GameObjectAI.h"

// ! Note:
// There are two types of AutoAccept and AutoComplete Quests.
// 1. Flags / Special Flags are set, and in that case they have a questgiver, and when speaking to him the quest is auto accepted / complete window pops.
// 2. Introduced in Cata, representing AutoTake (QUEST_FLAGS_AUTO_TAKE) and SelfComplete (QUEST_FLAGS_AUTO_SUBMIT), no questgiver.
// THEORETICALLY, the second method uses quest flags to "signal" the client, which sends the bools in the CMSG's as "true", and we can use them.
// However, this does not happen, as the bools are completely UNKNOWN and unable to be related to any flags. Thus, all checks must be done manually.

void WorldSession::HandleQuestgiverStatusQueryOpcode(WorldPacket & recvData)
{
    ObjectGuid guid;

    uint8 bitOrder[8] = {2, 7, 3, 1, 6, 0, 4, 5};
    recvData.ReadBitInOrder(guid, bitOrder);

    recvData.FlushBits();

    uint8 byteOrder[8] = {2, 3, 6, 5, 4, 1, 0, 7};
    recvData.ReadBytesSeq(guid, byteOrder);

    uint32 questStatus = DIALOG_STATUS_NONE;

    Object* questgiver = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
    if (!questgiver)
    {
        sLog->outInfo(LOG_FILTER_NETWORKIO, "Error in CMSG_QUESTGIVER_STATUS_QUERY, called for not found questgiver (Typeid: %u GUID: %u)", GuidHigh2TypeId(GUID_HIPART(guid)), GUID_LOPART(guid));
        return;
    }

    switch (questgiver->GetTypeId())
    {
        case TYPEID_UNIT:
        {
            sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_STATUS_QUERY for npc, guid = %u", uint32(GUID_LOPART(guid)));

            Creature* cr_questgiver = questgiver->ToCreature();
            if (!cr_questgiver->IsHostileTo(_player))       // do not show quest status to enemies
            {
                questStatus = sScriptMgr->GetDialogStatus(_player, cr_questgiver);
                if (questStatus == DIALOG_STATUS_SCRIPTED_NO_STATUS)
                    questStatus = getDialogStatus(_player, cr_questgiver);
            }
            break;
        }
        case TYPEID_GAMEOBJECT:
        {
            sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_STATUS_QUERY for GameObject guid = %u", uint32(GUID_LOPART(guid)));

            GameObject* go_questgiver = (GameObject*)questgiver;
            questStatus = sScriptMgr->GetDialogStatus(_player, go_questgiver);
            if (questStatus == DIALOG_STATUS_SCRIPTED_NO_STATUS)
                questStatus = getDialogStatus(_player, go_questgiver);
            break;
        }

        default:
            sLog->outError(LOG_FILTER_NETWORKIO, "QuestGiver called for unexpected type %u", questgiver->GetTypeId());
            break;
    }

    // Inform client about the status of the quest.
    _player->PlayerTalkClass->SendQuestGiverStatus(questStatus, guid);
}

void WorldSession::HandleQuestgiverHelloOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;

    uint8 bitOrder[8] = {2, 3, 5, 4, 7, 6, 0, 1};
    recvData.ReadBitInOrder(guid, bitOrder);

    recvData.FlushBits();

    uint8 byteOrder[8] = {7, 1, 2, 6, 3, 5, 0, 4};
    recvData.ReadBytesSeq(guid, byteOrder);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_HELLO npc = %u", GUID_LOPART(guid));

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!creature)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleQuestgiverHelloOpcode - Unit (GUID: %u) not found or you can't interact with him.",
            GUID_LOPART(guid));
        return;
    }

    // Remove fake death.
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // Stop the npc if it's moving.
    creature->StopMoving();

    if (sScriptMgr->OnGossipHello(_player, creature))
        return;

    _player->PrepareGossipMenu(creature, creature->GetCreatureTemplate()->GossipMenuId, true);
    _player->SendPreparedGossip(creature);

    creature->AI()->sGossipHello(_player);
}

void WorldSession::HandleQuestgiverAcceptQuestOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 questId;

    recvData >> questId;

    guid[3] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();

    bool unk1 = recvData.ReadBit();

    guid[5] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();

    recvData.FlushBits();

    uint8 byteOrder[8] = {3, 4, 7, 2, 5, 1, 6, 0};
    recvData.ReadBytesSeq(guid, byteOrder);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_ACCEPT_QUEST npc = %u, quest = %u, unk1 = %u", uint32(GUID_LOPART(guid)), questId, unk1);

    Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM | TYPEMASK_PLAYER);

    // No questgiver.
    if (!object)
    {
        _player->PlayerTalkClass->SendCloseGossip();
        _player->SaveToDB();
        _player->SetDivider(0);
        return;
    }

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        // Some kind of WPE protection.
        if (!_player->CanInteractWithQuestGiver(object))
        {
            _player->PlayerTalkClass->SendCloseGossip();
            _player->SetDivider(0);
            return;
	    }

        // Incorrect questgiver. For players, allow only sharable (from other players) / auto taken (from same player) quests.
        if ((object->GetTypeId() != TYPEID_PLAYER && !object->hasQuest(questId)) || (object->GetTypeId() == TYPEID_PLAYER && ((object == _player && !quest->HasFlag(QUEST_FLAGS_AUTO_TAKE)) || (object != _player && !object->ToPlayer()->CanShareQuest(questId)))))
        {
            _player->PlayerTalkClass->SendCloseGossip();
            _player->SaveToDB();
            _player->SetDivider(0);
            return;
        }

        // Prevent cheating.
        if (!GetPlayer()->CanTakeQuest(quest, true))
        {
            _player->PlayerTalkClass->SendCloseGossip();
            _player->SetDivider(0);
            return;
        }

        // Cannot take an unsharable quest from another player.
        if (object && object->GetTypeId() == TYPEID_PLAYER && object != _player && !quest->HasFlag(QUEST_FLAGS_SHARABLE))
        {
            _player->PlayerTalkClass->SendCloseGossip();
            _player->SetDivider(0);
            return;
        }

        // Send the sharable quest to the other players.
        if (_player->GetDivider() != 0)
        {
            Player* player = ObjectAccessor::FindPlayer(_player->GetDivider());
            if (player)
            {
                player->SendPushToPartyResponse(_player, QUEST_PARTY_MSG_ACCEPT_QUEST);
                _player->SetDivider(0);
            }
        }

        // Add the quest to the player and check all conditions.
        if (_player->CanAddQuest(quest, true))
        {
            _player->AddQuestAndCheckCompletion(quest, object);

            if (quest->HasFlag(QUEST_FLAGS_PARTY_ACCEPT))
            {
                if (Group* group = _player->GetGroup())
                {
                    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                    {
                        Player* player = itr->getSource();

                        if (!player || player == _player)     // Don't send it to non-existing / offline players or the player himself.
                            continue;

                        // If he can take the quest, his divider is set to the original player GUID, his gossip window closes and the confirmation req is sent.
                        if (player->CanTakeQuest(quest, true))
                        {
                            player->SetDivider(_player->GetGUID());
                            player->PlayerTalkClass->SendCloseGossip();
                            _player->SendQuestConfirmAccept(quest, player);
                        }
                    }
                }
            }

            // The Gossip window closes and the player gets the SrcSpell cast on himself.
            _player->PlayerTalkClass->SendCloseGossip();

            if (quest->GetSrcSpell() > 0)
                _player->CastSpell(_player, quest->GetSrcSpell(), true);

            // The basics are done. As a special case, AutoComplete methods are checked and the objectives completed if the quest IsAutoComplete.
            if (quest->IsAutoComplete())
            {
                // Add quest items for quests that require items.
                for (uint8 x = 0; x < QUEST_ITEM_OBJECTIVES_COUNT; ++x)
                {
                    uint32 id = quest->RequiredItemId[x];
                    uint32 count = quest->RequiredItemCount[x];
                    if (!id || !count)
                        continue;

                    uint32 curItemCount = _player->GetItemCount(id, true);

                    ItemPosCountVec dest;
                    uint8 msg = _player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, id, count - curItemCount);
                    if (msg == EQUIP_ERR_OK)
                    {
                        Item* item = _player->StoreNewItem(dest, id, true);
                        _player->SendNewItem(item, count - curItemCount, true, false);
                    }
                }

                // All creatures / GO's slain / casted (Not required, but otherwise the quest will display "Creatures slain 0/10").
                for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
                {
                    int32 creature = quest->RequiredNpcOrGo[i];
                    uint32 creatureCount = quest->RequiredNpcOrGoCount[i];

                    if (uint32 spell_id = quest->RequiredSpellCast[i])
                    {
                        for (uint16 z = 0; z < creatureCount; ++z)
                            if (creature > 0)
                                _player->CastedCreatureOrGOForQuest(creature, true, spell_id);
                            else
                                _player->CastedCreatureOrGOForQuest(creature, false, spell_id);
                    }
                    else if (creature > 0)
                    {
                        if (CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(creature))
                            for (uint16 z = 0; z < creatureCount; ++z)
                                _player->KilledMonster(cInfo, 0);
                    }
                    else if (creature < 0)
                    {
                        for (uint16 z = 0; z < creatureCount; ++z)
                            _player->CastedCreatureOrGO(creature, 0, 0);
                    }
                }

                // If the quest requires currency to complete.
                for (uint8 y = 0; y < QUEST_REQUIRED_CURRENCY_COUNT; y++)
                {
                    uint32 currency = quest->RequiredCurrencyId[y];
                    uint32 currencyCount = quest->RequiredCurrencyCount[y];

                    if (!currency || !currencyCount)
                        continue;

                    _player->ModifyCurrency(currency, currencyCount);
                }

                // If the quest requires reputation to complete.
                if (uint32 repFaction = quest->GetRepObjectiveFaction())
                {
                    uint32 repValue = quest->GetRepObjectiveValue();
                    uint32 curRep = _player->GetReputationMgr().GetReputation(repFaction);
                    if (curRep < repValue)
                        if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(repFaction))
                            _player->GetReputationMgr().SetReputation(factionEntry, repValue);
                }

                // If the quest requires a secondary reputation to complete.
                if (uint32 repFaction = quest->GetRepObjectiveFaction2())
                {
                    uint32 repValue2 = quest->GetRepObjectiveValue2();
                    uint32 curRep = _player->GetReputationMgr().GetReputation(repFaction);
                    if (curRep < repValue2)
                        if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(repFaction))
                            _player->GetReputationMgr().SetReputation(factionEntry, repValue2);
                }

                // If the quest requires money.
                int32 ReqOrRewMoney = quest->GetRewOrReqMoney();
                if (ReqOrRewMoney < 0)
                    _player->ModifyMoney(-ReqOrRewMoney);

                _player->CompleteQuest(quest->GetQuestId());
            }

            // return here, as for quests taken the gossip window has already closed.
            return;
        }
    }

    // This only happens if the player can't take the quest, even if he passed the original checks.
    _player->PlayerTalkClass->SendCloseGossip();
}

void WorldSession::HandleQuestgiverQueryQuestOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 questId;

    recvData >> questId;

    guid[6] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();

    bool unk1 = recvData.ReadBit();

    guid[5] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();

    recvData.FlushBits();

    recvData.ReadByteSeq(guid[5]);
    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[7]);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_QUERY_QUEST npc = %u, quest = %u, unk1 = %u", uint32(GUID_LOPART(guid)), questId, unk1);

    // Verify that the guid is valid and is a questgiver or involved in the requested quest. This opcode is only sent when actually speaking to an NPC.
    Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM);
    if (!object || (!object->hasQuest(questId) && !object->hasInvolvedQuest(questId)))
    {
        _player->PlayerTalkClass->SendCloseGossip();
        return;
    }

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        if ((_player->GetQuestStatus(questId) == QUEST_STATUS_NONE || _player->GetQuestStatus(questId) == QUEST_STATUS_REWARDED && quest->CanBeRepeated()) && !_player->CanTakeQuest(quest, true))
            return;

        // On Complete status, Incomplete or AutoComplete status, the NPC sends the Request Items. That function does the further needed handling. 
        if (_player->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE || _player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE || quest->HasFlag(QUEST_FLAGS_AUTOCOMPLETE))
            _player->PlayerTalkClass->SendQuestGiverRequestItems(quest, object->GetGUID(), _player->CanCompleteQuest(quest->GetQuestId()), true);
        else
        {
            if (quest->IsAutoAccept())
            {
                if (Creature* pQuestGiver = ObjectAccessor::GetCreature(*_player, guid))
                    if (pQuestGiver->IsAIEnabled)
                        sScriptMgr->OnQuestAccept(_player, pQuestGiver, quest);

                if (_player->CanAddQuest(quest, true))
                    _player->AddQuestAndCheckCompletion(quest, object);
            }

            _player->PlayerTalkClass->SendQuestGiverQuestDetails(quest, object->GetGUID(), true);
        }
    }
}

void WorldSession::HandleQuestQueryOpcode(WorldPacket & recvData)
{
    uint32 questId;
    recvData >> questId;

    bool unk1 = recvData.ReadBit();

    // Something weird here, sometimes the entry is sent a second time + a guid.
    // This does seem to be related to the second Quest Entry sending in the SMSG response, but no clue as to how yet.
    recvData.rfinish();

    if (!_player)
        return;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUEST_QUERY quest = %u", questId);

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
        _player->PlayerTalkClass->SendQuestQueryResponse(quest);
}

void WorldSession::HandleQuestgiverChooseRewardOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 questId, reward;

    recvData >> reward;     // 5.x - reward value is now an item ID and not slot ID.
    recvData >> questId;

    uint8 bitOrder[8] = {2, 0, 7, 5, 6, 4, 1, 3};
    recvData.ReadBitInOrder(guid, bitOrder);

    recvData.FlushBits();

    uint8 byteOrder[8] = {7, 2, 4, 5, 3, 6, 1, 0}; 
    recvData.ReadBytesSeq(guid, byteOrder);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_CHOOSE_REWARD npc = %u, quest = %u, reward = %u", uint32(GUID_LOPART(guid)), questId, reward);

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    Object* object = _player;

    if (!quest->IsRewChoiceItemValid(reward))
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Error in CMSG_QUESTGIVER_CHOOSE_REWARD: player %s (guid %d) tried to get invalid reward (%u) (probably packet hacking)", _player->GetName(), _player->GetGUIDLow(), reward);
        return;
    }

    if (!quest->HasFlag(QUEST_FLAGS_AUTO_SUBMIT))
    {
        object = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
        if (!object || !object->hasInvolvedQuest(questId))
            return;

        // Some kind of WPE protection.
        if (!_player->CanInteractWithQuestGiver(object))
            return;
    }

    if ((!_player->CanSeeStartQuest(quest) && _player->GetQuestStatus(questId) == QUEST_STATUS_NONE) || (_player->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE && !quest->IsAutoComplete()))
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "HACK ALERT: Player %s (guid: %u) is trying to complete quest (id: %u) but he has no right to do it!", _player->GetName(), _player->GetGUIDLow(), questId);
        return;
    }

    if (_player->CanRewardQuest(quest, reward, true))
    {
        _player->RewardQuest(quest, reward, object);

        switch (object->GetTypeId())
        {
            case TYPEID_UNIT:
            case TYPEID_PLAYER:
            {
                // For AutoSubmit player case is added here as it is almost the same, excluding AI script cases.
                Creature* creatureQGiver = object->ToCreature();
                if (!creatureQGiver || !(sScriptMgr->OnQuestReward(_player, creatureQGiver, quest, reward)))
                {
                    // Send next quest.
                    if (Quest const* nextQuest = _player->GetNextQuest(guid, quest))
                    {
                        if (nextQuest->IsAutoAccept() && _player->CanAddQuest(nextQuest, true) && _player->CanTakeQuest(nextQuest, true))
                            _player->AddQuestAndCheckCompletion(nextQuest, object);

                        _player->PlayerTalkClass->SendQuestGiverQuestDetails(nextQuest, guid, true);
                    }

                    if (creatureQGiver)
                        creatureQGiver->AI()->sQuestReward(_player, quest, reward);
                }
                break;
            }
            case TYPEID_GAMEOBJECT:
            {
                if (!sScriptMgr->OnQuestReward(_player, object->ToGameObject(), quest, reward))
                {
                    // Send next quest.
                    if (Quest const* nextQuest = _player->GetNextQuest(guid, quest))
                    {
                        if (nextQuest->IsAutoAccept() && _player->CanAddQuest(nextQuest, true) && _player->CanTakeQuest(nextQuest, true))
                            _player->AddQuestAndCheckCompletion(nextQuest, object);
                
                        _player->PlayerTalkClass->SendQuestGiverQuestDetails(nextQuest, guid, true);
                    }
                
                    object->ToGameObject()->AI()->QuestReward(_player, quest, reward);
                }
                break;
            }

            default: break;
        }
    }
    else _player->PlayerTalkClass->SendQuestGiverOfferReward(quest, guid, true);
}

void WorldSession::HandleQuestgiverRequestRewardOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 questId;

    recvData >> questId;

    uint8 bitOrder[8] = {4, 1, 7, 0, 3, 2, 6, 5};
    recvData.ReadBitInOrder(guid, bitOrder);

    recvData.FlushBits();

    uint8 byteOrder[8] = {7, 2, 6, 4, 3, 1, 5, 0};
    recvData.ReadBytesSeq(guid, byteOrder);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_REQUEST_REWARD npc = %u, quest = %u", uint32(GUID_LOPART(guid)), questId);

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    if (!quest->HasFlag(QUEST_FLAGS_AUTO_SUBMIT))
    {
        Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
        if (!object || !object->hasInvolvedQuest(questId))
            return;

        // Some kind of WPE protection.
        if (!_player->CanInteractWithQuestGiver(object))
            return;
    }

    if (_player->CanCompleteQuest(questId))
        _player->CompleteQuest(questId);

    if (_player->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
        return;

    _player->PlayerTalkClass->SendQuestGiverOfferReward(quest, guid, true);
}

void WorldSession::HandleQuestLogRemoveQuest(WorldPacket& recvData)
{
    uint8 slot;
    recvData >> slot;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTLOG_REMOVE_QUEST slot = %u", slot);

    if (slot < MAX_QUEST_LOG_SIZE)
    {
        if (uint32 questId = _player->GetQuestSlotQuestId(slot))
        {
            if (!_player->TakeQuestSourceItem(questId, true)) // Can't un-equip some items, reject quest cancel.
                return;

            if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            {
                if (quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_TIMED))
                    _player->RemoveTimedQuest(questId);

                // Destroy items received during the quest.
                for (uint8 i = 0; i < QUEST_SOURCE_ITEM_IDS_COUNT; ++i)
                    if (quest->RequiredSourceItemId[i] > 0 && quest->RequiredSourceItemCount[i] > 0)
                        _player->DestroyItemCount(quest->RequiredSourceItemId[i], quest->RequiredSourceItemCount[i], true, true);
            }

            _player->TakeQuestSourceItem(questId, true); // Remove quest src item from the player.
            _player->RemoveActiveQuest(questId);
            _player->GetAchievementMgr().RemoveTimedAchievement(ACHIEVEMENT_TIMED_TYPE_QUEST, questId);

            sLog->outInfo(LOG_FILTER_NETWORKIO, "Player %u abandoned quest %u", _player->GetGUIDLow(), questId);
        }

        _player->SetQuestSlot(slot, 0);

        _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED, 1);
    }
}

void WorldSession::HandleQuestConfirmAccept(WorldPacket& recvData)
{
    uint32 questId;
    recvData >> questId;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUEST_CONFIRM_ACCEPT questId = %u", questId);

    if (const Quest* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        if (!quest->HasFlag(QUEST_FLAGS_PARTY_ACCEPT))
            return;

        Player* pOriginalPlayer = ObjectAccessor::FindPlayer(_player->GetDivider());

        if (!pOriginalPlayer)
            return;

        if (quest->IsRaidQuest(pOriginalPlayer->GetMap()->GetDifficulty()))
        {
            if (!_player->IsInSameRaidWith(pOriginalPlayer))
                return;
        }
        else
        {
            if (!_player->IsInSameGroupWith(pOriginalPlayer))
                return;
        }

        if (_player->CanAddQuest(quest, true))
            _player->AddQuestAndCheckCompletion(quest, NULL); // NULL, this prevents DB script from duplicate running.

        _player->SetDivider(0);
    }
}

void WorldSession::HandleQuestgiverCompleteQuest(WorldPacket& recvData)
{
    ObjectGuid playerGuid;
    uint32 questId;

    recvData >> questId;

    playerGuid[6] = recvData.ReadBit();
    playerGuid[5] = recvData.ReadBit();
    playerGuid[7] = recvData.ReadBit();
    playerGuid[4] = recvData.ReadBit();
    playerGuid[3] = recvData.ReadBit();
    playerGuid[0] = recvData.ReadBit();

    bool unk1 = recvData.ReadBit();

    playerGuid[1] = recvData.ReadBit();
    playerGuid[2] = recvData.ReadBit();

    recvData.FlushBits();

    uint8 byteOrder[8] = {0, 5, 3, 2, 4, 6, 1, 7};
    recvData.ReadBytesSeq(playerGuid, byteOrder);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_COMPLETE_QUEST npc = %u, questId = %u", uint32(GUID_LOPART(playerGuid)), questId);

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    if (!quest->HasFlag(QUEST_FLAGS_AUTO_SUBMIT))
    {
        Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, playerGuid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
        if (!object || !object->hasInvolvedQuest(questId))
            return;

        // Some kind of WPE protection.
        if (!_player->CanInteractWithQuestGiver(object))
            return;
    }

    if (!_player->CanSeeStartQuest(quest) && _player->GetQuestStatus(questId) == QUEST_STATUS_NONE)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Possible hacking attempt: Player %s [playerGuid: %u] tried to complete questId [entry: %u] without being in possession of the questId!", _player->GetName(), _player->GetGUIDLow(), questId);
        return;
    }

    if (_player->InBattleground())
        if (Battleground* bg = _player->GetBattleground())
            bg->HandleQuestComplete(questId, _player);

    if (_player->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
    {
        if (quest->IsRepeatable())
            _player->PlayerTalkClass->SendQuestGiverRequestItems(quest, playerGuid, _player->CanCompleteRepeatableQuest(quest), false);
        else
            _player->PlayerTalkClass->SendQuestGiverRequestItems(quest, playerGuid, _player->CanRewardQuest(quest, false), false);
    }
    else
    {
        if (quest->GetReqItemsCount())                  // some items required
            _player->PlayerTalkClass->SendQuestGiverRequestItems(quest, playerGuid, _player->CanRewardQuest(quest, false), false);
        else                                            // no items required
            _player->PlayerTalkClass->SendQuestGiverOfferReward(quest, playerGuid, !quest->HasFlag(QUEST_FLAGS_AUTO_SUBMIT));
    }

    if (Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, playerGuid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT))
        if (Creature* creature = object->ToCreature())
            sScriptMgr->OnQuestComplete(_player, creature, quest);

    if (_player)
        _player->SaveToDB();
}

void WorldSession::HandlePushQuestToParty(WorldPacket& recvPacket)
{
    uint32 questId;
    recvPacket >> questId;

    if (!_player->CanShareQuest(questId))
        return;

    if (_player->GetQuestStatus(questId) == QUEST_STATUS_NONE || _player->GetQuestStatus(questId) == QUEST_STATUS_REWARDED)
        return;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_PUSHQUESTTOPARTY questId = %u", questId);

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    Player* sender = GetPlayer();

    Group* group = sender->GetGroup();
    if (!group)
        return;

    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* receiver = itr->getSource();

        // Skip sending it to yourself.
        if (!receiver || receiver == sender)
            continue;

        if (!receiver->SatisfyQuestStatus(quest, false))
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_HAVE_QUEST);
            continue;
        }

        if (receiver->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_FINISH_QUEST);
            continue;
        }

        if (!receiver->CanTakeQuest(quest, false))
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_CANT_TAKE_QUEST);
            continue;
        }

        if (!receiver->SatisfyQuestLog(false))
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_LOG_FULL);
            continue;
        }

        if (receiver->GetDivider() != 0)
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_BUSY);
            continue;
        }

        sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_SHARING_QUEST);
    
        if (quest->IsAutoAccept() && receiver->CanAddQuest(quest, true) && receiver->CanTakeQuest(quest, true))
            receiver->AddQuestAndCheckCompletion(quest, sender);
    
        if ((quest->IsAutoComplete() && quest->IsRepeatable() && !quest->IsDailyOrWeekly()) || quest->HasFlag(QUEST_FLAGS_AUTOCOMPLETE))
            receiver->PlayerTalkClass->SendQuestGiverRequestItems(quest, sender->GetGUID(), receiver->CanCompleteRepeatableQuest(quest), true);
        else
        {
            receiver->SetDivider(sender->GetGUID());
            receiver->PlayerTalkClass->SendQuestGiverQuestDetails(quest, sender->GetGUID(), true);
        }
    }
}

uint32 WorldSession::getDialogStatus(Player* player, Object* questgiver)
{
    uint32 result = DIALOG_STATUS_NONE;

    QuestRelationBounds qr;
    QuestRelationBounds qir;

    switch (questgiver->GetTypeId())
    {
        case TYPEID_GAMEOBJECT:
        {
            qr  = sObjectMgr->GetGOQuestRelationBounds(questgiver->GetEntry());
            qir = sObjectMgr->GetGOQuestInvolvedRelationBounds(questgiver->GetEntry());
            break;
        }

        case TYPEID_UNIT:
        {
            qr  = sObjectMgr->GetCreatureQuestRelationBounds(questgiver->GetEntry());
            qir = sObjectMgr->GetCreatureQuestInvolvedRelationBounds(questgiver->GetEntry());
            break;
        }

        default:  // It's impossible, but check.
            sLog->outError(LOG_FILTER_NETWORKIO, "Warning: GetDialogStatus called for unexpected type %u", questgiver->GetTypeId());
            return DIALOG_STATUS_NONE;
    }

    for (QuestRelations::const_iterator i = qir.first; i != qir.second; ++i)
    {
        uint32 result2 = 0;
        uint32 quest_id = i->second;
        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (!quest)
            continue;

        ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_QUEST_SHOW_MARK, quest->GetQuestId());
        if (!sConditionMgr->IsObjectMeetToConditions(player, conditions))
            continue;

        QuestStatus status = player->GetQuestStatus(quest_id);
        if ((status == QUEST_STATUS_COMPLETE && !player->GetQuestRewardStatus(quest_id)) || (quest->IsAutoComplete() && player->CanTakeQuest(quest, false)))
        {
            if (quest->IsAutoComplete() && quest->IsRepeatable() && !quest->IsDailyOrWeekly())
                result2 = DIALOG_STATUS_REWARD_REP;
            else
                result2 = DIALOG_STATUS_REWARD;
        }
        else if (status == QUEST_STATUS_INCOMPLETE)
            result2 = DIALOG_STATUS_INCOMPLETE;

        if (result2 > result)
            result = result2;
    }

    for (QuestRelations::const_iterator i = qr.first; i != qr.second; ++i)
    {
        uint32 result2 = 0;
        uint32 quest_id = i->second;
        Quest const* quest = sObjectMgr->GetQuestTemplate(quest_id);
        if (!quest)
            continue;

        ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_QUEST_SHOW_MARK, quest->GetQuestId());
        if (!sConditionMgr->IsObjectMeetToConditions(player, conditions))
            continue;

        QuestStatus status = player->GetQuestStatus(quest_id);
        if (status == QUEST_STATUS_NONE)
        {
            if (player->CanSeeStartQuest(quest))
            {
                if (player->SatisfyQuestLevel(quest, false))
                {
                    if (quest->IsAutoComplete())
                        result2 = DIALOG_STATUS_REWARD_REP;
                    else if (player->getLevel() <= ((player->GetQuestLevel(quest) == -1) ? player->getLevel() : player->GetQuestLevel(quest) + sWorld->getIntConfig(CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF)))
                    {
                        if (quest->IsDaily())
                            result2 = DIALOG_STATUS_AVAILABLE_REP;
                        else
                            result2 = DIALOG_STATUS_AVAILABLE;
                    }
                    else
                        result2 = DIALOG_STATUS_LOW_LEVEL_AVAILABLE;
                }
                else
                    result2 = DIALOG_STATUS_UNAVAILABLE;
            }
        }

        if (result2 > result)
            result = result2;
    }

    return result;
}

void WorldSession::HandleQuestgiverStatusMultipleQuery(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUESTGIVER_STATUS_MULTIPLE_QUERY");

    uint32 count = 0;
    ByteBuffer bitData, byteData;

    for (Player::ClientGUIDs::const_iterator itr = _player->m_clientGUIDs.begin(); itr != _player->m_clientGUIDs.end(); ++itr)
    {
        uint32 questStatus = DIALOG_STATUS_NONE;

        if (IS_CRE_OR_VEH_OR_PET_GUID(*itr))
        {
            // Need also pet quests case support.
            Creature* questgiver = ObjectAccessor::GetCreatureOrPetOrVehicle(*GetPlayer(), *itr);
            if (!questgiver || questgiver->IsHostileTo(_player))
                continue;
            if (!questgiver->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
                continue;

            questStatus = sScriptMgr->GetDialogStatus(_player, questgiver);
            if (questStatus == DIALOG_STATUS_SCRIPTED_NO_STATUS)
                questStatus = getDialogStatus(_player, questgiver);

            ObjectGuid NPCGuid = questgiver->GetGUID();

            bitData.WriteBit(NPCGuid[7]);
            bitData.WriteBit(NPCGuid[0]);
            bitData.WriteBit(NPCGuid[6]);
            bitData.WriteBit(NPCGuid[2]);
            bitData.WriteBit(NPCGuid[5]);
            bitData.WriteBit(NPCGuid[1]);
            bitData.WriteBit(NPCGuid[4]);
            bitData.WriteBit(NPCGuid[3]);

            byteData.WriteByteSeq(NPCGuid[5]);

            byteData << uint32(questStatus);

            byteData.WriteByteSeq(NPCGuid[4]);
            byteData.WriteByteSeq(NPCGuid[2]);
            byteData.WriteByteSeq(NPCGuid[3]);
            byteData.WriteByteSeq(NPCGuid[6]);
            byteData.WriteByteSeq(NPCGuid[1]);
            byteData.WriteByteSeq(NPCGuid[7]);
            byteData.WriteByteSeq(NPCGuid[0]);

            // Inform the client about the quest status.
            if (questStatus >= DIALOG_STATUS_NONE)
                _player->PlayerTalkClass->SendQuestGiverStatus(questStatus, questgiver->GetGUID());

            ++count;
        }
        else if (IS_GAMEOBJECT_GUID(*itr))
        {
            GameObject* questgiver = GetPlayer()->GetMap()->GetGameObject(*itr);
            if (!questgiver)
                continue;
            if (questgiver->GetGoType() != GAMEOBJECT_TYPE_QUESTGIVER)
                continue;

            questStatus = sScriptMgr->GetDialogStatus(_player, questgiver);
            if (questStatus == DIALOG_STATUS_SCRIPTED_NO_STATUS)
                questStatus = getDialogStatus(_player, questgiver);

            ObjectGuid GOGuid = questgiver->GetGUID();

            bitData.WriteBit(GOGuid[7]);
            bitData.WriteBit(GOGuid[0]);
            bitData.WriteBit(GOGuid[6]);
            bitData.WriteBit(GOGuid[2]);
            bitData.WriteBit(GOGuid[5]);
            bitData.WriteBit(GOGuid[1]);
            bitData.WriteBit(GOGuid[4]);
            bitData.WriteBit(GOGuid[3]);

            byteData.WriteByteSeq(GOGuid[5]);

            byteData << uint32(questStatus);

            byteData.WriteByteSeq(GOGuid[4]);
            byteData.WriteByteSeq(GOGuid[2]);
            byteData.WriteByteSeq(GOGuid[3]);
            byteData.WriteByteSeq(GOGuid[6]);
            byteData.WriteByteSeq(GOGuid[1]);
            byteData.WriteByteSeq(GOGuid[7]);
            byteData.WriteByteSeq(GOGuid[0]);

            // Inform the client about the quest status.
            if (questStatus >= DIALOG_STATUS_NONE)
                _player->PlayerTalkClass->SendQuestGiverStatus(questStatus, GOGuid);

            ++count;
        }
    }

    WorldPacket data(SMSG_QUESTGIVER_STATUS_MULTIPLE, 3 + count * (1 + 8 + 4));

    data.WriteBits(count, 21);

    data.append(bitData);
    data.FlushBits();
    data.append(byteData);

    SendPacket(&data);
}

void WorldSession::HandleQueryQuestsCompleted(WorldPacket& /*recvData*/)
{
    size_t rew_count = _player->GetRewardedQuestCount();

    WorldPacket data(SMSG_QUERY_QUESTS_COMPLETED_RESPONSE, 4 + 4 * rew_count);
    data << uint32(rew_count);

    const RewardedQuestSet &rewQuests = _player->getRewardedQuests();

    if (!rewQuests.empty())
        for (RewardedQuestSet::const_iterator itr = rewQuests.begin(); itr != rewQuests.end(); ++itr)
            data << uint32(*itr);

    SendPacket(&data);
}

void WorldSession::HandleQuestNPCQuery(WorldPacket& recvData)
{
    size_t RequestCount = recvData.ReadBits(22);
    size_t ResponseCount = 0;

    if (RequestCount)
    {
        WorldPacket Response(SMSG_QUEST_NPC_QUERY_RESPONSE);

        QuestRelations* NPCRelations = sObjectMgr->GetCreatureQuestInvolvedMap();
        ByteBuffer BitPart;
        ByteBuffer BytePart;

        for (size_t i = 0; i < RequestCount; i++)
        {
            size_t NPCCount = 0;
            uint32 QuestID;

            recvData >> QuestID;

            if (NPCRelations->size())
            {
                for (QuestRelations::const_iterator itr = NPCRelations->begin(); itr != NPCRelations->end(); ++itr)
                {
                    if (itr->second == QuestID)
                    {
                        BytePart << uint32(itr->first);
                        NPCCount++;
                    }
                }
            }

            ResponseCount++;
            BitPart.WriteBits(NPCCount, 22);
        }

        Response.WriteBits(ResponseCount, 21);

        if (ResponseCount) // Failproof :P.
        {
            Response.append(BitPart);
            Response.FlushBits();
            Response.append(BytePart);
        }

        SendPacket(&Response);
    }
}
