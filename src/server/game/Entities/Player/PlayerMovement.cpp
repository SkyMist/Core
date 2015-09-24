/*
 * Copyright (C) 2011-2015 SkyMist Gaming
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

#include "Player.h"
#include "PlayerMovement.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "Arena.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "BattlefieldWG.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "CellImpl.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "CharacterDatabaseCleaner.h"
#include "Chat.h"
#include "Common.h"
#include "ConditionMgr.h"
#include "CreatureAI.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "DisableMgr.h"
#include "Formulas.h"
#include "GameEventMgr.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "LFGMgr.h"
#include "Language.h"
#include "Log.h"
#include "MapInstanced.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"
#include "Pet.h"
#include "QuestDef.h"
#include "ReputationMgr.h"
#include "SkillDiscovery.h"
#include "SocialMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Transport.h"
#include "UpdateData.h"
#include "UpdateFieldFlags.h"
#include "UpdateMask.h"
#include "Util.h"
#include "Vehicle.h"
#include "Weather.h"
#include "WeatherMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "MovementStructures.h"
#include "Config.h"
#include "GameObjectAI.h"

    /*** Movement functions - Handled by PlayerMovement. ***/

// Movement information.

void Player::ReadMovementInfo(WorldPacket& data, MovementInfo* mi, ExtraMovementStatusElement* extras)
{
    bool hasTransportData = false;
    bool hasSpline = false;

    uint32 bitcounterLoop = 0;

    MovementStatusElements const* sequence = GetMovementStatusElementsSequence(data.GetOpcode());
    if (sequence == NULL)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "WorldSession::ReadMovementInfo: No movement sequence found for opcode 0x%04X", uint32(data.GetOpcode()));
        return;
    }

    ObjectGuid guid;
    ObjectGuid tguid;

    for (; *sequence != MSEEnd; ++sequence)
    {
        MovementStatusElements const& element = *sequence;
        if (element == MSEEnd)
            break;

        if (element >= MSEHasGuidByte0 && element <= MSEHasGuidByte7)
        {
            guid[element - MSEHasGuidByte0] = data.ReadBit();
            continue;
        }

        if (element >= MSEHasTransportGuidByte0 &&
            element <= MSEHasTransportGuidByte7)
        {
            if (hasTransportData)
                tguid[element - MSEHasTransportGuidByte0] = data.ReadBit();
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            data.ReadByteSeq(guid[element - MSEGuidByte0]);
            continue;
        }

        if (element >= MSETransportGuidByte0 &&
            element <= MSETransportGuidByte7)
        {
            if (hasTransportData)
                data.ReadByteSeq(tguid[element - MSETransportGuidByte0]);
            continue;
        }

        if (element >= MSEGenericDword0 &&
            element <= MSEGenericDword7)
        {
            data.read_skip<uint32>();
            continue;
        }

        switch (element)
        {
            case MSEExtraElement:
                extras->ReadNextElement(data);
                break;
            case MSEGeneric2bits0:
                data.ReadBits(2);
                break;
            case MSEUnkUIntCount:
                bitcounterLoop = data.ReadBits(22);
                break;
            case MSEUnkUIntLoop:
                data.read_skip(bitcounterLoop * sizeof(uint32));
                break;
            case MSEFlushBits:
                data.FlushBits();
                break;
            case MSEHasMovementFlags:
                mi->flags = !data.ReadBit();
                break;
            case MSEHasMovementFlags2:
                mi->flags2 = !data.ReadBit();
                break;
            case MSEHasTimestamp:
                mi->time = !data.ReadBit();
                break;
            case MSEHasOrientation:
                mi->pos.m_orientation = !data.ReadBit() ? 1.0f : 0.0f;
                break;
            case MSEHasTransportData:
                hasTransportData = data.ReadBit();
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    mi->has_t_time2 = data.ReadBit();
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    mi->has_t_time3 = data.ReadBit();
                break;
            case MSEHasPitch:
                mi->HavePitch = !data.ReadBit();
                break;
            case MSEHasFallData:
                mi->hasFallData = data.ReadBit();
                break;
            case MSEHasFallDirection:
                if (mi->hasFallData)
                    mi->hasFallDirection = data.ReadBit();
                break;
            case MSEHasSplineElevation:
                mi->HaveSplineElevation = !data.ReadBit();
                break;
            case MSEHasSpline:
                hasSpline = data.ReadBit();
                break;
            case MSEMovementFlags:
                if (mi->flags)
                    mi->flags = data.ReadBits(30);
                break;
            case MSEMovementFlags2:
                if (mi->flags2)
                    mi->flags2 = data.ReadBits(13);
                break;
            case MSETimestamp:
                if (mi->time)
                    data >> mi->time;
                break;
            case MSEPositionX:
                data >> mi->pos.m_positionX;
                break;
            case MSEPositionY:
                data >> mi->pos.m_positionY;
                break;
            case MSEPositionZ:
                data >> mi->pos.m_positionZ;
                break;
            case MSEOrientation:
                if (mi->pos.m_orientation != 0.0f)
                    mi->pos.SetOrientation(data.read<float>());
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionX;
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionY;
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionZ;
                break;
            case MSETransportOrientation:
                if (hasTransportData)
                    mi->t_pos.SetOrientation(data.read<float>());
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data >> mi->t_seat;
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data >> mi->t_time;
                break;
            case MSETransportTime2:
                if (hasTransportData && mi->has_t_time2)
                    data >> mi->t_time2;
                break;
            case MSETransportTime3:
                if (hasTransportData && mi->has_t_time3)
                    data >> mi->t_time3;
                break;
            case MSEPitch:
                if (mi->HavePitch)
                    data >> mi->pitch;
                break;
            case MSEFallTime:
                if (mi->hasFallData)
                    data >> mi->fallTime;
                break;
            case MSEFallVerticalSpeed:
                if (mi->hasFallData)
                    data >> mi->j_zspeed;
                break;
            case MSEFallCosAngle:
                if (mi->hasFallData && mi->hasFallDirection)
                    data >> mi->j_cosAngle;
                break;
            case MSEFallSinAngle:
                if (mi->hasFallData && mi->hasFallDirection)
                    data >> mi->j_sinAngle;
                break;
            case MSEFallHorizontalSpeed:
                if (mi->hasFallData && mi->hasFallDirection)
                    data >> mi->j_xyspeed;
                break;
            case MSESplineElevation:
                if (mi->HaveSplineElevation)
                    data >> mi->splineElevation;
                break;
            case MSEZeroBit:
            case MSEOneBit:
                data.ReadBit();
                break;
            case MSEHasUnkTime:
                mi->Alive32 = !data.ReadBit();
                break;
            case MSEUnkTime:
                if (mi->Alive32)
                    data >> mi->Alive32;
                break;
            case MSECounter:
                data.read_skip<uint32>();
                break;
            default:
                ASSERT(false && "Incorrect sequence element detected at ReadMovementInfo");
                break;
        }
    }

    mi->guid = guid;
    mi->t_guid = tguid;

    if (hasTransportData && mi->pos.m_positionX != mi->t_pos.m_positionX)
       if (GetTransport())
           GetTransport()->UpdatePosition(mi);
}

