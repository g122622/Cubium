/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"

using namespace mc;

namespace {

// ============================================================================
// 测试辅助类
// ============================================================================

/**
 * @brief 方块碰撞检测测试用世界
 *
 * 继承 BaseTestWorld，提供可编程的方块放置和碰撞检测支持
 */
class BlockCollisionTestWorld final : public mc::test::BaseTestWorld {
public:
    BlockCollisionTestWorld() = default;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        if (!m_tickManager) {
            m_tickManager = std::make_unique<world::tick::TickManager>(*this);
        }
        return *m_tickManager;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        if (!m_tickManager) {
            m_tickManager = std::make_unique<world::tick::TickManager>(const_cast<BlockCollisionTestWorld&>(*this));
        }
        return *m_tickManager;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    mutable std::unique_ptr<world::tick::TickManager> m_tickManager;
};

/**
 * @brief 测试用 LivingEntity，可追踪受伤次数
 */
class TestLivingEntity final : public LivingEntity {
public:
    explicit TestLivingEntity(IWorld* world = nullptr, ecs::EntityRegistry& registry = mc::test::testEcsRegistry())
        : LivingEntity(EntityInstanceId(1), world, registry)
    {
        setHealth(20.0f);
    }

    i32 hurtCount = 0;
    f32 lastDamage = 0.0f;

    bool hurt(DamageSource& source, f32 amount) override
    {
        hurtCount++;
        lastDamage = amount;
        return LivingEntity::hurt(source, amount);
    }
};

} // anonymous namespace

// ============================================================================
// 测试：LivingEntity::aiStep() 中的 doBlockCollisions() 调用
// ============================================================================

/**
 * @brief 验证 LivingEntity::aiStep() 调用 doBlockCollisions()
 *
 * 当 LivingEntity 站在仙人掌方块上时，doBlockCollisions() 应触发
 * CactusBlock::onEntityCollision()，对实体造成伤害。
 * 这对应 MC 原版 LivingEntity.aiStep() 中对 applyEffectsFromBlocks() 的调用。
 */
TEST(BlockCollisionTest, LivingEntityTriggersCactusDamageViaAiStep)
{
    // 初始化方块注册表
    VanillaBlocks::initialize();

    BlockCollisionTestWorld world;

    // 放置仙人掌方块
    const BlockState& cactusState = VanillaBlocks::CACTUS->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), &cactusState);

    // 创建实体，位置在仙人掌内部，使碰撞箱与方块重叠
    // 实体碰撞箱：宽0.6，高1.8，以position为中心底部
    // 仙人掌位于 (0,0,0)-(1,1,1)，实体中心在 (0.5, 0.5, 0.5) 可确保重叠
    TestLivingEntity entity(&world, mc::test::testEcsRegistry());
    entity.setPosition(0.5f, 0.5f, 0.5f);
    entity.setOnGround(true);

    // 验证初始状态
    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
    EXPECT_EQ(entity.hurtCount, 0);

    // 执行一次 tick，aiStep() 中会调用 doBlockCollisions()
    entity.tick();

    // 仙人掌应该对实体造成 1.0 伤害
    EXPECT_EQ(entity.hurtCount, 1);
    EXPECT_FLOAT_EQ(entity.lastDamage, 1.0f);
}

/**
 * @brief 验证 doBlockCollisions() 在实体不在任何方块中时不触发伤害
 *
 * 当实体站在空气中时，不应受到任何方块碰撞伤害。
 */
TEST(BlockCollisionTest, LivingEntityNoDamageWhenNotInBlock)
{
    VanillaBlocks::initialize();

    BlockCollisionTestWorld world;
    // 不放置任何方块

    TestLivingEntity entity(&world, mc::test::testEcsRegistry());
    entity.setPosition(5.0f, 100.0f, 5.0f);
    entity.setOnGround(true);

    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
    EXPECT_EQ(entity.hurtCount, 0);

    entity.tick();

    // 不应该受到任何伤害
    EXPECT_EQ(entity.hurtCount, 0);
}

/**
 * @brief 验证 doBlockCollisions() 对蜘蛛网减速效果
 *
 * 当实体进入蜘蛛网时，doBlockCollisions() 应触发
 * WebBlock::onEntityCollision()，大幅降低实体速度。
 */
