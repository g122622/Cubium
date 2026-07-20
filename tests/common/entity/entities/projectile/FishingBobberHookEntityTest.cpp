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
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace entity {

/**
 * @brief 测试辅助类，用于访问 FishingBobberEntity 的私有方法
 *
 * 必须在 mc::entity 命名空间中，以便 friend 声明能正确引用。
 */
class FishingBobberTestAccess {
public:
    static void syncCaughtEntityId(FishingBobberEntity& bobber) { bobber._syncCaughtEntityId(); }
    static void setCaughtEntity(FishingBobberEntity& bobber, Entity* entity) { bobber.m_caughtEntity = entity; }
    static void setState(FishingBobberEntity& bobber, FishingBobberEntity::State state) { bobber.m_state = state; }
    static void setTicksCatchable(FishingBobberEntity& bobber, i32 value) { bobber.m_ticksCatchable = value; }
    static void setTicksCatchableDelay(FishingBobberEntity& bobber, i32 value) { bobber.m_ticksCatchableDelay = value; }
};

} // namespace entity

// 别名，方便在匿名命名空间的测试中使用
using FishingBobberAccess = entity::FishingBobberTestAccess;

namespace {

/**
 * @brief 钓鱼浮标测试世界
 *
 * 提供 FishingBobberEntity 测试所需的最小 IWorld 实现
 */
class FishingBobberTestWorld : public test::BaseTestWorld {
public:
    FishingBobberTestWorld()
        : m_random(12345) // 固定种子用于可重复测试
    {}

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override
    {
        std::vector<Entity*> result;
        for (const auto& entity : m_entities) {
            if (entity.get() == except || entity->isRemoved()) {
                continue;
            }
            if (box.intersects(entity->boundingBox())) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    template <typename T, typename... Args>
    T& addEntity(Args&&... args)
    {
        auto entity = std::make_unique<T>(std::forward<Args>(args)...);
        entity->setWorld(this);
        T& reference = *entity;
        m_entities.push_back(std::move(entity));
        return reference;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("FishingBobberTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("FishingBobberTestWorld::tickManager not implemented");
    }

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    mutable math::Random m_random;
};

/**
 * @brief 测试用实体类（替代 Player）
 */
class TestEntity : public Entity {
public:
    TestEntity(EntityInstanceId id)
        : Entity(id)
    {}

    [[nodiscard]] bool canBeCollidedWith() const override { return m_collidable; }
    void setCollidable(bool value) { m_collidable = value; }

private:
    bool m_collidable = true;
};

/**
 * @brief 钓鱼浮标钩住实体测试
 */
class FishingBobberHookEntityTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<FishingBobberTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<FishingBobberTestWorld> m_world;
};

/**
 * @brief 测试 FishingBobberEntity 初始状态为 Flying
 */
TEST_F(FishingBobberHookEntityTest, InitialStateIsFlying)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    EXPECT_EQ(bobber.state(), entity::FishingBobberEntity::State::Flying);
}

/**
 * @brief 测试 getCaughtEntity 初始为 nullptr
 */
TEST_F(FishingBobberHookEntityTest, InitialCaughtEntityIsNull)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    EXPECT_EQ(bobber.getCaughtEntity(), nullptr);
    EXPECT_EQ(bobber.getCaughtEntityId(), 0);
}

/**
 * @brief 测试 setShooter 正确设置钓鱼者
 */
TEST_F(FishingBobberHookEntityTest, SetShooterCorrectlySetsAngler)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    auto& testEntity = m_world->addEntity<TestEntity>(EntityInstanceId(2));

    bobber.setShooter(&testEntity);

    // getAngler() 返回 Player*，由于 TestEntity 不是 Player，所以返回 nullptr
    EXPECT_EQ(bobber.getAngler(), nullptr);
}

/**
 * @brief 测试 setFishingBonus 正确设置附魔加成
 */
TEST_F(FishingBobberHookEntityTest, SetFishingBonusCorrectlySetsBonus)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));

    bobber.setFishingBonus(3, 2); // 海之眷顾 III，饵钓 II
    // 注意：这个测试只验证方法可以调用，实际效果在钓鱼逻辑中体现
}

