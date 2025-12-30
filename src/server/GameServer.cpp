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
    static const float kCapsuleHalfHeight = 0.5f;

    // 3. 遍历所有玩家，驱动 HeroCore + KCC
    for (auto& kv : _clients) {
        ClientInfo &info = kv.second;
        if (!info.hero)
            continue;

        hero::HeroCore &core = *info.hero;

        // 统一消费点：截取本 Tick 按钮快照，并清空跨 Tick 累积容器
        info.buttonsThisTick = info.pendingButtons;
        info.pendingButtons  = 0;

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

        info.prevButtonsDown = info.lastInput.buttonMask;

        // - mvState.Velocity / Yaw / Pitch 已在 CharacterMotor 内部更新
        // - IsGrounded 在 KCC 中根据地面情况写回
    }

    // [M3-2.6] 基于定时器的 WorldSnapshot 广播

    // 累积本帧 dt
    _snapshotTimer += dt;

    // 如果你希望“补帧”，可以用 while；如果不在乎补帧，用 if 也可以。
    while (_snapshotTimer >= SNAPSHOT_INTERVAL_SEC)
    {
        _snapshotTimer -= SNAPSHOT_INTERVAL_SEC;
        ++_serverTick;

        // 1) 从当前所有玩家状态构造一个 WorldSnapshot
        proto::WorldSnapshot snapshot{};
        buildWorldSnapshot(snapshot);

        // 2) 编码为 proto::Message
        proto::Message msg{};
        proto::EncodeWorldSnapshot(snapshot, msg);

        // 3) 序列化为字节（和 handlePing 一样的写法）
        std::vector<uint8_t> bytes;
        bytes.reserve(msg.header.length);

        proto::ByteWriter w(bytes);
        w.writeU16(msg.header.length);
        w.writeU16(msg.header.msgId);
        w.writeU32(msg.header.seq);
        bytes.insert(bytes.end(), msg.payload.begin(), msg.payload.end());

        // 4) 对所有已连接客户端通过 UDP 广播
        for (const auto& kv : _clients)
        {
            const uint32_t connId = kv.first;

            OutgoingPacket pkt{};
            pkt.isTcp        = false;        // 使用 UDP 发送
            pkt.connectionId = connId;
            pkt.bytes        = bytes;        // 复制一份，人数不多问题不大

            _outgoing.push_back(std::move(pkt));
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


    _clients[connectionId] = ClientInfo();

    ClientInfo& info = _clients[connectionId];
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
    buffer.lastInput       = &info.lastInput;
    buffer.buttonsThisTick = &info.buttonsThisTick;
    buffer.prevButtonsDown = &info.prevButtonsDown;

    auto netInputSource =
            std::make_shared<movement::NetworkInputMovementSource>(buffer);

    info.hero->AddMovementSource(netInputSource);

    info.lastInput.playerId = playerId;

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
void GameServer::handleInputCommand(uint32_t connectionId, const proto::Message& msg) {
    // 1. 解析 InputCommand 消息
    proto::InputCommand cmd{};
    if (!proto::DecodeInputCommand(msg, cmd)) {
        std::cout << "Failed to decode InputCommand from conn="
                  << connectionId << "\n";
        return;
    }

    // 2. 查找对应的客户端信息
    auto it = _clients.find(connectionId);
    if (it == _clients.end()) {
        std::cout << "InputCommand from unknown conn=" << connectionId
                  << " playerId=" << cmd.playerId << "\n";
        return;
    }

    ClientInfo &info = it->second;

    // 可选：检查 payload 中的 playerId 是否与服务器记录一致
    if (info.playerId != cmd.playerId) {
        std::cout << "InputCommand playerId mismatch: conn=" << connectionId
                  << " stored=" << info.playerId
                  << " msg=" << cmd.playerId << "\n";
        // 测试阶段先不丢弃，仍然覆盖
    }

    // 3) 计算包级别“按下边沿”：本包按下 & 上一包未按下
    const uint32_t prevPacketDown = info.lastInput.buttonMask;
    const uint32_t downEdge = cmd.buttonMask & ~prevPacketDown;

    // 4) 覆盖最新状态输入
    info.lastInput = cmd;

    // 5) 事件型输入：只累积按下边沿（跨 Tick 不丢）
    info.pendingButtons |= downEdge;

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

// [M3-2.6] 从服务器内部状态构造 WorldSnapshot
void GameServer::buildWorldSnapshot(proto::WorldSnapshot& outSnapshot)
{
    // 当前快照对应的服务器帧号
    outSnapshot.serverTick = _serverTick;

    outSnapshot.players.clear();
    outSnapshot.players.reserve(_clients.size());

    for (const auto& kv : _clients)
    {
        const ClientInfo& info = kv.second;
        if (!info.hero)
            continue;

        const hero::HeroCore&  hero      = *info.hero;
        const hero::HeroState& heroState = hero.GetState();
        const movement::PlayerState& mv  = heroState.playerState;

        proto::PlayerSnapshot ps{};

        // 身份信息
        ps.playerId = info.playerId;
        ps.heroId   = static_cast<uint32_t>(heroState.HeroId);

        // 位置 / 速度 / 朝向（kcc+movement 已经维护好的）
        ps.posX = mv.Position.x;
        ps.posY = mv.Position.y;
        ps.posZ = mv.Position.z;

        ps.velX = mv.Velocity.x;
        ps.velY = mv.Velocity.y;
        ps.velZ = mv.Velocity.z;

        ps.yaw   = mv.Yaw;
        ps.pitch = mv.Pitch;

        // 简单的移动状态（先直接从 PlayerState 映射）
        ps.locomotionState = mv.LocomotionState; // 0=Idle,1=Move（M3 简化）
        ps.actionState     = 0;                  // M3 暂不区分动作状态

        // 技能相关 / 状态旗标：M3 先全部占位 0
        ps.activeSkillSlot  = 0;
        ps.activeSkillPhase = 0;
        ps.statusFlags      = 0;

        // 血量与能量：从 HeroState 中取，简单 clamp 后转为 ushort。
        float hp   = heroState.Hp;
        float maxHp = heroState.MaxHp;
        if (hp < 0.0f)     hp = 0.0f;
        if (hp > maxHp)    hp = maxHp;

        ps.health = static_cast<uint16_t>(hp);   // 假设 MaxHp 不超过 65535
        ps.energy = 0;                           // 还没有能量系统，先 0

        outSnapshot.players.push_back(ps);
    }
}

void GameServer::BroadcastGameEvent(const proto::GameEvent& ev, bool reliableTcp)
{
    proto::Message msg{};
    proto::EncodeGameEvent(ev, msg);

    // 序列化为字节（头 + payload）
    std::vector<uint8_t> bytes;
    bytes.reserve(msg.header.length);

    proto::ByteWriter w(bytes);
    w.writeU16(msg.header.length);
    w.writeU16(msg.header.msgId);
    w.writeU32(msg.header.seq);
    bytes.insert(bytes.end(), msg.payload.begin(), msg.payload.end());

    // 对所有已连接客户端广播
    for (const auto& kv : _clients)
    {
        const uint32_t connId = kv.first;

        OutgoingPacket pkt{};
        pkt.isTcp        = reliableTcp; // [diff] 可选：关键事件走 TCP
        pkt.connectionId = connId;
        pkt.bytes        = bytes;

        _outgoing.push_back(std::move(pkt));
    }
}


