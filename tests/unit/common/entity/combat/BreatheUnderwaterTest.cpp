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

// CAN_BREATHE_UNDER_WATER 标签查询修复测试（守卫者水下呼吸）。
//
// 验证 LivingEntity::canBreatheUnderwater()（LivingEntity.cpp）对齐 vanilla 1.21.11
// LivingEntity.canBreatheUnderwater():385：
//   return this.getType().is(EntityTypeTags.CAN_BREATHE_UNDER_WATER);
//
// 此前缺陷：Cubium canBreatheUnderwater() 基类仅查 getCreatureAttribute()==Undead。
// GuardianEntity 继承 MonsterEntity（非 WaterMobEntity），getCreatureAttribute=Water（非 Undead），
// 无 canBreatheUnderwater override → 返 false → updateAirSupply 在水中消耗空气 → 守卫者/远古守卫者
// 在水中溺水扣血，与 vanilla 相反（vanilla 守卫者永久水下生存，CAN_BREATHE_UNDER_WATER 标签含
// guardian/elder_guardian）。改查标签对齐 vanilla。
//
// 测试设计（4 例，含正反对照）：
//   - GuardianCanBreatheUnderwater：守卫者 canBreatheUnderwater()==true（标签查询修复）
//   - GuardianDoesNotDrownInWater：守卫者入水多次 updateAirSupply → 空气不降、血量不降（行为级验证）
//   - CreeperDrownsInWater：苦力怕（不在标签）入水 updateAirSupply → 空气下降（无回归：非水生实体仍溺水）
//   - ZombieCanBreatheUnderwater：僵尸（标签内亡灵）canBreatheUnderwater()==true（亡灵仍正确）
//
// 守卫者需 setTypeId("minecraft:guardian") 使 getTypeId() 返回标签成员字符串，标签 contains 命中。
// updateAirSupply 行为测试用 setInWater(true) 模拟入水（isInWater() 返 m_inWater），测试世界
// getBlockState 返 AIR（非气泡柱），故 inBubbleColumn=false 走正常空气消耗路径。
//
// Ref: vanilla LivingEntity.java:385（canBreatheUnderwater 查 CAN_BREATHE_UNDER_WATER 标签）
// Ref: vanilla EntityTypeTagsProvider.java（CAN_BREATHE_UNDER_WATER 含 guardian/elder_guardian）
// Ref: LivingEntity.cpp（canBreatheUnderwater 改查标签 + isInitialized 安全回退）
// Ref: LivingEntity.cpp:2603（updateAirSupply canBreatheUnderwater 门控空气消耗）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/basic/CreeperEntity.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

// BaseTestWorld protected 构造，派生公开。playSound 默认空实现吸收 updateAirSupply 溺水时的音效。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;

    [[nodiscard]] world::tick::TickManager& tickManager() override { throw std::runtime_error("not implemented"); }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("not implemented");
    }
};

} // namespace

class BreatheUnderwaterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 实体类型标签初始化（进程级幂等）。CAN_BREATHE_UNDER_WATER 成员集在 initialize() 注册
        // （EntityTypeTags.cpp:592，含 guardian/elder_guardian/亡灵/水生生物）。未初始化时
        // canBreatheUnderwater() 走 isInitialized() 安全回退（仅亡灵），守卫者返 false。
        EntityTypeTags::initialize();
        // 方块与物品注册初始化（进程级幂等）。水下呼吸测试需构造钻石头盔物品堆
        // （Items::DIAMOND_HELMET），物品注册依赖方块注册先就绪（ArmorMaterialTest 同范式）。
        VanillaBlocks::initialize();
        Items::initialize();
        // 附魔注册初始化（进程级幂等）。水下呼吸附魔经装备管线注入 oxygen_bonus 属性修饰符，
        // 需 RespirationEnchantment 已注册才能 addEnchantment("minecraft:respiration", level)。
        item::enchant::EnchantmentRegistry::initialize();
    }

    TestWorld m_world;
};

// 守卫者可在水下呼吸（CAN_BREATHE_UNDER_WATER 标签含 guardian，修复后查标签返 true）。
TEST_F(BreatheUnderwaterTest, GuardianCanBreatheUnderwater)
{
    GuardianEntity guardian(EntityInstanceId(1), mc::test::testEcsRegistry());
    guardian.setWorld(&m_world);
    guardian.setTypeId("minecraft:guardian"); // 使 getTypeId() 命中标签成员

    // 修复前：getCreatureAttribute()==Water 非 Undead → false → 溺水
    // 修复后：标签含 guardian → true
    EXPECT_TRUE(guardian.canBreatheUnderwater());
}