void Player::UpdateFallInformationIfNeed(MovementInfo const& minfo, uint16 opcode)
{
    if (m_lastFallTime >= minfo.fallTime || m_lastFallZ <= minfo.pos.GetPositionZ() || opcode == CMSG_MOVE_FALL_LAND)
        SetFallInformation(minfo.fallTime, minfo.pos.GetPositionZ());
}

void Player::HandleFall(MovementInfo const& movementInfo)
{
    // Calculate total z distance of the fall.
    float z_diff = m_lastFallZ - movementInfo.pos.GetPositionZ();

    // Players with low fall distance, Feather Fall or physical immunity (charges used) are ignored. 14.57 can be calculated by resolving damageperc formula below to 0.
    if (z_diff >= 14.57f && !isDead() && !isGameMaster() &&
        !HasAuraType(SPELL_AURA_HOVER) && !HasAuraType(SPELL_AURA_FEATHER_FALL) &&
        !HasAuraType(SPELL_AURA_FLY) && !IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL) &&
        !(getClass() == CLASS_DEATH_KNIGHT && HasAura(59307) && HasAura(3714)))
    {
        // Safe fall, fall height reduction.
        int32 safe_fall = GetTotalAuraModifier(SPELL_AURA_SAFE_FALL);

        // Glyph of Safe Fall.
        if (HasAura(58033) && HasAura(1860))
            safe_fall += 10;

        float damageperc = 0.018f * (z_diff-safe_fall) - 0.2426f;

        if (damageperc > 0)
        {
            uint32 damage = (uint32)(damageperc * GetMaxHealth()*sWorld->getRate(RATE_DAMAGE_FALL));

            float height = movementInfo.pos.m_positionZ;
            UpdateGroundPositionZ(movementInfo.pos.m_positionX, movementInfo.pos.m_positionY, height);

            if (damage > 0)
            {
                // Prevent fall damage from being more than the player maximum health.
                if (damage > GetMaxHealth())
                    damage = GetMaxHealth();

                // Hack Gust of Wind.
                if (HasAura(43621))
                    damage = GetMaxHealth() / 2;

                uint32 original_health = GetHealth();
                uint32 final_damage = EnvironmentalDamage(DAMAGE_FALL, damage);

                // Recheck alive, might have died of EnvironmentalDamage, avoid cases when player die in fact like Spirit of Redemption case.
                if (isAlive() && final_damage < original_health)
                    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING, uint32(z_diff*100));
            }

            // Z given by moveinfo, LastZ, FallTime, WaterZ, MapZ, Damage, Safefall reduction.
            sLog->outDebug(LOG_FILTER_PLAYER, "FALLDAMAGE z = %f sz = %f pZ = %f FallTime = %d mZ = %f damage = %d SF = %d", movementInfo.pos.GetPositionZ(), height, GetPositionZ(), movementInfo.fallTime, height, damage, safe_fall);
        }
    }

    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_LANDING); // No fly zone - Parachute.
}

