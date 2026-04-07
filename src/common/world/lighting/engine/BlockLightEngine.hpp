#pragma once

#include "../storage/BlockLightStorage.hpp"
#include "../storage/SWMRNibbleArray.hpp"
#include "../storage/EmptinessMap.hpp"
#include "LevelBasedGraph.hpp"
#include "LightEngineUtils.hpp"
#include "../../block/BlockPos.hpp"
#include <unordered_map>

namespace mc {

// 前向声明
class IWorld;
class CollisionShape;

/**
 * @brief 方块光照引擎
 *
 * 实现方块光照的传播算法。
 * 方块光源（如火把、萤石）发出的光会向相邻方块传播，
 * 每传播一个方块衰减1级，直到衰减为0。
 *
 * 参考: net.minecraft.world.lighting.BlockLightEngine
 */
class BlockStarLightEngine : public StarLightEngine {
public:
    /**
     * @brief 构造函数
     * @param provider 区块光照提供者
     */
    explicit BlockStarLightEngine(StarLightLightingProvider* provider);

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
     * @brief 方块发光等级增加时调用
     *
     * 当方块被放置且发光等级大于0时调用。
     *
     * @param pos 方块位置
     * @param lightLevel 发光等级
     */
    void onBlockEmissionIncrease(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ, i32 lightLevel);

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
     * @brief 检查是否有待处理的工作
     */
    [[nodiscard]] bool hasWork() const;

    /**
     * @brief 处理光照更新
     *
     * @param maxUpdates 最大更新数量
     * @param updateSkyLight 是否更新天空光照（忽略，方块光照引擎不处理）
     * @param updateBlockLight 是否更新方块光照
     * @return 剩余配额
     */
    i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight);

    // ========================================================================
    // 空区块段检测
    // ========================================================================

    /**
     * @brief 更新区块的空区块段映射
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param chunk 区块指针
     */
    void updateEmptinessMap(i32 chunkX, i32 chunkZ, const IChunk* chunk);

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
    [[nodiscard]] bool isSectionEmpty(i64 sectionPos) const override;

private:
    BlockLightStorage m_storage;

    // 空区块段映射缓存
    std::unordered_map<i64, EmptinessMap> m_emptinessMaps;

    /**
     * @brief 获取指定位置的发光等级
     */
    [[nodiscard]] i32 getLightValue(i64 worldPos) const;

    /**
     * @brief 获取当前位置的光照等级
     */
    [[nodiscard]] i32 getLightLevel(i64 worldPos) const;

    /**
     * @brief 设置当前位置的光照等级
     */
    void setLightLevel(i64 worldPos, i32 level);

    /**
     * @brief 从缓存获取区块（覆盖基类以使用存储层的区块提供者）
     */
    [[nodiscard]] const IChunk* getChunkCached(i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 获取或创建空区块段映射
     */
    EmptinessMap* getOrCreateEmptinessMap(i64 columnPos);
};

} // namespace mc
