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
 * @brief 差分能力：属性修饰符的三阶段计算（`AttributeInstance` 最终值）。
 *
 * `list`：一组场景
 * `run` ：一行 -> { "value": 位模式 }
 *
 * ## 原版算法（`AttributeInstance.calculateValue`）
 *
 * ```java
 * double d0 = base;
 * for (m : ADD_VALUE)             d0 += m.amount();
 * double d1 = d0;
 * for (m : ADD_MULTIPLIED_BASE)   d1 += d0 * m.amount();     // 乘的是含加法的 d0
 * for (m : ADD_MULTIPLIED_TOTAL)  d1 *= 1.0 + m.amount();    // 逐个连乘
 * return attribute.sanitizeValue(d1);
 * ```
 *
 * 三个容易写错的点：
 *
 * 1. `ADD_MULTIPLIED_BASE` 乘的是**经过 ADD_VALUE 之后的 d0**，不是原始 base
 *    ——这正是操作名前缀 `add_multiplied_base` 的含义；
 * 2. `ADD_MULTIPLIED_TOTAL` 是**逐个乘 (1+amount)**，不是把 amount 相加后乘一次；
 * 3. `sanitizeValue` 对 **NaN 返回 minValue**（`RangedAttribute.sanitizeValue`），
 *    普通 clamp 会把 NaN 原样放行。
 *
 * ## 单元格式
 *
 * `base|op:amount;op:amount;...|min|max`
 *
 * - `op` 取 `add` / `base` / `total`，分别对应三个 `Operation`；
 * - 修饰符为空时写 `-`；
 * - amount 允许 `nan` / `inf` 以覆盖边界。
 */

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/attribute/Attribute.hpp"
#include "common/entity/attribute/AttributeInstance.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/** 把 f64 转成位模式字符串，避免两侧十进制表示的格式差异。 */
std::string f64Bits(f64 value)
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(bits));
    return buffer;
}

f64 parseNumber(const std::string& text)
{
    if (text == "nan") {
        return std::nan("");
    }
    if (text == "inf") {
        return std::numeric_limits<f64>::infinity();
    }
    if (text == "-inf") {
        return -std::numeric_limits<f64>::infinity();
    }
    return std::stod(text);
}

std::vector<std::string> splitBy(std::string_view text, char separator)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == separator) {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

/**
 * 场景表：`base|modifiers|min|max`。
 *
 * 覆盖要点：单一操作基线、加法与基础乘法并存（验证乘的是含加法的 d0）、
 * 多档总乘法（验证逐个连乘）、三操作混合、上下钳制、NaN 与无穷边界。
 */
constexpr const char* SCENES[] = {
    // --- 基线：单一操作 ---
    "10|-|0|100",
    "10|add:5|0|100",
    "10|base:0.5|0|100",
    "10|total:0.5|0|100",

    // --- 关键：加法 + 基础乘法并存 ---
    // 原版 d0 = 15，d1 = 15 + 15*0.5 = 22.5；误用原始 base 得 20
    "10|add:5;base:0.5|0|100",
    "10|add:100;base:0.5|0|1000",
    "0|add:10;base:1.0|0|100",

    // --- 关键：多档总乘法必须逐个连乘 ---
    "10|total:0.5;total:0.5|0|100",
    "10|total:1;total:1;total:1|0|100",
    "10|total:0.25;total:0;total:0.25|0|100",

    // --- 三种操作混合 ---
    "10|add:5;base:0.5;total:0.5|0|100",
    "20|add:-5;base:-0.5;total:-0.5|0|100",

    // --- 钳制 ---
    "10|add:1000|0|100",
    "10|add:-1000|0|100",
    "3|add:1000|-5|5",
    "100|add:0|0|100",

    // --- 精度与顺序 ---
    "0.1|add:0.2|0|100",
    "1|total:0.1;total:0.1|0|100",
    "3|add:0.1;add:0.2;base:0.3|0|100",

    // --- 边界：NaN / 无穷 ---
    "10|add:nan|0|100",
    "10|add:inf|0|100",
    "10|total:inf|0|100",
};

void writeList(FILE* out)
{
    for (const char* scene : SCENES) {
        std::fprintf(out, "%s\n", scene);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::vector<std::string> parts = splitBy(unit, '|');
    if (parts.size() != 4) {
        throw std::runtime_error("attribute_modifier_apply: 单元应为 4 段 'base|mods|min|max'：" + unit);
    }

    const f64 base = parseNumber(parts[0]);
    const f64 minValue = parseNumber(parts[2]);
    const f64 maxValue = parseNumber(parts[3]);

    const entity::attribute::Attribute attribute("minecraft:test", base, minValue, maxValue);
    entity::attribute::AttributeInstance instance(attribute);
    instance.setBaseValue(base);

    if (parts[1] != "-") {
        i32 index = 0;
        for (const std::string& token : splitBy(parts[1], ';')) {
            if (token.empty()) {
                continue;
            }
            const std::vector<std::string> pieces = splitBy(token, ':');
            if (pieces.size() != 2) {
                throw std::runtime_error("attribute_modifier_apply: 修饰符格式应为 op:amount：" + token);
            }

            entity::attribute::Operation operation{};
            if (pieces[0] == "add") {
                operation = entity::attribute::Operation::Addition;
            } else if (pieces[0] == "base") {
                operation = entity::attribute::Operation::MultiplyBase;
            } else if (pieces[0] == "total") {
                operation = entity::attribute::Operation::MultiplyTotal;
            } else {
                throw std::runtime_error("attribute_modifier_apply: 未知操作：" + pieces[0]);
            }

            instance.addModifier(entity::attribute::AttributeModifier(
                "test_" + std::to_string(index), "test", parseNumber(pieces[1]), operation));
            ++index;
        }
    }

    return json{{"unit", unit}, {"value", f64Bits(instance.getValue())}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(attribute_modifier_apply, &writeList, &runLine)

} // namespace mc::diff
