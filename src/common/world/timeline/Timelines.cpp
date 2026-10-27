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

#include "common/world/timeline/Timelines.hpp"

#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/util/TriState.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/attribute/AttributeModifier.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/level/MoonPhase.hpp"
#include "common/world/timeline/KeyframeTrack.hpp"
#include "common/world/timeline/TimelineRegistry.hpp"

#include <array>
#include <memory>
#include <mutex>
#include <utility>

namespace mc {
namespace world {
namespace timeline {
namespace {

using attribute::AttributeModifierOperation;

/// 布尔修改器单例（AND / NAND / OR / NOR / XOR / XNOR）
std::shared_ptr<const attribute::AttributeModifier<bool, bool>> _booleanModifier(AttributeModifierOperation operation)
{
    switch (operation) {
        case AttributeModifierOperation::And:
            return nonOwningSharedPtr(&attribute::BooleanModifier::AND);
        case AttributeModifierOperation::Nand:
            return nonOwningSharedPtr(&attribute::BooleanModifier::NAND);
        case AttributeModifierOperation::Or:
            return nonOwningSharedPtr(&attribute::BooleanModifier::OR);
        case AttributeModifierOperation::Nor:
            return nonOwningSharedPtr(&attribute::BooleanModifier::NOR);
        case AttributeModifierOperation::Xor:
            return nonOwningSharedPtr(&attribute::BooleanModifier::XOR);
        case AttributeModifierOperation::Xnor:
            return nonOwningSharedPtr(&attribute::BooleanModifier::XNOR);
        default:
            MC_ASSERT_RELEASE_MSG(false, "Not a boolean modifier operation");
            return nullptr;
    }
}

/// 浮点修改器（ADD / SUBTRACT / MULTIPLY / MINIMUM / MAXIMUM）
std::shared_ptr<const attribute::AttributeModifier<f32, f32>> _floatModifier(AttributeModifierOperation operation)
{
    switch (operation) {
        case AttributeModifierOperation::Add:
            return nonOwningSharedPtr(&attribute::FloatModifier::ADD);
        case AttributeModifierOperation::Subtract:
            return nonOwningSharedPtr(&attribute::FloatModifier::SUBTRACT);
        case AttributeModifierOperation::Multiply:
            return nonOwningSharedPtr(&attribute::FloatModifier::MULTIPLY);
        case AttributeModifierOperation::Minimum:
            return nonOwningSharedPtr(&attribute::FloatModifier::MINIMUM);
        case AttributeModifierOperation::Maximum:
            return nonOwningSharedPtr(&attribute::FloatModifier::MAXIMUM);
        default:
            MC_ASSERT_RELEASE_MSG(false, "Not a float modifier operation");
            return nullptr;
    }
}

/// 颜色修改器（ADD / SUBTRACT / MULTIPLY）
std::shared_ptr<const attribute::AttributeModifier<i32, i32>> _colorModifier(AttributeModifierOperation operation)
{
    switch (operation) {
        case AttributeModifierOperation::Add:
            return nonOwningSharedPtr(&attribute::RgbColorModifier::ADD);
        case AttributeModifierOperation::Subtract:
            return nonOwningSharedPtr(&attribute::RgbColorModifier::SUBTRACT);
        case AttributeModifierOperation::Multiply:
            return nonOwningSharedPtr(&attribute::RgbColorModifier::MULTIPLY);
        default:
            MC_ASSERT_RELEASE_MSG(false, "Not a color modifier operation");
            return nullptr;
    }
}

} // namespace

const Timeline& Timelines::DAY()
{
    static const Timeline instance = [] {
        // 日出日落前后的天光过渡使用对称三次贝塞尔缓动
        const EasingType daylightEasing = EasingType::symmetricCubicBezier(0.362f, 0.241f);

        // vanilla 常量：夜间雾/云/天空色的乘数（ARGB）
        constexpr i32 NIGHT_FOG_COLOR_MULTIPLIER = attribute::argbColor(255, 15, 15, 23);
        constexpr i32 NIGHT_CLOUD_COLOR_MULTIPLIER = attribute::argbColor(255, 26, 26, 38);
        constexpr i32 NIGHT_SKY_LIGHT_COLOR = attribute::argbColor(255, 122, 122, 255);
        constexpr i32 NIGHT_SKY_COLOR_MULTIPLIER = static_cast<i32>(0xFF000000u);
        constexpr f32 NIGHT_SKY_LIGHT_FACTOR = 0.24f;
        constexpr f32 NIGHT_SKY_LIGHT_LEVEL_FACTOR = 0.26666668f;

        return Timeline::Builder()
            .setPeriodTicks(24000)
            // 天体角度：日出（6000 tick）到日落（18000 tick）之间匀速旋转一整圈
            .addTrack(attribute::EnvironmentAttributes::SUN_ANGLE(),
                [daylightEasing](KeyframeTrack<f32>::Builder& builder) {
                    builder.setEasing(daylightEasing).addKeyframe(6000, 360.0f).addKeyframe(18000, 0.0f);
                })
            .addTrack(attribute::EnvironmentAttributes::MOON_ANGLE(),
                [daylightEasing](KeyframeTrack<f32>::Builder& builder) {
                    builder.setEasing(daylightEasing).addKeyframe(6000, 540.0f).addKeyframe(18000, 180.0f);
                })
            .addTrack(attribute::EnvironmentAttributes::STAR_ANGLE(),
                [daylightEasing](KeyframeTrack<f32>::Builder& builder) {
                    builder.setEasing(daylightEasing).addKeyframe(6000, 360.0f).addKeyframe(18000, 0.0f);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::FIREFLY_BUSH_SOUNDS(),
                _booleanModifier(AttributeModifierOperation::Or),
                std::nullopt,
                [](KeyframeTrack<bool>::Builder& builder) {
                    builder.addKeyframe(12600, true).addKeyframe(23401, false);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::FOG_COLOR(),
                _colorModifier(AttributeModifierOperation::Multiply),
                std::nullopt,
                [NIGHT_FOG_COLOR_MULTIPLIER](KeyframeTrack<i32>::Builder& builder) {
                    builder.addKeyframe(133, -1)
                        .addKeyframe(11867, -1)
                        .addKeyframe(13670, NIGHT_FOG_COLOR_MULTIPLIER)
                        .addKeyframe(22330, NIGHT_FOG_COLOR_MULTIPLIER);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::SKY_COLOR(),
                _colorModifier(AttributeModifierOperation::Multiply),
                std::nullopt,
                [NIGHT_SKY_COLOR_MULTIPLIER](KeyframeTrack<i32>::Builder& builder) {
                    builder.addKeyframe(133, -1)
                        .addKeyframe(11867, -1)
                        .addKeyframe(13670, NIGHT_SKY_COLOR_MULTIPLIER)
                        .addKeyframe(22330, NIGHT_SKY_COLOR_MULTIPLIER);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::SKY_LIGHT_COLOR(),
                _colorModifier(AttributeModifierOperation::Multiply),
                std::nullopt,
                [NIGHT_SKY_LIGHT_COLOR](KeyframeTrack<i32>::Builder& builder) {
                    builder.addKeyframe(730, -1)
                        .addKeyframe(11270, -1)
                        .addKeyframe(13140, NIGHT_SKY_LIGHT_COLOR)
                        .addKeyframe(22860, NIGHT_SKY_LIGHT_COLOR);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::SKY_LIGHT_FACTOR(),
                _floatModifier(AttributeModifierOperation::Multiply),
                std::nullopt,
                [NIGHT_SKY_LIGHT_FACTOR](KeyframeTrack<f32>::Builder& builder) {
                    builder.addKeyframe(730, 1.0f)
                        .addKeyframe(11270, 1.0f)
                        .addKeyframe(13140, NIGHT_SKY_LIGHT_FACTOR)
                        .addKeyframe(22860, NIGHT_SKY_LIGHT_FACTOR);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::SKY_LIGHT_LEVEL(),
                _floatModifier(AttributeModifierOperation::Multiply),
                std::nullopt,
                [NIGHT_SKY_LIGHT_LEVEL_FACTOR](KeyframeTrack<f32>::Builder& builder) {
                    builder.addKeyframe(133, 1.0f)
                        .addKeyframe(11867, 1.0f)
                        .addKeyframe(13670, NIGHT_SKY_LIGHT_LEVEL_FACTOR)
                        .addKeyframe(22330, NIGHT_SKY_LIGHT_LEVEL_FACTOR);
                })
            .addTrack(attribute::EnvironmentAttributes::SUNRISE_SUNSET_COLOR(),
                [](KeyframeTrack<i32>::Builder& builder) {
                    builder.addKeyframe(71, 1609540403)
                        .addKeyframe(310, 703969843)
                        .addKeyframe(565, 117167155)
                        .addKeyframe(730, 16770355)
                        .addKeyframe(11270, 16770355)
                        .addKeyframe(11397, 83679283)
                        .addKeyframe(11522, 268028723)
                        .addKeyframe(11690, 703969843)
                        .addKeyframe(11929, 1609540403)
                        .addKeyframe(12243, -1310226637)
                        .addKeyframe(12358, -857440717)
                        .addKeyframe(12512, -371166669)
                        .addKeyframe(12613, -153261261)
                        .addKeyframe(12732, -19242189)
                        .addKeyframe(12841, -19440589)
                        .addKeyframe(13035, -321760973)
                        .addKeyframe(13252, -1043577037)
                        .addKeyframe(13775, 918435635)
                        .addKeyframe(13888, 532362547)
                        .addKeyframe(14039, 163001139)
                        .addKeyframe(14192, 11744051)
                        .addKeyframe(21807, 11678515)
                        .addKeyframe(21961, 163001139)
                        .addKeyframe(22112, 532362547)
                        .addKeyframe(22225, 918435635)
                        .addKeyframe(22748, -1043577037)
                        .addKeyframe(22965, -321760973)
                        .addKeyframe(23159, -19440589)
                        .addKeyframe(23272, -19242189)
                        .addKeyframe(23488, -371166669)
                        .addKeyframe(23642, -857440717)
                        .addKeyframe(23757, -1310226637);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::STAR_BRIGHTNESS(),
                _floatModifier(AttributeModifierOperation::Maximum),
                std::nullopt,
                [](KeyframeTrack<f32>::Builder& builder) {
                    builder.addKeyframe(92, 0.037f)
                        .addKeyframe(627, 0.0f)
                        .addKeyframe(11373, 0.0f)
                        .addKeyframe(11732, 0.016f)
                        .addKeyframe(11959, 0.044f)
                        .addKeyframe(12399, 0.143f)
                        .addKeyframe(12729, 0.258f)
                        .addKeyframe(13228, 0.5f)
                        .addKeyframe(22772, 0.5f)
                        .addKeyframe(23032, 0.364f)
                        .addKeyframe(23356, 0.225f)
                        .addKeyframe(23758, 0.101f);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::CLOUD_COLOR(),
                _colorModifier(AttributeModifierOperation::Multiply),
                std::nullopt,
                [NIGHT_CLOUD_COLOR_MULTIPLIER](KeyframeTrack<i32>::Builder& builder) {
                    builder.addKeyframe(133, -1)
                        .addKeyframe(11867, -1)
                        .addKeyframe(13670, NIGHT_CLOUD_COLOR_MULTIPLIER)
                        .addKeyframe(22330, NIGHT_CLOUD_COLOR_MULTIPLIER);
                })
            .addTrack(attribute::EnvironmentAttributes::EYEBLOSSOM_OPEN(),
                [](KeyframeTrack<util::TriState>::Builder& builder) {
                    builder.addKeyframe(12600, util::TriState::True).addKeyframe(23401, util::TriState::False);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::CREAKING_ACTIVE(),
                _booleanModifier(AttributeModifierOperation::Or),
                std::nullopt,
                [](KeyframeTrack<bool>::Builder& builder) {
                    builder.addKeyframe(12600, true).addKeyframe(23401, false);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::TURTLE_EGG_HATCH_CHANCE(),
                _floatModifier(AttributeModifierOperation::Maximum),
                EasingType::CONSTANT,
                [](KeyframeTrack<f32>::Builder& builder) {
                    builder.addKeyframe(21062, 1.0f).addKeyframe(21905, 0.002f);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::CAT_WAKING_UP_GIFT_CHANCE(),
                _floatModifier(AttributeModifierOperation::Maximum),
                EasingType::CONSTANT,
                [](KeyframeTrack<f32>::Builder& builder) {
                    builder.addKeyframe(362, 0.0f).addKeyframe(23667, 0.7f);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::BEES_STAY_IN_HIVE(),
                _booleanModifier(AttributeModifierOperation::Or),
                std::nullopt,
                [](KeyframeTrack<bool>::Builder& builder) {
                    builder.addKeyframe(12542, true).addKeyframe(23460, false);
                })
            .addModifierTrack(attribute::EnvironmentAttributes::MONSTERS_BURN(),
                _booleanModifier(AttributeModifierOperation::Or),
                std::nullopt,
                [](KeyframeTrack<bool>::Builder& builder) {
                    builder.addKeyframe(12542, false).addKeyframe(23460, true);
                })
            .build();
    }();
    return instance;
}

const Timeline& Timelines::MOON()
{
    static const Timeline instance = [] {
        Timeline::Builder builder;
        builder.setPeriodTicks(MOON_PHASE_LENGTH * MOON_PHASE_COUNT);

        // 月相：每个相位开始即切换（阶跃）
        builder.addTrack(
            attribute::EnvironmentAttributes::MOON_PHASE(), [](KeyframeTrack<MoonPhase>::Builder& builder) {
                for (const MoonPhase phase : MOON_PHASES) {
                    builder.addKeyframe(moonPhaseStartTick(phase), phase);
                }
            });

        // 地表史莱姆生成概率：亮度越高的月相概率越高（最大值 0.5）
        builder.addModifierTrack(attribute::EnvironmentAttributes::SURFACE_SLIME_SPAWN_CHANCE(),
            _floatModifier(AttributeModifierOperation::Maximum),
            EasingType::CONSTANT,
            [](KeyframeTrack<f32>::Builder& builder) {
                for (const MoonPhase phase : MOON_PHASES) {
                    builder.addKeyframe(moonPhaseStartTick(phase),
                        MOON_BRIGHTNESS_PER_PHASE[static_cast<size_t>(moonPhaseIndex(phase))] * 0.5f);
                }
            });

        return builder.build();
    }();
    return instance;
}

const Timeline& Timelines::VILLAGER_SCHEDULE()
{
    using Activity = entity::ai::brain::schedule::Activity;

    static const Timeline instance = Timeline::Builder()
                                         .setPeriodTicks(24000)
                                         .addTrack(attribute::EnvironmentAttributes::VILLAGER_ACTIVITY(),
                                             [](KeyframeTrack<Activity>::Builder& builder) {
                                                 builder.addKeyframe(10, Activity::IDLE)
                                                     .addKeyframe(2000, Activity::WORK)
                                                     .addKeyframe(9000, Activity::MEET)
                                                     .addKeyframe(11000, Activity::IDLE)
                                                     .addKeyframe(12000, Activity::REST);
                                             })
                                         .addTrack(attribute::EnvironmentAttributes::BABY_VILLAGER_ACTIVITY(),
                                             [](KeyframeTrack<Activity>::Builder& builder) {
                                                 builder.addKeyframe(10, Activity::IDLE)
                                                     .addKeyframe(3000, Activity::PLAY)
                                                     .addKeyframe(6000, Activity::IDLE)
                                                     .addKeyframe(10000, Activity::PLAY)
                                                     .addKeyframe(12000, Activity::REST);
                                             })
                                         .build();
    return instance;
}

const Timeline& Timelines::EARLY_GAME()
{
    static const Timeline instance = [] {
        // 早期游戏：120000 tick（约 5 天）之后才允许生成劫掠巡逻队
        return Timeline::Builder()
            .addModifierTrack(attribute::EnvironmentAttributes::CAN_PILLAGER_PATROL_SPAWN(),
                _booleanModifier(AttributeModifierOperation::And),
                std::nullopt,
                [](KeyframeTrack<bool>::Builder& builder) {
                    builder.addKeyframe(0, false).addKeyframe(120000, true);
                })
            .build();
    }();
    return instance;
}

void Timelines::bootstrap()
{
    // 预定义时间线依赖属性注册表：属性以地址为键，而数据包加载器按名字解析引用。
    // 本函数在服务端启动与按需查询（如眼眸花环境）处均会调用，因此必须幂等。
    static std::once_flag bootstrapOnce;
    std::call_once(bootstrapOnce, [] {
        attribute::EnvironmentAttributes::registerAll();
        TimelineRegistry::registerAll();
    });

    // 触发各预定义时间线的构建（Meyers singleton，首次访问时完成初始化）。
    // 列举顺序与 minecraft:timeline 注册表的条目顺序一致。
    static const Timeline* const ALL_TIMELINES[] = {&DAY(), &MOON(), &VILLAGER_SCHEDULE(), &EARLY_GAME()};
    MC_UNUSED(ALL_TIMELINES);
}

} // namespace timeline
} // namespace world
} // namespace mc
