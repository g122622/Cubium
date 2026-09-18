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

// 爆炸保护附魔 EXPLOSION_KNOCKBACK_RESISTANCE 属性修饰符确定性单元测试。
//
// 背景：爆炸保护附魔经 enchantment.blast_protection 修饰符（Op0 ADD_VALUE，每级 +0.15）增加
// EXPLOSION_KNOCKBACK_RESISTANCE 属性。vanilla blast_protection.json 中 4 个盔甲槽位共享同一
// modifier id（"minecraft:enchantment.blast_protection"），vanilla AttributeInstance 按 id 去重
// （Map<UUID,Modifier>），故全套 IV 抗性 = 0.6（单条），击退力度衰减为 40%。
//
// Cubium AttributeInstance 用 vector+push_back（不去重），但 EnchantmentHelper::
// applyEnchantmentAttributeModifiers 每 slot add 前调 removeModifier(id)（find_if 删第一条同 id）。
// 4 槽位顺序处理（Feet→Legs→Chest→Head）时，后一槽位的 remove 会删前一槽位刚加的同 id 修饰符，
// 最终 m_modifiers 只剩 1 条 amount=0.6——巧合复现 vanilla 去重语义。
//
// 本测试确定性测量真实抗性值（无 AI/时序非确定性），验证：
//   1. 单件爆炸保护 IV（任一盔甲槽）→ 抗性 0.6（4×0.15）。
//   2. 全套 4 件爆炸保护 IV → 抗性应 ≤ 0.6（vanilla 去重语义，非 2.4 叠加）。
//   3. 无爆炸保护 → 抗性 0.0。
// 集成测试 BlastProtectionKnockbackTests 的 flaky 根因之一即数值假设错误（曾误以为全套抗性=1.0
// 击退×0），本单元测试以确定数值锚定真实行为。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::entity::attribute;
using namespace mc::item::enchant;

namespace {

// 测试用 mock 世界（同 EquipmentUpdateTest 范式）。
class BlastProtectionAttributeTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BlastProtectionAttributeTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BlastProtectionAttributeTestWorld::tickManager not implemented");
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void broadcastEntityStatus(EntityInstanceId, u8) override {}
};

// 测试用 LivingEntity：registerAttributes 注册 EXPLOSION_KNOCKBACK_RESISTANCE（LivingEntity 基类已注册）。
class BlastProtectionTestEntity : public LivingEntity {
public:
    BlastProtectionTestEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerData();
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // namespace

class BlastProtectionAttributeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            EnchantmentRegistry::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<BlastProtectionAttributeTestWorld>();
        m_entity = std::make_unique<BlastProtectionTestEntity>();
        m_entity->setWorld(m_world.get());
        // 首帧初始化装备快照（全空）。
        m_entity->detectEquipmentUpdates();
    }

    void TearDown() override
    {
        m_entity.reset();
        m_world.reset();
    }

    // 构造一件挂爆炸保护 IV 的钻石盔甲。
    ItemStack makeBlastProtectionArmor(const Item* item) const
    {
        ItemStack stack(item, 1);
        stack.addEnchantment("minecraft:blast_protection", 4);
        return stack;
    }

    std::unique_ptr<BlastProtectionAttributeTestWorld> m_world;
    std::unique_ptr<BlastProtectionTestEntity> m_entity;
};

// 无爆炸保护装备时，EXPLOSION_KNOCKBACK_RESISTANCE 应为默认 0.0。
TEST_F(BlastProtectionAttributeTest, NoBlastProtection_ZeroResistance)
{
    EXPECT_DOUBLE_EQ(m_entity->getAttributeValue(Attributes::EXPLOSION_KNOCKBACK_RESISTANCE, 0.0), 0.0);
}

// 单件爆炸保护 IV 钻石头盔（Head 槽）应提供抗性 0.6（4 级 × 0.15）。
TEST_F(BlastProtectionAttributeTest, SingleBlastProtectionIvHelmet_Resistance06)
{
    ASSERT_NE(Items::DIAMOND_HELMET, nullptr);
    m_entity->setEquipment(EquipmentSlot::Head, makeBlastProtectionArmor(Items::DIAMOND_HELMET));
    m_entity->detectEquipmentUpdates();

    // 单件 IV：0.0 + 4×0.15 = 0.6。
    EXPECT_DOUBLE_EQ(m_entity->getAttributeValue(Attributes::EXPLOSION_KNOCKBACK_RESISTANCE, 0.0), 0.6);
}