void Player::SendMovementSetCanTransitionBetweenSwimAndFly(bool apply)
{
    PacketSender(this, NULL_OPCODE, apply ? SMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY : SMSG_MOVE_UNSET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY).Send();
}

void Player::SendMovementSetCollisionHeight(float height)
{
    CreatureDisplayInfoEntry const* mountDisplayInfo = sCreatureDisplayInfoStore.LookupEntry(GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID));

    bool hasMountDisplayInfoScale = mountDisplayInfo ? true : false;
    float mountDisplayScale = GetObjectScale(); // hasMountDisplayInfoScale ? mountDisplayInfo->scale : 1.0f; but this causes scale issues (some mounts too big).

    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_MOVE_SET_COLLISION_HEIGHT, 2 + 8 + 4 + 4);

    data.WriteBit(!hasMountDisplayInfoScale); // mountDisplayInfo scale, inversed.

    data.WriteBit(guid[7]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[1]);

    data.WriteBits(3, 2); // Reason. 1 is for Mounts, 2 for Vehicles / mounts with vehicleId. This should NOT be 3.

    data.WriteBit(guid[2]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[4]);

    data.FlushBits();

    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[4]);    
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[2]);

    data << float(height);
    data << float(mountDisplayScale);
    data << uint32(sWorld->GetGameTime());  // Packet counter

    data.WriteByteSeq(guid[3]);

    if (hasMountDisplayInfoScale)
        data << uint32(mountDisplayInfo->Displayid);

    data.WriteByteSeq(guid[0]);

    SendDirectMessage(&data);

    // Send the update.
    static MovementStatusElements const heightElement = MSEExtraFloat;
    ExtraMovementStatusElement extra(&heightElement);
    extra.Data.floatData = height;

    PacketSender(this, NULL_OPCODE, NULL_OPCODE, SMSG_MOVE_UPDATE_COLLISION_HEIGHT, &extra).Send();
}

// Active mover set.

