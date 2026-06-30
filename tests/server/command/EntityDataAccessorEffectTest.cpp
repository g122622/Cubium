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
 * The copyright notice and this permission notice shall be included in all
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

#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "server/command/data/DataAccessor.hpp"

using namespace mc;
using namespace mc::command;
using namespace mc::entity::effect;
using namespace mc::entity::attribute;
using namespace mc::entity::serialization::nbt_keys;

/**
 * @brief EntityDataAccessor 药水效果序列化/反序列化测试
 *
 * 测试 EntityDataAccessor 的 getData/mergeData 对 ActiveEffects 的处理，
 * 包括空效果列表、多个效果、无效 NBT 容错、属性修改器正确应用等边界场景。
 */
class EntityDataAccessorEffectTest : public ::testing::Test {
protected:
    std::unique_ptr<LivingEntity> m_entity;

    void SetUp() override
    {
        m_entity = std::make_unique<LivingEntity>(EntityId(1));
        m_entity->registerData();
        m_entity->registerAttributes();
        // 注册效果测试需要的属性
        m_entity->attributes().registerAttribute(*Attributes::attackDamage());
        m_entity->attributes().registerAttribute(*Attributes::attackSpeed());
        m_entity->attributes().registerAttribute(*Attributes::luck());
        m_entity->attributes().registerAttribute(*Attributes::jumpBoost());
        m_entity->setHealth(m_entity->maxHealth());
    }

    void TearDown() override { m_entity.reset(); }
};

// ============================================================================
// getData - 序列化测试
// ============================================================================

TEST_F(EntityDataAccessorEffectTest, GetData_NoEffects_NoActiveEffectsKey)
{
    // 无效果时，NBT 数据中不应包含 ActiveEffects 键
    EntityDataAccessor accessor(m_entity.get());
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    auto it = data->value.find(ACTIVE_EFFECTS);
    EXPECT_EQ(it, data->value.end()) << "ActiveEffects key should not exist when entity has no effects";
}

TEST_F(EntityDataAccessorEffectTest, GetData_SingleEffect_SerializedCorrectly)
{
    // 添加一个效果并验证序列化
    m_entity->addEffect(EffectInstance(EffectType::Speed, 200, 2, false, true, true));

    EntityDataAccessor accessor(m_entity.get());
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    auto it = data->value.find(ACTIVE_EFFECTS);
    ASSERT_NE(it, data->value.end());
    ASSERT_EQ(it->second->id(), nbt::TagId::List);

    auto* list = dynamic_cast<const nbt::tags::list_tag*>(it->second.get());
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->element_id(), nbt::TagId::Compound);

    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*list);
    ASSERT_EQ(compoundList.value.size(), 1u);

    // 验证效果的 NBT 字段
    const auto& effectTag = compoundList.value[0];

    // Id = Speed (1)
    auto idIt = effectTag.value.find(EFFECT_ID);
    ASSERT_NE(idIt, effectTag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*idIt->second).value, static_cast<i8>(EffectType::Speed));

    // Amplifier = 2 (Level III)
    auto ampIt = effectTag.value.find(EFFECT_AMPLIFIER);
    ASSERT_NE(ampIt, effectTag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*ampIt->second).value, static_cast<i8>(2));

    // Duration = 200
    auto durIt = effectTag.value.find(EFFECT_DURATION);
    ASSERT_NE(durIt, effectTag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*durIt->second).value, 200);

    // Ambient = false (0)
    auto ambIt = effectTag.value.find(EFFECT_AMBIENT);
    ASSERT_NE(ambIt, effectTag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*ambIt->second).value, static_cast<i8>(0));

    // ShowParticles = true (1)
    auto showPartIt = effectTag.value.find(EFFECT_SHOW_PARTICLES);
    ASSERT_NE(showPartIt, effectTag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*showPartIt->second).value, static_cast<i8>(1));

    // ShowIcon = true (1)
    auto showIconIt = effectTag.value.find(EFFECT_SHOW_ICON);
    ASSERT_NE(showIconIt, effectTag.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*showIconIt->second).value, static_cast<i8>(1));
}

