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
 * @brief 差分能力：`Mth.lerpInt` / `Mth.lerpDiscrete` / `Mth.binarySearch`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * public static int lerpInt(float delta, int start, int end) {
 *     return start + Mth.floor(delta * (float)(end - start));
 * }
 *
 * public static int lerpDiscrete(float delta, int start, int end) {
 *     int i = end - start;
 *     return start + Mth.floor(delta * (float)(i - 1)) + (delta > 0.0F ? 1 : 0);
 * }
 *
 * public static int binarySearch(int min, int max, IntPredicate predicate) { ... }
 * ```
 *
 * ## 三处关键点
 *
 * 1. **`lerpInt` 乘的是 `span`，`lerpDiscrete` 乘的是 `span - 1`。**
 *
 *    离散取值有 `to - from + 1` 个候选端点，要让 `t = 1` 恰好落在 `to` 上，
 *    跨度只能是 `span - 1`。混用会让按进度挑选离散档位（音高、粒子数量）永远取不到末项。
 *
 * 2. **`lerpDiscrete` 末尾对 `t > 0` 补 1，`lerpInt` 没有。**
 *
 *    `t = 0.001` 时 `lerpInt` 返回 `from`，`lerpDiscrete` 返回 `from + 1`；而 `t = 0`
 *    两者都返回 `from`。这是两个函数最容易被写成一体的地方。
 *
 * 3. **`t` 是 f32，结果不夹取。**
 *
 *    `t` 超出 `[0, 1]` 时结果会跑出 `[from, to]`；`span` 为 0 时 `lerpDiscrete`
 *    的乘数是 `-1`，同样不特殊处理。
 *
 * 单元格式：
 * - `lerpInt|<t>|<from>|<to>`
 * - `lerpDiscrete|<t>|<from>|<to>`
 * - `search|<from>|<to>|<threshold>` —— 谓词为 `value >= threshold`
 *
 * 全部返回整数，无需浮点归一化。
 */
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// 设计要点：
/// - `t` 取 0 / 1 / 0.5 / 极小正值 / 略小于 1，覆盖两个函数的全部分支差异；
/// - `to - from` 取正、负、1（`lerpDiscrete` 的 `span - 1 == 0` 退化）三种；
/// - `t` 略小于 1 是关键：`lerpInt` 得 `to - 1`，`lerpDiscrete` 得 `to`；
/// - `binarySearch` 的谓词写成 `>= threshold`，threshold 取区间内外与边界值。
constexpr const char* UNITS[] = {
    // lerpInt：基本
    "lerpInt|0|0|10",
    "lerpInt|1|0|10",
    "lerpInt|0.5|0|10",
    "lerpInt|0.25|0|10",
    "lerpInt|0.75|0|10",
    // lerpInt：略小于 1（应得 to-1）
    "lerpInt|0.999|0|10",
    // lerpInt：略大于 0（应得 from）
    "lerpInt|0.001|0|10",
    // lerpInt：负跨度
    "lerpInt|0|10|0",
    "lerpInt|1|10|0",
    "lerpInt|0.5|10|0",
    // lerpInt：零跨度
    "lerpInt|0.5|5|5",
    // lerpDiscrete：基本
    "lerpDiscrete|0|0|10",
    "lerpDiscrete|1|0|10",
    "lerpDiscrete|0.5|0|10",
    "lerpDiscrete|0.25|0|10",
    "lerpDiscrete|0.75|0|10",
    // lerpDiscrete：略小于 1（应得 to）
    "lerpDiscrete|0.999|0|10",
    // lerpDiscrete：略大于 0（应得 from+1，与 lerpInt 不同）
    "lerpDiscrete|0.001|0|10",
    // lerpDiscrete：极小。
    "lerpDiscrete|0.0001|0|10",
    // lerpDiscrete：负跨度
    "lerpDiscrete|0|10|0",
    "lerpDiscrete|1|10|0",
    "lerpDiscrete|0.5|10|0",
    // lerpDiscrete：span 为 1（span-1 == 0，退化）
    "lerpDiscrete|0|5|6",
    "lerpDiscrete|0.5|5|6",
    "lerpDiscrete|1|5|6",
    // lerpDiscrete：零跨度（span-1 == -1）
    "lerpDiscrete|0.5|5|5",
    // binarySearch：阈值在区间内
    "search|0|10|5",
    "search|0|10|0",
    "search|0|10|9",
    // binarySearch：阈值在区间外
    "search|0|10|-1",
    "search|0|10|10",
    "search|0|10|99",
    // binarySearch：空区间
    "search|5|5|3",
    // binarySearch：负区间
    "search|-10|10|0",
    "search|-10|10|-10",
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
    if (parts.size() < 2) {
        throw std::runtime_error("mth_map_lerp: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "lerpInt" && parts.size() == 4) {
        const f32 t = std::stof(parts[1]);
        const i32 from = std::stoi(parts[2]);
        const i32 to = std::stoi(parts[3]);
        return json{{"unit", unit}, {"value", math::lerpInt(t, from, to)}}.dump();
    }
    if (mode == "lerpDiscrete" && parts.size() == 4) {
        const f32 t = std::stof(parts[1]);
        const i32 from = std::stoi(parts[2]);
        const i32 to = std::stoi(parts[3]);
        return json{{"unit", unit}, {"value", math::lerpDiscrete(t, from, to)}}.dump();
    }
    if (mode == "search" && parts.size() == 4) {
        const i32 from = std::stoi(parts[1]);
        const i32 to = std::stoi(parts[2]);
        const i32 threshold = std::stoi(parts[3]);
        const i32 result = math::binarySearch(from, to, [threshold](i32 value) {
            return value >= threshold;
        });
        return json{{"unit", unit}, {"value", result}}.dump();
    }

    throw std::runtime_error("mth_map_lerp: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_map_lerp, &writeList, &runLine)

} // namespace mc::diff
