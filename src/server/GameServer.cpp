//
// Created by tmsz on 25-11-27.
//

#include "GameServer.h"
#include "../protocol/Serializer.h"
#include "kcc/KCC.h"
#include "gameplay/movement/sources/NetworkInputMovementSource.h"
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
    if (dt <= 0.0f)
        return;

    // 1. 维护简单的 serverTime（保留原有行为）
    _serverTime += static_cast<uint32_t>(dt * 1000.0f);

    // 2. 准备 KCC 设置和胶囊体参数（当前版本所有英雄共用一份）
    static kcc::Settings s_kccSettings{};
    static const float kCapsuleRadius     = 0.5f;
    static const float kCapsuleHalfHeight = 0.9f;

    // 3. 遍历所有玩家，驱动 HeroCore + KCC
    for (auto& kv : _clients) {
        ClientInfo &info = kv.second;
        if (!info.hero)
            continue;

        hero::HeroCore &core = *info.hero;

        // 3.1 由 HeroCore 合成 MovementCommand，并计算“理想位移”（不含碰撞）
        movement::MovementCommand cmd = movement::MovementCommand::CreateEmpty();
        Vec3 desiredDisplacement = Vec3::zero();

        core.TickMovement(dt, cmd, desiredDisplacement);

        // 3.2 使用 KCC 处理碰撞 / 贴地 / 陡坡滑落等，回写 PlayerState.Position/IsGrounded
        movement::PlayerState &mvState = core.Movement();

        kcc::MovePlayer(
                g_collisionWorld,
                mvState,
                desiredDisplacement,
                s_kccSettings,
                kCapsuleRadius,
                kCapsuleHalfHeight
        );

        // - mvState.Velocity / Yaw / Pitch 已在 CharacterMotor 内部更新
        // - IsGrounded 在 KCC 中根据地面情况写回
        // - pendingButtons 在 NetworkInputMovementSource 中已被消费并清零
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

    // 简单起见，先使用一组通用配置；后续可根据 heroId 做表驱动
    hero::HeroConfig heroCfg{};
    heroCfg.MaxHp                        = 200.0f;
    heroCfg.Gravity                      = 20.0f;
    heroCfg.MinPitch                     = -89.0f;
    heroCfg.MaxPitch                     =  89.0f;
    heroCfg.HorizontalAccelerationGround = 80.0f;
    heroCfg.HorizontalDecelerationGround = 60.0f;
    heroCfg.HorizontalAccelerationAir    = 30.0f;
    heroCfg.HorizontalDecelerationAir    = 20.0f;

    // TODO: 根据关卡/出生点系统决定 spawnPos，这里先用原点附近占位
    Vec3 spawnPos{0.0f, 1.0f, 0.0f};

    info.hero = std::make_unique<hero::HeroCore>(
            hero::HeroId::Generic,
            spawnPos,
            heroCfg
    );

    // 为这个 Hero 挂一个 NetworkInputMovementSource
    movement::NetworkInputMovementSource::NetInputBuffer buffer{};
    buffer.lastInput      = &info.lastInput;
    buffer.pendingButtons = &info.pendingButtons;

    auto netInputSource =
            std::make_shared<movement::NetworkInputMovementSource>(buffer);

    info.hero->AddMovementSource(netInputSource);

    info.lastInput.playerId = playerId;
    _clients[connectionId] = std::move(info);

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

// ==== 修改: GameServer::handleInputCommand ====
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
        // 测试阶段先不丢弃，仍然覆盖
    }

    // 3. 覆盖“状态型输入”（轴 / 视角）
    info.lastInput = cmd;

    // 4. 对“事件型输入”（按钮）做 OR 累积：自上次 Tick 以来按过的按钮都不会丢
    info.pendingButtons |= cmd.buttonMask;
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

