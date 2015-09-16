#ifndef _AUTH_PACKETS
#define _AUTH_PACKETS
#pragma once

#include "Packets.h"

namespace Packets
{
    namespace Auth
    {
        const uint8 CihperSeed[] = { 0x08, 0xF1, 0x95, 0x9F, 0x47, 0xE5, 0xD2, 0xDB, 0xA1, 0x3D,
                                     0x77, 0x8F, 0x3F, 0x3E, 0xE7, 0x00, 0x40, 0xAA, 0xD3, 0x92, 0x26,
                                     0x71, 0x43, 0x47, 0x3A, 0x31, 0x08, 0xA6, 0xE7, 0xDC, 0x98, 0x2A };
        
        class AuthChallenge : public Packets::SetupServerPacket
        {
            public:
                AuthChallenge();
                ~AuthChallenge();
            
                void Process() override;
            
            public:
                uint32 challenge;
                uint8 DoSChallenge[32];
            
            private:
                uint8 _DoSZeroBits;
        };
        
        class AuthSession : public Packets::SetupClientPacket
        {
            public:
                AuthSession(const uv_buf_t* Data);
                ~AuthSession();
            
                void Process() override;
            
            private:
                void ProcessAddons();
            
            public:
                int16 clientBuild;
                uint32 clientSeed;
                uint64 dosResponse;
                uint8 shaDigest[20];
                std::string accountName;
            
            private:
                uint32 _gruntServerID;
                uint32 _region;
                uint32 _battlegroup;
                uint32 _realmIndex;
                uint8 _loginServerType;
                bool _useIPv6;
                ByteBuffer _addons;
        };
    };
};

#endif