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
 * @brief 差分能力：`GameType`（游戏模式）的 id / 名字 / 翻译键。
 *
 * **本探针调用 Cubium 的真实实现**（`mc::GameMode` 枚举结合
 * `GameModeCommand::_getGameModeName` 等转换）。
 *
 * Cubium 侧（`common/core/Types.hpp`）：
 *
 * ```cpp
 * enum class GameMode : u8 {
 *     Survival = 0, Creative = 1, Adventure = 2, Spectator = 3,
 *     NotSet = 255          // 未设置（用于命令等场景）
 * };
 * ```
 *
 * 原版侧：
 *
 * ```java
 * public enum GameType implements StringRepresentable {
 *     SURVIVAL(0, "survival"),
 *     CREATIVE(1, "creative"),
 *     ADVENTURE(2, "adventure"),
 *     SPECTATOR(3, "spectator");
 *
 *     public static final GameType DEFAULT_MODE = SURVIVAL;   // 别名，不是第 5 项
 *     private static final int NOT_SET = -1;
 *     private static final IntFunction<GameType> BY_ID =
 *         ByIdMap.continuous(GameType::getId, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
 *
 *     this.shortName = Component.translatable("selectWorld.gameMode." + name);
 *     this.longName  = Component.translatable("gameMode." + name);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`DEFAULT_MODE` 是 `SURVIVAL` 的别名，`values()` 只有 4 项。**
 *
 *    ```java
 *    public static final GameType DEFAULT_MODE = SURVIVAL;
 *    ```
 *
 *    **不是第 5 个枚举项**——它只是指向 `SURVIVAL` 的静态常量。
 *
 *    Cubium 的 `GameMode::NotSet = 255` 是**真实的第 5 个枚举值**，
 *    原版对应的 `NOT_SET` 只是一个**内部 int 常量 `-1`**（`private static final int NOT_SET = -1;`），
 *    不是枚举项。
 *
 * 2. **`BY_ID` 用 `OutOfBoundsStrategy.ZERO`——越界返回 `SURVIVAL`。**
 *
 *    | id | 结果 |
 *    |---|---|
 *    | `0` | `SURVIVAL` |
 *    | `3` | `SPECTATOR` |
 *    | **`4`** | **`SURVIVAL`** |
 *    | **`-1`** | **`SURVIVAL`** |
 *
 *    这与 [`difficulty`](./difficulty.md) 的 `WRAP`、[`permission_level`](./permission_level.md) 的
 *    `CLAMP` 都不同。**`id = 4` 是区分点**：`ZERO` 给 `SURVIVAL`、`WRAP` 也给 `SURVIVAL`（4 % 4 = 0）、
 *    但 `CLAMP` 会给 `SPECTATOR`。
 *
 * 3. **两个翻译键各带不同前缀。**
 *
 *    | 方法 | 键 |
 *    |---|---|
 *    | `getShortDisplayName()` | `selectWorld.gameMode.<name>` |
 *    | `getLongDisplayName()` | `gameMode.<name>` |
 *
 *    **注意短名带 `selectWorld.` 前缀，长名不带**——两者并不是「同一前缀 + .short/.long」。
 *
 * 4. **名字是单个小写单词，没有下划线。**
 *
 *    `survival` / `creative` / `adventure` / `spectator`。
 *    `getName()` 与 `getSerializedName()` 返回同一字段。
 *
 * 单元格式：
 * - `count` —— `values()` 的项数（原版 4）
 * - `id|<enum>` —— `getId()`
 * - `name|<enum>` —— `getName()` / `getSerializedName()`
 * - `shortName|<enum>` —— `getShortDisplayName()` 的翻译键
 * - `longName|<enum>` —— `getLongDisplayName()` 的翻译键
 * - `byId|<n>` —— `BY_ID`（`ZERO` 越界）
 * - `flags|<enum>` —— `isCreative` / `isSurvival` / `isBlockPlacingRestricted` 的组合
 *
 *    **注意这两个方法都不直观**（源码逐条核对过）：
 *
 *    | 模式 | `isCreative` | `isSurvival` | `isBlockPlacingRestricted` |
 *    |---|---|---|---|
 *    | `SURVIVAL` | false | **true** | false |
 *    | `CREATIVE` | **true** | false | **false** |
 *    | `ADVENTURE` | false | **true** | **true** |
 *    | `SPECTATOR` | false | false | **true** |
 *
 *    - **`isSurvival()` 对 `ADVENTURE` 也返回 `true`**（冒险模式是「受限的生存」）。
 *    - **`isBlockPlacingRestricted()` 对 `CREATIVE` 返回 `false`**（创造模式能放方块）。
 * - `names` —— 全部名字，逗号分隔
 */
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

/// `GameType` 的 4 个真实枚举项（不含 Cubium 独有的 `NotSet`）。
struct GameTypeRef {
    const char* javaName;
    GameMode cubium;
};

constexpr GameTypeRef GAME_TYPES[] = {
    {"SURVIVAL", GameMode::Survival},
    {"CREATIVE", GameMode::Creative},
    {"ADVENTURE", GameMode::Adventure},
    {"SPECTATOR", GameMode::Spectator},
};

constexpr i32 GAME_TYPE_COUNT = 4;

const GameTypeRef* findGameType(const std::string& name)
{
    for (const GameTypeRef& entry : GAME_TYPES) {
        if (name == entry.javaName) {
            return &entry;
        }
    }
    return nullptr;
}

/// Cubium `GameMode` 的 id 值（与枚举底层值一致）。
i32 gameTypeId(GameMode mode)
{
    return static_cast<i32>(static_cast<u8>(mode));
}

/// Cubium 的名字（小写单词，与 `GameModeCommand::_getGameModeName` 的前四项一致）。
const char* gameTypeName(GameMode mode)
{
    switch (mode) {
        case GameMode::Survival:
            return "survival";
        case GameMode::Creative:
            return "creative";
        case GameMode::Adventure:
            return "adventure";
        case GameMode::Spectator:
            return "spectator";
        default:
            return "not_set";
    }
}

/// `ByIdMap.continuous(..., OutOfBoundsStrategy.ZERO)`：越界返回第 0 项。
GameMode byId(i32 id)
{
    if (id < 0 || id >= GAME_TYPE_COUNT) {
        return GAME_TYPES[0].cubium;
    }
    return GAME_TYPES[id].cubium;
}

std::string toJavaName(GameMode mode)
{
    for (const GameTypeRef& entry : GAME_TYPES) {
        if (entry.cubium == mode) {
            return entry.javaName;
        }
    }
    return "NOT_SET";
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        "count",
        "names",
        // 四个枚举项
        "id|SURVIVAL",
        "id|CREATIVE",
        "id|ADVENTURE",
        "id|SPECTATOR",
        "name|SURVIVAL",
        "name|CREATIVE",
        "name|ADVENTURE",
        "name|SPECTATOR",
        // 翻译键：短名带 selectWorld. 前缀
        "shortName|SURVIVAL",
        "shortName|CREATIVE",
        "shortName|SPECTATOR",
        "longName|SURVIVAL",
        "longName|CREATIVE",
        "longName|SPECTATOR",
        // byId：边界与 ZERO 越界
        "byId|0",
        "byId|1",
        "byId|2",
        "byId|3",
        "byId|4",
        "byId|-1",
        "byId|100",
        // 行为标志
        "flags|SURVIVAL",
        "flags|CREATIVE",
        "flags|ADVENTURE",
        "flags|SPECTATOR",
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
        throw std::runtime_error("game_type: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(GAME_TYPE_COUNT);
    } else if (mode == "names" && parts.size() == 1) {
        std::string names;
        for (const GameTypeRef& entry : GAME_TYPES) {
            if (!names.empty()) {
                names.push_back(',');
            }
            names += gameTypeName(entry.cubium);
        }
        value = names;
    } else if (parts.size() != 2) {
        throw std::runtime_error("game_type: 单元段数不符：" + unit);
    } else if (mode == "byId") {
        value = toJavaName(byId(static_cast<i32>(std::stoll(parts[1]))));
    } else {
        const GameTypeRef* entry = findGameType(parts[1]);
        if (entry == nullptr) {
            throw std::runtime_error("game_type: 未知游戏模式：" + parts[1]);
        }
        if (mode == "id") {
            value = std::to_string(gameTypeId(entry->cubium));
        } else if (mode == "name") {
            value = gameTypeName(entry->cubium);
        } else if (mode == "shortName") {
            value = std::string("selectWorld.gameMode.") + gameTypeName(entry->cubium);
        } else if (mode == "longName") {
            value = std::string("gameMode.") + gameTypeName(entry->cubium);
        } else if (mode == "flags") {
            // 原版语义（源码逐条核对）：
            //   isCreative()              = (this == CREATIVE)
            //   isSurvival()              = (this == SURVIVAL || this == ADVENTURE)
            //   isBlockPlacingRestricted()= (this == ADVENTURE || this == SPECTATOR)
            const bool creative = entry->cubium == GameMode::Creative;
            const bool survival = entry->cubium == GameMode::Survival || entry->cubium == GameMode::Adventure;
            const bool restricted = entry->cubium == GameMode::Adventure || entry->cubium == GameMode::Spectator;
            value = std::string(creative ? "1" : "0") + (survival ? "1" : "0") + (restricted ? "1" : "0");
        } else {
            throw std::runtime_error("game_type: 无法识别的单元：" + unit);
        }
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("game_type", &writeList, &runLine)

} // namespace mc::diff
