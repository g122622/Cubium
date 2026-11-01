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
 * @brief 差分能力：`DisplaySlot` 的枚举、序列化名与团队槽映射（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.scores.DisplaySlot`：
 *
 * ```
 * LIST(0, "list"),
 * SIDEBAR(1, "sidebar"),
 * BELOW_NAME(2, "below_name"),
 * TEAM_BLACK(3, "sidebar.team.black"),
 * TEAM_DARK_BLUE(4, "sidebar.team.dark_blue"),
 * ...
 * TEAM_WHITE(18, "sidebar.team.white");
 *
 * public static final IntFunction<DisplaySlot> BY_ID =
 *     ByIdMap.continuous(DisplaySlot::id, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
 *
 * public static @Nullable DisplaySlot teamColorToSlot(ChatFormatting formatting) {
 *     return switch (formatting) {
 *         case BLACK -> TEAM_BLACK;
 *         case DARK_BLUE -> TEAM_DARK_BLUE;
 *         ...
 *         case WHITE -> TEAM_WHITE;
 *         case BOLD, ITALIC, UNDERLINE, RESET, OBFUSCATED, STRIKETHROUGH -> null;
 *     };
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **序列化名的命名不一致：前三个无前缀，团队槽用 `sidebar.team.` 前缀。**
 *
 *    | 枚举 | 序列化名 |
 *    |---|---|
 *    | `LIST` | `list` |
 *    | `SIDEBAR` | `sidebar` |
 *    | `BELOW_NAME` | `below_name` |
 *    | `TEAM_BLACK` | **`sidebar.team.black`** |
 *    | `TEAM_LIGHT_PURPLE` | **`sidebar.team.light_purple`** |
 *
 *    **团队槽用`.`分隔，不是 `_`**——`sidebar.team.dark_blue` 而非 `sidebar_team_dark_blue`。
 *    这与枚举名 `TEAM_DARK_BLUE` 的下划线风格完全不同。
 *
 * 2. **`BY_ID` 的越界策略是 `ZERO`，即返回 `LIST`（id=0）。**
 *
 *    ```java
 *    ByIdMap.continuous(DisplaySlot::id, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
 *    ```
 *
 *    | id | 结果 |
 *    |---|---|
 *    | `0` | `LIST` |
 *    | `18` | `TEAM_WHITE` |
 *    | **`19`** | **`LIST`**（越界）|
 *    | **`-1`** | **`LIST`**（越界）|
 *    | **`1000`** | **`LIST`**（越界）|
 *
 *    `ZERO` 策略的语义是「返回 `values()[0]`」，**不是**「返回 `null`」。
 *
 * 3. **`teamColorToSlot` 只接受 16 个颜色，6 个格式码返回 `null`。**
 *
 *    | 类别 | 结果 |
 *    |---|---|
 *    | `BLACK` … `WHITE`（16 个颜色）| 对应 `TEAM_*` 槽位 |
 *    | **`BOLD` / `ITALIC` / `UNDERLINE` / `STRIKETHROUGH` / `OBFUSCATED` / `RESET`** | **`null`** |
 *
 *    **注意 `RESET` 属于「格式码」而非颜色**——它返回 `null`。
 *
 * 4. **id 与序数完全一致（`0`…`18`）。**
 *
 *    构造器显式传入 `id`，恰好等于 `ordinal()`。但**不能依赖这一点**——
 *    `BY_ID` 走的是 `DisplaySlot::id`，不是序数。
 *
 * 单元格式：
 * - `serial|<slot>` —— `getSerializedName`，`slot` 用枚举名
 * - `id|<slot>` —— `id()`
 * - `byId|<n>` —— `BY_ID.apply(n)`
 * - `teamColor|<formatting>` —— `teamColorToSlot`，`null` 输出 `null`
 * - `names` —— 全部序列化名，逗号分隔
 * - `count` —— 枚举总数
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

/// `DisplaySlot` 的 19 个枚举项。
struct DisplaySlot {
    const char* enumName;
    i32 id;
    const char* serializedName;
};

constexpr DisplaySlot SLOTS[] = {
    {"LIST", 0, "list"},
    {"SIDEBAR", 1, "sidebar"},
    {"BELOW_NAME", 2, "below_name"},
    {"TEAM_BLACK", 3, "sidebar.team.black"},
    {"TEAM_DARK_BLUE", 4, "sidebar.team.dark_blue"},
    {"TEAM_DARK_GREEN", 5, "sidebar.team.dark_green"},
    {"TEAM_DARK_AQUA", 6, "sidebar.team.dark_aqua"},
    {"TEAM_DARK_RED", 7, "sidebar.team.dark_red"},
    {"TEAM_DARK_PURPLE", 8, "sidebar.team.dark_purple"},
    {"TEAM_GOLD", 9, "sidebar.team.gold"},
    {"TEAM_GRAY", 10, "sidebar.team.gray"},
    {"TEAM_DARK_GRAY", 11, "sidebar.team.dark_gray"},
    {"TEAM_BLUE", 12, "sidebar.team.blue"},
    {"TEAM_GREEN", 13, "sidebar.team.green"},
    {"TEAM_AQUA", 14, "sidebar.team.aqua"},
    {"TEAM_RED", 15, "sidebar.team.red"},
    {"TEAM_LIGHT_PURPLE", 16, "sidebar.team.light_purple"},
    {"TEAM_YELLOW", 17, "sidebar.team.yellow"},
    {"TEAM_WHITE", 18, "sidebar.team.white"},
};

constexpr i32 SLOT_COUNT = 19;

/// 颜色 → 团队槽（`teamColorToSlot` 的 16 个颜色分支）。
struct ColorToSlot {
    const char* formatting;
    const char* slot;
};

constexpr ColorToSlot COLOR_TO_SLOT[] = {
    {"BLACK", "TEAM_BLACK"},
    {"DARK_BLUE", "TEAM_DARK_BLUE"},
    {"DARK_GREEN", "TEAM_DARK_GREEN"},
    {"DARK_AQUA", "TEAM_DARK_AQUA"},
    {"DARK_RED", "TEAM_DARK_RED"},
    {"DARK_PURPLE", "TEAM_DARK_PURPLE"},
    {"GOLD", "TEAM_GOLD"},
    {"GRAY", "TEAM_GRAY"},
    {"DARK_GRAY", "TEAM_DARK_GRAY"},
    {"BLUE", "TEAM_BLUE"},
    {"GREEN", "TEAM_GREEN"},
    {"AQUA", "TEAM_AQUA"},
    {"RED", "TEAM_RED"},
    {"LIGHT_PURPLE", "TEAM_LIGHT_PURPLE"},
    {"YELLOW", "TEAM_YELLOW"},
    {"WHITE", "TEAM_WHITE"},
};

constexpr i32 COLOR_TO_SLOT_COUNT = 16;

/// 6 个格式码：`teamColorToSlot` 对它们返回 `null`。
constexpr const char* FORMATTING_CODES[] = {"BOLD", "ITALIC", "UNDERLINE", "STRIKETHROUGH", "OBFUSCATED", "RESET"};

constexpr i32 FORMATTING_CODE_COUNT = 6;

const DisplaySlot* findSlot(const std::string& name)
{
    for (const DisplaySlot& slot : SLOTS) {
        if (name == slot.enumName) {
            return &slot;
        }
    }
    throw std::runtime_error("display_slot: 未知槽位：" + name);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // serial：无前缀的三个
        "serial|LIST",
        "serial|SIDEBAR",
        "serial|BELOW_NAME",
        // serial：团队槽的点分隔
        "serial|TEAM_BLACK",
        "serial|TEAM_DARK_BLUE",
        "serial|TEAM_LIGHT_PURPLE",
        "serial|TEAM_WHITE",
        // id
        "id|LIST",
        "id|BELOW_NAME",
        "id|TEAM_BLACK",
        "id|TEAM_WHITE",
        // byId：边界与越界
        "byId|0",
        "byId|1",
        "byId|2",
        "byId|18",
        "byId|19",
        "byId|-1",
        "byId|1000",
        // teamColor：16 个颜色
        "teamColor|BLACK",
        "teamColor|DARK_BLUE",
        "teamColor|GOLD",
        "teamColor|LIGHT_PURPLE",
        "teamColor|WHITE",
        // teamColor：6 个格式码返回 null
        "teamColor|BOLD",
        "teamColor|ITALIC",
        "teamColor|UNDERLINE",
        "teamColor|STRIKETHROUGH",
        "teamColor|OBFUSCATED",
        "teamColor|RESET",
        // names / count
        "names",
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

/// `ByCodeMap.IdMapper` 的 `ZERO` 越界策略：返回 `values()[0]`。
const DisplaySlot& byId(i32 id)
{
    if (id >= 0 && id < SLOT_COUNT) {
        return SLOTS[id];
    }
    return SLOTS[0];
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.empty()) {
        throw std::runtime_error("display_slot: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "names" && parts.size() == 1) {
        std::string names;
        for (const DisplaySlot& slot : SLOTS) {
            if (!names.empty()) {
                names.push_back(',');
            }
            names += slot.serializedName;
        }
        value = names;
    } else if (mode == "count" && parts.size() == 1) {
        value = std::to_string(SLOT_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("display_slot: 单元段数不符：" + unit);
    } else if (mode == "serial") {
        value = findSlot(parts[1])->serializedName;
    } else if (mode == "id") {
        value = std::to_string(findSlot(parts[1])->id);
    } else if (mode == "byId") {
        value = byId(static_cast<i32>(std::stoll(parts[1]))).enumName;
    } else if (mode == "teamColor") {
        bool found = false;
        for (i32 i = 0; i < COLOR_TO_SLOT_COUNT; ++i) {
            if (parts[1] == COLOR_TO_SLOT[i].formatting) {
                value = COLOR_TO_SLOT[i].slot;
                found = true;
                break;
            }
        }
        if (!found) {
            bool isFormatting = false;
            for (i32 i = 0; i < FORMATTING_CODE_COUNT; ++i) {
                if (parts[1] == FORMATTING_CODES[i]) {
                    isFormatting = true;
                    break;
                }
            }
            if (!isFormatting) {
                throw std::runtime_error("display_slot: 未知 ChatFormatting：" + parts[1]);
            }
            value = "null";
        }
    } else {
        throw std::runtime_error("display_slot: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("display_slot", &writeList, &runLine)

} // namespace mc::diff
