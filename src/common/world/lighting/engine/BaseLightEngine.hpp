/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "LightEngineUtils.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace mc {

// 前向声明
class StarLightLightingProvider;
class BlockState;
class CollisionShape;
namespace world::chunk {
class IChunk;
}
using world::chunk::IChunk;
namespace world::chunk {
class ChunkSection;
}
using world::chunk::ChunkSection;

// ============================================================================
// 方向枚举与工具（参考 Starlight 的 AxisDirection）
// ============================================================================

/**
 * @brief 光照传播方向枚举
 *
 * 顺序很重要：正X(0)、负X(1)、正Z(2)、负Z(3)、正Y(4)、负Y(5)
 * 偶数为正方向，奇数为负方向，可通过 XOR 1 获取相反方向
 *
 * 注意：这与 Direction.hpp 中的 AxisDirection 不同，
 * AxisDirection 只有 Positive/Negative 两个值，
 * 而 LightAxisDirection 包含六个具体方向。
 */
enum class LightAxisDirection : u8 {
    POSITIVE_X = 0, // 东
    NEGATIVE_X = 1, // 西
    POSITIVE_Z = 2, // 南
    NEGATIVE_Z = 3, // 北
    POSITIVE_Y = 4, // 上
    NEGATIVE_Y = 5  // 下
};

/** 所有方向数组（顺序与 Starlight 一致） */
constexpr LightAxisDirection ALL_AXIS_DIRECTIONS[6] = {LightAxisDirection::POSITIVE_X,
    LightAxisDirection::NEGATIVE_X,
    LightAxisDirection::POSITIVE_Z,
    LightAxisDirection::NEGATIVE_Z,
    LightAxisDirection::POSITIVE_Y,
    LightAxisDirection::NEGATIVE_Y};

/** 仅水平方向 */
constexpr LightAxisDirection ONLY_HORIZONTAL_DIRECTIONS[4] = {LightAxisDirection::POSITIVE_X,
    LightAxisDirection::NEGATIVE_X,
    LightAxisDirection::POSITIVE_Z,
    LightAxisDirection::NEGATIVE_Z};

/** 所有方向位集 */
constexpr i32 ALL_DIRECTIONS_BITSET = 0x3F; // 0b111111

/**
 * @brief 获取方向的偏移量
 */
inline constexpr void _getDirectionOffset(LightAxisDirection dir, i32& dx, i32& dy, i32& dz) noexcept
{
    switch (dir) {
        case LightAxisDirection::POSITIVE_X:
            dx = 1;
            dy = 0;
            dz = 0;
            break;
        case LightAxisDirection::NEGATIVE_X:
            dx = -1;
            dy = 0;
            dz = 0;
            break;
        case LightAxisDirection::POSITIVE_Z:
            dx = 0;
            dy = 0;
            dz = 1;
            break;
        case LightAxisDirection::NEGATIVE_Z:
            dx = 0;
            dy = 0;
            dz = -1;
            break;
        case LightAxisDirection::POSITIVE_Y:
            dx = 0;
            dy = 1;
            dz = 0;
            break;
        case LightAxisDirection::NEGATIVE_Y:
            dx = 0;
            dy = -1;
            dz = 0;
            break;
    }
}

/**
 * @brief 获取方向的 NMS Direction（用于面遮挡查询）
 */
inline constexpr Direction _getNMSDirection(LightAxisDirection dir) noexcept
{
    switch (dir) {
        case LightAxisDirection::POSITIVE_X:
            return Direction::East;
        case LightAxisDirection::NEGATIVE_X:
            return Direction::West;
        case LightAxisDirection::POSITIVE_Z:
            return Direction::South;
        case LightAxisDirection::NEGATIVE_Z:
            return Direction::North;
        case LightAxisDirection::POSITIVE_Y:
            return Direction::Up;
        case LightAxisDirection::NEGATIVE_Y:
            return Direction::Down;
    }
    return Direction::None;
}

/**
 * @brief 获取相反方向
 * 偶数 XOR 1 得奇数（负方向），奇数 XOR 1 得偶数（正方向）
 */
inline constexpr LightAxisDirection _getOppositeDirection(LightAxisDirection dir) noexcept
{
    return static_cast<LightAxisDirection>(static_cast<u8>(dir) ^ 1);
}

/**
 * @brief 获取方向位集
 */
