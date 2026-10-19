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

/**
 * @file EnvironmentAttributeTest.cpp
 * @brief 环境属性系统与预定义时间线单元测试
 *
 * 覆盖：
 * 1. 修改器库：布尔/浮点/颜色修改器与属性类型允许的修改器
 * 2. 预定义时间线：DAY/MOON/EARLY_GAME 关键帧语义与轨道覆盖
 * 3. 环境属性系统：常量层 → 时间线层的层序、tick 缓存与位置层跳过
 * 4. 维度级装配：主世界/下界的时间线声明与属性查询
 */

#include <gtest/gtest.h>

#include "common/util/TriState.hpp"
#include "common/world/attribute/AttributeTypes.hpp"
#include "common/world/attribute/DimensionAttributes.hpp"
#include "common/world/attribute/EnvironmentAttributeMap.hpp"
#include "common/world/attribute/EnvironmentAttributeRegistry.hpp"
#include "common/world/attribute/EnvironmentAttributeSystem.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/level/MoonPhase.hpp"
#include "common/world/timeline/AttributeTrackSampler.hpp"
#include "common/world/timeline/Timeline.hpp"
#include "common/world/timeline/TimelineRegistry.hpp"
#include "common/world/timeline/Timelines.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>

namespace mc::world::attribute {
namespace {

using timeline::AttributeTrackSampler;
using timeline::KeyframeTrack;
using timeline::TimelineRegistry;

/// 以固定 dayTime 驱动的时间线采样器
template <typename Value>
class FixedClockSampler {
public:
    FixedClockSampler(const timeline::Timeline& trackSource, const EnvironmentAttribute<Value>& attribute, Value base)
        : m_base(std::move(base))
        , m_sampler(trackSource.createTrackSampler<Value, Value>(attribute, [this]() { return m_dayTime; }))
    {}

    /// 推进 tick 并采样指定 dayTime（模拟新的一 tick）
    [[nodiscard]] Value sample(i64 dayTime)
    {
        m_dayTime = dayTime;
        ++m_tick;
        return m_sampler->applyTimeBased(m_base, m_tick);
    }