// 守卫者入水后空气不消耗、血量不降（行为级验证：修复前会溺水扣血）。
//
// updateAirSupply 在 canBreatheUnderwater()==true 时不消耗空气（canBreathe=true 短路）。
// 多次调用后 air 应保持初始值（maxAir）、health 不变。修复前 canBreatheUnderwater=false →
// 空气逐 tick 下降，耗尽后 shouldTakeDrowningDamage 触发溺水扣血。
TEST_F(BreatheUnderwaterTest, GuardianDoesNotDrownInWater)
{
    GuardianEntity guardian(EntityInstanceId(1), mc::test::testEcsRegistry());
    guardian.setWorld(&m_world);
    guardian.setTypeId("minecraft:guardian");
    guardian.setHealth(guardian.maxHealth());
    guardian.setInWater(true); // 模拟入水（isInWater() 返 m_inWater）

    const i32 airBefore = guardian.air();
    const f32 healthBefore = guardian.health();

    // 推进多次空气更新（远超 maxAir，足以让非水生实体空气耗尽溺水）
    for (int i = 0; i < 400; ++i) {
        guardian.updateAirSupply();
    }

    // 守卫者可水下呼吸 → 空气不消耗、不溺水扣血
    EXPECT_EQ(guardian.air(), airBefore);
    EXPECT_FLOAT_EQ(guardian.health(), healthBefore);
}

// 苦力怕（不在 CAN_BREATHE_UNDER_WATER 标签）入水后空气消耗（无回归：非水生实体仍溺水）。
//
// 对照组：证明修复未误伤普通怪物。苦力怕 canBreatheUnderwater()==false → updateAirSupply
// 在水中消耗空气，多次调用后 air 下降。
TEST_F(BreatheUnderwaterTest, CreeperDrownsInWater)
{
    CreeperEntity creeper(EntityInstanceId(1), mc::test::testEcsRegistry());
    creeper.setWorld(&m_world);
    creeper.setTypeId("minecraft:creeper");
    creeper.setHealth(creeper.maxHealth());
    creeper.setInWater(true);

    const i32 airBefore = creeper.air();
    ASSERT_GT(airBefore, 0);

    // 推进空气更新，苦力怕不可水下呼吸 → 空气下降
    creeper.updateAirSupply();
    EXPECT_LT(creeper.air(), airBefore);
}

// 僵尸（标签内亡灵）可在水下呼吸（亡灵仍正确，无回归）。
//
// 僵尸 getCreatureAttribute==Undead，标签 #undead 子标签成员含 zombie。修复前（Undead 判定）
// 与修复后（标签查询）均返 true。本测试保护亡灵水下呼吸不回归。
TEST_F(BreatheUnderwaterTest, ZombieCanBreatheUnderwater)
{
    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setWorld(&m_world);
    zombie.setTypeId("minecraft:zombie");

    EXPECT_TRUE(zombie.canBreatheUnderwater());
}

// ============================================================================
// 水下呼吸附魔 oxygen_bonus 属性端到端测试（任务 #257）
//
// 验证完整链路：附魔头盔 → setEquipment → detectEquipmentUpdates →
// EnchantmentHelper::applyEnchantmentAttributeModifiers → oxygen_bonus 修饰符应用 →
// decreaseAirSupply 概率消耗。单元测试 DecreaseAirSupply_OxygenBonusReducesConsumption
// 用 attributes().addModifier 直接注入修饰符绕过装备管线，本测试补足装备管线闭环。
//
// 对齐 vanilla 1.21.11：水下呼吸 III 经 enchantment.respiration 修饰符（ADD_VALUE，HEAD 槽位，
// 每级 +1.0）给 oxygen_bonus +3.0，decreaseAirSupply 读 oxygen_bonus=3 时 75% 不消耗
// （1/(3+1)=25% 消耗概率）。
// ============================================================================

