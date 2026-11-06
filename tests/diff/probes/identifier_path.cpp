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
 * @brief 差分能力：`Identifier` 的字符串派生方法（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.resources.Identifier` 的实例方法：
 *
 * ```java
 * public Identifier withPath(String path) {
 *     return new Identifier(this.namespace, assertValidPath(this.namespace, path));
 * }
 * public Identifier withPrefix(String prefix) { return this.withPath(prefix + this.path); }
 * public Identifier withSuffix(String suffix) { return this.withPath(this.path + suffix); }
 *
 * public String toDebugFileName() {
 *     return this.toString().replace('/', '_').replace(':', '_');
 * }
 *
 * public String toLanguageKey() { return this.namespace + "." + this.path; }
 *
 * public String toShortLanguageKey() {
 *     return this.namespace.equals("minecraft") ? this.path : this.toLanguageKey();
 * }
 *
 * public String toShortString() {
 *     return this.namespace.equals("minecraft") ? this.path : this.toString();
 * }
 *
 * public String toLanguageKey(String prefix) { return prefix + "." + this.toLanguageKey(); }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`toLanguageKey()` 只把 `:` 换成 `.`，路径里的 `/` 保持不变。**
 *
 *    ```java
 *    return this.namespace + "." + this.path;
 *    ```
 *
 *    - `minecraft:block/stone` → **`minecraft.block/stone`**（`/` 保留！）
 *
 *    这与直觉不同——很多人以为翻译键会把 `/` 也换成 `.`。实际**只有命名空间与路径之间的
 *    那个 `:` 变成 `.`**。
 *
 *    因此 `"block.minecraft.block/stone"` 才是完整键，而不是 `"block.minecraft.block.stone"`。
 *
 * 2. **`toShortLanguageKey()` / `toShortString()` 的简写只对 `minecraft` 命名空间生效。**
 *
 *    ```java
 *    return this.namespace.equals("minecraft") ? this.path : this.toLanguageKey();
 *    ```
 *
 *    - `minecraft:block/stone` → `block/stone`
 *    - `foo:bar/baz` → `foo.bar/baz`（语言键）/ `foo:bar/baz`（短字符串）
 *
 *    **`equals` 是精确匹配**——`Minecraft:block/stone` 因命名空间大写本就非法，走不到这里。
 *
 * 3. **`toDebugFileName()` 把 `/` 与 `:` 都换成 `_`，但 `.` 保留。**
 *
 *    ```java
 *    return this.toString().replace('/', '_').replace(':', '_');
 *    ```
 *
 *    - `minecraft:block/stone` → **`minecraft_block_stone`**
 *    - `foo:bar/baz.txt` → **`foo_bar_baz.txt`**（`.` 保留）
 *
 *    先替换 `/` 再替换 `:` 与顺序无关（两者互不影响）。
 *
 * 4. **`withPrefix` / `withSuffix` 会**重新校验**拼接后的路径，非法时抛异常。**
 *
 *    ```java
 *    public Identifier withPrefix(String prefix) { return this.withPath(prefix + this.path); }
 *    ```
 *
 *    `withPath` 内部调用 `assertValidPath`，所以：
 *
 *    - `minecraft:stone` 加前缀 `"block/"` → `minecraft:block/stone`（合法）
 *    - `minecraft:stone` 加前缀 `"Block/"` → **抛异常**（`B` 非法）
 *
 *    **这意味着 `withPrefix` 不是纯字符串拼接**，它会失败。
 *
 * 单元格式：
 * - `lang|<namespace>|<path>` —— `toLanguageKey()`
 * - `shortLang|<namespace>|<path>` —— `toShortLanguageKey()`
 * - `shortStr|<namespace>|<path>` —— `toShortString()`
 * - `debugFile|<namespace>|<path>` —— `toDebugFileName()`
 * - `langPrefix|<namespace>|<path>|<prefix>` —— `toLanguageKey(String)`
 * - `withPrefix|<namespace>|<path>|<prefix>` —— `withPrefix`
 * - `withSuffix|<namespace>|<path>|<suffix>` —— `withSuffix`
 * - `withPath|<namespace>|<path>|<newPath>` —— `withPath`
 *
 * 构造失败（命名空间或路径非法）统一输出 `"<invalid>"`——**两侧一致**，避免 oracle 抛异常。
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

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // lang：/ 保留
        "lang|minecraft|block/stone",
        "lang|foo|bar/baz",
        "lang|minecraft|stone",
        // shortLang
        "shortLang|minecraft|block/stone",
        "shortLang|foo|bar/baz",
        // shortStr
        "shortStr|minecraft|block/stone",
        "shortStr|foo|bar/baz",
        // debugFile
        "debugFile|minecraft|block/stone",
        "debugFile|foo|bar/baz.txt",
        // langPrefix
        "langPrefix|minecraft|stone|block",
        "langPrefix|foo|bar|item",
        // withPrefix / withSuffix
        "withPrefix|minecraft|stone|block/",
        "withPrefix|minecraft|stone|Block/",
        "withSuffix|minecraft|stone|_top",
        "withSuffix|minecraft|stone|_TOP",
        // withPath
        "withPath|minecraft|stone|block/stone",
        "withPath|minecraft|stone|Stone",
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
    if (parts.size() < 3) {
        throw std::runtime_error("identifier_path: 单元至少三段：" + unit);
    }

    const std::string mode = parts[0];
    const std::string& nameSpace = parts[1];
    const std::string& path = parts[2];

    if (mode == "lang" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", ResourceLocation(nameSpace, path).toLanguageKey()}}.dump();
    }
    if (mode == "shortLang" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", ResourceLocation(nameSpace, path).toShortLanguageKey()}}.dump();
    }
    if (mode == "shortStr" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", ResourceLocation(nameSpace, path).toShortString()}}.dump();
    }
    if (mode == "debugFile" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", ResourceLocation(nameSpace, path).toDebugFileName()}}.dump();
    }
    if (mode == "langPrefix" && parts.size() == 4) {
        return json{{"unit", unit}, {"value", parts[3] + "." + ResourceLocation(nameSpace, path).toLanguageKey()}}.dump();
    }
    if (mode == "withPrefix" && parts.size() == 4) {
        const std::string combined = parts[3] + path;
        if (!ResourceLocation::isValidPath(combined)) {
            return json{{"unit", unit}, {"value", "<invalid>"}}.dump();
        }
        return json{{"unit", unit}, {"value", nameSpace + ":" + combined}}.dump();
    }
    if (mode == "withSuffix" && parts.size() == 4) {
        const std::string combined = path + parts[3];
        if (!ResourceLocation::isValidPath(combined)) {
            return json{{"unit", unit}, {"value", "<invalid>"}}.dump();
        }
        return json{{"unit", unit}, {"value", nameSpace + ":" + combined}}.dump();
    }
    if (mode == "withPath" && parts.size() == 4) {
        if (!ResourceLocation::isValidPath(parts[3])) {
            return json{{"unit", unit}, {"value", "<invalid>"}}.dump();
        }
        return json{{"unit", unit}, {"value", nameSpace + ":" + parts[3]}}.dump();
    }

    throw std::runtime_error("identifier_path: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(identifier_path, &writeList, &runLine)

} // namespace mc::diff
