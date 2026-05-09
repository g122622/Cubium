#pragma once

#include "../../Block.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 海草方块
 *
 * 水下植物，可以放置在水下地面上。
 * 可以通过骨粉催熟变成高海草。
 *
 * ## 特性
 * - 单格水下植物
 * - 需要固体支撑
 * - 可用骨粉催熟变成高海草
 *
 * 参考: net.minecraft.block.SeaGrassBlock
 */
class SeagrassBlock : public Block {
public:
    explicit SeagrassBlock(const BlockProperties& properties);
    ~SeagrassBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
