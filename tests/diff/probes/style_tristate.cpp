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
 * @brief 差分能力：`Style` 三态格式标志的**序列化**行为。
 *
 * **本探针调用 Cubium 的真实实现**（`mc::text::Style` 的 setter + `toJson()`）。
 *
 * ## 为什么比序列化而不是比 getter
 *
 * 原版 `Style` 的三态字段是**包私有**的，没有公开的三态访问器：
 *
 * ```java
 * final Boolean bold;                  // 包私有，外部读不到
 * public boolean isBold() {            // bold == Boolean.TRUE
 *     return this.bold == Boolean.TRUE;
 * }
 * ```
 *
 * **`isBold()` 无法区分「未设置」与「显式 false」**——两者都返回 `false`。
 *
 * Cubium 则把三态暴露在 getter 上（`bold()` 返回 `std::optional<bool>`），
 * 这是**接口设计的差异，不是缺陷**。真正必须一致的是**序列化结果**：
 * 原版存档里 `"bold": false` 与「没有 `bold` 键」是两种不同的值。
 *
 * 因此本能力比对 `toJson()` 的输出，而不是内部字段。
 *
 * ## 三个关键点
 *
 * 1. **未设置 → 不输出键。**
 *
 *    | 操作 | `toJson()` |
 *    |---|---|
 *    | 默认构造 | `{}` |
 *    | `setBold(nullopt)` | `{}` |
 *
 * 2. **显式 `false` → 输出 `false`，且必须是 JSON 布尔而非字符串。**
 *
 *    | 操作 | `toJson()` |
 *    |---|---|
 *    | `setBold(false)` | `{"bold":false}` |
 *    | `setItalic(false)` | `{"italic":false}` |
 *
 *    原版 `Style.Serializer` 对 `Boolean.FALSE` 写出 `false`。若 Cubium 用
 *    `if (*value)` 判断再写，显式 false 会被**静默丢弃**——这正是本能力要抓的缺陷。
 *
 * 3. **显式 `true` → 输出 `true`。**
 *
 *    与 2 对称，但单独验证可以区分「只处理了 false」的实现。
 *
 * 单元格式：
 * - `count` —— 五个标志的数量
 * - `flags` —— 五个标志名，逗号分隔
 * - `empty` —— 默认构造的 `toJson()`
 * - `colorSet|<FORMATTING>` —— 设置颜色后的 `toJson()`
 * - `set|<flag>|<true|false|unset>` —— 设置单个标志后的 `toJson()`
 * - `setAll|<t|f>` —— 五个标志全部设同一值后的 `toJson()`
 * - `setThenClear|<flag>|<true|false>` —— 设值再清除，应回到 `{}`
 * - `asBool|<flag>|<true|false|unset>` —— `isXxx()` 的返回值
 */
#include <algorithm>\n#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/text/TextStyle.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 五个三态标志。
struct FlagRef {
    const char* name;
    void (text::Style::*setter)(std::optional<bool>) noexcept;
    bool (text::Style::*asBool)() const noexcept;
};

constexpr FlagRef FLAGS[] = {
    {"bold", &text::Style::setBold, &text::Style::isBold},
    {"italic", &text::Style::setItalic, &text::Style::isItalic},
    {"underlined", &text::Style::setUnderlined, &text::Style::isUnderlined},
    {"strikethrough", &text::Style::setStrikethrough, &text::Style::isStrikethrough},
    {"obfuscated", &text::Style::setObfuscated, &text::Style::isObfuscated},
};

constexpr i32 FLAG_COUNT = 5;

const FlagRef* findFlag(const std::string& name)
{
    for (const FlagRef& flag : FLAGS) {
        if (name == flag.name) {
            return &flag;
        }
    }
    return nullptr;
}

/// 把 `true` / `false` / `unset` 解析为三态值。
std::optional<bool> parseTriState(const std::string& text)
{
    if (text == "true") {
        return true;
    }
    if (text == "false") {
        return false;
    }
    return std::nullopt;
}

