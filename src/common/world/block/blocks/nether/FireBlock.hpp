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

    // ========== 放置回调 ==========

    /**
     * @brief 方块被添加到世界时调用
     *
     * 参考 MC 1.16.5 AbstractFireBlock.onBlockAdded
     * 在放置时立即检测并点燃下界传送门（而不是在tick时）。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 新方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

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
     * @brief 检查位置是否可以燃烧（有可燃方块）
     *
     * 检查指定位置周围是否有可燃方块。
     * 参考 MC 1.16.5: FireBlock.canBurn()
     *
     * @param world 世界读取器
     * @param pos 要检查的位置
     * @return 如果位置可以燃烧返回 true
     */
    [[nodiscard]] virtual bool canBurn(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 尝试火焰蔓延
     *
     * 在火焰 tick 时调用，尝试将火焰蔓延到周围方块。
     * 参考 MC 1.16.5: FireBlock.tick() 中的蔓延逻辑
     *
     * @param world 世界引用
     * @param pos 火焰位置
     * @param age 火焰年龄 (0-15)
     * @param random 随机数生成器
     */
    void trySpread(IWorld& world, const BlockPos& pos, i32 age, math::IRandom& random);

    /**
     * @brief 尝试点燃下界传送门
     *
     * 检查火焰周围是否形成有效的下界传送门框架，
     * 如果有效则点燃传送门。
     *
     * 参考 MC 1.16.5 AbstractFireBlock.onBlockAdded
     *
     * @param world 世界引用
     * @param pos 火焰位置
     * @return 是否成功点燃传送门
     */
    bool tryLightNetherPortal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块是否可燃
     */
    [[nodiscard]] bool isFlammable(const BlockState& state) const;

    /**
     * @brief 检查火焰是否会被雨淋灭
     *
     * 参考 MC 1.16.5: FireBlock.canDie()
     *
     * @param world 世界引用
     * @param pos 火焰位置
     * @return 如果火焰会被雨淋灭返回 true
     */
    [[nodiscard]] bool canDie(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查指定位置是否会被雨淋灭
     *
     * 同 canDie，用于远距离蔓延检查。
     *
     * @param world 世界引用
     * @param pos 位置
     * @return 如果位置会被雨淋灭返回 true
     */
    [[nodiscard]] bool canDieAt(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 获取目标位置周围的火焰蔓延鼓励值
     *
     * 参考 MC 1.16.5: FireBlock.getNeighborEncouragement()
     *
     * @param world 世界引用
     * @param pos 目标位置
     * @return 最大火焰蔓延速度
     */
    [[nodiscard]] i32 getNeighborEncouragement(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 尝试点燃指定位置
     *
     * 参考 MC 1.16.5: FireBlock.tryCatchFire()
     *
     * @param world 世界引用
     * @param pos 目标位置
     * @param chance 基础点燃概率分母
     * @param random 随机数生成器
     * @param age 火焰年龄
     * @param face 点燃面
     */
    void tryCatchFire(IWorld& world, const BlockPos& pos, i32 chance, math::IRandom& random, i32 age, Direction face);

    /**
     * @brief 检查周围是否有可燃方块
     *
     * 参考 MC 1.16.5: FireBlock.areNeighborsFlammable()
     *
     * @param world 世界读取器
     * @param pos 位置
     * @return 如果周围有可燃方块返回 true
     */
    [[nodiscard]] bool areNeighborsFlammable(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 检查位置是否可以被点燃
     *
     * 参考 MC 1.16.5: FireBlock.canCatchFire() / IForgeBlock.canCatchFire()
     *
     * @param world 世界引用
     * @param pos 目标位置
     * @param face 点燃面
     * @return 如果位置可以被点燃返回 true
     */
    [[nodiscard]] bool canCatchFire(IWorld& world, const BlockPos& pos, Direction face) const;

    /// 火焰伤害
    i32 m_fireDamage;

    /// 火焰形状
    CollisionShape m_shape;
};

/**
 * @brief 灵魂火方块
 * TODO 移到单独文件中，很简单
 *
 * 在下界生成的蓝色火焰，伤害更高。
 * 只能在灵魂沙或灵魂土上点燃。
 *
 * 参考: net.minecraft.block.SoulFireBlock
 */
class SoulFireBlock : public FireBlock {
public:
    explicit SoulFireBlock(const BlockProperties& properties);
    ~SoulFireBlock() override = default;

    // ========== 放置逻辑 ==========

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

/**
 * @brief 下界传送门方块
 * TODO 移到单独文件中，很简单
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
 * TODO 移到单独文件中，很简单
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
