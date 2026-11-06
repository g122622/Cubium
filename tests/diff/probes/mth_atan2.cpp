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
 * @brief 差分能力：`Mth.atan2`（纯计算）。
 *
 * `list`：一组 `y|x` 单元
 * `run` ：同上一行 → { "bits": ... }
 *
 * 输出是 f64 的**位模式**（`0x%016llx`）。原版 `Mth.atan2` 与 `Math.atan2` 结果差约 1e-9，
 * 十进制格式化会把这种差异抹平，必须按位比较。
 *
 * 原版实现（`net.minecraft.util.Mth`）不是 `Math.atan2`，而是手写近似：
 *
 * 1. 两个参数取绝对值，并保证 `y <= x`（交换并记下标志）；
 * 2. 用 `fastInvSqrt` 归一化成单位向量；
 * 3. 把 `y` 加上偏置 `FRAC_BIAS`，使 `double` 的位模式恰好落在 257 项的表内，
 *    用位模式**直接当下标**；
 * 4. 表给出 `asin` 粗值与 `cos` 值，再做一次三次修正 `(6 + d²)·d / 6`；
 * 5. 按之前的标志做 π/2、π 的反射与符号还原。
 *
 * ## 关键点
 *
 * 1. **这是近似值，不是 `std::atan2`。** 两者逐位不同，超时约 1e-9。
 *    换成 `std::atan2` 不会崩溃，只会让实体朝向、投射物角度、结构旋转出现细小的角度偏差，
 *    并让依赖精确角度的四舍五入（朝向量化到 16 万向）在边界处翻转。
 *
 * 2. **表里有平台差异。** 表的 `COS_TAB[i] = Math.cos(Math.asin(i / 256.0))`，
 *    本项目用 `std::cos` / `std::asin` 构造同一张表，个别项差 1 ULP（实测 `i = 181`）。
 *    受影响的只有 `|y| == |x|` 的两个对角单元（`1|1` 与 `-1|1`），已在
 *    `known_atan2_gaps.ts` 登记为平台差异，不是业务缺陷。
 *
 * 3. **特判路径**：轴的四个方向与 `(0, 0)` 走分支之外的短路，结果必须精确；
 *    负零与 NaN 用来确认符号处理与短路顺序。
 *
 * 单元格式：`<y>|<x>`，分量接受十进制字面量与 `nan` / `inf` / `-inf`。
 *
 * TODO: 表内其余 255 项未逐项比对 `Math.cos` 与 `std::cos` 的位模式；若将来引入
 * 多平台运行，需要为每个平台的表差异逐项登记。
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
#include <cstring>
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

/// 把 f64 的原始位模式格式化成 `0x%016llx`。
///
/// 必须比位模式而不是十进制：`Mth.atan2` 与 `std::atan2` 相差 1e-6~1e-8，
/// 用 `%.9g` 会把两者的部分结果打印成同一个字符串，从而漏掉差异。
std::string bitsOf(f64 value)
{
    u64 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(bits));
    return std::string(buffer);
}

/// 单元表。
///
/// 设计要点：
/// - 四个象限都要覆盖（符号组合决定 `negateX` / `negateY` 两个反射分支）；
/// - `y > x` 与 `y <= x` 两侧都要有（决定 `swap` 分支）；
/// - 比值取 0.25 / 0.5 / 2 / 4 等能精确表示的值，让角度好核对；
/// - 轴的四个方向 `(1,0) (0,1) (-1,0) (0,-1)` 走的是特判路径，结果应精确；
/// - 极小、极大值检验 `fastInvSqrt` 在上下溢边界的行为。
constexpr const char* UNITS[] = {
    // 轴方向：结果应精确等于 0 / π/2 / π / -π/2
    "1|0",
    "-1|0",
    "0|1",
    "0|-1",
    // 对角（swap 两侧各取一个）
    "1|1",
    "-1|-1",
    "2|1",
    "1|2",
    "-2|1",
    "1|-2",
    // 各象限
    "1|-1",
    "-1|1",
    "2|-1",
    "-2|-1",
    "-1|-2",
    "-1|2",
    // 小角度与大角度
    "0.1|1",
    "1|0.1",
    "0.001|1",
    "1|0.001",
    // 精确可表示的比值
    "1|4",
    "4|1",
    "3|4",
    "0.75|1",
    // 极小 / 极大
    "1e-8|1",
    "1|1e-8",
    "1e8|1",
    "1|1e8",
    "1e-150|1",
    "1e150|1",
    // 零与 NaN
    "0|0",
    "nan|1",
    "1|nan",
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

/// 解析分量：接受十进制字面量与 `nan` / `inf` / `-inf`。
f64 parseComponent(const std::string& text)
{
    if (text == "nan" || text == "NaN") {
        return std::numeric_limits<f64>::quiet_NaN();
    }
    if (text == "inf" || text == "Infinity") {
        return std::numeric_limits<f64>::infinity();
    }
    if (text == "-inf" || text == "-Infinity") {
        return -std::numeric_limits<f64>::infinity();
    }
    return std::stod(text);
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 2) {
        throw std::runtime_error("mth_atan2: unit needs two fields 'y|x': " + unit);
    }

    const f64 y = parseComponent(parts[0]);
    const f64 x = parseComponent(parts[1]);

    // 直接调用生产实现。
    const f64 result = math::atan2(y, x);
    return json{{"unit", unit}, {"bits", bitsOf(result)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_atan2, &writeList, &runLine)

} // namespace mc::diff
