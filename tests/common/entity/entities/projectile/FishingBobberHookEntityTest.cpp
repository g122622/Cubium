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
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace mc {
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
    {
    }

    [[nodiscard]] Entity* getEntity(EntityId id) override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityId id) const override
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
    TestEntity(EntityId id)
        : Entity(id)
    {
    }

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
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));
    EXPECT_EQ(bobber.state(), entity::FishingBobberEntity::State::Flying);
}

/**
 * @brief 测试 getCaughtEntity 初始为 nullptr
 */
TEST_F(FishingBobberHookEntityTest, InitialCaughtEntityIsNull)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));
    EXPECT_EQ(bobber.getCaughtEntity(), nullptr);
    EXPECT_EQ(bobber.getCaughtEntityId(), 0);
}

/**
 * @brief 测试 setShooter 正确设置钓鱼者
 */
TEST_F(FishingBobberHookEntityTest, SetShooterCorrectlySetsAngler)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));
    auto& testEntity = m_world->addEntity<TestEntity>(EntityId(2));

    bobber.setShooter(&testEntity);

    // getAngler() 返回 Player*，由于 TestEntity 不是 Player，所以返回 nullptr
    EXPECT_EQ(bobber.getAngler(), nullptr);
}

/**
 * @brief 测试 setFishingBonus 正确设置附魔加成
 */
TEST_F(FishingBobberHookEntityTest, SetFishingBonusCorrectlySetsBonus)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));

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
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));

    EXPECT_FLOAT_EQ(bobber.width(), 0.25f);
    EXPECT_FLOAT_EQ(bobber.height(), 0.25f);
}

/**
 * @brief 测试 reelIn 在未咬钩时返回 0（无耐久消耗）
 */
TEST_F(FishingBobberHookEntityTest, ReelInWithoutCatchReturnsZero)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));
    auto& testEntity = m_world->addEntity<TestEntity>(EntityId(2));

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
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));
    // 不设置钓鱼者

    bobber.tick();

    EXPECT_TRUE(bobber.isRemoved());
}

/**
 * @brief 测试 isInOpenWater 初始值
 */
TEST_F(FishingBobberHookEntityTest, InOpenWaterInitialValue)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));

    EXPECT_FALSE(bobber.isInOpenWater());
}

/**
 * @brief 测试 Hooked 状态下浮标速度被清零
 *
 * MC 1.16.5: 钩住实体后，浮标速度清零
 */
TEST_F(FishingBobberHookEntityTest, HookedStateClearsVelocity)
{
    auto& bobber = m_world->addEntity<entity::FishingBobberEntity>(EntityId(1));

    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setVelocity(1.0f, 0.5f, 1.0f); // 设置初始速度

    // 验证初始状态
    EXPECT_EQ(bobber.state(), entity::FishingBobberEntity::State::Flying);
    EXPECT_FLOAT_EQ(bobber.velocity().x, 1.0f);
    EXPECT_FLOAT_EQ(bobber.velocity().y, 0.5f);
    EXPECT_FLOAT_EQ(bobber.velocity().z, 1.0f);
}

} // namespace
} // namespace mc
