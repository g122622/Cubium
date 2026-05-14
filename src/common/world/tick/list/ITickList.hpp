#pragma once

#include "../../block/BlockPos.hpp"
#include "../base/TickPriority.hpp"

namespace mc::world::tick {

/**
 * @brief Tick调度列表接口
 *
 * 定义tick调度的通用接口，支持方块tick、流体tick等。
 * 实现可以是ServerTickList（服务端）或EmptyTickList（客户端/空实现）。
 *
 * 参考: net.minecraft.world.ITickList
 *
 * 用法示例:
 * @code
 * ITickList<Block>& blockTicks = world.blockTicks();
 *
 * // 调度一个方块tick
 * blockTicks.scheduleTick(pos, block, 10);  // 10 tick延迟
 *
 * // 检查是否有待执行的tick
 * if (blockTicks.isTickScheduled(pos, block)) {
 *     // ...
 * }
 * @endcode
 *
 * @tparam T 目标类型（Block、Fluid等）
 */
template <typename T>
class ITickList {
public:
    virtual ~ITickList() = default;

    /**
     * @brief 是否已调度tick（待执行）
     *
     * 检查指定位置和目标是否已经在调度队列中。
     *
     * @param pos 位置
     * @param target 目标
     * @return 是否已调度
     */
    [[nodiscard]] virtual bool isTickScheduled(const BlockPos& pos, T& target) const = 0;

    /**
     * @brief 是否在本tick内待执行
     *
     * 检查指定位置和目标是否在本游戏刻内即将执行。
     * 与isTickScheduled的区别：isTickPending只检查当前tick，
     * 而isTickScheduled检查所有未来tick。
     *
     * @param pos 位置
     * @param target 目标
     * @return 是否本tick待执行
     */
    [[nodiscard]] virtual bool isTickPending(const BlockPos& pos, T& target) const = 0;

    /**
     * @brief 调度tick（普通优先级）
     *
     * @param pos 位置
     * @param target 目标
     * @param delay 延迟tick数（相对于当前游戏刻）
     */
    virtual void scheduleTick(const BlockPos& pos, T& target, i32 delay)
    {
        scheduleTick(pos, target, delay, TickPriority::Normal);
    }

    /**
     * @brief 调度tick（指定优先级）
     *
     * @param pos 位置
     * @param target 目标
     * @param delay 延迟tick数
     * @param priority 优先级
     */
    virtual void scheduleTick(const BlockPos& pos, T& target, i32 delay, TickPriority priority) = 0;

    /**
     * @brief 取消tick
     *
     * 移除指定位置和目标的调度。如果不存在则无操作。
     *
     * @param pos 位置
     * @param target 目标
     * @return 是否成功取消
     */
    virtual bool cancelTick(const BlockPos& pos, T& target)
    {
        // 默认实现不支持取消
        return false;
    }

    /**
     * @brief 获取待处理tick数量
     */
    [[nodiscard]] virtual size_t pendingCount() const = 0;
};

} // namespace mc::world::tick
