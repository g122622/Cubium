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
 * @brief 差分能力：维度类型参数（`DimensionType` 的全部字段）。
 *
 * `list`：每个维度 × 每个字段
 * `run` ：一行 -> { "value": ... }
 *
 * ## 为什么需要它
 *
 * `dimension_type` 是**数据包驱动的注册表**（`data/<ns>/dimension_type/*.json`），
 * 里面的每个字段都直接决定游戏行为，而且彼此独立、互不牵连：
 *
 * - `coordinate_scale` 决定下界与主世界的坐标换算（1:8）；
 * - `ambient_light` 决定无光照处的底色亮度；
 * - `fixed_time` 让末地永远停在黄昏；
 * - `min_y` / `height` / `logical_height` 决定建筑高度与传送门规则；
 * - `has_ceiling` 决定能否使用床与地图；
 * - `monster_spawn_light_level` / `monster_spawn_block_light_limit` 决定刷怪光照阈值。
 *
 * 抄错任何一项都不会崩溃，只会让某个维度的手感整体偏差——这是最难靠人工发现的一类
 * 数据错误，也是把这类表逐项比对的最大价值。
 *
 * ## 单元格式
 *
 * `<dimension>|<field>`，字段名与原版 `DimensionType` 的 record 组件一致。
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/dimension/DimensionType.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 三个原版维度（与 `data/minecraft/dimension_type/*.json` 一一对应）。
constexpr const char* DIMENSIONS[] = {"minecraft:overworld", "minecraft:the_nether", "minecraft:the_end"};

/// 参与比对的字段名，顺序与原版 record 组件一致。
constexpr const char* FIELDS[] = {"has_fixed_time",
    "has_skylight",
    "has_ceiling",
    "coordinate_scale",
    "min_y",
    "height",
    "logical_height",
    "infiniburn",
    "ambient_light",
    "monster_spawn_light_level_min",
    "monster_spawn_light_level_max",
    "monster_spawn_block_light_limit",
    "skybox",
    "cardinal_light_type"};

DimensionId dimensionIdOf(const std::string& name)
{
    if (name == "minecraft:the_nether") {
        return -1;
    }
    if (name == "minecraft:the_end") {
        return 1;
    }
    return 0;
}

std::string floatBits(f32 value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
}

void writeList(FILE* out)
{
    for (const char* dimension : DIMENSIONS) {
        for (const char* field : FIELDS) {
            std::fprintf(out, "%s|%s\n", dimension, field);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t separator = unit.find('|');
    if (separator == std::string::npos) {
        throw std::runtime_error("dimension_type_params: 单元应为 'dimension|field'：" + unit);
    }
    const std::string dimensionName = unit.substr(0, separator);
    const std::string field = unit.substr(separator + 1);

    const DimensionType type = DimensionType::fromId(dimensionIdOf(dimensionName));

    json result{{"unit", unit}};
    if (field == "has_fixed_time") {
        result["value"] = type.hasFixedTime();
    } else if (field == "has_skylight") {
        result["value"] = type.hasSkyLight();
    } else if (field == "has_ceiling") {
        result["value"] = type.hasCeiling();
    } else if (field == "coordinate_scale") {
        result["value"] = floatBits(type.coordinateScale());
    } else if (field == "min_y") {
        result["value"] = type.minHeight();
    } else if (field == "height") {
        result["value"] = type.maxHeight() - type.minHeight();
    } else if (field == "logical_height") {
        result["value"] = type.logicalHeight();
    } else if (field == "infiniburn") {
        result["value"] = type.infiniburn();
    } else if (field == "ambient_light") {
        result["value"] = floatBits(type.ambientLight());
    } else if (field == "monster_spawn_light_level_min") {
        result["value"] = type.monsterSpawnLightMin();
    } else if (field == "monster_spawn_light_level_max") {
        result["value"] = type.monsterSpawnLightMax();
    } else if (field == "monster_spawn_block_light_limit") {
        result["value"] = type.monsterSpawnBlockLightLimit();
    } else {
        // skybox / cardinal_light_type 两项 Cubium 尚未建模（它们只影响客户端渲染，
        // 而客户端已停止维护），统一返回 missing，由编排层登记为已知缺口。
        result["missing"] = true;
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("dimension_type_params", &writeList, &runLine)

} // namespace mc::diff