text::TextFormatting formattingFromJavaName(const std::string& name)
{
    struct Pair {
        const char* javaName;
        text::TextFormatting value;
    };

    constexpr Pair PAIRS[] = {
        {"RED", text::TextFormatting::Red},
        {"WHITE", text::TextFormatting::White},
        {"BLACK", text::TextFormatting::Black},
        {"GOLD", text::TextFormatting::Gold},
    };
    for (const Pair& pair : PAIRS) {
        if (name == pair.javaName) {
            return pair.value;
        }
    }
    throw std::runtime_error("style_tristate: 未支持的颜色：" + name);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        "count",
        "flags",
        "empty",
        "colorSet|RED",
        "colorSet|WHITE",
        // 单个标志：true / false / unset
        "set|bold|true",
        "set|bold|false",
        "set|bold|unset",
        "set|italic|true",
        "set|italic|false",
        "set|underlined|true",
        "set|underlined|false",
        "set|strikethrough|true",
        "set|strikethrough|false",
        "set|obfuscated|true",
        "set|obfuscated|false",
        // 全部标志同时设置
        "setAll|t",
        "setAll|f",
        // 设值后清除
        "setThenClear|bold|true",
        "setThenClear|bold|false",
        "setThenClear|italic|false",
        // isXxx() 的行为（与原版 isBold() 对照）
        "asBool|bold|unset",
        "asBool|bold|false",
        "asBool|bold|true",
        "asBool|italic|unset",
        "asBool|italic|false",
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

/// 把 JSON 对象的键按固定顺序重排，消除键序差异（JSON 对象本无序）。
std::string canonicalJson(const json& value)
{
    if (!value.is_object()) {
        return value.dump();
    }
    // 原版 Style.Serializer 的输出顺序：color, bold, italic, underlined,
    // strikethrough, obfuscated, insertion, clickEvent, hoverEvent。
    static constexpr const char* ORDER[] = {
        "color",
        "bold",
        "italic",
        "underlined",
        "strikethrough",
        "obfuscated",
        "insertion",
        "clickEvent",
        "hoverEvent",
    };

    // nlohmann::json 的 object 底层是 std::map，会按键名字典序重排，
    // 因此这里手工拼接字符串以保留原版的字段顺序。
    std::string out = "{";
    bool first = true;

    auto append = [&](const std::string& key, const json& item) {
        if (!first) {
            out.push_back(',');
        }
        first = false;
        out += json(key).dump();
        out.push_back(':');
        out += item.dump();
    };

    for (const auto* key : ORDER) {
        if (value.contains(key)) {
            append(key, value[key]);
        }
    }

    // 兜底：把未列出的键按字典序追加，避免未来新增字段被静默丢弃。
    std::vector<std::string> rest;
    for (const auto& item : value.items()) {
        bool listed = false;
        for (const auto* key : ORDER) {
            if (item.key() == key) {
                listed = true;
                break;
            }
        }
        if (!listed) {
            rest.push_back(item.key());
        }
    }
    std::sort(rest.begin(), rest.end());
    for (const auto& key : rest) {
        append(key, value[key]);
    }

    out.push_back('}');
    return out;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.empty()) {
        throw std::runtime_error("style_tristate: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;
    text::Style style;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(FLAG_COUNT);
    } else if (mode == "flags" && parts.size() == 1) {
        std::string names;
        for (const FlagRef& flag : FLAGS) {
            if (!names.empty()) {
                names.push_back(',');
            }
            names += flag.name;
        }
        value = names;
    } else if (mode == "empty" && parts.size() == 1) {
        value = canonicalJson(style.toJson());
    } else if (mode == "colorSet" && parts.size() == 2) {
        style.setColor(formattingFromJavaName(parts[1]));
        value = canonicalJson(style.toJson());
    } else if (mode == "setAll" && parts.size() == 2) {
        const std::optional<bool> flagValue = parts[1] == "t" ? std::optional<bool>{true} : std::optional<bool>{false};
        for (const FlagRef& flag : FLAGS) {
            (style.*flag.setter)(flagValue);
        }
        value = canonicalJson(style.toJson());
    } else if (mode == "set" && parts.size() == 3) {
        const FlagRef* flag = findFlag(parts[1]);
        if (flag == nullptr) {
            throw std::runtime_error("style_tristate: 未知标志：" + parts[1]);
        }
        (style.*flag->setter)(parseTriState(parts[2]));
        value = canonicalJson(style.toJson());
    } else if (mode == "setThenClear" && parts.size() == 3) {
        const FlagRef* flag = findFlag(parts[1]);
        if (flag == nullptr) {
            throw std::runtime_error("style_tristate: 未知标志：" + parts[1]);
        }
        (style.*flag->setter)(parseTriState(parts[2]));
        (style.*flag->setter)(std::nullopt);
        value = canonicalJson(style.toJson());
    } else if (mode == "asBool" && parts.size() == 3) {
        const FlagRef* flag = findFlag(parts[1]);
        if (flag == nullptr) {
            throw std::runtime_error("style_tristate: 未知标志：" + parts[1]);
        }
        (style.*flag->setter)(parseTriState(parts[2]));
        value = (style.*flag->asBool)() ? "true" : "false";
    } else {
        throw std::runtime_error("style_tristate: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("style_tristate", &writeList, &runLine)

} // namespace mc::diff
