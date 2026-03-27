#pragma once

#include "../../Block.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 侦测器方块
 *
 * 侦测器可以检测方块变化并输出短脉冲。
 *
 * ## 特性
 * - 方块变化检测：检测前端方块的变化
 * - 2 tick脉冲输出
 * - 方向性：只能从背面输出
 * - 观察面和输出面分离
 *
 * ## 容易踩的坑
 * - 脉冲输出需要精确的tick控制
 * - 方块放置/破坏检测
 * - 方向性处理
 *
 * 参考: net.minecraft.block.ObserverBlock
 */
class ObserverBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ObserverBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state, Direction facing,
        const BlockState& facingState, IWorld& world,
        const BlockPos& currentPos, const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side
    ) const override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side
    ) const override;

    // ========== 侦测器特有方法 ==========

    /**
     * @brief 获取侦测器朝向（输出方向）
     *
     * @param state 方块状态
     * @return Direction 输出方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 检查是否正在输出信号
     *
     * @param state 方块状态
     * @return true 如果正在输出
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 设置输出状态
     *
     * @param state 方块状态
     * @param powered 是否输出
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPowered(BlockState state, bool powered);

private:
    /**
     * @brief 检测并触发
     *
     * 当检测到变化时，调度脉冲输出。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void detect(IWorld& world, const BlockPos& pos, const BlockState& state);

    /// 脉冲持续时间（tick）
    static constexpr i32 PULSE_DURATION = 2;
};

} // namespace blocks
} // namespace mc
