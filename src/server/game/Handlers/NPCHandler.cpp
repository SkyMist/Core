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

#include "Common.h"
#include "Language.h"
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "UpdateMask.h"
#include "ObjectAccessor.h"
#include "Creature.h"
#include "Pet.h"
#include "PetDefines.h"
#include "ReputationMgr.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "ScriptMgr.h"
#include "CreatureAI.h"
#include "SpellInfo.h"

void WorldSession::HandleTabardVendorActivateOpcode(WorldPacket& recvData)
{
    uint64 guid;
    recvData >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TABARDDESIGNER);
    if (!unit)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleTabardVendorActivateOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendTabardVendorActivate(guid);
}

void WorldSession::SendTabardVendorActivate(uint64 guid)
{
    WorldPacket data(MSG_TABARDVENDOR_ACTIVATE, 8);
    data << guid;
    SendPacket(&data);
}

void WorldSession::HandleBankerActivateOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_BANKER_ACTIVATE");

    guid[3] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[5]);

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_BANKER);
    if (!unit)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleBankerActivateOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendShowBank(guid);
}

void WorldSession::SendShowBank(ObjectGuid guid)
{
    WorldPacket data(SMSG_SHOW_BANK, 1 + 8);

    data.WriteBit(guid[7]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[2]);

    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[2]);

    SendPacket(&data);
}

void WorldSession::HandleTrainerListOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;

    guid[2] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[5]);

    SendTrainerList(guid);
}

void WorldSession::SendTrainerList(uint64 guid)
{
    std::string str = GetTrinityString(LANG_NPC_TAINER_HELLO);
    SendTrainerList(guid, str);
}

