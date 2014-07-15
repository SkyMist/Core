/*
 * Copyright (C) 2011-2014 Project SkyFire <http://www.projectskyfire.org/>
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2014 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include "DatabaseEnv.h"
#include "Mail.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Language.h"
#include "DBCStores.h"
#include "Item.h"
#include "AccountMgr.h"
#include "GuildMgr.h"

void WorldSession::HandleSendMail(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint64 money, COD;
    std::string receiverName, subject, body;
    uint32 bodyLength, subjectLength, receiverLength, StationeryID;
    uint32 unk1;

    recvData >> unk1 >> COD >> StationeryID >> money;

    uint8 items_count = recvData.ReadBits(5);              // attached items count
    
    mailbox[0] = recvData.ReadBit();
    
    if (items_count > MAX_MAIL_ITEMS)                      // client limit
    {
        GetPlayer()->SendMailResult(0, MAIL_SEND, MAIL_ERR_TOO_MANY_ATTACHMENTS);
        recvData.rfinish();                   // set to end to avoid warnings spam
        return;
    }
    
    ObjectGuid itemGUIDs[MAX_MAIL_ITEMS];
    
    for (uint8 i = 0; i < items_count; ++i)
    {
        itemGUIDs[i][6] = recvData.ReadBit();
        itemGUIDs[i][5] = recvData.ReadBit();
        itemGUIDs[i][1] = recvData.ReadBit();
        itemGUIDs[i][3] = recvData.ReadBit();
        itemGUIDs[i][0] = recvData.ReadBit();
        itemGUIDs[i][4] = recvData.ReadBit();
        itemGUIDs[i][7] = recvData.ReadBit();
        itemGUIDs[i][2] = recvData.ReadBit();
    }
    
    mailbox[2] = recvData.ReadBit();
    mailbox[3] = recvData.ReadBit();
    mailbox[7] = recvData.ReadBit();
    mailbox[5] = recvData.ReadBit();
    
    bodyLength = recvData.ReadBits(11);
    receiverLength = recvData.ReadBits(9);
    
    mailbox[1] = recvData.ReadBit();
    mailbox[4] = recvData.ReadBit();
    mailbox[6] = recvData.ReadBit();
    
    subjectLength = recvData.ReadBits(9);
    
    recvData.ReadByteSeq(mailbox[0]);
    recvData.ReadByteSeq(mailbox[2]);
    recvData.ReadByteSeq(mailbox[6]);
    recvData.ReadByteSeq(mailbox[3]);
    recvData.ReadByteSeq(mailbox[4]);
    recvData.ReadByteSeq(mailbox[5]);

    for (uint8 i = 0; i < items_count; ++i)
    {
        recvData.ReadByteSeq(itemGUIDs[i][6]);
        recvData.ReadByteSeq(itemGUIDs[i][3]);
        recvData.ReadByteSeq(itemGUIDs[i][0]);
        recvData.ReadByteSeq(itemGUIDs[i][5]);
        recvData.ReadByteSeq(itemGUIDs[i][7]);
        recvData.ReadByteSeq(itemGUIDs[i][4]);
        recvData.read_skip<uint8>();            // item slot in mail, not used
        recvData.ReadByteSeq(itemGUIDs[i][1]);
        recvData.ReadByteSeq(itemGUIDs[i][2]);
    }
    
    subject = recvData.ReadString(subjectLength);
    recvData.ReadByteSeq(mailbox[1]);
    receiverName = recvData.ReadString(receiverLength);
    recvData.ReadByteSeq(mailbox[7]);
    body = recvData.ReadString(bodyLength);

    

    // packet read complete, now do check

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    if (receiverName.empty())
        return;

    Player* player = _player;

    if (player->getLevel() < sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_MAIL_SENDER_REQ), sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ));
        return;
    }

    uint64 receiverGuid = 0;
    if (normalizePlayerName(receiverName))
        receiverGuid = sObjectMgr->GetPlayerGUIDByName(receiverName);

    if (!receiverGuid)
    {
        TC_LOG_INFO("network", "Player %u is sending mail to %s (GUID: not existed!) with subject %s "
            "and body %s includes %u items, " UI64FMTD " copper and " UI64FMTD " COD copper with StationeryID = %u, unk1 = %u",
            player->GetGUIDLow(), receiverName.c_str(), subject.c_str(), body.c_str(),
            items_count, money, COD, StationeryID, unk1);
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_NOT_FOUND);
        return;
    }

    TC_LOG_INFO("network", "Player %u is sending mail to %s (GUID: %u) with subject %s and body %s "
        "includes %u items, " UI64FMTD " copper and " UI64FMTD " COD copper with Stationery = %u, unk1 = %u",
        player->GetGUIDLow(), receiverName.c_str(), GUID_LOPART(receiverGuid), subject.c_str(),
        body.c_str(), items_count, money, COD, StationeryID, unk1);

    if (player->GetGUID() == receiverGuid)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANNOT_SEND_TO_SELF);
        return;
    }

    uint32 cost = items_count ? 30 * items_count : 30;  // price hardcoded in client

    uint64 reqmoney = cost + money;

    if (!player->HasEnoughMoney(reqmoney) && !player->IsGameMaster())
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Player* receiver = ObjectAccessor::FindPlayer(receiverGuid);

    uint32 receiverTeam = 0;
    uint8 mailsCount = 0;                                  //do not allow to send to one player more than 100 mails
    uint8 receiverLevel = 0;
    uint32 receiverAccountId = 0;

    if (receiver)
    {
        receiverTeam = receiver->GetTeam();
        mailsCount = receiver->GetMailSize();
        receiverLevel = receiver->getLevel();
        receiverAccountId = receiver->GetSession()->GetAccountId();
    }
    else
    {
        receiverTeam = sObjectMgr->GetPlayerTeamByGUID(receiverGuid);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAIL_COUNT);
        stmt->setUInt32(0, GUID_LOPART(receiverGuid));

        PreparedQueryResult result = CharacterDatabase.Query(stmt);
        if (result)
        {
            Field* fields = result->Fetch();
            mailsCount = fields[0].GetUInt64();
        }

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_LEVEL);
        stmt->setUInt32(0, GUID_LOPART(receiverGuid));

        result = CharacterDatabase.Query(stmt);
        if (result)
        {
            Field* fields = result->Fetch();
            receiverLevel = fields[0].GetUInt8();
        }

        receiverAccountId = sObjectMgr->GetPlayerAccountIdByGUID(receiverGuid);
    }

    // do not allow to have more than 100 mails in mailbox.. mails count is in opcode uint8!!! - so max can be 255..
    if (mailsCount > 100)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_CAP_REACHED);
        return;
    }

    // test the receiver's Faction... or all items are account bound
    bool accountBound = items_count ? true : false;
    for (uint8 i = 0; i < items_count; ++i)
    {
        if (Item* item = player->GetItemByGuid(itemGUIDs[i]))
        {
            ItemTemplate const* itemProto = item->GetTemplate();
            if (!itemProto || !(itemProto->Flags & ITEM_PROTO_FLAG_BIND_TO_ACCOUNT))
            {
                accountBound = false;
                break;
            }
        }
    }

    if (!accountBound && player->GetTeam() != receiverTeam && !HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_MAIL))
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_YOUR_TEAM);
        return;
    }

    if (receiverLevel < sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_MAIL_RECEIVER_REQ), sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ));
        return;
    }

    Item* items[MAX_MAIL_ITEMS];

    for (uint8 i = 0; i < items_count; ++i)
    {
        if (!itemGUIDs[i])
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        Item* item = player->GetItemByGuid(itemGUIDs[i]);

        // prevent sending bag with items (cheat: can be placed in bag after adding equipped empty bag to mail)
        if (!item)
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        if (!item->CanBeTraded(true))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (item->IsBoundAccountWide() && item->IsSoulBound() && player->GetSession()->GetAccountId() != receiverAccountId)
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_NOT_SAME_ACCOUNT);
            return;
        }

        if (item->GetTemplate()->Flags & ITEM_PROTO_FLAG_CONJURED || item->GetUInt32Value(ITEM_FIELD_EXPIRATION))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (COD && item->HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_WRAPPED))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANT_SEND_WRAPPED_COD);
            return;
        }

        if (item->IsNotEmptyBag())
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_DESTROY_NONEMPTY_BAG);
            return;
        }

        items[i] = item;
    }

    player->SendMailResult(0, MAIL_SEND, MAIL_OK);

    player->ModifyMoney(-int64(reqmoney));
    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL, cost);

    bool needItemDelay = false;

    MailDraft draft(subject, body);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    if (items_count > 0 || money > 0)
    {
        bool log = HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE);
        if (items_count > 0)
        {
            for (uint8 i = 0; i < items_count; ++i)
            {
                Item* item = items[i];
                if (log)
                {
                    sLog->outCommand(GetAccountId(), "GM %s (GUID: %u) (Account: %u) mail item: %s (Entry: %u Count: %u) "
                        "to player: %s (GUID: %u) (Account: %u)", GetPlayerName().c_str(), GetGuidLow(), GetAccountId(),
                        item->GetTemplate()->Name1.c_str(), item->GetEntry(), item->GetCount(),
                        receiverName.c_str(), GUID_LOPART(receiverGuid), receiverAccountId);
                }

                item->SetNotRefundable(GetPlayer()); // makes the item no longer refundable
                player->MoveItemFromInventory(items[i]->GetBagSlot(), item->GetSlot(), true);

                item->DeleteFromInventoryDB(trans);     // deletes item from character's inventory
                item->SetOwnerGUID(receiverGuid);
                item->SaveToDB(trans);                  // recursive and not have transaction guard into self, item not in inventory and can be save standalone

                draft.AddItem(item);
            }

            // if item send to character at another account, then apply item delivery delay
            needItemDelay = player->GetSession()->GetAccountId() != receiverAccountId;
        }

        if (log && money > 0)
        {
            sLog->outCommand(GetAccountId(), "GM %s (GUID: %u) (Account: %u) mail money: " UI64FMTD " to player: %s (GUID: %u) (Account: %u)",
                GetPlayerName().c_str(), GetGuidLow(), GetAccountId(), money, receiverName.c_str(), GUID_LOPART(receiverGuid), receiverAccountId);
        }
    }

    // If theres is an item, there is a one hour delivery delay if sent to another account's character.
    uint32 deliver_delay = needItemDelay ? sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY) : 0;

    // Mail sent between guild members arrives instantly if they have the guild perk "Guild Mail"
    if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId()))
        if (guild->GetLevel() >= 17 && guild->IsMember(receiverGuid))
            deliver_delay = 0;

    // will delete item or place to receiver mail list
    draft
        .AddMoney(money)
        .AddCOD(COD)
        .SendMailTo(trans, MailReceiver(receiver, GUID_LOPART(receiverGuid)), MailSender(player), body.empty() ? MAIL_CHECK_MASK_COPIED : MAIL_CHECK_MASK_HAS_BODY, deliver_delay);

    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
}

// Called when mail is read.
void WorldSession::HandleMailMarkAsRead(WorldPacket& recvData)
{
    ObjectGuid GuidMail;
    uint32 mailId;

	GuidMail[4] = recvData.ReadBit();
	GuidMail[0] = recvData.ReadBit();
    GuidMail[2] = recvData.ReadBit();
    GuidMail[7] = recvData.ReadBit();
    GuidMail[1] = recvData.ReadBit();
    GuidMail[6] = recvData.ReadBit();
    GuidMail[3] = recvData.ReadBit();

    recvData >> mailId;

    GuidMail[5] = recvData.ReadBit();

    recvData.ReadByteSeq(GuidMail[2]);
    recvData.ReadByteSeq(GuidMail[5]);
    recvData.ReadByteSeq(GuidMail[0]);
    recvData.ReadByteSeq(GuidMail[6]);
    recvData.ReadByteSeq(GuidMail[1]);
    recvData.ReadByteSeq(GuidMail[3]);
    recvData.ReadByteSeq(GuidMail[7]);
    recvData.ReadByteSeq(GuidMail[4]);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(GuidMail, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;
    Mail* m = player->GetMail(mailId);
    if (m)
    {
        if (player->unReadMails)
            --player->unReadMails;
        m->checked = m->checked | MAIL_CHECK_MASK_READ;
        player->m_mailsUpdated = true;
        m->state = MAIL_STATE_CHANGED;
    }
}

//called when client deletes mail
void WorldSession::HandleMailDelete(WorldPacket& recvData)
{
    //uint64 mailbox;
    uint32 mailId;
    //recvData >> mailbox;
    recvData.read_skip<uint32>();                          // mailTemplateId
    recvData >> mailId;
    

    /*if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;*/

    Mail* m = _player->GetMail(mailId);
    Player* player = _player;
    player->m_mailsUpdated = true;
    if (m)
    {
        // delete shouldn't show up for COD mails
        if (m->COD)
        {
            player->SendMailResult(mailId, MAIL_DELETED, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        m->state = MAIL_STATE_DELETED;
    }
    player->SendMailResult(mailId, MAIL_DELETED, MAIL_OK);
}

void WorldSession::HandleMailReturnToSender(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;

    recvData >> mailId;

    mailbox[1] = recvData.ReadBit();
    mailbox[7] = recvData.ReadBit();
    mailbox[6] = recvData.ReadBit();
    mailbox[3] = recvData.ReadBit();
    mailbox[4] = recvData.ReadBit();
    mailbox[0] = recvData.ReadBit();
    mailbox[2] = recvData.ReadBit();
    mailbox[5] = recvData.ReadBit();

    recvData.ReadByteSeq(mailbox[7]);
    recvData.ReadByteSeq(mailbox[3]);
    recvData.ReadByteSeq(mailbox[6]);
    recvData.ReadByteSeq(mailbox[0]);
    recvData.ReadByteSeq(mailbox[2]);
    recvData.ReadByteSeq(mailbox[1]);
    recvData.ReadByteSeq(mailbox[5]);
    recvData.ReadByteSeq(mailbox[4]);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;
    Mail* m = player->GetMail(mailId);
    if (!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        player->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_ERR_INTERNAL_ERROR);
        return;
    }
    //we can return mail now, so firstly delete the old one
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_BY_ID);
    stmt->setUInt32(0, mailId);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM_BY_ID);
    stmt->setUInt32(0, mailId);
    trans->Append(stmt);

    player->RemoveMail(mailId);

    // only return mail if the player exists (and delete if not existing)
    if (m->messageType == MAIL_NORMAL && m->sender)
    {
        MailDraft draft(m->subject, m->body);
        if (m->mailTemplateId)
            draft = MailDraft(m->mailTemplateId, false);     // items already included

        if (m->HasItems())
        {
            for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
            {
                if (Item* const item = player->GetMItem(itr2->item_guid))
                    draft.AddItem(item);
                player->RemoveMItem(itr2->item_guid);
            }
        }
        draft.AddMoney(m->money).SendReturnToSender(GetAccountId(), m->receiver, m->sender, trans);
    }

    CharacterDatabase.CommitTransaction(trans);

    delete m;                                               //we can deallocate old mail
    player->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_OK);
}

