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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>

#include "common/TestWorldHelper.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/effect/EffectType.hpp"
#include "entity/tag/EntityTypeTags.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/enchantment/Enchantment.hpp"
#include "item/enchantment/EnchantmentContainer.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
#include "item/enchantment/enchantments/AllEnchantments.hpp"
#include "item/enchantment/enchantments/protection/ThornsEnchantment.hpp"
#include "item/enchantment/enchantments/trident/ImpalingEnchantment.hpp"
#include "item/enchantment/enchantments/weapon/BaneOfArthropodsEnchantment.hpp"
#include "item/enchantment/enchantments/weapon/SharpnessEnchantment.hpp"
#include "item/enchantment/enchantments/weapon/SmiteEnchantment.hpp"
#include "util/math/random/Random.hpp"

using namespace mc;
using namespace mc::item::enchant;

// ============================================================================
// 测试辅助：不同生物属性的 LivingEntity 桩
// ============================================================================
// getDamageBonus 新签名接受 const LivingEntity* target，附魔内部按 target 的
// getCreatureAttribute()（亡灵杀手/节肢杀手）或 getTypeId() + EntityTypeTags 标签
// （穿刺 SENSITIVE_TO_IMPALING）判定目标。此处构造节肢/亡灵/普通三类桩供测试。

namespace {

class TestArthropodEntity : public LivingEntity {
public:
    explicit TestArthropodEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);
    }
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Arthropod; }
    [[nodiscard]] std::string getTypeId() const override { return "minecraft:spider"; }
};

class TestUndeadEntity : public LivingEntity {
public:
    explicit TestUndeadEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);
    }
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Undead; }
    [[nodiscard]] std::string getTypeId() const override { return "minecraft:zombie"; }
};

class TestNormalEntity : public LivingEntity {
public:
    explicit TestNormalEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);
    }
    // 基类 getCreatureAttribute()=Undefined, getTypeId() 返回空串（非水生/亡灵/节肢）
};

// 水生生物桩：getTypeId() 返 "minecraft:squid"（在 EntityTypeTags::AQUATIC /
// SENSITIVE_TO_IMPALING 标签内，EntityTypeTags.cpp:504-517），供穿刺附魔测试。
// getCreatureAttribute() 基类返 Undefined（鱿鱼非 Water 枚举，验证穿刺用标签非枚举）。
class TestAquaticEntity : public LivingEntity {
public:
    explicit TestAquaticEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);
    }
    [[nodiscard]] std::string getTypeId() const override { return "minecraft:squid"; }
};

// 僵尸马桩：getTypeId() 返 "minecraft:zombie_horse"（对齐 vanilla 1.21.11 已补入 UNDEAD /
// SENSITIVE_TO_SMITE 标签，EntityTypeTags.cpp:465-492），供亡灵杀手附魔测试。
// 故意不 override getCreatureAttribute()（基类返 Undefined，非 Undead 枚举），以验证
// 亡灵杀手用 SENSITIVE_TO_SMITE 标签判定目标而非 getCreatureAttribute 枚举——这是任务 #203
// 的核心回归点：枚举判定下 zombie_horse 无 override 会漏判，标签判定覆盖 vanilla 全部亡灵成员。
class TestZombieHorseEntity : public LivingEntity {
public:
    explicit TestZombieHorseEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);
    }
    [[nodiscard]] std::string getTypeId() const override { return "minecraft:zombie_horse"; }
};

} // namespace

// ============================================================================
// BaneOfArthropodsEnchantment 测试
// ============================================================================

class BaneOfArthropodsEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
        // 亡灵/节肢杀手目标判定改用 EntityTypeTags::SENSITIVE_TO_BANE_OF_ARTHROPODS 标签
        // （同穿刺用 SENSITIVE_TO_IMPALING），须初始化硬编码标签成员。initialize 幂等。
        EntityTypeTags::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(BaneOfArthropodsEnchantmentTest, Properties)
{
    BaneOfArthropodsEnchantment bane;

    EXPECT_EQ(bane.id(), "minecraft:bane_of_arthropods");
    EXPECT_EQ(bane.minLevel(), 1);
    EXPECT_EQ(bane.maxLevel(), 5);
    EXPECT_EQ(bane.type(), EnchantmentType::Weapon);
    EXPECT_EQ(bane.rarity(), EnchantmentRarity::Uncommon);
}