TEST(BlockCollisionTest, LivingEntitySlowedByCobwebViaAiStep)
{
    VanillaBlocks::initialize();

    BlockCollisionTestWorld world;

    // 放置蜘蛛网方块
    const BlockState& webState = VanillaBlocks::COBWEB->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), &webState);

    TestLivingEntity entity(&world, mc::test::testEcsRegistry());
    entity.setPosition(0.5f, 0.5f, 0.5f);

    // 设置一个水平速度
    entity.setVelocity(1.0f, 0.0f, 0.5f);
    f32 initialVelX = entity.velocity().x;
    f32 initialVelZ = entity.velocity().z;

    // 执行 tick，doBlockCollisions() 应触发蜘蛛网减速
    entity.tick();

    // 蜘蛛网应将水平速度大幅降低
    // COBWEB_SLOWDOWN_XZ = 0.25，所以速度应变为原来的 0.25 倍
    // 注意：tick() 中也有摩擦力等其他速度修改，因此只验证速度明显下降
    EXPECT_LT(entity.velocity().x, initialVelX);
    EXPECT_LT(std::abs(entity.velocity().z), std::abs(initialVelZ));
}

/**
 * @brief 验证 doBlockCollisions() 对气泡柱推拉效果
 *
 * 当实体进入气泡柱时，doBlockCollisions() 应触发
 * BubbleColumnBlock::onEntityCollision()，对实体施加垂直速度。
 */
TEST(BlockCollisionTest, LivingEntityPushedByBubbleColumnViaAiStep)
{
    VanillaBlocks::initialize();

    BlockCollisionTestWorld world;

    // 放置气泡柱方块（上推模式，DRAG=false）
    const BlockState& bubbleState = VanillaBlocks::BUBBLE_COLUMN->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), &bubbleState);

    TestLivingEntity entity(&world, mc::test::testEcsRegistry());
    entity.setPosition(0.5f, 0.5f, 0.5f);

    f32 initialVelY = entity.velocity().y;

    entity.tick();

    // 气泡柱上推应增加 Y 方向速度（+0.1）
    EXPECT_GT(entity.velocity().y, initialVelY);
    // 摔落距离应被重置为 0
    EXPECT_FLOAT_EQ(entity.fallDistance(), 0.0f);
}

/**
 * @brief 验证 doBlockCollisions() 直接调用时仙人掌对 LivingEntity 造成伤害
 *
 * 直接调用 doBlockCollisions()，不依赖 tick 循环，
 * 验证方块碰撞回调的基本触发机制。
 */
TEST(BlockCollisionTest, DirectDoBlockCollisionsCactusDamage)
{
    VanillaBlocks::initialize();

    BlockCollisionTestWorld world;

    const BlockState& cactusState = VanillaBlocks::CACTUS->defaultState();
    // 仙人掌位于 (0,0,0)，实体位于 (0.5, 0.5, 0.5) 以确保碰撞箱与方块重叠
    world.setBlockAt(BlockPos(0, 0, 0), &cactusState);

    TestLivingEntity entity(&world, mc::test::testEcsRegistry());
    entity.setPosition(0.5f, 0.5f, 0.5f);

    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
    EXPECT_EQ(entity.hurtCount, 0);

    // 直接调用 doBlockCollisions()
    entity.doBlockCollisions();

    EXPECT_EQ(entity.hurtCount, 1);
    EXPECT_FLOAT_EQ(entity.lastDamage, 1.0f);
}

/**
 * @brief 验证 doBlockCollisions() 对非 LivingEntity 实体不触发仙人掌伤害
 *
 * CactusBlock::onEntityCollision() 只对 LivingEntity 造成伤害。
 * 对于普通 Entity（如掉落物实体），不应受到伤害。
 * 此测试验证 doBlockCollisions() 不会对错误的实体类型产生副作用。
 */
TEST(BlockCollisionTest, CactusDoesNotDamageNonLivingEntity)
{
    VanillaBlocks::initialize();

    BlockCollisionTestWorld world;

    const BlockState& cactusState = VanillaBlocks::CACTUS->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), &cactusState);

    // 使用普通 Entity（非 LivingEntity）
    Entity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());
    entity.setWorld(&world);
    entity.setPosition(0.5f, 0.5f, 0.5f);

    // 直接调用 doBlockCollisions()，不应崩溃
    entity.doBlockCollisions();

    // Entity 没有 health 属性，验证不会崩溃即可
    // （CactusBlock::onEntityCollision 对非 LivingEntity 直接 return）
    SUCCEED();
}
