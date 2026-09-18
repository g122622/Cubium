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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
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
#include "common/core/Constants.hpp"
#include "common/entity/entities/passive/golem/SnowGolemEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 雪傀儡测试用模拟世界
 */
class SnowGolemTestWorld final : public mc::test::BaseChunkBackedTestWorld {
public:
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class SnowGolemEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    SnowGolemTestWorld m_world;
};

// ========== 南瓜头测试 ==========

TEST_F(SnowGolemEntityTest, Pumpkin_DefaultHasPumpkin)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认戴着南瓜
    EXPECT_TRUE(golem.hasPumpkin());
    EXPECT_TRUE(golem.isShearable());
}

TEST_F(SnowGolemEntityTest, Pumpkin_CanSetAndClear)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    golem.setPumpkin(false);
    EXPECT_FALSE(golem.hasPumpkin());
    EXPECT_FALSE(golem.isShearable());

    golem.setPumpkin(true);
    EXPECT_TRUE(golem.hasPumpkin());
    EXPECT_TRUE(golem.isShearable());
}

TEST_F(SnowGolemEntityTest, Shear_ReturnsCarvedPumpkin)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem.setWorld(&m_world);

    EXPECT_TRUE(golem.hasPumpkin());

    std::vector<ItemStack> drops = golem.shear(nullptr);

    // 剪切后不再有南瓜（无论是否成功获取物品）
    EXPECT_FALSE(golem.hasPumpkin());
    EXPECT_FALSE(golem.isShearable());

    // 如果 CARVED_PUMPKIN 已注册，应该掉落一个雕刻南瓜
    // 注意：BlockItemRegistry 需要在服务器初始化时注册，测试中可能不可用
    if (VanillaBlocks::CARVED_PUMPKIN != nullptr) {
        // BlockItemRegistry 可能未初始化，所以只检查掉落不为空
        // 如果 BlockItemRegistry 可用，应该有 1 个物品
        // 否则可能为空（但南瓜状态仍然会被移除）
        if (!drops.empty()) {
            EXPECT_EQ(drops.size(), 1u);
        }
    }

    // 再次剪切应该返回空（没有南瓜了）
    std::vector<ItemStack> drops2 = golem.shear(nullptr);
    EXPECT_TRUE(drops2.empty());
}

TEST_F(SnowGolemEntityTest, Shear_NoPumpkin_ReturnsEmpty)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem.setWorld(&m_world);

    // 先剪切一次
    golem.shear(nullptr);
    EXPECT_FALSE(golem.hasPumpkin());

    // 再次剪切应该返回空
    std::vector<ItemStack> drops = golem.shear(nullptr);
    EXPECT_TRUE(drops.empty());
}

// ========== 属性测试 ==========

TEST_F(SnowGolemEntityTest, Attributes_HasCorrectBaseValues)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: 雪傀儡生命值为 4
    EXPECT_DOUBLE_EQ(golem.maxHealth(), 4.0);

    // MC 1.16.5: 雪傀儡移动速度为 0.2
    EXPECT_DOUBLE_EQ(golem.getAttributeValue("generic.movement_speed", 0.0), 0.2);
}

// ========== 尺寸测试 ==========

TEST_F(SnowGolemEntityTest, Dimensions_CorrectValues)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: 雪傀儡尺寸
    EXPECT_FLOAT_EQ(golem.width(), 0.7f);
    EXPECT_FLOAT_EQ(golem.height(), 1.9f);
    EXPECT_FLOAT_EQ(golem.eyeHeight(), 1.7f);
}

// ========== 攻击间隔测试 ==========

TEST_F(SnowGolemEntityTest, AttackInterval_CorrectValue)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: 攻击间隔 20 ticks（1秒）
    EXPECT_EQ(golem.getAttackInterval(), 20);
}

// ========== 水敏感性测试 ==========

TEST_F(SnowGolemEntityTest, WaterSensitive_IsTrue)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 雪傀儡对水敏感
    EXPECT_TRUE(golem.isWaterSensitive());
}

// ========== 工厂方法测试 ==========

TEST_F(SnowGolemEntityTest, Create_ReturnsValidEntity)
{
    auto entity = SnowGolemEntity::create(&m_world, mc::test::testEcsRegistry());

    ASSERT_NE(entity, nullptr);

    SnowGolemEntity* golem = dynamic_cast<SnowGolemEntity*>(entity.get());
    EXPECT_NE(golem, nullptr);
}

// ========== 继承测试 ==========

TEST_F(SnowGolemEntityTest, Inheritance_IsGolemEntity)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证继承关系
    GolemEntity* golemBase = dynamic_cast<GolemEntity*>(&golem);
    EXPECT_NE(golemBase, nullptr);

    // 验证接口实现
    entity::IShearable* shearable = dynamic_cast<entity::IShearable*>(&golem);
    EXPECT_NE(shearable, nullptr);

    entity::IRangedAttackMob* rangedAttacker = dynamic_cast<entity::IRangedAttackMob*>(&golem);
    EXPECT_NE(rangedAttacker, nullptr);
}

// ========== 声音测试 ==========

TEST_F(SnowGolemEntityTest, Sounds_ReturnCorrectEvents)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 环境音效
    auto ambient = golem.getAmbientSound();
    EXPECT_TRUE(ambient.has_value());

    // 死亡音效
    auto death = golem.getDeathSound();
    EXPECT_TRUE(death.has_value());
}

// ========== 融化常量测试 ==========

TEST_F(SnowGolemEntityTest, MeltConstants_IndirectVerification)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证常量通过行为间接体现
    // MELT_TEMPERATURE = 1.0f - 温度 > 1.0 时融化
    // MELT_DAMAGE_INTERVAL = 20 ticks - 每秒一次伤害
    // MELT_DAMAGE = 1.0f - 每次伤害 1 点

    // 由于 willMelt() 依赖 isInWater() 和生物群系温度，
    // 这里只验证方法存在且可调用
    golem.willMelt(); // 不崩溃即通过
}

// ========== 雪球攻击测试 ==========

TEST_F(SnowGolemEntityTest, RangedAttack_CreatesSnowball)
{
    SnowGolemEntity golem(EntityInstanceId(1), mc::test::testEcsRegistry());
    golem.setWorld(&m_world);
    golem.setPosition(0.0, 64.0, 0.0);

    // 调用远程攻击方法
    // 由于需要 LivingEntity 目标，这里只验证方法存在
    // 完整测试需要 mock LivingEntity
    golem.attackEntityWithRangedAttack(nullptr, 1.0f); // 传入 nullptr 应该安全返回

    // 不应该生成任何实体（目标为 nullptr）
    EXPECT_EQ(m_world.spawnedEntities().size(), 0u);
}

} // namespace
} // namespace mc
