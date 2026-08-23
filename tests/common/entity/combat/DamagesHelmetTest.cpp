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
 * LIABILITY IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// DAMAGES_HELMET 标签运行时查询测试。
//
// 验证 LivingEntity::actuallyHurt（LivingEntity.cpp:310）对 DAMAGES_HELMET 标签伤害源
// （成员集 = {FallingAnvil, FallingBlock, FallingStalactite}，DamageTypeTags.cpp:811-815）
// 命中戴头盔的实体时，伤害 ×0.75（减免 1/4），对齐 vanilla LivingEntity.hurtServer:1182-1185：
//   if (source.is(DamageTypeTags.DAMAGES_HELMET) && !helmet.isEmpty()) {
//       hurtHelmet(source, amount);   // 基类空实现（LivingEntity.java:1793-1794）
//       amount *= 0.75F;
//   }
// 该分支位于 hurtServer 主流程护甲减伤（getDamageAfterArmorAbsorb）之前，独立于 bypassesArmor
// 门控——即无论伤害是否绕过护甲，只要戴头盔且伤害类型属于 DAMAGES_HELMET，均先 ×0.75。
//
// 此前缺陷：Cubium actuallyHurt 完全缺失 DAMAGES_HELMET 分支，坠落铁砧/坠落方块/坠落钟乳石
// 命中戴头盔实体时全额伤害，偏离 vanilla（vanilla 中头盔吸收 1/4 此类伤害）。
//
// 修复：actuallyHurt 的 IS_FREEZING 段（1.5）后、护甲减伤段（2）前新增 DAMAGES_HELMET 分支
// （1.6，LivingEntity.cpp:345-354）：戴头盔→hurtHelmet（基类空实现带 TODO）+ amount *= 0.75f。
//
// 测试设计（三例交叉验证，TestLivingEntity 直接调用 public actuallyHurt 观察扣血）：
//   - FallingAnvilReducedByHelmet：戴钻石头盔受 fallingAnvil 10 伤害，扣 7.5（×0.75）。
//   - FallingAnvilNotReducedWithoutHelmet：无头盔受 fallingAnvil 10 伤害，扣 10（对照证明
//     伤害本身生效，排除"伤害未生效致戴头盔也扣 0"的假通过）。
//   - MagicNotAffectedByHelmet：戴头盔受 magic 10 伤害，扣 10（对照证明仅 DAMAGES_HELMET 标签
//     伤害受头盔减伤，magic 不在标签内不减免——排除"头盔减伤对所有伤害生效"的过宽实现）。
//
// 测试数值依据（TestLivingEntity 无护甲值/无附魔/无药水，armor=0 toughness=0）：
//   戴头盔 fallingAnvil 10 → DAMAGES_HELMET ×0.75=7.5 → applyArmorCalculations(armor=0)不减
//   → applyPotionDamageCalculations(无附魔无药水)不减 → 扣 7.5。
//   无头盔 fallingAnvil 10 → 跳过 DAMAGES_HELMET → applyArmorCalculations(armor=0)不减 → 扣 10。
//   戴头盔 magic 10 → magic 不在 DAMAGES_HELMET 标签跳过 → applyArmorCalculations(magic bypassesArmor
//   跳过)不减 → applyPotionDamageCalculations(无附魔)不减 → 扣 10。
//
// 注：fallingAnvil 不 bypassesArmor（EnvironmentalDamage），但 TestLivingEntity armor 属性=0，
// getDamageAfterAbsorb(10, 0, 0)=10 不减；magic bypassesArmor=true 跳过护甲段。两者最终均不减，
// 仅 DAMAGES_HELMET 分支造成戴头盔差异。
//
// Ref: vanilla LivingEntity.java:1182-1185（hurtServer DAMAGES_HELMET 分支）
// Ref: vanilla LivingEntity.java:1793-1794（hurtHelmet 基类空实现）
// Ref: LivingEntity.cpp:310（actuallyHurt）/ :345-354（DAMAGES_HELMET 分支）/ :451（hurtHelmet 空实现）
// Ref: DamageTypeTags.cpp:811（DAMAGES_HELMET 成员 = {FallingAnvil, FallingBlock, FallingStalactite}）
// Ref: DamageSource.hpp:1010（fallingAnvil 工厂）/ :865（magic 工厂）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

// 测试用 LivingEntity 子类：参照 tests/entity/LivingEntityTests.cpp:140 与
// BypassesEnchantmentsTest.cpp 的 TestLivingEntity 范式。actuallyHurt 在 LivingEntity.hpp:207
// 为 public virtual，可直接调用，无需 using 暴露。走基类 EquipmentComponent 链路
// （LivingEntity 构造 attach EquipmentComponent），setEquipment/getEquipment 数据源一致。
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// BaseTestWorld 的默认构造函数为 protected（TestWorldHelper.hpp:158），无法作为测试 fixture
// 的成员字段或局部变量直接构造。此处派生并公开默认构造，使测试可在 SetUp 中创建实例。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

