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

#include "ObjectAccessor.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "Log.h"
#include "AreaTrigger.h"

AreaTrigger::AreaTrigger() : WorldObject(false), _duration(0), m_caster(NULL), m_visualRadius(0.0f)
{
    m_objectType |= TYPEMASK_AREATRIGGER;
    m_objectTypeId = TYPEID_AREATRIGGER;

    m_updateFlag = UPDATEFLAG_STATIONARY_POSITION;

    m_valuesCount = AREATRIGGER_END;
}

AreaTrigger::~AreaTrigger()
{
    ASSERT(!m_caster);
}

void AreaTrigger::AddToWorld()
{
    ///- Register the AreaTrigger for guid lookup and for caster
    if (!IsInWorld())
    {
        sObjectAccessor->AddObject(this);
        WorldObject::AddToWorld();
        BindToCaster();
    }
}

void AreaTrigger::RemoveFromWorld()
{
    ///- Remove the AreaTrigger from the accessor and from all lists of objects in world
    if (IsInWorld())
    {
        UnbindFromCaster();
        WorldObject::RemoveFromWorld();
        sObjectAccessor->RemoveObject(this);
    }
}

bool AreaTrigger::CreateAreaTrigger(uint32 guidlow, uint32 triggerEntry, Unit* caster, SpellInfo const* spell, Position const& pos)
{
    SetMap(caster->GetMap());
    Relocate(pos);
    if (!IsPositionValid())
    {
        sLog->outError("misc", "AreaTrigger (spell %u) not created. Invalid coordinates (X: %f Y: %f)", spell->Id, GetPositionX(), GetPositionY());
        return false;
    }

    WorldObject::_Create(guidlow, HIGHGUID_AREATRIGGER, caster->GetPhaseMask());

    SetEntry(triggerEntry);
    SetDuration(spell->GetDuration());
    SetObjectScale(1);

    SetUInt64Value(AREATRIGGER_FIELD_CASTER, caster->GetGUID());
    SetUInt32Value(AREATRIGGER_FIELD_SPELL_ID, spell->Id);
    SetUInt32Value(AREATRIGGER_FIELD_SPELL_VISUAL_ID, spell->SpellVisual[0]);
    SetUInt32Value(AREATRIGGER_FIELD_DURATION, spell->GetDuration());
    SetFloatValue(AREATRIGGER_FIELD_EXPLICIT_SCALE, GetObjectScale());

    switch (spell->Id)
    {
        case 116011:// Rune of Power
            SetVisualRadius(3.5f);
            break;
        case 116235:// Amethyst Pool
            SetVisualRadius(3.5f);
            break;
        case 123811: // Pheromones of Zeal Zor'lok
            SetVisualRadius(100.0f);
            SetDuration(5000000);
            break;
        case 122731: // Noise Cancelling Zor'lok
            SetVisualRadius(4.0f);
            break;

        default: break;
    }

    if (!GetMap()->AddToMap(this))
        return false;

    return true;
}