void Player::SetMover(Unit* target)
{
    m_mover->m_movedPlayer = NULL;
    m_mover = target;
    m_mover->m_movedPlayer = this;

    if (m_mover)
    {
        WorldPacket data(SMSG_MOVE_SET_ACTIVE_MOVER, 8 + 1);
        ObjectGuid guid = target->GetGUID();
    
        uint8 bitOrder[8] = { 1, 2, 4, 5, 3, 0, 7, 6 };
        data.WriteBitInOrder(guid, bitOrder);

        data.FlushBits();

        uint8 byteOrder[8] = { 4, 1, 5, 7, 3, 2, 0, 6 };
        data.WriteBytesSeq(guid, byteOrder);

        GetSession()->SendPacket(&data);
    }
}

// Teleportation.

bool Player::TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options)
{
    //sAnticheatMgr->DisableAnticheatDetection(this,true);

    if (!MapManager::IsValidMAP(mapid, false))
    {
        sLog->outError(LOG_FILTER_MAPS, "TeleportTo: invalid map (%d) given when teleporting player (GUID: %u, name: %s, map: %d, X: %f, Y: %f, Z: %f, O: %f).",
            mapid, GetGUIDLow(), GetName(), GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        return false;
    }

    if (!SkyMistCore::IsValidMapCoord(x, y, z, orientation))
    {
        sLog->outError(LOG_FILTER_MAPS, "TeleportTo: invalid coordinates (X: %f, Y: %f, Z: %f, O: %f) given when teleporting player (GUID: %u, name: %s, map: %d, X: %f, Y: %f, Z: %f, O: %f).",
            x, y, z, orientation, GetGUIDLow(), GetName(), GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        return false;
    }

    if (AccountMgr::IsPlayerAccount(GetSession()->GetSecurity()) && DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP, mapid, this))
    {
        sLog->outError(LOG_FILTER_MAPS, "Player (GUID: %u, name: %s) tried to enter a forbidden map %u", GetGUIDLow(), GetName(), mapid);
        SendTransferAborted(mapid, TRANSFER_ABORT_MAP_NOT_ALLOWED);
        return false;
    }

    // preparing unsummon pet if lost (we must get pet before teleportation or will not find it later)
    Pet* pet = GetPet();

    MapEntry const* mEntry = sMapStore.LookupEntry(mapid);

    // don't let enter battlegrounds without assigned battleground id (for example through areatrigger)...
    // don't let gm level > 1 either
    if (!InBattleground() && mEntry->IsBattlegroundOrArena())
        return false;

    // client without expansion support
    if (GetSession()->Expansion() < mEntry->Expansion())
    {
        sLog->outDebug(LOG_FILTER_MAPS, "Player %s using client without required expansion tried teleport to non accessible map %u", GetName(), mapid);

        if (GetTransport())
        {
            m_transport->RemovePassenger(this);
            m_transport = NULL;
            m_movementInfo.t_pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
            m_movementInfo.t_time = 0;
            m_movementInfo.t_seat = -1;
            RepopAtGraveyard();                             // teleport to near graveyard if on transport, looks blizz like :)
        }

        SendTransferAborted(mapid, TRANSFER_ABORT_INSUF_EXPAN_LVL, mEntry->Expansion());

        return false;                                       // normal client can't teleport to this map...
    }
    else
        sLog->outDebug(LOG_FILTER_MAPS, "Player %s is being teleported to map %u", GetName(), mapid);

    if (m_vehicle)
        ExitVehicle();

    // Reset movement flags at teleport, because player will continue move with these flags after teleport. Send just those that count.
    SetUnitMovementFlags(GetUnitMovementFlags() & MOVEMENTFLAG_MASK_HAS_PLAYER_STATUS_OPCODE);
    DisableSpline();

    // Clear unit emote state.
    ClearEmotes();

    if (m_transport)
    {
        if (!(options & TELE_TO_NOT_LEAVE_TRANSPORT))
        {
            m_transport->RemovePassenger(this);
            m_transport = NULL;
            m_movementInfo.t_pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
            m_movementInfo.t_time = 0;
            m_movementInfo.t_seat = -1;
            m_movementInfo.t_guid = 0;
        }
    }

    // The player was ported to another map and loses the duel immediately.
    // We have to perform this check before the teleport, otherwise the
    // ObjectAccessor won't find the flag.
    if (duel && GetMapId() != mapid && GetMap()->GetGameObject(GetUInt64Value(PLAYER_DUEL_ARBITER)))
        DuelComplete(DUEL_FLED);

    // Druids eclipse power == 0 after teleport, so we need to remove last eclipse power
    if (getClass() == CLASS_DRUID && getLevel() > 20 && GetSpecializationId(GetActiveSpec()) == SPEC_DRUID_BALANCE)
        RemoveLastEclipsePower();

    // On teleport monks healing sphere's dissapear, so we should remove aura counter
    if (getClass() == CLASS_MONK && GetSpecializationId(GetActiveSpec()) == SPEC_MONK_MISTWEAVER && HasAura(124458))
        RemoveAura(124458);


    if (GetMapId() == mapid)
    {
        //lets reset far teleport flag if it wasn't reset during chained teleports
        SetSemaphoreTeleportFar(false);
        //setup delayed teleport flag
        SetDelayedTeleportFlag(IsCanDelayTeleport());
        //if teleport spell is casted in Unit::Update() func
        //then we need to delay it until update process will be finished
        if (IsHasDelayedTeleport())
        {
            SetSemaphoreTeleportNear(true);
            //lets save teleport destination for player
            m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
            m_teleport_options = options;
            return true;
        }

        if (!(options & TELE_TO_NOT_UNSUMMON_PET))
        {
            //same map, only remove pet if out of range for new position
            if (pet && !pet->IsWithinDist3d(x, y, z, GetMap()->GetVisibilityRange()))
                UnsummonPetTemporaryIfAny();
        }

        if (!(options & TELE_TO_NOT_LEAVE_COMBAT))
            CombatStop();

        // this will be used instead of the current location in SaveToDB
        m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
        SetFallInformation(0, z);

        // code for finish transfer called in WorldSession::HandleMovementOpcodes()
        // at client packet CMSG_MOVE_TELEPORT_ACK
        SetSemaphoreTeleportNear(true);
        // near teleport, triggering send CMSG_MOVE_TELEPORT_ACK from client at landing
        if (!GetSession()->PlayerLogout())
        {
            Position oldPos;
            GetPosition(&oldPos);

            // Apply Hover Height.
            if (HasUnitMovementFlag(MOVEMENTFLAG_HOVER))
                z += GetFloatValue(UNIT_FIELD_HOVERHEIGHT);

            Relocate(x, y, z, orientation);
            SendTeleportPacket(oldPos); // this automatically relocates to oldPos in order to broadcast the packet in the right place
        }
    }
    else
    {
        // Pandaria
        if (mapid == 870  && getLevel() < 85 && getClass() != CLASS_MONK  && !isGameMaster())
            return false;

        // Tréfonds
         if ( mapid == 646  && getLevel() < 80 && !isGameMaster())
            return false;

        if (GetMapId() == 860 && GetTeamId() == TEAM_NEUTRAL)
            return false;

        // far teleport to another map
        Map* oldmap = IsInWorld() ? GetMap() : NULL;
        // check if we can enter before stopping combat / removing pet / totems / interrupting spells

        // Check enter rights before map getting to avoid creating instance copy for player
        // this check not dependent from map instance copy and same for all instance copies of selected map
        if (!sMapMgr->CanPlayerEnter(mapid, this, false))
            return false;

        if (Group* group = GetGroup())
        {
            if (mEntry->IsDungeon())
                group->IncrementPlayersInInstance();
            else
                group->DecrementPlayersInInstance();
        }

        //I think this always returns true. Correct me if I am wrong.
        // If the map is not created, assume it is possible to enter it.
        // It will be created in the WorldPortAck.
        //Map* map = sMapMgr->FindBaseNonInstanceMap(mapid);
        //if (!map || map->CanEnter(this))
        {
            //lets reset near teleport flag if it wasn't reset during chained teleports
            SetSemaphoreTeleportNear(false);
            //setup delayed teleport flag
            SetDelayedTeleportFlag(IsCanDelayTeleport());
            //if teleport spell is casted in Unit::Update() func
            //then we need to delay it until update process will be finished
            if (IsHasDelayedTeleport())
            {
                SetSemaphoreTeleportFar(true);
                //lets save teleport destination for player
                m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
                m_teleport_options = options;
                return true;
            }

            SetSelection(0);
            CombatStop();
            ResetContestedPvP();

            // remove player from battleground on far teleport (when changing maps)
            if (Battleground const* bg = GetBattleground())
            {
                // Note: at battleground join battleground id set before teleport
                // and we already will found "current" battleground
                // just need check that this is targeted map or leave
                if (bg->GetMapId() != mapid)
                    LeaveBattleground(false);                   // don't teleport to entry point
            }

            // remove arena spell coldowns/buffs now to also remove pet's cooldowns before it's temporarily unsummoned
            if (mEntry->IsBattleArena())
            {
                RemoveArenaSpellCooldowns(true);
                RemoveArenaAuras();
                if (pet)
                    pet->RemoveArenaAuras();
            }

            // remove pet on map change
            if (pet)
                UnsummonPetTemporaryIfAny();

            // remove all dyn objects and AreaTrigger
            RemoveAllDynObjects();
            RemoveAllAreaTriggers();

            // stop spellcasting
            // not attempt interrupt teleportation spell at caster teleport
            if (!(options & TELE_TO_SPELL))
                if (IsNonMeleeSpellCasted(true))
                    InterruptNonMeleeSpells(true);

            //remove auras before removing from map...
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CHANGE_MAP | AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);

            if (!GetSession()->PlayerLogout())
            {
                // send transfer packets
                bool teleportedBySpell = false;
                WorldPacket data(SMSG_TRANSFER_PENDING, 4 + 4 + 4);
                data << uint32(mapid);
                data.WriteBit(teleportedBySpell);
                data.WriteBit(m_transport != NULL);

                data.FlushBits();

                if (m_transport)
                    data << m_transport->GetEntry() << GetMapId();

                if (teleportedBySpell)
                    data << uint32(0); // spell id

                GetSession()->SendPacket(&data);
            }

            // remove from old map now
            if (oldmap)
                oldmap->RemovePlayerFromMap(this, false);

            // new final coordinates
            float final_x = x;
            float final_y = y;
            float final_z = z;
            float final_o = orientation;

            if (m_transport)
            {
                final_x += m_movementInfo.t_pos.GetPositionX();
                final_y += m_movementInfo.t_pos.GetPositionY();
                final_z += m_movementInfo.t_pos.GetPositionZ();
                final_o += m_movementInfo.t_pos.GetOrientation();
            }

            m_teleport_dest = WorldLocation(mapid, final_x, final_y, final_z, final_o);
            SetFallInformation(0, final_z);
            // if the player is saved before worldportack (at logout for example)
            // this will be used instead of the current location in SaveToDB

            if (!GetSession()->PlayerLogout())
            {
                WorldPacket data(SMSG_NEW_WORLD, 4 + 4 + 4 + 4 + 4);
                data << uint32(mapid);
                data << float(m_teleport_dest.GetPositionY());
                data << float(m_teleport_dest.GetPositionZ());
                data << float(m_teleport_dest.GetOrientation());
                data << float(m_teleport_dest.GetPositionX());
                GetSession()->SendPacket(&data);
                SendSavedInstances();
            }

            // move packet sent by client always after far teleport
            // code for finish transfer to new map called in WorldSession::HandleMoveWorldportAckOpcode at client packet
            SetSemaphoreTeleportFar(true);
        }
        //else
        //    return false;
    }
    return true;
}