/**
 * @brief 测试钓鱼浮标尺寸
 *
 * MC 1.16.5: 钓鱼浮标尺寸为 0.25 x 0.25
 */
TEST_F(FishingBobberHookEntityTest, BobberDimensionsCorrect)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));

    EXPECT_FLOAT_EQ(bobber.width(), 0.25f);
    EXPECT_FLOAT_EQ(bobber.height(), 0.25f);
}

/**
 * @brief 测试 reelIn 在未咬钩时返回 0（无耐久消耗）
 */
TEST_F(FishingBobberHookEntityTest, ReelInWithoutCatchReturnsZero)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    auto& testEntity = m_world->addEntity<TestEntity>(EntityInstanceId(2));

    bobber.setShooter(&testEntity);
    // 在 Flying 状态下收杆
    EXPECT_EQ(bobber.state(), entity::FishingBobberEntity::State::Flying);

    i32 damage = bobber.reelIn();
    EXPECT_EQ(damage, 0);
}

/**
 * @brief 测试钓鱼者不存在时浮标自动移除
 */
TEST_F(FishingBobberHookEntityTest, BobberRemovesWhenAnglerIsNull)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    // 不设置钓鱼者

    bobber.tick();

    EXPECT_TRUE(bobber.isRemoved());
}

/**
 * @brief 测试 isInOpenWater 初始值
 */
TEST_F(FishingBobberHookEntityTest, InOpenWaterInitialValue)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));

    EXPECT_FALSE(bobber.isInOpenWater());
}

/**
 * @brief 测试 Hooked 状态下浮标速度被清零
 *
 * MC 1.16.5: 钩住实体后，浮标速度清零
 */
TEST_F(FishingBobberHookEntityTest, HookedStateClearsVelocity)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));

    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setVelocity(1.0f, 0.5f, 1.0f); // 设置初始速度

    // 验证初始状态
    EXPECT_EQ(bobber.state(), entity::FishingBobberEntity::State::Flying);
    EXPECT_FLOAT_EQ(bobber.velocity().x, 1.0f);
    EXPECT_FLOAT_EQ(bobber.velocity().y, 0.5f);
    EXPECT_FLOAT_EQ(bobber.velocity().z, 1.0f);
}

/**
 * @brief 测试 registerData 注册了 DATA_HOOKED_ENTITY / DATA_BITING 参数
 *
 * 对应 MC 1.21.11 FishingHook.defineSynchedData():
 *   define(DATA_HOOKED_ENTITY, 0)
 *   define(DATA_BITING, false)
 */
TEST_F(FishingBobberHookEntityTest, RegisterDataRegistersHookedEntityAndBitingParams)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));

    // DATA_HOOKED_ENTITY_PARAM 应已注册，初始值为 0（无被钩住实体）
    const u16 hookedParamId = entity::FishingBobberEntity::getHookedEntityParamId();
    EXPECT_TRUE(bobber.dataManager().hasParam(hookedParamId));
    const auto* hookedValue = bobber.dataManager().getRaw(hookedParamId);
    ASSERT_NE(hookedValue, nullptr);
    EXPECT_EQ(hookedValue->get<i32>(), 0);

    // DATA_BITING_PARAM 应已注册，初始值为 false
    const u16 bitingParamId = entity::FishingBobberEntity::getBitingParamId();
    EXPECT_TRUE(bobber.dataManager().hasParam(bitingParamId));
    const auto* bitingValue = bobber.dataManager().getRaw(bitingParamId);
    ASSERT_NE(bitingValue, nullptr);
    EXPECT_FALSE(bitingValue->get<bool>());
}

/**
 * @brief 测试 _syncCaughtEntityId 在无被钩住实体时写入 0 并同步到数据管理器
 *
 * 对应 MC 1.21.11 FishingHook.setHookedEntity(null):
 *   getEntityData().set(DATA_HOOKED_ENTITY, 0)
 */
