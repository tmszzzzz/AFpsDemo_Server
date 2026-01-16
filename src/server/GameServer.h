//
// Created by tmsz on 25-11-27.
//

#ifndef DEMO0_SERVER_GAMESERVER_H
#define DEMO0_SERVER_GAMESERVER_H


#include "../protocol/Messages.h"
#include "gameplay/hero/HeroEntity.h"
#include "collision/CollisionWorld.h"
#include "collision/PhysicsWorld.h"
#include "gameplay/projectile/ProjectileSystem.h"
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>

struct OutgoingPacket
{
    bool isTcp;
    uint32_t connectionId;
    std::vector<uint8_t> bytes;
};

class GameServer
{
public:
    GameServer();

    // 供 NetTransport 调用：接收到一条完整消息
    void OnMessage(uint32_t connectionId, const proto::Message& msg);

    // 逻辑 tick（里程碑 2 可以只用于维护时间）
    void Tick(float dt);

    // 获取待发送的数据
    void CollectOutgoing(std::vector<OutgoingPacket>& out);

    // 连接关闭时通知
    void OnConnectionClosed(uint32_t connectionId);

    // 用playerId反查connId
    bool FindConnIdByPlayerId(uint32_t playerId, uint32_t& outConnId) const;

    collision::CollisionWorld g_collisionWorld;
    collision::PhysicsWorld _physicsWorld;

private:
    struct ClientInfo {
        uint32_t playerId = 0;

        // 网络输入缓冲（网络层）：最新状态 + 跨 tick 聚合的 down-edge
        proto::InputCommand lastInput{};
        uint32_t pendingButtons = 0;

        // Gameplay 输入视图（每 tick 由 lastInput/pendingButtons 生成）
        ServerInputFrame inputFrame{};

        // 该连接对应的英雄核心（一个玩家一个 HeroEntity）
        std::unique_ptr<gameplay::HeroEntity> hero;

    };

    uint32_t _nextPlayerId = 1;
    uint32_t _serverTime   = 0; // 简单计数用作 serverTime

    // [M3-2.6] 服务器帧序号与快照定时器
    uint32_t _serverTick    = 0;    // 当前已经生成的快照帧号
    float    _snapshotTimer = 0.0f; // 累积 dt，用于控制快照频率

    std::unordered_map<uint32_t, ClientInfo> _clients; // connId -> info

    std::vector<OutgoingPacket> _outgoing; // 每 tick 挤出来的待发包

    projectile::ProjectileSystem _projectiles;

    void handleJoinRequest(uint32_t connectionId, const proto::Message& msg);
    void handlePing(uint32_t connectionId, const proto::Message& msg);
    void handleInputCommand(uint32_t connectionId, const proto::Message& msg);

    // [M3-2.6] 构造 WorldSnapshot 的工具函数
    void buildWorldSnapshot(proto::WorldSnapshot& outSnapshot);

    // GameEvent 广播
    void BroadcastGameEvent(const proto::GameEvent& ev, bool reliableTcp = false);
};



#endif //DEMO0_SERVER_GAMESERVER_H