void WorldSession::SendTrainerList(uint64 guid, const std::string& strTitle)
{
    TC_LOG_DEBUG("network", "WORLD: SendTrainerList");

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        TC_LOG_DEBUG("network", "WORLD: SendTrainerList - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // trainer list loaded at check;
    if (!unit->isCanTrainingOf(_player, true))
        return;

    CreatureTemplate const* ci = unit->GetCreatureTemplate();

    if (!ci)
    {
        TC_LOG_DEBUG("network", "WORLD: SendTrainerList - (GUID: %u) NO CREATUREINFO!", GUID_LOPART(guid));
        return;
    }

    TrainerSpellData const* trainer_spells = unit->GetTrainerSpells();
    if (!trainer_spells)
    {
        TC_LOG_DEBUG("network", "WORLD: SendTrainerList - Training spells not found for creature (GUID: %u Entry: %u)",
            GUID_LOPART(guid), unit->GetEntry());
        return;
    }

    ObjectGuid oGuid = guid;

    WorldPacket data(SMSG_TRAINER_LIST, 8+4+4+trainer_spells->spellList.size()*38 + strTitle.size()+1);

    size_t count_pos = data.wpos();
    data.WriteBits(trainer_spells->spellList.size(), 19);

    data.WriteBit(oGuid[3]);
    data.WriteBit(oGuid[2]);
    data.WriteBit(oGuid[0]);
    data.WriteBit(oGuid[7]);
    data.WriteBit(oGuid[1]);
    data.WriteBit(oGuid[5]);
    data.WriteBits(strTitle.size(), 11);
    data.WriteBit(oGuid[6]);
    data.WriteBit(oGuid[4]);
    data.FlushBits();

    data.WriteByteSeq(oGuid[3]);

    // reputation discount
    float fDiscountMod = _player->GetReputationPriceDiscount(unit);
    bool can_learn_primary_prof = GetPlayer()->GetFreePrimaryProfessionPoints() > 0;

    uint32 count = 0;
    for (TrainerSpellMap::const_iterator itr = trainer_spells->spellList.begin(); itr != trainer_spells->spellList.end(); ++itr)
    {
        TrainerSpell const* tSpell = &itr->second;

        bool valid = true;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (!tSpell->learnedSpell[i])
                continue;
            if (!_player->IsSpellFitByClassAndRace(tSpell->learnedSpell[i]))
            {
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;

        TrainerSpellState state = _player->GetTrainerSpellState(tSpell);

        data << uint8(state == TRAINER_SPELL_GREEN_DISABLED ? TRAINER_SPELL_GREEN : state);
        data << uint32(tSpell->spell);                      // learned spell (or cast-spell in profession case)
        data << uint32(tSpell->reqSkill);
        data << uint32(floor(tSpell->spellCost * fDiscountMod));

        // spells required (3 max)
        uint8 maxReq = 0;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (!tSpell->learnedSpell[i])
                continue;
            SpellsRequiringSpellMapBounds spellsRequired = sSpellMgr->GetSpellsRequiredForSpellBounds(tSpell->learnedSpell[i]);
            for (SpellsRequiringSpellMap::const_iterator itr2 = spellsRequired.first; itr2 != spellsRequired.second && maxReq < 3; ++itr2)
            {
                data << uint32(itr2->second);
                ++maxReq;
            }
            if (maxReq == 3)
                break;
        }
        while (maxReq < 3)
        {
            data << uint32(0);
            ++maxReq;
        }

        data << uint8(tSpell->reqLevel);
        data << uint32(tSpell->reqSkillValue);

        ++count;
    }

    data.WriteByteSeq(oGuid[1]);
    data.WriteByteSeq(oGuid[6]);
    data.WriteByteSeq(oGuid[0]);

    data.WriteString(strTitle);
    data << uint32(trainer_spells->trainerType);

    data.WriteByteSeq(oGuid[2]);
    data.WriteByteSeq(oGuid[4]);
    data.WriteByteSeq(oGuid[5]);
    data.WriteByteSeq(oGuid[7]);

    data << uint32(1); // different value for each trainer, also found in CMSG_TRAINER_BUY_SPELL

    data.PutBits(count_pos, count, 19);
    SendPacket(&data);
}

void WorldSession::HandleTrainerBuySpellOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 spellId;
    uint32 trainerId;

    recvData >> trainerId >> spellId;

    guid[6] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[5]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[3]);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_TRAINER_BUY_SPELL NpcGUID=%u, learn spell id is: %u", uint32(GUID_LOPART(guid)), spellId);

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleTrainerBuySpellOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // check trainer class conditions
    if (!unit->isCanTrainingOf(_player, true))
    {
        SendTrainerBuyFailed(guid, spellId, 0);
        return;
    }

    // check present spell in trainer spell list
    TrainerSpellData const* trainer_spells = unit->GetTrainerSpells();
    if (!trainer_spells)
    {
        SendTrainerBuyFailed(guid, spellId, 0);
        return;
    }

    // not found, cheat?
    TrainerSpell const* trainer_spell = trainer_spells->Find(spellId);
    if (!trainer_spell)
    {
        SendTrainerBuyFailed(guid, spellId, 0);
        return;
    }

    // can't be learn, cheat? Or double learn with lags...
    if (_player->GetTrainerSpellState(trainer_spell) != TRAINER_SPELL_GREEN)
    {
        SendTrainerBuyFailed(guid, spellId, 0);
        return;
    }

    // apply reputation discount
    uint32 nSpellCost = uint32(floor(trainer_spell->spellCost * _player->GetReputationPriceDiscount(unit)));

    // check money requirement
    if (!_player->HasEnoughMoney(uint64(nSpellCost)))
    {
        SendTrainerBuyFailed(guid, spellId, 1);
        return;
    }

    _player->ModifyMoney(-int64(nSpellCost));

    unit->SendPlaySpellVisualKit(179, 0);       // 53 SpellCastDirected
    _player->SendPlaySpellVisualKit(362, 1);    // 113 EmoteSalute

    // learn explicitly or cast explicitly
    if (trainer_spell->IsCastable())
        _player->CastSpell(_player, trainer_spell->spell, true);
    else
        _player->learnSpell(spellId, false);

    WorldPacket data(SMSG_TRAINER_BUY_SUCCEEDED, 12);
    data << uint64(guid);
    data << uint32(spellId);
    SendPacket(&data);
}

