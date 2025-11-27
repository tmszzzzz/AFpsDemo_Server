//
// Created by tmsz on 25-11-27.
//

#ifndef DEMO0_SERVER_NETTRANSPORT_H
#define DEMO0_SERVER_NETTRANSPORT_H


// net/NetTransport.h
#include "../protocol/Messages.h"
#include "../server/GameServer.h"
#include <asio.hpp>
#include <memory>
#include <unordered_map>

class NetTransport
{
public:
    NetTransport(asio::io_context& io,
                 uint16_t tcpPort,
                 uint16_t udpPort,
                 GameServer& gameServer);

    void Start();
    void Poll();   // 单线程版本，可以空实现（asio 自己跑）

    void sendPackets(const std::vector<OutgoingPacket>& packets);

private:
    struct Connection
    {
        uint32_t id;
        asio::ip::tcp::socket socket;
        std::vector<uint8_t> recvBuffer;
        std::vector<uint8_t> sendBuffer;

        uint16_t expectedLength = 0;
    };

    asio::io_context& _io;
    asio::ip::tcp::acceptor _acceptor;
    asio::ip::udp::socket   _udpSocket;

    GameServer& _gameServer;

    uint32_t _nextConnId = 1;
    std::unordered_map<uint32_t, std::shared_ptr<Connection>> _connections;
    // UDP 端：connectionId -> endpoint
    std::unordered_map<uint32_t, asio::ip::udp::endpoint> _udpEndpoints;

    void doAccept();
    void onAccepted(std::shared_ptr<Connection> conn,
                    const asio::error_code& ec);

    void startTcpRead(std::shared_ptr<Connection> conn);
    void handleTcpReadHeader(std::shared_ptr<Connection> conn,
                             const asio::error_code& ec, std::size_t bytes);
    void handleTcpReadBody(std::shared_ptr<Connection> conn,
                           const asio::error_code& ec, std::size_t bytes);

    void startUdpReceive();

};



#endif //DEMO0_SERVER_NETTRANSPORT_H