void AreaTrigger::Update(uint32 p_time)
{
    if (GetDuration() > int32(p_time))
        _duration -= p_time;
    else
        Remove(); // expired

    WorldObject::Update(p_time);

    // Handle all special AreaTriggers here.

    SpellInfo const* m_spellInfo = sSpellMgr->GetSpellInfo(GetUInt32Value(AREATRIGGER_FIELD_SPELL_ID));
    if (!m_spellInfo)
        return;

    if (!GetCaster())
    {
        Remove();
        return;
    }

    Unit* caster = GetCaster();
    float radius = 0.0f;

    // Custom MoP Script
    switch (m_spellInfo->Id)
    {
        case 102793: // Ursol's Vortex
        {
            std::list<Unit*> targetList;
            radius = 8.0f;

            Trinity::NearestAttackableUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                    if (!(*itr)->HasAura(127797))
                        caster->CastSpell(*itr, 127797, true);

            break;
        }

        case 115460: // Healing Sphere
        {
            std::list<Unit*> targetList;
            radius = 1.0f;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
            {
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    caster->CastSpell(*itr, 115464, true); // Healing Sphere heal
                    SetDuration(0);
                    return;
                }
            }

            break;
        }

        case 115817: // Cancel Barrier
        {
            std::list<Unit*> targetList;
            radius = 6.0f;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                    (*itr)->CastSpell(*itr, 115856, true);

            break;
        }

        case 116011: // Rune of Power
        {
            std::list<Unit*> targetList;
            bool affected = false;
            radius = 2.25f;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
            {
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    if ((*itr)->GetGUID() == caster->GetGUID())
                    {
                        caster->CastSpell(*itr, 116014, true); // Rune of Power
                        affected = true;

                        if (caster->ToPlayer())
                            caster->ToPlayer()->UpdateManaRegen();

                        return;
                    }
                }
            }

            if (!affected)
                caster->RemoveAura(116014);

            break;
        }

        case 116235: // Amethyst Pool
        {
            std::list<Unit*> targetList;
            radius = 10.0f;

            Trinity::NearestAttackableUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
            {
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    // Amethyst Pool - Periodic Damage
                    if ((*itr)->GetDistance(this) > 3.5f)
                        (*itr)->RemoveAura(130774);
                    else if (!(*itr)->HasAura(130774))
                        caster->CastSpell(*itr, 130774, true);
                }
            }

            break;
        }

        case 144692: // Pool of Fire Ordos
        {
            std::list<Player*> targetList;
            radius = 15.0f;

            GetPlayerListInGrid(targetList, 200.0f);

            if (!targetList.empty())
            {
                for (std::list<Player*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    // Pool of Fire - Periodic Damage
                    if ((*itr)->GetDistance(this) > radius)
                        (*itr)->RemoveAurasDueToSpell(144693);
                    else if (!(*itr)->HasAura(144693))
                        caster->AddAura(144693, *itr);
                }
            }
            break;
        }

        case 116546: // Draw Power
        {
            std::list<Unit*> targetList;
            radius = 30.0f;

            Trinity::NearestAttackableUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
            {
                if ((*itr)->IsInAxe(caster, this, 2.0f))
                {
                    if (!(*itr)->HasAura(116663))
                        caster->AddAura(116663, *itr);
                }
                else
                    (*itr)->RemoveAurasDueToSpell(116663);
            }

            break;
        }
        case 117032: // Healing Sphere (Afterlife)
        {
            std::list<Unit*> targetList;
            radius = 1.0f;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
            {
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    if ((*itr)->GetGUID() == caster->GetGUID())
                    {
                        caster->CastSpell(*itr, 125355, true); // Heal for 15% of life
                        SetDuration(0);
                        return;
                    }
                }
            }

            break;
        }

        case 119031: // Gift of the Serpent (Mastery)
        {
            std::list<Unit*> targetList;
            radius = 1.0f;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
            {
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    caster->CastSpell(*itr, 124041, true); // Gift of the Serpent heal
                    SetDuration(0);
                    return;
                }
            }

            break;
        }

        case 121286: // Chi Sphere (Afterlife)
        {
            std::list<Unit*> targetList;
            radius = 1.0f;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
            {
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    if ((*itr)->GetGUID() == caster->GetGUID())
                    {
                        caster->CastSpell(*itr, 121283, true); // Restore 1 Chi
                        SetDuration(0);
                        return;
                    }
                }
            }

            break;
        }

        case 121536: // Angelic Feather
        {
            std::list<Unit*> targetList;
            radius = 1.0f;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
            {
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    caster->CastSpell(*itr, 121557, true); // Angelic Feather increase speed
                    SetDuration(0);
                    return;
                }
            }

            break;
        }

        case 122035: // Path of Blossom
        {
            std::list<Unit*> targetList;
            radius = 1.0f;

            Trinity::NearestAttackableUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            if (!targetList.empty())
            {
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    caster->CastSpell(*itr, 122036, true); // Path of Blossom damage
                    SetDuration(0);
                    return;
                }
            }

            break;
        }

        case 124503: // Gift of the Ox
        case 124506: // Gift of the Ox²
        {
            std::list<Unit*> targetList;
            radius = 1.0f;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, caster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targetList, u_check);
            VisitNearbyObject(radius, searcher);

            for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
            {
                if ((*itr)->GetGUID() != caster->GetGUID())
                    continue;

                caster->CastSpell(*itr, 124507, true); // Gift of the Ox - Heal
                SetDuration(0);
                return;
            }

            break;
        }

        case 123811: // Pheromones of Zeal Zor'lok
        {
            std::list<Player*> targetList;
            radius = 100.0f;
            float zPos = 407.0f;

            GetPlayerListInGrid(targetList, 100.0f);

            if (!targetList.empty())
            {
                for (std::list<Player*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    // Pheromones of Zeal - Periodic Damage
                    if ((*itr)->GetDistance(this) > radius || (*itr)->GetPositionZ() > zPos)
                        (*itr)->RemoveAurasDueToSpell(123812);
                    else
                    {
					    if (!(*itr)->HasAura(123812))
                            caster->AddAura(123812, *itr);
                    }
                }
            }
            break;
        }

        case 122731: // Noise Cancelling Zor'lok
        {
            std::list<Player*> targetList;
            radius = 3.5f;
            uint32 playersInside = 0;
            uint32 maxPlayersInside = 0;
            Difficulty difficulty = caster->GetMap()->GetDifficulty();

            GetPlayerListInGrid(targetList, 5.0f);

            if (!targetList.empty())
            {
                for (std::list<Player*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    // Check and increase player number having the aura.
                    if ((*itr)->HasAura(122706))
                        ++playersInside;

                    switch(difficulty)
                    {
                        case RAID_DIFFICULTY_10MAN_NORMAL:
                            maxPlayersInside = 4;
                            break;
                        case RAID_DIFFICULTY_25MAN_NORMAL:
                            maxPlayersInside = 9;
                            break;
                        case RAID_DIFFICULTY_10MAN_HEROIC:
                            maxPlayersInside = 3;
                            break;
                        case RAID_DIFFICULTY_25MAN_HEROIC:
                            maxPlayersInside = 7;
                            break;
                    }

                    // Noise Cancelling - Apply damage reduction buff.
                    if ((*itr)->GetDistance(this) > radius)
                    {
                        (*itr)->RemoveAurasDueToSpell(122706);
                        --playersInside;
                    }
                    else
                    {
					    if (!(*itr)->HasAura(122706) && playersInside <= maxPlayersInside)
                        {
                            caster->AddAura(122706, *itr);
                            ++playersInside;
                        }
                    }
                }
            }
            break;
        }

        default: break;
    }
}

