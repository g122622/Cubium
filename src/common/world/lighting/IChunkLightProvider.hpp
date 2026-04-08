#pragma once

#include "../IWorld.hpp"
#include "../block/BlockPos.hpp"
#include "../chunk/IChunk.hpp"
#include "../chunk/ChunkPos.hpp"
#include "LightType.hpp"

namespace mc {

/**
 * @brief 光照系统需要的区块/世界访问接口
 *
 * 这个接口只给光照实现层使用，不建议外部业务直接依赖。
 */
class IChunkLightProvider {
public:
    virtual ~IChunkLightProvider() = default;

    [[nodiscard]] virtual IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) = 0;
    [[nodiscard]] virtual const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const = 0;
    [[nodiscard]] virtual const BlockState* getBlockStateForLight(const BlockPos& pos) const = 0;
    [[nodiscard]] virtual IWorld* getWorld() = 0;
    [[nodiscard]] virtual const IWorld* getWorld() const = 0;
    virtual void markLightChanged(LightType type, const SectionPos& pos) = 0;
    [[nodiscard]] virtual bool hasSkyLight() const = 0;
    [[nodiscard]] virtual i32 getMinBuildHeight() const = 0;
    [[nodiscard]] virtual i32 getMaxBuildHeight() const = 0;
    [[nodiscard]] virtual i32 getSectionCount() const = 0;
};

} // namespace mc