void WorldSession::SendTrainerBuyFailed(uint64 guid, uint32 spellId, uint32 reason)
{
    WorldPacket data(SMSG_TRAINER_BUY_FAILED, 16);
    data << uint64(guid);
    data << uint32(spellId);        // should be same as in packet from client
    data << uint32(reason);         // 1 == "Not enough money for trainer service." 0 == "Trainer service %d unavailable."
    SendPacket(&data);
}

void WorldSession::HandleGossipHelloOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_GOSSIP_HELLO");

    ObjectGuid guid;
    
    guid[6] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[5]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[3]);

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!unit)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleGossipHelloOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    // set faction visible if needed
    if (FactionTemplateEntry const* factionTemplateEntry = sFactionTemplateStore.LookupEntry(unit->getFaction()))
        _player->GetReputationMgr().SetVisible(factionTemplateEntry);

    GetPlayer()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TALK);
    // remove fake death
    //if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
    //    GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    if (unit->IsArmorer() || unit->IsCivilian() || unit->IsQuestGiver() || unit->IsServiceProvider() || unit->IsGuard())
        unit->StopMoving();

    // If spiritguide, no need for gossip menu, just put player into resurrect queue
    if (unit->IsSpiritGuide())
    {
        Battleground* bg = _player->GetBattleground();
        if (bg)
        {
            bg->AddPlayerToResurrectQueue(unit->GetGUID(), _player->GetGUID());
            sBattlegroundMgr->SendAreaSpiritHealerQueryOpcode(_player, bg, unit->GetGUID());
            return;
        }
    }

    if (!sScriptMgr->OnGossipHello(_player, unit))
    {
//        _player->TalkedToCreature(unit->GetEntry(), unit->GetGUID());
        _player->PrepareGossipMenu(unit, unit->GetCreatureTemplate()->GossipMenuId, true);
        _player->SendPreparedGossip(unit);
    }
    unit->AI()->sGossipHello(_player);
}

void WorldSession::HandleSpiritHealerActivateOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_SPIRIT_HEALER_ACTIVATE");

    uint64 guid;

    recvData >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_SPIRITHEALER);
    if (!unit)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleSpiritHealerActivateOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendSpiritResurrect();
}

void WorldSession::SendSpiritResurrect()
{
    _player->ResurrectPlayer(0.5f, true);

    _player->DurabilityLossAll(0.25f, true);

    // get corpse nearest graveyard
    WorldSafeLocsEntry const* corpseGrave = NULL;
    Corpse* corpse = _player->GetCorpse();
    if (corpse)
        corpseGrave = sObjectMgr->GetClosestGraveYard(
            corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetMapId(), _player->GetTeam());

    // now can spawn bones
    _player->SpawnCorpseBones();

    // teleport to nearest from corpse graveyard, if different from nearest to player ghost
    if (corpseGrave)
    {
        WorldSafeLocsEntry const* ghostGrave = sObjectMgr->GetClosestGraveYard(
            _player->GetPositionX(), _player->GetPositionY(), _player->GetPositionZ(), _player->GetMapId(), _player->GetTeam());

        if (corpseGrave != ghostGrave)
            _player->TeleportTo(corpseGrave->map_id, corpseGrave->x, corpseGrave->y, corpseGrave->z, _player->GetOrientation());
        // or update at original position
        else
            _player->UpdateObjectVisibility();
    }
    // or update at original position
    else
        _player->UpdateObjectVisibility();
}

