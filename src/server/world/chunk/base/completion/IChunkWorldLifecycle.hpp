#pragma once

#include "common/core/Types.hpp"
#include "server/world/gen/spawn/WorldGenSpawner.hpp"
#include <vector>

namespace mc::server {

/** @brief 区块核心向世界层交接激活和退场，核心不依赖 ServerWorld。 */
class IChunkWorldLifecycle {
public:
    virtual ~IChunkWorldLifecycle() = default;

    /** @brief 提交激活消息；世界副作用由主线程推进。 */
    virtual void enqueue(
        ChunkCoord x, ChunkCoord z, std::vector<SpawnedEntityData>&& entities, bool needsPostProcess) = 0;

    /** @brief 确认退场后执行世界通知，期间区块仍然存在。 */
    virtual void notifyUnload(ChunkCoord x, ChunkCoord z) = 0;

    /** @brief 移除已退场生命周期的激活标记。 */
    virtual void forget(ChunkCoord x, ChunkCoord z) = 0;

    /** @brief 判断主线程是否仍需接收激活消息。 */
    virtual bool pending() const = 0;
};

} // namespace mc::server
