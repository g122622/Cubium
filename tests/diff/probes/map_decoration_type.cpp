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
 * @brief 差分能力：地图装饰类型（`MapDecorationTypes`）的注册名映射。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`mc::world::map::decorationTypeToString` / `decorationTypeFromString`）。
 *
 * 原版 1.21.11 的地图装饰已从枚举改为**注册表对象**，序列化名是注册表键：
 *
 * ```java
 * public static final Holder<MapDecorationType> WOODLAND_MANSION =
 *     register("mansion", "woodland_mansion", true, 5393476, false, true);
 * //         ^^^^^^^^^  注册名（序列化用）  ^^^^^^^^^^^^^^^^ 纹理 id
 * ```
 *
 * **`register(资源名, 纹理名, ...)` 的第一个参数才是序列化名。**
 *
 * ## 核心发现：村庄的资源名与纹理名顺序相反
 *
 * | 常量 | 资源名（**序列化**） | 纹理名 |
 * |---|---|---|
 * | `WOODLAND_MANSION` | `mansion` | `woodland_mansion` |
 * | `OCEAN_MONUMENT` | `monument` | `ocean_monument` |
 * | **`DESERT_VILLAGE`** | **`village_desert`** | `desert_village` |
 * | **`PLAINS_VILLAGE`** | **`village_plains`** | `plains_village` |
 * | **`SAVANNA_VILLAGE`** | **`village_savanna`** | `savanna_village` |
 * | **`SNOWY_VILLAGE`** | **`village_snowy`** | `snowy_village` |
 * | **`TAIGA_VILLAGE`** | **`village_taiga`** | `taiga_village` |
 * | `WHITE_BANNER` | `banner_white` | `white_banner` |
 *
 * **两套命名规则互相矛盾**：
 * - 府邸/神殿：序列化名是**短名**（`mansion`），纹理是长名
 * - 村庄/旗帜：序列化名是**前缀式**（`village_desert` / `banner_white`），纹理是后缀式
 *
 * **Cubium 的 `decorationTypeToString` 对村庄返回纹理名 `desert_village`**，
 * 而原版序列化名是 `village_desert`——这是写存档/网络时的不兼容。
 *
 * 单元格式：
 * - `toString|<enum>` —— `decorationTypeToString`
 * - `fromString|<text>` —— `decorationTypeFromString`（未命中输出 `null`）
 * - `count` —— 枚举项数（不含 `COUNT` 哨兵）
 */
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/map/MapDecoration.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 35 个真实的装饰类型（排除 `COUNT` 哨兵）。
struct DecorationRef {
    const char* enumName;
    world::map::DecorationType cubium;
};

constexpr DecorationRef DECORATIONS[] = {
    {"PLAYER", world::map::DecorationType::PLAYER},
    {"FRAME", world::map::DecorationType::FRAME},
    {"RED_MARKER", world::map::DecorationType::RED_MARKER},
    {"BLUE_MARKER", world::map::DecorationType::BLUE_MARKER},
    {"TARGET_X", world::map::DecorationType::TARGET_X},
    {"TARGET_POINT", world::map::DecorationType::TARGET_POINT},
    {"PLAYER_OFF_MAP", world::map::DecorationType::PLAYER_OFF_MAP},
    {"PLAYER_OFF_LIMITS", world::map::DecorationType::PLAYER_OFF_LIMITS},
    {"WOODLAND_MANSION", world::map::DecorationType::MANSION},
    {"OCEAN_MONUMENT", world::map::DecorationType::MONUMENT},
    {"WHITE_BANNER", world::map::DecorationType::BANNER_WHITE},
    {"ORANGE_BANNER", world::map::DecorationType::BANNER_ORANGE},
    {"MAGENTA_BANNER", world::map::DecorationType::BANNER_MAGENTA},
    {"LIGHT_BLUE_BANNER", world::map::DecorationType::BANNER_LIGHT_BLUE},
    {"YELLOW_BANNER", world::map::DecorationType::BANNER_YELLOW},
    {"LIME_BANNER", world::map::DecorationType::BANNER_LIME},
    {"PINK_BANNER", world::map::DecorationType::BANNER_PINK},
    {"GRAY_BANNER", world::map::DecorationType::BANNER_GRAY},
    {"LIGHT_GRAY_BANNER", world::map::DecorationType::BANNER_LIGHT_GRAY},
    {"CYAN_BANNER", world::map::DecorationType::BANNER_CYAN},
    {"PURPLE_BANNER", world::map::DecorationType::BANNER_PURPLE},
    {"BLUE_BANNER", world::map::DecorationType::BANNER_BLUE},
    {"BROWN_BANNER", world::map::DecorationType::BANNER_BROWN},
    {"GREEN_BANNER", world::map::DecorationType::BANNER_GREEN},
    {"RED_BANNER", world::map::DecorationType::BANNER_RED},
    {"BLACK_BANNER", world::map::DecorationType::BANNER_BLACK},
    {"RED_X", world::map::DecorationType::RED_X},
    {"DESERT_VILLAGE", world::map::DecorationType::DESERT_VILLAGE},
    {"PLAINS_VILLAGE", world::map::DecorationType::PLAINS_VILLAGE},
    {"SAVANNA_VILLAGE", world::map::DecorationType::SAVANNA_VILLAGE},
    {"SNOWY_VILLAGE", world::map::DecorationType::SNOWY_VILLAGE},
    {"TAIGA_VILLAGE", world::map::DecorationType::TAIGA_VILLAGE},
    {"JUNGLE_TEMPLE", world::map::DecorationType::JUNGLE_TEMPLE},
    {"SWAMP_HUT", world::map::DecorationType::SWAMP_HUT},
    {"TRIAL_CHAMBERS", world::map::DecorationType::TRIAL_CHAMBERS},
};