//called when player takes item attached in mail
void WorldSession::HandleMailTakeItem(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;
    uint32 itemId;

    recvData >> mailId;
    recvData >> itemId;

    mailbox[3] = recvData.ReadBit();
    mailbox[2] = recvData.ReadBit();
    mailbox[0] = recvData.ReadBit();
    mailbox[5] = recvData.ReadBit();
    mailbox[6] = recvData.ReadBit();
    mailbox[1] = recvData.ReadBit();
    mailbox[7] = recvData.ReadBit();
    mailbox[4] = recvData.ReadBit();

    recvData.ReadByteSeq(mailbox[3]);
    recvData.ReadByteSeq(mailbox[2]);
    recvData.ReadByteSeq(mailbox[0]);
    recvData.ReadByteSeq(mailbox[5]);
    recvData.ReadByteSeq(mailbox[6]);
    recvData.ReadByteSeq(mailbox[7]);
    recvData.ReadByteSeq(mailbox[1]);
    recvData.ReadByteSeq(mailbox[4]);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if (!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    // prevent cheating with skip client money check
    if (!player->HasEnoughMoney(uint64(m->COD)))
    {
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Item* it = player->GetMItem(itemId);

    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, it, false);
    if (msg == EQUIP_ERR_OK)
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        m->RemoveItem(itemId);
        m->removedItems.push_back(itemId);

        if (m->COD > 0)                                     //if there is COD, take COD money from player and send them to sender by mail
        {
            uint64 sender_guid = MAKE_NEW_GUID(m->sender, 0, HIGHGUID_PLAYER);
            Player* receiver = ObjectAccessor::FindPlayer(sender_guid);

            uint32 sender_accId = 0;

            if (HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
            {
                std::string sender_name;
                if (receiver)
                {
                    sender_accId = receiver->GetSession()->GetAccountId();
                    sender_name = receiver->GetName();
                }
                else
                {
                    // can be calculated early
                    sender_accId = sObjectMgr->GetPlayerAccountIdByGUID(sender_guid);

                    if (!sObjectMgr->GetPlayerNameByGUID(sender_guid, sender_name))
                        sender_name = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);
                }
                sLog->outCommand(GetAccountId(), "GM %s (Account: %u) receiver mail item: %s (Entry: %u Count: %u) and send COD money: " UI64FMTD " to player: %s (Account: %u)",
                    GetPlayerName().c_str(), GetAccountId(), it->GetTemplate()->Name1.c_str(), it->GetEntry(), it->GetCount(), m->COD, sender_name.c_str(), sender_accId);
            }
            else if (!receiver)
                sender_accId = sObjectMgr->GetPlayerAccountIdByGUID(sender_guid);

            // check player existence
            if (receiver || sender_accId)
            {
                MailDraft(m->subject, "")
                    .AddMoney(m->COD)
                    .SendMailTo(trans, MailReceiver(receiver, m->sender), MailSender(MAIL_NORMAL, m->receiver), MAIL_CHECK_MASK_COD_PAYMENT);
            }

            player->ModifyMoney(-int32(m->COD));
        }
        m->COD = 0;
        m->state = MAIL_STATE_CHANGED;
        player->m_mailsUpdated = true;
        player->RemoveMItem(it->GetGUIDLow());

        uint32 count = it->GetCount();                      // save counts before store and possible merge with deleting
        it->SetState(ITEM_UNCHANGED);                       // need to set this state, otherwise item cannot be removed later, if neccessary
        player->MoveItemToInventory(dest, it, true);

        player->SaveInventoryAndGoldToDB(trans);
        player->_SaveMail(trans);
        CharacterDatabase.CommitTransaction(trans);

        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_OK, 0, itemId, count);
    }
    else
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_EQUIP_ERROR, msg);
}

