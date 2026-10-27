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
 * @file damage_type.cpp
 * @brief 差分能力：伤害类型的数据包字段（`data/minecraft/damage_type/*.json`）。
 *
 * `list`：已注册的全部伤害类型（`minecraft:<path>`）
 * `run` ：{ "damageType": "minecraft:in_fire" }
 *         → 难度缩放策略、体力消耗系数、附加效果
 *
 * 这三项直接决定：难度切换时伤害是否缩放、玩家被打后掉多少饱和度、
 * 以及「燃烧 / 冻结 / 中毒」这类附加效果挂不挂得上。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTag.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using mc::DamageScaling;
using mc::DamageType;
namespace DamageTypeNames = mc::DamageTypeNames;

/// Cubium 的 DamageType 是硬编码枚举，枚举值本身不是稀疏的，按 0..N-1 遍历。
constexpr int DAMAGE_TYPE_COUNT = 50;

std::string nameOf(DamageType type)
{
    return DamageTypeNames::getResourceLocation(type).toString();
}

/// 与 oracle 同格式：把 f32 打印成 5 位小数，两侧比的是同一个 f32 值而不是二进制展开。
f64 roundToFloat5(f32 value)
{
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.5f", static_cast<double>(value));
    return std::stod(buffer);
}

void writeList(FILE* out)
{
    std::vector<std::string> names;
    for (int i = 0; i < DAMAGE_TYPE_COUNT; ++i) {
        const std::string name = nameOf(static_cast<DamageType>(i));
        if (!name.empty()) {
            names.push_back(name);
        }
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

const char* scalingName(DamageScaling scaling)
{
    switch (scaling) {
        case DamageScaling::Always:
            return "always";
        case DamageScaling::WhenCausedByLivingNonPlayer:
            return "when_caused_by_living_non_player";
        case DamageScaling::Never:
        default:
            return "never";
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("damageType").get<std::string>();

    const auto type = DamageTypeNames::fromString(requested);
    if (!type.has_value()) {
        return json{{"damageType", requested}, {"missing", true}}.dump();
    }
    return json{{"damageType", requested},
        {"scaling", scalingName(mc::damageScaling(*type))},
        {"exhaustion", roundToFloat5(mc::damageExhaustion(*type))},
        {"effects", mc::damageEffectsName(mc::damageEffects(*type))}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("damage_type", &writeList, &runLine)

} // namespace mc::diff
