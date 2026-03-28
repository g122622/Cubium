#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

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

    [[nodiscard]] ActionResult onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const override { return true; }
};

/**
 * @brief 海龟蛋方块
 *
 * 海龟产下的蛋，会缓慢孵化。
 *
 * 状态属性：
 * - EGGS_1_4: 蛋的数量 (1-4)
 * - HATCH_0_2: 孵化阶段 (0-2)
 *
 * 参考: net.minecraft.block.TurtleEggBlock
 */
class TurtleEggBlock : public Block {
public:
    explicit TurtleEggBlock(const BlockProperties& properties);
    ~TurtleEggBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getEggs(const BlockState& state) const;
    [[nodiscard]] BlockState withEggs(i32 count) const;

    [[nodiscard]] i32 getHatch(const BlockState& state) const;
    [[nodiscard]] BlockState withHatch(i32 hatch) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 孵化逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    std::array<CollisionShape, 4> m_shapesByEggCount;
};

/**
 * @brief 被感染方块基类
 *
 * 外观与普通方块相同，但被破坏时会生成蠹虫。
 *
 * 参考: net.minecraft.block.InfestedBlock
 */
class InfestedBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param hostBlock 被感染的方块ID
     * @param properties 方块属性
     */
    InfestedBlock(u32 hostBlock, const BlockProperties& properties);
    ~InfestedBlock() override = default;

    // ========== 破坏 ==========

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 掉落 ==========

    [[nodiscard]] u32 getHostBlock() const { return m_hostBlock; }

private:
    /// 被感染的方块ID
    u32 m_hostBlock;
};

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

    [[nodiscard]] ActionResult onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const override { return true; }

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }
};

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

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }
};

} // namespace blocks
} // namespace mc