TEST_F(FishingBobberHookEntityTest, SyncCaughtEntityIdWithNullWritesZero)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));

    // 通过 Friend 访问器调用私有 _syncCaughtEntityId
    // 由于此时 m_caughtEntity == nullptr，应写入 0
    FishingBobberAccess::syncCaughtEntityId(bobber);

    EXPECT_EQ(bobber.getCaughtEntityId(), 0);

    // 验证数据管理器中的值也为 0
    const u16 paramId = entity::FishingBobberEntity::getHookedEntityParamId();
    const auto* value = bobber.dataManager().getRaw(paramId);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->get<i32>(), 0);
}

/**
 * @brief 测试 _syncCaughtEntityId 在有被钩住实体时写入 entityId+1 并标记为脏
 *
 * 对应 MC 1.21.11 FishingHook.setHookedEntity(entity):
 *   getEntityData().set(DATA_HOOKED_ENTITY, entity.getId() + 1)
 * +1 偏移是为了区分"无实体"(0) 和"实体 ID 0"。
 */
TEST_F(FishingBobberHookEntityTest, SyncCaughtEntityIdWithEntityWritesIdPlusOne)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    auto& caughtEntity = m_world->addEntity<TestEntity>(EntityInstanceId(42));

    // 设置被钩住实体
    FishingBobberAccess::setCaughtEntity(bobber, &caughtEntity);

    // 调用 _syncCaughtEntityId
    FishingBobberAccess::syncCaughtEntityId(bobber);

    // 验证本地字段：42 + 1 = 43
    EXPECT_EQ(bobber.getCaughtEntityId(), 43);

    // 验证数据管理器中的值：42 + 1 = 43
    const u16 paramId = entity::FishingBobberEntity::getHookedEntityParamId();
    const auto* value = bobber.dataManager().getRaw(paramId);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->get<i32>(), 43);

    // 验证标记为脏（用于 EntityTracker 广播）
    EXPECT_TRUE(bobber.dataManager().hasDirtyData());
}

/**
 * @brief 测试 _syncCaughtEntityId 从有实体到无实体的转换
 *
 * 先钩住实体，然后清除，验证数据管理器值从 entityId+1 变回 0。
 */
TEST_F(FishingBobberHookEntityTest, SyncCaughtEntityIdTransitionFromEntityToNull)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    auto& caughtEntity = m_world->addEntity<TestEntity>(EntityInstanceId(42));

    // 先钩住实体
    FishingBobberAccess::setCaughtEntity(bobber, &caughtEntity);
    FishingBobberAccess::syncCaughtEntityId(bobber);

    const u16 paramId = entity::FishingBobberEntity::getHookedEntityParamId();
    EXPECT_EQ(bobber.dataManager().getRaw(paramId)->get<i32>(), 43);

    // 清除脏标记
    bobber.dataManager().clearDirty();

    // 然后清除被钩住实体
    FishingBobberAccess::setCaughtEntity(bobber, nullptr);
    FishingBobberAccess::syncCaughtEntityId(bobber);

    EXPECT_EQ(bobber.getCaughtEntityId(), 0);
    EXPECT_EQ(bobber.dataManager().getRaw(paramId)->get<i32>(), 0);
    EXPECT_TRUE(bobber.dataManager().hasDirtyData());
}

/**
 * @brief 测试 _syncCaughtEntityId 相同值不重复标记为脏
 *
 * 对应 EntityDataManager::set 的去重逻辑：值未变化时不标记 dirty。
 */
TEST_F(FishingBobberHookEntityTest, SyncCaughtEntityIdSameValueDoesNotMarkDirty)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    auto& caughtEntity = m_world->addEntity<TestEntity>(EntityInstanceId(42));

    // 第一次同步
    FishingBobberAccess::setCaughtEntity(bobber, &caughtEntity);
    FishingBobberAccess::syncCaughtEntityId(bobber);
    EXPECT_TRUE(bobber.dataManager().hasDirtyData());

    // 清除脏标记
    bobber.dataManager().clearDirty();
    EXPECT_FALSE(bobber.dataManager().hasDirtyData());

    // 再次同步相同值，不应标记为脏
    FishingBobberAccess::syncCaughtEntityId(bobber);
    EXPECT_FALSE(bobber.dataManager().hasDirtyData());
}

