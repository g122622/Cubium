#pragma once

#include "FireBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 灵魂火方块
 *
 * 在下界生成的蓝色火焰，伤害更高。
 * 只能在灵魂沙或灵魂土上点燃。
 *
 * MC ID: minecraft:soul_fire
 *
 * 参考: net.minecraft.block.SoulFireBlock
 */
class SoulFireBlock : public FireBlock {
public:
    explicit SoulFireBlock(const BlockProperties& properties);
    ~SoulFireBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 检查方块是否可以作为灵魂火的基座
     *
     * 参考 MC 1.16.5: SoulFireBlock.func_235577_c_
     *
     * @param block 要检查的方块
     * @return 如果方块是灵魂沙或灵魂土，返回 true
     */
    [[nodiscard]] static bool isSoulFireBase(const Block* block);

protected:
    [[nodiscard]] bool canBurn(IBlockReader& world, const BlockPos& pos) const override;
};

} // namespace blocks
} // namespace mc
