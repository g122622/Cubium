#pragma once

#include "ITickList.hpp"

namespace mc::world::tick {

/**
 * @brief 空Tick列表实现
 *
 * 用于客户端或不需要tick处理的场景。
 * 单例模式，所有方法返回空操作。
 *
 * 参考: net.minecraft.world.EmptyTickList
 *
 * 用法示例:
 * @code
 * // 获取空tick列表实例
 * ITickList<Block>& ticks = EmptyTickList<Block>::get();
 *
 * // 所有操作都是无操作
 * ticks.scheduleTick(pos, block, 10);  // 不做任何事情
 * ticks.isTickScheduled(pos, block);    // 始终返回false
 * @endcode
 *
 * @tparam T 目标类型
 */
template <typename T>
class EmptyTickList : public ITickList<T> {
public:
    /**
     * @brief 获取单例实例
     */
    static EmptyTickList<T>& get()
    {
        static EmptyTickList<T> instance;
        return instance;
    }

    [[nodiscard]] bool isTickScheduled(const BlockPos& pos, T& target) const override { return false; }

    [[nodiscard]] bool isTickPending(const BlockPos& pos, T& target) const override { return false; }

    void scheduleTick(const BlockPos& pos, T& target, i32 delay) override
    {
        // 空操作
    }

    void scheduleTick(const BlockPos& pos, T& target, i32 delay, TickPriority priority) override
    {
        // 空操作
    }

    [[nodiscard]] size_t pendingCount() const override { return 0; }

private:
    EmptyTickList() = default;
    EmptyTickList(const EmptyTickList&) = delete;
    EmptyTickList& operator=(const EmptyTickList&) = delete;
};

} // namespace mc::world::tick
