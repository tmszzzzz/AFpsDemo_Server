//
// Created by tmsz on 25-11-27.
//

// net/NetTransport.cpp
#include "NetTransport.h"
#include "../protocol/Serializer.h"
#include <iostream>

using asio::ip::tcp;
using asio::ip::udp;

NetTransport::NetTransport(asio::io_context& io,
                           uint16_t tcpPort,
                           uint16_t udpPort,
                           GameServer& gameServer)
        : _io(io)
        , _acceptor(io, tcp::endpoint(tcp::v4(), tcpPort))
        , _udpSocket(io, udp::endpoint(udp::v4(), udpPort))
        , _gameServer(gameServer)
{
    std::cout << "NetTransport listening TCP:" << tcpPort
              << " UDP:" << udpPort << "\n";
}

void NetTransport::Start()
{
    doAccept();
    startUdpReceive();
}

void NetTransport::Poll()
{
    // MT-0 下，如果 main 调用了 io_context.run()，这里可以为空
}

// 接受新 TCP 连接
void NetTransport::doAccept()
{
    auto conn = std::make_shared<Connection>(
            Connection{ _nextConnId++, tcp::socket(_io), {}, {}, 0 });

    _acceptor.async_accept(conn->socket,
                           [this, conn](const asio::error_code& ec)
                           {
                               onAccepted(conn, ec);
                           });
}

void NetTransport::onAccepted(std::shared_ptr<Connection> conn,
                              const asio::error_code& ec)
{
    if (ec)
    {
        std::cout << "Accept error: " << ec.message() << "\n";
        return;
    }

    std::cout << "New TCP connection id=" << conn->id << "\n";
    _connections[conn->id] = conn;
    startTcpRead(conn);

    // 继续接受下一条连接
    doAccept();
}

// TCP：从头开始读取 length + msgId + seq
void NetTransport::startTcpRead(std::shared_ptr<Connection> conn)
{
    conn->recvBuffer.resize(sizeof(proto::MsgHeader));
    asio::async_read(conn->socket,
                     asio::buffer(conn->recvBuffer),
                     [this, conn](const asio::error_code& ec, std::size_t bytes)
                     {
                         handleTcpReadHeader(conn, ec, bytes);
                     });
}

void NetTransport::handleTcpReadHeader(std::shared_ptr<Connection> conn,
                                       const asio::error_code& ec,
                                       std::size_t bytes)
{
    if (ec)
    {
        std::cout << "Read header error, conn=" << conn->id
                  << " : " << ec.message() << "\n";
        _gameServer.OnConnectionClosed(conn->id);
        _connections.erase(conn->id);
        return;
    }
    if (bytes != sizeof(proto::MsgHeader))
    {
        std::cout << "Invalid header size\n";
        return;
    }

    proto::ByteReader r(conn->recvBuffer.data(), conn->recvBuffer.size());
    uint16_t length = 0, msgId = 0;
    uint32_t seq = 0;
    r.readU16(length);
    r.readU16(msgId);
    r.readU32(seq);

    conn->expectedLength = length;
    size_t payloadSize = length > sizeof(proto::MsgHeader)
                         ? length - sizeof(proto::MsgHeader)
                         : 0;

    conn->recvBuffer.resize(payloadSize);
    if (payloadSize == 0)
    {
        // 空 payload，直接处理
        proto::Message m{};
        m.header.length = length;
        m.header.msgId  = msgId;
        m.header.seq    = seq;
        _gameServer.OnMessage(conn->id, m);
        startTcpRead(conn);
        return;
    }

    asio::async_read(conn->socket,
                     asio::buffer(conn->recvBuffer),
                     [this, conn, length, msgId, seq](const asio::error_code& ec, std::size_t bytes)
                     {
                         handleTcpReadBody(conn, ec, bytes, length, msgId, seq);
                     });
}

void NetTransport::handleTcpReadBody(std::shared_ptr<Connection> conn,
                                     const asio::error_code& ec,
                                     std::size_t bytes,
                                     uint16_t length,
                                     uint16_t msgId,
                                     uint32_t seq)
{
    if (ec)
    {
        std::cout << "Read body error, conn=" << conn->id
                  << " : " << ec.message() << "\n";
        _gameServer.OnConnectionClosed(conn->id);
        _connections.erase(conn->id);
        return;
    }

    proto::Message m{};
    m.header.length = length;
    m.header.msgId  = msgId;
    m.header.seq    = seq;
    m.payload       = std::move(conn->recvBuffer);

    _gameServer.OnMessage(conn->id, m);
    startTcpRead(conn);
}

