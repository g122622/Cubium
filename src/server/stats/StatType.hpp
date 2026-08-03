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

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace mc {
namespace server {
namespace stats {

/**
 * @brief 统计类型分类
 *
 * Minecraft 1.16.5 的统计系统分为以下几大类：
 * - mined: 挖掘方块次数
 * - crafted: 合成物品次数
 * - used: 使用物品次数
 * - broken: 物品损坏次数
 * - picked_up: 拾取物品次数
 * - dropped: 丢弃物品次数
 * - killed: 击杀实体次数
 * - killed_by: 被实体击杀次数
 * - custom: 自定义统计（如游戏时间、距离等）
 */
enum class StatType : u8 {
    Mined,    ///< minecraft.mined:{block_id} - 挖掘方块
    Crafted,  ///< minecraft.crafted:{item_id} - 合成物品
    Used,     ///< minecraft.used:{item_id} - 使用物品
    Broken,   ///< minecraft.broken:{item_id} - 物品损坏
    PickedUp, ///< minecraft.picked_up:{item_id} - 拾取物品
    Dropped,  ///< minecraft.dropped:{item_id} - 丢弃物品
    Killed,   ///< minecraft.killed:{entity_id} - 击杀实体
    KilledBy, ///< minecraft.killed_by:{entity_id} - 被实体击杀
    Custom    ///< minecraft.custom:{stat_id} - 自定义统计
};

/**
 * @brief 获取统计类型的名称前缀
 *
 * @param type 统计类型
 * @return 名称前缀（如 "mined", "crafted" 等）
 */
[[nodiscard]] inline std::string_view getStatTypePrefix(StatType type) noexcept
{
    switch (type) {
        case StatType::Mined:
            return "mined";
        case StatType::Crafted:
            return "crafted";
        case StatType::Used:
            return "used";
        case StatType::Broken:
            return "broken";
        case StatType::PickedUp:
            return "picked_up";
        case StatType::Dropped:
            return "dropped";
        case StatType::Killed:
            return "killed";
        case StatType::KilledBy:
            return "killed_by";
        case StatType::Custom:
            return "custom";
        default:
            return "unknown";
    }
}

/**
 * @brief 从名称前缀解析统计类型
 *
 * @param prefix 名称前缀
 * @return 统计类型，如果无法识别则返回 nullopt
 */
[[nodiscard]] std::optional<StatType> parseStatType(std::string_view prefix) noexcept;

/**
 * @brief 构建统计的资源位置
 *
 * 格式：minecraft.{type}:{id}
 * 例如：minecraft.mined:minecraft.stone
 *       minecraft.custom:minecraft.play_one_minute
 *
 * @param type 统计类型
 * @param id 资源ID（方块、物品、实体或自定义统计ID）
 * @return 完整的资源位置
 */
[[nodiscard]] ResourceLocation buildStatLocation(StatType type, const ResourceLocation& id);

} // namespace stats
} // namespace server
} // namespace mc