void WorldSession::HandleBinderActivateOpcode(WorldPacket& recvData)
{
    ObjectGuid npcGuid;

    npcGuid[5] = recvData.ReadBit();
    npcGuid[4] = recvData.ReadBit();
    npcGuid[0] = recvData.ReadBit();
    npcGuid[7] = recvData.ReadBit();
    npcGuid[2] = recvData.ReadBit();
    npcGuid[3] = recvData.ReadBit();
    npcGuid[6] = recvData.ReadBit();
    npcGuid[1] = recvData.ReadBit();

    recvData.ReadByteSeq(npcGuid[6]);
    recvData.ReadByteSeq(npcGuid[3]);
    recvData.ReadByteSeq(npcGuid[7]);
    recvData.ReadByteSeq(npcGuid[4]);
    recvData.ReadByteSeq(npcGuid[5]);
    recvData.ReadByteSeq(npcGuid[0]);
    recvData.ReadByteSeq(npcGuid[1]);
    recvData.ReadByteSeq(npcGuid[2]);

    if (!GetPlayer()->IsInWorld() || !GetPlayer()->IsAlive())
        return;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_INNKEEPER);
    if (!unit)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleBinderActivateOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(npcGuid)));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendBindPoint(unit);
}

void WorldSession::SendBindPoint(Creature* npc)
{
    // prevent set homebind to instances in any case
    if (GetPlayer()->GetMap()->Instanceable())
        return;

    uint32 bindspell = 3286;

    // send spell for homebinding (3286)
    npc->CastSpell(_player, bindspell, true);

    WorldPacket data(SMSG_TRAINER_BUY_SUCCEEDED, 12);
    data << uint64(npc->GetGUID());
    data << uint32(bindspell);
    SendPacket(&data);

    _player->PlayerTalkClass->SendCloseGossip();
}

void WorldSession::HandleListStabledPetsOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Recv CMSG_LIST_STABLED_PETS");
    uint64 npcGUID;

    recvData >> npcGUID;

    if (!CheckStableMaster(npcGUID))
        return;

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // remove mounts this fix bug where getting pet from stable while mounted deletes pet.
    if (GetPlayer()->IsMounted())
        GetPlayer()->RemoveAurasByType(SPELL_AURA_MOUNTED);

    SendStablePet(npcGUID);
}

void WorldSession::SendStablePet(uint64 guid)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_SLOTS_DETAIL);

    stmt->setUInt32(0, _player->GetGUIDLow());
    stmt->setUInt8(1, PET_SLOT_HUNTER_FIRST);
    stmt->setUInt8(2, PET_SLOT_STABLE_LAST);

    _sendStabledPetCallback.SetParam(guid);
    _sendStabledPetCallback.SetFutureResult(CharacterDatabase.AsyncQuery(stmt));
}

void WorldSession::SendStablePetCallback(PreparedQueryResult result, uint64 guid)
{
    if (!GetPlayer())
        return;

    TC_LOG_DEBUG("network", "WORLD: Sent SMSG_LIST_STABLED_PETS.");

    WorldPacket data(SMSG_LIST_STABLED_PETS, 200);           // guessed size
    ObjectGuid npcGuid = guid;
    ByteBuffer dataBuffer;

    data.WriteBit(npcGuid[6]);
    data.WriteBit(npcGuid[1]);
    data.WriteBit(npcGuid[5]);
    data.WriteBit(npcGuid[4]);
    data.WriteBit(npcGuid[2]);
    data.WriteBits(result ? result->GetRowCount() : 0, 19);
    data.WriteBit(npcGuid[7]);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            std::string name = fields[5].GetString();

            data.WriteBits(name.size(), 8);

            dataBuffer << uint32(fields[3].GetUInt32());          // creature entry
            dataBuffer << uint32(fields[2].GetUInt32());          // petnumber
            dataBuffer << uint32(fields[4].GetUInt16());          // level
            dataBuffer << uint8(fields[1].GetUInt8() < uint8(PET_SLOT_STABLE_FIRST) ? 1 : 3);       // 1 = current, 2/3 = in stable (any from 4, 5, ... create problems with proper show)

            uint32 modelId = 0;

            if (CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(fields[3].GetUInt32()))
                modelId = cInfo->Modelid1 ? cInfo->Modelid1 : cInfo->Modelid2;

            dataBuffer << uint32(fields[1].GetUInt8());           // slot
            dataBuffer << uint32(modelId);                        // creature modelid

            if (name.size())
                dataBuffer.append(name.c_str(), name.size());
        }
        while (result->NextRow());
    }

    data.WriteBit(npcGuid[3]);
    data.WriteBit(npcGuid[0]);
    data.FlushBits();

    if (dataBuffer.size())
        data.append(dataBuffer);

    data.WriteByteSeq(npcGuid[2]);
    data.WriteByteSeq(npcGuid[0]);
    data.WriteByteSeq(npcGuid[6]);
    data.WriteByteSeq(npcGuid[1]);
    data.WriteByteSeq(npcGuid[7]);
    data.WriteByteSeq(npcGuid[5]);
    data.WriteByteSeq(npcGuid[3]);
    data.WriteByteSeq(npcGuid[4]);

    SendPacket(&data);
}

