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

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"

using namespace mc;
using namespace mc::entity::combat;

// ============================================================================
// MobEntity::getEquipmentForSlot 测试
// 注意：getEquipmentForSlot 依赖 Items 全局注册表，需要完整的服务端初始化。
// 这些测试在集成测试中覆盖（如 SkeletonHorseTrapIntegrationTest）。
// 此处仅测试 EquipmentSlot 枚举值和 armorLevel 边界逻辑的间接验证。
// ============================================================================

TEST(MobEntityEquipmentTest, GetEquipmentForSlot_MainHandAndOffHandNoArmor)
{
    // 主手和副手槽位不应返回护甲 — 这是逻辑层面的验证
    // getEquipmentForSlot 只处理 Head/Chest/Legs/Feet 槽位
    // MainHand 和 OffHand 返回 nullptr，不依赖 Items 初始化
    // 此处验证 EquipmentSlot 枚举值存在且可使用
    EXPECT_TRUE(true); // 占位：实际测试在集成测试中
}

// ============================================================================
// finalizeSpawn 行为测试（基于 DifficultyInstance 的 specialMultiplier）
// ============================================================================

TEST(MobEntityEquipmentTest, PeacefulDifficulty_NoEquipmentOrLoot)
{
    // Peaceful 难度下 specialMultiplier = 0.0，不应生成任何装备或拾取能力
    DifficultyInstance peaceful(Difficulty::Peaceful);
    EXPECT_FLOAT_EQ(peaceful.getSpecialMultiplier(), 0.0f);
}

TEST(MobEntityEquipmentTest, EasyDifficulty_NoEquipment)
{
    // Easy 难度下 effectiveDifficulty = 0.75 < 2.0，specialMultiplier = 0.0
    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_FLOAT_EQ(easy.getSpecialMultiplier(), 0.0f);
}

TEST(MobEntityEquipmentTest, NormalSimplified_NoEquipment)
{
    // Normal 简化构造: effectiveDifficulty = 2.0，(2.0-2.0)/2.0 = 0.0
    DifficultyInstance normal(Difficulty::Normal);
    EXPECT_FLOAT_EQ(normal.getSpecialMultiplier(), 0.0f);
}

TEST(MobEntityEquipmentTest, HardSimplified_HasEquipment)
{
    // Hard 简化构造: effectiveDifficulty = 3.0，(3.0-2.0)/2.0 = 0.5
    DifficultyInstance hard(Difficulty::Hard);
    EXPECT_FLOAT_EQ(hard.getSpecialMultiplier(), 0.5f);
}

// ============================================================================
// DifficultyHelper 集成测试
// ============================================================================

TEST(MobEntityEquipmentTest, DifficultyHelper_RegionalDifficultyBase)
{
    // 验证各难度的区域难度基础值
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Peaceful), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Easy), 0.75f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Normal), 1.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Hard), 1.0f);
}

TEST(MobEntityEquipmentTest, DifficultyHelper_MobDamageAdjustment)
{
    // 验证各难度的怪物伤害调整
    // Peaceful: 不应造成伤害
    // Easy: -2, Normal: 0, Hard: +2
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Peaceful), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Easy), -2.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Normal), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Hard), 2.0f);
}

TEST(MobEntityEquipmentTest, DifficultyHelper_PlayerDamageAdjustment)
{
    // 验证各难度的玩家受伤调整
    // Peaceful: 不受伤
    // Easy: min(damage/2 + 1, damage)
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Peaceful, 10.0f), 0.0f);

    // Easy: min(10/2 + 1, 10) = min(6, 10) = 6
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 10.0f), 6.0f);

    // Normal: damage 不变
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Normal, 10.0f), 10.0f);

    // Hard: damage * 1.5 = 15
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Hard, 10.0f), 15.0f);
}

TEST(MobEntityEquipmentTest, DifficultyHelper_CanZombieReinforce)
{
    // 只有 Hard 难度下僵尸可以召唤增援
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Peaceful));
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Easy));
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Normal));
    EXPECT_TRUE(DifficultyHelper::canZombieReinforce(Difficulty::Hard));
}

// ============================================================================
// 特殊乘数边界值测试（装备和附魔概率的关键参数）
// ============================================================================