// ============================================================================
// DAMAGES_HELMET 测试
// ============================================================================

class DamagesHelmetTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 物品初始化使 ItemStack(Items::DIAMOND_HELMET) 能解析物品类型。
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级单例，s_initialized 守卫幂等）。DAMAGES_HELMET 标签成员集
        // （= {FallingAnvil, FallingBlock, FallingStalactite}）在 initialize() 中通过 addAll 注册
        // （DamageTypeTags.cpp:811）。未初始化时标签成员集为空，source.is(DAMAGES_HELMET) 恒返 false，
        // 修复分支形同虚设——这正是缺陷状态下戴头盔也全额伤害的根因。真实游戏启动时会调
        // initialize()，测试须模拟之。标签无 clear/reset，进程级常驻，初始化只会让其他测试更贴近
        // vanilla，不破坏。
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    // 给实体戴上钻石头盔（无附魔，本测试只关心 DAMAGES_HELMET ×0.75 减伤，与附魔无关）。
    void equipHelmet(TestLivingEntity& entity) const
    {
        ItemStack helmet(Items::DIAMOND_HELMET, 1);
        entity.setEquipment(EquipmentSlot::Head, helmet);
    }

    // actuallyHurt 全路径在 health>0、伤害源无 trueSource、实体 typeId 为空（makeSoundEventId 返
    // nullopt 使 playHurtSound 早退）时不访问 world；setWorld 仅作保险以对齐全链路上下文。
    TestWorld m_world;
};

// 戴头盔受坠落铁砧伤害，DAMAGES_HELMET 分支生效，伤害 ×0.75。
//
// TestLivingEntity（maxHealth=20）戴钻石头盔，受 fallingAnvil 10 点伤害：
// DAMAGES_HELMET ×0.75=7.5 → 护甲(armor=0)不减 → 药水(无附魔无药水)不减 → 扣 7.5，剩余 12.5。
// 修复前此处会扣 10（缺 DAMAGES_HELMET 分支，全额伤害），剩余 10。
TEST_F(DamagesHelmetTest, FallingAnvilReducedByHelmet)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    equipHelmet(entity);
    ASSERT_FALSE(entity.getEquipment(EquipmentSlot::Head).isEmpty());

    const f32 healthBefore = entity.health();
    auto source = DamageSources::fallingAnvil();

    entity.actuallyHurt(source, 10.0f);

    EXPECT_FLOAT_EQ(entity.health(), healthBefore - 7.5f);
}

// 对照：无头盔受坠落铁砧伤害，DAMAGES_HELMET 分支不触发（头盔槽 isEmpty），全额伤害。
//
// 同一实体不戴头盔，受 fallingAnvil 10 点伤害：跳过 DAMAGES_HELMET 分支 → 护甲(armor=0)不减
// → 药水不减 → 扣 10，剩余 10。若本测试失败（扣 0 或扣 7.5）说明伤害本身未生效或分支误触发，
// 则 FallingAnvilReducedByHelmet 的"扣 7.5"无法归因于头盔减伤（假通过）。
TEST_F(DamagesHelmetTest, FallingAnvilNotReducedWithoutHelmet)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    ASSERT_TRUE(entity.getEquipment(EquipmentSlot::Head).isEmpty());

    const f32 healthBefore = entity.health();
    auto source = DamageSources::fallingAnvil();

    entity.actuallyHurt(source, 10.0f);

    EXPECT_FLOAT_EQ(entity.health(), healthBefore - 10.0f);
}

// 对照：戴头盔受 magic 伤害，magic 不在 DAMAGES_HELMET 标签内，头盔不减伤。
//
// 同一戴头盔实体受 magic 10 点伤害：magic 不属于 DAMAGES_HELMET 跳过分支 → magic bypassesArmor
// 跳过护甲段 → 药水(无附魔)不减 → 扣 10，剩余 10。若本测试失败（扣 7.5）说明头盔减伤对所有
// 伤害类型过宽生效（误把非 DAMAGES_HELMET 伤害也 ×0.75），偏离 vanilla 仅 DAMAGES_HELMET 标签
// 伤害受头盔减伤的语义。
TEST_F(DamagesHelmetTest, MagicNotAffectedByHelmet)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    equipHelmet(entity);
    ASSERT_FALSE(entity.getEquipment(EquipmentSlot::Head).isEmpty());

    const f32 healthBefore = entity.health();
    auto source = DamageSources::magic();

    entity.actuallyHurt(source, 10.0f);

    EXPECT_FLOAT_EQ(entity.health(), healthBefore - 10.0f);
}
