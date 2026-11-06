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
 * @brief 差分能力：`DyeColor` 的染色值与查找（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.item.DyeColor`：
 *
 * ```java
 * WHITE(0, "white", 16383998, MapColor.SNOW,            15790320, 16777215)
 * ORANGE(1, "orange", 16351261, MapColor.COLOR_ORANGE,  15435844, 16738335)
 * MAGENTA(2, "magenta", 13061821, MapColor.COLOR_MAGENTA, 12801229, 16711935)
 * LIGHT_BLUE(3, "light_blue", 3847130, MapColor.COLOR_LIGHT_BLUE, 6719955, 10141901)
 * YELLOW(4, "yellow", 16701501, MapColor.COLOR_YELLOW,  14602026, 16776960)
 * LIME(5, "lime", 8439583, MapColor.COLOR_LIGHT_GREEN,  4312372, 12582656)
 * PINK(6, "pink", 15961002, MapColor.COLOR_PINK,        14188952, 16738740)
 * GRAY(7, "gray", 4673362, MapColor.COLOR_GRAY,         4408131, 8421504)
 * LIGHT_GRAY(8, "light_gray", 10329495, MapColor.COLOR_LIGHT_GRAY, 11250603, 13882323)
 * CYAN(9, "cyan", 1481884, MapColor.COLOR_CYAN,         2651799, 65535)
 * PURPLE(10, "purple", 8991416, MapColor.COLOR_PURPLE,  8073150, 10494192)
 * BLUE(11, "blue", 3949738, MapColor.COLOR_BLUE,        2437522, 255)
 * BROWN(12, "brown", 8606770, MapColor.COLOR_BROWN,     5320730, 9127187)
 * GREEN(13, "green", 6192150, MapColor.COLOR_GREEN,     3887386, 65280)
 * RED(14, "red", 11546150, MapColor.COLOR_RED,          11743532, 16711680)
 * BLACK(15, "black", 1908001, MapColor.COLOR_BLACK,     1973019, 0)
 *
 * // 构造器里：textureDiffuseColor = ARGB.opaque(rgb)
 * private static final IntFunction<DyeColor> BY_ID =
 *     ByIdMap.continuous(DyeColor::getId, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
 * private static final Int2ObjectOpenHashMap<DyeColor> BY_FIREWORK_COLOR = ...;
 *
 * public int getTextureDiffuseColor() { return this.textureDiffuseColor; }
 * public int getFireworkColor() { return this.fireworkColor; }
 * public int getTextColor() { return this.textColor; }
 * public static DyeColor byId(int id) { return BY_ID.apply(id); }
 * public static DyeColor byFireworkColor(int color) { return BY_FIREWORK_COLOR.get(color); }
 * public static DyeColor byName(String name, DyeColor fallback) { ... }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **三个颜色字段互不相同，用途各异。**
 *
 *    | 字段 | 含义 | WHITE 的值 |
 *    |---|---|---|
 *    | `rgb` | 原始 RGB（无 alpha） | `16383998` = `0xF9FFFE` |
 *    | `textureDiffuseColor` | `ARGB.opaque(rgb)` = `0xFF000000 \| rgb` | `-385556` （有符号） |
 *    | `fireworkColor` | 烟花火箭的颜色（**独立**，不是 rgb） | `15790320` = `0xF0F0F0` |
 *    | `textColor` | 聊天文本颜色（**又不同**） | `16777215` = `0xFFFFFF` |
 *
 *    **`fireworkColor` 与 `textColor` 都不是从 `rgb` 派生的**，三个值各自独立。例如 WHITE 的
 *    `rgb` 是 `0xF9FFFE`（略微偏黄的白色），但 `textColor` 是纯白 `0xFFFFFF`。
 *
 * 2. **`byId` 越界返回 `WHITE`（id 0），不返回 `null`。**
 *
 *    ```java
 *    private static final IntFunction<DyeColor> BY_ID =
 *        ByIdMap.continuous(DyeColor::getId, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
 *    // ZERO 策略：id >= 0 && id < 16 ? at[id] : at[0]
 *    ```
 *
 *    `byId(-1)`、`byId(16)`、`byId(999)` **全部返回 `WHITE`**。这与 `ChatFormatting.getById`
 *    的行为（负数返回 `RESET`，正数越界返回 `null`）**完全不同**，两套枚举的越界策略不一致。
 *
 * 3. **`byFireworkColor` 按值查找，找不到返回 `null`。**
 *
 *    `BY_FIREWORK_COLOR` 是 `fireworkColor → DyeColor` 的哈希表。因为 16 个
 *    `fireworkColor` 互不相同，查表是全单射。传入不存在的颜色返回 `null`。
 *
 * 4. **`byName` 需要回退值。**
 *
 *    ```java
 *    public static DyeColor byName(String name, DyeColor fallback) {
 *        DyeColor found = CODEC.byName(name);
 *        return found != null ? found : fallback;
 *    }
 *    ```
 *
 *    名字匹配用 `getSerializedName()`（即小写名，如 `"light_blue"`），**大小写敏感**，
 *    与 `ChatFormatting.getByName` 的宽松 `cleanName` 完全不同。
 *
 * 单元格式：
 * - `byId|<id>` —— 输出枚举名
 * - `byName|<名字>` —— 严格匹配 `serializedName`
 * - `byFirework|<color>` —— 输出枚举名或 `null`
 * - `info|<枚举名>` —— 输出 `id,rgb,diffuse,firework,textColor,name`
 *
 * TODO: 未覆盖 `getMixedColor`（依赖 `ServerLevel` 与配方系统）、`getMapColor` 的
 * `MapColor` 对象（只输出其 id 需要额外数据），以及 `CODEC` / `STREAM_CODEC` 的序列化形式。
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/color/DyeColor.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using mc::DyeColor;
using nlohmann::json;

/// 原版枚举名（大写），对应 Java `DyeColor.name()`。
std::string enumNameOf(DyeColor color)
{
    std::string result(dyeColorName(color));
    for (char& ch : result) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return result;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // byId：全部合法 id
        "byId|0",
        "byId|1",
        "byId|2",
        "byId|3",
        "byId|4",
        "byId|5",
        "byId|6",
        "byId|7",
        "byId|8",
        "byId|9",
        "byId|10",
        "byId|11",
        "byId|12",
        "byId|13",
        "byId|14",
        "byId|15",
        // byId：越界（全部返回 WHITE）
        "byId|-1",
        "byId|16",
        "byId|999",
        "byId|-999",
        // byName：严格大小写与小写名
        "byName|white",
        "byName|light_blue",
        "byName|WHITE",
        "byName|Light_Blue",
        "byName|orange",
        "byName|not_a_color",
        // byFirework：合法与非法
        "byFirework|15790320",
        "byFirework|16777215",
        "byFirework|0",
        "byFirework|12345",
        "byFirework|15435844",
        // info：抽查若干
        "info|WHITE",
        "info|ORANGE",
        "info|LIGHT_BLUE",
        "info|BLACK",
        "info|RED",
        "info|CYAN",
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
        throw std::runtime_error("dye_color: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "byId" && parts.size() == 2) {
        const DyeColor found = dyeColorById(std::stoi(parts[1]));
        return json{{"unit", unit}, {"value", enumNameOf(found)}}.dump();
    }
    if (mode == "byName" && parts.size() == 2) {
        const std::optional<DyeColor> found = dyeColorByName(parts[1]);
        return json{{"unit", unit}, {"value", found.has_value() ? enumNameOf(*found) : "null"}}.dump();
    }
    if (mode == "byFirework" && parts.size() == 2) {
        const std::optional<DyeColor> found = dyeColorByFireworkColor(static_cast<u32>(std::stoi(parts[1])));
        return json{{"unit", unit}, {"value", found.has_value() ? enumNameOf(*found) : "null"}}.dump();
    }
    if (mode == "info" && parts.size() == 2) {
        // info 的输入是枚举名（大写），byName 认的是小写序列化名，这里统一转小写
        std::string lowered;
        for (char ch : parts[1]) {
            lowered.push_back((ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch);
        }
        const std::optional<DyeColor> found = dyeColorByName(lowered);
        if (!found.has_value()) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        // 输出顺序与原 oracle 一致：id, textureDiffuse, diffuseArg, firework, textArg, 序列化名。
        // 打包值按 Java int 的**有符号**形式输出（如 0xFFFFFFFF 写作 -1）。
        const auto signedInt = [](u32 value) {
            return static_cast<i32>(value);
        };
        const std::string value = std::to_string(static_cast<i32>(*found)) + "," +
            std::to_string(dyeTextureDiffuseColor(*found)) + "," +
            std::to_string(signedInt(0xFF000000U | dyeTextureDiffuseColor(*found))) + "," +
            std::to_string(dyeFireworkColor(*found)) + "," +
            std::to_string(signedInt(0xFF000000U | dyeTextColor(*found))) + "," + std::string(dyeColorName(*found));
        return json{{"unit", unit}, {"value", value}}.dump();
    }

    throw std::runtime_error("dye_color: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(dye_color, &writeList, &runLine)

} // namespace mc::diff
