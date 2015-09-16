/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Master.h"
#include <Packets/Auth.h>

GameServiceHandler::GameServiceHandler(uv_tcp_t* ClientSocket) : Networking::ServiceHandler(ClientSocket)
{
    _cipherSeed = new uint8 [sizeof(Packets::Auth::CihperSeed)];
    _cipherSize = sizeof(Packets::Auth::CihperSeed);
    memcpy(_cipherSeed, Packets::Auth::CihperSeed, _cipherSize);
}

GameServiceHandler::~GameServiceHandler()
{
    
}

void GameServiceHandler::ProcessMessage(Packets::NormalClientPacket *Packet)
{
    
}

void GameServiceHandler::Authenticate(Packets::SetupClientPacket* Packet)
{
    Packets::Auth::AuthSession* AuthSession = dynamic_cast<Packets::Auth::AuthSession*>(Packet);
    AuthSession->Process();
    
    //make the authentication process here
    
    //1. lookup for such username -> AUTH_UNKNOWN_ACCOUNT
    //2. see if username or ip is banned -> AUTH_BANNED
    //3. see if already online -> AUTH_ALREADY_ONLINE
    //4. see if correct build -> AUTH_VERSION_MISMATCH
    //5. see if dos challenge is ok -> AUTH_SESSION_EXPIRED
    
    //6. check hash -> AUTH_FAILED
    uint32 Zero = 0;
    CryptoPP::SHA1 hash;
    byte* Digest = new byte[hash.DigestSize()];
    
    hash.Update(reinterpret_cast<const byte*>(AuthSession->accountName.c_str()), AuthSession->accountName.size());
    hash.Update(reinterpret_cast<byte*>(&Zero), sizeof(Zero));
    hash.Update(reinterpret_cast<byte*>(&AuthSession->clientSeed), sizeof(AuthSession->clientSeed));
    hash.Update(reinterpret_cast<byte*>(&_serverChallenge), sizeof(_serverChallenge));
    hash.Update(reinterpret_cast<byte*>(_sessionKey), sizeof(_sessionKey));
    hash.Final(Digest);
    
    if(memcmp(Digest, AuthSession->shaDigest, hash.DigestSize()) != 0)
    {
        //auth failed
    }
    else
    {
        //check if the server can handle any more sessions atm
        //if not - put at queue
        sMaster->AddServiceHandler(AuthSession->accountName, this);
    }
}