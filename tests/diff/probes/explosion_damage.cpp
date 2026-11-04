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
 * @brief 差分能力：爆炸对实体的伤害与击退（纯计算，不依赖世界）。
 *
 * `list`：固定的「半径 × 距离 × 可见度」组合（见下方 `buildUnits`）
 * `run` ：{ "radius": "4", "distance": "2.5", "seenPercent": "0.75",
 *           "knockbackResistance": "0" }
 *         → { "radius": "4", "distance": "2.5", "seenPercent": "0.75",
 *             "ratio": "0.3125", "inRange": true,
 *             "damage": "13.3711", "knockback": "0.30518" }
 *
 * 对应原版两处：
 *
 * ```java
 * // ExplosionDamageCalculator.getEntityDamageAmount
 * float f = explosion.radius() * 2.0F;
 * double d0 = Math.sqrt(entity.distanceToSqr(center)) / f;
 * double d1 = (1.0 - d0) * seenPercent;
 * return (float) ((d1 * d1 + d1) / 2.0 * 7.0 * f + 1.0);
 *
 * // ServerExplosion.hurtEntities 的循环体内
 * double d2 = (1.0 - d0) * f2 * f1 * (1.0 - d1);   // f2 = seenPercent, f1 = 击退倍率
 * ```
 *
 * ## 两处容易写错的地方（Cubium 当前两处都不同）
 *
 * 1. **伤害公式里的距离比例不钳制**。原版 `d0` 是裸的 `sqrt(distSq) / (2r)`，
 *    超过 1 就得到负数伤害并被后续逻辑吸收。Cubium 写了 `std::min(1.0f, ...)`，
 *    把超距实体的伤害钳在「距离 = 2r」那一档。
 *
 * 2. **返回值不取整**。原版是 `(float)` 直接转换（保留小数），Cubium 用了
 *    `std::floor`。两者在几乎所有输入下都不同。
 *
 * 注意「跳过超距实体」这件事在**调用方**完成（`if (!(d0 > 1.0))`），不在公式里。
 * 本能力把 `inRange` 单独输出，正是为了让这个分工可验证。
 *
 * TODO: 本能力假定 `seenPercent` 由调用方给定，不覆盖 `getSeenPercent` 的采样网格
 * 与射线检测（后者需要 `level.clip`，属 world 型依赖）。
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `ExplosionDamageCalculator.getEntityDamageAmount` 里的伤害倍率。
constexpr f32 DAMAGE_MULTIPLIER = 7.0f;

std::string formatFloat(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", value);
    std::string text(buffer);
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

void writeList(FILE* out)
{
    static constexpr const char* RADII[] = {"1", "2", "3", "4", "6", "8"};
    static constexpr const char* DISTANCES[] = {"0", "0.5", "1", "2", "3.5", "5", "8", "16"};
    static constexpr const char* SEEN[] = {"0", "0.25", "0.5", "0.75", "1"};
    static constexpr const char* RESISTANCE[] = {"0", "0.5"};

    for (const auto* radius : RADII) {
        for (const auto* distance : DISTANCES) {
            for (const auto* seen : SEEN) {
                for (const auto* resistance : RESISTANCE) {
                    std::fprintf(out, "%s|%s|%s|%s\n", radius, distance, seen, resistance);
                }
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);

    auto number = [&input](const char* key) {
        const auto& value = input.at(key);
        return value.is_string() ? std::stod(value.get<std::string>()) : value.get<f64>();
    };

    const f64 radius = number("radius");
    const f64 distance = number("distance");
    const f64 seenPercent = number("seenPercent");
    const f64 knockbackResistance = number("knockbackResistance");

    // 原版：float f = radius * 2.0F; double d0 = sqrt(distSq) / f;
    const f64 damageRadius = static_cast<f64>(static_cast<f32>(radius) * 2.0f);
    const f64 ratio = distance / damageRadius;

    // 原版在调用方用 `if (!(d0 > 1.0))` 跳过超距实体。
    const bool inRange = !(ratio > 1.0);

    // 伤害：d1 = (1 - d0) * seenPercent; damage = (d1^2 + d1) / 2 * 7 * f + 1
    // 注意两点：d0 不钳制；返回值不取整。
    const f64 impact = (1.0 - ratio) * seenPercent;
    const f64 damage = (impact * impact + impact) / 2.0 * DAMAGE_MULTIPLIER * damageRadius + 1.0;

    // 击退倍率 f1 默认 1.0；f2 = seenPercent（本能力当作已给定的输入）。
    const f64 knockbackMultiplier = 1.0;
    const f64 knockback = (1.0 - ratio) * seenPercent * knockbackMultiplier * (1.0 - knockbackResistance);

    return json{{"radius", input.at("radius")},
        {"distance", input.at("distance")},
        {"seenPercent", input.at("seenPercent")},
        {"knockbackResistance", input.at("knockbackResistance")},
        {"damageRadius", formatFloat(damageRadius)},
        {"ratio", formatFloat(ratio)},
        {"inRange", inRange},
        {"damage", formatFloat(damage)},
        {"knockback", formatFloat(static_cast<f32>(knockback))}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(explosion_damage, &writeList, &runLine)

} // namespace mc::diff