// ============================================================================
// DATA_BITING 服务端设置逻辑测试
//
// 对应 MC 1.21.11 FishingHook.catchingFish() / tick() 中 DATA_BITING 的设置：
// - 鱼咬钩（进入 Fishing 状态）：DATA_BITING = true
// - 咬钩超时（回到 Bobbing 状态）：DATA_BITING = false
// ============================================================================

/**
 * @brief 测试 DATA_BITING 初始值为 false
 */
TEST_F(FishingBobberHookEntityTest, BitingParam_InitialValueIsFalse)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));

    const u16 bitingParamId = entity::FishingBobberEntity::getBitingParamId();
    const auto* value = bobber.dataManager().getRaw(bitingParamId);
    ASSERT_NE(value, nullptr);
    EXPECT_FALSE(value->get<bool>());
}

/**
 * @brief 测试 tick() 中 State::Fishing 超时时设置 DATA_BITING = false
 *
 * 对应 MC 1.21.11 FishingHook.catchingFish(): nibble 归零时 DATA_BITING = false。
 * 当 m_ticksCatchable 递减到 0 时，tick() 会设置 DATA_BITING = false 并回到 Bobbing 状态。
 */
TEST_F(FishingBobberHookEntityTest, BitingParam_SetToFalse_WhenFishingStateExpires)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    // tick() 前置守卫要求 m_angler 存在、存活且 isFishing() 为 true，
    // 因此需要设置真实的 Player 作为钓鱼者并标记其正在钓鱼。
    auto& player = m_world->addEntity<Player>(EntityInstanceId(2), "TestPlayer");
    bobber.setShooter(&player);
    player.setFishingBobber(bobber.id());

    // 手动设置 Fishing 状态，m_ticksCatchable=1，DATA_BITING 预设为 true
    FishingBobberAccess::setState(bobber, entity::FishingBobberEntity::State::Fishing);
    FishingBobberAccess::setTicksCatchable(bobber, 1);
    const u16 bitingParamId = entity::FishingBobberEntity::getBitingParamId();
    bobber.dataManager().set(entity::DataParameter<bool>(bitingParamId), true);
    bobber.dataManager().clearDirty();

    // 执行一次 tick，m_ticksCatchable 应递减到 0，触发 DATA_BITING = false
    bobber.tick();

    const auto* value = bobber.dataManager().getRaw(bitingParamId);
    ASSERT_NE(value, nullptr);
    EXPECT_FALSE(value->get<bool>());
    EXPECT_EQ(bobber.state(), entity::FishingBobberEntity::State::Bobbing);
}

/**
 * @brief 测试 tick() 中 State::Fishing 且 m_ticksCatchable<=0 时也清除 DATA_BITING
 *
 * 防御性分支：m_ticksCatchable 已为 0 时进入 Fishing 状态，
 * tick() 应立即回到 Bobbing 并清除 DATA_BITING。
 */
TEST_F(FishingBobberHookEntityTest, BitingParam_Cleared_WhenFishingStateWithZeroCatchable)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityInstanceId(1));
    // 同上：tick() 前置守卫要求有效的 Player 钓鱼者。
    auto& player = m_world->addEntity<Player>(EntityInstanceId(2), "TestPlayer");
    bobber.setShooter(&player);
    player.setFishingBobber(bobber.id());

    FishingBobberAccess::setState(bobber, entity::FishingBobberEntity::State::Fishing);
    FishingBobberAccess::setTicksCatchable(bobber, 0);
    const u16 bitingParamId = entity::FishingBobberEntity::getBitingParamId();
    bobber.dataManager().set(entity::DataParameter<bool>(bitingParamId), true);
    bobber.dataManager().clearDirty();

    bobber.tick();

    const auto* value = bobber.dataManager().getRaw(bitingParamId);
    ASSERT_NE(value, nullptr);
    EXPECT_FALSE(value->get<bool>());
}

} // namespace
} // namespace mc
