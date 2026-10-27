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

// death_protection 数据组件测试。
//
// 覆盖：
//   - 不死图腾物品默认携带 death_protection 组件（对齐 vanilla Items.TOTEM_OF_UNDYING）
//   - 物品栈覆盖优先于物品默认值（覆盖定义完全替代默认效果列表）
//   - 持有任意带该组件的物品即可免死（不再以物品类型硬编码判定）
//   - ConsumeEffect 列表 JSON 往返（apply_effects / remove_effects / clear_all / teleport / play_sound）
//   - NBT 组件往返（toNbt → fromNbt）保留 death_effects

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <nlohmann/json.hpp>

using namespace mc;

namespace {

/// 测试玩家：注册属性并设置生命值
class DeathProtectionPlayer final : public Player {
public:
    DeathProtectionPlayer()
        : Player(EntityInstanceId(1), "DeathProtectionPlayer", mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(1.0f);
    }
};

/// 仅用于让 apply_effects 不按概率跳过的最小定义
item::component::DeathProtectionPayload simplePayload()
{
    item::component::ConsumeEffectEntry apply{};
    apply.kind = item::component::ConsumeEffectKind::ApplyEffects;
    apply.effects.push_back({entity::effect::EffectInstance(entity::effect::EffectType::Speed, 200, 0), 1.0f});
    item::component::DeathProtectionPayload payload;
    payload.effects.push_back(std::move(apply));
    return payload;
}

} // namespace

class DeathProtectionComponentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::ensureInitialized();
    }

    class TestWorld final : public mc::test::BaseTestWorld {
    public:
        TestWorld() = default;
    };

    TestWorld m_world;
};

// 不死图腾物品默认携带 death_protection 组件（2 条：清空效果 + 施加三种效果）。
TEST_F(DeathProtectionComponentTest, TotemCarriesDefaultComponent)
{
    ASSERT_NE(Items::TOTEM_OF_UNDYING, nullptr);
    ItemStack stack(*Items::TOTEM_OF_UNDYING, 1);

    const auto& component = stack.getDeathProtection();
    ASSERT_TRUE(component.has_value());
    ASSERT_EQ(component->effects.size(), 2u);
    EXPECT_EQ(component->effects[0].kind, item::component::ConsumeEffectKind::ClearAllEffects);
    EXPECT_EQ(component->effects[1].kind, item::component::ConsumeEffectKind::ApplyEffects);
    ASSERT_EQ(component->effects[1].effects.size(), 3u);
    EXPECT_EQ(component->effects[1].effects[0].instance->type(), entity::effect::EffectType::Regeneration);
    EXPECT_EQ(component->effects[1].effects[1].instance->type(), entity::effect::EffectType::Absorption);
    EXPECT_EQ(component->effects[1].effects[2].instance->type(), entity::effect::EffectType::FireResistance);
}

// 普通物品不含该组件。
TEST_F(DeathProtectionComponentTest, PlainItemHasNoComponent)
{
    ASSERT_NE(Items::STICK, nullptr);
    ItemStack stack(*Items::STICK, 1);
    EXPECT_FALSE(stack.getDeathProtection().has_value());
}

// 持有不死图腾受致死伤害：免死、图腾消耗、效果链路（清空 + 施加）执行。
TEST_F(DeathProtectionComponentTest, TotemProtectsThroughComponent)
{
    DeathProtectionPlayer player;
    player.setWorld(&m_world);
    player.getMutableMainHandItem() = ItemStack(*Items::TOTEM_OF_UNDYING, 1);
    // 先加一个会被 clear_all_effects 清掉的效果
    player.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 200));

    auto source = DamageSources::generic();
    player.hurt(source, 100.0f);

    EXPECT_FALSE(player.isDead());
    EXPECT_FLOAT_EQ(player.health(), 1.0f);
    EXPECT_TRUE(player.getMutableMainHandItem().isEmpty());
    EXPECT_FALSE(player.hasEffect(entity::effect::EffectType::Slowness)) << "clear_all_effects 应清空旧效果";
    EXPECT_TRUE(player.hasEffect(entity::effect::EffectType::Regeneration));
    EXPECT_TRUE(player.hasEffect(entity::effect::EffectType::Absorption));
    EXPECT_TRUE(player.hasEffect(entity::effect::EffectType::FireResistance));
}

// 物品栈覆盖可替换物品默认定义：任意物品带自定义组件即可免死。
TEST_F(DeathProtectionComponentTest, StackOverrideProtectsWithCustomItem)
{
    ASSERT_NE(Items::STICK, nullptr);
    DeathProtectionPlayer player;
    player.setWorld(&m_world);
    ItemStack stack(*Items::STICK, 1);
    stack.setDeathProtection(simplePayload());
    player.getMutableMainHandItem() = stack;

    auto source = DamageSources::generic();
    player.hurt(source, 100.0f);

    EXPECT_FALSE(player.isDead());
    EXPECT_FLOAT_EQ(player.health(), 1.0f);
    EXPECT_TRUE(player.getMutableMainHandItem().isEmpty());
    EXPECT_TRUE(player.hasEffect(entity::effect::EffectType::Speed));
}