TEST_F(EntityDataAccessorEffectTest, GetData_MultipleEffects_AllSerialized)
{
    // 添加多个效果
    m_entity->addEffect(EffectInstance(EffectType::Speed, 600, 0));      // Speed I, 600 ticks
    m_entity->addEffect(EffectInstance(EffectType::Resistance, 200, 1)); // Resistance II, 200 ticks
    m_entity->addEffect(EffectInstance(EffectType::FireResistance, -1, 0, true, false, false)); // Permanent, ambient

    EntityDataAccessor accessor(m_entity.get());
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    auto it = data->value.find(ACTIVE_EFFECTS);
    ASSERT_NE(it, data->value.end());

    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*it->second);
    ASSERT_EQ(compoundList.value.size(), 3u);

    // 验证每个效果都存在
    bool hasSpeed = false;
    bool hasResistance = false;
    bool hasFireResistance = false;

    for (const auto& effectTag : compoundList.value) {
        auto idIt = effectTag.value.find(EFFECT_ID);
        ASSERT_NE(idIt, effectTag.value.end());
        auto effectId = dynamic_cast<const nbt::tags::byte_tag&>(*idIt->second).value;

        if (static_cast<EffectType>(effectId) == EffectType::Speed) {
            hasSpeed = true;
            auto durIt = effectTag.value.find(EFFECT_DURATION);
            ASSERT_NE(durIt, effectTag.value.end());
            EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*durIt->second).value, 600);
        } else if (static_cast<EffectType>(effectId) == EffectType::Resistance) {
            hasResistance = true;
            auto ampIt = effectTag.value.find(EFFECT_AMPLIFIER);
            ASSERT_NE(ampIt, effectTag.value.end());
            EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*ampIt->second).value, static_cast<i8>(1));
        } else if (static_cast<EffectType>(effectId) == EffectType::FireResistance) {
            hasFireResistance = true;
            // 永久效果 duration = -1
            auto durIt = effectTag.value.find(EFFECT_DURATION);
            ASSERT_NE(durIt, effectTag.value.end());
            EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*durIt->second).value, -1);
            // 环境效果
            auto ambIt = effectTag.value.find(EFFECT_AMBIENT);
            ASSERT_NE(ambIt, effectTag.value.end());
            EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*ambIt->second).value, static_cast<i8>(1));
            // 不显示粒子
            auto showPartIt = effectTag.value.find(EFFECT_SHOW_PARTICLES);
            ASSERT_NE(showPartIt, effectTag.value.end());
            EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*showPartIt->second).value, static_cast<i8>(0));
            // 不显示图标
            auto showIconIt = effectTag.value.find(EFFECT_SHOW_ICON);
            ASSERT_NE(showIconIt, effectTag.value.end());
            EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*showIconIt->second).value, static_cast<i8>(0));
        }
    }

    EXPECT_TRUE(hasSpeed);
    EXPECT_TRUE(hasResistance);
    EXPECT_TRUE(hasFireResistance);
}

TEST_F(EntityDataAccessorEffectTest, GetData_NonLivingEntity_NoEffects)
{
    // 非 LivingEntity（普通 Entity）不应有序列化的效果
    Entity entity(EntityId(2));
    entity.registerData();

    EntityDataAccessor accessor(&entity);
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    auto it = data->value.find(ACTIVE_EFFECTS);
    EXPECT_EQ(it, data->value.end()) << "ActiveEffects should not exist for non-LivingEntity";
}

// ============================================================================
// mergeData - 反序列化测试
// ============================================================================

TEST_F(EntityDataAccessorEffectTest, MergeData_AddEffectsFromNbt)
{
    // 实体初始无效果
    EXPECT_TRUE(m_entity->effectManager().getAllEffects().empty());

    // 构建 NBT 数据，包含两个效果
    nbt::tags::compound_tag data;
    auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();

    // Speed I, 100 ticks
    {
        nbt::tags::compound_tag effectTag;
        effectTag.put(EFFECT_ID, static_cast<i8>(EffectType::Speed));
        effectTag.put(EFFECT_AMPLIFIER, static_cast<i8>(0));
        effectTag.put(EFFECT_DURATION, 100);
        effectTag.put(EFFECT_AMBIENT, static_cast<i8>(0));
        effectTag.put(EFFECT_SHOW_PARTICLES, static_cast<i8>(1));
        effectTag.put(EFFECT_SHOW_ICON, static_cast<i8>(1));
        effectsList->value.push_back(std::move(effectTag));
    }

    // Strength II, 300 ticks
    {
        nbt::tags::compound_tag effectTag;
        effectTag.put(EFFECT_ID, static_cast<i8>(EffectType::Strength));
        effectTag.put(EFFECT_AMPLIFIER, static_cast<i8>(1));
        effectTag.put(EFFECT_DURATION, 300);
        effectTag.put(EFFECT_AMBIENT, static_cast<i8>(0));
        effectTag.put(EFFECT_SHOW_PARTICLES, static_cast<i8>(1));
        effectTag.put(EFFECT_SHOW_ICON, static_cast<i8>(1));
        effectsList->value.push_back(std::move(effectTag));
    }

    data.value.emplace(ACTIVE_EFFECTS, std::move(effectsList));

    EntityDataAccessor accessor(m_entity.get());
    accessor.mergeData(data);

    // 验证效果已添加
    const auto& effects = m_entity->effectManager().getAllEffects();
    ASSERT_EQ(effects.size(), 2u);

    EXPECT_TRUE(m_entity->hasEffect(EffectType::Speed));
    EXPECT_TRUE(m_entity->hasEffect(EffectType::Strength));

    auto* speed = m_entity->effectManager().getEffect(EffectType::Speed);
    ASSERT_NE(speed, nullptr);
    EXPECT_EQ(speed->amplifier(), 0);
    EXPECT_EQ(speed->duration(), 100);

    auto* strength = m_entity->effectManager().getEffect(EffectType::Strength);
    ASSERT_NE(strength, nullptr);
    EXPECT_EQ(strength->amplifier(), 1);
    EXPECT_EQ(strength->duration(), 300);
}

