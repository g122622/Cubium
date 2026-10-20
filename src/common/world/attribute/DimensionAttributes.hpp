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

#pragma once

#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/util/TriState.hpp"
#include "common/world/attribute/EnvironmentAttributeMap.hpp"
#include "common/world/attribute/EnvironmentAttributeRegistry.hpp"
#include "common/world/attribute/EnvironmentAttributeSystem.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/level/MoonPhase.hpp"
#include "common/world/timeline/Timeline.hpp"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

namespace mc {

class DimensionType;

namespace world {
namespace attribute {

/**
 * @brief 维度级环境属性装配
 *
 * 负责把维度类型声明的常量属性（`dimension_type.attributes`）与时间线
 * （`dimension_type.timelines`）组装成 EnvironmentAttributeSystem 的默认层。
 *
 * 层序与 vanilla 一致：维度常量层 → 群系位置层（按查询位置的群系属性表）→ 时间线层。
 * 天气层（`WeatherAttributes`）尚未接入；装饰性视觉效果（天空色、雾色等）的时间线层照常构建，
 * 供诊断与后续渲染/天气实现查询。
 */
class DimensionAttributes {
public:
    /// 群系解析器：世界坐标 → 该位置所属群系
    using BiomeResolver = std::function<BiomeId(const Vector3& pos)>;

    /**
     * @brief 按维度装配环境属性系统
     *
     * 层序与 vanilla 一致：维度常量层 → 群系位置层 → 维度声明的时间线层。
     *
     * @param dimensionType 维度类型（常量属性表与时间线声明）
     * @param dayTimeGetter dayTime 读取函数（周期时间线使用）
     * @param gameTimeGetter gameTime 读取函数（无周期时间线使用）
     * @param biomeResolver 群系解析器；为空时不装配群系位置层（如无维度上下文的查询）
     * @return 环境属性系统
     */
    [[nodiscard]] static std::unique_ptr<EnvironmentAttributeSystem> createSystem(const DimensionType& dimensionType,
        std::function<i64()> dayTimeGetter,
        std::function<i64()> gameTimeGetter,
        const BiomeResolver& biomeResolver = {});
};

/// 支持时间线驱动的属性值类型（与预定义属性集一致）
template <typename Value>
constexpr bool isTimelineValue()
{
    return std::is_same_v<Value, bool> || std::is_same_v<Value, f32> || std::is_same_v<Value, i32> ||
        std::is_same_v<Value, util::TriState> || std::is_same_v<Value, MoonPhase> ||
        std::is_same_v<Value, entity::ai::brain::schedule::Activity>;
}

namespace detail {

/// 维度内是否有任一群系声明了该属性（用于决定是否装配群系位置层）
[[nodiscard]] bool anyBiomeDeclaresAttribute(const void* attribute);

/**
 * @brief 为单个属性追加"常量层 → 群系位置层 → 时间线层"
 *
 * 时间线以属性地址为键存储类型擦除的轨道，轨道的值/参数类型只有属性本身知道，
 * 因此这里以属性自身的值类型实例化：
 * 1. 常量层：属性出现在维度属性表中时，按表声明的修改器修改默认值；
 * 2. 群系位置层：维度内任一群系声明了该属性时，按查询位置所属群系的属性表修改；
 * 3. 时间线层：维度声明了该属性所属时间线且时间线内含此属性轨道时，创建轨道采样器。
 *
 * 层序与 vanilla 一致：常量层 → 群系层 → 时间线层。
 */
template <typename Value>
void appendAttributeLayers(EnvironmentAttributeSystemBuilder& builder,
    const EnvironmentAttribute<Value>& attribute,
    const EnvironmentAttributeMap& constants,
    const std::vector<const timeline::Timeline*>& timelines,
    const std::function<i64()>& dayTimeGetter,
    const std::function<i64()>& gameTimeGetter,
    const DimensionAttributes::BiomeResolver& biomeResolver)
{
    if (constants.contains(&attribute)) {
        builder.addConstantLayer(attribute, constants);
    }

    // 群系层：维度内任一群系声明了该属性时，按查询位置的群系应用其属性表。
    // 群系解析器由调用方提供（服务端按 BiomeManager 的 Voronoi 抖动查询，生成期按区块群系查询）。
    if (biomeResolver && detail::anyBiomeDeclaresAttribute(&attribute)) {
        builder.addPositionalLayer<Value>(
            attribute, [&attribute, biomeResolver](const Value& value, const Vector3& pos) -> Value {
                const BiomeId biomeId = biomeResolver(pos);
                const biome::Biome& biome = biome::BiomeRegistry::instance().get(biomeId);
                Value result = value;
                biome.attributes().applyModifier(&attribute, &result);
                return result;
            });
    }

    for (const timeline::Timeline* timeline : timelines) {
        if (!timeline->hasTrack(&attribute)) {
            continue;
        }

        if constexpr (isTimelineValue<Value>()) {
            const i64 periodTicks = timeline->periodTicks().value_or(0);
            const std::function<i64()>& clock = periodTicks > 0 ? dayTimeGetter : gameTimeGetter;
            builder.addTimeBasedLayer(attribute, timeline->createTrackSampler<Value, Value>(attribute, clock));
        } else {
            spdlog::warn(
                "Environment attribute '{}' has an unsupported value type for timeline layers", attribute.name());
        }
    }
}

} // namespace detail

} // namespace attribute
} // namespace world
} // namespace mc
