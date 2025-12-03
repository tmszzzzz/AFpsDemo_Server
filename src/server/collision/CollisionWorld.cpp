//
// Created by tmsz on 25-12-1.
//

#include "CollisionWorld.h"

#include <fstream>
#include <iostream> // 如果不想要日志可以去掉

namespace collision
{
    // 与 Unity 导出脚本中的 MAGIC / VERSION 保持一致
    static constexpr uint32_t SCOL_MAGIC   = 0x4C4F4353u; // 'SCOL'
    static constexpr uint32_t SCOL_VERSION = 1u;

    bool CollisionWorld::loadFromFile(const std::string& path)
    {
        return LoadCollisionWorld(path, *this);
    }

    bool LoadCollisionWorld(const std::string& path, CollisionWorld& outWorld)
    {
        std::ifstream fs(path, std::ios::binary);
        if (!fs)
        {
            std::cerr << "[CollisionWorld] Failed to open file: " << path << "\n";
            return false;
        }

        // 小工具：安全读取 N 字节
        auto readOrFail = [&fs](void* dst, std::size_t size) -> bool
        {
            fs.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(size));
            return fs.good();
        };

        uint32_t magic = 0;
        uint32_t version = 0;

        if (!readOrFail(&magic, sizeof(magic)) ||
            !readOrFail(&version, sizeof(version)))
        {
            std::cerr << "[CollisionWorld] File too small or read error\n";
            return false;
        }

        if (magic != SCOL_MAGIC)
        {
            std::cerr << "[CollisionWorld] Invalid magic in file: " << path << "\n";
            return false;
        }

        if (version != SCOL_VERSION)
        {
            std::cerr << "[CollisionWorld] Unsupported version " << version
                      << " in file: " << path << "\n";
            return false;
        }

        float worldScale = 1.0f;
        if (!readOrFail(&worldScale, sizeof(worldScale)))
        {
            std::cerr << "[CollisionWorld] Failed to read worldScale\n";
            return false;
        }

        uint32_t boxCount = 0;
        if (!readOrFail(&boxCount, sizeof(boxCount)))
        {
            std::cerr << "[CollisionWorld] Failed to read boxCount\n";
            return false;
        }

        CollisionWorld world;
        world.worldScale = worldScale;
        world.boxes.resize(boxCount);

        for (uint32_t i = 0; i < boxCount; ++i)
        {
            Obb& b = world.boxes[i];

            // center
            if (!readOrFail(&b.center.x, sizeof(float)) ||
                !readOrFail(&b.center.y, sizeof(float)) ||
                !readOrFail(&b.center.z, sizeof(float)))
            {
                std::cerr << "[CollisionWorld] Failed to read center for box " << i << "\n";
                return false;
            }

            // rotation (quat x,y,z,w)
            if (!readOrFail(&b.rotation.x, sizeof(float)) ||
                !readOrFail(&b.rotation.y, sizeof(float)) ||
                !readOrFail(&b.rotation.z, sizeof(float)) ||
                !readOrFail(&b.rotation.w, sizeof(float)))
            {
                std::cerr << "[CollisionWorld] Failed to read rotation for box " << i << "\n";
                return false;
            }

            // halfExtents
            if (!readOrFail(&b.halfExtents.x, sizeof(float)) ||
                !readOrFail(&b.halfExtents.y, sizeof(float)) ||
                !readOrFail(&b.halfExtents.z, sizeof(float)))
            {
                std::cerr << "[CollisionWorld] Failed to read halfExtents for box " << i << "\n";
                return false;
            }

            // flags
            if (!readOrFail(&b.flags, sizeof(b.flags)))
            {
                std::cerr << "[CollisionWorld] Failed to read flags for box " << i << "\n";
                return false;
            }
        }

        // 读成功，替换输出
        outWorld = std::move(world);
        return true;
    }
}
