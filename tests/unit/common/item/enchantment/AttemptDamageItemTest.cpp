/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

/**
 * @file AttemptDamageItemTest.cpp
 * @brief 测试 ItemStack::attemptDamageItem 的随机源选择逻辑
 *
 * 验证：
 * 1. 传入 LivingEntity 时使用世界随机源（entity->world()->getRandom()）
 * 2. 传入 nullptr 时降级使用线程局部静态随机源
 * 3. 无实体参数的重载正确降级
 * 4. 耐久保护附魔在不同随机源下概率合理
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::item::enchant;

// ============================================================================
// 测试用 LivingEntity，可以设置 world 指针
// ============================================================================

class DamageTestLivingEntity : public LivingEntity {
public:
    explicit DamageTestLivingEntity(
        IWorld* world = nullptr, ecs::EntityRegistry& registry = mc::test::testEcsRegistry())
        : LivingEntity(EntityInstanceId(1), world, registry)
    {
        registerData();
        registerAttributes();
    }
};

// ============================================================================
// 测试用世界，使用固定种子的随机数生成器
// ============================================================================

class DamageTestWorld : public mc::test::BaseTestWorld {
public:
    DamageTestWorld() = default;

    // 暴露随机数生成器用于验证
    math::Random& testRandom() { return m_random; }
};

// ============================================================================
// 测试夹具
// ============================================================================

class AttemptDamageItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }

    DamageTestWorld world;
};

// ============================================================================
// 基础功能测试：无附魔物品的耐久损耗
// ============================================================================

TEST_F(AttemptDamageItemTest, NoEnchantmentDamageWithoutEntity)
{
    // 无附魔物品：耐久保护不生效，直接扣除耐久
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    ItemStack stack(diamondSword, 1);
    i32 maxDamage = stack.getMaxDamage();

    bool broken = stack.attemptDamageItem(100);
    EXPECT_FALSE(broken);
    EXPECT_EQ(stack.getDamage(), 100);
    EXPECT_EQ(stack.getMaxDamage() - stack.getDamage(), maxDamage - 100);
}

TEST_F(AttemptDamageItemTest, NoEnchantmentDamageWithEntity)
{
    // 无附魔物品 + 传入实体：耐久保护不生效，直接扣除耐久
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    DamageTestLivingEntity entity(&world, mc::test::testEcsRegistry());

    ItemStack stack(diamondSword, 1);
    i32 maxDamage = stack.getMaxDamage();

    bool broken = stack.attemptDamageItem(100, &entity);
    EXPECT_FALSE(broken);
    EXPECT_EQ(stack.getDamage(), 100);
    EXPECT_EQ(stack.getMaxDamage() - stack.getDamage(), maxDamage - 100);
}

TEST_F(AttemptDamageItemTest, NoEnchantmentDamageWithNullEntity)
{
    // 无附魔物品 + 传入 nullptr：与无参数版本行为一致
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    ItemStack stack(diamondSword, 1);

    bool broken = stack.attemptDamageItem(100, nullptr);
    EXPECT_FALSE(broken);
    EXPECT_EQ(stack.getDamage(), 100);
}

// ============================================================================
// 物品损坏测试
// ============================================================================

TEST_F(AttemptDamageItemTest, ItemBreaksWithoutEntity)
{
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    ItemStack stack(diamondSword, 1);
    bool broken = stack.attemptDamageItem(2000);
    EXPECT_TRUE(broken);
    EXPECT_TRUE(stack.isEmpty());
}

TEST_F(AttemptDamageItemTest, ItemBreaksWithEntity)
{
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    DamageTestLivingEntity entity(&world, mc::test::testEcsRegistry());

    ItemStack stack(diamondSword, 1);
    bool broken = stack.attemptDamageItem(2000, &entity);
    EXPECT_TRUE(broken);
    EXPECT_TRUE(stack.isEmpty());
}

// ============================================================================
// 关键测试：传入实体时使用世界随机源
// ============================================================================

TEST_F(AttemptDamageItemTest, WithEntityUsesWorldRandom)
{
    // 验证：传入 LivingEntity 时，耐久保护概率计算使用世界随机源。
    // 方法：使用固定种子的世界随机源，多次尝试损坏带有耐久附魔的物品，
    // 统计被抵消的次数。如果有耐久保护（等级 I），约 50% 的伤害应被抵消。
    // 如果使用的是线程局部静态随机源（种子不确定），统计结果会不稳定。
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    DamageTestLivingEntity entity(&world, mc::test::testEcsRegistry());

    // 添加耐久 I 附魔
    int ignoreCount = 0;
    constexpr int TOTAL_TRIALS = 200;
    constexpr int DAMAGE_PER_TRIAL = 1;

    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack(diamondSword, 1);
        stack.addEnchantment("minecraft:unbreaking", 1);

        i32 damageBefore = stack.getDamage();
        stack.attemptDamageItem(DAMAGE_PER_TRIAL, &entity);
        i32 damageAfter = stack.getDamage();

        if (damageAfter == damageBefore) {
            // 耐久保护抵消了这次伤害
            ignoreCount++;
        }
    }

    // 耐久 I 非护甲：50% 概率忽略伤害
    // 200 次试验，期望约 100 次忽略
    // 使用较宽松的范围以确保确定性种子的测试稳定性
    EXPECT_GT(ignoreCount, 60) << "耐久 I 应忽略约50%的伤害，实际忽略 " << ignoreCount << "/" << TOTAL_TRIALS;
    EXPECT_LT(ignoreCount, 140) << "耐久 I 应忽略约50%的伤害，实际忽略 " << ignoreCount << "/" << TOTAL_TRIALS;
}

TEST_F(AttemptDamageItemTest, WithoutEntityFallbackRandom)
{
    // 验证：不传实体时降级使用线程局部静态随机源，耐久保护仍然生效。
    // 即使没有实体，耐久附魔仍然应该有概率抵消伤害。
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    int ignoreCount = 0;
    constexpr int TOTAL_TRIALS = 200;

    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack(diamondSword, 1);
        stack.addEnchantment("minecraft:unbreaking", 1);

        i32 damageBefore = stack.getDamage();
        stack.attemptDamageItem(1);
        i32 damageAfter = stack.getDamage();

        if (damageAfter == damageBefore) {
            ignoreCount++;
        }
    }

    // 耐久 I 非护甲：50% 概率忽略伤害
    // 即使使用线程局部静态随机源，统计上应该接近50%
    EXPECT_GT(ignoreCount, 60) << "降级随机源下耐久 I 应忽略约50%的伤害，实际忽略 " << ignoreCount << "/"
                               << TOTAL_TRIALS;
    EXPECT_LT(ignoreCount, 140) << "降级随机源下耐久 I 应忽略约50%的伤害，实际忽略 " << ignoreCount << "/"
                                << TOTAL_TRIALS;
}

TEST_F(AttemptDamageItemTest, WithNullEntityFallbackRandom)
{
    // 验证：传入 nullptr 时降级使用线程局部静态随机源
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    int ignoreCount = 0;
    constexpr int TOTAL_TRIALS = 200;

    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack(diamondSword, 1);
        stack.addEnchantment("minecraft:unbreaking", 1);

        i32 damageBefore = stack.getDamage();
        stack.attemptDamageItem(1, nullptr);
        i32 damageAfter = stack.getDamage();

        if (damageAfter == damageBefore) {
            ignoreCount++;
        }
    }

    // 耐久 I 非护甲：50% 概率忽略伤害
    EXPECT_GT(ignoreCount, 60);
    EXPECT_LT(ignoreCount, 140);
}

// ============================================================================
// 实体不在世界中时的降级行为
// ============================================================================

TEST_F(AttemptDamageItemTest, EntityWithoutWorldFallbackRandom)
{
    // 验证：实体存在但不在世界中（world() 返回 nullptr）时，降级使用线程局部静态随机源
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    // 创建没有世界的实体
    DamageTestLivingEntity entityNoWorld(nullptr, mc::test::testEcsRegistry());
    EXPECT_EQ(entityNoWorld.world(), nullptr);

    int ignoreCount = 0;
    constexpr int TOTAL_TRIALS = 200;

    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack(diamondSword, 1);
        stack.addEnchantment("minecraft:unbreaking", 1);

        i32 damageBefore = stack.getDamage();
        stack.attemptDamageItem(1, &entityNoWorld);
        i32 damageAfter = stack.getDamage();

        if (damageAfter == damageBefore) {
            ignoreCount++;
        }
    }

    // 耐久 I 非护甲：50% 概率忽略伤害
    EXPECT_GT(ignoreCount, 60);
    EXPECT_LT(ignoreCount, 140);
}

// ============================================================================
// 耐久保护等级测试
// ============================================================================

TEST_F(AttemptDamageItemTest, HigherUnbreakingLevelMoreProtection)
{
    // 验证：更高的耐久等级提供更强的保护
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    DamageTestLivingEntity entity(&world, mc::test::testEcsRegistry());

    int ignoreCountLevel1 = 0;
    int ignoreCountLevel3 = 0;
    constexpr int TOTAL_TRIALS = 500;

    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack1(diamondSword, 1);
        stack1.addEnchantment("minecraft:unbreaking", 1);
        i32 before1 = stack1.getDamage();
        stack1.attemptDamageItem(1, &entity);
        if (stack1.getDamage() == before1) {
            ignoreCountLevel1++;
        }
    }

    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack3(diamondSword, 1);
        stack3.addEnchantment("minecraft:unbreaking", 3);
        i32 before3 = stack3.getDamage();
        stack3.attemptDamageItem(1, &entity);
        if (stack3.getDamage() == before3) {
            ignoreCountLevel3++;
        }
    }

    // 耐久 III（75%忽略）应该比耐久 I（50%忽略）忽略更多伤害
    EXPECT_GT(ignoreCountLevel3, ignoreCountLevel1)
        << "耐久 III 应比耐久 I 忽略更多伤害。Level I: " << ignoreCountLevel1 << ", Level III: " << ignoreCountLevel3;
}

// ============================================================================
// 护甲耐久保护概率测试
// ============================================================================

TEST_F(AttemptDamageItemTest, ArmorUnbreakingReducedEffectiveness)
{
    // 验证：护甲的耐久保护有 60% 概率不生效，因此实际忽略概率更低
    // 需要一个护甲物品
    Item* diamondChestplate = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_chestplate"));
    ASSERT_NE(diamondChestplate, nullptr);
    ASSERT_TRUE(diamondChestplate->isArmor());

    DamageTestLivingEntity entity(&world, mc::test::testEcsRegistry());

    int ignoreCountArmor = 0;
    int ignoreCountNonArmor = 0;
    constexpr int TOTAL_TRIALS = 500;

    // 护甲 + 耐久 I：实际忽略概率 = 0.4 * 0.5 = 20%
    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack(diamondChestplate, 1);
        stack.addEnchantment("minecraft:unbreaking", 1);
        i32 before = stack.getDamage();
        stack.attemptDamageItem(1, &entity);
        if (stack.getDamage() == before) {
            ignoreCountArmor++;
        }
    }

    // 非护甲 + 耐久 I：实际忽略概率 = 50%
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack(diamondSword, 1);
        stack.addEnchantment("minecraft:unbreaking", 1);
        i32 before = stack.getDamage();
        stack.attemptDamageItem(1, &entity);
        if (stack.getDamage() == before) {
            ignoreCountNonArmor++;
        }
    }

    // 非护甲的忽略概率应该高于护甲
    EXPECT_GT(ignoreCountNonArmor, ignoreCountArmor)
        << "非护甲耐久 I 应比护甲耐久 I 忽略更多伤害。护甲: " << ignoreCountArmor
        << ", 非护甲: " << ignoreCountNonArmor;
}

// ============================================================================
// 多点损耗测试：耐久附魔对 amount>1 的损耗应做独立伯努利试验（对齐 vanilla RemoveBinomial）
// ============================================================================

TEST_F(AttemptDamageItemTest, MultiPointDamageAppliesIndependentTrials)
{
    // 验证：amount>1 时，耐久附魔应对【原始 amount】做固定次数独立伯努利试验，累加被忽略的点数，
    // 而非每次忽略后减小循环上界。对齐 vanilla 1.21.11 RemoveBinomial.process（RemoveBinomial.java:21-23
    // 小 amount 路径：for (j < amount) { if (nextFloat < chance) removed++; } return amount - removed）。
    //
    // 此前缺陷：ItemStack::attemptDamageItem 用 `for (i < amount) { if (ignore) --amount; }`，每次忽略后
    // 减小循环上界，致 amount>1 时试验次数减少、保护效果被削弱。以 amount=2 level=1 非盔甲（chance=0.5）为例：
    //   - vanilla（独立试验）：P(剩0)=0.25, P(剩1)=0.5, P(剩2)=0.25，期望剩余 = 1.0
    //   - 旧实现（上界自减）：P(剩0)=0, P(剩1)=0.75, P(剩2)=0.25，期望剩余 = 1.25（保护偏弱）
    // 修复后用 removed 累加器对齐 vanilla，期望剩余回到 1.0。
    //
    // 统计断言：500 次试验总剩余损耗，vanilla 期望 = 500*1.0 = 500，旧实现期望 = 500*1.25 = 625。
    // 区间 [400, 600] 容忍统计波动，同时排除旧实现的 625+（旧实现总剩余约 625，>600 → FAIL 暴露缺陷）。
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    DamageTestLivingEntity entity(&world, mc::test::testEcsRegistry());

    constexpr int TOTAL_TRIALS = 500;
    constexpr int DAMAGE_PER_TRIAL = 2; // amount=2，触发多点损耗路径
    i32 totalRemainingDamage = 0;

    for (int trial = 0; trial < TOTAL_TRIALS; ++trial) {
        ItemStack stack(diamondSword, 1);
        stack.addEnchantment("minecraft:unbreaking", 1); // level=1, chance=0.5

        i32 damageBefore = stack.getDamage();
        stack.attemptDamageItem(DAMAGE_PER_TRIAL, &entity);
        i32 damageAfter = stack.getDamage();
        totalRemainingDamage += (damageAfter - damageBefore);
    }

    // vanilla 期望总剩余 = 500 * 1.0 = 500；旧缺陷实现期望 = 500 * 1.25 = 625。
    // 区间 [400, 600] 排除旧实现（625 > 600），验证修复对齐 vanilla 独立试验语义。
    EXPECT_GE(totalRemainingDamage, 400) << "多点损耗总剩余偏低（过多忽略），期望约 500（vanilla 独立试验），实际 "
                                         << totalRemainingDamage;
    EXPECT_LE(totalRemainingDamage, 600)
        << "多点损耗总剩余偏高（保护偏弱，疑旧实现上界自减缺陷未修复），期望约 500（vanilla），"
        << "旧缺陷实现约 625，实际 " << totalRemainingDamage;
}

// ============================================================================
// 确定性测试：固定种子的世界随机源应产生可重复的结果
// ============================================================================

TEST_F(AttemptDamageItemTest, WorldRandomProducesDeterministicResults)
{
    // 验证：使用固定种子的世界随机源，相同操作应产生相同的损伤序列
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    DamageTestLivingEntity entity(&world, mc::test::testEcsRegistry());

    // 第一轮：记录损伤序列
    std::vector<i32> damages1;
    for (int trial = 0; trial < 20; ++trial) {
        ItemStack stack(diamondSword, 1);
        stack.addEnchantment("minecraft:unbreaking", 3);
        stack.attemptDamageItem(1, &entity);
        damages1.push_back(stack.getDamage());
    }

    // 重置世界随机源为相同种子
    world.testRandom().setSeed(12345);

    // 第二轮：使用重置后的随机源
    std::vector<i32> damages2;
    for (int trial = 0; trial < 20; ++trial) {
        ItemStack stack(diamondSword, 1);
        stack.addEnchantment("minecraft:unbreaking", 3);
        stack.attemptDamageItem(1, &entity);
        damages2.push_back(stack.getDamage());
    }

    // 两轮的损伤序列应完全相同
    ASSERT_EQ(damages1.size(), damages2.size());
    for (size_t i = 0; i < damages1.size(); ++i) {
        EXPECT_EQ(damages1[i], damages2[i]) << "固定种子世界随机源在第 " << i << " 次试验产生了不同结果";
    }
}
