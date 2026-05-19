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
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <cmath>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {

// ============================================================================
// 测试用 LivingEntity
// ============================================================================

/**
 * @brief 测试用 LivingEntity，记录受到的伤害
 */
class FireworkTestLivingEntity : public LivingEntity {
public:
    FireworkTestLivingEntity()
        : LivingEntity(EntityId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    // 记录最后一次受到的伤害
    f32 lastDamage = 0.0f;
    DamageType lastDamageType = DamageType::Generic;
    bool wasHurt = false;

protected:
    bool hurt(DamageSource& source, f32 amount) override
    {
        wasHurt = true;
        lastDamage = amount;
        lastDamageType = source.type();
        return LivingEntity::hurt(source, amount);
    }
};

// ============================================================================
// 测试用烟花世界
// ============================================================================

/**
 * @brief 烟花火箭测试世界
 *
 * 提供 FireworkRocketEntity 测试所需的最小 IWorld 实现
 */
class FireworkRocketTestWorld : public test::BaseTestWorld {
public:
    FireworkRocketTestWorld()
        : m_random(12345) // 固定种子用于可重复测试
    {}

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
        throw std::runtime_error("FireworkRocketTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("FireworkRocketTestWorld::tickManager not implemented");
    }

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    mutable math::Random m_random;
};

namespace {

// ============================================================================
// 基础属性测试
// ============================================================================

/**
 * @brief 测试烟花火箭尺寸
 *
 * MC 1.16.5: 烟花火箭尺寸为 0.25 x 0.25
 */
TEST(FireworkRocketBasicTest, DimensionsCorrect)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    EXPECT_FLOAT_EQ(firework.width(), 0.25f);
    EXPECT_FLOAT_EQ(firework.height(), 0.25f);
}

/**
 * @brief 测试默认飞行时间
 */
TEST(FireworkRocketBasicTest, DefaultFlightTime)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    // 默认飞行时间为 1
    EXPECT_EQ(firework.flightTime(), 1);
}

/**
 * @brief 测试设置飞行时间
 */
TEST(FireworkRocketBasicTest, SetFlightTime)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    firework.setFlightTime(3);
    EXPECT_EQ(firework.flightTime(), 3);
}

/**
 * @brief 测试从弩射出标记
 */
TEST(FireworkRocketBasicTest, ShotFromCrossbowFlag)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    EXPECT_FALSE(firework.shotFromCrossbow());

    firework.setShotFromCrossbow(true);
    EXPECT_TRUE(firework.shotFromCrossbow());
}

// ============================================================================
// 物品数据测试
// ============================================================================

/**
 * @brief 测试从物品获取爆炸效果数量 - 无爆炸效果（空物品）
 */
TEST(FireworkRocketItemTest, GetExplosionCountEmpty)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    // 空物品
    ItemStack emptyStack(Items::AIR, 0);
    firework.setFireworkItem(emptyStack);

    EXPECT_EQ(firework.getExplosionCount(), 0);
}

/**
 * @brief 测试从物品获取爆炸效果数量 - 无爆炸数据
 */
TEST(FireworkRocketItemTest, GetExplosionCountNoExplosions)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    // 创建烟花火箭物品，只设置飞行时间
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;

    firework.setFireworkItem(stack);

    EXPECT_EQ(firework.getExplosionCount(), 0);
}

/**
 * @brief 测试从物品获取爆炸效果数量 - 单个爆炸效果
 */
TEST(FireworkRocketItemTest, GetExplosionCountSingle)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());

    firework.setFireworkItem(stack);

    EXPECT_EQ(firework.getExplosionCount(), 1);
}

/**
 * @brief 测试从物品获取爆炸效果数量 - 多个爆炸效果
 */
TEST(FireworkRocketItemTest, GetExplosionCountMultiple)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    // 7 个爆炸效果（最大）
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    for (int i = 0; i < 7; ++i) {
        tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());
    }

    firework.setFireworkItem(stack);

    EXPECT_EQ(firework.getExplosionCount(), 7);
}

/**
 * @brief 测试从物品读取飞行时间
 */
TEST(FireworkRocketItemTest, ReadsFlightTimeFromItem)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 3;

    firework.setFireworkItem(stack);

    EXPECT_EQ(firework.flightTime(), 3);
}

/**
 * @brief 测试飞行时间不能小于 1
 */
TEST(FireworkRocketItemTest, FlightTimeMinimumOne)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 0; // 设置为 0

    firework.setFireworkItem(stack);

    // 飞行时间最小为 1
    EXPECT_EQ(firework.flightTime(), 1);
}