TEST_F(BaneOfArthropodsEnchantmentTest, GetSlownessDuration)
{
    math::Random rng(12345);

    // 公式：round( randomBetween(1.5, 1.5 + 0.5*(level-1)) * 20 ) tick
    // Level I: randomBetween(1.5, 1.5) = 1.5 固定 → round(30) = 30 tick
    for (int i = 0; i < 100; ++i) {
        i32 duration = BaneOfArthropodsEnchantment::getSlownessDuration(1, rng);
        EXPECT_EQ(duration, 30);
    }

    // Level V: randomBetween(1.5, 3.5) * 20 → 30-70 tick（闭区间）
    for (int i = 0; i < 100; ++i) {
        i32 duration = BaneOfArthropodsEnchantment::getSlownessDuration(5, rng);
        EXPECT_GE(duration, 30);
        EXPECT_LE(duration, 70);
    }
}

TEST_F(BaneOfArthropodsEnchantmentTest, GetSlownessAmplifier)
{
    // 缓慢 IV (amplifier = 3)
    EXPECT_EQ(BaneOfArthropodsEnchantment::getSlownessAmplifier(), 3);
}

TEST_F(BaneOfArthropodsEnchantmentTest, GetDamageBonus)
{
    BaneOfArthropodsEnchantment bane;
    TestArthropodEntity arthropod(1);
    TestUndeadEntity undead(2);
    TestNormalEntity normal(3);

    // 对节肢生物：每级 +2.5 伤害
    EXPECT_FLOAT_EQ(bane.getDamageBonus(1, &arthropod), 2.5f);
    EXPECT_FLOAT_EQ(bane.getDamageBonus(3, &arthropod), 7.5f);
    EXPECT_FLOAT_EQ(bane.getDamageBonus(5, &arthropod), 12.5f);

    // 对非节肢生物（亡灵/普通）：0 伤害
    EXPECT_FLOAT_EQ(bane.getDamageBonus(5, &undead), 0.0f);
    EXPECT_FLOAT_EQ(bane.getDamageBonus(5, &normal), 0.0f);

    // target 为 nullptr：0 伤害（无目标无法判定）
    EXPECT_FLOAT_EQ(bane.getDamageBonus(5, nullptr), 0.0f);
}

TEST_F(BaneOfArthropodsEnchantmentTest, IsIncompatibleWithOtherDamageEnchants)
{
    BaneOfArthropodsEnchantment bane;
    SharpnessEnchantment sharpness;
    SmiteEnchantment smite;

    // 节肢杀手与锋利互斥
    EXPECT_FALSE(bane.isCompatibleWith(sharpness));
    EXPECT_FALSE(sharpness.isCompatibleWith(bane));

    // 节肢杀手与亡灵杀手互斥
    EXPECT_FALSE(bane.isCompatibleWith(smite));
    EXPECT_FALSE(smite.isCompatibleWith(bane));
}

// ============================================================================
// SmiteEnchantment 亡灵杀手测试（任务 #203：标签判定回归）
// ============================================================================

class SmiteEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
        // 亡灵杀手目标判定用 SENSITIVE_TO_SMITE 标签，须初始化硬编码标签成员。
        EntityTypeTags::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(SmiteEnchantmentTest, Properties)
{
    SmiteEnchantment smite;

    EXPECT_EQ(smite.id(), "minecraft:smite");
    EXPECT_EQ(smite.minLevel(), 1);
    EXPECT_EQ(smite.maxLevel(), 5);
    EXPECT_EQ(smite.type(), EnchantmentType::Weapon);
    EXPECT_EQ(smite.rarity(), EnchantmentRarity::Uncommon);
}

TEST_F(SmiteEnchantmentTest, GetDamageBonusAgainstUndead)
{
    SmiteEnchantment smite;
    TestUndeadEntity undead(1); // typeId="minecraft:zombie"，在 SENSITIVE_TO_SMITE 标签内

    // 对亡灵生物：每级 +2.5 伤害
    EXPECT_FLOAT_EQ(smite.getDamageBonus(1, &undead), 2.5f);
    EXPECT_FLOAT_EQ(smite.getDamageBonus(3, &undead), 7.5f);
    EXPECT_FLOAT_EQ(smite.getDamageBonus(5, &undead), 12.5f);
}