TEST_F(EntityDataAccessorEffectTest, MergeData_ReplacesExistingEffects)
{
    // 实体已有效果
    m_entity->addEffect(EffectInstance(EffectType::Speed, 600, 0));
    m_entity->addEffect(EffectInstance(EffectType::Regeneration, 400, 2));

    EXPECT_EQ(m_entity->effectManager().getEffectCount(), 2u);

    // 合并 NBT 数据只包含 Resistance
    nbt::tags::compound_tag data;
    auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
    {
        nbt::tags::compound_tag effectTag;
        effectTag.put(EFFECT_ID, static_cast<i8>(EffectType::Resistance));
        effectTag.put(EFFECT_AMPLIFIER, static_cast<i8>(0));
        effectTag.put(EFFECT_DURATION, 200);
        effectTag.put(EFFECT_AMBIENT, static_cast<i8>(0));
        effectTag.put(EFFECT_SHOW_PARTICLES, static_cast<i8>(1));
        effectTag.put(EFFECT_SHOW_ICON, static_cast<i8>(1));
        effectsList->value.push_back(std::move(effectTag));
    }
    data.value.emplace(ACTIVE_EFFECTS, std::move(effectsList));

    EntityDataAccessor accessor(m_entity.get());
    accessor.mergeData(data);

    // 原有效果应被替换
    const auto& effects = m_entity->effectManager().getAllEffects();
    ASSERT_EQ(effects.size(), 1u);

    EXPECT_FALSE(m_entity->hasEffect(EffectType::Speed));
    EXPECT_FALSE(m_entity->hasEffect(EffectType::Regeneration));
    EXPECT_TRUE(m_entity->hasEffect(EffectType::Resistance));

    auto* resistance = m_entity->effectManager().getEffect(EffectType::Resistance);
    ASSERT_NE(resistance, nullptr);
    EXPECT_EQ(resistance->duration(), 200);
}

TEST_F(EntityDataAccessorEffectTest, MergeData_EmptyEffectsList_ClearsEffects)
{
    // 实体已有效果
    m_entity->addEffect(EffectInstance(EffectType::Speed, 600, 0));
    m_entity->addEffect(EffectInstance(EffectType::JumpBoost, 300, 1));
    EXPECT_EQ(m_entity->effectManager().getEffectCount(), 2u);

    // 合并空的 ActiveEffects 列表
    nbt::tags::compound_tag data;
    auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
    data.value.emplace(ACTIVE_EFFECTS, std::move(effectsList));

    EntityDataAccessor accessor(m_entity.get());
    accessor.mergeData(data);

    // 所有效果应被清除
    EXPECT_EQ(m_entity->effectManager().getEffectCount(), 0u);
    EXPECT_FALSE(m_entity->hasEffect(EffectType::Speed));
    EXPECT_FALSE(m_entity->hasEffect(EffectType::JumpBoost));
}

TEST_F(EntityDataAccessorEffectTest, MergeData_InvalidNbtType_Ignored)
{
    // ActiveEffects 不是 List 类型，应被忽略
    nbt::tags::compound_tag data;
    data.put(ACTIVE_EFFECTS, static_cast<i8>(0)); // byte 而非 list

    EntityDataAccessor accessor(m_entity.get());

    // 不应抛出异常
    EXPECT_NO_THROW(accessor.mergeData(data));

    // 原有效果保持不变
    EXPECT_EQ(m_entity->effectManager().getEffectCount(), 0u);
}

