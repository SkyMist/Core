#include "Networking.h"
#include "Packets/Headers.h"
#include "Log.h"

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
    uv_read_stop(_clientSocket);
    
}

void Networking::ServiceHandler::OnDataReceived(const uv_buf_t* Data, std::size_t Size)
{
    if (!_isInitialized)
    {
        std::size_t BufferSize = static_cast<std::size_t>(*reinterpret_cast<uint16*>(Data->base));
        std::string ClientGreeting = reinterpret_cast<const char*>(Data->base+2);
        
        if (BufferSize == 48 && ClientGreeting.compare("WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER"))
            _isInitialized = true;
        
        //send smsg_auth_challenge
    }
    else
    {
        if(_isAuthenticated)
        {
            //use the DWORD type header
        }
        else
        {
            //use the WORD type header
        }
    }
}

void Networking::ServiceHandler::SendPacket(Packets::NormalServerPacket& Packet)
{
    
}

void Networking::ServiceHandler::SendPacket(Packets::SetupServerPacket& Packet)
{
    WriteToSocket(Packet.ToBuffer());
}

void Networking::ServiceHandler::WriteToSocket(uv_buf_t* Buffer)
{
    uv_write_t* writeRequest = new uv_write_t;
    writeRequest->data = reinterpret_cast<void*>(Buffer);
    uv_write(writeRequest, _clientSocket, Buffer, 1, Networking::WriteCallback);
}