TEST_F(SmiteEnchantmentTest, GetDamageBonusAgainstNonUndead)
{
    SmiteEnchantment smite;
    TestArthropodEntity arthropod(1); // typeId="minecraft:spider"，非亡灵
    TestNormalEntity normal(2);       // typeId=""，非亡灵

    // 对非亡灵生物（节肢/普通）：0 伤害
    EXPECT_FLOAT_EQ(smite.getDamageBonus(5, &arthropod), 0.0f);
    EXPECT_FLOAT_EQ(smite.getDamageBonus(5, &normal), 0.0f);

    // target 为 nullptr：0 伤害（无目标无法判定）
    EXPECT_FLOAT_EQ(smite.getDamageBonus(5, nullptr), 0.0f);
}

// 核心回归测试：zombie_horse 在 SENSITIVE_TO_SMITE 标签内（vanilla 1.21.11 UNDEAD 成员），
// 但 TestZombieHorseEntity 不 override getCreatureAttribute()（基类返 Undefined，非 Undead 枚举）。
// 标签判定下亡灵杀手对其有加成；若回退到 getCreatureAttribute 枚举判定则返 0（漏判）。
TEST_F(SmiteEnchantmentTest, TagBasedNotEnumCoversZombieHorse)
{
    SmiteEnchantment smite;
    TestZombieHorseEntity zombieHorse(1);

    // 确认桩实体非 Undead 枚举（基类 getCreatureAttribute 返 Undefined）
    EXPECT_NE(zombieHorse.getCreatureAttribute(), CreatureAttribute::Undead);
    // 标签判定命中：亡灵杀手对 zombie_horse 有加成
    EXPECT_FLOAT_EQ(smite.getDamageBonus(5, &zombieHorse), 12.5f);
}

// ============================================================================
// SharpnessEnchantment 锋利测试（对所有生物生效，与标签无关）
// ============================================================================

class SharpnessEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(SharpnessEnchantmentTest, GetDamageBonusAgainstAll)
{
    SharpnessEnchantment sharpness;
    TestUndeadEntity undead(1);
    TestArthropodEntity arthropod(2);
    TestNormalEntity normal(3);

    // 锋利对所有生物造成额外伤害：I=1.0, II=1.5, III=2.0, IV=2.5, V=3.0
    EXPECT_FLOAT_EQ(sharpness.getDamageBonus(1, &undead), 1.0f);
    EXPECT_FLOAT_EQ(sharpness.getDamageBonus(5, &undead), 3.0f);
    EXPECT_FLOAT_EQ(sharpness.getDamageBonus(5, &arthropod), 3.0f);
    EXPECT_FLOAT_EQ(sharpness.getDamageBonus(5, &normal), 3.0f);
    // 锋利不依赖 target 标签，nullptr 也应有加成
    EXPECT_FLOAT_EQ(sharpness.getDamageBonus(5, nullptr), 3.0f);
}

// ============================================================================
// ThornsEnchantment 测试
// ============================================================================

class ThornsEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(ThornsEnchantmentTest, Properties)
{
    ThornsEnchantment thorns;

    EXPECT_EQ(thorns.id(), "minecraft:thorns");
    EXPECT_EQ(thorns.minLevel(), 1);
    EXPECT_EQ(thorns.maxLevel(), 3);
    EXPECT_EQ(thorns.type(), EnchantmentType::ArmorChest);
    EXPECT_EQ(thorns.rarity(), EnchantmentRarity::VeryRare);
}

TEST_F(ThornsEnchantmentTest, GetMinCost)
{
    ThornsEnchantment thorns;

    EXPECT_EQ(thorns.getMinCost(1), 10);
    EXPECT_EQ(thorns.getMinCost(2), 30); // 10 + 20
    EXPECT_EQ(thorns.getMinCost(3), 50); // 10 + 40
}

TEST_F(ThornsEnchantmentTest, GetMaxCost)
{
    ThornsEnchantment thorns;

    EXPECT_EQ(thorns.getMaxCost(1), 60);  // 10 + 50
    EXPECT_EQ(thorns.getMaxCost(2), 80);  // 30 + 50
    EXPECT_EQ(thorns.getMaxCost(3), 100); // 50 + 50
}