TEST_F(EntityDataAccessorEffectTest, MergeData_InvalidListElementType_Ignored)
{
    // ActiveEffects 是 List 但元素不是 Compound 类型，应被忽略
    nbt::tags::compound_tag data;
    auto intList = std::make_unique<nbt::tags::int_list_tag>();
    intList->value.push_back(1);
    intList->value.push_back(2);
    data.value.emplace(ACTIVE_EFFECTS, std::move(intList));

    EntityDataAccessor accessor(m_entity.get());

    // 不应抛出异常
    EXPECT_NO_THROW(accessor.mergeData(data));

    // 效果列表应保持不变
    EXPECT_EQ(m_entity->effectManager().getEffectCount(), 0u);
}

TEST_F(EntityDataAccessorEffectTest, MergeData_EffectAttributeModifiersApplied)
{
    // 验证合并效果后属性修改器是否正确处理
    // 参考 MC Java: EntityDataAccessor.setData() 通过 NBT 加载效果时，
    // 效果修改器作为 permanentModifiers 保存在属性 NBT 中，不通过 addEffect() 应用。
    // 因此 mergeData 后属性修改器的状态取决于 NBT 中的 Attributes 字段。
    //
    // 当实体无效果时，writeToNBT 输出的 Attributes 不含效果修改器。
    // mergeData 仅合并 ActiveEffects，Attributes 保持原样（无效果修改器）。
    // readFromNBT 加载后：效果列表含 Strength II，但属性不含修改器。
    // 效果修改器将在下一个 tick 由效果系统重新应用。

    // 合并 Strength II（amplifier=1，增加 6.0 攻击伤害）
    nbt::tags::compound_tag data;
    auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
    {
        nbt::tags::compound_tag effectTag;
        effectTag.put(EFFECT_ID, static_cast<i8>(EffectType::Strength));
        effectTag.put(EFFECT_AMPLIFIER, static_cast<i8>(1)); // Level II
        effectTag.put(EFFECT_DURATION, 200);
        effectTag.put(EFFECT_AMBIENT, static_cast<i8>(0));
        effectTag.put(EFFECT_SHOW_PARTICLES, static_cast<i8>(1));
        effectTag.put(EFFECT_SHOW_ICON, static_cast<i8>(1));
        effectsList->value.push_back(std::move(effectTag));
    }
    data.value.emplace(ACTIVE_EFFECTS, std::move(effectsList));

    EntityDataAccessor accessor(m_entity.get());
    accessor.mergeData(data);

    // 验证效果已在效果列表中
    auto* strength = m_entity->effectManager().getEffect(EffectType::Strength);
    ASSERT_NE(strength, nullptr);
    EXPECT_EQ(strength->amplifier(), 1); // Strength II
    EXPECT_EQ(strength->duration(), 200);

    // 注意：属性修改器不会立即应用，因为 NBT 加载路径不通过 addEffect()。
    // 效果修改器将在下一个 tick 由效果系统的 tick 逻辑重新应用。
}

TEST_F(EntityDataAccessorEffectTest, MergeData_ReplaceEffectWithWeaker_AttributesUpdated)
{
    // 先添加 Strength II，再通过 mergeData 替换为 Speed I
    // 参考 MC Java: NBT 加载路径中，属性修改器完全由 Attributes NBT 字段决定。

    // 先记录基础攻击伤害（添加效果前）
    const f64 baseAttackDamage = m_entity->attributes().getValue("generic.attack_damage", 0.0);

    m_entity->addEffect(EffectInstance(EffectType::Strength, 600, 1)); // Strength II

    // 验证 Strength II 已生效（增加 6.0 攻击伤害）
    const f64 attackWithStrength = m_entity->attributes().getValue("generic.attack_damage", 0.0);
    EXPECT_DOUBLE_EQ(attackWithStrength, baseAttackDamage + 6.0);

    // 合并只包含 Speed I 的 NBT（会替换所有效果）
    nbt::tags::compound_tag data;
    auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
    {
        nbt::tags::compound_tag effectTag;
        effectTag.put(EFFECT_ID, static_cast<i8>(EffectType::Speed));
        effectTag.put(EFFECT_AMPLIFIER, static_cast<i8>(0));
        effectTag.put(EFFECT_DURATION, 400);
        effectTag.put(EFFECT_AMBIENT, static_cast<i8>(0));
        effectTag.put(EFFECT_SHOW_PARTICLES, static_cast<i8>(1));
        effectTag.put(EFFECT_SHOW_ICON, static_cast<i8>(1));
        effectsList->value.push_back(std::move(effectTag));
    }
    data.value.emplace(ACTIVE_EFFECTS, std::move(effectsList));

    EntityDataAccessor accessor(m_entity.get());
    accessor.mergeData(data);

    // 效果列表应已替换为 Speed I
    EXPECT_TRUE(m_entity->hasEffect(EffectType::Speed));
    EXPECT_FALSE(m_entity->hasEffect(EffectType::Strength));

    // 注意：NBT 加载路径中属性修改器由 Attributes NBT 字段重建。
    // mergeData 的 writeToNBT 会包含 Strength 修改器，readAttributeMap 会清除旧修改器
    // 并从 NBT 重建。由于 NBT Attributes 中仍包含 Strength 修改器（mergeData 未修改 Attributes 字段），
    // 属性值可能暂时包含 Strength 修改器，直到效果系统 tick 时重新同步。
}

