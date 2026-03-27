#pragma once

#include "../../core/Types.hpp"
#include "../block/BlockPos.hpp"
#include <unordered_set>
#include <mutex>
#include <atomic>

namespace mc {
namespace world {
namespace redstone {

/**
 * @brief 红石上下文 - 防止递归更新
 *
 * 红石信号传播可能导致无限递归更新（例如红石火把反馈）。
 * 此类跟踪正在更新的位置，防止重复更新。
 *
 * ## 使用示例
 * ```cpp
 * RedstoneContext ctx;
 * if (!ctx.isUpdating(pos)) {
 *     ctx.beginUpdate(pos);
 *     // 执行红石更新...
 *     ctx.endUpdate(pos);
 * }
 * ```
 *
 * ## 线程安全
 * 内部使用 mutex 保护，支持多线程访问。
 *
 * 参考 MC 1.16.5 World.redstoneUpdateEnabled
 */
class RedstoneContext {
public:
    /// 最大更新深度限制
    static constexpr i32 MAX_DEPTH = 512;

    RedstoneContext() = default;

    /**
     * @brief 检查位置是否正在更新
     *
     * @param pos 方块位置
     * @return true 如果位置正在更新中
     */
    [[nodiscard]] bool isUpdating(const BlockPos& pos) const;

    /**
     * @brief 开始更新某个位置
     *
     * 将位置加入更新集合，防止递归更新。
     * 更新完成后必须调用 endUpdate。
     *
     * @param pos 更新位置
     */
    void beginUpdate(const BlockPos& pos);

    /**
     * @brief 结束更新某个位置
     *
     * 从更新集合中移除位置。
     *
     * @param pos 更新位置
     */
    void endUpdate(const BlockPos& pos);

    /**
     * @brief 检查是否可以继续增加更新深度
     *
     * @return true 如果当前深度小于最大深度
     */
    [[nodiscard]] bool canPushDepth() const;

    /**
     * @brief 增加更新深度
     */
    void pushDepth();

    /**
     * @brief 减少更新深度
     */
    void popDepth();

    /**
     * @brief 获取当前更新深度
     */
    [[nodiscard]] i32 depth() const { return m_depth.load(std::memory_order_relaxed); }

    /**
     * @brief 清空所有状态
     *
     * 在世界卸载或重置时调用。
     */
    void clear();

    /**
     * @brief 获取当前正在更新的位置数量
     */
    [[nodiscard]] size_t updatingCount() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_set<BlockPos> m_updatingPositions;
    std::atomic<i32> m_depth{0};
};

} // namespace redstone
} // namespace world
} // namespace mc
