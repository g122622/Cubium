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

#include "common/TestWorldHelper.hpp"
#include "core/Constants.hpp"
#include "entity/effect/EffectType.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/loot/context/LootContext.hpp"
#include "item/loot/functions/LootFunctions.hpp"
#include "resource/ResourceLocation.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::loot;
using namespace mc::entity::effect;

// Test implementation of IWorld for loot testing
class SetStewEffectTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SetStewEffectTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SetStewEffectTestWorld::tickManager not implemented");
    }
};

/**
 * @brief SetStewEffectFunction 测试
 *
 * 测试谜之炖菜效果设置功能
 * 参考: MC 1.16.5 net.minecraft.loot.functions.SetStewEffect
 */
class SetStewEffectTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_random = std::make_unique<math::Random>(12345);
    }

    void TearDown() override { m_random.reset(); }

    SetStewEffectTestWorld m_world;
    std::unique_ptr<math::Random> m_random;
};

// ============================================================================
// 基本功能测试
// ============================================================================

TEST_F(SetStewEffectTest, EmptyEffects_NoChange)
{
    // 空效果列表不应该修改物品
    SetStewEffectFunction func;
    ItemStack stack(Items::SUSPICIOUS_STEW, 1);
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    EXPECT_FALSE(result.hasTag());
}

