#ifndef _PACKETS_H
#define _PACKETS_H
#pragma once

#include "Headers.h"
#include "ByteBuffer.h"

namespace Packets
{
    class Packet
    {
        public:
            virtual void Process();
        
        protected:
            ByteBuffer _buffer;
    };
    
    template<class Header>
    class ServerPacket : public Packet
    {
        public:
            ServerPacket(ServerOpcodes Opcode);
            ~ServerPacket();
        
            uv_buf_t* ToBuffer();
        
        private:
            ServerOpcodes _opcode;
            Header _header;
    };
    
    template<class Header>
    class ClientPacket : public Packet
    {
        public:
            ClientPacket(const uv_buf_t* Buffer);
            ~ClientPacket();
        
            ClientOpcodes GetOpcode();
        
        private:
            ClientOpcodes _opcode;
    };
    
    typedef ClientPacket<NormalHeader> NormalClientPacket;
    typedef ClientPacket<SetupHeader> SetupClientPacket;
    typedef ServerPacket<NormalHeader> NormalServerPacket;
    typedef ServerPacket<SetupHeader> SetupServerPacket;
};

#endif