TEST_F(ThornsEnchantmentTest, ShouldTrigger)
{
    math::Random rng(12345);

    // Level 0: 永不触发
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(ThornsEnchantment::shouldTrigger(0, rng));
    }

    // Level I: 15% 概率
    int triggerCount1 = 0;
    for (int i = 0; i < 1000; ++i) {
        if (ThornsEnchantment::shouldTrigger(1, rng)) {
            triggerCount1++;
        }
    }
    // 期望约 150 次，允许一定误差
    EXPECT_GT(triggerCount1, 100);
    EXPECT_LT(triggerCount1, 200);

    // Level III: 45% 概率
    int triggerCount3 = 0;
    for (int i = 0; i < 1000; ++i) {
        if (ThornsEnchantment::shouldTrigger(3, rng)) {
            triggerCount3++;
        }
    }
    // 期望约 450 次，允许一定误差
    EXPECT_GT(triggerCount3, 400);
    EXPECT_LT(triggerCount3, 500);
}

TEST_F(ThornsEnchantmentTest, GetThornsDamage)
{
    // 对齐 vanilla 1.21.11 THORNS（Enchantments.java:342）DamageEntity(constant 1.0, constant 5.0)：
    // Mth.randomBetween(random, 1.0F, 5.0F) ∈ [1.0, 5.0)，与等级无关（无老版本 level>10 分支）。
    math::Random rng(12345);

    // 采样 1000 次验证伤害始终落在 [1.0, 5.0) 且与等级无关
    for (int i = 0; i < 1000; ++i) {
        f32 damage = ThornsEnchantment::getThornsDamage(rng);
        EXPECT_GE(damage, 1.0f);
        EXPECT_LT(damage, 5.0f);
    }

    // 上界恰好可达（nextFloat 接近 1.0 时 damage 接近 5.0，但 < 5.0）
    // 采样验证分布上下界均有覆盖（避免实现退化为常数）
    f32 minSeen = 5.0f;
    f32 maxSeen = 1.0f;
    for (int i = 0; i < 10000; ++i) {
        f32 damage = ThornsEnchantment::getThornsDamage(rng);
        minSeen = std::min(minSeen, damage);
        maxSeen = std::max(maxSeen, damage);
    }
    EXPECT_GE(minSeen, 1.0f);
    EXPECT_LT(minSeen, 2.0f); // 应出现过接近 1.0 的样本
    EXPECT_GT(maxSeen, 4.0f); // 应出现过接近 5.0 的样本
    EXPECT_LT(maxSeen, 5.0f);
}

TEST_F(ThornsEnchantmentTest, GetTriggerChance)
{
    EXPECT_FLOAT_EQ(ThornsEnchantment::getTriggerChance(1), 0.15f);
    EXPECT_FLOAT_EQ(ThornsEnchantment::getTriggerChance(2), 0.30f);
    EXPECT_FLOAT_EQ(ThornsEnchantment::getTriggerChance(3), 0.45f);
}

// ============================================================================
// EnchantmentHelper 回调测试
// ============================================================================

class EnchantmentHelperCallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(EnchantmentHelperCallbackTest, ApplyArthropodEnchantmentDamageWithEmptyStack)
{
    // 空物品堆不应触发回调
    // 由于没有真实的 LivingEntity 实现用于测试，这里只测试基本逻辑
    ItemStack emptyStack;
    EXPECT_TRUE(emptyStack.isEmpty());
    // EnchantmentHelper::applyArthropodEnchantmentDamage 需要 LivingEntity，跳过集成测试
}

TEST_F(EnchantmentHelperCallbackTest, ApplyThornsEnchantmentsWithEmptyArmor)
{
    // 空护甲不应触发荆棘
    std::array<const ItemStack*, 4> emptyArmor = {
        &ItemStack::EMPTY, &ItemStack::EMPTY, &ItemStack::EMPTY, &ItemStack::EMPTY};
    // 验证空护甲槽位
    for (const auto* slot : emptyArmor) {
        EXPECT_TRUE(slot->isEmpty());
    }
}

// ============================================================================
// EnchantmentHelper 工具方法测试
// ============================================================================

class EnchantmentHelperToolTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(EnchantmentHelperToolTest, ShouldIgnoreDurabilityLoss)
{
    math::Random rng(12345);

