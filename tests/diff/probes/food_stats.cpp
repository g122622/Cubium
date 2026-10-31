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
 * @brief 差分能力：饥饿值与饱和度的变化（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行
 *         → { "foodLevel": 12, "saturationLevel": "3", "exhaustionLevel": "0", ... }
 *
 * 复刻原版 `FoodData` 的三处纯计算：
 *
 * ```java
 * // 1) add(food, saturation) —— 进食或再生调用
 * this.foodLevel       = Mth.clamp(food + this.foodLevel, 0, 20);
 * this.saturationLevel = Mth.clamp(saturation + this.saturationLevel, 0.0F, (float) this.foodLevel);
 *
 * // 2) eat(food, saturationModifier)
 * add(food, FoodConstants.saturationByModifier(food, saturationModifier));
 * // saturationByModifier(n, m) = n * m * 2.0F
 *
 * // 3) addExhaustion(e)
 * //    this.exhaustionLevel = Math.min(this.exhaustionLevel + e, 40.0F);
 * ```
 *
 * ## 三个容易写错的细节
 *
 * 1. **两次 clamp 的顺序**。饱和度上界用的是**刚更新过的** foodLevel，而不是旧的。
 *    先算饱和度再更新饥饿值会得到不同结果。
 *
 * 2. **add 有下界**。`Mth.clamp(x, 0, 20)` 会把负数抬到 0；只写 `min(x, 20)` 在
 *    `food + foodLevel < 0` 时得到负值。Cubium 的 `FoodStats::addStats` 正是只写了
 *    `std::min`，没有下界。
 *
 * 3. **addExhaustion 有 40 的上限**（`Math.min(level + e, 40.0F)`）。Cubium 的
 *    `addExhaustion` 同样是 `min(x, 40)`，这一处与原版一致。
 *
 * 单元格式：
 * - `add|<food>|<saturation>|<foodLevel>|<saturationLevel>`
 * - `eat|<nutrition>|<modifier>|<foodLevel>|<saturationLevel>`
 * - `exhaust|<delta>|<exhaustionLevel>`
 *
 * TODO: Cubium 的 `FoodStats` 已存在但上述三处与原版不同；待修正后本能力可直接调用它。
 * 本能力当前覆盖的是**公式与钳制边界**。
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

/// 原版 `FoodData` 的饥饿值上限。
constexpr i32 MAX_FOOD_LEVEL = 20;

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

/**
 * @brief 原版 `Mth.clamp`。
 */
i32 clampInt(i32 value, i32 minValue, i32 maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}

f32 clampFloat(f32 value, f32 minValue, f32 maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}

/**
 * @brief 原版 `FoodConstants.saturationByModifier`。
 */
f32 saturationByModifier(i32 nutrition, f32 modifier)
{
    return static_cast<f32>(nutrition) * modifier * 2.0f;
}

struct FoodState {
    i32 foodLevel = 20;
    f32 saturationLevel = 5.0f;
    f32 exhaustionLevel = 0.0f;
};

/**
 * @brief 原版 `add(food, saturation)`：两次 clamp，顺序不能反。
 */
void add(FoodState& state, i32 food, f32 saturation)
{
    // 先更新饥饿值，饱和度的上界要用**更新后**的值。
    state.foodLevel = clampInt(food + state.foodLevel, 0, MAX_FOOD_LEVEL);
    state.saturationLevel = clampFloat(saturation + state.saturationLevel, 0.0f, static_cast<f32>(state.foodLevel));
}

/// 单元表：覆盖上界钳制、下界钳制、饱和度的新旧边界关系、exhaustion 的累加。
constexpr const char* UNITS[] = {
    // 进食：正常范围
    "add|4|2.4|10|3",
    // 进食：饥饿值到顶（饱和度上界 = 20）
    "add|8|9.6|20|5",
    // 进食：饥饿值接近顶部，饱和度被新值压住
    "add|1|10|19|0",
    // 进食：负饥饿值触发**下界钳制**（只写 min 会得到负数）
    "add|-25|0|10|3",
    // 进食：负饥饿值但仍为正
    "add|-3|0|10|3",
    // 进食：负饱和度触发下界
    "add|0|-10|10|3",
    // 进食：两者同时到顶
    "add|100|100|0|0",
    // 进食：饥饿值归零时饱和度上界为 0
    "add|-20|5|10|3",
    // eat：营养 × 修正 × 2
    "eat|4|0.6|10|3",
    "eat|8|0.8|5|0",
    "eat|1|0.1|19|0",
    "eat|20|0.5|0|0",
    "eat|6|1|14|2",
    "eat|2|0|10|3",
    // exhaustion：原版无上限
    "exhaust|4|0",
    "exhaust|4|3.5",
    "exhaust|1.5|3",
    "exhaust|100|0",
    "exhaust|0.25|39.9",
    "exhaust|0|7",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);

    FoodState state;
    std::string mode;

    if (parts.size() == 5 && (parts[0] == "add" || parts[0] == "eat")) {
        mode = parts[0];
        state.foodLevel = std::stoi(parts[3]);
        state.saturationLevel = static_cast<f32>(std::stod(parts[4]));
        if (mode == "add") {
            add(state, std::stoi(parts[1]), static_cast<f32>(std::stod(parts[2])));
        } else {
            const i32 nutrition = std::stoi(parts[1]);
            const f32 modifier = static_cast<f32>(std::stod(parts[2]));
            add(state, nutrition, saturationByModifier(nutrition, modifier));
        }
    } else if (parts.size() == 3 && parts[0] == "exhaust") {
        mode = parts[0];
        state.exhaustionLevel = static_cast<f32>(std::stod(parts[2]));
        // 原版 addExhaustion：Math.min(level + e, 40.0F)。
        state.exhaustionLevel = std::min(state.exhaustionLevel + static_cast<f32>(std::stod(parts[1])), 40.0f);
    } else {
        throw std::runtime_error("food_stats: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit},
        {"mode", mode},
        {"foodLevel", state.foodLevel},
        {"saturationLevel", formatFloat(static_cast<f64>(state.saturationLevel))},
        {"exhaustionLevel", formatFloat(static_cast<f64>(state.exhaustionLevel))}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("food_stats", &writeList, &runLine)

} // namespace mc::diff