bool Player::TeleportTo(WorldLocation const &loc, uint32 options /*= 0*/)
{
    return TeleportTo(loc.GetMapId(), loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetOrientation(), options);
}

bool Player::TeleportToBGEntryPoint()
{
    if (m_bgData.joinPos.m_mapId == MAPID_INVALID)
        return false;

    ScheduleDelayedOperation(DELAYED_BG_MOUNT_RESTORE);
    ScheduleDelayedOperation(DELAYED_BG_TAXI_RESTORE);
    ScheduleDelayedOperation(DELAYED_BG_GROUP_RESTORE);

    return TeleportTo(m_bgData.joinPos);
}

// Summoning.

void Player::SummonIfPossible(bool agree)
{
    if (!agree)
    {
        m_summon_expire = 0;
        return;
    }

    // expire and auto declined
    if (m_summon_expire < time(NULL))
        return;

    // stop taxi flight at summon
    if (isInFlight())
    {
        GetMotionMaster()->MovementExpired();
        CleanupAfterTaxiFlight();
    }

    // drop flag at summon
    // this code can be reached only when GM is summoning player who carries flag, because player should be immune to summoning spells when he carries flag
    if (Battleground* bg = GetBattleground())
        bg->EventPlayerDroppedFlag(this);

    m_summon_expire = 0;

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS, 1);

    TeleportTo(m_summon_mapid, m_summon_x, m_summon_y, m_summon_z, GetOrientation());
}

    /*** Positions functions - Handled by PlayerMovement. ***/

