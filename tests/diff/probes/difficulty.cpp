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
 * @brief 差分能力：`Difficulty`（难度枚举，纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.Difficulty`：
 *
 * ```java
 * public enum Difficulty implements StringRepresentable {
 *     PEACEFUL(0, "peaceful"),
 *     EASY(1, "easy"),
 *     NORMAL(2, "normal"),
 *     HARD(3, "hard");
 *
 *     private static final IntFunction<Difficulty> BY_ID =
 *         ByIdMap.continuous(Difficulty::getId, values(), ByIdMap.OutOfBoundsStrategy.WRAP);
 *
 *     public int getId() { return this.id; }
 *
 *     public Component getDisplayName() {
 *         return Component.translatable("options.difficulty." + this.key);
 *     }
 *
 *     public Component getInfo() {
 *         return Component.translatable("options.difficulty." + this.key + ".info");
 *     }
 *
 *     @Deprecated
 *     public static Difficulty byId(int id) { return BY_ID.apply(id); }
 *
 *     public static @Nullable Difficulty byName(String name) { return CODEC.byName(name); }
 *
 *     public String getKey() { return this.key; }
 *     public String getSerializedName() { return this.key; }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`BY_ID` 用 `OutOfBoundsStrategy.WRAP`，不是 `ZERO`。**
 *
 *    | id | 结果 | 说明 |
 *    |---|---|---|
 *    | `0` | `PEACEFUL` | |
 *    | `1` | `EASY` | |
 *    | `2` | `NORMAL` | |
 *    | `3` | `HARD` | |
 *    | **`4`** | **`PEACEFUL`** | 回绕（4 % 4 = 0）|
 *    | **`5`** | **`EASY`** | 回绕（5 % 4 = 1）|
 *    | **`-1`** | **`HARD`** | 回绕（-1 → 3）|
 *    | **`-4`** | **`PEACEFUL`** | 回绕（-4 → 0）|
 *    | **`1000`** | **`PEACEFUL`** | 1000 % 4 = 0 |
 *
 *    **这与 [`display_slot`](./display_slot.md) / [`objective_criteria`](./objective_criteria.md) /
 *    [`team_rule`](./team_rule.md) 的 `ZERO` 策略完全不同**——那些越界返回第一项，
 *    这里越界**按模回绕**。
 *
 *    `id = 4` 时两者都返回 `PEACEFUL`（碰巧一致），但 `id = 5` 时
 *    `ZERO` 返回 `PEACEFUL`、`WRAP` 返回 `EASY`——**分道扬镳**。
 *
 * 2. **翻译键有两层后缀：`getDisplayName` 是 `options.difficulty.<key>`，
 *    `getInfo` 是 `options.difficulty.<key>.info`。**
 *
 *    | 单元 | 结果 |
 *    |---|---|
 *    | `displayName\|HARD` | `options.difficulty.hard` |
 *    | `info\|HARD` | `options.difficulty.hard.info` |
 *
 *    注意 `.info` 是**追加在最后**，不是在中间。
 *
 * 3. **`byName` 走 `CODEC.byName`，失败返回 `null`。**
 *
 *    `CODEC` 是 `StringRepresentable.fromEnum`，其 `byName` 对未知名返回 `null`（不抛异常）。
 *
 * 4. **`getKey()` 与 `getSerializedName()` 返回同一字段。**
 *
 *    序列化名是 `peaceful` / `easy` / `normal` / `hard`——全小写、无下划线。
 *
 * 单元格式：
 * - `serial|<difficulty>` —— `getSerializedName()`
 * - `id|<difficulty>` —— `getId()`
 * - `key|<difficulty>` —— `getKey()`
 * - `displayName|<difficulty>` —— `getDisplayName()` 的翻译键
 * - `info|<difficulty>` —— `getInfo()` 的翻译键
 * - `byId|<n>` —— `Difficulty.byId(n)`（回绕语义）
 * - `byName|<name>` —— `Difficulty.byName(name)`，无匹配 `null`
 * - `count` —— 枚举项数
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

/// `Difficulty` 的 4 个枚举项。
struct Difficulty {
    const char* enumName;
    i32 id;
    const char* key;
};

constexpr Difficulty DIFFICULTIES[] = {
    {"PEACEFUL", 0, "peaceful"},
    {"EASY", 1, "easy"},
    {"NORMAL", 2, "normal"},
    {"HARD", 3, "hard"},
};

constexpr i32 DIFFICULTY_COUNT = 4;

const Difficulty* findDifficulty(const std::string& name)
{
    for (const Difficulty& difficulty : DIFFICULTIES) {
        if (name == difficulty.enumName) {
            return &difficulty;
        }
    }
    throw std::runtime_error("difficulty: 未知难度：" + name);
}

/// `ByIdMap.continuous(..., OutOfBoundsStrategy.WRAP)`：按模回绕。
const Difficulty& byId(i32 id)
{
    // Java 的 % 对负数结果仍为负，需再 +size 一次。
    i32 index = id % DIFFICULTY_COUNT;
    if (index < 0) {
        index += DIFFICULTY_COUNT;
    }
    return DIFFICULTIES[index];
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // serial / key / id
        "serial|PEACEFUL",
        "serial|EASY",
        "serial|NORMAL",
        "serial|HARD",
        "key|PEACEFUL",
        "key|HARD",
        "id|PEACEFUL",
        "id|EASY",
        "id|NORMAL",
        "id|HARD",
        // displayName / info
        "displayName|PEACEFUL",
        "displayName|HARD",
        "info|PEACEFUL",
        "info|EASY",
        "info|HARD",
        // byId：边界与 WRAP 回绕
        "byId|0",
        "byId|1",
        "byId|2",
        "byId|3",
        "byId|4",
        "byId|5",
        "byId|-1",
        "byId|-4",
        "byId|1000",
        // byName
        "byName|peaceful",
        "byName|hard",
        "byName|PEACEFUL",
        "byName|",
        // count
        "count",
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
        throw std::runtime_error("difficulty: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(DIFFICULTY_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("difficulty: 单元段数不符：" + unit);
    } else if (mode == "serial" || mode == "key") {
        value = findDifficulty(parts[1])->key;
    } else if (mode == "id") {
        value = std::to_string(findDifficulty(parts[1])->id);
    } else if (mode == "displayName") {
        value = std::string("options.difficulty.") + findDifficulty(parts[1])->key;
    } else if (mode == "info") {
        value = std::string("options.difficulty.") + findDifficulty(parts[1])->key + ".info";
    } else if (mode == "byId") {
        value = byId(static_cast<i32>(std::stoll(parts[1]))).enumName;
    } else if (mode == "byName") {
        value = "null";
        for (const Difficulty& difficulty : DIFFICULTIES) {
            if (parts[1] == difficulty.key) {
                value = difficulty.enumName;
                break;
            }
        }
    } else {
        throw std::runtime_error("difficulty: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(difficulty, &writeList, &runLine)

} // namespace mc::diff