TEST_F(SetStewEffectTest, EmptyStack_NoChange)
{
    // 空物品堆应该返回空
    SetStewEffectFunction func;
    func.addEffect("minecraft:poison", RandomValueRange(5.0f, 10.0f));
    ItemStack stack; // 空堆
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(SetStewEffectTest, WrongItemType_NoChange)
{
    // 对非谜之炖菜物品不应该添加效果
    SetStewEffectFunction func;
    func.addEffect("minecraft:poison", RandomValueRange(5.0f, 10.0f));
    ItemStack stack(Items::APPLE, 1); // 不是谜之炖菜
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    EXPECT_FALSE(result.hasTag());
}

TEST_F(SetStewEffectTest, SingleEffect_AppliedToStew)
{
    // 测试单个效果应用到谜之炖菜
    SetStewEffectFunction func;
    func.addEffect("minecraft:poison", RandomValueRange(5.0f)); // 5秒 = 100 ticks
    ItemStack stack(Items::SUSPICIOUS_STEW, 1);
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    ASSERT_TRUE(result.hasTag());

    const nlohmann::json* tag = result.getTag();
    ASSERT_TRUE(tag->contains("Effects"));
    ASSERT_TRUE((*tag)["Effects"].is_array());
    ASSERT_EQ(1, (*tag)["Effects"].size());

    // 检查效果
    const auto& effect = (*tag)["Effects"][0];
    EXPECT_EQ(static_cast<i8>(static_cast<i32>(EffectType::Poison)), effect["EffectId"].get<i8>());
    // 持续时间应该是秒*20 = 5*20 = 100 ticks
    EXPECT_EQ(100, effect["EffectDuration"].get<i32>());
}

TEST_F(SetStewEffectTest, MultipleEffects_RandomSelection)
{
    // 测试多个效果时随机选择一个
    SetStewEffectFunction func;
    func.addEffect("minecraft:poison", RandomValueRange(5.0f));
    func.addEffect("minecraft:blindness", RandomValueRange(10.0f));
    func.addEffect("minecraft:nausea", RandomValueRange(15.0f));

    // 统计每个效果被选择的次数
    std::unordered_map<i8, int> selectionCounts;

    for (int i = 0; i < 100; ++i) {
        ItemStack stack(Items::SUSPICIOUS_STEW, 1);
        m_random = std::make_unique<math::Random>(i + 1000); // 不同的种子
        LootContext context(m_world, *m_random);

        ItemStack result = func.apply(stack, context);
        ASSERT_TRUE(result.hasTag());

        const nlohmann::json* tag = result.getTag();
        ASSERT_TRUE(tag->contains("Effects"));
        ASSERT_EQ(1, (*tag)["Effects"].size());

        i8 effectId = (*tag)["Effects"][0]["EffectId"].get<i8>();
        selectionCounts[effectId]++;
    }

    // 所有效果都应该被选择至少一次
    EXPECT_EQ(3, selectionCounts.size()) << "All effects should be selected at least once";
    for (const auto& [id, count] : selectionCounts) {
        EXPECT_GT(count, 0) << "Effect " << static_cast<int>(id) << " should be selected at least once";
    }
}

TEST_F(SetStewEffectTest, DurationRange)
{
    // 测试持续时间范围
    SetStewEffectFunction func;
    func.addEffect("minecraft:regeneration", RandomValueRange(3.0f, 8.0f)); // 3-8秒

    bool sawMin = false;
    bool sawMax = false;

    for (int i = 0; i < 100; ++i) {
        ItemStack stack(Items::SUSPICIOUS_STEW, 1);
        m_random = std::make_unique<math::Random>(i + 2000);
        LootContext context(m_world, *m_random);

        ItemStack result = func.apply(stack, context);
        const nlohmann::json* tag = result.getTag();
        i32 duration = (*tag)["Effects"][0]["EffectDuration"].get<i32>();

        // 持续时间应该在 60-160 ticks 之间 (3-8秒 * 20)
        EXPECT_GE(duration, 60);
        EXPECT_LE(duration, 160);

        if (duration == 60) sawMin = true;
        if (duration == 160) sawMax = true;
    }

    // 至少应该看到最小和最大值
    EXPECT_TRUE(sawMin) << "Should see minimum duration";
    EXPECT_TRUE(sawMax) << "Should see maximum duration";
}

// ============================================================================
// 效果类型解析测试
// ============================================================================

TEST_F(SetStewEffectTest, ResourceLocationFormat)
{
    // 测试完整资源位置格式
    SetStewEffectFunction func;
    func.addEffect("minecraft:night_vision", RandomValueRange(5.0f));
    ItemStack stack(Items::SUSPICIOUS_STEW, 1);
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    const nlohmann::json* tag = result.getTag();
    i8 effectId = (*tag)["Effects"][0]["EffectId"].get<i8>();
    EXPECT_EQ(static_cast<i8>(static_cast<i32>(EffectType::NightVision)), effectId);
}

TEST_F(SetStewEffectTest, ShortNameFormat)
{
    // 测试简写格式
    SetStewEffectFunction func;
    func.addEffect("wither", RandomValueRange(5.0f));
    ItemStack stack(Items::SUSPICIOUS_STEW, 1);
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    const nlohmann::json* tag = result.getTag();
    i8 effectId = (*tag)["Effects"][0]["EffectId"].get<i8>();
    EXPECT_EQ(static_cast<i8>(static_cast<i32>(EffectType::Wither)), effectId);
}

TEST_F(SetStewEffectTest, InvalidEffectName_NoEffect)
{
    // 测试无效效果名称
    SetStewEffectFunction func;
    func.addEffect("minecraft:invalid_effect", RandomValueRange(5.0f));
    ItemStack stack(Items::SUSPICIOUS_STEW, 1);
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    // 无效效果不应添加任何数据
    EXPECT_FALSE(result.hasTag());
}

// ============================================================================
// 瞬间效果测试
// ============================================================================

TEST_F(SetStewEffectTest, InstantEffect_NoTickMultiplication)
{
    // 瞬间效果的持续时间不应该乘以20
    SetStewEffectFunction func;
    func.addEffect("minecraft:instant_health", RandomValueRange(1.0f)); // 瞬间治疗
    ItemStack stack(Items::SUSPICIOUS_STEW, 1);
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    const nlohmann::json* tag = result.getTag();
    i32 duration = (*tag)["Effects"][0]["EffectDuration"].get<i32>();

    // 瞬间效果不应乘以20
    EXPECT_EQ(1, duration) << "Instant effects should not have duration multiplied by 20";
}

TEST_F(SetStewEffectTest, SaturationEffect_NoTickMultiplication)
{
    // 饱和效果是瞬间效果
    SetStewEffectFunction func;
    func.addEffect("minecraft:saturation", RandomValueRange(2.0f));
    ItemStack stack(Items::SUSPICIOUS_STEW, 1);
    LootContext context(m_world, *m_random);

    ItemStack result = func.apply(stack, context);
    const nlohmann::json* tag = result.getTag();
    i32 duration = (*tag)["Effects"][0]["EffectDuration"].get<i32>();

    // 瞬间效果不应乘以20
    EXPECT_EQ(2, duration) << "Saturation is an instant effect";
}

// ============================================================================
// 多次应用测试
// ============================================================================

TEST_F(SetStewEffectTest, MultipleApplications_AppendEffects)
{
    // 多次应用应该追加效果到数组
    SetStewEffectFunction func;
    func.addEffect("minecraft:poison", RandomValueRange(5.0f));

    ItemStack stack(Items::SUSPICIOUS_STEW, 1);
    LootContext context1(m_world, *m_random);
    stack = func.apply(stack, context1); // 注意：apply 返回修改后的拷贝

    // 再次应用（使用相同的效果）
    m_random = std::make_unique<math::Random>(54321);
    LootContext context2(m_world, *m_random);
    ItemStack result = func.apply(stack, context2);

    // 应该有两个效果
    const nlohmann::json* tag = result.getTag();
    ASSERT_TRUE(tag->contains("Effects"));
    EXPECT_EQ(2, (*tag)["Effects"].size()) << "Effects should be appended, not replaced";
}

// ============================================================================
// Clone 测试
// ============================================================================

TEST_F(SetStewEffectTest, Clone)
{
    // 测试克隆功能
    SetStewEffectFunction original;
    original.addEffect("minecraft:poison", RandomValueRange(5.0f, 10.0f));
    original.addEffect("minecraft:blindness", RandomValueRange(3.0f, 7.0f));

    std::unique_ptr<LootFunction> cloned = original.clone();
    ASSERT_NE(nullptr, cloned);

    // 类型检查
    SetStewEffectFunction* clonedFunc = dynamic_cast<SetStewEffectFunction*>(cloned.get());
    ASSERT_NE(nullptr, clonedFunc);

    // 验证效果列表被正确克隆
    const auto& originalEffects = original.getEffects();
    const auto& clonedEffects = clonedFunc->getEffects();

    EXPECT_EQ(originalEffects.size(), clonedEffects.size());
    for (size_t i = 0; i < originalEffects.size(); ++i) {
        EXPECT_EQ(originalEffects[i].effectId, clonedEffects[i].effectId);
        EXPECT_FLOAT_EQ(originalEffects[i].duration.getMin(), clonedEffects[i].duration.getMin());
        EXPECT_FLOAT_EQ(originalEffects[i].duration.getMax(), clonedEffects[i].duration.getMax());
    }
}

// ============================================================================
// getType 测试
// ============================================================================

TEST_F(SetStewEffectTest, GetType)
{
    SetStewEffectFunction func;
    EXPECT_EQ("set_stew_effect", func.getType());
}