// 覆盖为自定义条目：完全按覆盖定义执行，不叠加物品默认的效果列表。
TEST_F(DeathProtectionComponentTest, OverrideReplacesTotemDefaultEffectList)
{
    DeathProtectionPlayer player;
    player.setWorld(&m_world);
    ItemStack stack(*Items::TOTEM_OF_UNDYING, 1);
    item::component::ConsumeEffectEntry apply{};
    apply.kind = item::component::ConsumeEffectKind::ApplyEffects;
    apply.effects.push_back({entity::effect::EffectInstance(entity::effect::EffectType::Speed, 200, 0), 1.0f});
    item::component::DeathProtectionPayload payload;
    payload.effects.push_back(std::move(apply));
    stack.setDeathProtection(std::move(payload));
    player.getMutableMainHandItem() = stack;

    auto source = DamageSources::generic();
    player.hurt(source, 100.0f);

    EXPECT_FALSE(player.isDead());
    EXPECT_TRUE(player.hasEffect(entity::effect::EffectType::Speed));
    // 覆盖不含 clear_all / 图腾默认效果：不应出现在实体上
    EXPECT_FALSE(player.hasEffect(entity::effect::EffectType::Regeneration));
    EXPECT_FALSE(player.hasEffect(entity::effect::EffectType::Absorption));
}

// ConsumeEffect 列表 JSON 往返：五类条目均保留。
TEST_F(DeathProtectionComponentTest, ConsumeEffectsJsonRoundTrip)
{
    const auto json = nlohmann::json::parse(R"([
        {"type": "minecraft:clear_all_effects"},
        {"type": "minecraft:apply_effects", "effects": [
            {"id": "minecraft:regeneration", "duration": 900, "amplifier": 1},
            {"id": "minecraft:absorption", "duration": 100, "amplifier": 1}
        ]},
        {"type": "minecraft:remove_effects", "effects": ["minecraft:poison"]},
        {"type": "minecraft:teleport_randomly", "diameter": 16.0},
        {"type": "minecraft:play_sound", "sound": "minecraft:item.totem.use"}
    ])");

    const auto parsed = item::component::consumeEffectsFromJson(json);
    ASSERT_EQ(parsed.size(), 5u);
    EXPECT_EQ(parsed[0].kind, item::component::ConsumeEffectKind::ClearAllEffects);
    EXPECT_EQ(parsed[1].kind, item::component::ConsumeEffectKind::ApplyEffects);
    ASSERT_EQ(parsed[1].effects.size(), 2u);
    ASSERT_TRUE(parsed[1].effects[0].instance.has_value());
    EXPECT_EQ(parsed[1].effects[0].instance->type(), entity::effect::EffectType::Regeneration);
    EXPECT_EQ(parsed[1].effects[0].instance->duration(), 900);
    EXPECT_EQ(parsed[1].effects[0].instance->amplifier(), 1);
    EXPECT_EQ(parsed[2].kind, item::component::ConsumeEffectKind::RemoveEffects);
    ASSERT_EQ(parsed[2].removedEffects.size(), 1u);
    EXPECT_EQ(parsed[2].removedEffects[0], entity::effect::EffectType::Poison);
    EXPECT_EQ(parsed[3].kind, item::component::ConsumeEffectKind::TeleportRandomly);
    EXPECT_FLOAT_EQ(parsed[3].teleportDiameter, 16.0f);
    EXPECT_EQ(parsed[4].kind, item::component::ConsumeEffectKind::PlaySound);
    EXPECT_EQ(parsed[4].soundId, "minecraft:item.totem.use");

    const auto reencoded = item::component::consumeEffectsToJson(parsed);
    const auto reparsed = item::component::consumeEffectsFromJson(reencoded);
    ASSERT_EQ(reparsed.size(), parsed.size());
    for (usize i = 0; i < reparsed.size(); ++i) {
        EXPECT_EQ(reparsed[i].kind, parsed[i].kind);
    }
    EXPECT_EQ(reparsed[2].removedEffects, parsed[2].removedEffects);
    EXPECT_EQ(reparsed[4].soundId, parsed[4].soundId);
}

// 未知 ConsumeEffect 类型被丢弃（对齐 vanilla 未知 type 解码失败）。
TEST_F(DeathProtectionComponentTest, UnknownConsumeEffectTypeDropped)
{
    const auto json = nlohmann::json::parse(R"([
        {"type": "minecraft:clear_all_effects"},
        {"type": "minecraft:unknown_effect"}
    ])");
    const auto parsed = item::component::consumeEffectsFromJson(json);
    ASSERT_EQ(parsed.size(), 1u);
    EXPECT_EQ(parsed[0].kind, item::component::ConsumeEffectKind::ClearAllEffects);
}

// NBT 组件往返：death_effects 结构在 toNbt → fromNbt 后保留。
TEST_F(DeathProtectionComponentTest, NbtRoundTripPreservesDeathEffects)
{
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    stack.setDeathProtection(simplePayload());

    nbt::tags::compound_tag tag;
    stack.toNbt(tag);
    auto restored = ItemStack::fromNbt(tag);
    ASSERT_TRUE(restored.success());

    const auto& component = restored.value().getDeathProtection();
    ASSERT_TRUE(component.has_value());
    ASSERT_EQ(component->effects.size(), 1u);
    ASSERT_EQ(component->effects[0].effects.size(), 1u);
    ASSERT_TRUE(component->effects[0].effects[0].instance.has_value());
    EXPECT_EQ(component->effects[0].effects[0].instance->type(), entity::effect::EffectType::Speed);
    EXPECT_EQ(component->effects[0].effects[0].instance->duration(), 200);
}
