#include "Auth.h"

Packets::Auth::AuthChallenge::AuthChallenge() : Packets::SetupServerPacket(SMSG_AUTH_CHALLENGE)
{
    challenge = 0;
    memset(&DoSChallenge, 0, sizeof(DoSChallenge));
    _DoSZeroBits = 0;
}

Packets::Auth::AuthChallenge::~AuthChallenge()
{
    
}

void Packets::Auth::AuthChallenge::Process()
{
    _buffer << challenge;
    _buffer.append(&DoSChallenge, sizeof(DoSChallenge));
    _buffer << _DoSZeroBits;
}

Packets::Auth::AuthSession::AuthSession(const uv_buf_t* Data) : Packets::SetupClientPacket(Data)
{
    
}

Packets::Auth::AuthSession::~AuthSession()
{
    
}

void Packets::Auth::AuthSession::Process()
{
    _buffer >> _gruntServerID;
    _buffer >> clientBuild;
    _buffer >> _region;
    _buffer >> _battlegroup;
    _buffer >> _realmIndex;
    _buffer >> _loginServerType;
    _buffer.read<uint8>();
    _buffer >> clientSeed;
    _buffer >> dosResponse;
    _buffer.read(shaDigest, sizeof(shaDigest));
    std::size_t AccountLength = _buffer.ReadBits(11);
    accountName = _buffer.ReadString(AccountLength);
    _useIPv6 = _buffer.ReadBit();
    uint32 AddonsSize = _buffer.read<uint32>();
    _addons.resize(AddonsSize);
    uint8* Addons = new uint8[AddonsSize];
    _buffer.read(Addons, AddonsSize);
    _addons.append(Addons, AddonsSize);
    delete[] Addons;
}

void Packets::Auth::AuthSession::ProcessAddons()
{
    
}