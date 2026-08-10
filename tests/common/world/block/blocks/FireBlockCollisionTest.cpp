/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/EntityType.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/damage/DamageSource.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 火焰碰撞测试用实体
 *
 * 简单的 LivingEntity 实现，用于测试火焰碰撞伤害
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity(EntityInstanceId id, IWorld* world, ecs::EntityRegistry& registry)
        : LivingEntity(id, world, registry)
        , m_hurtCount(0)
        , m_lastDamage(0.0f)
        , m_lastDamageType(static_cast<DamageType>(-1))
    {}

    // 重写 hurt 方法以追踪伤害
    bool hurt(DamageSource& source, f32 amount) override
    {
        m_hurtCount++;
        m_lastDamage = amount;
        m_lastDamageType = source.type();
        return LivingEntity::hurt(source, amount);
    }

    [[nodiscard]] i32 hurtCount() const { return m_hurtCount; }
    [[nodiscard]] f32 lastDamage() const { return m_lastDamage; }
    [[nodiscard]] DamageType lastDamageType() const { return m_lastDamageType; }

    void resetHurtStats()
    {
        m_hurtCount = 0;
        m_lastDamage = 0.0f;
        m_lastDamageType = static_cast<DamageType>(-1);
    }

protected:
    i32 m_hurtCount;
    f32 m_lastDamage;
    DamageType m_lastDamageType;
};

/**
 * @brief 火焰免疫测试用实体
 */
class FireImmuneTestEntity : public TestLivingEntity {
public:
    FireImmuneTestEntity(EntityInstanceId id, IWorld* world, ecs::EntityRegistry& registry)
        : TestLivingEntity(id, world, registry)
        , m_immuneToFire(true)
    {}

    [[nodiscard]] bool isImmuneToFire() const override { return m_immuneToFire; }

    void setImmuneToFire(bool immune) { m_immuneToFire = immune; }

private:
    bool m_immuneToFire;
};

/**
 * @brief 火焰测试用世界
 */
class FireTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

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

    [[nodiscard]] bool isUltraWarm() const override { return false; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<FireTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

private:
    void ensureTickManager() const
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(const_cast<FireTestWorld&>(*this));
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    mutable std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
};

class FireBlockCollisionTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ========== Entity 火焰方法测试 ==========

TEST_F(FireBlockCollisionTest, Entity_GetFireTimer_EqualsFire)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    // 初始值应为 0
    EXPECT_EQ(entity.fire(), 0);
    EXPECT_EQ(entity.getFireTimer(), 0);

    // 设置火焰（setFire 接收 ticks）
    entity.setFire(100); // 100 ticks = 5 秒
    EXPECT_EQ(entity.fire(), 100);
    EXPECT_EQ(entity.getFireTimer(), 100);

    // getFireTimer 应该与 fire 相同
    entity.forceFireTicks(50);
    EXPECT_EQ(entity.fire(), 50);
    EXPECT_EQ(entity.getFireTimer(), 50);
}

TEST_F(FireBlockCollisionTest, Entity_SetFire_OnlyIncreases)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    // 设置初始火焰
    entity.setFire(100); // 100 ticks
    EXPECT_EQ(entity.fire(), 100);

    // 尝试设置更小的值，不应改变
    entity.setFire(60);            // 60 ticks < 100 ticks
    EXPECT_EQ(entity.fire(), 100); // 保持 100

    // 设置更大的值，应该更新
    entity.setFire(200);           // 200 ticks > 100 ticks
    EXPECT_EQ(entity.fire(), 200); // 更新为 200
}

TEST_F(FireBlockCollisionTest, Entity_ForceFireTicks_SetsDirectly)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    entity.setFire(200);
    EXPECT_EQ(entity.fire(), 200);

    // forceFireTicks 可以减少值
    entity.forceFireTicks(50);
    EXPECT_EQ(entity.fire(), 50);

    // forceFireTicks 可以设置为负值（用于短暂火焰免疫期）
    entity.forceFireTicks(-10);
    EXPECT_EQ(entity.fire(), -10);

    // forceFireTicks 可以设置为 0
    entity.forceFireTicks(0);
    EXPECT_EQ(entity.fire(), 0);
}

TEST_F(FireBlockCollisionTest, Entity_IsImmuneToFire_DefaultFalse)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    // 默认情况下，实体不免疫火焰（取决于 EntityType）
    // TestLivingEntity 没有注册到 EntityRegistry，所以默认返回 false
    EXPECT_FALSE(entity.isImmuneToFire());
}

TEST_F(FireBlockCollisionTest, Entity_IsImmuneToFire_Overrideable)
{
    FireTestWorld world;
    FireImmuneTestEntity immuneEntity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    // 默认免疫
    EXPECT_TRUE(immuneEntity.isImmuneToFire());

    // 可以关闭免疫
    immuneEntity.setImmuneToFire(false);
    EXPECT_FALSE(immuneEntity.isImmuneToFire());

    // 可以重新开启
    immuneEntity.setImmuneToFire(true);
    EXPECT_TRUE(immuneEntity.isImmuneToFire());
}

// ========== FireBlock::onEntityCollision 测试 ==========

