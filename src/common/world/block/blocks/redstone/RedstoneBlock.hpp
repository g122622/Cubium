#pragma once

#include "../../Block.hpp"
#include "../../../redstone/RedstonePower.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 红石块方块
 *
 * 实体方块，始终输出15强度红石信号。
 * 可以被活塞推动。
 *
 * ## 特性
 * - 始终输出强度15的信号
 * - 强信号输出到所有六个方向
 * - 可以被活塞推动
 * - 不需要方块实体
 *
 * 参考: net.minecraft.block.RedstoneBlock
 */
class RedstoneBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneBlock(const BlockProperties& properties);

    // ========== 红石接口 ==========

    /**
     * @brief 检查是否可以提供红石信号
     * @return 始终返回true
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取弱红石信号
     *
     * 红石块输出弱信号强度15到所有方向。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 信号输出方向
     * @return 信号强度15
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side
    ) const override;

    /**
     * @brief 获取强红石信号
     *
     * 红石块输出强信号强度15到所有方向。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 信号输出方向
     * @return 信号强度15
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side
    ) const override;
};

} // namespace blocks
} // namespace mc
