//
// Created by tmsz on 25-11-27.
//

// server/GameServer.cpp
#include "GameServer.h"
#include "../protocol/Serializer.h"
#include "CollisionWorld.h"
#include <iostream>

GameServer::GameServer()
{
    const std::string path = "res/world.scol";
    if (!g_collisionWorld.loadFromFile(path))
    {
        std::cerr << "Failed to load collision world from " << path << "\n";
    }

    std::cout << "Loaded collision world: "
              << g_collisionWorld.boxes.size() << " boxes\n";

    std::cout << "GameServer created\n";
}

void GameServer::OnMessage(uint32_t connectionId, const proto::Message& msg)
{
    using proto::MsgId;
    auto id = static_cast<MsgId>(msg.header.msgId);

    switch (id)
    {
        case MsgId::JoinRequest:
            handleJoinRequest(connectionId, msg);
            break;
        case MsgId::Ping:
            handlePing(connectionId, msg);
            break;
        default:
            std::cout << "Unknown msgId=" << msg.header.msgId
                      << " from conn=" << connectionId << "\n";
            break;
    }
}

void GameServer::Tick(float dt)
{
    // 简单地用 tick 累加 serverTime，真实项目可用 chrono
    _serverTime += static_cast<uint32_t>(dt * 1000.0f);
}

void GameServer::CollectOutgoing(std::vector<OutgoingPacket>& out)
{
    out.insert(out.end(),
               std::make_move_iterator(_outgoing.begin()),
               std::make_move_iterator(_outgoing.end()));
    _outgoing.clear();
}

void GameServer::OnConnectionClosed(uint32_t connectionId)
{
    auto it = _clients.find(connectionId);
    if (it != _clients.end())
    {
        std::cout << "Player " << it->second.playerId
                  << " disconnected (conn=" << connectionId << ")\n";
        _clients.erase(it);
    }
}

void GameServer::handleJoinRequest(uint32_t connectionId, const proto::Message& msg)
{
    proto::JoinRequest req{};
    if (!proto::DecodeJoinRequest(msg, req))
    {
        std::cout << "Failed to decode JoinRequest from conn="
                  << connectionId << "\n";
        return;
    }

    uint32_t playerId = _nextPlayerId++;
    _clients[connectionId] = ClientInfo{ playerId };

    std::cout << "JoinRequest from conn=" << connectionId
              << " name=" << req.playerName
              << " -> playerId=" << playerId << "\n";

    // 回 JoinAccept
    proto::JoinAccept acc{};
    acc.playerId = playerId;
    acc.serverProtocolVersion = req.protocolVersion; // 暂时直接回同版本

    proto::Message resp{};
    proto::EncodeJoinAccept(acc, resp);

    // 序列化为字节（头 + payload）
    std::vector<uint8_t> bytes;
    bytes.reserve(resp.header.length);
    // 写 header
    proto::ByteWriter w(bytes);
    w.writeU16(resp.header.length);
    w.writeU16(resp.header.msgId);
    w.writeU32(resp.header.seq);
    // 再写 payload
    bytes.insert(bytes.end(), resp.payload.begin(), resp.payload.end());

    OutgoingPacket pkt{};
    pkt.isTcp       = true;
    pkt.connectionId = connectionId;
    pkt.bytes       = std::move(bytes);

    _outgoing.push_back(std::move(pkt));
}

void GameServer::handlePing(uint32_t connectionId, const proto::Message& msg)
{
    proto::Ping ping{};
    if (!proto::DecodePing(msg, ping))
    {
        std::cout << "Failed to decode Ping from conn="
                  << connectionId << "\n";
        return;
    }

    proto::Pong pong{};
    pong.clientTime = ping.clientTime;
    pong.serverTime = _serverTime;

    proto::Message resp{};
    proto::EncodePong(pong, resp);

    std::vector<uint8_t> bytes;
    bytes.reserve(resp.header.length);
    proto::ByteWriter w(bytes);
    w.writeU16(resp.header.length);
    w.writeU16(resp.header.msgId);
    w.writeU32(resp.header.seq);
    bytes.insert(bytes.end(), resp.payload.begin(), resp.payload.end());

    OutgoingPacket pkt{};
    pkt.isTcp       = false; // 走 UDP 回
    pkt.connectionId = connectionId;
    pkt.bytes       = std::move(bytes);

    _outgoing.push_back(std::move(pkt));
}

bool GameServer::FindConnIdByPlayerId(uint32_t playerId, uint32_t& outConnId) const
{
    for (const auto& kv : _clients)
    {
        uint32_t connId = kv.first;
        const ClientInfo& info = kv.second;
        if (info.playerId == playerId)
        {
            outConnId = connId;
            return true;
        }
    }
    return false;
}

