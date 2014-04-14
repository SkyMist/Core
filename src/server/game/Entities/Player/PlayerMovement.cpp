/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#include "Player.h"
#include "PlayerMovement.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
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

void Player::ReadMovementInfo(WorldPacket& data, MovementInfo* mi, Movement::ExtraMovementStatusElement* extras /*= NULL*/)
{
    MovementStatusElements const* sequence = GetMovementStatusElementsSequence(data.GetOpcode());
    if (!sequence)
    {
        TC_LOG_ERROR("network", "Player::ReadMovementInfo: No movement sequence found for opcode %s", GetOpcodeNameForLogging(data.GetOpcode(), false).c_str());
        return;
    }

    bool hasMovementFlags = false;
    bool hasMovementFlags2 = false;
    bool hasTimestamp = false;
    bool hasOrientation = false;
    bool hasTransportData = false;
    bool hasTransportTime2 = false;
    bool hasTransportTime3 = false;
    bool hasPitch = false;
    bool hasFallData = false;
    bool hasFallDirection = false;
    bool hasSplineElevation = false;
    bool hasUnkTime = false;
    uint32 counterCount = 0u;

    ObjectGuid guid;
    ObjectGuid tguid;

    for (; *sequence != MSEEnd; ++sequence)
    {
        MovementStatusElements const& element = *sequence;

        switch (element)
        {
            case MSEHasGuidByte0:
            case MSEHasGuidByte1:
            case MSEHasGuidByte2:
            case MSEHasGuidByte3:
            case MSEHasGuidByte4:
            case MSEHasGuidByte5:
            case MSEHasGuidByte6:
            case MSEHasGuidByte7:
                guid[element - MSEHasGuidByte0] = data.ReadBit();
                break;
            case MSEHasTransportGuidByte0:
            case MSEHasTransportGuidByte1:
            case MSEHasTransportGuidByte2:
            case MSEHasTransportGuidByte3:
            case MSEHasTransportGuidByte4:
            case MSEHasTransportGuidByte5:
            case MSEHasTransportGuidByte6:
            case MSEHasTransportGuidByte7:
                if (hasTransportData)
                    tguid[element - MSEHasTransportGuidByte0] = data.ReadBit();
                break;
            case MSEGuidByte0:
            case MSEGuidByte1:
            case MSEGuidByte2:
            case MSEGuidByte3:
            case MSEGuidByte4:
            case MSEGuidByte5:
            case MSEGuidByte6:
            case MSEGuidByte7:
                data.ReadByteSeq(guid[element - MSEGuidByte0]);
                break;
            case MSETransportGuidByte0:
            case MSETransportGuidByte1:
            case MSETransportGuidByte2:
            case MSETransportGuidByte3:
            case MSETransportGuidByte4:
            case MSETransportGuidByte5:
            case MSETransportGuidByte6:
            case MSETransportGuidByte7:
                if (hasTransportData)
                    data.ReadByteSeq(tguid[element - MSETransportGuidByte0]);
                break;
            case MSEHasMovementFlags:
                hasMovementFlags = !data.ReadBit();
                break;
            case MSEHasMovementFlags2:
                hasMovementFlags2 = !data.ReadBit();
                break;
            case MSEHasTimestamp:
                hasTimestamp = !data.ReadBit();
                break;
            case MSEHasOrientation:
                hasOrientation = !data.ReadBit();
                break;
            case MSEHasTransportData:
                hasTransportData = data.ReadBit();
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    hasTransportTime2 = data.ReadBit();
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    hasTransportTime3 = data.ReadBit();
                break;
            case MSEHasPitch:
                hasPitch = !data.ReadBit();
                break;
            case MSEHasFallData:
                hasFallData = data.ReadBit();
                break;
            case MSEHasFallDirection:
                if (hasFallData)
                    hasFallDirection = data.ReadBit();
                break;
            case MSEHasSplineElevation:
                hasSplineElevation = !data.ReadBit();
                break;
            case MSEHasSpline:
                data.ReadBit();
                break;
            case MSEMovementFlags:
                if (hasMovementFlags)
                    mi->flags = data.ReadBits(30);
                break;
            case MSEMovementFlags2:
                if (hasMovementFlags2)
                    mi->flags2 = data.ReadBits(13);
                break;
            case MSETimestamp:
                if (hasTimestamp)
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
                if (hasOrientation)
                    mi->pos.SetOrientation(data.read<float>());
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data >> mi->transport.pos.m_positionX;
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data >> mi->transport.pos.m_positionY;
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data >> mi->transport.pos.m_positionZ;
                break;
            case MSETransportOrientation:
                if (hasTransportData)
                    mi->transport.pos.SetOrientation(data.read<float>());
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data >> mi->transport.seat;
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data >> mi->transport.time;
                break;
            case MSETransportTime2:
                if (hasTransportData && hasTransportTime2)
                    data >> mi->transport.time2;
                break;
            case MSETransportTime3:
                if (hasTransportData && hasTransportTime3)
                    data >> mi->transport.time3;
                break;
            case MSEPitch:
                if (hasPitch)
                    mi->pitch = G3D::wrap(data.read<float>(), float(-M_PI), float(M_PI));
                break;
            case MSEFallTime:
                if (hasFallData)
                    data >> mi->jump.fallTime;
                break;
            case MSEFallVerticalSpeed:
                if (hasFallData)
                    data >> mi->jump.zspeed;
                break;
            case MSEFallCosAngle:
                if (hasFallData && hasFallDirection)
                    data >> mi->jump.cosAngle;
                break;
            case MSEFallSinAngle:
                if (hasFallData && hasFallDirection)
                    data >> mi->jump.sinAngle;
                break;
            case MSEFallHorizontalSpeed:
                if (hasFallData && hasFallDirection)
                    data >> mi->jump.xyspeed;
                break;
            case MSESplineElevation:
                if (hasSplineElevation)
                    data >> mi->splineElevation;
                break;
            case MSECounterCount:
                counterCount = data.ReadBits(22);
                break;
            case MSECounter:
                for (int i = 0; i != counterCount; i++)
                    data.read_skip<uint32>();   /// @TODO: Maybe compare it with m_movementCounter to verify that packets are sent & received in order?
                break;
            case MSEHasUnkTime:
                hasUnkTime = !data.ReadBit();
                break;
            case MSEUnkTime:
                if (hasUnkTime)
                    data.read_skip<uint32>();
                break;
            case MSEZeroBit:
            case MSEOneBit:
                data.ReadBit();
                break;
            case MSEExtraElement:
                extras->ReadNextElement(data);
                break;
            default:
                ASSERT(Movement::PrintInvalidSequenceElement(element, __FUNCTION__));
                break;
        }
    }

    mi->guid = guid;
    mi->transport.guid = tguid;

    //! Anti-cheat checks. Please keep them in seperate if () blocks to maintain a clear overview.
    //! Might be subject to latency, so just remove improper flags.
    #ifdef TRINITY_DEBUG
    #define REMOVE_VIOLATING_FLAGS(check, maskToRemove) \
    { \
        if (check) \
        { \
            TC_LOG_DEBUG("entities.unit", "Player::ReadMovementInfo: Violation of MovementFlags found (%s). " \
                "MovementFlags: %u, MovementFlags2: %u for player GUID: %u. Mask %u will be removed.", \
                STRINGIZE(check), mi->GetMovementFlags(), mi->GetExtraMovementFlags(), GetGUIDLow(), maskToRemove); \
            mi->RemoveMovementFlag((maskToRemove)); \
        } \
    }
    #else
    #define REMOVE_VIOLATING_FLAGS(check, maskToRemove) \
        if (check) \
            mi->RemoveMovementFlag((maskToRemove));
    #endif

    /*! This must be a packet spoofing attempt. MOVEMENTFLAG_ROOT sent from the client is not valid
        in conjunction with any of the moving movement flags such as MOVEMENTFLAG_FORWARD.
        It will freeze clients that receive this player's movement info.
    */

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ROOT),
        MOVEMENTFLAG_ROOT);

    //! Cannot hover without SPELL_AURA_HOVER
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_HOVER) && !HasAuraType(SPELL_AURA_HOVER),
        MOVEMENTFLAG_HOVER);

    //! Cannot ascend and descend at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ASCENDING) && mi->HasMovementFlag(MOVEMENTFLAG_DESCENDING),
        MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);

    //! Cannot move left and right at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_LEFT) && mi->HasMovementFlag(MOVEMENTFLAG_RIGHT),
        MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT);

    //! Cannot strafe left and right at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_STRAFE_LEFT) && mi->HasMovementFlag(MOVEMENTFLAG_STRAFE_RIGHT),
        MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT);

    //! Cannot pitch up and down at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_PITCH_UP) && mi->HasMovementFlag(MOVEMENTFLAG_PITCH_DOWN),
        MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);

    //! Cannot move forwards and backwards at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FORWARD) && mi->HasMovementFlag(MOVEMENTFLAG_BACKWARD),
        MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD);

    //! Cannot walk on water without SPELL_AURA_WATER_WALK
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && !HasAuraType(SPELL_AURA_WATER_WALK),
        MOVEMENTFLAG_WATERWALKING);

    //! Cannot feather fall without SPELL_AURA_FEATHER_FALL
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FALLING_SLOW) && !HasAuraType(SPELL_AURA_FEATHER_FALL),
        MOVEMENTFLAG_FALLING_SLOW);

    /*! Cannot fly if no fly auras present. Exception is being a GM.
        Note that we check for account level instead of Player::IsGameMaster() because in some
        situations it may be feasable to use .gm fly on as a GM without having .gm on,
        e.g. aerial combat.
    */

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_CAN_FLY) && ToPlayer()->GetSession()->GetSecurity() == SEC_PLAYER &&
        !ToPlayer()->m_mover->HasAuraType(SPELL_AURA_FLY) &&
        !ToPlayer()->m_mover->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED),
        MOVEMENTFLAG_FLYING | MOVEMENTFLAG_CAN_FLY);

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_CAN_FLY) && mi->HasMovementFlag(MOVEMENTFLAG_FALLING),
        MOVEMENTFLAG_FALLING);

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FALLING) && (!hasFallData || !hasFallDirection), MOVEMENTFLAG_FALLING);

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION) &&
        (!hasSplineElevation || G3D::fuzzyEq(mi->splineElevation, 0.0f)), MOVEMENTFLAG_SPLINE_ELEVATION);

    // Client first checks if spline elevation != 0, then verifies flag presence
    if (hasSplineElevation)
        mi->AddMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION);

    #undef REMOVE_VIOLATING_FLAGS
}

