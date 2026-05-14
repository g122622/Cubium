#pragma once

#include "../../Block.hpp"

namespace mc {

class IWorld;
class Player;

namespace blocks {

/**
 * @brief 刷怪笼方块
 *
 * 自动生成生物的方块。
 *
 * 参考: net.minecraft.block.SpawnerBlock
 */
class SpawnerBlock : public Block {
public:
    explicit SpawnerBlock(const BlockProperties& properties);
    ~SpawnerBlock() override = default;

    // ========== 交互 ==========

    [[nodiscard]] ActionResultType onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const override { return true; }

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }
};

} // namespace blocks
} // namespace mc