// Position loading / saving.

bool Player::LoadPositionFromDB(uint32& mapid, float& x, float& y, float& z, float& o, bool& in_flight, uint64 guid)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_POSITION);
    stmt->setUInt32(0, GUID_LOPART(guid));
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
        return false;

    Field* fields = result->Fetch();

    x = fields[0].GetFloat();
    y = fields[1].GetFloat();
    z = fields[2].GetFloat();
    o = fields[3].GetFloat();
    mapid = fields[4].GetUInt16();
    in_flight = !fields[5].GetString().empty();

    return true;
}

void Player::SavePositionInDB(uint32 mapid, float x, float y, float z, float o, uint32 zone, uint64 guid)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_POSITION);

    stmt->setFloat(0, x);
    stmt->setFloat(1, y);
    stmt->setFloat(2, z);
    stmt->setFloat(3, o);
    stmt->setUInt16(4, uint16(mapid));
    stmt->setUInt16(5, uint16(zone));
    stmt->setUInt32(6, GUID_LOPART(guid));

    CharacterDatabase.Execute(stmt);
}

// Position update.

bool Player::UpdatePosition(float x, float y, float z, float orientation, bool teleport)
{
    if (!Unit::UpdatePosition(x, y, z, orientation, teleport))
        return false;

    //if (movementInfo.flags & MOVEMENTFLAG_MOVING)
    //    mover->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE);
    //if (movementInfo.flags & MOVEMENTFLAG_TURNING)
    //    mover->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);
    //AURA_INTERRUPT_FLAG_JUMP not sure

    // group update
    if (GetGroup())
        SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);

    // code block for underwater state update
    // Unit::SetPosition() checks for validity and update our coordinates
    // so we re-fetch them instead of using "raw" coordinates from function params
    UpdateUnderwaterState(GetMap(), GetPositionX(), GetPositionY(), GetPositionZ());

    if (GetTrader() && !IsWithinDistInMap(GetTrader(), INTERACTION_DISTANCE))
        GetSession()->SendCancelTrade();

    CheckAreaExploreAndOutdoor();

    return true;
}