// UDP 接收 Ping/Pong（和以后更多 UDP 消息）
void NetTransport::startUdpReceive()
{
    // 这里为了简单，用一个局部缓冲 + endpoint
    auto buf = std::make_shared<std::array<uint8_t, 1024>>();
    auto sender = std::make_shared<udp::endpoint>();

    _udpSocket.async_receive_from(
            asio::buffer(*buf), *sender,
            [this, buf, sender](const asio::error_code& ec, std::size_t bytes)
            {
                if (!ec && bytes >= sizeof(proto::MsgHeader))
                {
                    // 解析 header
                    proto::ByteReader r(buf->data(), bytes);
                    uint16_t length = 0, msgId = 0;
                    uint32_t seq = 0;
                    r.readU16(length);
                    r.readU16(msgId);
                    r.readU32(seq);

                    size_t payloadSize = length > sizeof(proto::MsgHeader)
                                         ? length - sizeof(proto::MsgHeader)
                                         : 0;
                    proto::Message m{};
                    m.header.length = length;
                    m.header.msgId  = msgId;
                    m.header.seq    = seq;
                    if (payloadSize > 0 && payloadSize <= bytes - sizeof(proto::MsgHeader))
                    {
                        m.payload.assign(buf->data() + sizeof(proto::MsgHeader),
                                         buf->data() + sizeof(proto::MsgHeader) + payloadSize);
                    }

                    using proto::MsgId;
                    MsgId id = static_cast<MsgId>(msgId);

                    if (id == MsgId::UdpBind)
                    {
                        // UDP 绑定消息：payload 中带 playerId
                        proto::UdpBind bind{};
                        if (proto::DecodeUdpBind(m, bind))
                        {
                            uint32_t connId = 0;
                            if (_gameServer.FindConnIdByPlayerId(bind.playerId, connId))
                            {
                                _udpEndpoints[connId]    = *sender;
                                _endpointToConn[*sender] = connId;
                                std::cout << "UdpBind playerId=" << bind.playerId
                                          << " -> connId=" << connId << "\n";
                            }
                            else
                            {
                                std::cout << "UdpBind with unknown playerId="
                                          << bind.playerId << "\n";
                            }
                        }
                    }
                    else
                    {
                        // 普通 UDP 消息：通过 endpoint 反查 connId
                        auto it = _endpointToConn.find(*sender);
                        if (it != _endpointToConn.end())
                        {
                            uint32_t connId = it->second;
                            _gameServer.OnMessage(connId, m);
                        }
                        else
                        {
                            std::cout << "UDP msg from unknown endpoint, msgId="
                                      << msgId << " bytes=" << bytes << "\n";
                        }
                    }
                }

                // 继续收
                startUdpReceive();
            });
}

// 由 main 循环密集调用：把 GameServer 生成的 OutgoingPacket 发出去
void NetTransport::sendPackets(const std::vector<OutgoingPacket>& packets)
{
    for (auto& pkt : packets)
    {
        auto it = _connections.find(pkt.connectionId);
        if (it == _connections.end()) continue;

        if (pkt.isTcp)
        {
            auto conn = it->second;
            asio::async_write(conn->socket,
                              asio::buffer(pkt.bytes),
                              [id = conn->id](const asio::error_code& ec, std::size_t)
                              {
                                  if (ec)
                                      std::cout << "TCP send error, conn=" << id
                                                << " : " << ec.message() << "\n";
                              });
        }
        else
        {
            auto epIt = _udpEndpoints.find(pkt.connectionId);
            if (epIt == _udpEndpoints.end())
                continue;
            _udpSocket.async_send_to(asio::buffer(pkt.bytes),
                                     epIt->second,
                                     [](const asio::error_code& ec, std::size_t)
                                     {
                                         if (ec)
                                             std::cout << "UDP send error: " << ec.message() << "\n";
                                     });
        }
    }
}
