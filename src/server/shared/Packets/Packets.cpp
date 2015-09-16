#include "Packets.h"

void Packets::Packet::Process()
{
    
};

template<class Header>
Packets::ServerPacket<Header>::ServerPacket(ServerOpcodes Opcode) : _opcode(Opcode)
{
    
}

template<class Header>
Packets::ServerPacket<Header>::~ServerPacket()
{
    
}

template<class Header>
uv_buf_t* Packets::ServerPacket<Header>::ToBuffer()
{
    uv_buf_t* Buffer = new uv_buf_t;
    std::size_t PacketSize = _buffer.size() + sizeof(Header);
    Buffer->base = new char[PacketSize];
    Buffer->len = PacketSize;
    
    _header = Header(PacketSize, _opcode);
    memcpy(Buffer->base, &_header, sizeof(Header));
    memcpy(Buffer->base + sizeof(Header), _buffer.contents(), _buffer.size());
    
    return Buffer;
}

template<class Header>
Packets::ClientPacket<Header>::ClientPacket(const uv_buf_t* Buffer)
{

}

template<>
Packets::ClientPacket<Packets::NormalHeader>::ClientPacket(const uv_buf_t* Buffer)
{
    Packets::NormalHeader _header;
    memcpy(&_header, Buffer->base, sizeof(Packets::NormalHeader));
    _opcode = static_cast<ClientOpcodes>(_header.GetOpcode());
    _buffer.append(Buffer->base + sizeof(Packets::NormalHeader), _header.GetPacketSize());
}

template<>
Packets::ClientPacket<Packets::SetupHeader>::ClientPacket(const uv_buf_t* Buffer)
{
    Packets::SetupHeader _header;
    memcpy(&_header, Buffer->base, sizeof(Packets::SetupHeader));
    _opcode = static_cast<ClientOpcodes>(_header._opcode);
    _buffer.append(Buffer->base + sizeof(Packets::SetupHeader), _header.size);
}

template<class Header>
Packets::ClientPacket<Header>::~ClientPacket()
{
    
}