//
// Created by tmsz on 25-11-27.
//

#include "GameServer.h"
#include "../protocol/Serializer.h"
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
        case MsgId::InputCommand:
            handleInputCommand(connectionId, msg);
            break;
        default:
            std::cout << "Unknown msgId=" << msg.header.msgId
                      << " from conn=" << connectionId << "\n";
            break;
    }
}

void GameServer::Tick(float dt) {
    // 简单地用 tick 累加 serverTime，真实项目可用 chrono
    _serverTime += static_cast<uint32_t>(dt * 1000.0f);

    // --- M3：根据 lastInput 推进 PlayerState（无重力、无碰撞） ---
    static constexpr float kMoveSpeed = 5.0f;          // 统一移动速度
    static constexpr float kInputEps = 1e-3f;         // 输入判零阈值
    static constexpr float kDeg2Rad = 3.1415926535f / 180.0f;

    for (auto &kv: _clients) {
        ClientInfo &info = kv.second;
        movement::PlayerState &st = info.state;
        const proto::InputCommand &in = info.lastInput;

        // 1. 先同步视角（即使不走路，也让朝向跟随输入）
        st.Yaw = in.yaw;
        st.Pitch = in.pitch;

        // 2. 从输入中取本地平面移动向量（XZ 平面上的“摇杆”）
        float moveX = in.moveX; // 本地右轴
        float moveY = in.moveY; // 本地前轴

        float lenSq = moveX * moveX + moveY * moveY;

        Vec3 moveDirWorld{0.0f, 0.0f, 0.0f};

        if (lenSq > kInputEps * kInputEps) {
            // 2.1 归一化本地输入，避免对角线更快
            float len = std::sqrt(lenSq);
            float localX = moveX / len;
            float localZ = moveY / len;

            // 2.2 按 Y 轴旋转到世界坐标：基于当前 Yaw
            float yawRad = st.Yaw * kDeg2Rad;
            float c = std::cos(yawRad);
            float s = std::sin(yawRad);

            // 约定：Z 轴为“前”，X 轴为“右”
            // 本地(X=右,Z=前) 旋转到世界：
            moveDirWorld.x = localX * c + localZ * s;
            moveDirWorld.y = 0.0f;
            moveDirWorld.z = localZ * c - localX * s;
        }

        if (moveDirWorld.x == 0.0f &&
            moveDirWorld.y == 0.0f &&
            moveDirWorld.z == 0.0f) {
            // 3.a 没有输入：速度置零、状态 Idle（保持位置不变）
            st.Velocity = Vec3{0.0f, 0.0f, 0.0f};
            st.LocomotionState = 0; // Idle
        } else {
            // 3.b 有输入：按固定速度在平面上移动
            st.Velocity = moveDirWorld * kMoveSpeed;
            st.Position += st.Velocity * dt;

            st.LocomotionState = 1; // Move
        }
    }
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

void GameServer::handleJoinRequest(uint32_t connectionId, const proto::Message& msg) {
    proto::JoinRequest req{};
    if (!proto::DecodeJoinRequest(msg, req)) {
        std::cout << "Failed to decode JoinRequest from conn="
                  << connectionId << "\n";
        return;
    }

    uint32_t playerId = _nextPlayerId++;

    ClientInfo info{};
    info.playerId = playerId;

    // 可选：让 lastInput 内的 playerId 也对齐，方便后续调试/日志
    info.lastInput.playerId = playerId;
    _clients[connectionId] = info;

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
    pkt.isTcp = true;
    pkt.connectionId = connectionId;
    pkt.bytes = std::move(bytes);

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

void GameServer::handleInputCommand(uint32_t connectionId, const proto::Message& msg)
{
    // 1. 解析 InputCommand 消息
    proto::InputCommand cmd{};
    if (!proto::DecodeInputCommand(msg, cmd))
    {
        std::cout << "Failed to decode InputCommand from conn="
                  << connectionId << "\n";
        return;
    }

    // 2. 查找对应的客户端信息
    auto it = _clients.find(connectionId);
    if (it == _clients.end())
    {
        std::cout << "InputCommand from unknown conn=" << connectionId
                  << " playerId=" << cmd.playerId << "\n";
        return;
    }

    ClientInfo& info = it->second;

    // 可选：检查 payload 中的 playerId 是否与服务器记录一致
    if (info.playerId != cmd.playerId)
    {
        std::cout << "InputCommand playerId mismatch: conn=" << connectionId
                  << " stored=" << info.playerId
                  << " msg=" << cmd.playerId << "\n";
        // 测试阶段先不丢弃，仍然覆盖 lastInput
    }

    // 3. 更新该玩家的 lastInput（供 Tick 使用）
    info.lastInput = cmd;
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