void WorldSession::HandleMailTakeMoney(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint64 money;
    uint32 mailId;

    recvData >> money;
    recvData >> mailId;

    mailbox[3] = recvData.ReadBit();
    mailbox[5] = recvData.ReadBit();
    mailbox[4] = recvData.ReadBit();
    mailbox[2] = recvData.ReadBit();
    mailbox[6] = recvData.ReadBit();
    mailbox[1] = recvData.ReadBit();
    mailbox[0] = recvData.ReadBit();
    mailbox[7] = recvData.ReadBit();

    recvData.ReadByteSeq(mailbox[5]);
    recvData.ReadByteSeq(mailbox[6]);
    recvData.ReadByteSeq(mailbox[0]);
    recvData.ReadByteSeq(mailbox[2]);
    recvData.ReadByteSeq(mailbox[3]);
    recvData.ReadByteSeq(mailbox[7]);
    recvData.ReadByteSeq(mailbox[1]);
    recvData.ReadByteSeq(mailbox[4]);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if ((!m || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL)) ||
        (money > 0 && m->money != money))
    {
        player->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    if (!player->ModifyMoney(m->money, false))
    {
        player->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_TOO_MUCH_GOLD);
        return;
    }

    m->money = 0;
    m->state = MAIL_STATE_CHANGED;
    player->m_mailsUpdated = true;

    player->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_OK);

    // save money and mail to prevent cheating
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    player->SaveGoldToDB(trans);
    player->_SaveMail(trans);
    CharacterDatabase.CommitTransaction(trans);
}

