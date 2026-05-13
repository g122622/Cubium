#pragma once

#include "../../Block.hpp"
#include "../../../../util/property/Properties.hpp"

namespace mc {

class IWorld;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 岩浆块方块
 *
 * 下界的岩浆块，站在上面会受伤，在水中会产生气泡柱。
 *
 * MC ID: minecraft:magma_block
 *
 * 参考 MC 1.16.5 MagmaBlock
 */
class MagmaBlock : public Block {
public:
    explicit MagmaBlock(BlockProperties properties);

    /**
     * @brief 方块被添加时
     *
     * 调度 tick 以检查气泡柱。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居更新
     *
     * 当上方有水时调度 tick。
     */
    void neighborChanged(
        IWorld& world,
        const BlockPos& pos,
        Block& neighborBlock,
        const BlockPos& neighborPos,
        bool isMoving
    ) override;

    /**
     * @brief Tick 更新
     *
     * 在上方生成气泡柱。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 随机刻
     *
     * 在水中产生气泡效果。
     */
    void randomTick(
        IWorld& world,
        const BlockPos& pos,
        BlockState& state,
        math::IRandom& random
    ) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const override { return true; }
};

} // namespace blocks
} // namespace mc
