/*Copyright (C) 2014 SkyMist Project.
*
* This file is NOT free software. Third-party users can NOT redistribute it or modify it :). 
* If you find it, you are either hacking something, or very lucky (presuming someone else managed to hack it).
*/

#ifndef _OBJECT_DYNAMIC_UPDATE_FIELDS_H
#define _OBJECT_DYNAMIC_UPDATE_FIELDS_H

#include "Common.h"
#include "UpdateMask.h"
#include "GridReference.h"
#include "ObjectDefines.h"
#include "Map.h"

#include <set>
#include <string>
#include <sstream>

class DynamicFields
{
public:
    DynamicFields()
    {
        _dynamicValues = new uint32[Count];
        _dynamicChangedFields = new bool[Count];

        Clear();
    }
    
    ~DynamicFields()
    {
        delete[] _dynamicValues;
        _dynamicValues = NULL;

        delete[] _dynamicChangedFields;
        _dynamicChangedFields = NULL;
    }

    void Clear()
    {
        memset(_dynamicValues, 0, sizeof(uint32) * Count);
        memset(_dynamicChangedFields, 0, sizeof(bool) * Count);
    }

    void ClearMask()
    {
        memset(_dynamicChangedFields, 0, sizeof(bool) * Count);
    }

    uint32* _dynamicValues;
    bool* _dynamicChangedFields;

    const static uint32 Count = sizeof(uint32) * 8;
};

#endif
