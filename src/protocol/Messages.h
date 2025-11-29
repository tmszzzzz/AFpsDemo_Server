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
        JoinRequest = 1,
        JoinAccept  = 2,
        Ping        = 10,
        Pong        = 11,
        UdpBind     = 20,
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

    // 泛型消息包装
    struct Message
    {
        MsgHeader header{};
        std::vector<uint8_t> payload; // 仅存 payload，不含 header 本身
    };
}


#endif //DEMO0_SERVER_MESSAGES_H
