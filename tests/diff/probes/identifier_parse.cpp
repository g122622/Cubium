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
 * @brief 差分能力：`Identifier` 的解析与校验（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.resources.Identifier`：
 *
 * ```java
 * public static final char NAMESPACE_SEPARATOR = ':';
 * public static final String DEFAULT_NAMESPACE = "minecraft";
 *
 * public static boolean validPathChar(char c) {
 *     return c == '_' || c == '-' || c >= 'a' && c <= 'z'
 *         || c >= '0' && c <= '9' || c == '/' || c == '.';
 * }
 *
 * private static boolean validNamespaceChar(char c) {
 *     return c == '_' || c == '-' || c >= 'a' && c <= 'z'
 *         || c >= '0' && c <= '9' || c == '.';        // 注意：没有 '/'
 * }
 *
 * public static Identifier bySeparator(String s, char sep) {
 *     int i = s.indexOf(sep);
 *     if (i >= 0) {
 *         String path = s.substring(i + 1);
 *         if (i != 0) {
 *             String namespace = s.substring(0, i);
 *             return createUntrusted(namespace, path);   // 校验后构造，非法则抛异常
 *         }
 *         return withDefaultNamespace(path);
 *     }
 *     return withDefaultNamespace(s);
 * }
 *
 * public static Identifier withDefaultNamespace(String path) {
 *     return new Identifier("minecraft", assertValidPath("minecraft", path));
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **命名空间与路径的字符白名单只差一个 `/`。**
 *
 *    | 字符 | 命名空间 | 路径 |
 *    |---|---|---|
 *    | `_` `-` `.` `0-9` `a-z` | 允许 | 允许 |
 *    | **`/`** | **禁止** | **允许** |
 *    | `A-Z` | 禁止 | 禁止 |
 *    | 其他 | 禁止 | 禁止 |
 *
 *    `minecraft:foo/bar` 合法，`foo/bar:baz` **非法**。
 *
 * 2. **空串是合法的。**
 *
 *    `isValidPath("")` 与 `isValidNamespace("")` 都是 `true`——循环零次直接返回。
 *
 *    **原版没有任何非空校验**：`Identifier.parse("")` 会得到 `minecraft:`（空路径），不抛异常。
 *
 * 3. **`bySeparator` 只看「第一个」分隔符，且 `i == 0` 时走默认命名空间。**
 *
 *    - `"foo:bar:baz"` → 命名空间 `foo`，路径 **`bar:baz`**（路径含 `:` → **非法**，抛异常）
 *    - `":foo"` → `i == 0` → `minecraft:foo`
 *    - `"foo:"` → 路径为空串 → **合法**（见上一条）
 *    - `"foo"` → 无分隔符 → `minecraft:foo`
 *
 * 4. **`tryParse` 与 `parse` 的区别只在错误处理。**
 *
 *    `parse` 在非法时抛 `IdentifierException`；`tryParse` 返回 `null`。
 *    本能力用 `"<invalid>"` 字符串表示 `null` / 异常，**两侧都这样表示**，避免抛异常导致
 *    oracle 整体失败。
 *
 * 单元格式：
 * - `path|<text>` —— `isValidPath`
 * - `ns|<text>` —— `isValidNamespace`
 * - `pathChar|<codepoint>` —— `validPathChar`
 * - `parse|<text>` —— `parse`，输出 `namespace:path`；非法输出 `"<invalid>"`
 * - `tryParse|<text>` —— `tryParse`，输出 `namespace:path`；`null` 输出 `"<invalid>"`
 * - `tryBuild|<namespace>|<path>` —— `tryBuild`
 * - `bySep|<text>|<sep>` —— `bySeparator`，`sep` 为单个字符
 * - `withDefault|<text>` —— `withDefaultNamespace`
 */
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 把解析结果渲染成 `namespace:path`；无效时给出 `"<invalid>"`。
std::string renderParsed(const std::optional<ResourceLocation>& parsed)
{
    if (!parsed.has_value()) {
        return "<invalid>";
    }
    return parsed->toString();
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // path / ns 校验
        "path|foo/bar",
        "path|foo/bar.txt",
        "path|Foo",
        "path|foo bar",
        "path|",
        "ns|foo",
        "ns|foo/bar",
        "ns|Foo",
        "ns|",
        // pathChar
        "pathChar|97",
        "pathChar|65",
        "pathChar|47",
        "pathChar|58",
        "pathChar|95",
        // parse：常规
        "parse|minecraft:stone",
        "parse|foo:bar/baz",
        "parse|foo",
        "parse|:foo",
        "parse|foo:",
        // parse：非法
        "parse|Foo:bar",
        "parse|foo/bar:baz",
        "parse|foo:bar:baz",
        // tryParse
        "tryParse|minecraft:stone",
        "tryParse|Foo:bar",
        "tryParse|foo:bar:baz",
        // tryBuild
        "tryBuild|foo|bar",
        "tryBuild|Foo|bar",
        "tryBuild|foo|Bar",
        // bySep
        "bySep|foo.bar|.",
        "bySep|foo|.",
        "bySep|.bar|.",
        // withDefault
        "withDefault|foo/bar",
        "withDefault|Foo",
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
    if (parts.size() < 2) {
        throw std::runtime_error("identifier_parse: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "path" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", ResourceLocation::isValidPath(parts[1])}}.dump();
    }
    if (mode == "ns" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", ResourceLocation::isValidNamespace(parts[1])}}.dump();
    }
    if (mode == "pathChar" && parts.size() == 2) {
        const i32 codepoint = std::stoi(parts[1]);
        const bool valid = codepoint >= 0 && codepoint < 128 && ResourceLocation::validPathChar(static_cast<char>(codepoint));
        return json{{"unit", unit}, {"value", valid}}.dump();
    }
    if (mode == "parse" || mode == "tryParse") {
        if (parts.size() != 2) {
            throw std::runtime_error("identifier_parse: 需要 2 段：" + unit);
        }
        return json{{"unit", unit}, {"value", renderParsed(ResourceLocation::tryBySeparator(parts[1], ':'))}}.dump();
    }
    if (mode == "tryBuild" && parts.size() == 3) {
        if (!ResourceLocation::isValidNamespace(parts[1]) || !ResourceLocation::isValidPath(parts[2])) {
            return json{{"unit", unit}, {"value", "<invalid>"}}.dump();
        }
        return json{{"unit", unit}, {"value", parts[1] + ":" + parts[2]}}.dump();
    }
    if (mode == "bySep" && parts.size() == 3) {
        if (parts[2].size() != 1) {
            throw std::runtime_error("identifier_parse: 分隔符须为单字符：" + unit);
        }
        return json{{"unit", unit}, {"value", renderParsed(ResourceLocation::tryBySeparator(parts[1], parts[2][0]))}}.dump();
    }
    if (mode == "withDefault" && parts.size() == 2) {
        if (!ResourceLocation::isValidPath(parts[1])) {
            return json{{"unit", unit}, {"value", "<invalid>"}}.dump();
        }
        return json{{"unit", unit}, {"value", "minecraft:" + parts[1]}}.dump();
    }

    throw std::runtime_error("identifier_parse: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(identifier_parse, &writeList, &runLine)

} // namespace mc::diff