constexpr i32 DECORATION_COUNT = 35;

const DecorationRef* findDecoration(const std::string& name)
{
    for (const DecorationRef& entry : DECORATIONS) {
        if (name == entry.enumName) {
            return &entry;
        }
    }
    return nullptr;
}

/// 把 Cubium 的枚举值转回原版常量名。
std::string toJavaName(world::map::DecorationType value)
{
    for (const DecorationRef& entry : DECORATIONS) {
        if (entry.cubium == value) {
            return entry.enumName;
        }
    }
    return "null";
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        "count",
        // 前 10 项（无命名冲突）
        "toString|PLAYER",
        "toString|FRAME",
        "toString|RED_MARKER",
        "toString|BLUE_MARKER",
        "toString|TARGET_X",
        "toString|TARGET_POINT",
        "toString|PLAYER_OFF_MAP",
        "toString|PLAYER_OFF_LIMITS",
        // 短序列化名 + 长纹理名
        "toString|WOODLAND_MANSION",
        "toString|OCEAN_MONUMENT",
        // 旗帜：banner_<color> 前缀式
        "toString|WHITE_BANNER",
        "toString|ORANGE_BANNER",
        "toString|MAGENTA_BANNER",
        "toString|LIGHT_BLUE_BANNER",
        "toString|YELLOW_BANNER",
        "toString|LIME_BANNER",
        "toString|PINK_BANNER",
        "toString|GRAY_BANNER",
        "toString|LIGHT_GRAY_BANNER",
        "toString|CYAN_BANNER",
        "toString|PURPLE_BANNER",
        "toString|BLUE_BANNER",
        "toString|BROWN_BANNER",
        "toString|GREEN_BANNER",
        "toString|RED_BANNER",
        "toString|BLACK_BANNER",
        "toString|RED_X",
        // 村庄：序列化名是 village_<biome>，Cubium 输出 <biome>_village——关键差异点
        "toString|DESERT_VILLAGE",
        "toString|PLAINS_VILLAGE",
        "toString|SAVANNA_VILLAGE",
        "toString|SNOWY_VILLAGE",
        "toString|TAIGA_VILLAGE",
        // 其余
        "toString|JUNGLE_TEMPLE",
        "toString|SWAMP_HUT",
        "toString|TRIAL_CHAMBERS",
        // fromString：接受两种写法
        "fromString|mansion",
        "fromString|woodland_mansion",
        "fromString|village_desert",
        "fromString|desert_village",
        "fromString|village_taiga",
        // fromString：未命中
        "fromString|dunno",
        "fromString|",
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
        throw std::runtime_error("map_decoration_type: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(DECORATION_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("map_decoration_type: 单元段数不符：" + unit);
    } else if (mode == "toString") {
        const DecorationRef* entry = findDecoration(parts[1]);
        if (entry == nullptr) {
            throw std::runtime_error("map_decoration_type: 未知装饰类型：" + parts[1]);
        }
        value = world::map::decorationTypeToString(entry->cubium);
    } else if (mode == "fromString") {
        const std::optional<world::map::DecorationType> parsed = world::map::decorationTypeFromString(parts[1]);
        value = parsed ? toJavaName(*parsed) : "null";
    } else {
        throw std::runtime_error("map_decoration_type: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(map_decoration_type, &writeList, &runLine)

} // namespace mc::diff