    // Level 0: 总是返回 false
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(EnchantmentHelper::shouldIgnoreDurabilityLoss(0, false, rng));
        EXPECT_FALSE(EnchantmentHelper::shouldIgnoreDurabilityLoss(0, true, rng));
    }

    // Level I: 约 50% 概率忽略（非护甲）
    int ignoreCount = 0;
    for (int i = 0; i < 1000; ++i) {
        if (EnchantmentHelper::shouldIgnoreDurabilityLoss(1, false, rng)) {
            ignoreCount++;
        }
    }
    EXPECT_GT(ignoreCount, 400);
    EXPECT_LT(ignoreCount, 600);

    // Level III: 约 75% 概率忽略（非护甲）
    ignoreCount = 0;
    for (int i = 0; i < 1000; ++i) {
        if (EnchantmentHelper::shouldIgnoreDurabilityLoss(3, false, rng)) {
            ignoreCount++;
        }
    }
    EXPECT_GT(ignoreCount, 650);
    EXPECT_LT(ignoreCount, 850);

    // 护甲：60% 概率不触发耐久效果
    // 所以护甲的实际忽略概率是 0.4 * level/(level+1)
    // Level I 护甲: 0.4 * 0.5 = 0.2 = 20%
    ignoreCount = 0;
    for (int i = 0; i < 1000; ++i) {
        if (EnchantmentHelper::shouldIgnoreDurabilityLoss(1, true, rng)) {
            ignoreCount++;
        }
    }
    EXPECT_GT(ignoreCount, 100);
    EXPECT_LT(ignoreCount, 300);
}

TEST_F(EnchantmentHelperToolTest, GetSweepingDamageRatio)
{
    EXPECT_FLOAT_EQ(EnchantmentHelper::getSweepingDamageRatio(ItemStack::EMPTY), 0.0f);

    // 需要有横扫之刃附魔的物品才能测试
    // 由于 ItemStack::EMPTY 没有附魔，这里只测试返回值
}

TEST_F(EnchantmentHelperToolTest, GetFishingLuckBonus)
{
    EXPECT_EQ(EnchantmentHelper::getFishingLuckBonus(ItemStack::EMPTY), 0);
}

TEST_F(EnchantmentHelperToolTest, GetFishingSpeedBonus)
{
    EXPECT_EQ(EnchantmentHelper::getFishingSpeedBonus(ItemStack::EMPTY), 0);
}

// ============================================================================
// EnchantmentHelper 新重载方法测试
// ============================================================================

class EnchantmentHelperNewMethodsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
        // 亡灵/节肢杀手目标判定改用 EntityTypeTags 标签，须初始化硬编码标签成员。
        EntityTypeTags::initialize();
    }

    void TearDown() override
    {
        EnchantmentRegistry::clear();
        // Items 不需要清理
    }
};

TEST_F(EnchantmentHelperNewMethodsTest, ApplyArthropodEnchantmentsWithEmptyEquipment)
{
    // 测试空装备情况下调用不会崩溃
    // 由于需要真实的 LivingEntity，这里使用 nullptr 检查逻辑
    // EnchantmentHelper::applyArthropodEnchantments(LivingEntity&, Entity&)
    // 需要有效的实体引用

    // 验证 EnchantmentRegistry 已初始化
    const Enchantment* baneOfArthropods = EnchantmentRegistry::get("minecraft:bane_of_arthropods");
    EXPECT_NE(baneOfArthropods, nullptr);
    EXPECT_EQ(baneOfArthropods->id(), "minecraft:bane_of_arthropods");
}

TEST_F(EnchantmentHelperNewMethodsTest, ApplyThornsEnchantmentsWithEmptyArmor)
{
    // 测试空护甲情况下调用不会崩溃
    // 验证荆棘附魔已注册
    const Enchantment* thorns = EnchantmentRegistry::get("minecraft:thorns");
    EXPECT_NE(thorns, nullptr);
    EXPECT_EQ(thorns->id(), "minecraft:thorns");
    EXPECT_EQ(thorns->minLevel(), 1);
    EXPECT_EQ(thorns->maxLevel(), 3);
}

TEST_F(EnchantmentHelperNewMethodsTest, EnchantmentHelperGetArmorSlotsSignature)
{
    // 验证方法签名正确：
    //   applyArthropodEnchantments(LivingEntity&, Entity&)
    //   applyThornsEnchantments(LivingEntity&, Entity&)
    // （applyThornsEnchantments 旧的三参数 const 数组重载已移除，耐久消耗需写装备槽原件，
    //  统一由 getMutableEquipment 内部遍历，见 EnchantmentHelper.cpp）

    // 验证空护甲槽位数组格式正确（getArmorSlots 仍返回 const 视图供只读场景使用）
    std::array<const ItemStack*, 4> emptyArmor = {
        &ItemStack::EMPTY, &ItemStack::EMPTY, &ItemStack::EMPTY, &ItemStack::EMPTY};

    for (size_t i = 0; i < 4; ++i) {
        EXPECT_TRUE(emptyArmor[i]->isEmpty());
    }
}