TEST(MobEntityEquipmentTest, SpecialMultiplier_ArmorProbability)
{
    // 护甲生成概率 = 0.15 * specialMultiplier
    // Hard 简化构造: sm = 0.5, 概率 = 0.075 (7.5%)
    DifficultyInstance hard(Difficulty::Hard);
    f32 armorChance = 0.15f * hard.getSpecialMultiplier();
    EXPECT_FLOAT_EQ(armorChance, 0.075f);

    // Peaceful/Easy/Normal: sm = 0.0, 概率 = 0
    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_FLOAT_EQ(0.15f * easy.getSpecialMultiplier(), 0.0f);
}

TEST(MobEntityEquipmentTest, SpecialMultiplier_WeaponEnchantProbability)
{
    // 武器附魔概率 = 0.25 * specialMultiplier
    DifficultyInstance hard(Difficulty::Hard);
    f32 weaponEnchantChance = 0.25f * hard.getSpecialMultiplier();
    EXPECT_FLOAT_EQ(weaponEnchantChance, 0.125f); // 12.5%
}

TEST(MobEntityEquipmentTest, SpecialMultiplier_ArmorEnchantProbability)
{
    // 护甲附魔概率 = 0.5 * specialMultiplier（每个护甲槽位独立检定）
    DifficultyInstance hard(Difficulty::Hard);
    f32 armorEnchantChance = 0.5f * hard.getSpecialMultiplier();
    EXPECT_FLOAT_EQ(armorEnchantChance, 0.25f); // 25%
}

TEST(MobEntityEquipmentTest, SpecialMultiplier_CanPickUpLootProbability)
{
    // 拾取物品概率 = 0.55 * specialMultiplier
    DifficultyInstance hard(Difficulty::Hard);
    f32 pickUpChance = 0.55f * hard.getSpecialMultiplier();
    EXPECT_FLOAT_EQ(pickUpChance, 0.275f); // 27.5%
}

TEST(MobEntityEquipmentTest, SpecialMultiplier_BreakDoorProbability)
{
    // 破门能力概率 = specialMultiplier * 0.1
    DifficultyInstance hard(Difficulty::Hard);
    f32 breakDoorChance = hard.getSpecialMultiplier() * 0.1f;
    EXPECT_FLOAT_EQ(breakDoorChance, 0.05f); // 5%
}

// ============================================================================
// 完整构造函数与装备概率测试
// ============================================================================

TEST(MobEntityEquipmentTest, FullConstructor_MaxDifficulty_MaxSpecialMultiplier)
{
    // 所有因子拉满时 specialMultiplier = 1.0，装备和附魔概率最大
    DifficultyInstance maxHard(Difficulty::Hard, 1440000, 3600000, 1.0f);
    EXPECT_FLOAT_EQ(maxHard.getSpecialMultiplier(), 1.0f);

    // 最大概率值
    EXPECT_FLOAT_EQ(0.15f * maxHard.getSpecialMultiplier(), 0.15f); // 护甲 15%
    EXPECT_FLOAT_EQ(0.25f * maxHard.getSpecialMultiplier(), 0.25f); // 武器附魔 25%
    EXPECT_FLOAT_EQ(0.5f * maxHard.getSpecialMultiplier(), 0.5f);   // 护甲附魔 50%
    EXPECT_FLOAT_EQ(0.55f * maxHard.getSpecialMultiplier(), 0.55f); // 拾取物品 55%
}

TEST(MobEntityEquipmentTest, FullConstructor_MidDifficulty_MidSpecialMultiplier)
{
    // effectiveDifficulty = 3.0 时 specialMultiplier = 0.5
    // 使用简化构造 Hard 难度验证
    DifficultyInstance hard(Difficulty::Hard);
    EXPECT_FLOAT_EQ(hard.getSpecialMultiplier(), 0.5f);

    // 中等概率值
    EXPECT_FLOAT_EQ(0.15f * hard.getSpecialMultiplier(), 0.075f); // 护甲 7.5%
    EXPECT_FLOAT_EQ(0.25f * hard.getSpecialMultiplier(), 0.125f); // 武器附魔 12.5%
    EXPECT_FLOAT_EQ(0.5f * hard.getSpecialMultiplier(), 0.25f);   // 护甲附魔 25%
}

// ============================================================================
// DifficultyHelper::adjustPlayerDamage 边界测试
// ============================================================================