// 戴水下呼吸 III 头盔经装备管线后 oxygen_bonus 属性值 == 3.0（修饰符注入闭环验证）。
TEST_F(BreatheUnderwaterTest, RespirationHelmetAppliesOxygenBonus)
{
    CreeperEntity creeper(EntityInstanceId(1), mc::test::testEcsRegistry());
    creeper.setWorld(&m_world);
    creeper.setTypeId("minecraft:creeper");

    // 装备前 oxygen_bonus 为默认 0.0
    EXPECT_DOUBLE_EQ(creeper.attributes().getValue(entity::attribute::Attributes::OXYGEN_BONUS, -1.0), 0.0);

    // 构造水下呼吸 III 钻石头盔并装备到 HEAD 槽
    const Item* diamondHelmet = Items::DIAMOND_HELMET;
    ASSERT_NE(diamondHelmet, nullptr);
    ItemStack helmetStack(diamondHelmet, 1);
    helmetStack.addEnchantment("minecraft:respiration", 3);
    creeper.setEquipment(EquipmentSlot::Head, helmetStack);

    // detectEquipmentUpdates 应用装备附魔的常驻属性修饰符（对齐 LivingEntity.tick 调用顺序）
    creeper.detectEquipmentUpdates();

    // 装备后 oxygen_bonus 应为 3.0（水下呼吸 III 每级 +1.0，证明装备管线注入成功）
    EXPECT_DOUBLE_EQ(creeper.attributes().getValue(entity::attribute::Attributes::OXYGEN_BONUS, -1.0), 3.0);

    // 卸下头盔后修饰符应移除，oxygen_bonus 回到 0.0（对齐 stopLocationBasedEffects 清理）
    creeper.setEquipment(EquipmentSlot::Head, ItemStack());
    creeper.detectEquipmentUpdates();
    EXPECT_DOUBLE_EQ(creeper.attributes().getValue(entity::attribute::Attributes::OXYGEN_BONUS, -1.0), 0.0);
}

// 戴水下呼吸 III 头盔入水后氧气消耗量显著低于无附魔对照（概率门控端到端）。
//
// 无附魔时 oxygen_bonus=0，decreaseAirSupply 每 tick 必消耗 1，N tick 消耗量 == N（确定性）。
// 水下呼吸 III 时 oxygen_bonus=3，75% 不消耗，N=300 期望消耗 ~75。断言附魔消耗量 < N/2
// （<150）稳健证明概率门控端到端生效，同时附魔消耗量 > 0 证明非"完全不消耗"的水下呼吸效果
// （区别于 canBreatheUnderwater 标签实体的零消耗）。
TEST_F(BreatheUnderwaterTest, RespirationHelmetSlowsAirConsumption)
{
    constexpr i32 kTicks = 300;

    // 对照组：无附魔苦力怕入水，氧气确定性地每 tick 消耗 1
    CreeperEntity baseline(EntityInstanceId(1), mc::test::testEcsRegistry());
    baseline.setWorld(&m_world);
    baseline.setTypeId("minecraft:creeper");
    baseline.setHealth(baseline.maxHealth());
    baseline.setInWater(true);
    const i32 baselineAirBefore = baseline.air();
    for (i32 i = 0; i < kTicks; ++i) {
        baseline.updateAirSupply();
    }
    const i32 baselineConsumed = baselineAirBefore - baseline.air();
    // 无附魔 oxygen_bonus=0 必消耗，消耗量 == kTicks（验证确定性对照成立）
    EXPECT_EQ(baselineConsumed, kTicks);

    // 实验组：戴水下呼吸 III 头盔的苦力怕入水
    CreeperEntity enchanted(EntityInstanceId(2), mc::test::testEcsRegistry());
    enchanted.setWorld(&m_world);
    enchanted.setTypeId("minecraft:creeper");
    enchanted.setHealth(enchanted.maxHealth());

    const Item* diamondHelmet = Items::DIAMOND_HELMET;
    ASSERT_NE(diamondHelmet, nullptr);
    ItemStack helmetStack(diamondHelmet, 1);
    helmetStack.addEnchantment("minecraft:respiration", 3);
    enchanted.setEquipment(EquipmentSlot::Head, helmetStack);
    enchanted.detectEquipmentUpdates();
    // 确认修饰符已应用
    ASSERT_DOUBLE_EQ(enchanted.attributes().getValue(entity::attribute::Attributes::OXYGEN_BONUS, -1.0), 3.0);

    enchanted.setInWater(true);
    const i32 enchantedAirBefore = enchanted.air();
    for (i32 i = 0; i < kTicks; ++i) {
        enchanted.updateAirSupply();
    }
    const i32 enchantedConsumed = enchantedAirBefore - enchanted.air();

    // 水下呼吸 III 显著降低消耗（期望 ~75，断言 < kTicks/2=150 稳健），且非零消耗
    // （区别于 canBreatheUnderwater 标签实体的零消耗）
    EXPECT_LT(enchantedConsumed, kTicks / 2);
    EXPECT_GT(enchantedConsumed, 0);
}
