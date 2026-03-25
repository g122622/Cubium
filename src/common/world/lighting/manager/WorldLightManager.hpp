#pragma once

#include "../engine/BlockLightEngine.hpp"
#include "../engine/SkyLightEngine.hpp"
#include "../storage/SWMRNibbleArray.hpp"
#include "../LightType.hpp"
#include "../../chunk/ChunkPos.hpp"
#include <memory>
#include <mutex>

namespace mc {

/**
 * @brief 世界光照管理器
 *
 * 协调方块光照和天空光照引擎，提供统一的光照计算接口。
 * 根据维度配置，可能只有方块光照（如下界）或两者都有（如主世界）。
 *
 * 参考: net.minecraft.world.lighting.WorldLightManager
 */
class WorldLightManager {
public:
    /**
     * @brief 构造函数
     *
     * @param provider 区块光照提供者
     * @param hasBlockLight 是否有方块光照
     * @param hasSkyLight 是否有天空光照
     */
    WorldLightManager(IChunkLightProvider* provider, bool hasBlockLight, bool hasSkyLight);

    // ========================================================================
    // 光照操作
    // ========================================================================

    /**
     * @brief 检查方块的光照
     *
     * 当方块状态改变时调用，重新计算该位置的光照。
     *
     * @param pos 方块位置
     */
    void checkBlock(const BlockPos& pos);

    /**
     * @brief 方块发光等级增加时调用
     *
     * 当方块被放置且发光等级大于0时调用。
     *
     * @param pos 方块位置
     * @param lightLevel 发光等级
     */
    void onBlockEmissionIncrease(const BlockPos& pos, i32 lightLevel);

    /**
     * @brief 检查是否有待处理的光照工作
     */
    [[nodiscard]] bool hasLightWork() const;

    /**
     * @brief 处理光照更新
     *
     * @param maxUpdates 最大更新数量
     * @param updateSkyLight 是否更新天空光照
     * @param updateBlockLight 是否更新方块光照
     * @return 剩余更新配额
     */
    i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight);

    // ========================================================================
    // 区块段管理
    // ========================================================================

    /**
     * @brief 更新区块段状态
     *
     * 当区块段加载/卸载或变为空时调用。
     *
     * @param pos 区块段位置
     * @param isEmpty 是否为空（没有方块）
     */
    void updateSectionStatus(const SectionPos& pos, bool isEmpty);

    /**
     * @brief 启用/禁用区块的光源
     *
     * 当区块加载/卸载时调用。
     *
     * @param pos 区块位置
     * @param enable 是否启用
     */
    void enableLightSources(const ChunkPos& pos, bool enable);

    // ========================================================================
    // 光照访问
    // ========================================================================

    /**
     * @brief 获取光照引擎
     *
     * @param type 光照类型
     * @return 光照引擎指针，如果不存在返回nullptr
     */
    [[nodiscard]] BlockLightEngine* getBlockLightEngine();
    [[nodiscard]] const BlockLightEngine* getBlockLightEngine() const;
    [[nodiscard]] SkyLightEngine* getSkyLightEngine();
    [[nodiscard]] const SkyLightEngine* getSkyLightEngine() const;

    /**
     * @brief 获取指定位置的实际亮度
     *
     * 考虑天空减暗因子，计算最终亮度。
     *
     * @param pos 方块位置
     * @param skyDarkening 天空减暗因子（0-15）
     * @return 亮度值 (0-15)
     */
    [[nodiscard]] i32 getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const;

    /**
     * @brief 获取方块光照等级
     */
    [[nodiscard]] u8 getBlockLight(const BlockPos& pos) const;

    /**
     * @brief 获取天空光照等级
     */
    [[nodiscard]] u8 getSkyLight(const BlockPos& pos) const;

    // ========================================================================
    // 数据管理
    // ========================================================================

    /**
     * @brief 设置光照数据
     *
     * 用于从存档加载光照数据。
     *
     * @param type 光照类型
     * @param pos 区块段位置
     * @param array 光照数组
     * @param retain 是否保留（防止被覆盖）
     */
    void setData(LightType type, const SectionPos& pos, const NibbleArray& array, bool retain);

    /**
     * @brief 获取光照数据
     *
     * @param type 光照类型
     * @param pos 区块段位置
     * @return 光照数组指针，如果不存在返回nullptr
     */
    [[nodiscard]] SWMRNibbleArray* getData(LightType type, const SectionPos& pos);

    /**
     * @brief 保留区块数据
     *
     * 防止光照数据在区块卸载时被清除。
     *
     * @param pos 区块位置
     * @param retain 是否保留
     */
    void retainData(const ChunkPos& pos, bool retain);

    // ========================================================================
    // 调试信息
    // ========================================================================

    /**
     * @brief 获取调试信息
     *
     * @param type 光照类型
     * @param pos 区块段位置
     * @return 调试字符串
     */
    [[nodiscard]] String getDebugInfo(LightType type, const SectionPos& pos) const;

private:
    mutable std::recursive_mutex m_mutex;
    std::unique_ptr<BlockLightEngine> m_blockLight;
    std::unique_ptr<SkyLightEngine> m_skyLight;
    IChunkLightProvider* m_provider;
    bool m_hasBlockLight;
    bool m_hasSkyLight;
};

} // namespace mc