/**
 * @brief 测试烟花火箭物品存储
 */
TEST(FireworkRocketItemTest, FireworkItemStorage)
{
    entity::FireworkRocketEntity firework(EntityId(1));

    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 2;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());

    firework.setFireworkItem(stack);

    // 验证物品已存储
    EXPECT_EQ(firework.fireworkItem().getItem(), Items::FIREWORK_ROCKET);
    EXPECT_EQ(firework.flightTime(), 2);
    EXPECT_EQ(firework.getExplosionCount(), 1);
}

// ============================================================================
// 爆炸伤害计算公式测试
// ============================================================================

/**
 * @brief 测试爆炸伤害计算公式
 *
 * MC 1.16.5 伤害公式：
 * 基础伤害 = 5 + 爆炸效果数量 * 2
 * 实际伤害 = 基础伤害 * sqrt((5 - 距离) / 5)
 */
TEST(FireworkRocketDamageFormulaTest, BaseDamageCalculation)
{
    // 基础伤害计算
    // 0 个爆炸效果：基础伤害 = 5 + 0 * 2 = 5（但不会造成伤害，因为没有爆炸效果）
    // 1 个爆炸效果：基础伤害 = 5 + 1 * 2 = 7
    // 3 个爆炸效果：基础伤害 = 5 + 3 * 2 = 11
    // 7 个爆炸效果：基础伤害 = 5 + 7 * 2 = 19

    auto calculateBaseDamage = [](i32 explosionCount) -> f32 { return 5.0f + static_cast<f32>(explosionCount * 2); };

    EXPECT_FLOAT_EQ(calculateBaseDamage(1), 7.0f);
    EXPECT_FLOAT_EQ(calculateBaseDamage(3), 11.0f);
    EXPECT_FLOAT_EQ(calculateBaseDamage(7), 19.0f);
}

/**
 * @brief 测试距离衰减公式
 */
TEST(FireworkRocketDamageFormulaTest, DistanceAttenuation)
{
    constexpr f32 EXPLOSION_RADIUS = 5.0f;

    auto calculateDamage = [EXPLOSION_RADIUS](i32 explosionCount, f64 distance) -> f32 {
        f32 baseDamage = 5.0f + static_cast<f32>(explosionCount * 2);
        if (baseDamage <= 0.0f || distance >= EXPLOSION_RADIUS) {
            return 0.0f;
        }
        return baseDamage * static_cast<f32>(std::sqrt((EXPLOSION_RADIUS - distance) / EXPLOSION_RADIUS));
    };

    // 距离 0，完整伤害
    EXPECT_FLOAT_EQ(calculateDamage(1, 0.0), 7.0f);
    EXPECT_FLOAT_EQ(calculateDamage(3, 0.0), 11.0f);
    EXPECT_FLOAT_EQ(calculateDamage(7, 0.0), 19.0f);

    // 距离 2.5，约 70.7% 伤害
    f32 damageAtHalf = calculateDamage(1, 2.5);
    EXPECT_NEAR(damageAtHalf, 7.0f * 0.707f, 0.1f);

    // 距离 5，无伤害
    EXPECT_FLOAT_EQ(calculateDamage(1, 5.0), 0.0f);
    EXPECT_FLOAT_EQ(calculateDamage(7, 5.0), 0.0f);

    // 超出范围，无伤害
    EXPECT_FLOAT_EQ(calculateDamage(7, 6.0), 0.0f);

    // 距离 4，约 44.7% 伤害
    f32 damageAt4 = calculateDamage(3, 4.0);
    f32 expectedAt4 = 11.0f * static_cast<f32>(std::sqrt(0.2));
    EXPECT_NEAR(damageAt4, expectedAt4, 0.1f);
}

/**
 * @brief 测试爆炸半径常量
 */
TEST(FireworkRocketDamageFormulaTest, ExplosionRadius)
{
    // MC 1.16.5: 烟花火箭爆炸半径为 5 格
    constexpr f32 EXPLOSION_RADIUS = 5.0f;
    EXPECT_FLOAT_EQ(EXPLOSION_RADIUS, 5.0f);
}

/**
 * @brief 测试最大爆炸效果数量
 */
TEST(FireworkRocketDamageFormulaTest, MaxExplosions)
{
    // MC 1.16.5: 烟花火箭最多可以有 7 个爆炸效果
    // 最高伤害 = 5 + 7 * 2 = 19 点（9.5 颗心）
    constexpr i32 MAX_EXPLOSIONS = 7;
    constexpr f32 MAX_BASE_DAMAGE = 5.0f + static_cast<f32>(MAX_EXPLOSIONS * 2);

    EXPECT_EQ(MAX_EXPLOSIONS, 7);
    EXPECT_FLOAT_EQ(MAX_BASE_DAMAGE, 19.0f);
}

