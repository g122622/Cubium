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
 * @brief 火方块基类
 *
 * 火的基类，包括普通火和灵魂火。
 * 火可以蔓延到可燃方块上。
 *
 * 状态属性：
 * - AGE_0_15: 火的年龄（影响蔓延）
 * - NORTH/SOUTH/EAST/WEST/UP: 各方向的连接
 *
 * 参考: net.minecraft.block.FireBlock
 */
class FireBlock : public Block {
public:
    explicit FireBlock(const BlockProperties& properties, i32 fireDamage = 1);
    ~FireBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] BlockState withAge(i32 age) const;

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

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

protected:
    /**
     * @brief 检查是否可以燃烧
     */
    [[nodiscard]] virtual bool canBurn(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 尝试蔓延到周围
     */
    void trySpread(IWorld& world, const BlockPos& pos, i32 age, math::IRandom& random);

    /**
     * @brief 尝试点燃下界传送门
     *
     * 检查火焰周围是否形成有效的下界传送门框架，
     * 如果有效则点燃传送门。
     *
     * @param world 世界引用
     * @param pos 火焰位置
     */
    void tryLightNetherPortal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块是否可燃
     */
    [[nodiscard]] bool isFlammable(const BlockState& state) const;

    /// 火焰伤害
    i32 m_fireDamage;

    /// 火焰形状
    CollisionShape m_shape;
};

/**
 * @brief 灵魂火方块
 *
 * 在下界生成的蓝色火焰，伤害更高。
 *
 * 参考: net.minecraft.block.SoulFireBlock
 */
class SoulFireBlock : public FireBlock {
public:
    explicit SoulFireBlock(const BlockProperties& properties);
    ~SoulFireBlock() override = default;

protected:
    [[nodiscard]] bool canBurn(IBlockReader& world, const BlockPos& pos) const override;
};

/**
 * @brief 下界传送门方块
 *
 * 下界传送门的紫色方块，玩家可以穿过传送到下界。
 *
 * 状态属性：
 * - HORIZONTAL_AXIS: 传送门轴向 (X 或 Z)
 *
 * 参考: net.minecraft.block.NetherPortalBlock
 */
class NetherPortalBlock : public Block {
public:
    explicit NetherPortalBlock(const BlockProperties& properties);
    ~NetherPortalBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] Axis getAxis(const BlockState& state) const;

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
    CollisionShape m_xAxisShape;
    CollisionShape m_zAxisShape;
};

/**
 * @brief 下界疣方块
 *
 * 在下界自然生成的红色方块，可以放置在任何地方。
 *
 * 参考: net.minecraft.block.NetherWartBlock
 */
class NetherWartBlock : public Block {
public:
    explicit NetherWartBlock(const BlockProperties& properties);
    ~NetherWartBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] BlockState withAge(i32 age) const;
    [[nodiscard]] i32 getMaxAge() const { return 3; }

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

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    std::array<CollisionShape, 4> m_shapesByAge;
};

} // namespace blocks
} // namespace mc
