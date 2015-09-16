#include "Networking.h"
#include "Packets/Auth.h"
#include "Log.h"
#include "../../../../dep/cryptopp/cryptlib.h"
#include "../../../../dep/cryptopp/osrng.h"

int Networking::Server::StartNetworking(std::string Address, uint16 port)
{
    int errno;
    sockaddr_in* ListenAddress = new sockaddr_in;
    uv_ip4_addr(Address.c_str(), port, ListenAddress);
    
    //create the default loop
    uv_loop_init(&_loop);
    
    //create the acceptor socket
    uv_tcp_init(&_loop, &_acceptSocket);
    
    _acceptSocket.data = reinterpret_cast<void*>(this);
    
    if((errno = uv_tcp_bind(&_acceptSocket, reinterpret_cast<sockaddr*>(ListenAddress), 0)) != 0)
    {
        return errno;
    }
    else
    {
        if((errno = uv_listen(reinterpret_cast<uv_stream_t*>(&_acceptSocket), 128, Networking::AcceptConnection)) != 0)
        {
            return errno;
        }
    }
    
    return 0;
}

void Networking::Server::StopNetworking()
{
    uv_close(reinterpret_cast<uv_handle_t*>(&_acceptSocket), NULL);
}

void Networking::Server::CreateServiceHandlerInstance(uv_tcp_t* ClientSocket)
{
    uv_close(reinterpret_cast<uv_handle_t*>(ClientSocket), NULL);
    sLog->outDebug(LOG_FILTER_NETWORKIO, "called the abastact CreateServiceHandler");
}

void Networking::Server::SetEventLoop(uv_loop_t& Loop)
{
    _loop = Loop;
}

void Networking::AcceptConnection(uv_stream_t* server, int status)
{
    if(status == 0)
    {
        uv_tcp_t* ClientSocket = new uv_tcp_t;
        uv_tcp_init(server->loop, ClientSocket);
        uv_accept(server, reinterpret_cast<uv_stream_t*>(ClientSocket));
        reinterpret_cast<Networking::Server*>(server)->CreateServiceHandlerInstance(ClientSocket);
    }
}

void Networking::AllocateCallback(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    buf = new uv_buf_t;
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
}

void Networking::ReadCallback(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    if(nread > 0)
    {
        reinterpret_cast<Networking::ServiceHandler*>(stream->data)->OnDataReceived(buf, nread);
    }
}

void Networking::WriteCallback(uv_write_t* req, int status)
{
    uv_buf_t* Buffer = reinterpret_cast<uv_buf_t*>(req->data);
    //if (status == 0)
    //{
        delete[] Buffer->base;
        delete Buffer;
    //}
    delete req;
}

void Networking::CloseCallback(uv_shutdown_t* req, int status)
{
    delete req;
}

Networking::ServiceHandler::ServiceHandler(uv_tcp_t* ClientSocket)
{
    _clientSocket = reinterpret_cast<uv_stream_t*>(ClientSocket);
    _clientSocket->data = reinterpret_cast<void*>(this);
    _isInitialized, _isAuthenticated = false;
    
    const char Greeting[] = "WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT";
    uv_buf_t* Buffer = new uv_buf_t;
    Buffer->base = new char[sizeof(Greeting) + 2];
    Buffer->len = sizeof(Greeting) + 2;
    *reinterpret_cast<uint16*>(Buffer->base) = sizeof(Greeting);
    memcpy(Buffer->base+2, Greeting, sizeof(Greeting));
    
    WriteToSocket(Buffer);
}


Networking::ServiceHandler::~ServiceHandler()
{
    uv_shutdown_t* shutdownReq = new uv_shutdown_t;
    uv_shutdown(shutdownReq, reinterpret_cast<uv_stream_t*>(&_clientSocket), Networking::CloseCallback);
    uv_read_stop(reinterpret_cast<uv_stream_t*>(&_clientSocket));
    uv_close(reinterpret_cast<uv_handle_t*>(&_clientSocket), NULL);
}

