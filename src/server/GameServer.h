//
// Created by tmsz on 25-11-27.
//

#ifndef DEMO0_SERVER_GAMESERVER_H
#define DEMO0_SERVER_GAMESERVER_H


#include "../protocol/Messages.h"
#include "collision/CollisionWorld.h"
#include "gameplay/movement/core/PlayerState.h"
#include "gameplay/movement/sources/InputCommandMovementSource.h"
#include "gameplay/heroes/HeroCore.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

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

private:
    struct ClientInfo
    {
        uint32_t playerId = 0;

        // 网络输入缓冲：
        // - lastInput 表示“最新一帧输入状态”
        // - pendingButtons 自上次 Tick 以来 OR 的“按钮事件”
        proto::InputCommand lastInput{};
        uint32_t            pendingButtons = 0;

        // 该连接对应的英雄核心（一个玩家一个 HeroCore）
        std::unique_ptr<hero::HeroCore> hero;

    };

    uint32_t _nextPlayerId = 1;
    uint32_t _serverTime   = 0; // 简单计数用作 serverTime
    std::unordered_map<uint32_t, ClientInfo> _clients; // connId -> info

    std::vector<OutgoingPacket> _outgoing; // 每 tick 挤出来的待发包

    void handleJoinRequest(uint32_t connectionId, const proto::Message& msg);
    void handlePing(uint32_t connectionId, const proto::Message& msg);
    void handleInputCommand(uint32_t connectionId, const proto::Message& msg);
};



#endif //DEMO0_SERVER_GAMESERVER_H
