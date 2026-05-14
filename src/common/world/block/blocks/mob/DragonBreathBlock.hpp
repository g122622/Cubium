#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../Block.hpp"

namespace mc {

class IWorld;
class Entity;

namespace blocks {

/**
 * @brief 龙息方块
 *
 * 末影龙喷出的龙息滞留药水效果方块。
 *
 * 参考: net.minecraft.block.DragonBreathBlock
 */
class DragonBreathBlock : public Block {
public:
    explicit DragonBreathBlock(const BlockProperties& properties);
    ~DragonBreathBlock() override = default;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }
};

} // namespace blocks
} // namespace mc
