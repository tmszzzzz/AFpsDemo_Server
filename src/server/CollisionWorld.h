//
// Created by tmsz on 25-12-1.
//

#ifndef DEMO0_SERVER_COLLISIONWORLD_H
#define DEMO0_SERVER_COLLISIONWORLD_H


#include <cstdint>
#include <string>
#include <vector>

namespace server
{
    struct Vec3
    {
        float x, y, z;
    };

    struct Quat
    {
        float x, y, z, w;
    };

    // 与 Unity 导出的 BoxCollider 对应的 OBB
    struct Obb
    {
        Vec3  center;      // 世界空间中心
        Quat  rotation;    // 世界空间旋转(四元数 x,y,z,w)
        Vec3  halfExtents; // 世界空间半尺寸
        uint32_t flags;    // bit0 = Walkable
    };

    struct CollisionWorld
    {
        float worldScale = 1.0f;       // 目前 Unity 端固定写 1.0f
        std::vector<Obb> boxes;        // 所有静态 Box

        // 便利函数：从文件加载
        bool loadFromFile(const std::string& path);
    };

    // 独立函数形式的接口（你也可以只用成员函数）
    bool LoadCollisionWorld(const std::string& path, CollisionWorld& outWorld);
}



#endif //DEMO0_SERVER_COLLISIONWORLD_H