//called when player lists his received mails
void WorldSession::HandleGetMailList(WorldPacket& recvData)
{
    ObjectGuid mailbox;

    mailbox[5] = recvData.ReadBit();
    mailbox[3] = recvData.ReadBit();
    mailbox[6] = recvData.ReadBit();
    mailbox[0] = recvData.ReadBit();
    mailbox[1] = recvData.ReadBit();
    mailbox[4] = recvData.ReadBit();
    mailbox[7] = recvData.ReadBit();
    mailbox[2] = recvData.ReadBit();

    recvData.ReadByteSeq(mailbox[4]);
    recvData.ReadByteSeq(mailbox[7]);
    recvData.ReadByteSeq(mailbox[3]);
    recvData.ReadByteSeq(mailbox[5]);
    recvData.ReadByteSeq(mailbox[6]);
    recvData.ReadByteSeq(mailbox[0]);
    recvData.ReadByteSeq(mailbox[1]);
    recvData.ReadByteSeq(mailbox[2]);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;

    //load players mails, and mailed items
    if (!player->m_mailsLoaded)
        player->_LoadMail();

    WorldPacket Packet(SMSG_MAIL_LIST_RESULT);           // guess size
    ByteBuffer ByteData;
    
    std::size_t TotalMailCount = GetPlayer()->GetMailSize();
    PlayerMails::iterator itr = player->GetMailBegin();
    
    if(TotalMailCount <= 50)
    {
        Packet.WriteBits(TotalMailCount, 18);
    }
    else
    {
        Packet.WriteBits(50, 18);
    }
    // the client displays max 50 messages, no point in sending any more until those are read and deleted
    
    for(std::size_t i = 0; i < 50; i++)
    {
        bool HasDword18 = false;
        bool HasDword1C = false;
        bool HasPlayerSender = false;
        bool HasNonPlayerSender = false;
        uint8 item_count = (*itr)->items.size();                // max count is MAX_MAIL_ITEMS (12)
        uint32 SenderEntry = 0;
        ObjectGuid PlayerSender = 0;
        
        switch ((*itr)->messageType)
        {
            case MAIL_NORMAL:                                       // sender guid
                PlayerSender = MAKE_NEW_GUID((*itr)->sender, 0, HIGHGUID_PLAYER);
                HasPlayerSender = true;
                break;
            case MAIL_CREATURE:
            case MAIL_GAMEOBJECT:
            case MAIL_AUCTION:
            case MAIL_CALENDAR:
                HasNonPlayerSender = true;
                SenderEntry = (*itr)->sender;                       // creature/gameobject entry, auction id, calendar event id?
                break;
        }
        
        Packet.WriteBit(HasNonPlayerSender);
        Packet.WriteBits((*itr)->subject.size(), 8);
        Packet.WriteBit(HasDword18);                                //HasDword18, unk for now
        Packet.WriteBit(HasPlayerSender);
        Packet.WriteBit(HasDword1C);                                //HasDword1C, unk for now
        
        if(HasPlayerSender)
        {
            Packet.WriteBit(PlayerSender[6]);
            Packet.WriteBit(PlayerSender[0]);
            Packet.WriteBit(PlayerSender[3]);
            Packet.WriteBit(PlayerSender[2]);
            Packet.WriteBit(PlayerSender[5]);
            Packet.WriteBit(PlayerSender[7]);
            Packet.WriteBit(PlayerSender[1]);
            Packet.WriteBit(PlayerSender[4]);
        }
        
        Packet.WriteBits((*itr)->body.size(), 13);
        Packet.WriteBits(item_count, 17);
        
        for (uint8 i = 0; i < item_count; ++i)
        {
            Packet.WriteBit(0);                                     // unk bit, related to a string O_O
        }
        
        //Item Part Is Here, skipping it for now
        for (uint8 i = 0; i < item_count; ++i)
        {
            Item* item = player->GetMItem((*itr)->items[i].item_guid);
            
            // stack count
            ByteData << uint32((item ? item->GetCount() : 0));
            // entry
            ByteData << uint32((item ? item->GetEntry() : 0));
            // can be negative
            ByteData << int32((item ? item->GetItemRandomPropertyId() : 0));
            // unk
            ByteData << uint32((item ? item->GetItemSuffixFactor() : 0));
            // item index (0-6?)
            ByteData << uint8(i);
            
            for (uint8 j = 0; j < MAX_INSPECTED_ENCHANTMENT_SLOT; ++j)
            {
                ByteData << uint32((item ? item->GetEnchantmentDuration((EnchantmentSlot)j) : 0));
                ByteData << uint32((item ? item->GetEnchantmentCharges((EnchantmentSlot)j) : 0));
                ByteData << uint32((item ? item->GetEnchantmentId((EnchantmentSlot)j) : 0));
            }
            
            // item guid low?
            ByteData << uint32((item ? item->GetGUIDLow() : 0));
            // max durability
            ByteData << uint32(0);                                              // Don't put max durability here, breaks sending or receiving mail as causes the 0 durability bug.
            // charges
            ByteData << uint32((item ? item->GetSpellCharges() : 0));
            // this string is new to mop, for now empty
            ByteData << uint32(0);                                              // after it put the string
            // durability
            ByteData << uint32((item ? item->GetUInt32Value(ITEM_FIELD_DURABILITY) : 0));
        }
        
        ByteData << uint64((*itr)->money);                          // Gold
        
        if(HasDword18)
            ByteData << uint32(0);                                  // dword18
        
        ByteData << uint32((*itr)->mailTemplateId);
        
        if(HasPlayerSender)
        {
            ByteData.WriteByteSeq(PlayerSender[7]);
            ByteData.WriteByteSeq(PlayerSender[0]);
            ByteData.WriteByteSeq(PlayerSender[6]);
            ByteData.WriteByteSeq(PlayerSender[5]);
            ByteData.WriteByteSeq(PlayerSender[4]);
            ByteData.WriteByteSeq(PlayerSender[2]);
            ByteData.WriteByteSeq(PlayerSender[3]);
            ByteData.WriteByteSeq(PlayerSender[1]);
        }
        
        ByteData << uint8((*itr)->messageType);
        ByteData.WriteString((*itr)->subject);
        ByteData << uint32(0);                                      // PackageID, from Package.dbc or smth like that
        ByteData << uint32((*itr)->stationery);
        ByteData << uint32((*itr)->checked);
        
        if(HasDword1C)
            ByteData << uint32(0);                                  // dword1C
        
        ByteData << float(float((*itr)->expire_time-time(NULL))/DAY);
        ByteData.WriteString((*itr)->body);
        
        if(HasNonPlayerSender)
            ByteData << SenderEntry;
        
        ByteData << uint64((*itr++)->COD);
    }
    
    Packet.FlushBits();
    Packet.append(ByteData);
    
    Packet << uint32(TotalMailCount);
    
    SendPacket(&Packet);

    // recalculate m_nextMailDelivereTime and unReadMails
    _player->UpdateNextMailTimeAndUnreads();
}