void Player::UpdateUnderwaterState(Map* m, float x, float y, float z)
{
    // temporary hack
    // player can't be on the ship and in the water at the same time
    // If this is not submarine
    if (GetTransport())
        return;

    LiquidData liquid_status;
    ZLiquidStatus res = m->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &liquid_status);
    if (!res)
    {
        m_MirrorTimerFlags &= ~(UNDERWATER_INWATER | UNDERWATER_INLAVA | UNDERWATER_INSLIME | UNDERWARER_INDARKWATER);
        if (_lastLiquid && _lastLiquid->SpellId)
            RemoveAurasDueToSpell(_lastLiquid->SpellId);

        _lastLiquid = NULL;
        return;
    }

    if (uint32 liqEntry = liquid_status.entry)
    {
        LiquidTypeEntry const* liquid = sLiquidTypeStore.LookupEntry(liqEntry);
        if (_lastLiquid && _lastLiquid->SpellId && _lastLiquid->Id != liqEntry)
            RemoveAurasDueToSpell(_lastLiquid->SpellId);

        if (liquid && liquid->SpellId)
        {
            if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER))
            {
                if (!HasAura(liquid->SpellId))
                    CastSpell(this, liquid->SpellId, true);
            }
            else
                RemoveAurasDueToSpell(liquid->SpellId);
        }

        _lastLiquid = liquid;
    }
    else if (_lastLiquid && _lastLiquid->SpellId)
    {
        RemoveAurasDueToSpell(_lastLiquid->SpellId);
        _lastLiquid = NULL;
    }


    // All liquids type - check under water position
    if (liquid_status.type_flags & (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_SLIME))
    {
        if (res & LIQUID_MAP_UNDER_WATER)
            m_MirrorTimerFlags |= UNDERWATER_INWATER;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INWATER;
    }

    // Allow travel in dark water on taxi or transport
    if ((liquid_status.type_flags & MAP_LIQUID_TYPE_DARK_WATER) && !isInFlight() && !GetTransport())
        m_MirrorTimerFlags |= UNDERWARER_INDARKWATER;
    else
        m_MirrorTimerFlags &= ~UNDERWARER_INDARKWATER;

    // in lava check, anywhere in lava level
    if (liquid_status.type_flags & MAP_LIQUID_TYPE_MAGMA)
    {
        if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER | LIQUID_MAP_WATER_WALK))
            m_MirrorTimerFlags |= UNDERWATER_INLAVA;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INLAVA;
    }
    // in slime check, anywhere in slime level
    if (liquid_status.type_flags & MAP_LIQUID_TYPE_SLIME)
    {
        if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER | LIQUID_MAP_WATER_WALK))
            m_MirrorTimerFlags |= UNDERWATER_INSLIME;
        else
            m_MirrorTimerFlags &= ~UNDERWATER_INSLIME;
    }
}

