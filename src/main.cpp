// src/main.cpp
#include "server/GameServer.h"
#include "net/NetTransport.h"
#include <asio.hpp>
#include <chrono>
#include <thread>

int main()
{
    asio::io_context io;
    GameServer gameServer;

    const uint16_t tcpPort = 5000;
    const uint16_t udpPort = 5001;

    NetTransport net(io, tcpPort, udpPort, gameServer);
    net.Start();

    // 简单的单线程循环（MT-0）
    auto last = std::chrono::steady_clock::now();

    while (true)
    {
        io.poll(); // 处理网络事件（也可以用 io.run_one()）

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;

        gameServer.Tick(dt);

        std::vector<OutgoingPacket> out;
        gameServer.CollectOutgoing(out);
        net.sendPackets(out);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

