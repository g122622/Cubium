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
 * @brief 差分能力：`BossEvent.BossBarColor` / `BossBarOverlay`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.BossEvent`：
 *
 * ```java
 * public enum BossBarColor implements StringRepresentable {
 *     PINK("pink", ChatFormatting.RED),
 *     BLUE("blue", ChatFormatting.BLUE),
 *     RED("red", ChatFormatting.DARK_RED),
 *     GREEN("green", ChatFormatting.GREEN),
 *     YELLOW("yellow", ChatFormatting.YELLOW),
 *     PURPLE("purple", ChatFormatting.DARK_BLUE),
 *     WHITE("white", ChatFormatting.WHITE);
 *
 *     public ChatFormatting getFormatting() { return this.formatting; }
 *     public String getName() { return this.name; }
 *     public String getSerializedName() { return this.name; }
 * }
 *
 * public enum BossBarOverlay implements StringRepresentable {
 *     PROGRESS("progress"),
 *     NOTCHED_6("notched_6"),
 *     NOTCHED_10("notched_10"),
 *     NOTCHED_12("notched_12"),
 *     NOTCHED_20("notched_20");
 *
 *     public String getName() { return this.name; }
 *     public String getSerializedName() { return this.name; }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`BossBarColor` 的 `ChatFormatting` 映射极不直观。**
 *
 *    | `BossBarColor` | 序列化名 | `ChatFormatting` |
 *    |---|---|---|
 *    | **`PINK`** | `pink` | **`RED`** |
 *    | `BLUE` | `blue` | `BLUE` |
 *    | **`RED`** | `red` | **`DARK_RED`** |
 *    | `GREEN` | `green` | `GREEN` |
 *    | `YELLOW` | `yellow` | `YELLOW` |
 *    | **`PURPLE`** | `purple` | **`DARK_BLUE`** |
 *    | `WHITE` | `white` | `WHITE` |
 *
 *    **三处名字不匹配**：
 *    - `PINK` → `RED`（粉色用亮红渲染）
 *    - `RED` → `DARK_RED`（红色用暗红渲染）
 *    - `PURPLE` → `DARK_BLUE`（紫色用深蓝渲染）
 *
 *    若 Cubium 按名字直接映射（`BossBarColor.valueOf(name)` 再取同名 `ChatFormatting`），
 *    **7 项里有 3 项会错**。
 *
 * 2. **两个枚举的命名风格不一致。**
 *
 *    - `BossBarColor`：全小写（`pink`、`blue`、`red`…）
 *    - `BossBarOverlay`：**保留下划线**（`notched_6`、`notched_10`…）
 *
 *    `NOTCHED_6` 的序列化名是 `notched_6`，**不是** `notched6`。
 *
 * 3. **`BossBarOverlay` 的数字是「等分格数」，不是「像素数」。**
 *
 *    `notched_6` 表示 6 等分，`notched_10` / `notched_12` / `notched_20` 同理。
 *
 *    **跳过了 8 和 16**——没有 `notched_8` 或 `notched_16`。
 *
 * 4. **`getName()` 与 `getSerializedName()` 返回同一个字段。**
 *
 *    两个方法都是 `return this.name;`——没有独立的人类可读名。
 *
 * 单元格式：
 * - `colorName|<color>` —— `BossBarColor.getName()`
 * - `colorSerial|<color>` —— `BossBarColor.getSerializedName()`
 * - `colorFormatting|<color>` —— `BossBarColor.getFormatting().getName()`
 * - `overlayName|<overlay>` —— `BossBarOverlay.getName()`
 * - `overlaySerial|<overlay>` —— `BossBarOverlay.getSerializedName()`
 * - `colorCount` / `overlayCount` —— 枚举项数
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

/// `BossBarColor`：序列化名 + 对应的 `ChatFormatting` 名。
struct BossBarColor {
    const char* enumName;
    const char* serializedName;
    const char* formattingName;
};

constexpr BossBarColor COLORS[] = {
    {"PINK", "pink", "red"},
    {"BLUE", "blue", "blue"},
    {"RED", "red", "dark_red"},
    {"GREEN", "green", "green"},
    {"YELLOW", "yellow", "yellow"},
    {"PURPLE", "purple", "dark_blue"},
    {"WHITE", "white", "white"},
};

constexpr i32 COLOR_COUNT = 7;

/// `BossBarOverlay`。
struct BossBarOverlay {
    const char* enumName;
    const char* serializedName;
};

constexpr BossBarOverlay OVERLAYS[] = {
    {"PROGRESS", "progress"},
    {"NOTCHED_6", "notched_6"},
    {"NOTCHED_10", "notched_10"},
    {"NOTCHED_12", "notched_12"},
    {"NOTCHED_20", "notched_20"},
};

constexpr i32 OVERLAY_COUNT = 5;

const BossBarColor* findColor(const std::string& name)
{
    for (const BossBarColor& color : COLORS) {
        if (name == color.enumName) {
            return &color;
        }
    }
    throw std::runtime_error("boss_bar: 未知颜色：" + name);
}

const BossBarOverlay* findOverlay(const std::string& name)
{
    for (const BossBarOverlay& overlay : OVERLAYS) {
        if (name == overlay.enumName) {
            return &overlay;
        }
    }
    throw std::runtime_error("boss_bar: 未知叠加层：" + name);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // colorName：全部 7 项
        "colorName|PINK",
        "colorName|BLUE",
        "colorName|RED",
        "colorName|GREEN",
        "colorName|YELLOW",
        "colorName|PURPLE",
        "colorName|WHITE",
        // colorSerial
        "colorSerial|PINK",
        "colorSerial|RED",
        // colorFormatting：名字不匹配的三项
        "colorFormatting|PINK",
        "colorFormatting|RED",
        "colorFormatting|PURPLE",
        // colorFormatting：名字匹配的三项
        "colorFormatting|BLUE",
        "colorFormatting|GREEN",
        "colorFormatting|YELLOW",
        "colorFormatting|WHITE",
        // overlayName：保留下划线
        "overlayName|PROGRESS",
        "overlayName|NOTCHED_6",
        "overlayName|NOTCHED_10",
        "overlayName|NOTCHED_12",
        "overlayName|NOTCHED_20",
        // overlaySerial
        "overlaySerial|NOTCHED_6",
        "overlaySerial|NOTCHED_20",
        // count
        "colorCount",
        "overlayCount",
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
        throw std::runtime_error("boss_bar: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "colorCount" && parts.size() == 1) {
        value = std::to_string(COLOR_COUNT);
    } else if (mode == "overlayCount" && parts.size() == 1) {
        value = std::to_string(OVERLAY_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("boss_bar: 单元段数不符：" + unit);
    } else if (mode == "colorName") {
        value = findColor(parts[1])->serializedName;
    } else if (mode == "colorSerial") {
        value = findColor(parts[1])->serializedName;
    } else if (mode == "colorFormatting") {
        value = findColor(parts[1])->formattingName;
    } else if (mode == "overlayName") {
        value = findOverlay(parts[1])->serializedName;
    } else if (mode == "overlaySerial") {
        value = findOverlay(parts[1])->serializedName;
    } else {
        throw std::runtime_error("boss_bar: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("boss_bar", &writeList, &runLine)

} // namespace mc::diff