void WorldSession::SendStableResult(uint8 res)
{
    WorldPacket data(SMSG_STABLE_RESULT, 1);
    data << uint8(res);
    SendPacket(&data);
}

void WorldSession::HandleStableSetPetSlot(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Recv CMSG_SET_PET_SLOT.");

    ObjectGuid npcGuid;
    uint32 pet_number;
    uint8 new_slot;

    recvData >> pet_number >> new_slot;

    npcGuid[3] = recvData.ReadBit();
    npcGuid[2] = recvData.ReadBit();
    npcGuid[4] = recvData.ReadBit();
    npcGuid[6] = recvData.ReadBit();
    npcGuid[0] = recvData.ReadBit();
    npcGuid[1] = recvData.ReadBit();
    npcGuid[7] = recvData.ReadBit();
    npcGuid[5] = recvData.ReadBit();

    recvData.ReadByteSeq(npcGuid[5]);
    recvData.ReadByteSeq(npcGuid[3]);
    recvData.ReadByteSeq(npcGuid[2]);
    recvData.ReadByteSeq(npcGuid[0]);
    recvData.ReadByteSeq(npcGuid[1]);
    recvData.ReadByteSeq(npcGuid[4]);
    recvData.ReadByteSeq(npcGuid[7]);
    recvData.ReadByteSeq(npcGuid[6]);

    if (!CheckStableMaster(npcGuid))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    if (new_slot > MAX_PET_STABLES)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    Pet* pet = _player->GetPet();

    // If we move the pet already summoned...
    if (pet && pet->GetCharmInfo() && pet->GetCharmInfo()->GetPetNumber() == pet_number)
        _player->RemovePet(pet, PET_SLOT_ACTUAL_PET_SLOT, false, pet->m_Stampeded);

    // If we move to the pet already summoned...
    if (pet && GetPlayer()->m_currentPetSlot == new_slot)
        _player->RemovePet(pet, PET_SLOT_ACTUAL_PET_SLOT, false, pet->m_Stampeded);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_SLOT_BY_ID);

    stmt->setUInt32(0, _player->GetGUIDLow());
    stmt->setUInt32(1, pet_number);

    _setPetSlotCallback.SetParam(new_slot);
    _setPetSlotCallback.SetFutureResult(CharacterDatabase.AsyncQuery(stmt));
}

