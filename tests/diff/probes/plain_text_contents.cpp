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
 * @brief 差分能力：`PlainTextContents` 的构造与 `toString`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.network.chat.contents.PlainTextContents`：
 *
 * ```java
 * public interface PlainTextContents extends ComponentContents {
 *     PlainTextContents EMPTY = new PlainTextContents() {
 *         @Override public String toString() { return "empty"; }
 *         @Override public String text() { return ""; }
 *     };
 *
 *     static PlainTextContents create(String text) {
 *         return text.isEmpty() ? EMPTY : new PlainTextContents.LiteralContents(text);
 *     }
 *
 *     String text();
 *
 *     public record LiteralContents(String text) implements PlainTextContents {
 *         @Override public String toString() { return "literal{" + this.text + "}"; }
 *     }
 * }
 * ```
 *
 * 以及 `Component.literal`：
 *
 * ```java
 * static MutableComponent literal(String text) {
 *     return MutableComponent.create(PlainTextContents.create(text));
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **空串折叠为单例 `EMPTY`，`toString()` 是 `"empty"`。**
 *
 *    | 输入 | 实现 | `toString()` |
 *    |---|---|---|
 *    | `""` | `EMPTY` 单例 | **`empty`** |
 *    | `"abc"` | `LiteralContents` | **`literal{abc}`** |
 *
 *    **注意格式不对称**：`EMPTY` 是裸词 `empty`，没有 `literal{}` 包裹。
 *    若 Cubium 统一用 `literal{}` 包裹，空串会输出 `literal{}`——与原版不同。
 *
 * 2. **空串是单例，因此 `create("") == create("")` 为 `true`（identity）。**
 *
 *    ```java
 *    return text.isEmpty() ? EMPTY : new PlainTextContents.LiteralContents(text);
 *    ```
 *
 *    非空串**每次都是新对象**，identity 不等。
 *
 *    这对 `==` 比较的代码路径有实际影响（`Component` 的 intern 化依赖它）。
 *
 * 3. **`LiteralContents` 是 `record`，`equals` 按 `text` 比较。**
 *
 *    `new LiteralContents("abc").equals(new LiteralContents("abc"))` 为 `true`。
 *    而 `EMPTY.equals(...)` 走的是匿名类的 `Object.equals`——**只有 identity 相等**。
 *
 *    所以 **`create("").equals(create(""))` 为 `true`（同一实例），
 *    但 `create("").equals(某非空字面)` 为 `false`。**
 *
 * 4. **`text()` 对 `EMPTY` 返回 `""`，与输入一致。**
 *
 *    所以 `create(t).text().equals(t)` **恒真**——折叠只影响 `toString` 与 identity，
 *    **不影响文本内容**。
 *
 * 单元格式：
 * - `create|<text>` —— `PlainTextContents.create` 后的 `toString()`
 * - `text|<text>` —— `create(...).text()`
 * - `identity|<text>` —— `create(x) == create(x)`（两次调用是否同一实例）
 * - `eq|<a>|<b>` —— `create(a).equals(create(b))`
 * - `string|<text>` —— `Component.literal(x).getString()`
 * - `component|<text>` —— `Component.literal(x).getContents().toString()`
 */
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 一个 `PlainTextContents`：空串映射为 `EMPTY` 单例。
struct PlainTextContents {
    bool isEmptySingleton = false;
    std::string text;
};

PlainTextContents create(const std::string& text)
{
    if (text.empty()) {
        return PlainTextContents{true, ""};
    }
    return PlainTextContents{false, text};
}

/// 原版 `PlainTextContents.toString`。
std::string toString(const PlainTextContents& contents)
{
    if (contents.isEmptySingleton) {
        return "empty";
    }
    return "literal{" + contents.text + "}";
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // create：空串折叠
        "create|",
        "create|abc",
        "create| ",
        "create|a b",
        "create|{}",
        "create|literal{x}",
        // text：折叠不影响内容
        "text|",
        "text|abc",
        // identity：空串是单例
        "identity|",
        "identity|abc",
        // eq
        "eq||",
        "eq|abc|abc",
        "eq|abc|abd",
        "eq||abc",
        // component：通过 Component.literal 观测
        "component|",
        "component|abc",
        "string|",
        "string|abc",
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
        throw std::runtime_error("plain_text_contents: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "create" && parts.size() == 2) {
        value = toString(create(parts[1]));
    } else if (mode == "text" && parts.size() == 2) {
        value = create(parts[1]).text;
    } else if (mode == "identity" && parts.size() == 2) {
        // 原版：text.isEmpty() ? EMPTY : new LiteralContents(text)
        // 空串返回同一单例，非空串每次新建。
        value = create(parts[1]).isEmptySingleton ? "true" : "false";
    } else if (mode == "eq" && parts.size() == 3) {
        const PlainTextContents left = create(parts[1]);
        const PlainTextContents right = create(parts[2]);
        // 两个 EMPTY 是同一实例 → true；
        // 两个 LiteralContents 按 text 比较 → 值相等即 true；
        // 一空一非空 → false。
        const bool equal = left.isEmptySingleton && right.isEmptySingleton
            ? true
            : (!left.isEmptySingleton && !right.isEmptySingleton && left.text == right.text);
        value = equal ? "true" : "false";
    } else if (mode == "component" && parts.size() == 2) {
        // Component.literal(x).getContents() 就是 create(x)。
        value = toString(create(parts[1]));
    } else if (mode == "string" && parts.size() == 2) {
        // Component.literal(x).getString() 走 visit，返回原始文本。
        value = create(parts[1]).text;
    } else {
        throw std::runtime_error("plain_text_contents: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(plain_text_contents, &writeList, &runLine)

} // namespace mc::diff
