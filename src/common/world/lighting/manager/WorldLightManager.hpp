#pragma once

#include "../../block/BlockPos.hpp"
#include "../../chunk/ChunkPos.hpp"
#include "../../../util/NibbleArray.hpp"
#include "../IChunkLightProvider.hpp"
#include "../LightType.hpp"

#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 世界光照管理器
 *
 * 这是对外唯一可见的光照门面，内部的光照存储和传播细节都被隐藏在实现文件里。
 */
class WorldLightManager {
public:
    /**
     * @brief 构造函数
     * @param provider 光照数据提供者
     * @param hasBlockLight 是否有方块光照
     * @param hasSkyLight 是否有天空光照
     */
    WorldLightManager(IChunkLightProvider* provider, bool hasBlockLight, bool hasSkyLight);
    ~WorldLightManager();

    WorldLightManager(const WorldLightManager&) = delete;
    WorldLightManager& operator=(const WorldLightManager&) = delete;
    WorldLightManager(WorldLightManager&&) noexcept;
    WorldLightManager& operator=(WorldLightManager&&) noexcept;

    // ========================================================================
    // 光照操作
    // ========================================================================

    void checkBlock(i32 x, i32 y, i32 z);
    void onBlockEmissionIncrease(i32 x, i32 y, i32 z, i32 lightLevel);
    [[nodiscard]] bool hasLightWork() const;
    i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight);

    // ========================================================================
    // 区块段管理
    // ========================================================================

    void updateSectionStatus(const SectionPos& pos, bool isEmpty);
    void enableLightSources(const ChunkPos& pos, bool enable);

    // ========================================================================
    // 光照访问
    // ========================================================================

    [[nodiscard]] bool hasBlockLight() const;
    [[nodiscard]] bool hasSkyLight() const;
    [[nodiscard]] i32 getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const;
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const;
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const;

    // ========================================================================
    // 数据管理
    // ========================================================================

    void setData(LightType type, const SectionPos& pos, const NibbleArray& array, bool retain);
    [[nodiscard]] std::vector<u8> getData(LightType type, const SectionPos& pos) const;
    void retainData(const ChunkPos& pos, bool retain);

    // ========================================================================
    // 调试信息
    // ========================================================================

    [[nodiscard]] String getDebugInfo(LightType type, const SectionPos& pos) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace mc