void Player::UpdateFallInformationIfNeed(MovementInfo const& minfo, uint16 opcode)
{
    if (m_lastFallTime >= minfo.jump.fallTime || m_lastFallZ <= minfo.pos.GetPositionZ() || opcode == MSG_MOVE_FALL_LAND)
        SetFallInformation(minfo.jump.fallTime, minfo.pos.GetPositionZ());
}

void Player::HandleFall(MovementInfo const& movementInfo)
{
    // calculate total z distance of the fall
    float z_diff = m_lastFallZ - movementInfo.pos.GetPositionZ();
    //TC_LOG_DEBUG("misc", "zDiff = %f", z_diff);

    //Players with low fall distance, Feather Fall or physical immunity (charges used) are ignored
    // 14.57 can be calculated by resolving damageperc formula below to 0
    if (z_diff >= 14.57f && !isDead() && !IsGameMaster() &&
        !HasAuraType(SPELL_AURA_HOVER) && !HasAuraType(SPELL_AURA_FEATHER_FALL) &&
        !HasAuraType(SPELL_AURA_FLY) && !IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL) && !IsInWater())
    {
        //Safe fall, fall height reduction
        int32 safe_fall = GetTotalAuraModifier(SPELL_AURA_SAFE_FALL);

        float damageperc = 0.018f*(z_diff-safe_fall)-0.2426f;

        if (damageperc > 0)
        {
            uint32 damage = (uint32)(damageperc * GetMaxHealth()*sWorld->getRate(RATE_DAMAGE_FALL));

            float height = movementInfo.pos.m_positionZ;
            UpdateGroundPositionZ(movementInfo.pos.m_positionX, movementInfo.pos.m_positionY, height);

            if (damage > 0)
            {
                //Prevent fall damage from being more than the player maximum health
                if (damage > GetMaxHealth())
                    damage = GetMaxHealth();

                // Gust of Wind
                if (HasAura(43621))
                    damage = GetMaxHealth()/2;

                uint32 original_health = GetHealth();
                uint32 final_damage = EnvironmentalDamage(DAMAGE_FALL, damage);

                // recheck alive, might have died of EnvironmentalDamage, avoid cases when player die in fact like Spirit of Redemption case
                if (IsAlive() && final_damage < original_health)
                    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING, uint32(z_diff*100));
            }

            //Z given by moveinfo, LastZ, FallTime, WaterZ, MapZ, Damage, Safefall reduction
            TC_LOG_DEBUG("entities.player", "FALLDAMAGE z=%f sz=%f pZ=%f FallTime=%d mZ=%f damage=%d SF=%d", movementInfo.pos.GetPositionZ(), height, GetPositionZ(), movementInfo.jump.fallTime, height, damage, safe_fall);
        }
    }

    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_LANDING); // No fly zone - Parachute
}

