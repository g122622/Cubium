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
 * @brief 差分能力：`ObjectiveCriteria.RenderType` 的字符串双向映射。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`mc::scoreboard::renderTypeToString` / `renderTypeParse` / `renderTypeFromString`）。
 *
 * Cubium 侧（`common/scoreboard/core/ScoreCriteriaRenderType.hpp`）：
 *
 * ```cpp
 * enum class RenderType : u8 { Integer = 0, Hearts = 1 };
 * const char* renderTypeToString(RenderType) noexcept;          // 未知 → "integer"
 * std::optional<RenderType> renderTypeParse(const std::string&) noexcept;  // 严格
 * RenderType renderTypeFromString(const std::string&) noexcept; // 未知 → Integer
 * ```
 *
 * 原版侧：
 *
 * ```java
 * public enum RenderType implements StringRepresentable {
 *     INTEGER("integer"),
 *     HEARTS("hearts");
 *     public static final StringRepresentable.EnumCodec<RenderType> CODEC;
 *     public String getId();
 *     public String getSerializedName();      // 与 getId() 同值
 *     public static RenderType byId(String);  // CODEC.byName，未知返回 null
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **只有两项，且顺序是 `INTEGER` 在前。**
 *
 *    | 枚举 | `getSerializedName()` |
 *    |---|---|
 *    | `INTEGER` | `integer` |
 *    | `HEARTS` | `hearts` |
 *
 *    名字是全小写单词，与枚举名的大小写不同。
 *
 * 2. **`renderTypeFromString` 对未知输入回退到 `Integer`——原版 `byId` 也是同样行为。**
 *
 *    原版 `RenderType.byId(String)` 的实现是
 *    `CODEC.byName(input, INTEGER)`——**带默认值的重载**，未知输入返回 `INTEGER`，
 *    而不是 `null`。
 *
 *    **这与 [`team_rule_strings`](./team_rule_strings.md) 的 `CODEC.byName(input)` 不同**：
 *    那里是单参数重载（未知返回 `null`），这里是双参数重载（未知返回默认值）。
 *    Cubium 的宽松入口 `renderTypeFromString` 恰好与原版一致。
 *
 *    Cubium 另提供的严格入口 `renderTypeParse` 返回 `std::optional`，
 *    供需要区分「非法输入」的调用方使用，原版无对应入口。
 *
 * 3. **数据修复 `ObjectiveRenderTypeFix` 把旧值 `"health"` 映射为 `"hearts"`。**
 *
 *    ```java
 *    return objective.equals("health") ? "hearts" : "integer";
 *    ```
 *
 *    说明历史上的名字是 `health`，现在是 `hearts`。**`health` 不是合法序列化名**。
 *
 * 单元格式：
 * - `toString|<enum>` —— `renderTypeToString`
 * - `parse|<text>` —— `renderTypeFromString`（与原版 `byId` 一致，未命中回退 `INTEGER`）
 * - `count` —— 枚举项数
 * - `names` —— 全部序列化名，逗号分隔
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// `RenderType` 的枚举项。
struct RenderTypeRef {
    const char* javaName;
    scoreboard::RenderType cubium;
};

constexpr RenderTypeRef RENDER_TYPES[] = {
    {"INTEGER", scoreboard::RenderType::Integer},
    {"HEARTS", scoreboard::RenderType::Hearts},
};

constexpr i32 RENDER_TYPE_COUNT = 2;

const RenderTypeRef* findRenderType(const std::string& name)
{
    for (const RenderTypeRef& entry : RENDER_TYPES) {
        if (name == entry.javaName) {
            return &entry;
        }
    }
    return nullptr;
}

/// 把 Cubium 的枚举值转回原版枚举名。
std::string toJavaName(scoreboard::RenderType value)
{
    for (const RenderTypeRef& entry : RENDER_TYPES) {
        if (entry.cubium == value) {
            return entry.javaName;
        }
    }
    return "null";
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        "count",
        "names",
        "toString|INTEGER",
        "toString|HEARTS",
        // 反向命中
        "parse|integer",
        "parse|hearts",
        // 反向未命中：原版 byId 用带默认值的 byName 重载，回退到 INTEGER
        "parse|dunno",
        "parse|",
        "parse|INTEGER",
        "parse|HEARTS",
        "parse|health",
    };
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
    if (parts.empty()) {
        throw std::runtime_error("render_type_strings: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(RENDER_TYPE_COUNT);
    } else if (mode == "names" && parts.size() == 1) {
        std::string names;
        for (const RenderTypeRef& entry : RENDER_TYPES) {
            if (!names.empty()) {
                names.push_back(',');
            }
            names += scoreboard::renderTypeToString(entry.cubium);
        }
        value = names;
    } else if (parts.size() != 2) {
        throw std::runtime_error("render_type_strings: 单元段数不符：" + unit);
    } else if (mode == "toString") {
        const RenderTypeRef* entry = findRenderType(parts[1]);
        if (entry == nullptr) {
            throw std::runtime_error("render_type_strings: 未知渲染类型：" + parts[1]);
        }
        value = scoreboard::renderTypeToString(entry->cubium);
    } else if (mode == "parse") {
        // 原版 RenderType.byId 走 byName(String, INTEGER) 重载——未知输入回退 INTEGER。
        // 因此这里用 Cubium 的宽松入口 renderTypeFromString，语义才与原版一致。
        value = toJavaName(scoreboard::renderTypeFromString(parts[1]));
    } else {
        throw std::runtime_error("render_type_strings: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(render_type_strings, &writeList, &runLine)

} // namespace mc::diff