    /// 在当前 tick 内再次采样（与 sample 等价，只是语义上强调同 tick）
    [[nodiscard]] Value sampleAt(i64 dayTime) { return sample(dayTime); }

private:
    Value m_base;
    i64 m_dayTime = 0;
    i64 m_tick = 0;
    std::unique_ptr<timeline::AttributeTrackSampler<Value, Value>> m_sampler;
};

} // namespace

// ============================================================================
// 修改器
// ============================================================================

TEST(AttributeModifierTest, BooleanModifiersFollowVanillaSemantics)
{
    EXPECT_TRUE(BooleanModifier::AND.apply(true, true));
    EXPECT_FALSE(BooleanModifier::AND.apply(true, false));
    EXPECT_TRUE(BooleanModifier::NAND.apply(true, false));
    EXPECT_FALSE(BooleanModifier::NAND.apply(true, true));
    EXPECT_TRUE(BooleanModifier::OR.apply(false, true));
    EXPECT_FALSE(BooleanModifier::OR.apply(false, false));
    EXPECT_TRUE(BooleanModifier::NOR.apply(false, false));
    EXPECT_FALSE(BooleanModifier::NOR.apply(false, true));
    EXPECT_TRUE(BooleanModifier::XOR.apply(true, false));
    EXPECT_FALSE(BooleanModifier::XOR.apply(true, true));
    EXPECT_TRUE(BooleanModifier::XNOR.apply(true, true));
}

TEST(AttributeModifierTest, FloatModifiersAreSaturatingFree)
{
    EXPECT_FLOAT_EQ(FloatModifier::ADD.apply(2.0f, 3.0f), 5.0f);
    EXPECT_FLOAT_EQ(FloatModifier::SUBTRACT.apply(5.0f, 3.0f), 2.0f);
    EXPECT_FLOAT_EQ(FloatModifier::MULTIPLY.apply(2.0f, 0.5f), 1.0f);
    EXPECT_FLOAT_EQ(FloatModifier::MINIMUM.apply(2.0f, 3.0f), 2.0f);
    EXPECT_FLOAT_EQ(FloatModifier::MAXIMUM.apply(2.0f, 3.0f), 3.0f);
}

TEST(AttributeModifierTest, ColorMultiplySupportsWhiteIdentity)
{
    // 乘数为 -1（白色）时原样返回，避免逐通道取整误差
    constexpr i32 COLOR = 0xFF102030;
    EXPECT_EQ(RgbColorModifier::MULTIPLY.apply(COLOR, -1), COLOR);
    EXPECT_EQ(RgbColorModifier::MULTIPLY.apply(-1, COLOR), COLOR);

    // 50% 灰 (128/255) 乘到各通道：0x10*128/255=8、0x20*128/255=16、0x30*128/255=24
    EXPECT_EQ(RgbColorModifier::MULTIPLY.apply(COLOR, 0xFF808080), static_cast<i32>(0xFF081018u));
}

TEST(AttributeModifierTest, ColorAddAndSubtractSaturate)
{
    // ADD 只作用于 RGB 并保留基础色透明度；(0x80+0x00, 0x80+0x00, 0x80+0xFF) 中蓝色饱和到 255
    EXPECT_EQ(static_cast<u32>(RgbColorModifier::ADD.apply(0xFF808080, 0xFF0000FF)), 0xFF8080FFu);
    EXPECT_EQ(static_cast<u32>(RgbColorModifier::SUBTRACT.apply(0xFF102030, 0xFF40506F)), 0xFF000000u);
}

TEST(AttributeTypeTest, ModifierLibraryMatchesVanilla)
{
    const auto& floatTypes = AttributeTypes::FLOAT();
    EXPECT_TRUE(floatTypes.isAllowedModifier(AttributeModifierOperation::Add));
    EXPECT_TRUE(floatTypes.isAllowedModifier(AttributeModifierOperation::Maximum));
    EXPECT_FALSE(floatTypes.isAllowedModifier(AttributeModifierOperation::Or));

    const auto& booleanTypes = AttributeTypes::BOOLEAN();
    EXPECT_TRUE(booleanTypes.isAllowedModifier(AttributeModifierOperation::Xnor));
    EXPECT_FALSE(booleanTypes.isAllowedModifier(AttributeModifierOperation::Multiply));

    // 覆写修改器对所有类型均合法
    EXPECT_TRUE(AttributeTypes::ACTIVITY().isAllowedModifier(AttributeModifierOperation::Override));
    EXPECT_FALSE(AttributeTypes::ACTIVITY().isAllowedModifier(AttributeModifierOperation::Maximum));
}

// ============================================================================
// 预定义时间线
// ============================================================================

TEST(TimelinesTest, DayTimelinePeriodAndAngles)
{
    EXPECT_EQ(timeline::Timelines::DAY().periodTicks(), std::optional<i32>(24000));

    FixedClockSampler<f32> sunAngle(timeline::Timelines::DAY(),
        EnvironmentAttributes::SUN_ANGLE(),
        EnvironmentAttributes::SUN_ANGLE().defaultValue());
    // 日出（6000）与日落（18000）是关键帧，中间按对称贝塞尔缓动旋转
    EXPECT_NEAR(sunAngle.sample(6000), 360.0f, 1e-3f);
    EXPECT_NEAR(sunAngle.sample(18000), 0.0f, 1e-3f);
    // 两侧关键帧值相同（360 与 0 表示同一朝向）时缓动不产生中途值
    const f32 noonAngle = sunAngle.sample(9000);
    EXPECT_GE(noonAngle, 0.0f);
    EXPECT_LE(noonAngle, 360.0f);
}

TEST(TimelinesTest, DayTimelineEyeblossomOpensAtNight)
{
    FixedClockSampler<util::TriState> open(timeline::Timelines::DAY(),
        EnvironmentAttributes::EYEBLOSSOM_OPEN(),
        EnvironmentAttributes::EYEBLOSSOM_OPEN().defaultValue());

    EXPECT_EQ(open.sample(12599), util::TriState::False);
    EXPECT_EQ(open.sample(12600), util::TriState::True);
    EXPECT_EQ(open.sample(23400), util::TriState::True);
    EXPECT_EQ(open.sample(23401), util::TriState::False);
}

TEST(TimelinesTest, DayTimelineNightColorsAreMultiplied)
{
    FixedClockSampler<i32> fogColor(timeline::Timelines::DAY(),
        EnvironmentAttributes::FOG_COLOR(),
        EnvironmentAttributes::FOG_COLOR().defaultValue());

    // 基础值（乘法轨道以属性默认值为基础）；白天乘数为白色 (-1)，结果不变
    const i32 baseValue = EnvironmentAttributes::FOG_COLOR().defaultValue();
    fogColor.sample(6000);
    EXPECT_EQ(fogColor.sampleAt(6000), baseValue);

    // 夜间乘数 (ARGB 255,15,15,23) 把各个通道压缩：默认值各通道均为 0，乘后仍为 0
    const i32 nightValue = fogColor.sampleAt(18000);
    EXPECT_EQ(nightValue & 0xFF, 0);
    EXPECT_EQ((nightValue >> 8) & 0xFF, 0);
    EXPECT_EQ((nightValue >> 16) & 0xFF, 0);

    // 以非零基础值采样时能看到乘数生效
    FixedClockSampler<i32> nightFromWhite(
        timeline::Timelines::DAY(), EnvironmentAttributes::FOG_COLOR(), static_cast<i32>(0xFFFFFFFFu));
    const i32 whiteNight = nightFromWhite.sample(18000);
    EXPECT_EQ(whiteNight & 0xFF, 23);
    EXPECT_EQ((whiteNight >> 8) & 0xFF, 15);
    EXPECT_EQ((whiteNight >> 16) & 0xFF, 15);
}

TEST(TimelinesTest, MoonTimelineCyclesEachPhasePerDay)
{
    EXPECT_EQ(timeline::Timelines::MOON().periodTicks(), std::optional<i32>(MOON_PHASE_LENGTH * MOON_PHASE_COUNT));

    FixedClockSampler<MoonPhase> moonPhase(timeline::Timelines::MOON(),
        EnvironmentAttributes::MOON_PHASE(),
        EnvironmentAttributes::MOON_PHASE().defaultValue());
    EXPECT_EQ(moonPhase.sample(0), MoonPhase::FullMoon);
    EXPECT_EQ(moonPhase.sample(23999), MoonPhase::FullMoon);
    EXPECT_EQ(moonPhase.sample(24000), MoonPhase::WaningGibbous);
    EXPECT_EQ(moonPhase.sample(96000), MoonPhase::NewMoon);
    EXPECT_EQ(moonPhase.sample(168000), MoonPhase::WaxingGibbous);
    // 周期回绕：第 9 天回到满月
    EXPECT_EQ(moonPhase.sample(192000), MoonPhase::FullMoon);
}

TEST(TimelinesTest, MoonTimelineSlimeChanceFollowsBrightness)
{
    FixedClockSampler<f32> slime(timeline::Timelines::MOON(),
        EnvironmentAttributes::SURFACE_SLIME_SPAWN_CHANCE(),
        EnvironmentAttributes::SURFACE_SLIME_SPAWN_CHANCE().defaultValue());

    EXPECT_FLOAT_EQ(slime.sample(0), 0.5f);
    EXPECT_FLOAT_EQ(slime.sample(48000), 0.25f);
    EXPECT_FLOAT_EQ(slime.sample(96000), 0.0f);
    EXPECT_FLOAT_EQ(slime.sample(144000), 0.25f);
}

TEST(TimelinesTest, EarlyGameBlocksPatrolsForFirstFiveDays)
{
    FixedClockSampler<bool> patrol(timeline::Timelines::EARLY_GAME(),
        EnvironmentAttributes::CAN_PILLAGER_PATROL_SPAWN(),
        EnvironmentAttributes::CAN_PILLAGER_PATROL_SPAWN().defaultValue());

    // AND 修改器以属性默认值（true）为基础：前 120000 tick 被压成 false
    EXPECT_FALSE(patrol.sample(0));
    EXPECT_FALSE(patrol.sample(119999));
    EXPECT_TRUE(patrol.sample(120000));
    EXPECT_TRUE(patrol.sample(240000));
}

TEST(TimelinesRegistryTest, BootstrapRegistersAllBuiltInTimelines)
{
    timeline::Timelines::bootstrap();

    EXPECT_NE(TimelineRegistry::find("minecraft:day"), nullptr);
    EXPECT_NE(TimelineRegistry::find("day"), nullptr);
    EXPECT_NE(TimelineRegistry::find("minecraft:villager_schedule"), nullptr);
    EXPECT_NE(TimelineRegistry::find("minecraft:early_game"), nullptr);
    EXPECT_EQ(TimelineRegistry::find("minecraft:unknown"), nullptr);

    EXPECT_NE(EnvironmentAttributeRegistry::find("minecraft:visual/sky_color"), nullptr);
    EXPECT_NE(EnvironmentAttributeRegistry::find("minecraft:gameplay/monsters_burn"), nullptr);
    EXPECT_NE(EnvironmentAttributeRegistry::find("visual/sky_color"), nullptr);
}

// ============================================================================
// 环境属性系统
// ============================================================================

namespace {

/// 测试用浮点属性（静态实例：属性以地址为身份）
const EnvironmentAttribute<f32>& _testFloatAttribute()
{
    static const EnvironmentAttribute<f32> attribute = EnvironmentAttribute<f32>::builder(
        []() -> const std::string& {
            static const std::string name = "test/float";
            return name;
        },
        AttributeTypes::FLOAT(),
        2.0f)
                                                           .build();
    return attribute;
}

/// 测试用时间线：周期 2000；轨道 0 → 1、1000 → 3（线性），以乘修改器叠加
timeline::Timeline _makeTestTimeline(const EnvironmentAttribute<f32>& attribute)
{
    return timeline::Timeline::Builder()
        .setPeriodTicks(2000)
        .addModifierTrack<f32, f32>(attribute,
            nonOwningSharedPtr(&FloatModifier::MULTIPLY),
            std::nullopt,
            [](timeline::KeyframeTrack<f32>::Builder& builder) {
                builder.addKeyframe(0, 1.0f).addKeyframe(1000, 3.0f);
            })
        .build();
}

} // namespace

TEST(EnvironmentAttributeMapTest, SetAndModifyApplyModifiers)
{
    const auto& attribute = _testFloatAttribute();

    const EnvironmentAttributeMap overrideMap = EnvironmentAttributeMap::Builder().set(attribute, 5.0f).build();
    f32 value = attribute.defaultValue();
    overrideMap.applyModifier(&attribute, &value);
    EXPECT_FLOAT_EQ(value, 5.0f);

    const EnvironmentAttributeMap doubled =
        EnvironmentAttributeMap::Builder().modify(attribute, AttributeModifierOperation::Multiply, 2.0f).build();
    value = attribute.defaultValue();
    doubled.applyModifier(&attribute, &value);
    EXPECT_FLOAT_EQ(value, 4.0f);
    // 系统常量层总是从默认值开始求值，因此入参的当前值不影响非覆写修改器
    value = 10.0f;
    doubled.applyModifier(&attribute, &value);
    EXPECT_FLOAT_EQ(value, 4.0f);
}

TEST(EnvironmentAttributeSystemTest, ConstantLayerAppliesBeforeTimelineLayer)
{
    const auto& attribute = _testFloatAttribute();

    // 时间线：0 → 1、1000 → 3（线性），以乘 2 修改器叠加；周期 2000，
    // 因此 1500 落在 1000 → 2000 的回绕延伸段内
    const timeline::Timeline timeline = _makeTestTimeline(attribute);

    // 属性实例是静态对象，查询时钟需在此之后设置；闭包以指针读取，保证按查询时刻取值
    static i64 dayTime = 0;
    const std::unique_ptr<EnvironmentAttributeSystem> system = [&] {
        EnvironmentAttributeSystemBuilder systemBuilder;
        systemBuilder.addConstantLayer(attribute, EnvironmentAttributeMap::Builder().set(attribute, 4.0f).build());
        systemBuilder.addTimeBasedLayer(
            attribute, timeline.createTrackSampler<f32, f32>(attribute, [] { return dayTime; }));
        return std::make_unique<EnvironmentAttributeSystem>(systemBuilder.build());
    }();

    // 常量层先应用（默认值 2 → 4），时间线层再乘上采样值
    dayTime = 0;
    EXPECT_FLOAT_EQ(system->getDimensionValue(attribute), 4.0f);
    system->invalidateTickCache();
    EXPECT_FLOAT_EQ(system->getDimensionValue(attribute), 4.0f);
    dayTime = 1000;
    system->invalidateTickCache();
    EXPECT_FLOAT_EQ(system->getDimensionValue(attribute), 12.0f);

    // per-tick 缓存：沿用当前 tick 序号时复用缓存，不再重新采样
    dayTime = 500;
    EXPECT_FLOAT_EQ(system->getDimensionValue(attribute), 12.0f);
    system->invalidateTickCache();
    EXPECT_FLOAT_EQ(system->getDimensionValue(attribute), 8.0f);

    // 拷贝包含当 tick 的维度缓存；失效后重新求值。
    // 注意：层闭包按引用共享，采样器自身仍持有原系统 tick 序号的缓存，
    // 因此改变 dayTime 后需同时失效原系统，才能保证拷贝按新时刻重采样。
    dayTime = 1500;
    const EnvironmentAttributeSystem copy = *system;
    EXPECT_FLOAT_EQ(copy.getDimensionValue(attribute), 8.0f);
    system->invalidateTickCache();
    copy.invalidateTickCache();
    // 1500 位于 1000 → 2000 的回绕延伸段内，采样值从 3 线性过渡到 1（中点 2），乘 4 得 8
    EXPECT_FLOAT_EQ(copy.getDimensionValue(attribute), 8.0f);
}

TEST(EnvironmentAttributeSystemTest, InvalidateTickCacheResamplesTimelineLayer)
{
    const auto& attribute = _testFloatAttribute();
    const timeline::Timeline timeline = _makeTestTimeline(attribute);

    static i64 dayTime = 0;
    dayTime = 0;
    EnvironmentAttributeSystemBuilder builder;
    builder.addConstantLayer(attribute, EnvironmentAttributeMap::Builder().set(attribute, 4.0f).build());
    builder.addTimeBasedLayer(attribute, timeline.createTrackSampler<f32, f32>(attribute, [] { return dayTime; }));
    const EnvironmentAttributeSystem system = builder.build();

    // 每个 tick 只应用一次"系统序号 → 采样器缓存"：失效后必须按新 dayTime 重新采样，
    // 不能继续命中层自身的旧缓存（与 vanilla AttributeTrackSampler 的缓存语义一致）
    EXPECT_FLOAT_EQ(system.getDimensionValue(attribute), 4.0f);
    system.invalidateTickCache();
    dayTime = 1000;
    EXPECT_FLOAT_EQ(system.getDimensionValue(attribute), 12.0f);
    system.invalidateTickCache();
    dayTime = 500;
    EXPECT_FLOAT_EQ(system.getDimensionValue(attribute), 8.0f);
    system.invalidateTickCache();
    dayTime = 0;
    EXPECT_FLOAT_EQ(system.getDimensionValue(attribute), 4.0f);
}

TEST(EnvironmentAttributeSystemTest, PositionalLayerSkippedForNonPositionalQuery)
{
    const auto& attribute = _testFloatAttribute();

    EnvironmentAttributeSystemBuilder builder;
    builder.addPositionalLayer<f32>(
        attribute, [](const f32& value, const Vector3& pos) -> f32 { return value + pos.x; });
    const EnvironmentAttributeSystem system = builder.build();

    EXPECT_TRUE(system.isAffectedByPosition(attribute));
    EXPECT_FLOAT_EQ(system.getValue(attribute, Vector3(10.0f, 0.0f, 0.0f)), 12.0f);
    // 不考虑位置时跳过位置层
    EXPECT_FLOAT_EQ(system.getDimensionValue(attribute), 2.0f);
}

TEST(EnvironmentAttributeSystemTest, UnknownAttributeFallsBackToDefault)
{
    const EnvironmentAttributeSystem system;

    EXPECT_FLOAT_EQ(system.getDimensionValue(EnvironmentAttributes::FOG_END_DISTANCE()), 1024.0f);
    EXPECT_EQ(system.getDimensionValue(EnvironmentAttributes::EYEBLOSSOM_OPEN()), util::TriState::Default);
    EXPECT_EQ(system.attributeCount(), 0u);
}

// ============================================================================
// 维度级装配
// ============================================================================

TEST(DimensionAttributesTest, OverworldDeclaresDayTimelineAndNetherDoesNot)
{
    const DimensionType overworld = DimensionType::overworld();
    const DimensionType nether = DimensionType::nether();

    EXPECT_FALSE(overworld.timelines().empty());
    EXPECT_TRUE(nether.timelines().empty());
}

TEST(DimensionAttributesTest, OverworldEyeblossomFollowsDayTime)
{
    timeline::Timelines::bootstrap();

    const auto makeSystem = [](i64 dayTime) {
        return DimensionAttributes::createSystem(
            DimensionType::overworld(), [dayTime]() { return dayTime; }, [dayTime]() { return dayTime; });
    };

    EXPECT_EQ(makeSystem(6000)->getDimensionValue(EnvironmentAttributes::EYEBLOSSOM_OPEN()), util::TriState::False);
    EXPECT_EQ(makeSystem(18000)->getDimensionValue(EnvironmentAttributes::EYEBLOSSOM_OPEN()), util::TriState::True);
}

TEST(DimensionAttributesTest, NetherKeepsDefaultValues)
{
    timeline::Timelines::bootstrap();

    const std::unique_ptr<EnvironmentAttributeSystem> system = DimensionAttributes::createSystem(
        DimensionType::nether(), [] { return i64{18000}; }, [] { return i64{18000}; });

    // 下界不声明日夜时间线：眼眸花保持默认（不主动切换）
    EXPECT_EQ(system->getDimensionValue(EnvironmentAttributes::EYEBLOSSOM_OPEN()), util::TriState::Default);
    EXPECT_EQ(system->getDimensionValue(EnvironmentAttributes::MOON_PHASE()), MoonPhase::FullMoon);
}

// ============================================================================
// 数据包属性解析（biome/dimension attributes）
// ============================================================================

TEST(EnvironmentAttributeJsonTest, ParsesLiteralAndFullFormArguments)
{
    timeline::Timelines::bootstrap();

    EnvironmentAttributeMap::Builder builder;
    // 直接给出参数：等价于覆写
    ASSERT_TRUE(builder.setFromJson("minecraft:visual/sky_color", nlohmann::json("#00ff80")).success());
    // 完整写法：修改器 + 参数
    ASSERT_TRUE(builder
            .setFromJson("minecraft:visual/fog_start_distance",
                nlohmann::json::parse(R"({"modifier": "multiply", "argument": 2.5})"))
            .success());
    ASSERT_TRUE(builder.setFromJson("minecraft:gameplay/monsters_burn", nlohmann::json(true)).success());
    const EnvironmentAttributeMap map = builder.build();

    i32 skyColor = 0;
    map.applyModifier(&EnvironmentAttributes::SKY_COLOR(), &skyColor);
    EXPECT_EQ(static_cast<u32>(skyColor), 0xFF00FF80u);

    // 非覆写修改器以属性默认值为基础（fog_start_distance 默认 0 → 仍为 0），
    f32 fogStart = EnvironmentAttributes::FOG_START_DISTANCE().defaultValue();
    map.applyModifier(&EnvironmentAttributes::FOG_START_DISTANCE(), &fogStart);
    EXPECT_FLOAT_EQ(fogStart, EnvironmentAttributes::FOG_START_DISTANCE().defaultValue() * 2.5f);

    bool monstersBurn = false;
    map.applyModifier(&EnvironmentAttributes::MONSTERS_BURN(), &monstersBurn);
    EXPECT_TRUE(monstersBurn);
}

TEST(EnvironmentAttributeJsonTest, RejectsUnknownAttributeAndDisallowedModifier)
{
    timeline::Timelines::bootstrap();

    EnvironmentAttributeMap::Builder builder;
    const Result<void> unknown = builder.setFromJson("minecraft:visual/not_a_real_attribute", nlohmann::json(1));
    EXPECT_FALSE(unknown.success());

    // 布尔属性不允许 multiply
    const Result<void> disallowed = builder.setFromJson(
        "minecraft:gameplay/monsters_burn", nlohmann::json::parse(R"({"modifier": "multiply", "argument": true})"));
    EXPECT_FALSE(disallowed.success());

    // 无法解析的参数
    const Result<void> badValue = builder.setFromJson("minecraft:gameplay/monsters_burn", nlohmann::json("yes"));
    EXPECT_FALSE(badValue.success());
}

TEST(DimensionAttributesTest, BiomeLayerAppliesBiomeAttributesByPosition)
{
    timeline::Timelines::bootstrap();

    // 构造一个群系：plains 声明 increased_fire_burnout = true，其余位置不受影响
    auto& registry = world::biome::BiomeRegistry::instance();
    registry.initialize();
    const EnvironmentAttributeMap original = registry.get(Biomes::Plains).attributes();

    auto builder = EnvironmentAttributeMap::Builder();
    ASSERT_TRUE(builder.setFromJson("minecraft:gameplay/increased_fire_burnout", nlohmann::json(true)).success());
    registry.getMutable(Biomes::Plains).setAttributes(builder.build());

    const DimensionAttributes::BiomeResolver resolver = [](const Vector3& pos) {
        return pos.x >= 0.0f ? Biomes::Plains : Biomes::Desert;
    };
    const std::unique_ptr<EnvironmentAttributeSystem> system = DimensionAttributes::createSystem(
        DimensionType::nether(), [] { return i64{0}; }, [] { return i64{0}; }, resolver);

    EXPECT_TRUE(system->getValue(EnvironmentAttributes::INCREASED_FIRE_BURNOUT(), Vector3(4.0f, 64.0f, 4.0f)));
    EXPECT_FALSE(system->getValue(EnvironmentAttributes::INCREASED_FIRE_BURNOUT(), Vector3(-4.0f, 64.0f, 4.0f)));
    // 非位置查询跳过群系层
    EXPECT_FALSE(system->getDimensionValue(EnvironmentAttributes::INCREASED_FIRE_BURNOUT()));

    registry.getMutable(Biomes::Plains).setAttributes(original);
}

} // namespace mc::world::attribute
