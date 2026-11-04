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
 * @brief 差分能力：`SoundSource`（声音通道枚举，纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.sounds.SoundSource`：
 *
 * ```java
 * public enum SoundSource {
 *     MASTER("master"),
 *     MUSIC("music"),
 *     RECORDS("record"),
 *     WEATHER("weather"),
 *     BLOCKS("block"),
 *     HOSTILE("hostile"),
 *     NEUTRAL("neutral"),
 *     PLAYERS("player"),
 *     AMBIENT("ambient"),
 *     VOICE("voice"),
 *     UI("ui");
 *
 *     private final String name;
 *
 *     public String getName() { return this.name; }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`SoundSource` 没有实现 `StringRepresentable`，也不提供 `CODEC`。**
 *
 *    它只有 `getName()`——**没有 `getSerializedName()`**。
 *    这与 [`objective_criteria`](./objective_criteria.md) 之外的绝大多数枚举不同，
 *    也与 [`boss_bar`](./boss_bar.md)（两者都有）不同。
 *
 *    **后果**：`SoundSource` 无法直接走 `StringRepresentable.fromEnum` 序列化，
 *    需要手写的 `Map` 或 switch。
 *
 * 2. **三个复数枚举映射到单数名字。**
 *
 *    | 枚举 | `getName()` | 说明 |
 *    |---|---|---|
 *    | **`RECORDS`** | **`record`** | 单数 |
 *    | **`BLOCKS`** | **`block`** | 单数 |
 *    | **`PLAYERS`** | **`player`** | 单数 |
 *
 *    其余 8 项都是「枚举名小写」。
 *
 *    **若 Cubium 统一用「枚举名小写」，会让这 3 项出错**（`records` 而非 `record`）。
 *
 * 3. **`MASTER` 与 `MUSIC` 是「全局音量通道」，不是空间音源。**
 *
 *    `master` 是总音量，`music` 是背景音乐——两者不参与 3D 定位。
 *    其余 9 项（`record` / `weather` / `block` / `hostile` / `neutral` / `player` /
 *    `ambient` / `voice` / `ui`）是分类音源。
 *
 *    这一区别**不影响 `getName()`**，但解释了为什么 `master` 在列表最前。
 *
 * 4. **顺序是「全局 → 世界 → 生物 → 界面」的粗分类，不是字母序。**
 *
 *    `master` → `music` → `record` → `weather` → `block` → `hostile` → `neutral` →
 *    `player` → `ambient` → `voice` → `ui`
 *
 *    **`ui` 在最后**，`ambient` 在 `voice` 前——不是字母序。
 *
 * 单元格式：
 * - `name|<source>` —— `getName()`
 * - `count` —— 枚举项数
 * - `byName|<name>` —— 按 `getName()` 反查枚举名（无匹配输出 `null`）
 * - `names` —— 全部 `getName()`，逗号分隔
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

/// `SoundSource` 的 11 个枚举项，按序数排列。
struct SoundSource {
    const char* enumName;
    const char* name;
};

constexpr SoundSource SOURCES[] = {
    {"MASTER", "master"},
    {"MUSIC", "music"},
    {"RECORDS", "record"},
    {"WEATHER", "weather"},
    {"BLOCKS", "block"},
    {"HOSTILE", "hostile"},
    {"NEUTRAL", "neutral"},
    {"PLAYERS", "player"},
    {"AMBIENT", "ambient"},
    {"VOICE", "voice"},
    {"UI", "ui"},
};

constexpr i32 SOURCE_COUNT = 11;

const SoundSource* findSource(const std::string& name)
{
    for (const SoundSource& source : SOURCES) {
        if (name == source.enumName) {
            return &source;
        }
    }
    throw std::runtime_error("sound_source: 未知枚举名：" + name);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // name：全部 11 项
        "name|MASTER",
        "name|MUSIC",
        "name|RECORDS",
        "name|WEATHER",
        "name|BLOCKS",
        "name|HOSTILE",
        "name|NEUTRAL",
        "name|PLAYERS",
        "name|AMBIENT",
        "name|VOICE",
        "name|UI",
        // byName：复数枚举反查单数名
        "byName|record",
        "byName|block",
        "byName|player",
        "byName|master",
        "byName|ui",
        // byName：不存在的名字
        "byName|records",
        "byName|blocks",
        "byName|players",
        "byName|MASTER",
        "byName|",
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
        throw std::runtime_error("sound_source: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(SOURCE_COUNT);
    } else if (mode == "names" && parts.size() == 1) {
        std::string names;
        for (const SoundSource& source : SOURCES) {
            if (!names.empty()) {
                names.push_back(',');
            }
            names += source.name;
        }
        value = names;
    } else if (parts.size() != 2) {
        throw std::runtime_error("sound_source: 单元段数不符：" + unit);
    } else if (mode == "name") {
        value = findSource(parts[1])->name;
    } else if (mode == "byName") {
        value = "null";
        for (const SoundSource& source : SOURCES) {
            if (parts[1] == source.name) {
                value = source.enumName;
                break;
            }
        }
    } else {
        throw std::runtime_error("sound_source: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(sound_source, &writeList, &runLine)

} // namespace mc::diff