// 全套 4 件爆炸保护 IV 钻石甲：vanilla 按 modifier id 去重，抗性应仍为 0.6（非 2.4 叠加）。
// 此测试锚定 Cubium 当前行为（remove-first+add 在 4 同 id 槽位下复现去重），并暴露与 vanilla
// 语义是否真正一致。若 Cubium 叠加成 2.4（封顶 1.0），本测试 FAIL，提示 AttributeInstance
// 需改为按 id 去重以对齐 vanilla。
TEST_F(BlastProtectionAttributeTest, FullBlastProtectionIvSet_ResistanceDedupedTo06)
{
    ASSERT_NE(Items::DIAMOND_HELMET, nullptr);
    ASSERT_NE(Items::DIAMOND_CHESTPLATE, nullptr);
    ASSERT_NE(Items::DIAMOND_LEGGINGS, nullptr);
    ASSERT_NE(Items::DIAMOND_BOOTS, nullptr);

    m_entity->setEquipment(EquipmentSlot::Head, makeBlastProtectionArmor(Items::DIAMOND_HELMET));
    m_entity->setEquipment(EquipmentSlot::Chest, makeBlastProtectionArmor(Items::DIAMOND_CHESTPLATE));
    m_entity->setEquipment(EquipmentSlot::Legs, makeBlastProtectionArmor(Items::DIAMOND_LEGGINGS));
    m_entity->setEquipment(EquipmentSlot::Feet, makeBlastProtectionArmor(Items::DIAMOND_BOOTS));
    m_entity->detectEquipmentUpdates();

    const f64 resistance = m_entity->getAttributeValue(Attributes::EXPLOSION_KNOCKBACK_RESISTANCE, 0.0);
    // vanilla 语义：4 同 id 修饰符去重，全套抗性 = 0.6（与单件相同）。
    // 若此处为 1.0（2.4 封顶），说明 Cubium 叠加了多件同 id 修饰符，偏离 vanilla 去重语义。
    EXPECT_DOUBLE_EQ(resistance, 0.6)
        << "Full blast protection IV set should dedupe same-id modifiers to 0.6 (vanilla Map<id> semantics), "
        << "got " << resistance << " (1.0 would indicate vector stacking bug)";
}

// 卸下全套后抗性应回到 0.0（验证修饰符正确移除，无残留）。
TEST_F(BlastProtectionAttributeTest, FullSetUnequipped_ResistanceReturnsToZero)
{
    ASSERT_NE(Items::DIAMOND_HELMET, nullptr);
    ASSERT_NE(Items::DIAMOND_CHESTPLATE, nullptr);
    ASSERT_NE(Items::DIAMOND_LEGGINGS, nullptr);
    ASSERT_NE(Items::DIAMOND_BOOTS, nullptr);

    m_entity->setEquipment(EquipmentSlot::Head, makeBlastProtectionArmor(Items::DIAMOND_HELMET));
    m_entity->setEquipment(EquipmentSlot::Chest, makeBlastProtectionArmor(Items::DIAMOND_CHESTPLATE));
    m_entity->setEquipment(EquipmentSlot::Legs, makeBlastProtectionArmor(Items::DIAMOND_LEGGINGS));
    m_entity->setEquipment(EquipmentSlot::Feet, makeBlastProtectionArmor(Items::DIAMOND_BOOTS));
    m_entity->detectEquipmentUpdates();

    EXPECT_GT(m_entity->getAttributeValue(Attributes::EXPLOSION_KNOCKBACK_RESISTANCE, 0.0), 0.0);

    // 全部卸下。
    m_entity->setEquipment(EquipmentSlot::Head, ItemStack());
    m_entity->setEquipment(EquipmentSlot::Chest, ItemStack());
    m_entity->setEquipment(EquipmentSlot::Legs, ItemStack());
    m_entity->setEquipment(EquipmentSlot::Feet, ItemStack());
    m_entity->detectEquipmentUpdates();

    EXPECT_DOUBLE_EQ(m_entity->getAttributeValue(Attributes::EXPLOSION_KNOCKBACK_RESISTANCE, 0.0), 0.0)
        << "Resistance should return to 0.0 after unequipping all blast protection armor";
}