inline constexpr i32 getDirectionBitset(LightAxisDirection dir) noexcept
{
    return 1 << static_cast<u8>(dir);
}

/**
 * @brief 获取排除某方向后的位集
 */
inline constexpr i32 _getEverythingButDirection(LightAxisDirection dir) noexcept
{
    return ALL_DIRECTIONS_BITSET ^ getDirectionBitset(dir);
}

/**
 * @brief 获取排除某方向及其反方向后的位集
 */
inline constexpr i32 _getEverythingButOppositeDirection(LightAxisDirection dir) noexcept
{
    return ALL_DIRECTIONS_BITSET ^ (getDirectionBitset(dir) | getDirectionBitset(_getOppositeDirection(dir)));
}

/**
 * @brief 基于级别的传播图（Starlight 优化版）
 *
 * 参考: ca.spottedleaf.moonrise.patches.starlight.light.StarLightEngine
 *
 * 队列元素采用紧凑64位编码：
 * - 位 0-5: X坐标（相对于编码偏移，6位 = 64格范围）
 * - 位 6-11: Z坐标（相对于编码偏移，6位 = 64格范围）
 * - 位 12-27: Y坐标（相对于编码偏移，16位）
 * - 位 28-31: 传播级别 (0-15)
 * - 位 32-37: 传播方向位集
 * - 位 38-40: 状态标志
 * - 位 41-63: 未使用
 */
class StarLightEngine {
public:
    /** 最大级别数（光照最大15级 + 1个溢出级） */
    static constexpr i32 MAX_LEVEL_COUNT = 16;

    // 标志位定义（与 Starlight 对齐）
    /** 标志：需要写入光照等级（用于光源恢复） */
    static constexpr u64 FLAG_WRITE_LEVEL = 1ULL << 63;

    /** 标志：需要重新检查光照等级 */
    static constexpr u64 FLAG_RECHECK_LEVEL = 1ULL << 62;

    /** 标志：当前方块有条件透明面（需要检查面遮挡） */
    static constexpr u64 FLAG_HAS_SIDED_TRANSPARENT_BLOCKS = 1ULL << 61;

    virtual ~StarLightEngine() = default;

    // ========================================================================
    // 公共接口
    // ========================================================================

    /**
     * @brief 检查是否有待处理的更新
     */
    [[nodiscard]] bool needsUpdate() const noexcept { return m_needsUpdate; }

    /**
     * @brief 获取待处理更新数量
     */
    [[nodiscard]] i32 queuedUpdateSize() const noexcept
    {
        return static_cast<i32>(m_increaseQueueInitialLength + m_decreaseQueueInitialLength);
    }

    /**
     * @brief 检查是否有待处理的工作
     */
    [[nodiscard]] bool hasWork() const noexcept { return m_needsUpdate || queuedUpdateSize() > 0; }

    /**
     * @brief 重置队列状态
     *
     * 在新的光照操作开始前调用，确保队列状态干净。
     * 与 Moonrise StarLightEngine.light() 中的重置逻辑一致。
     */
    void resetQueueState() noexcept
    {
        m_increaseQueueInitialLength = 0;
        m_decreaseQueueInitialLength = 0;
        m_needsUpdate = false;
    }

    /**
     * @brief 安排光照更新
     *
     * 将位置添加到增亮队列，用于后续处理。
     *
     * @param pos 编码的世界位置
     */
    void scheduleUpdate(i64 pos)
    {
        // 使用最大级别和所有方向
        // 对于增加队列，使用级别 15（表示需要计算）
        appendToIncreaseQueue(
            static_cast<u64>(pos) | (static_cast<u64>(15) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32));
        m_needsUpdate = true;
    }

    /**
     * @brief 处理所有更新
     * @param lightAccess 光照区块访问器
     * @param maxUpdates 最大更新数量
     * @return 剩余配额
     */
    i32 performUpdates(StarLightLightingProvider* lightAccess, i32 maxUpdates);

