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
 * @brief 差分能力：附魔互斥性判定（`Enchantment.areCompatible`，纯计算）。
 *
 * `list`：单元由 harness 侧 `listEnchantmentCompatUnits()` 从数据包计算得出
 * `run` ：单元 → { "value": "true"/"false" }
 *
 * 复刻原版：
 *
 * ```java
 * public static boolean areCompatible(Holder<Enchantment> a, Holder<Enchantment> b) {
 *     return !a.equals(b)
 *         && !a.value().exclusiveSet.contains(b)
 *         && !b.value().exclusiveSet.contains(a);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **同一附魔永远不兼容自己。** 第一个条件 `!a.equals(b)` 让 `areCompatible(x, x)` 恒为
 *    `false`。这不是「互斥」，而是「不能重复附魔同一个」，但走同一条返回路径。
 *
 * 2. **检查是双向的，且是「或」的关系。** 只要 `a.exclusiveSet` 含 `b` **或** `b.exclusiveSet`
 *    含 `a`，就不兼容。两个方向的 `exclusiveSet` 不必对称：单向声明足以阻断双向组合，所以
 *    数据包里只需在一侧写入即可。
 *
 * 3. **`exclusiveSet` 常是 `#tag` 引用。** 数据包里写的是
 *    `"exclusive_set": "#minecraft:exclusive_set/damage"` 这种形态，必须展开标签后
 *    才得到实际的附魔集合。展开由 harness 侧完成（`listEnchantmentCompatUnits`），
 *    探针收到的是已经展开的扁平列表。
 *
 * 4. **兼容性不涉及物品。** `areCompatible` 只看两个附魔之间的关系，与它们分别能否附到同一
 *    物品上无关——那是 `canEnchant` / `supportedItems` 的职责（见
 *    `enchantment_applicability`）。两个都适用于某物品的附魔，仍可能因互斥而不能共存。
 *
 * 单元格式：`<a>|<b>|<aExcl>|<bExcl>`
 *
 * - `a`、`b` 是附魔 id
 * - `aExcl`、`bExcl` 是各自展开后的 `exclusive_set`，逗号分隔；空串表示空集
 *
 * TODO: 探针不校验附魔 id 是否真实存在（那是 oracle 侧注册表的职责）；若 a/b 不在注册表里，
 * oracle 输出 `absent`，探针会输出 `true`/`false`，框架会报差异。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 把一个逗号分隔的列表拆开；空串表示空列表。
std::vector<std::string> splitNames(const std::string& text)
{
    if (text.empty()) {
        return {};
    }
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

bool contains(const std::vector<std::string>& names, const std::string& target)
{
    return std::find(names.begin(), names.end(), target) != names.end();
}

/**
 * @brief 原版 `Enchantment.areCompatible`。
 *
 * 附魔用 id 字符串标识，相等即视为同一个。
 */
bool areCompatible(const std::string& a,
    const std::string& b,
    const std::vector<std::string>& aExclusive,
    const std::vector<std::string>& bExclusive)
{
    return a != b && !contains(aExclusive, b) && !contains(bExclusive, a);
}

void writeList(FILE* /*out*/)
{
    // 单元由 harness 侧从数据包计算，见 harnesses/enchantment_compat.ts。
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
    if (parts.size() != 4) {
        throw std::runtime_error("enchantment_compat: 单元应为 4 段 '<a>|<b>|<aExcl>|<bExcl>'：" + unit);
    }

    const bool result = areCompatible(parts[0], parts[1], splitNames(parts[2]), splitNames(parts[3]));

    return json{{"unit", unit}, {"value", result ? "true" : "false"}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(enchantment_compat, &writeList, &runLine)

} // namespace mc::diff
