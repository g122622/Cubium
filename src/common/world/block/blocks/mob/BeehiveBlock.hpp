#pragma once

#include "../../Block.hpp"
#include "../../BlockTags.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Player;

namespace blocks {

/**
 * @brief 蜂巢/蜂箱方块
 *
 * 蜜蜂居住和产蜜的方块。
 *
 * 状态属性：
 * - HONEY_LEVEL_0_5: 蜂蜜等级 (0-5)
 * - FACING: 朝向
 *
 * 参考: net.minecraft.block.BeehiveBlock
 */
class BeehiveBlock : public Block {
public:
    explicit BeehiveBlock(const BlockProperties& properties);
    ~BeehiveBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getHoneyLevel(const BlockState& state) const;
    [[nodiscard]] BlockState withHoneyLevel(i32 level) const;
    [[nodiscard]] i32 getMaxHoneyLevel() const { return 5; }

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 交互 ==========

    [[nodiscard]] ActionResultType onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const override { return true; }
};

} // namespace blocks
} // namespace mc