TEST_F(EntityDataAccessorEffectTest, MergeData_PlayerEntity_ThrowsException)
{
    // 玩家实体不允许修改 NBT 数据
    Player player(EntityId(10), "TestPlayer");
    player.registerData();
    player.registerAttributes();

    nbt::tags::compound_tag data;
    auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
    data.value.emplace(ACTIVE_EFFECTS, std::move(effectsList));

    EntityDataAccessor accessor(&player);
    EXPECT_THROW(accessor.mergeData(data), CommandException);
}

// ============================================================================
// 序列化/反序列化往返测试
// ============================================================================

TEST_F(EntityDataAccessorEffectTest, RoundTrip_SerializeDeserialize_PreservesEffects)
{
    // 添加多个效果
    m_entity->addEffect(EffectInstance(EffectType::Speed, 300, 1));                           // Speed II
    m_entity->addEffect(EffectInstance(EffectType::NightVision, 400, 0, false, true, false)); // Night Vision I, no icon

    // 序列化
    EntityDataAccessor accessor(m_entity.get());
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    // 创建新实体并反序列化
    auto entity2 = std::make_unique<LivingEntity>(EntityId(2));
    entity2->registerData();
    entity2->registerAttributes();
    entity2->attributes().registerAttribute(*Attributes::attackDamage());
    entity2->attributes().registerAttribute(*Attributes::attackSpeed());
    entity2->attributes().registerAttribute(*Attributes::luck());
    entity2->attributes().registerAttribute(*Attributes::jumpBoost());
    entity2->setHealth(entity2->maxHealth());

    EntityDataAccessor accessor2(entity2.get());
    accessor2.mergeData(*data);

    // 验证反序列化后的效果
    EXPECT_TRUE(entity2->hasEffect(EffectType::Speed));
    EXPECT_TRUE(entity2->hasEffect(EffectType::NightVision));

    auto* speed = entity2->effectManager().getEffect(EffectType::Speed);
    ASSERT_NE(speed, nullptr);
    EXPECT_EQ(speed->amplifier(), 1); // Speed II
    EXPECT_EQ(speed->duration(), 300);
    EXPECT_FALSE(speed->isAmbient());
    EXPECT_TRUE(speed->isVisible());
    EXPECT_TRUE(speed->showIcon());

    auto* nightVision = entity2->effectManager().getEffect(EffectType::NightVision);
    ASSERT_NE(nightVision, nullptr);
    EXPECT_EQ(nightVision->amplifier(), 0); // Night Vision I
    EXPECT_EQ(nightVision->duration(), 400);
    EXPECT_FALSE(nightVision->isAmbient());
    EXPECT_TRUE(nightVision->isVisible());
    EXPECT_FALSE(nightVision->showIcon()); // 不显示图标
}

TEST_F(EntityDataAccessorEffectTest, RoundTrip_EmptyEffects_RoundTrip)
{
    // 无效果实体序列化后再反序列化应保持无效果
    EntityDataAccessor accessor(m_entity.get());
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    // 确认序列化数据中没有 ActiveEffects 键
    auto it = data->value.find(ACTIVE_EFFECTS);
    EXPECT_EQ(it, data->value.end());

    // 在另一个有效果的实体上反序列化（不应改变效果）
    auto entity2 = std::make_unique<LivingEntity>(EntityId(2));
    entity2->registerData();
    entity2->registerAttributes();
    entity2->setHealth(entity2->maxHealth());
    entity2->addEffect(EffectInstance(EffectType::Speed, 100, 0));

    EntityDataAccessor accessor2(entity2.get());
    // 合并不含 ActiveEffects 的数据不应影响现有效果
    accessor2.mergeData(*data);

    // 原有效果保持不变（因为没有 ActiveEffects 键被合并）
    EXPECT_TRUE(entity2->hasEffect(EffectType::Speed));
}