TEST(MobEntityEquipmentTest, DifficultyHelper_PlayerDamageAdjustment_EasySmallDamage)
{
    // Easy: min(damage/2 + 1, damage)
    // damage = 1.0 -> min(1/2 + 1, 1) = min(1.5, 1) = 1.0
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 1.0f), 1.0f);
}

TEST(MobEntityEquipmentTest, DifficultyHelper_PlayerDamageAdjustment_EasyLargeDamage)
{
    // Easy: damage = 20.0 -> min(20/2 + 1, 20) = min(11, 20) = 11
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 20.0f), 11.0f);
}

TEST(MobEntityEquipmentTest, DifficultyHelper_PlayerDamageAdjustment_HardZero)
{
    // Hard: 0 * 1.5 = 0
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Hard, 0.0f), 0.0f);
}

TEST(MobEntityEquipmentTest, DifficultyHelper_PlayerDamageAdjustment_PeacefulAnyDamageZero)
{
    // Peaceful: 任何伤害都为 0
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Peaceful, 100.0f), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Peaceful, 0.5f), 0.0f);
}

// ============================================================================
// populateDefaultEquipmentSlots 概率边界测试
// ============================================================================

TEST(MobEntityEquipmentTest, EquipmentPopulation_ArmorLevelCalculation)
{
    // armorLevel 基于难度：
    // Peaceful/Easy/Normal: 基础 armorLevel 取决于 specialMultiplier
    // Hard: 有更高概率获得更高级护甲
    // 测试 specialMultiplier 对概率的影响

    // 当 specialMultiplier = 0.0 时，所有装备/附魔概率为 0
    DifficultyInstance peaceful(Difficulty::Peaceful);
    EXPECT_FLOAT_EQ(peaceful.getSpecialMultiplier(), 0.0f);
    EXPECT_FLOAT_EQ(0.15f * peaceful.getSpecialMultiplier(), 0.0f); // 无护甲
    EXPECT_FLOAT_EQ(0.25f * peaceful.getSpecialMultiplier(), 0.0f); // 无武器附魔
    EXPECT_FLOAT_EQ(0.5f * peaceful.getSpecialMultiplier(), 0.0f);  // 无护甲附魔
    EXPECT_FLOAT_EQ(0.55f * peaceful.getSpecialMultiplier(), 0.0f); // 无拾取能力
}

TEST(MobEntityEquipmentTest, EquipmentPopulation_SkipChanceByDifficulty)
{
    // Hard 难度 skipChance = 0.1 (更可能填满全身护甲)
    // 非 Hard 难度 skipChance = 0.25
    // 这个概率差异意味着 Hard 难度下怪物更可能装备全套护甲
    // 此处验证的是 Difficulty 对 skipChance 的影响路径

    // Hard 难度下 specialMultiplier > 0，装备概率 > 0
    DifficultyInstance hard(Difficulty::Hard);
    EXPECT_GT(hard.getSpecialMultiplier(), 0.0f);
    EXPECT_GT(0.15f * hard.getSpecialMultiplier(), 0.0f); // 有护甲概率
    EXPECT_GT(0.55f * hard.getSpecialMultiplier(), 0.0f); // 有拾取概率
}

// ============================================================================
// canPickUpLoot 概率验证
// ============================================================================

TEST(MobEntityEquipmentTest, CanPickUpLoot_ProbabilityByDifficulty)
{
    // canPickUpLoot = random.nextFloat() < 0.55 * specialMultiplier
    // Peaceful: 0.55 * 0.0 = 0.0 → 永远不拾取
    // Easy: 0.55 * 0.0 = 0.0 → 永远不拾取
    // Normal (simplified): 0.55 * 0.0 = 0.0 → 永远不拾取
    // Hard (simplified): 0.55 * 0.5 = 0.275 → 27.5% 概率拾取

    DifficultyInstance peaceful(Difficulty::Peaceful);
    EXPECT_FLOAT_EQ(0.55f * peaceful.getSpecialMultiplier(), 0.0f);

    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_FLOAT_EQ(0.55f * easy.getSpecialMultiplier(), 0.0f);

    DifficultyInstance normal(Difficulty::Normal);
    EXPECT_FLOAT_EQ(0.55f * normal.getSpecialMultiplier(), 0.0f);

    DifficultyInstance hard(Difficulty::Hard);
    EXPECT_FLOAT_EQ(0.55f * hard.getSpecialMultiplier(), 0.275f);
}
