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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, EITHER THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "core/Types.hpp"
#include <string>
#include <string_view>

namespace mc {

/**
 * @brief 维度ID枚举
 *
 * 标识MC中的三个维度，用于地图数据中标记地图所属维度。
 * 值与MC Java版的维度ID对应：0=主世界, -1=下界, 1=末地。
 */
enum class MapDimensionId : i32 { Overworld = 0, Nether = -1, End = 1 };

/**
 * @brief 将 MapDimensionId 转换为 Minecraft 命名空间格式的维度名称字符串
 *
 * 返回 Java 版 1.16+ 使用的标准维度标识符，如 "minecraft:overworld"。
 *
 * @param id 维度ID
 * @return 命名空间格式的维度名称
 */
[[nodiscard]] inline std::string_view dimensionIdToString(MapDimensionId id)
{
    switch (id) {
        case MapDimensionId::Overworld:
            return "minecraft:overworld";
        case MapDimensionId::Nether:
            return "minecraft:the_nether";
        case MapDimensionId::End:
            return "minecraft:the_end";
        default:
            return "minecraft:overworld";
    }
}

/**
 * @brief 将维度名称字符串转换为 MapDimensionId
 *
 * 支持以下格式：
 * - 命名空间格式："minecraft:overworld"、"minecraft:the_nether"、"minecraft:the_end"
 * - 简短格式："overworld"、"the_nether"、"the_end"
 * - 数字格式：0、-1、1
 * 未知维度名称默认返回 Overworld。
 *
 * @param name 维度名称字符串
 * @return 对应的 MapDimensionId
 */
[[nodiscard]] inline MapDimensionId dimensionIdFromString(std::string_view name)
{
    if (name == "minecraft:overworld" || name == "overworld") {
        return MapDimensionId::Overworld;
    }
    if (name == "minecraft:the_nether" || name == "the_nether") {
        return MapDimensionId::Nether;
    }
    if (name == "minecraft:the_end" || name == "the_end") {
        return MapDimensionId::End;
    }
    // 尝试数字格式（旧版存档兼容）
    if (name == "0") {
        return MapDimensionId::Overworld;
    }
    if (name == "-1") {
        return MapDimensionId::Nether;
    }
    if (name == "1") {
        return MapDimensionId::End;
    }
    return MapDimensionId::Overworld;
}

/**
 * @brief 将 DimensionId（i32）转换为 Minecraft 命名空间格式的维度名称字符串
 *
 * @param id 维度ID（0=主世界, -1=下界, 1=末地）
 * @return 命名空间格式的维度名称
 */
[[nodiscard]] inline std::string_view dimensionIdToString(DimensionId id)
{
    return dimensionIdToString(static_cast<MapDimensionId>(id));
}

/**
 * @brief 将维度名称字符串转换为 DimensionId（i32）
 *
 * 支持命名空间格式、简短格式和数字格式的维度名称。
 * 未知维度名称默认返回 0（主世界）。
 *
 * @param name 维度名称字符串
 * @return 对应的 DimensionId
 */
[[nodiscard]] inline DimensionId dimensionNameToId(std::string_view name)
{
    return static_cast<DimensionId>(dimensionIdFromString(name));
}

} // namespace mc
