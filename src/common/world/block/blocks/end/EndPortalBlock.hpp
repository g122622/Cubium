#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 末地传送门方块
 *
 * 进入后传送到末地的传送门方块。
 * 由末地传送门框架组成，放满末影之眼后激活。
 *
 * 状态属性：无
 *
 * 参考: net.minecraft.block.EndPortalBlock
 */
class EndPortalBlock : public Block {
public:
    explicit EndPortalBlock(const BlockProperties& properties);
    ~EndPortalBlock() override = default;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

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

/**
 * @brief 末地传送门框架方块
 *
 * 用于构建末地传送门的框架方块。
 * 可以放入末影之眼。
 *
 * 状态属性：
 * - EYE: 是否有眼
 * - FACING: 朝向（水平）
 *
 * 参考: net.minecraft.block.EndPortalFrameBlock
 */
class EndPortalFrameBlock : public Block {
public:
    explicit EndPortalFrameBlock(const BlockProperties& properties);
    ~EndPortalFrameBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] bool hasEye(const BlockState& state) const;
    [[nodiscard]] Direction getFacing(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    CollisionShape m_frameShape;
    CollisionShape m_frameWithEyeShape;
};

/**
 * @brief 末地折跃门方块
 *
 * 在末地城和主岛之间传送的方块。
 *
 * 状态属性：无
 *
 * 参考: net.minecraft.block.EndGatewayBlock
 */
class EndGatewayBlock : public Block {
public:
    explicit EndGatewayBlock(const BlockProperties& properties);
    ~EndGatewayBlock() override = default;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

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

/**
 * @brief 紫颂植物方块
 *
 * 末地的植物，可以生长紫颂果。
 *
 * 状态属性：
 * - NORTH/SOUTH/EAST/WEST/DOWN/UP: 各方向连接
 *
 * 参考: net.minecraft.block.ChorusPlantBlock
 */
class ChorusPlantBlock : public Block {
public:
    explicit ChorusPlantBlock(const BlockProperties& properties);
    ~ChorusPlantBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    /**
     * @brief 检查是否连接到指定方向
     */
    [[nodiscard]] bool canConnect(IBlockReader& world, const BlockPos& pos, Direction direction) const;

    CollisionShape m_centerShape;
    CollisionShape m_armShapes[6];  // 6个方向
};

/**
 * @brief 紫颂花方块
 *
 * 紫颂植物的顶部，可以生长。
 *
 * 状态属性：
 * - AGE_0_5: 生长阶段
 *
 * 参考: net.minecraft.block.ChorusFlowerBlock
 */
class ChorusFlowerBlock : public Block {
public:
    explicit ChorusFlowerBlock(const BlockProperties& properties);
    ~ChorusFlowerBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] BlockState withAge(i32 age) const;
    [[nodiscard]] i32 getMaxAge() const { return 5; }

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    std::array<CollisionShape, 6> m_shapesByAge;
};

/**
 * @brief 龙蛋方块
 *
 * 末影龙死亡后掉落的方块，点击会传送。
 *
 * 参考: net.minecraft.block.DragonEggBlock
 */
class DragonEggBlock : public Block {
public:
    explicit DragonEggBlock(const BlockProperties& properties);
    ~DragonEggBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 交互 ==========

    [[nodiscard]] ActionResult onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    /**
     * @brief 传送龙蛋到新位置
     */
    void teleport(IWorld& world, const BlockPos& pos, const BlockState& state);

    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