    /**
     * @brief 执行一个 tick 的光照更新
     * @param maxUpdates 最大更新数量
     * @param updateSkyLight 是否更新天空光照
     * @param updateBlockLight 是否更新方块光照
     * @return 剩余配额
     */
    virtual i32 tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight);

    /**
     * @brief 更新区块段状态
     * @param pos 区块段位置
     * @param isEmpty 是否为空
     */
    virtual void updateSectionStatus(const SectionPos& pos, bool isEmpty);

    /**
     * @brief 设置光照数据
     */
    virtual void setData(const SectionPos& pos, const NibbleArray& array, bool retain) = 0;

    /**
     * @brief 获取光照数据
     */
    [[nodiscard]] virtual SWMRNibbleArray* getData(const SectionPos& pos) = 0;

    /**
     * @brief 获取光照数据（只读）
     */
    [[nodiscard]] virtual const SWMRNibbleArray* getData(const SectionPos& pos) const = 0;

    // ========================================================================
    // 缓存管理
    // ========================================================================

    /**
     * @brief 设置世界引用（用于获取世界信息）
     */
    virtual void setWorld(void* world);

    /**
     * @brief 初始化缓存
     */
    void setupCaches(StarLightLightingProvider* lightAccess,
        i32 centerX,
        i32 centerY,
        i32 centerZ,
        bool relaxed,
        bool loadTwoRadius);

    /**
     * @brief 清除缓存
     */
    void destroyCaches();

    /**
     * @brief 同步可见侧数据并通知变更
     */
    void updateVisible(StarLightLightingProvider* lightAccess);

    // ========================================================================
    // 区块光照操作
    // ========================================================================

    /**
     * @brief 处理区块内方块变化
     */
    void blocksChangedInChunk(StarLightLightingProvider* lightAccess,
        i32 chunkX,
        i32 chunkZ,
        const std::vector<BlockPos>& positions,
        const std::vector<bool>& changedSections);

    /**
     * @brief 处理空区块段变化
     * @return 如果空映射变更返回新的空映射，否则返回 nullptr
     */
    std::vector<bool> handleEmptySectionChanges(StarLightLightingProvider* lightAccess,
        const IChunk* chunk,
        const std::vector<bool>& emptinessChanges,
        bool isUnlit);

    /**
     * @brief 强制处理空区块段变化
     *
     * 用于已正确光照的区块，只需要重新加载光照数据到缓存。
     * 与 handleEmptySectionChanges 的区别是会强制将区块数据加载到缓存，
     * 并且不使用 unlit 模式。
     *
     * 参考: Moonrise StarLightEngine.forceHandleEmptySectionChanges
     *
     * @param lightAccess 光照区块访问器
     * @param chunk 区块
     * @param emptySections 空区块段标记数组
     */
    void forceHandleEmptySectionChanges(
        StarLightLightingProvider* lightAccess, const IChunk* chunk, const std::vector<bool>& emptySections);

    /**
     * @brief 检查区块边缘
     */
    void checkChunkEdges(StarLightLightingProvider* lightAccess, i32 chunkX, i32 chunkZ);

    /**
     * @brief 照亮区块（完整流程，包含缓存管理）
     *
     * 封装 setupCaches、缓存设置、lightChunk、updateVisible、destroyCaches 的完整流程。
     * 用于区块首次加载时的初始光照计算。
     *
     * @param lightAccess 光照区块访问器
     * @param chunk 要照亮的目标区块
     * @param needsEdgeChecks 是否需要检查边缘
     */
    void light(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks = true);

    /**
     * @brief 照亮区块（子类实现，不包含缓存管理）
     */
    virtual void lightChunk(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks);

