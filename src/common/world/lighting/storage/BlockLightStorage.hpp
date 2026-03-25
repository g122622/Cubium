#pragma once

#include "SectionLightStorage.hpp"
#include <unordered_map>

namespace mc {

/**
 * @brief 方块光照存储（使用 SWMR）
 *
 * 管理方块光照的数据存储，使用单写多读 Nibble 数组。
 */
class BlockLightStorage : public SectionLightStorage<BlockLightDataMap> {
public:
    explicit BlockLightStorage(IChunkLightProvider* provider);

    // ========================================================================
    // 光照访问
    // ========================================================================

    /**
     * @brief 获取指定位置的光照等级，如果不存在返回0
     */
    [[nodiscard]] u8 getLightOrDefault(i64 worldPos) const override;

    /**
     * @brief 获取指定位置的实际光照等级
     */
    [[nodiscard]] u8 getLight(i64 worldPos) const;

    /**
     * @brief 设置指定位置的光照等级
     */
    void setLight(i64 worldPos, u8 light);

private:
    [[nodiscard]] const SWMRNibbleArray* getArrayInSection(i64 sectionPos, bool useCache) const;
};

} // namespace mc
