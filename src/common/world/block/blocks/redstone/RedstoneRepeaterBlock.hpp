#pragma once

#include "RedstoneDiodeBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 红石中继器方块
 *
 * 红石中继器可以延长和延迟红石信号，还可以锁定信号状态。
 *
 * ## 特性
 * - 信号再生：输出始终为15强度
 * - 延迟可调：1-4档，对应2-8 tick延迟
 * - 锁定机制：侧面有信号时锁定当前输出
 * - 方向性：只能从背面输入，正面输出
 *
 * ## 容易踩的坑
 * - 延迟 = 档位 × 2 tick
 * - 锁定时保持当前输出不变
 * - 面向其他中继器时使用更高优先级
 *
 * 参考: net.minecraft.block.RepeaterBlock
 */
class RedstoneRepeaterBlock : public RedstoneDiodeBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneRepeaterBlock(const BlockProperties& properties);

    // ========== 红石二极管接口实现 ==========

    [[nodiscard]] i32 getDelay(const BlockState& state) const override;

    [[nodiscard]] bool shouldBePowered(IWorld& world, const BlockPos& pos,
                                       const BlockState& state) const override;

    [[nodiscard]] bool isLocked(IWorld& world, const BlockPos& pos,
                                const BlockState& state) const override;

    // ========== 中继器特有方法 ==========

    /**
     * @brief 获取延迟档位
     *
     * @param state 方块状态
     * @return i32 延迟档位（1-4）
     */
    [[nodiscard]] static i32 getDelaySetting(const BlockState& state);

    /**
     * @brief 设置延迟档位
     *
     * @param state 方块状态
     * @param delay 延迟档位（1-4）
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withDelay(BlockState state, i32 delay);

    /**
     * @brief 检查是否锁定
     *
     * @param state 方块状态
     * @return true 如果锁定
     */
    [[nodiscard]] static bool isLockedState(const BlockState& state);

private:
    /// 最小延迟档位
    static constexpr i32 MIN_DELAY = 1;
    /// 最大延迟档位
    static constexpr i32 MAX_DELAY = 4;
    /// 每档延迟 tick 数
    static constexpr i32 DELAY_MULTIPLIER = 2;
};

} // namespace blocks
} // namespace mc