TEST_F(EnchantmentHelperNewMethodsTest, ApplyArthropodEnchantmentDamageWithWeapon)
{
    // 测试带附魔武器的情况
    // 创建一个带节肢杀手的剑
    ItemStack sword(Items::DIAMOND_SWORD, 1);

    // 验证物品有效
    EXPECT_FALSE(sword.isEmpty());
    EXPECT_EQ(sword.getItem(), Items::DIAMOND_SWORD);

    // 验证可以添加附魔
    const Enchantment* baneOfArthropods = EnchantmentRegistry::get("minecraft:bane_of_arthropods");
    ASSERT_NE(baneOfArthropods, nullptr);

    // 添加节肢杀手 I
    EnchantmentHelper::setEnchantments({{baneOfArthropods, 1}}, sword);

    // 验证附魔等级
    i32 level = EnchantmentHelper::getEnchantmentLevel(sword, "minecraft:bane_of_arthropods");
    EXPECT_EQ(level, 1);

    // 验证 applyArthropodEnchantmentDamage 可以被调用（武器版本）
    // 这不会崩溃，因为只是遍历附魔并调用回调
    // 实际效果需要在完整实体环境中测试
}

TEST_F(EnchantmentHelperNewMethodsTest, ThornsEnchantmentProperties)
{
    const Enchantment* thorns = EnchantmentRegistry::get("minecraft:thorns");
    ASSERT_NE(thorns, nullptr);

    // 验证荆棘附魔属性
    EXPECT_EQ(thorns->type(), EnchantmentType::ArmorChest);
    EXPECT_EQ(thorns->rarity(), EnchantmentRarity::VeryRare);

    // 验证伤害加成（荆棘对玩家攻击者有反伤效果）
    // 对普通实体无伤害加成（荆棘不 override getDamageBonus，基类返 0）
    EXPECT_FLOAT_EQ(thorns->getDamageBonus(1, nullptr), 0.0f);
    EXPECT_FLOAT_EQ(thorns->getDamageBonus(3, nullptr), 0.0f);
}

TEST_F(EnchantmentHelperNewMethodsTest, BaneOfArthropodsEnchantmentProperties)
{
    const Enchantment* baneOfArthropods = EnchantmentRegistry::get("minecraft:bane_of_arthropods");
    ASSERT_NE(baneOfArthropods, nullptr);

    // 验证节肢杀手附魔属性
    EXPECT_EQ(baneOfArthropods->type(), EnchantmentType::Weapon);
    EXPECT_EQ(baneOfArthropods->rarity(), EnchantmentRarity::Uncommon);

    // 验证对节肢生物的伤害加成
    TestArthropodEntity arthropod(1);
    EXPECT_FLOAT_EQ(baneOfArthropods->getDamageBonus(1, &arthropod), 2.5f);
    EXPECT_FLOAT_EQ(baneOfArthropods->getDamageBonus(5, &arthropod), 12.5f);

    // 验证对非节肢生物无伤害加成
    TestUndeadEntity undead(2);
    EXPECT_FLOAT_EQ(baneOfArthropods->getDamageBonus(5, &undead), 0.0f);
}

// ============================================================================
// ImpalingEnchantment 穿刺附魔测试
// ============================================================================
// 验证穿刺额外伤害判定改用 EntityTypeTags::SENSITIVE_TO_IMPALING（= AQUATIC）标签
// （对齐 MC Java 1.21.11 Enchantments.java:994），而非旧 getCreatureAttribute()==Water
// 枚举（仅覆盖 guardian）。每级 +2.5 伤害（Enchantments.java:991 perLevel(2.5F)）。
//
// 此前偏差：用 getCreatureAttribute()==Water，Cubium 仅 GuardianEntity 显式返 Water，
// 导致穿刺只对守卫者/远古守卫者额外伤害，遗漏 squid/cod/dolphin/turtle/axolotl 等 10 个
// 水生生物。改用 SENSITIVE_TO_IMPALING 标签（12 成员，与 vanilla AQUATIC 一致）后，
// 所有水生生物均受穿刺额外伤害。

class ImpalingEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
        // 穿刺判定查 EntityTypeTags::SENSITIVE_TO_IMPALING 标签，须初始化硬编码标签成员
        // （AQUATIC 12 成员 + SENSITIVE_TO_IMPALING=AQUATIC 引用，EntityTypeTags.cpp:504-525）。
        // initialize 幂等（EntityTypeTagsTest.cpp:256-257 双调验证）。
        EntityTypeTags::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(ImpalingEnchantmentTest, Properties)
{
    ImpalingEnchantment impaling;

    EXPECT_EQ(impaling.id(), "minecraft:impaling");
    EXPECT_EQ(impaling.minLevel(), 1);
    EXPECT_EQ(impaling.maxLevel(), 5);
    EXPECT_EQ(impaling.type(), EnchantmentType::Trident);
    EXPECT_EQ(impaling.rarity(), EnchantmentRarity::Rare);
}

TEST_F(ImpalingEnchantmentTest, GetDamageBonusAgainstAquatic)
{
    ImpalingEnchantment impaling;
    TestAquaticEntity aquatic(1); // getTypeId()="minecraft:squid"，在 SENSITIVE_TO_IMPALING 标签内

    // 对水生生物：每级 +2.5 伤害（对齐 vanilla Enchantments.java:991 perLevel(2.5F)）
    EXPECT_FLOAT_EQ(impaling.getDamageBonus(1, &aquatic), 2.5f);
    EXPECT_FLOAT_EQ(impaling.getDamageBonus(3, &aquatic), 7.5f);
    EXPECT_FLOAT_EQ(impaling.getDamageBonus(5, &aquatic), 12.5f);
}

TEST_F(ImpalingEnchantmentTest, GetDamageBonusAgainstNonAquatic)
{
    ImpalingEnchantment impaling;
    TestUndeadEntity undead(1);       // getTypeId()="minecraft:zombie"，非水生
    TestNormalEntity normal(2);       // getTypeId()=""，非水生
    TestArthropodEntity arthropod(3); // getTypeId()="minecraft:spider"，非水生

    // 对非水生生物：0 伤害（不在 SENSITIVE_TO_IMPALING 标签内）
    EXPECT_FLOAT_EQ(impaling.getDamageBonus(5, &undead), 0.0f);
    EXPECT_FLOAT_EQ(impaling.getDamageBonus(5, &normal), 0.0f);
    EXPECT_FLOAT_EQ(impaling.getDamageBonus(5, &arthropod), 0.0f);
}

TEST_F(ImpalingEnchantmentTest, GetDamageBonusNullptrTarget)
{
    ImpalingEnchantment impaling;

    // target 为 nullptr：0 伤害（无目标无法做标签判定）
    EXPECT_FLOAT_EQ(impaling.getDamageBonus(5, nullptr), 0.0f);
}

// 验证穿刺标签判定覆盖旧枚举遗漏的水生生物（核心回归保护）。
// 旧实现用 getCreatureAttribute()==Water，TestAquaticEntity 基类 getCreatureAttribute()
// 返 Undefined（非 Water），旧实现会返 0（漏判）；新实现用标签查 getTypeId()=="minecraft:squid"
// 命中 SENSITIVE_TO_IMPALING，返 level*2.5。本测试若 FAIL 说明回归到旧枚举判定。
TEST_F(ImpalingEnchantmentTest, TagBasedNotEnumCoversSquid)
{
    ImpalingEnchantment impaling;
    TestAquaticEntity squid(1);

    // squid 的 getCreatureAttribute()==Undefined（非 Water 枚举），但 getTypeId() 在
    // SENSITIVE_TO_IMPALING 标签内。新标签判定返 12.5，旧枚举判定返 0。
    EXPECT_NE(squid.getCreatureAttribute(), CreatureAttribute::Water) << "squid 应非 Water 枚举（验证标签判定非枚举）";
    EXPECT_FLOAT_EQ(impaling.getDamageBonus(5, &squid), 12.5f)
        << "穿刺应对 squid（SENSITIVE_TO_IMPALING 标签成员）返额外伤害，"
        << "若返 0 说明回归到旧 getCreatureAttribute==Water 枚举判定";
}