//used when player copies mail body to his inventory
void WorldSession::HandleMailCreateTextItem(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;

    recvData >> mailId;

    mailbox[3] = recvData.ReadBit();
    mailbox[1] = recvData.ReadBit();
    mailbox[0] = recvData.ReadBit();
    mailbox[7] = recvData.ReadBit();
    mailbox[4] = recvData.ReadBit();
    mailbox[5] = recvData.ReadBit();
    mailbox[2] = recvData.ReadBit();
    mailbox[6] = recvData.ReadBit();

    recvData.ReadByteSeq(mailbox[7]);
    recvData.ReadByteSeq(mailbox[0]);
    recvData.ReadByteSeq(mailbox[4]);
    recvData.ReadByteSeq(mailbox[6]);
    recvData.ReadByteSeq(mailbox[5]);
    recvData.ReadByteSeq(mailbox[2]);
    recvData.ReadByteSeq(mailbox[3]);
    recvData.ReadByteSeq(mailbox[1]);

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(mailbox, GAMEOBJECT_TYPE_MAILBOX))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if (!m || (m->body.empty() && !m->mailTemplateId) || m->state == MAIL_STATE_DELETED || m->deliver_time > time(NULL))
    {
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    Item* bodyItem = new Item;                              // This is not bag and then can be used new Item.
    if (!bodyItem->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_ITEM), MAIL_BODY_ITEM_TEMPLATE, player))
    {
        delete bodyItem;
        return;
    }

    // in mail template case we need create new item text
    if (m->mailTemplateId)
    {
        MailTemplateEntry const* mailTemplateEntry = sMailTemplateStore.LookupEntry(m->mailTemplateId);
        if (!mailTemplateEntry)
        {
            player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        bodyItem->SetText(mailTemplateEntry->content);
    }
    else
        bodyItem->SetText(m->body);

    bodyItem->SetUInt32Value(ITEM_FIELD_CREATOR, m->sender);
    bodyItem->SetFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_MAIL_TEXT_MASK);

    TC_LOG_INFO("network", "HandleMailCreateTextItem mailid=%u", mailId);

    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, bodyItem, false);
    if (msg == EQUIP_ERR_OK)
    {
        m->checked = m->checked | MAIL_CHECK_MASK_COPIED;
        m->state = MAIL_STATE_CHANGED;
        player->m_mailsUpdated = true;

        player->StoreItem(dest, bodyItem, true);
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_OK);
    }
    else
    {
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_EQUIP_ERROR, msg);
        delete bodyItem;
    }
}