void Player::SetFallInformation(uint32 time, float z)
{
    m_lastFallTime = time;
    m_lastFallZ = z;
}

// Summoning.

void Player::SetSummonPoint(uint32 mapid, float x, float y, float z)
{
    m_summon_expire = time(NULL) + MAX_PLAYER_SUMMON_DELAY;
    m_summon_mapid = mapid;
    m_summon_x = x;
    m_summon_y = y;
    m_summon_z = z;
}

// Inns and Resting system.

void Player::InnEnter(time_t time, uint32 mapid, float x, float y, float z)
{
    inn_pos_mapid = mapid;
    inn_pos_x = x;
    inn_pos_y = y;
    inn_pos_z = z;
    time_inn_enter = time;
}

// Recall position.

void Player::SaveRecallPosition()
{
    m_recallMap = GetMapId();
    m_recallX = GetPositionX();
    m_recallY = GetPositionY();
    m_recallZ = GetPositionZ();
    m_recallO = GetOrientation();
}

// Homebind coordinates.

void Player::SetHomebind(WorldLocation const& loc, uint32 area_id)
{
    m_homebindMapId  = loc.GetMapId();
    m_homebindAreaId = area_id;
    m_homebindX      = loc.GetPositionX();
    m_homebindY      = loc.GetPositionY();
    m_homebindZ      = loc.GetPositionZ();

    // Update sql homebind.
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PLAYER_HOMEBIND);
    stmt->setUInt16(0, m_homebindMapId);
    stmt->setUInt16(1, m_homebindAreaId);
    stmt->setFloat (2, m_homebindX);
    stmt->setFloat (3, m_homebindY);
    stmt->setFloat (4, m_homebindZ);
    stmt->setUInt32(5, GetGUIDLow());
    CharacterDatabase.Execute(stmt);
}

// Start position.

WorldLocation Player::GetStartPosition() const
{
    PlayerInfo const* info = sObjectMgr->GetPlayerInfo(getRace(), getClass());

    uint32 mapId = info->mapId;
    if (getClass() == CLASS_DEATH_KNIGHT && HasSpell(50977))
        mapId = 0;

    return WorldLocation(mapId, info->positionX, info->positionY, info->positionZ, 0);
}