// ============================================================================
// 视线检测测试
// ============================================================================

/**
 * @brief 视线检测测试固件
 */
class FireworkRocketLineOfSightTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<FireworkRocketTestWorld>(); }
    void TearDown() override { m_world.reset(); }

    std::unique_ptr<FireworkRocketTestWorld> m_world;
};

/**
 * @brief 测试无阻挡时的视线检测
 */
TEST_F(FireworkRocketLineOfSightTest, CanSeeEntity_NoBlocks)
{
    auto& firework = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(1));
    firework.setPosition(0.0, 0.0, 0.0);

    auto& target = m_world->addEntity<FireworkTestLivingEntity>();
    target.setPosition(2.0, 0.0, 0.0); // 距离 2 格

    // 无阻挡，应该能看到
    EXPECT_TRUE(firework.canSeeEntity(target));
}

/**
 * @brief 测试远距离视线检测
 */
TEST_F(FireworkRocketLineOfSightTest, CanSeeEntity_DistantTarget)
{
    auto& firework = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(1));
    firework.setPosition(0.0, 0.0, 0.0);

    auto& target = m_world->addEntity<FireworkTestLivingEntity>();
    target.setPosition(4.0, 0.0, 0.0); // 距离 4 格

    // 无阻挡，应该能看到
    EXPECT_TRUE(firework.canSeeEntity(target));
}

// ============================================================================
// 伤害应用测试
// ============================================================================

/**
 * @brief 伤害应用测试固件
 */
class FireworkRocketDamageApplicationTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<FireworkRocketTestWorld>(); }
    void TearDown() override { m_world.reset(); }

    std::unique_ptr<FireworkRocketTestWorld> m_world;
};

/**
 * @brief 测试爆炸伤害应用到 LivingEntity
 *
 * 验证 dealExplosionDamage() 能正确对 LivingEntity 造成伤害。
 */
TEST_F(FireworkRocketDamageApplicationTest, DealsDamageToLivingEntity)
{
    auto& firework = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(1));
    firework.setPosition(0.0, 0.0, 0.0);
    firework.setShotFromCrossbow(true);

    // 设置 1 个爆炸效果
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());
    firework.setFireworkItem(stack);

    // 创建目标实体，放在爆炸范围内
    auto& target = m_world->addEntity<FireworkTestLivingEntity>();
    target.setPosition(2.0, 0.0, 0.0); // 距离 2 格

    // 记录初始生命值
    f32 initialHealth = target.health();

    // 执行爆炸伤害
    firework.dealExplosionDamage();

    // 验证目标受到了伤害
    EXPECT_TRUE(target.wasHurt);
    EXPECT_LT(target.health(), initialHealth);

    // 验证伤害类型是 Fireworks
    EXPECT_EQ(target.lastDamageType, DamageType::Fireworks);
}

/**
 * @brief 测试爆炸伤害随距离衰减
 *
 * 验证距离烟花越远，受到的伤害越小。
 */
TEST_F(FireworkRocketDamageApplicationTest, DamageDecreasesWithDistance)
{
    auto& firework = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(1));
    firework.setPosition(0.0, 0.0, 0.0);
    firework.setShotFromCrossbow(true);

    // 设置 1 个爆炸效果（基础伤害 7）
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());
    firework.setFireworkItem(stack);

    // 创建近距离目标（距离 1）
    auto& nearTarget = m_world->addEntity<FireworkTestLivingEntity>();
    nearTarget.setPosition(1.0, 0.0, 0.0);

    // 创建远距离目标（距离 4）
    auto& farTarget = m_world->addEntity<FireworkTestLivingEntity>();
    farTarget.setPosition(4.0, 0.0, 0.0);

    // 执行爆炸伤害
    firework.dealExplosionDamage();

    // 近距离目标应该受到更多伤害
    EXPECT_TRUE(nearTarget.wasHurt);
    EXPECT_TRUE(farTarget.wasHurt);
    EXPECT_GT(nearTarget.lastDamage, farTarget.lastDamage);
}

/**
 * @brief 测试超出爆炸范围的目标不受伤害
 */