/// @todo Fix me! ... this void has probably bad condition, but good data are sent
void WorldSession::HandleQueryNextMailTime(WorldPacket& /*recvData*/)
{
    WorldPacket data(SMSG_NEXT_MAIL_TIME_RESPONSE);

    if (!_player->m_mailsLoaded)
        _player->_LoadMail();

    if (_player->unReadMails > 0)
    {
        ByteBuffer BytePart;
        data << float(1);                                  // float
        data.WriteBits(_player->GetMailSize(), 20);

        time_t now = time(NULL);
        std::set<uint32> sentSenders;
        for (PlayerMails::iterator itr = _player->GetMailBegin(); itr != _player->GetMailEnd(); ++itr)
        {
            Mail* m = (*itr);

            // must be not checked yet
            if (m->checked & MAIL_CHECK_MASK_READ)
                continue;

            // and already delivered
            if (now < m->deliver_time)
                continue;

            // only send each mail sender once
            if (sentSenders.count(m->sender))
                continue;

            bool HasDword8 = true;
            bool HasDwordC = true;
            ObjectGuid Sender = m->sender;

            data.WriteBit(Sender[4]);
            data.WriteBit(Sender[2]);
            data.WriteBit(HasDword8);            //has dword8
            data.WriteBit(HasDwordC);            //has dwordC
            data.WriteBit(Sender[1]);
            data.WriteBit(Sender[3]);
            data.WriteBit(Sender[6]);
            data.WriteBit(Sender[5]);
            data.WriteBit(Sender[0]);
            data.WriteBit(Sender[7]);


            BytePart.WriteByteSeq(Sender[6]);

            if (HasDwordC)
                BytePart << uint32(GetMSTimeDiffToNow(now));            //this is some time, the value is the same as ConstantTime in SMSG_MESSAGECHAT

            BytePart.WriteByteSeq(Sender[4]);
            BytePart.WriteByteSeq(Sender[2]);
            BytePart.WriteByteSeq(Sender[0]);
            BytePart << float(m->deliver_time - now);
            BytePart << uint32(m->messageType);
            BytePart.WriteByteSeq(Sender[1]);
            BytePart.WriteByteSeq(Sender[7]);
            BytePart << uint8(0);                   //unk
            BytePart << uint32(m->stationery);

            if (HasDword8)
                BytePart << uint32(GetMSTimeDiffToNow(now));            //this is some time, the value is the same as ConstantTime in SMSG_MESSAGECHAT

            BytePart.WriteByteSeq(Sender[3]);
            BytePart.WriteByteSeq(Sender[5]);

            //BytePart << uint32(m->messageType != MAIL_NORMAL ? m->sender : 0);  // non-player entries
            sentSenders.insert(m->sender);
        }
        data.FlushBits();
        data.append(BytePart);
    }
    else
    {
        data << float(-1);
        data.WriteBits(0, 20);
        data.FlushBits();
    }
    SendPacket(&data);
}