TEST_F(FireBlockCollisionTest, OnEntityCollision_ImmuneEntity_NoDamage)
{
    FireTestWorld world;
    FireImmuneTestEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());
    entity.setImmuneToFire(true);

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 免疫实体不受伤害
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);

    EXPECT_EQ(entity.hurtCount(), 0);
    EXPECT_EQ(entity.fire(), 0); // 火焰计时器不变
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_NormalEntity_TakesDamage)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());
    entity.setHealth(20.0f); // 设置生命值

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 普通实体受到伤害
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);

    EXPECT_EQ(entity.hurtCount(), 1);
    EXPECT_EQ(entity.lastDamage(), 1.0f); // 普通火焰伤害 1.0
    EXPECT_EQ(entity.lastDamageType(), DamageType::InFire);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_IncrementsFireTimer)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 初始火焰计时器为 0
    EXPECT_EQ(entity.getFireTimer(), 0);

    // commit 6398bbbf3 起对齐 MC Java：碰撞时先 forceFireTicks(timer+1)，
    // 再 igniteForSeconds(8)（>=0 时）。首次碰撞 timer 0→1→被 igniteForSeconds
    // 覆盖为 160。
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    EXPECT_EQ(entity.getFireTimer(), 160);

    // 第二次碰撞：timer 160→161，igniteForSeconds(8) 因 161>160 不覆盖
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    EXPECT_EQ(entity.getFireTimer(), 161);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_ImmunityEndIgnitesEntity)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 设置火焰计时器为 -1（模拟短暂免疫期即将结束）
    // MC 1.16.5: 当 timer 为负时，表示实体处于短暂火焰免疫期
    entity.forceFireTicks(-1);

    // 碰撞时 timer 从 -1 增加到 0，然后触发 setFire(160)
    // MC 1.16.5 逻辑：
    // 1. forceFireTicks(getFireTimer() + 1) → timer 从 -1 变为 0
    // 2. if (getFireTimer() == 0) → true
    // 3. setFire(160) → timer 被设置为 160
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);

    // timer 最终变为 160（8 秒 = 160 ticks）
    EXPECT_EQ(entity.getFireTimer(), 160);
    EXPECT_EQ(entity.fire(), 160);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_NegativeTimerIgnites)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 设置火焰计时器为 -5（深度免疫期）
    entity.forceFireTicks(-5);

    // 连续碰撞直到 timer 变为 0 并触发点燃
    // -5 → -4 → -3 → -2 → -1 → 0（触发 setFire）
    for (int i = 0; i < 5; ++i) {
        VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    }

    // 第 5 次碰撞时 timer 从 -1 变为 0，触发 setFire(160)
    // 所以 timer 最终变为 160
    EXPECT_EQ(entity.getFireTimer(), 160);
    EXPECT_EQ(entity.fire(), 160);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_FirstCollisionIgnites)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 初始 timer 为 0
    EXPECT_EQ(entity.getFireTimer(), 0);

    // commit 6398bbbf3 起对齐 MC Java：首次碰撞即点燃 8 秒（160 ticks）。
    // 旧逻辑 if(getFireTimer()==0) 在递增后不再成立故不点燃；新逻辑条件为
    // getRemainingFireTicks()>=0，递增后 1>=0 成立，触发 igniteForSeconds(8)。
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);

    EXPECT_EQ(entity.getFireTimer(), 160);
    EXPECT_EQ(entity.fire(), 160);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_SoulFire_HigherDamage)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());
    entity.setHealth(20.0f);

    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);
    const BlockState& soulFireState = VanillaBlocks::SOUL_FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 灵魂火造成 2.0 伤害
    // SoulFireBlock 继承自 FireBlock，m_fireDamage = 2
    FireBlock* soulFireBlock = const_cast<FireBlock*>(static_cast<const FireBlock*>(VanillaBlocks::SOUL_FIRE));
    soulFireBlock->onEntityCollision(soulFireState, world, pos, entity);

    EXPECT_EQ(entity.hurtCount(), 1);
    EXPECT_EQ(entity.lastDamage(), 2.0f); // 灵魂火伤害 2.0
    EXPECT_EQ(entity.lastDamageType(), DamageType::InFire);
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_NonLivingEntity_TimerIncreases)
{
    FireTestWorld world;

    // Entity 基类不是 LivingEntity，不会受到伤害
    Entity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // Entity 基类不受伤害（dynamic_cast<LivingEntity*> 返回 nullptr）
    // 但火焰计时器仍按 onEntityCollision 逻辑处理：首次碰撞被 igniteForSeconds(8)
    // 覆盖为 160（commit 6398bbbf3）。
    EXPECT_EQ(entity.getFireTimer(), 0);
    VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    EXPECT_EQ(entity.getFireTimer(), 160);
    // isOnFire() 检查 fire > 0，timer=160 故点燃
    EXPECT_TRUE(entity.isOnFire());
}

TEST_F(FireBlockCollisionTest, OnEntityCollision_MultipleCollisions_EachTick)
{
    FireTestWorld world;
    TestLivingEntity entity(EntityInstanceId(1), &world, mc::test::testEcsRegistry());
    entity.setHealth(100.0f); // 高生命值以承受多次伤害

    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    BlockPos pos(0, 0, 0);

    // 模拟实体在火焰中停留多个 tick
    for (int i = 0; i < 5; ++i) {
        VanillaBlocks::FIRE->onEntityCollision(fireState, world, pos, entity);
    }

    // 应该受到 5 次伤害
    EXPECT_EQ(entity.hurtCount(), 5);
    EXPECT_EQ(entity.lastDamage(), 1.0f);

    // commit 6398bbbf3 起对齐 MC Java：首次碰撞 igniteForSeconds(8) 覆盖为 160，
    // 后续每次 forceFireTicks(timer+1) 递增且 igniteForSeconds(8) 不再覆盖
    // （timer 已 >160）。5 次后 timer = 160 + 4 = 164。
    EXPECT_EQ(entity.getFireTimer(), 164);
}

} // namespace
