#pragma once

#include "../agricultural/BushBlock.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 睡莲方块
 *
 * 漂浮在水面上的扁平植物。
 * 可以放置在水面上，玩家可以在上面行走。
 *
 * 参考: net.minecraft.block.LilyPadBlock
 */
class LilyPadBlock : public BushBlock {
public:
    explicit LilyPadBlock(const BlockProperties& properties);
    ~LilyPadBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

protected:
    /**
     * @brief 检查下方是否可支撑（水）
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState,
        IWorld& world,
        const BlockPos& groundPos) const override;
};

} // namespace blocks
} // namespace mc
