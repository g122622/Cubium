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
 * @brief 差分能力：`Identifier` 的三种构造入口与 `toString`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "<namespace>:<path>" 或 "<exception>" }
 *
 * 复刻原版 `net.minecraft.resources.Identifier` 的三个静态入口：
 *
 * ```java
 * public static Identifier parse(String text) { return bySeparator(text, ':'); }
 *
 * public static @Nullable Identifier tryParse(String text) { return tryBySeparator(text, ':');
 * }
 *
 * public static Identifier withDefaultNamespace(String path) {
 *     return new Identifier("minecraft", assertValidPath("minecraft", path));
 * }
 *
 * public static Identifier bySeparator(String text, char separator) {
 *     int i = text.indexOf(separator);
 *     if (i >= 0) {
 *         String path = text.substring(i + 1);
 *         if (i != 0) {
 *             String namespace = text.substring(0, i);
 *             return createUntrusted(namespace, path);
 *         } else {
 *             return withDefaultNamespace(path);          // 首字符是冒号 → 剩余部分当路径
 *         }
 *     } else {
 *         return withDefaultNamespace(text);              // 无冒号 → 整个串当路径
 *     }
 * }
 *
 * public static @Nullable Identifier tryBySeparator(String text, char separator) {
 *     int i = text.indexOf(separator);
 *     if (i >= 0) {
 *         String path = text.substring(i + 1);
 *         if (!isValidPath(path)) return null;
 *         else if (i != 0) {
 *             String namespace = text.substring(0, i);
 *             return isValidNamespace(namespace) ? new Identifier(namespace, path) : null;
 *         } else {
 *             return new Identifier("minecraft", path);
 *         }
 *     } else {
 *         return isValidPath(text) ? new Identifier("minecraft", text) : null;
 *     }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`withDefaultNamespace` 不解析 `:`，整个输入都当路径。**
 *
 *    ```java
 *    return new Identifier("minecraft", assertValidPath("minecraft", text));
 *    ```
 *
 *    所以 `withDefaultNamespace("foo:bar")` **抛异常**（`:` 不是合法路径字符），
 *    而 `parse("foo:bar")` 得 `foo:bar`。
 *
 *    **这是本能力最核心的差异**：`withDefaultNamespace` 只补默认命名空间，
 *    不拆分已有命名空间。
 *
 * 2. **`parse` 只在「冒号位置为 0」或「无冒号」时补默认命名空间。**
 *
 *    | 输入 | `parse` |
 *    |---|---|
 *    | `stone` | `minecraft:stone`（无冒号）|
 *    | `:stone` | `minecraft:stone`（冒号在 0 位）|
 *    | `foo:bar` | `foo:bar`（正常拆分）|
 *    | `a:b:c` | **抛异常**（路径 `b:c` 含 `:`）|
 *
 *    注意 `bySeparator` 用的是 `indexOf`（**第一个**冒号），不是最后一个。
 *
 * 3. **`tryParse` 与 `parse` 的分支结构不同——`tryParse` 先校验路径。**
 *
 *    ```java
 *    if (!isValidPath(path)) return null;   // 先查路径
 *    else if (i != 0) { ... 校验 namespace ... }
 *    ```
 *
 *    而 `parse` 是 `createUntrusted(namespace, path)` 里先 `assertValidNamespace`、
 *    再 `assertValidPath`。**两者的异常消息顺序因此不同**，但返回 `null` 还是抛异常是一致的。
 *
 * 4. **命名空间与路径的字符集只差一个 `/`。**
 *
 *    ```java
 *    private static boolean validNamespaceChar(char c) {
 *        return c == '_' || c == '-' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9' || c == '.';
 *    }
 *    public static boolean validPathChar(char c) {
 *        return c == '_' || c == '-' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9'
 *            || c == '/' || c == '.';
 *    }
 *    ```
 *
 *    **两者都不含 `:`**（分隔符）与**大写字母**。
 *
 * 单元格式：
 * - `parse|<text>` —— `parse`，成功输出 `namespace:path`，异常输出 `!<异常类型>`
 * - `tryParse|<text>` —— `tryParse`，成功输出 `namespace:path`，失败输出 `null`
 * - `default|<text>` —— `withDefaultNamespace`，同上
 * - `toString|<ns>|<path>` —— 直接构造后 `toString`（用于验证恒等）
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

constexpr const char* DEFAULT_NAMESPACE = "minecraft";

bool validNamespaceChar(char ch)
{
    return ch == '_' || ch == '-' || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '.';
}

bool validPathChar(char ch)
{
    return ch == '_' || ch == '-' || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '/' || ch == '.';
}

bool isValidNamespace(const std::string& text)
{
    for (char ch : text) {
        if (!validNamespaceChar(ch)) {
            return false;
        }
    }
    return true;
}

bool isValidPath(const std::string& text)
{
    for (char ch : text) {
        if (!validPathChar(ch)) {
            return false;
        }
    }
    return true;
}

/// 一个已构造的 `Identifier`。
struct Identifier {
    std::string nameSpace;
    std::string path;
};

std::string toString(const Identifier& identifier)
{
    return identifier.nameSpace + ":" + identifier.path;
}

/// `withDefaultNamespace`：整个输入当路径，不解析 `:`。
bool withDefaultNamespace(const std::string& path, Identifier& out)
{
    if (!isValidPath(path)) {
        return false;
    }
    out.nameSpace = DEFAULT_NAMESPACE;
    out.path = path;
    return true;
}

/// `bySeparator`：抛异常时返回 `false`。
bool bySeparator(const std::string& text, Identifier& out)
{
    const std::size_t index = text.find(':');
    if (index != std::string::npos) {
        const std::string path = text.substr(index + 1);
        if (index != 0) {
            const std::string nameSpace = text.substr(0, index);
            // createUntrusted：先校验命名空间，再校验路径。
            if (!isValidNamespace(nameSpace) || !isValidPath(path)) {
                return false;
            }
            out.nameSpace = nameSpace;
            out.path = path;
            return true;
        }
        return withDefaultNamespace(path, out);
    }
    return withDefaultNamespace(text, out);
}

/// `tryBySeparator`：失败返回 `false`（对应 `null`）。
bool tryBySeparator(const std::string& text, Identifier& out)
{
    const std::size_t index = text.find(':');
    if (index != std::string::npos) {
        const std::string path = text.substr(index + 1);
        if (!isValidPath(path)) {
            return false;
        }
        if (index != 0) {
            const std::string nameSpace = text.substr(0, index);
            if (!isValidNamespace(nameSpace)) {
                return false;
            }
            out.nameSpace = nameSpace;
            out.path = path;
            return true;
        }
        out.nameSpace = DEFAULT_NAMESPACE;
        out.path = path;
        return true;
    }
    if (!isValidPath(text)) {
        return false;
    }
    out.nameSpace = DEFAULT_NAMESPACE;
    out.path = text;
    return true;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // 无冒号：三者一致
        "parse|stone",
        "tryParse|stone",
        "default|stone",
        // 首字符是冒号：parse/tryParse 补默认命名空间，default 失败
        "parse|:stone",
        "tryParse|:stone",
        "default|:stone",
        // 正常拆分：default 失败（冒号不是合法路径字符）
        "parse|foo:bar",
        "tryParse|foo:bar",
        "default|foo:bar",
        // 多冒号：第一个冒号后的路径含冒号
        "parse|a:b:c",
        "tryParse|a:b:c",
        "default|a:b:c",
        // 大写字母：三者都失败
        "parse|Stone",
        "tryParse|Stone",
        "default|Stone",
        "parse|foo:Bar",
        // 空串
        "parse|",
        "tryParse|",
        "default|",
        // 路径含斜杠
        "parse|minecraft:block/stone",
        "tryParse|block/stone",
        "default|block/stone",
        // 命名空间含斜杠（非法）
        "parse|a/b:stone",
        "tryParse|a/b:stone",
        // 路径含冒号但命名空间合法
        "tryParse|minecraft:a:b",
        // toString 恒等
        "toString|minecraft|stone",
        "toString|foo|block/bar",
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

    // 注意：`parse|`（空输入）会被 splitPipe 切成两段，第二段为空串。
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("identifier_resolve: 单元段数不符：" + unit);
    }

    const std::string mode = parts[0];
    Identifier identifier;
    std::string value;

    if (mode == "toString" && parts.size() == 3) {
        identifier.nameSpace = parts[1];
        identifier.path = parts[2];
        value = toString(identifier);
    } else if (mode == "parse") {
        value = bySeparator(parts[1], identifier) ? toString(identifier) : "!IdentifierException";
    } else if (mode == "tryParse") {
        value = tryBySeparator(parts[1], identifier) ? toString(identifier) : "null";
    } else if (mode == "default") {
        value = withDefaultNamespace(parts[1], identifier) ? toString(identifier) : "!IdentifierException";
    } else {
        throw std::runtime_error("identifier_resolve: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(identifier_resolve, &writeList, &runLine)

} // namespace mc::diff