protected:
    /**
     * @brief 构造函数
     * @param isSkyLight 是否为天空光照引擎
     */
    explicit StarLightEngine(bool isSkyLight);

    // ========================================================================
    // 抽象方法（子类实现）
    // ========================================================================

    /** 获取区块的空映射 */
    [[nodiscard]] virtual const bool* getEmptinessMap(const IChunk* chunk) const = 0;

    /** 设置区块的空映射 */
    virtual void setEmptinessMap(const IChunk* chunk, const bool* map) = 0;

    /** 获取区块的光照数组 */
    [[nodiscard]] virtual SWMRNibbleArray* const* getNibblesOnChunk(const IChunk* chunk) const = 0;

    /** 设置区块的光照数组 */
    virtual void setNibbles(const IChunk* chunk, SWMRNibbleArray* const* nibbles) = 0;

    /** 检查区块是否可用于光照计算 */
    [[nodiscard]] virtual bool canUseChunk(const IChunk* chunk) const = 0;

    /** 初始化区块段 Nibble 数组 */
    virtual void initNibble(i32 chunkX, i32 chunkY, i32 chunkZ, bool extrude, bool initRemovedNibbles) = 0;

    /** 设置 Nibble 数组为 null */
    virtual void setNibbleNull(i32 chunkX, i32 chunkY, i32 chunkZ) = 0;

    /** 检查方块（子类实现具体逻辑） */
    virtual void checkBlock(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ) = 0;

    /** 计算光照值（用于验证和重新计算） */
    [[nodiscard]] virtual i32 calculateLightValue(
        StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ, i32 expected) = 0;

    /** 传播方块变化 */
    virtual void propagateBlockChanges(
        StarLightLightingProvider* lightAccess, const IChunk* chunk, const std::vector<BlockPos>& positions) = 0;

    /** 检查区块边缘（子类可扩展） */
    virtual void checkChunkEdges(
        StarLightLightingProvider* lightAccess, const IChunk* chunk, i32 fromSection, i32 toSection);

    /** 检查单个区块段边缘 */
    void checkChunkEdge(
        StarLightLightingProvider* lightAccess, const IChunk* chunk, i32 chunkX, i32 chunkY, i32 chunkZ);

    /** 从邻居传播光照到区块 */
    void propagateNeighbourLevels(
        StarLightLightingProvider* lightAccess, const IChunk* chunk, i32 fromSection, i32 toSection);

    // ========================================================================
    // 缓存访问方法
    // ========================================================================

    /** 从缓存获取区块 */
    [[nodiscard]] const IChunk* getChunkInCache(i32 chunkX, i32 chunkZ) const;

    /** 设置区块到缓存 */
    void setChunkInCache(i32 chunkX, i32 chunkZ, const IChunk* chunk);

    /** 从缓存获取区块段 */
    [[nodiscard]] const ChunkSection* getChunkSection(i32 chunkX, i32 chunkY, i32 chunkZ) const;

    /** 设置区块段到缓存 */
    void setChunkSectionInCache(i32 chunkX, i32 chunkY, i32 chunkZ, const ChunkSection* section);

    /** 设置区块的所有区块段到缓存 */
    void setBlocksForChunkInCache(i32 chunkX, i32 chunkZ, const ChunkSection* const* sections);

    /** 从缓存获取 Nibble 数组 */
    [[nodiscard]] SWMRNibbleArray* getNibbleFromCache(i32 chunkX, i32 chunkY, i32 chunkZ) const;

    /** 设置 Nibble 数组到缓存 */
    void setNibbleInCache(i32 chunkX, i32 chunkY, i32 chunkZ, SWMRNibbleArray* nibble);

    /** 设置区块的所有 Nibble 数组到缓存 */
    void setNibblesForChunkInCache(i32 chunkX, i32 chunkZ, SWMRNibbleArray* const* nibbles);

    /** 从缓存获取空映射 */
    [[nodiscard]] const bool* getEmptinessMap(i32 chunkX, i32 chunkZ) const;

    /** 设置空映射到缓存 */
    void setEmptinessMapCache(i32 chunkX, i32 chunkZ, const bool* map);

    /** 获取方块状态 */
    [[nodiscard]] const BlockState* getBlockState(i32 worldX, i32 worldY, i32 worldZ) const;

    /** 获取方块状态（通过区块段索引） */
    [[nodiscard]] const BlockState* getBlockState(i32 sectionIndex, i32 localIndex) const;

    /** 获取光照等级 */
    [[nodiscard]] i32 getLightLevel(i32 worldX, i32 worldY, i32 worldZ) const;

    /** 获取光照等级（通过区块段索引） */
    [[nodiscard]] i32 getLightLevel(i32 sectionIndex, i32 localIndex) const;

    /** 设置光照等级 */
    void setLightLevel(i32 worldX, i32 worldY, i32 worldZ, i32 level);

    /** 设置光照等级（带世界坐标） */
    void setLightLevel(i32 sectionIndex, i32 localIndex, i32 worldX, i32 worldY, i32 worldZ, i32 level);

    /** 光照更新后通知 */
    void postLightUpdate(i32 worldX, i32 worldY, i32 worldZ);

    // ========================================================================
    // 队列操作
    // ========================================================================

    /** 添加到增亮队列 */
    void appendToIncreaseQueue(u64 queueValue);

    /** 添加到减亮队列 */
    void appendToDecreaseQueue(u64 queueValue);

    /** 扩展增亮队列 */
    void resizeIncreaseQueue();

    /** 扩展减亮队列 */
    void resizeDecreaseQueue();

    /** 执行增亮传播 */
    void performLightIncrease(StarLightLightingProvider* lightAccess);

    /** 执行减亮传播 */
    void performLightDecrease(StarLightLightingProvider* lightAccess);

    // ========================================================================
    // 工具方法
    // ========================================================================

    /** 创建填充为空的 Nibble 数组 */
    [[nodiscard]] static std::vector<SWMRNibbleArray*> getFilledEmptyLight(i32 totalLightSections);

    /** 设置编码偏移 */
    void setupEncodeOffset(i32 centerX, i32 centerY, i32 centerZ);

    /**
     * @brief 检查两个方块之间是否遮挡光线
     *
     * 条件透明检查：判断光线是否可以从源方块传播到目标方块。
     * 使用 VoxelShape 系统进行精确的面遮挡检测。
     *
     * @param fromState 源方块状态（光线发出位置）
     * @param toState 目标方块状态（光线进入位置）
     * @param direction 光线传播方向（从源到目标）
     * @return true 如果面被完全遮挡（光线无法通过）
     */
    [[nodiscard]] static bool isFaceOccluded(
        const BlockState* fromState, const BlockState* toState, LightAxisDirection direction);

    /**
     * @brief 检查方块是否使用形状进行光照遮挡
     *
     * 某些方块（如台阶、楼梯、栅栏）有非完整方块的碰撞形状，
     * 需要精确的面遮挡检测。
     *
     * @param state 方块状态
     * @return true 如果需要使用形状进行遮挡检测
     */
    [[nodiscard]] static bool useShapeForLightOcclusion(const BlockState* state);

