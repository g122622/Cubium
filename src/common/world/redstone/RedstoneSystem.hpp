#pragma once

#include "RedstoneContext.hpp"
#include "RedstonePower.hpp"
#include "../../core/Types.hpp"
#include "../block/BlockPos.hpp"
#include "../tick/base/TickPriority.hpp"
#include <unordered_set>
#include <vector>

namespace mc {

// 前向声明
class IWorld;
class Block;

namespace world {
namespace redstone {

/**
 * @brief 红石系统管理器
 *
 * 协调红石信号的计算、更新和传播。
 * 使用单例模式，通过 IWorld 接口与世界交互。
 *
 * ## 核心职责
 * 1. 红石信号传播和更新调度
 * 2. 防止无限递归更新
 * 3. 更新批处理和优化
 *
 * ## 使用示例
 * ```cpp
 * auto& redstone = RedstoneSystem::instance();
 *
 * // 更新相邻方块的红石状态
 * redstone.updateNeighbors(world, pos, block);
 *
 * // 调度延迟更新
 * redstone.scheduleUpdate(world, pos, block, 2, TickPriority::High);
 *
 * // 检查位置是否正在更新
 * if (!redstone.isUpdating(pos)) {
 *     redstone.beginUpdate(pos);
 *     // 执行红石计算...
 *     redstone.endUpdate(pos);
 * }
 * ```
 *
 * ## 线程安全
 * 内部使用 RedstoneContext 保护共享状态。
 *
 * 参考 MC 1.16.5 World.redstonePowerProvider
 */
class RedstoneSystem {
public:
    /**
     * @brief 获取单例实例
     */
    static RedstoneSystem& instance();

    // ========== 红石更新 ==========

    /**
     * @brief 更新相邻方块的红石状态
     *
     * 当红石信号变化时，通知相邻方块更新状态。
     * 这会触发 neighborChanged 回调。
     *
     * @param world 世界引用
     * @param pos 信号源位置
     * @param block 信号源方块（用于更新回调）
     */
    void updateNeighbors(IWorld& world, const BlockPos& pos, Block& block);

    /**
     * @brief 更新指定方向以外的相邻方块
     *
     * 用于红石线等需要跳过特定方向的更新。
     *
     * @param world 世界引用
     * @param pos 信号源位置
     * @param block 信号源方块
     * @param skipDirection 跳过的方向
     */
    void updateNeighborsExcept(IWorld& world, const BlockPos& pos,
                               Block& block, Direction skipDirection);

    /**
     * @brief 更新水平和下方的方块
     *
     * 用于红石火把等不向上传播信号的情况。
     *
     * @param world 世界引用
     * @param pos 信号源位置
     * @param block 信号源方块
     */
    void updateNeighborsHorizontalAndDown(IWorld& world, const BlockPos& pos, Block& block);

    /**
     * @brief 更新指定方块周围的比较器
     *
     * 当容器内容变化时调用。
     *
     * @param world 世界引用
     * @param pos 容器位置
     */
    void updateComparators(IWorld& world, const BlockPos& pos);

    // ========== 更新调度 ==========

    /**
     * @brief 调度红石更新
     *
     * 安排延迟tick执行的红石更新。
     * 红石更新默认使用高优先级。
     *
     * @param world 世界引用
     * @param pos 更新位置
     * @param block 方块引用
     * @param delay 延迟tick数
     * @param priority tick优先级（默认High）
     */
    void scheduleUpdate(IWorld& world, const BlockPos& pos, Block& block,
                       i32 delay, tick::TickPriority priority = tick::TickPriority::High);

    /**
     * @brief 调度极优先更新
     *
     * 用于红石二极管（中继器、比较器）面朝另一个二极管时。
     * 确保更新顺序正确。
     *
     * @param world 世界引用
     * @param pos 更新位置
     * @param block 方块引用
     * @param delay 延迟tick数
     */
    void scheduleExtremelyHighPriorityUpdate(IWorld& world, const BlockPos& pos,
                                              Block& block, i32 delay);

    // ========== 递归保护 ==========

    /**
     * @brief 检查位置是否正在被更新
     *
     * @param pos 检查位置
     * @return true 如果位置正在更新中
     */
    [[nodiscard]] bool isUpdating(const BlockPos& pos) const {
        return m_context.isUpdating(pos);
    }

    /**
     * @brief 开始更新某个位置
     *
     * @param pos 更新位置
     */
    void beginUpdate(const BlockPos& pos) {
        m_context.beginUpdate(pos);
    }

    /**
     * @brief 结束更新某个位置
     *
     * @param pos 更新位置
     */
    void endUpdate(const BlockPos& pos) {
        m_context.endUpdate(pos);
    }

    /**
     * @brief 检查是否可以增加更新深度
     */
    [[nodiscard]] bool canPushDepth() const {
        return m_context.canPushDepth();
    }

    /**
     * @brief 增加更新深度
     */
    void pushDepth() {
        m_context.pushDepth();
    }

    /**
     * @brief 减少更新深度
     */
    void popDepth() {
        m_context.popDepth();
    }

    // ========== 信号查询 ==========

    /**
     * @brief 获取位置的间接红石信号强度
     *
     * 等同于调用 RedstonePower::getRedstonePowerFromNeighbors
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 最大信号强度 0-15
     */
    [[nodiscard]] i32 getIndirectPower(IWorld& world, const BlockPos& pos) const {
        return RedstonePower::getRedstonePowerFromNeighbors(world, pos);
    }

    /**
     * @brief 检查方块是否被红石充能
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return true 如果被充能
     */
    [[nodiscard]] bool isBlockPowered(IWorld& world, const BlockPos& pos) const {
        return RedstonePower::isPowered(world, pos);
    }

    // ========== 重置 ==========

    /**
     * @brief 清空所有状态
     *
     * 在世界卸载时调用。
     */
    void clear() {
        m_context.clear();
    }

private:
    RedstoneSystem() = default;

    /// 红石上下文（防止递归）
    RedstoneContext m_context;
};

} // namespace redstone
} // namespace world
} // namespace mc
