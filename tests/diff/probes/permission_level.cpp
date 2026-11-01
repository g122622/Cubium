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
 * @brief 差分能力：`PermissionLevel`（权限等级枚举，纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.server.permissions.PermissionLevel`：
 *
 * ```java
 * public enum PermissionLevel implements StringRepresentable {
 *     ALL("all", 0),
 *     MODERATORS("moderators", 1),
 *     GAMEMASTERS("gamemasters", 2),
 *     ADMINS("admins", 3),
 *     OWNERS("owners", 4);
 *
 *     public static final Codec<PermissionLevel> CODEC =
 *         StringRepresentable.fromEnum(PermissionLevel::values);
 *
 *     private static final IntFunction<PermissionLevel> BY_ID =
 *         ByIdMap.continuous(level -> level.id, values(), ByIdMap.OutOfBoundsStrategy.CLAMP);
 *
 *     public static final Codec<PermissionLevel> INT_CODEC =
 *         Codec.INT.xmap(BY_ID::apply, level -> level.id);
 *
 *     public boolean isEqualOrHigherThan(PermissionLevel other) {
 *         return this.id >= other.id;
 *     }
 *
 *     public static PermissionLevel byId(int id) { return BY_ID.apply(id); }
 *     public int id() { return this.id; }
 *     public String getSerializedName() { return this.name; }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`BY_ID` 用 `OutOfBoundsStrategy.CLAMP`——第三种越界策略。**
 *
 *    | id | 结果 | 说明 |
 *    |---|---|---|
 *    | `0` | `ALL` | |
 *    | `4` | `OWNERS` | |
 *    | **`-1`** | **`ALL`** | **钳到下界**（不是回绕成 `OWNERS`）|
 *    | **`-100`** | **`ALL`** | 钳到下界 |
 *    | **`5`** | **`OWNERS`** | **钳到上界**（不是回绕成 `ALL`）|
 *    | **`100`** | **`OWNERS`** | 钳到上界 |
 *
 *    **三种策略在 `id = 5` 时给出三个不同结果**：
 *
 *    | 策略 | `-1` | `5` |
 *    |---|---|---|
 *    | `ZERO` | 第 0 项 | 第 0 项 |
 *    | `WRAP` | 最后一项 | 第 1 项 |
 *    | **`CLAMP`** | **最后一项** | **最后一项** |
 *
 *    注意 `CLAMP` 与 `WRAP` 在 `id = -1` 时**恰好一致**（都得到最后一项），
 *    **只有在正向越界（`id = 5`）时才分道扬镳**。
 *
 * 2. **`isEqualOrHigherThan` 是 `this.id >= other.id`——数值越大权限越高。**
 *
 *    | 单元 | 结果 |
 *    |---|---|
 *    | `higher\|OWNERS\|ALL` | `true` |
 *    | `higher\|ALL\|OWNERS` | `false` |
 *    | `higher\|ALL\|ALL` | **`true`**（`>=` 不是 `>`）|
 *
 *    **相等时返回 `true`**——「等于或高于」语义。
 *
 * 3. **序列化名是复数，只有 `ALL` 是单数。**
 *
 *    | 枚举 | 序列化名 |
 *    |---|---|
 *    | `ALL` | `all` |
 *    | **`MODERATORS`** | **`moderators`** |
 *    | **`GAMEMASTERS`** | **`gamemasters`** |
 *    | **`ADMINS`** | **`admins`** |
 *    | **`OWNERS`** | **`owners`** |
 *
 *    这与 [`sound_source`](./sound_source.md) 的「复数枚举 → 单数名」**正好相反**：
 *    那里是枚举名复数、名字单数；这里是两者都复数。
 *
 * 4. **`getSerializedName()` 返回 `name` 字段，没有 `getName()`。**
 *
 *    与 [`difficulty`](./difficulty.md) 不同（那里有 `getKey()` + `getSerializedName()`）。
 *
 * 单元格式：
 * - `serial|<level>` —— `getSerializedName()`
 * - `id|<level>` —— `id()`
 * - `byId|<n>` —— `byId(n)`（`CLAMP` 语义）
 * - `higher|<a>|<b>` —— `a.isEqualOrHigherThan(b)`
 * - `count` —— 枚举项数
 * - `names` —— 全部序列化名，逗号分隔
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

/// `PermissionLevel` 的 5 个枚举项。
struct PermissionLevel {
    const char* enumName;
    const char* serializedName;
    i32 id;
};

constexpr PermissionLevel LEVELS[] = {
    {"ALL", "all", 0},
    {"MODERATORS", "moderators", 1},
    {"GAMEMASTERS", "gamemasters", 2},
    {"ADMINS", "admins", 3},
    {"OWNERS", "owners", 4},
};

constexpr i32 LEVEL_COUNT = 5;

const PermissionLevel* findLevel(const std::string& name)
{
    for (const PermissionLevel& level : LEVELS) {
        if (name == level.enumName) {
            return &level;
        }
    }
    throw std::runtime_error("permission_level: 未知等级：" + name);
}

/// `ByIdMap.continuous(..., OutOfBoundsStrategy.CLAMP)`：钳到 `[0, size-1]`。
const PermissionLevel& byId(i32 id)
{
    if (id < 0) {
        return LEVELS[0];
    }
    if (id >= LEVEL_COUNT) {
        return LEVELS[LEVEL_COUNT - 1];
    }
    return LEVELS[id];
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // serial
        "serial|ALL",
        "serial|MODERATORS",
        "serial|GAMEMASTERS",
        "serial|ADMINS",
        "serial|OWNERS",
        // id
        "id|ALL",
        "id|MODERATORS",
        "id|GAMEMASTERS",
        "id|ADMINS",
        "id|OWNERS",
        // byId：边界
        "byId|0",
        "byId|1",
        "byId|2",
        "byId|3",
        "byId|4",
        // byId：CLAMP 的两端钳制
        "byId|-1",
        "byId|-100",
        "byId|5",
        "byId|100",
        // higher：>= 语义
        "higher|OWNERS|ALL",
        "higher|ALL|OWNERS",
        "higher|ALL|ALL",
        "higher|ADMINS|GAMEMASTERS",
        "higher|GAMEMASTERS|ADMINS",
        // count / names
        "count",
        "names",
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
        throw std::runtime_error("permission_level: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(LEVEL_COUNT);
    } else if (mode == "names" && parts.size() == 1) {
        std::string names;
        for (const PermissionLevel& level : LEVELS) {
            if (!names.empty()) {
                names.push_back(',');
            }
            names += level.serializedName;
        }
        value = names;
    } else if (parts.size() < 2) {
        throw std::runtime_error("permission_level: 单元段数不足：" + unit);
    } else if (mode == "serial") {
        value = findLevel(parts[1])->serializedName;
    } else if (mode == "id") {
        value = std::to_string(findLevel(parts[1])->id);
    } else if (mode == "byId") {
        value = byId(static_cast<i32>(std::stoll(parts[1]))).enumName;
    } else if (mode == "higher" && parts.size() == 3) {
        const i32 left = findLevel(parts[1])->id;
        const i32 right = findLevel(parts[2])->id;
        value = left >= right ? "true" : "false";
    } else {
        throw std::runtime_error("permission_level: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("permission_level", &writeList, &runLine)

} // namespace mc::diff
