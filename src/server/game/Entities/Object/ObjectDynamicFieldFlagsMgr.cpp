/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#include "Object.h"
#include "ObjectMovementMgr.h"
#include "Common.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "Creature.h"
#include "CreatureMovementMgr.h"
#include "Player.h"
#include "PlayerMovementMgr.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include "Util.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Transport.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "VMapFactory.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SpellAuraEffects.h"
#include "UpdateFieldFlags.h"
#include "TemporarySummon.h"
#include "Totem.h"
#include "OutdoorPvPMgr.h"
#include "MovementPacketBuilder.h"
#include "DynamicTree.h"
#include "Unit.h"
#include "UnitMovementMgr.h"
#include "Group.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"

/*** Object Dynamic Field Flags functions ***/

void Object::SetDynamicFieldUInt32Value(uint32 index, uint16 offset, uint32 value)
{
    ASSERT(m_dynamicfields.find(index) != m_dynamicfields.end() || PrintDynamicIndexError(index, true));

    DynamicFieldsList::iterator itr = m_dynamicfields.find(index);

    if (itr->second.values[offset].valueNumber != value)
    {
        itr->second.values[offset].valueNumber = value;
        itr->second.values[offset].valueUpdated = 1;

        if (m_inWorld && !m_objectUpdated)
        {
            sObjectAccessor->AddUpdateObject(this);
            m_objectUpdated = true;
        }
    }
}

uint32 Object::GetDynamicFieldUInt32Value(uint32 index, uint16 offset) const
{
    ASSERT(m_dynamicfields.find(index) != m_dynamicfields.end() || PrintDynamicIndexError(index, false));

    DynamicFieldsList::const_iterator itr = m_dynamicfields.find(index);

    return itr->second.values[offset].valueNumber;
}

bool Object::PrintDynamicIndexError(uint32 index, bool set) const
{
    TC_LOG_ERROR("misc", "Attempt to %s non-existing dynamic field index: %u (max index: %u) for object typeid: %u type mask: %u", (set ? "set a value into a" : "get value from a"), index, 
    (GetTypeId() == TYPEID_UNIT && index >= UNIT_DYNAMIC_WORLD_EFFECTS ||  GetTypeId() == TYPEID_PLAYER && index >= PLAYER_DYNAMIC_DAILY_QUESTS_COMPLETED) ? 
	GetDynamicFieldDefaultSize(index) * 2 : GetDynamicFieldDefaultSize(index), GetTypeId(), m_objectType);

    return false;
}

/*** Object Dynamic Field Flags update handling and general usage calls ***/

// Initialize the dynamic fields system.
void Object::InitializeDynamicFields()
{
    m_dynamicfields.clear();

    LoadDynamicFields();
}

// Load the dynamic fields.
void Object::LoadDynamicFields()
{
    for (uint32 fieldSize = 0; fieldSize < GetDynamicFieldEntryNumberCount(); fieldSize++)
    {
        uint32 fieldEntry = GetDynamicFieldIndexFromEntryNumber(fieldSize);

        DynamicField newDynamicField;
        newDynamicField.entry = fieldEntry;
        newDynamicField.offsets = GetDynamicFieldDefaultSize(fieldEntry);
		newDynamicField.values.resize(newDynamicField.offsets);

        for (uint16 offsetNum = 0; offsetNum < newDynamicField.offsets; offsetNum++)
        {
            newDynamicField.values[offsetNum].valueNumber = 0;
            newDynamicField.values[offsetNum].valueUpdated = 0;
        }

        m_dynamicfields[fieldEntry] = newDynamicField;
    }
}

// Get entry indexes for unit types.
uint32 Object::GetDynamicFieldIndexFromEntryNumber(uint32 entry)
{
    uint32 index = 0;

    switch (entry)
    {
         case 0:
             if (GetTypeId() == TYPEID_UNIT)
                 index = UNIT_DYNAMIC_PASSIVE_SPELLS;
             else if (GetTypeId() == TYPEID_PLAYER)
                 index = PLAYER_DYNAMIC_RESEARCH_SITES;
             else if (GetTypeId() == TYPEID_GAMEOBJECT)
                 index = GAMEOBJECT_DYNAMIC_UNK;
             else if (GetTypeId() == TYPEID_ITEM)
                 index = ITEM_DYNAMIC_MODIFIERS;
             break;
         case 1:
             if (GetTypeId() == TYPEID_UNIT)
                 index = UNIT_DYNAMIC_WORLD_EFFECTS;
             else if (GetTypeId() == TYPEID_PLAYER)
                 index = PLAYER_DYNAMIC_DAILY_QUESTS_COMPLETED;
             break;

        default: break;
    }

    return index;
}

// Get entry number count for unit types.
uint32 Object::GetDynamicFieldEntryNumberCount()
{
    uint32 size = 0;

    if (GetTypeId() == TYPEID_UNIT || GetTypeId() == TYPEID_PLAYER)
        size = 2; // 2 dynamic fields each.
    else if (GetTypeId() == TYPEID_GAMEOBJECT || GetTypeId() == TYPEID_ITEM)
        size = 1; // 1 dynamic fields each.

    return size;
}

// This is used to see with how much the default size changed for the dynamic flags.
uint32 Object::GetDynamicFieldDefaultSize(uint32 index) const
{
    // Here we define and retrieve the size for each dynamic field, based on a given index argument.
    // One offset can hold one value, as values are not added on top of each other.
    // That means the size of the flags indicate the actual offset numbers.
    uint32 size = 0;

    // We also see what type of object got updated so the indexes don't get mixed up.
    switch (GetTypeId())
    {
         case TYPEID_UNIT:
             if (index == UNIT_DYNAMIC_PASSIVE_SPELLS || index == UNIT_DYNAMIC_WORLD_EFFECTS) size = 257;
             break;
         case TYPEID_PLAYER:
             if (index == PLAYER_DYNAMIC_RESEARCH_SITES || index == PLAYER_DYNAMIC_DAILY_QUESTS_COMPLETED) size = 2;
             break;
         case TYPEID_GAMEOBJECT:
             if (index == GAMEOBJECT_DYNAMIC_UNK) size = 1;
             break;
         case TYPEID_ITEM:
             if (index == ITEM_DYNAMIC_MODIFIERS) size = 72;
             break;

         default: break;
    }

    return size;
}
