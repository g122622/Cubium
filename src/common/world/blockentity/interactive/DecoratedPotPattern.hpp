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

#include "core/Types.hpp"
#include <string>

namespace mc {
namespace blockentity {

/**
 * @brief 饰纹陶罐图案枚举
 *
 * 定义饰纹陶罐每一面可能显示的图案类型。
 * MC 1.20+ 共有 23 种陶片图案 + 1 种空白（砖块）图案。
 * 每种图案对应一个陶片物品，砖块则作为空白面的替代品。
 *
 * 图案 ID 用于：
 * - 陶片物品到图案纹理的映射
 * - 饰纹陶罐方块实体的 NBT 序列化
 * - 客户端渲染纹理查找
 */
enum class DecoratedPotPattern : u8 {
    // 空白图案（砖块面）
    Blank = 0, // blank - 默认砖块面（使用 decorated_pot_side 纹理）

    // 1.20 考古学陶片图案（20种）
    Angler = 1,      // angler - 钓鱼者
    Archer = 2,      // archer - 射手
    ArmsUp = 3,      // arms_up - 举手
    Blade = 4,       // blade - 刀刃
    Brewer = 5,      // brewer - 酿造者
    Burn = 6,        // burn - 燃烧
    Danger = 7,      // danger - 危险
    Explorer = 8,    // explorer - 探险者
    Friend = 9,      // friend - 朋友
    Heart = 10,      // heart - 心
    Heartbreak = 11, // heartbreak - 碎心
    Howl = 12,       // howl - 嚎叫
    Miner = 13,      // miner - 矿工
    Mourner = 14,    // mourner - 哀悼者
    Plenty = 15,     // plenty - 丰饶
    Prize = 16,      // prize - 奖赏
    Sheaf = 17,      // sheaf - 麦捆
    Shelter = 18,    // shelter - 庇护所
    Skull = 19,      // skull - 骷髅
    Snort = 20,      // snort - 喷鼻

    // 1.21 试炼密室陶片图案（3种）
    Flow = 21,   // flow - 涡流
    Guster = 22, // guster - 旋风
    Scrape = 23, // scrape - 刮削

    Count = 24 // 图案总数
};

/**
 * @brief 饰纹陶罐图案工具类
 *
 * 提供 DecoratedPotPattern 的辅助方法，包括：
 * - 根据图案名获取枚举值
 * - 获取图案的资源路径名（用于纹理查找）
 * - 获取图案的翻译键
 */
class DecoratedPotPatterns {
public:
    /**
     * @brief 根据图案名获取图案类型
     *
     * @param name 图案名（如 "angler", "flow"）
     * @return 图案类型，如果未找到返回 Blank
     */
    [[nodiscard]] static DecoratedPotPattern byName(const std::string& name);

    /**
     * @brief 获取图案的资源路径名
     *
     * 用于客户端纹理查找。返回格式为 "{name}_pottery_pattern"，
     * 例如 "angler_pottery_pattern"、"flow_pottery_pattern"。
     * Blank 图案返回 "decorated_pot_side"。
     *
     * @param pattern 图案类型
     * @return 纹理资源路径名
     */
    [[nodiscard]] static std::string getAssetId(DecoratedPotPattern pattern);

    /**
     * @brief 获取图案的翻译键
     *
     * 返回格式为 "item.minecraft.{name}_pottery_sherd"，
     * 例如 "item.minecraft.angler_pottery_sherd"。
     * Blank 图案返回 "block.minecraft.decorated_pot"。
     *
     * @param pattern 图案类型
     * @return 翻译键
     */
    [[nodiscard]] static std::string getTranslationKey(DecoratedPotPattern pattern);

    /**
     * @brief 获取图案的简单名称
     *
     * 返回枚举值对应的简单名称，如 "angler"、"flow"、"blank"。
     *
     * @param pattern 图案类型
     * @return 简单名称
     */
    [[nodiscard]] static std::string getName(DecoratedPotPattern pattern);

    /**
     * @brief 检查图案是否为空白（砖块面）
     *
     * @param pattern 图案类型
     * @return 如果是空白图案返回 true
     */
    [[nodiscard]] static bool isBlank(DecoratedPotPattern pattern) { return pattern == DecoratedPotPattern::Blank; }
};

} // namespace blockentity
} // namespace mc
