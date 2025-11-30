//
// Created by tmsz on 25-11-26.
//

#ifndef DEMO0_SERVER_MESSAGES_H
#define DEMO0_SERVER_MESSAGES_H

#include <cstdint>
#include <string>
#include <vector>

namespace proto
{
    // 消息 ID
    enum class MsgId : uint16_t
    {
        JoinRequest = 0x1,
        JoinAccept  = 0x2,
        Ping        = 0x10,
        Pong        = 0x11,
        UdpBind     = 0x20,
        // === M3 新增：玩家输入 / 世界快照 / 事件 ===
        InputCommand  = 0x30,  // 客户端 -> 服务端：输入命令
        WorldSnapshot = 0x40,  // 服务端 -> 客户端：世界状态快照
        GameEvent     = 0x50,  // 服务端 -> 客户端：游戏事件
    };

    // 统一消息头（用于 TCP/UDP）
    struct MsgHeader
    {
        uint16_t length;  // 包总长（含头）
        uint16_t msgId;   // MsgId
        uint32_t seq;     // 包序号（里程碑 2 先不用，填 0 即可）
    };

    // JoinRequest: C -> S
    struct JoinRequest
    {
        uint16_t protocolVersion;
        std::string playerName;
    };

    // JoinAccept: S -> C
    struct JoinAccept
    {
        uint32_t playerId;
        uint16_t serverProtocolVersion;
    };

    // Ping: C -> S（主要走 UDP）
    struct Ping
    {
        uint32_t clientTime; // 客户端自己的时间戳/计数
    };

    // Pong: S -> C
    struct Pong
    {
        uint32_t clientTime; // 原样返回
        uint32_t serverTime; // 服务器时间戳/计数
    };

    // UdpBind: C -> S
    struct UdpBind
    {
        uint32_t playerId; // 希望绑定到 UDP 通道的玩家 ID
    };

    // InputCommand: C -> S
    struct InputCommand
    {
        uint32_t playerId;    // 哪个玩家发来的

        uint16_t seq;         // 输入序号（回滚时会用）
        uint32_t clientTick;  // 客户端本地tick（M3可以先不用）

        float moveX;          // -1..1，水平轴 (A/D)
        float moveY;          // -1..1，垂直轴 (W/S)
        float yaw;            // 朝向
        float pitch;          // 视角 pitch

        uint32_t buttonMask;  // 按键bitmask
    };

    static constexpr uint32_t MOUSE_FIRE_PRI = 1u << 0;
    static constexpr uint32_t MOUSE_FIRE_SEC = 1u << 1;
    static constexpr uint32_t BUTTON_JUMP = 1u << 2;
    static constexpr uint32_t BUTTON_ULTRA = 1u << 3;
    static constexpr uint32_t BUTTON_SKILL_E = 1u << 4;
    static constexpr uint32_t BUTTON_SKILL_SHIFT = 1u << 5;
    static constexpr uint32_t BUTTON_SKILL_CTRL = 1u << 6;
    static constexpr uint32_t BUTTON_HIT_V = 1u << 7;

    // PlayerSnapshot: S -> C
    struct PlayerSnapshot
    {
        uint32_t playerId;
        uint32_t heroId;         // M3先全0

        float posX, posY, posZ;
        float velX, velY, velZ;
        float yaw, pitch;

        uint8_t locomotionState;   // 0=Idle, 1=Move（M3先用这两个）
        uint8_t actionState;       // 0=Normal

        uint8_t activeSkillSlot;   // Dash固定用2
        uint8_t activeSkillPhase;  // 0=未激活, 1=激活中

        uint32_t statusFlags;      // 先全0

        uint16_t health;           // 先写100
        uint16_t energy;           // 先写0
    };

    // WorldSnapshot: S -> C
    struct WorldSnapshot
    {
        uint32_t serverTick;
        std::vector<PlayerSnapshot> players;
    };

    enum class GameEventType : uint8_t
    {
        DashStarted = 1, // M3占位
    };

    //GameEvent: S -> C
    struct GameEvent
    {
        GameEventType type;

        uint32_t serverTick;
        uint32_t casterPlayerId;

        uint8_t  skillSlot;    // Dash 固定 2
        uint8_t  phaseIndex;   // 先固定 0

        uint32_t targetId;     // M3先写0

        float param0;          // 例如 dashDistance
        float param1;          // 先写0或dashDuration占位
    };

    // 泛型消息包装
    struct Message
    {
        MsgHeader header{};
        std::vector<uint8_t> payload; // 仅存 payload，不含 header 本身
    };
}


#endif //DEMO0_SERVER_MESSAGES_H
