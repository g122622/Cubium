#pragma once

#include "BlockLightEngine.hpp"
#include "LightEngineUtils.hpp"
#include "../storage/SkyLightStorage.hpp"
#include "../storage/SWMRNibbleArray.hpp"
#include "../../block/Block.hpp"

namespace mc {

/**
 * @brief 天空光照引擎
 *
 * 实现天空光照的传播算法。
 * 天空光照从天空向下传播，在透明方块中传播时不衰减，
 * 只有在不透明方块阻挡时才会衰减。
 *
 * 参考: net.minecraft.world.lighting.SkyLightEngine
 */
class SkyStarLightEngine : public StarLightEngine {
public:
    /**
     * @brief 构造函数
     * @param provider 区块光照提供者
     */
    explicit SkyStarLightEngine(StarLightLightingProvider* provider);

    // ========================================================================
    // 光照操作
    // ========================================================================

    /**
     * @brief 检查指定位置的光照
     *
     * 调度该位置及其相邻位置的光照更新。
     *
     * @param pos 方块位置
     */
    void checkBlock(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ);

    /**
     * @brief 获取指定位置的光照等级
     *
     * @param pos 方块位置
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] u8 getLightFor(i32 worldX, i32 worldY, i32 worldZ) const;

    /**
     * @brief 更新区块段状态
     *
     * @param pos 区块段位置
     * @param isEmpty 是否为空
     */
    void updateSectionStatus(const SectionPos& pos, bool isEmpty);

    /**
     * @brief 设置光照数据
     *
     * @param pos 区块段位置
     * @param array 光照数组（SWMR格式）
     * @param retain 是否保留
     */
    void setData(const SectionPos& pos, SWMRNibbleArray&& array, bool retain);

    /**
     * @brief 设置光照数据（从 NibbleArray）
     *
     * @param pos 区块段位置
     * @param array 光照数组
     * @param retain 是否保留
     */
    void setData(const SectionPos& pos, const NibbleArray& array, bool retain);

    /**
     * @brief 获取光照数组（更新侧）
     *
     * @param pos 区块段位置
     * @return 光照数组指针
     */
    [[nodiscard]] SWMRNibbleArray* getData(const SectionPos& pos);

    /**
     * @brief 启用/禁用区块列
     *
     * @param columnPos 区块列位置
     * @param enabled 是否启用
     */
    void setColumnEnabled(i64 columnPos, bool enabled);

    /**
     * @brief 检查是否有待处理的工作
     */
    [[nodiscard]] bool hasWork() const;

    /**
     * @brief 处理光照更新
     *
     * @param maxUpdates 最大更新数量
     * @param updateSkyLight 是否更新天空光照
     * @param updateBlockLight 是否更新方块光照（忽略）
     * @return 剩余配额
     */
    i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight);

protected:
    // ========================================================================
    // LevelBasedGraph 接口实现
    // ========================================================================

    [[nodiscard]] bool isRoot(i64 pos) const override;
    [[nodiscard]] i32 computeLevel(i64 pos, i64 excludedSource, i32 level) override;
    void notifyNeighbors(i64 pos, i32 level, bool isDecreasing, u8 directionBits) override;
    [[nodiscard]] i32 getLevel(i64 pos) const override;
    void setLevel(i64 pos, i32 level) override;
    [[nodiscard]] i32 getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) override;

private:
    SkyLightStorage m_storage;

    /**
     * @brief 获取指定位置的发光等级（天空光照始终为0）
     */
    [[nodiscard]] i32 getLightValue(i64 worldPos) const;

    /**
     * @brief 获取当前位置的光照等级
     */
    [[nodiscard]] i32 getLightLevel(i64 worldPos);

    /**
     * @brief 设置当前位置的光照等级
     */
    void setLightLevel(i64 worldPos, i32 level);

    /**
     * @brief 从SWMRNibbleArray获取光照等级
     */
    [[nodiscard]] i32 getLevelFromArray(const SWMRNibbleArray* array, i64 worldPos) const;

    /**
     * @brief 从缓存获取区块（覆盖基类以使用存储层的区块提供者）
     */
    [[nodiscard]] const IChunk* getChunkCached(i32 chunkX, i32 chunkZ) const;
};

} // namespace mc