void Player::SendMovementSetCanTransitionBetweenSwimAndFly(bool apply)
{
    Movement::PacketSender(this, NULL_OPCODE, apply ? SMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY : SMSG_MOVE_UNSET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY).Send();
}

void Player::SendMovementSetCollisionHeight(float height)
{
    static MovementStatusElements const heightElement = MSEExtraFloat;

    Movement::ExtraMovementStatusElement extra(&heightElement);
    extra.Data.floatData = height;

    Movement::PacketSender(this, NULL_OPCODE, SMSG_MOVE_SET_COLLISION_HEIGHT, SMSG_MOVE_UPDATE_COLLISION_HEIGHT, &extra).Send();
}

// Active mover set.

void Player::SetMover(Unit* target)
{
    m_mover->m_movedPlayer = NULL;
    m_mover = target;
    m_mover->m_movedPlayer = this;

    ObjectGuid guid = target->GetGUID();

    WorldPacket data(SMSG_MOVE_SET_ACTIVE_MOVER, 9);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[6]);

    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[6]);

    SendDirectMessage(&data);
}

// Teleportation.

bool Player::TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options, bool raidDifficultyChange)
{
    if (!MapManager::IsValidMapCoord(mapid, x, y, z, orientation))
    {
        TC_LOG_ERROR("maps", "TeleportTo: invalid map (%d) or invalid coordinates (X: %f, Y: %f, Z: %f, O: %f) given when teleporting player (GUID: %u, name: %s, map: %d, X: %f, Y: %f, Z: %f, O: %f).",
            mapid, x, y, z, orientation, GetGUIDLow(), GetName().c_str(), GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        return false;
    }

    if (!GetSession()->HasPermission(rbac::RBAC_PERM_SKIP_CHECK_DISABLE_MAP) && DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP, mapid, this))
    {
        TC_LOG_ERROR("maps", "Player (GUID: %u, name: %s) tried to enter a forbidden map %u", GetGUIDLow(), GetName().c_str(), mapid);
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
        TC_LOG_DEBUG("maps", "Player %s using client without required expansion tried teleport to non accessible map %u", GetName().c_str(), mapid);

        if (GetTransport())
        {
            m_transport->RemovePassenger(this);
            m_transport = NULL;
            m_movementInfo.ResetTransport();
            RepopAtGraveyard();                             // teleport to near graveyard if on transport, looks blizz like :)
        }

        SendTransferAborted(mapid, TRANSFER_ABORT_INSUF_EXPAN_LVL, mEntry->Expansion());

        return false;                                       // normal client can't teleport to this map...
    }
    else
        TC_LOG_DEBUG("maps", "Player %s is being teleported to map %u", GetName().c_str(), mapid);

    if (m_vehicle)
        ExitVehicle();

    // reset movement flags at teleport, because player will continue move with these flags after teleport
    SetUnitMovementFlags(GetUnitMovementFlags() & MOVEMENTFLAG_MASK_HAS_PLAYER_STATUS_OPCODE);
    m_movementInfo.ResetJump();
    DisableSpline();

    // clear unit emote state
    HandleEmote(EMOTE_ONESHOT_NONE);

    if (m_transport)
    {
        if (!(options & TELE_TO_NOT_LEAVE_TRANSPORT))
        {
            m_transport->RemovePassenger(this);
            m_transport = NULL;
            m_movementInfo.ResetTransport();
        }
    }

    // The player was ported to another map and loses the duel immediately.
    // We have to perform this check before the teleport, otherwise the
    // ObjectAccessor won't find the flag.
    if (duel && GetMapId() != mapid && GetMap()->GetGameObject(GetUInt64Value(PLAYER_FIELD_DUEL_ARBITER)))
        DuelComplete(DUEL_FLED);

    if (GetMapId() == mapid && !raidDifficultyChange)
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
            if (HasUnitMovementFlag(MOVEMENTFLAG_HOVER))
                z += GetFloatValue(UNIT_FIELD_HOVER_HEIGHT);
            Relocate(x, y, z, orientation);
            SendTeleportPacket(oldPos); // this automatically relocates to oldPos in order to broadcast the packet in the right place
        }
    }
    else
    {
        if (getClass() == CLASS_DEATH_KNIGHT && GetMapId() == 609 && !IsGameMaster() && !HasSpell(50977))
            return false;

        // far teleport to another map
        Map* oldmap = IsInWorld() ? GetMap() : NULL;
        // check if we can enter before stopping combat / removing pet / totems / interrupting spells

        // Check enter rights before map getting to avoid creating instance copy for player
        // this check not dependent from map instance copy and same for all instance copies of selected map
        if (!sMapMgr->CanPlayerEnter(mapid, this, false))
            return false;

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

            // remove all dyn objects
            RemoveAllDynObjects();

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
                WorldPacket data(SMSG_TRANSFER_PENDING, 4 + 4 + 4 + 1);       

                data << uint32(mapid);
                data.WriteBit(0);       // Unknown.

                bool currentlyHasTransport = (m_transport && m_transport->GetGOInfo()->type == GAMEOBJECT_TYPE_MO_TRANSPORT) ? true : false;
                data.WriteBit(currentlyHasTransport);
                if (currentlyHasTransport)
                {
                    // Might be in wrong order
                    data << GetMapId();
                    data << m_transport->GetEntry();
                }

                GetSession()->SendPacket(&data);
            }

            // remove from old map now
            if (oldmap)
                oldmap->RemovePlayerFromMap(this, false);

            m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
            SetFallInformation(0, z);
            // if the player is saved before worldportack (at logout for example)
            // this will be used instead of the current location in SaveToDB

            if (!GetSession()->PlayerLogout())
            {
                GetSession()->SendNewWorld(m_teleport_dest);
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
    if (IsInFlight())
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

  /*if (movementInfo.flags & MOVEMENTFLAG_MOVING)
        mover->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE);
    if (movementInfo.flags & MOVEMENTFLAG_TURNING)
        mover->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);
    AURA_INTERRUPT_FLAG_JUMP not sure*/

    if (GetGroup())
        SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION); // Group position update.

    if (GetTrader() && !IsWithinDistInMap(GetTrader(), INTERACTION_DISTANCE))
        GetSession()->SendCancelTrade();

    CheckAreaExploreAndOutdoor();

    return true;
}

void Player::UpdateUnderwaterState(Map* m, float x, float y, float z)
{
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
    if ((liquid_status.type_flags & MAP_LIQUID_TYPE_DARK_WATER) && !IsInFlight() && !GetTransport())
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

void Player::SetHomebind(WorldLocation const& loc, uint32 areaId)
{
    loc.GetPosition(m_homebindX, m_homebindY, m_homebindZ);

    m_homebindMapId = loc.GetMapId();
    m_homebindAreaId = areaId;

    // update sql homebind
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

// SMSG_NEW_WORLD position sending (teleports etc).
void WorldSession::SendNewWorld(WorldLocation const &location)
{
    if (sMapMgr->IsValidMapCoord(location))
    {
        WorldPacket worldPosition(SMSG_NEW_WORLD, 20);
        worldPosition << location.GetMapId() << location.GetPositionY() << location.GetPositionZ() << location.GetOrientation() << location.GetPositionX();
        SendPacket(&worldPosition);
    }
    else
    {
        WorldPacket worldAbort(SMSG_NEW_WORLD_ABORT);
        SendPacket(&worldAbort);
    }
}
