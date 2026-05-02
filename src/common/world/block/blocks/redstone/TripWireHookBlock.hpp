#pragma once

#include "world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 绊线钩方块
 *
 * 绊线钩是绊线系统的触发器，用于检测绊线的状态变化并输出红石信号。
 *
 * ## 特性
 * - 可附着在方块侧面
 * - 与绊线配合使用
 * - 最大检测距离42格
 * - 红石信号输出
 * - 被剪断时触发
 *
 * 参考: net.minecraft.block.TripWireHookBlock
 */
class TripWireHookBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TripWireHookBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state, Direction facing,
        const BlockState& facingState, IWorld& world,
        const BlockPos& currentPos, const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(const BlockState& state, IWorld& world,
                                   const BlockPos& pos, Direction side) const override;

    [[nodiscard]] i32 getStrongPower(const BlockState& state, IWorld& world,
                                     const BlockPos& pos, Direction side) const override;

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

    // ========== 绊线钩特有方法 ==========

    /**
     * @brief 检查绊线钩是否被触发
     * @param state 方块状态
     * @return true 如果被触发
     */
    [[nodiscard]] static bool isPowered(const BlockState& state);

    /**
     * @brief 检查绊线钩是否连接
     * @param state 方块状态
     * @return true 如果连接
     */
    [[nodiscard]] static bool isConnected(const BlockState& state);

    /**
     * @brief 获取绊线钩朝向
     * @param state 方块状态
     * @return Direction 朝向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 设置触发状态
     * @param state 方块状态
     * @param powered 是否触发
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withPowered(BlockState state, bool powered);

    /**
     * @brief 设置连接状态
     * @param state 方块状态
     * @param connected 是否连接
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withConnected(BlockState state, bool connected);

private:
    /**
     * @brief 计算并更新绊线钩状态
     * @param world 世界引用
     * @param pos 绊线钩位置
     * @param facing 朝向
     * @param currentState 当前方块状态
     * @param shouldTriggerOnChange 是否在状态改变时触发
     * @return true 如果成功连接
     */
    bool calculateState(IWorld& world, const BlockPos& pos, Direction facing,
                        const BlockState& currentState, bool shouldTriggerOnChange);

    /**
     * @brief 检测绊线链
     * @param world 世界引用
     * @param pos 绊线钩位置
     * @param facing 朝向
     * @param outOtherHookPos 输出：另一端绊线钩位置
     * @return true 如果找到完整的绊线链
     */
    [[nodiscard]] bool checkForTripwire(IWorld& world, const BlockPos& pos,
                                         Direction facing, BlockPos& outOtherHookPos) const;
};

} // namespace blocks
} // namespace mc
