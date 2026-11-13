#pragma once

#include "server/world/chunk/ServerChunkManager.hpp"
#include "server/world/chunk/lifecycle/ChunkLifecycleController.hpp"

namespace mc::server {

/** @brief 测试显式观察内部状态；生产门面不暴露调度器和注册表。 */
class ChunkSystemTestAccess {
public:
    /** @brief 取得待测生命周期运行时。 */
    static ChunkLifecycleController& lifecycle(ServerChunkManager& manager)
    {
        return *manager.m_lifecycle;
    }
};

} // namespace mc::server