void Networking::ServiceHandler::OnDataReceived(const uv_buf_t* Data, std::size_t Size)
{
    if (!_isInitialized)
    {
        std::size_t BufferSize = static_cast<std::size_t>(*reinterpret_cast<uint16*>(Data->base));
        std::string ClientGreeting = reinterpret_cast<const char*>(Data->base+2);
        
        if (BufferSize == 48 && ClientGreeting.compare("WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER"))
        {
            _isInitialized = true;
            
            CryptoPP::AutoSeededRandomPool rng;
            _serverChallenge = rng.GenerateWord32();
            
            Packets::Auth::AuthChallenge Challenge;
            
            Challenge.challenge = _serverChallenge;
            memcpy(Challenge.DoSChallenge, _cipherSeed, _cipherSize);
            
            SendPacket(Challenge);
        }
        else
        {
            //close the connection, the greeting wasn't transmited properly
        }
    }
    else
    {
        if(_isAuthenticated)
        {
            _decryptCipher.ProcessString(reinterpret_cast<byte*>(Data->base), sizeof(Packets::NormalHeader));
            Packets::NormalClientPacket* Packet = new Packets::NormalClientPacket(Data);
            ProcessMessage(Packet);
        }
        else
        {
            Packets::SetupClientPacket* Packet = new Packets::SetupClientPacket(Data);
            ProcessSetupMessage(Packet);
        }
    }
}

void Networking::ServiceHandler::SendPacket(Packets::NormalServerPacket& Packet)
{
    Packet.Process();
    _encryptCipher.ProcessString(reinterpret_cast<byte*>(Packet.ToBuffer()->base), sizeof(Packets::NormalHeader));
    WriteToSocket(Packet.ToBuffer());
}

void Networking::ServiceHandler::SendPacket(Packets::SetupServerPacket& Packet)
{
    Packet.Process();
    WriteToSocket(Packet.ToBuffer());
}

void Networking::ServiceHandler::WriteToSocket(uv_buf_t* Buffer)
{
    uv_write_t* writeRequest = new uv_write_t;
    writeRequest->data = reinterpret_cast<void*>(Buffer);
    uv_write(writeRequest, _clientSocket, Buffer, 1, Networking::WriteCallback);
}

void Networking::ServiceHandler::ProcessSetupMessage(Packets::SetupClientPacket *Packet)
{
    switch (Packet->GetOpcode())
    {
        case Packets::CMSG_AUTH_SESSION:
        {
            try {
                Authenticate(Packet);
            } catch (std::runtime_error& authError) {
                sLog->outError(LOG_FILTER_CHARACTER, authError.what());
            }
            break;
        }
    }
}

void Networking::ServiceHandler::Authenticate(Packets::SetupClientPacket* Packet)
{
    throw new std::runtime_error("No Authentication mechanism provided for session");
}

void Networking::ServiceHandler::InitializeCipher()
{
    std::size_t SeedSize = _cipherSize >> 1;
    byte* dropBuffer = new byte[1024];
    
    byte* DecryptSeed = new byte[SeedSize];
    memcpy(DecryptSeed, &_cipherSeed[15], SeedSize);
    
    byte* EncryptSeed = new byte[SeedSize];
    memcpy(EncryptSeed, &_cipherSeed[0], SeedSize);
    
    CryptoPP::HMAC<CryptoPP::SHA1> DecryptHMAC(DecryptSeed, SeedSize);
    byte* DecryptSeedDigest = new byte[DecryptHMAC.DIGESTSIZE];
    
    DecryptHMAC.Update(_sessionKey, sizeof(_sessionKey));
    DecryptHMAC.Final(DecryptSeedDigest);
    _decryptCipher = CryptoPP::Weak::ARC4(DecryptSeedDigest, DecryptHMAC.DIGESTSIZE);
    memset(dropBuffer, 0, 1024);
    _decryptCipher.ProcessString(dropBuffer, 1024);
    delete[] DecryptSeed;
    
    CryptoPP::HMAC<CryptoPP::SHA1> EncryptHMAC(EncryptSeed, SeedSize);
    byte* EncryptSeedDigest = new byte[EncryptHMAC.DIGESTSIZE];
    
    EncryptHMAC.Update(_sessionKey, sizeof(_sessionKey));
    EncryptHMAC.Final(EncryptSeedDigest);
    _encryptCipher = CryptoPP::Weak::ARC4(EncryptSeedDigest, EncryptHMAC.DIGESTSIZE);
    memset(dropBuffer, 0, 1024);
    _encryptCipher.ProcessString(dropBuffer, 1024);
    delete[] EncryptSeed;
    
    delete[] dropBuffer;
}
