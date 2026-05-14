#pragma once

#include "ChestBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 陷阱箱方块
 *
 * 继承自箱子方块，额外提供红石信号输出功能。
 * 输出的红石信号强度等于打开箱子的玩家数量（最大15）。
 *
 * 参考: net.minecraft.block.TrappedChestBlock
 */
class TrappedChestBlock : public ChestBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TrappedChestBlock(const BlockProperties& properties);

    // ========== 红石 ==========

    /**
     * @brief 检查是否可以提供红石信号
     * @return 始终返回true
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const override { return true; }

    /**
     * @brief 获取弱红石信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 信号输出方向
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override;

    /**
     * @brief 获取强红石信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 信号输出方向
     * @return 仅从顶面输出强信号
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override;

    /**
     * @brief 获取方块实体类型
     * @return TrappedChest类型
     */
    [[nodiscard]] BlockEntityType getBlockEntityType() const override { return BlockEntityType::TrappedChest; }
};

} // namespace blocks
} // namespace mc
