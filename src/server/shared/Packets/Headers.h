#ifndef _PACKET_HEADERS_H
#define _PACKET_HEADERS_H
#pragma once

#include "Common.h"

namespace Packets
{
    //these values are just for testing purposes
    enum ClientOpcodes : uint32
    {
        CMSG_AUTH_SESSION = 0x0001
    };
    
    enum ServerOpcodes : uint32
    {
        SMSG_AUTH_CHALLENGE = 0x0001
    };
    
    struct SetupHeader
    {
        SetupHeader(std::size_t Size, uint32 cmd) : _opcode(cmd)
        {
            if (Size <= 0xFFFF)
                size = Size + 6;
        }
        
        SetupHeader()
        {
            size = 0;
            _opcode = 0;
        }

        uint16 size;
        uint32 _opcode;
    };
    
    struct NormalHeader
    {
        NormalHeader(std::size_t size, uint32 cmd)
        {
            packed = ((size + 4) << 13) | (cmd & 0x1FFF);
        }
        
        NormalHeader()
        {
            packed = 0;
        }
        
        //this returns the size of the data, without the opcode itself
        std::size_t GetPacketSize()
        {
            std::size_t Size = (packed >> 13) - 4;
            return Size;
        }
        
        uint32 GetOpcode()
        {
            return packed & 0x1FFF;
        }
        
        uint32 packed;
    };
};

#endif