protected:
    // 世界引用
    void* m_world = nullptr;
    bool m_isSkyLight = false;
    bool m_isClientSide = false;

    // 世界高度范围（从世界高度常量计算）
    // 段坐标范围：Y >> SECTION_SHIFT，MIN_BUILD_HEIGHT=-64 -> section -4，MAX_BUILD_HEIGHT=320 -> section 19
    i32 m_minSection = world::MIN_BUILD_HEIGHT >> world::SECTION_SHIFT;
    i32 m_maxSection = (world::MAX_BUILD_HEIGHT - 1) >> world::SECTION_SHIFT;
    // 光照段需要额外缓冲段（上方和下方各一个）
    i32 m_minLightSection = m_minSection - 1;
    i32 m_maxLightSection = m_maxSection + 1;

    // 编码偏移（用于坐标压缩）
    i32 m_encodeOffsetX = 0;
    i32 m_encodeOffsetY = 0;
    i32 m_encodeOffsetZ = 0;
    i32 m_coordinateOffset = 0;

    // 区块缓存偏移
    i32 m_chunkOffsetX = 0;
    i32 m_chunkOffsetY = 0;
    i32 m_chunkOffsetZ = 0;
    i32 m_chunkIndexOffset = 0;
    i32 m_chunkSectionIndexOffset = 0;

    // 发光掩码（方块光照为 0xF，天空光照为 0）
    i32 m_emittedLightMask = 0;

    // 区块缓存（5x5）
    std::array<const IChunk*, 25> m_chunkCache{};

    // 区块段缓存
    const ChunkSection** m_sectionCache = nullptr;
    i32 m_sectionCacheSize = 0;

    // Nibble 数组缓存
    SWMRNibbleArray** m_nibbleCache = nullptr;

    // 空映射缓存
    std::array<const bool*, 25> m_emptinessMapCache{};

    // 通知更新缓存（客户端）
    bool* m_notifyUpdateCache = nullptr;

    // 增亮队列（使用紧凑64位编码）
    std::vector<u64> m_increaseQueue;
    i32 m_increaseQueueInitialLength = 0;

    // 减亮队列
    std::vector<u64> m_decreaseQueue;
    i32 m_decreaseQueueInitialLength = 0;

    // 区块边缘检查延迟更新
    std::array<i32, 256> m_chunkCheckDelayedUpdatesCenter{};
    std::array<i32, 256> m_chunkCheckDelayedUpdatesNeighbour{};

    bool m_needsUpdate = false;

private:
    // 预计算的方向检查数组
    static std::array<std::vector<LightAxisDirection>, 64> s_oldCheckDirections;
    static bool s_directionsInitialized;

    static void _initializeDirections();
};

} // namespace mc