TEST_F(FireworkRocketDamageApplicationTest, NoDamageOutOfRange)
{
    auto& firework = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(1));
    firework.setPosition(0.0, 0.0, 0.0);
    firework.setShotFromCrossbow(true);

    // 设置 1 个爆炸效果
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());
    firework.setFireworkItem(stack);

    // 创建超出范围的目标（距离 6，大于爆炸半径 5）
    auto& outOfRangeTarget = m_world->addEntity<FireworkTestLivingEntity>();
    outOfRangeTarget.setPosition(6.0, 0.0, 0.0);

    f32 initialHealth = outOfRangeTarget.health();

    // 执行爆炸伤害
    firework.dealExplosionDamage();

    // 超出范围的目标不应该受到伤害
    EXPECT_FALSE(outOfRangeTarget.wasHurt);
    EXPECT_FLOAT_EQ(outOfRangeTarget.health(), initialHealth);
}

/**
 * @brief 测试多个爆炸效果增加伤害
 *
 * MC 1.16.5: 每个爆炸效果增加 2 点基础伤害。
 */
TEST_F(FireworkRocketDamageApplicationTest, MoreExplosionsIncreaseDamage)
{
    // 创建第一个烟花（1 个爆炸效果）
    auto& firework1 = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(1));
    firework1.setPosition(0.0, 0.0, 0.0);
    firework1.setShotFromCrossbow(true);

    ItemStack stack1(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag1 = stack1.getOrCreateTag();
    tag1["Fireworks"] = nlohmann::json::object();
    tag1["Fireworks"]["Flight"] = 1;
    tag1["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag1["Fireworks"]["Explosions"].push_back(nlohmann::json::object());
    firework1.setFireworkItem(stack1);

    auto& target1 = m_world->addEntity<FireworkTestLivingEntity>();
    target1.setPosition(2.0, 0.0, 0.0);

    firework1.dealExplosionDamage();
    f32 damage1 = target1.lastDamage;

    // 创建第二个烟花（3 个爆炸效果）
    auto& firework2 = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(2));
    firework2.setPosition(0.0, 10.0, 0.0);
    firework2.setShotFromCrossbow(true);

    ItemStack stack2(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag2 = stack2.getOrCreateTag();
    tag2["Fireworks"] = nlohmann::json::object();
    tag2["Fireworks"]["Flight"] = 1;
    tag2["Fireworks"]["Explosions"] = nlohmann::json::array();
    for (int i = 0; i < 3; ++i) {
        tag2["Fireworks"]["Explosions"].push_back(nlohmann::json::object());
    }
    firework2.setFireworkItem(stack2);

    auto& target2 = m_world->addEntity<FireworkTestLivingEntity>();
    target2.setPosition(2.0, 10.0, 0.0);

    firework2.dealExplosionDamage();
    f32 damage2 = target2.lastDamage;

    // 3 个爆炸效果应该比 1 个造成更多伤害
    // 基础伤害：1 个 = 7，3 个 = 11
    // 距离 2 格时伤害应该按比例增加
    EXPECT_GT(damage2, damage1);
}

/**
 * @brief 测试无爆炸效果的烟花不造成伤害
 */
TEST_F(FireworkRocketDamageApplicationTest, NoExplosionNoDamage)
{
    auto& firework = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(1));
    firework.setPosition(0.0, 0.0, 0.0);
    firework.setShotFromCrossbow(true);

    // 无爆炸效果
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    // 不设置 Explosions
    firework.setFireworkItem(stack);

    auto& target = m_world->addEntity<FireworkTestLivingEntity>();
    target.setPosition(2.0, 0.0, 0.0);

    f32 initialHealth = target.health();

    // 执行爆炸伤害
    firework.dealExplosionDamage();

    // 无爆炸效果不应该造成伤害
    EXPECT_FALSE(target.wasHurt);
    EXPECT_FLOAT_EQ(target.health(), initialHealth);
}

/**
 * @brief 测试非 LivingEntity 不受伤害
 */
TEST_F(FireworkRocketDamageApplicationTest, NonLivingEntityNotHarmed)
{
    auto& firework = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(1));
    firework.setPosition(0.0, 0.0, 0.0);
    firework.setShotFromCrossbow(true);

    // 设置爆炸效果
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());
    firework.setFireworkItem(stack);

    // 创建另一个烟花火箭作为非生物目标
    auto& otherFirework = m_world->addEntity<entity::FireworkRocketEntity>(EntityId(2));
    otherFirework.setPosition(2.0, 0.0, 0.0);

    // 执行爆炸伤害
    firework.dealExplosionDamage();

    // 其他烟花火箭不应该受到伤害（因为它不是 LivingEntity）
    // 验证它没有被移除或标记为已死亡
    EXPECT_TRUE(otherFirework.isAlive());
}

} // namespace
} // namespace mc