void AreaTrigger::Remove()
{
    if (IsInWorld())
    {
        SpellInfo const* m_spellInfo = sSpellMgr->GetSpellInfo(GetUInt32Value(AREATRIGGER_FIELD_SPELL_ID));
        if (!m_spellInfo)
            return;

        switch (m_spellInfo->Id)
        {
            case 116011: // Rune of Power : Remove the buff if caster is still in radius
                if (m_caster && m_caster->HasAura(116014))
                    m_caster->RemoveAura(116014);
                break;

            case 144692: // Pool of Fire Ordos
            {
                std::list<Player*> targetList;
                GetPlayerListInGrid(targetList, 100.0f);
                if (!targetList.empty())
                    for (std::list<Player*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                        (*itr)->RemoveAurasDueToSpell(144693); // Pool of Fire - Periodic Damage Remove.
                break;
            }

            case 123811: // Pheromones of Zeal Zor'lok
            {
                std::list<Player*> targetList;
                GetPlayerListInGrid(targetList, 100.0f);
                if (!targetList.empty())
                    for (std::list<Player*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                        (*itr)->RemoveAurasDueToSpell(123812); // Pheromones of Zeal - Periodic Damage Remove.
                break;
            }

            case 122731: // Noise Cancelling Zor'lok
            {
                std::list<Player*> targetList;
                GetPlayerListInGrid(targetList, 100.0f);
                if (!targetList.empty())
                    for (std::list<Player*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                        (*itr)->RemoveAurasDueToSpell(122706); // Noise Cancelling - Buff Remove.
                break;
            }

            default: break;
        }

        SendObjectDeSpawnAnim(GetGUID());
        RemoveFromWorld();
        AddObjectToRemoveList();
    }
}

void AreaTrigger::BindToCaster()
{
    m_caster = ObjectAccessor::GetUnit(*this, GetCasterGUID());

    if (m_caster)
        m_caster->_RegisterAreaTrigger(this);
}

void AreaTrigger::UnbindFromCaster()
{
    ASSERT(m_caster);
    m_caster->_UnregisterAreaTrigger(this);
    m_caster = NULL;
}
