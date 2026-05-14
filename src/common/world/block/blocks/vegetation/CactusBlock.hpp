#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 仙人掌方块
 *
 * 生长在沙漠的仙人掌，可以堆叠生长。
 * 接触仙人掌会造成伤害。
 *
 * 状态属性：
 * - AGE_0_15: 年龄（用于纹理）
 *
 * 参考: net.minecraft.block.CactusBlock
 */
class CactusBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CactusBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~CactusBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] const BlockState& withAge(i32 age) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    /// 各年龄阶段的形状
    std::array<CollisionShape, 16> m_shapesByAge;
};

} // namespace blocks
} // namespace mc
