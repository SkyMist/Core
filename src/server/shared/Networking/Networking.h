#ifndef _NETWORKING_H
#define _NETWORKING_H

#pragma once
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include "Common.h"
#include "Packets/Packets.h"
#include "../../../../dep/cryptopp/cryptlib.h"
#include "../../../../dep/cryptopp/arc4.h"
#include "../../../../dep/cryptopp/sha.h"
#include "../../../../dep/cryptopp/hmac.h"

namespace Networking
{
    void AcceptConnection(uv_stream_t* server, int status);
    void AllocateCallback(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    void ReadCallback(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    void WriteCallback(uv_write_t* req, int status);
    void CloseCallback(uv_shutdown_t* req, int status);
    
    class Server
    {
        protected:
            int StartNetworking(std::string Address, uint16 port);
            void StopNetworking();
        
        public:
            virtual void CreateServiceHandlerInstance(uv_tcp_t* ClientSocket);
            void SetEventLoop(uv_loop_t& Loop);
        
        private:
            uv_loop_t _loop;
            uv_tcp_t _acceptSocket;
    };

    class ServiceHandler
    {
        public:
            ServiceHandler(uv_tcp_t* ClientSocket);
            ~ServiceHandler();
        
            void OnDataReceived(const uv_buf_t* Data, std::size_t Size);
        
        protected:
            void SendPacket(Packets::NormalServerPacket& Packet);
            void SendPacket(Packets::SetupServerPacket& Packet);
        
            virtual void ProcessMessage(Packets::NormalClientPacket* Packet);
            void ProcessSetupMessage(Packets::SetupClientPacket* Packet);
        
            virtual void Authenticate(Packets::SetupClientPacket* Packet);
        
        private:
            void WriteToSocket(uv_buf_t* Buffer);
            void InitializeCipher();
        
        protected:
            uint32 _serverChallenge;
            uint8 _sessionKey[40];
            uint8* _cipherSeed;
            std::size_t _cipherSize;
        
        private:
            uv_stream_t* _clientSocket;
            bool _isInitialized;
            bool _isAuthenticated;
            CryptoPP::Weak::ARC4 _encryptCipher;
            CryptoPP::Weak::ARC4 _decryptCipher;
    };
}

#endif