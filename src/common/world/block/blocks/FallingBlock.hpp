#pragma once

#include "../Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 可下落方块基类
 *
 * 用于沙子、红沙、砾石等会受重力影响的方块。
 * 当下方方块无法支撑时，调度计划刻并生成下落方块实体。
 *
 * 参考: net.minecraft.block.FallingBlock
 */
class FallingBlock : public Block {
public:
    explicit FallingBlock(const BlockProperties& properties);
    ~FallingBlock() override = default;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(IWorld& world, const BlockPos& pos,
                         Block& neighborBlock, const BlockPos& neighborPos,
                         bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

private:
    [[nodiscard]] bool canFallThrough(const BlockState* state) const;

    static constexpr i32 FALL_DELAY_TICKS = 2;
};

} // namespace blocks
} // namespace mc