void WorldSession::HandleStableSetPetSlotCallback(PreparedQueryResult result, uint32 petId)
{
    if (!GetPlayer())
        return;

    if (!result)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    Field* fields = result->Fetch();

    uint32 slot         = fields[0].GetUInt8();
    uint32 petEntry     = fields[1].GetUInt32();
    uint32 pet_number   = fields[2].GetUInt32();

    if (!petEntry)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(petEntry);
    if (!creatureInfo || !creatureInfo->IsTameable(_player->CanTameExoticPets()))
    {
        // If we try to stable exotic pets.
        if (creatureInfo && creatureInfo->IsTameable(true))
            SendStableResult(STABLE_ERR_EXOTIC);
        else
            SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PET_DATA_OWNER);
    {
        stmt->setUInt32(0, petId);
        stmt->setUInt32(1, slot);
        stmt->setUInt32(2, GetPlayer()->GetGUIDLow());

        trans->Append(stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PET_DATA_OWNER_ID);
    {
        stmt->setUInt32(0, slot);
        stmt->setUInt32(1, petId);
        stmt->setUInt32(2, GetPlayer()->GetGUIDLow());
        stmt->setUInt32(3, pet_number);

        trans->Append(stmt);
    }

    CharacterDatabase.CommitTransaction(trans);

    if (petId != 100)
    {
        // We need to remove and add the new pet to a different slot
        // GetPlayer()->setPetSlotUsed((PetSlot)slot, false);
        _player->setPetSlotUsed((PetSlot)slot, false);
        _player->setPetSlotUsed((PetSlot)petId, true);
        SendStableResult(STABLE_SUCCESS_UNSTABLE);
    }
    else
        SendStableResult(STABLE_ERR_STABLE);
}

void WorldSession::HandleRepairItemOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_REPAIR_ITEM");

    ObjectGuid npcGUID, itemGUID;
    bool guildBank; // new in 2.3.2, bool that means from guild bank money

    npcGUID[3] = recvData.ReadBit();
    itemGUID[3] = recvData.ReadBit();
    itemGUID[6] = recvData.ReadBit();
    guildBank = recvData.ReadBit();
    npcGUID[2] = recvData.ReadBit();
    itemGUID[0] = recvData.ReadBit();
    npcGUID[7] = recvData.ReadBit();
    npcGUID[6] = recvData.ReadBit();
    npcGUID[0] = recvData.ReadBit();
    itemGUID[7] = recvData.ReadBit();
    itemGUID[4] = recvData.ReadBit();
    npcGUID[5] = recvData.ReadBit();
    itemGUID[5] = recvData.ReadBit();
    npcGUID[4] = recvData.ReadBit();
    itemGUID[2] = recvData.ReadBit();
    npcGUID[1] = recvData.ReadBit();
    itemGUID[1] = recvData.ReadBit();

    recvData.ReadByteSeq(itemGUID[6]);
    recvData.ReadByteSeq(npcGUID[1]);
    recvData.ReadByteSeq(npcGUID[0]);
    recvData.ReadByteSeq(npcGUID[2]);
    recvData.ReadByteSeq(itemGUID[2]);
    recvData.ReadByteSeq(itemGUID[5]);
    recvData.ReadByteSeq(itemGUID[1]);
    recvData.ReadByteSeq(npcGUID[6]);
    recvData.ReadByteSeq(npcGUID[7]);
    recvData.ReadByteSeq(itemGUID[3]);
    recvData.ReadByteSeq(itemGUID[7]);
    recvData.ReadByteSeq(npcGUID[3]);
    recvData.ReadByteSeq(npcGUID[4]);
    recvData.ReadByteSeq(npcGUID[5]);
    recvData.ReadByteSeq(itemGUID[0]);
    recvData.ReadByteSeq(itemGUID[4]);

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(npcGUID, UNIT_NPC_FLAG_REPAIR);
    if (!unit)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleRepairItemOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(npcGUID)));
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // reputation discount
    float discountMod = _player->GetReputationPriceDiscount(unit);

    if (itemGUID)
    {
        TC_LOG_DEBUG("network", "ITEM: Repair item, itemGUID = %u, npcGUID = %u", GUID_LOPART(itemGUID), GUID_LOPART(npcGUID));

        Item* item = _player->GetItemByGuid(itemGUID);
        if (item)
            _player->DurabilityRepair(item->GetPos(), true, discountMod, guildBank);
    }
    else
    {
        TC_LOG_DEBUG("network", "ITEM: Repair all items, npcGUID = %u", GUID_LOPART(npcGUID));
        _player->DurabilityRepairAll(true, discountMod, guildBank);
    }
}

