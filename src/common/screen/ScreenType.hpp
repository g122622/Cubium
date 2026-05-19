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

/**
 * @brief 屏幕类型枚举
 *
 * 定义所有可打开的屏幕/界面类型
 */
enum class ScreenType : u8 {
    Unknown = 0,

    // 玩家背包
    Inventory,         ///< 玩家背包（含2x2合成）
    CreativeInventory, ///< 创造模式背包

    // 容器
    Chest,       ///< 箱子
    DoubleChest, ///< 大箱子
    ShulkerBox,  ///< 潜影盒
    Barrel,      ///< 木桶

    // 工作台
    CraftingTable,    ///< 工作台 (3x3合成)
    Furnace,          ///< 熔炉
    BlastFurnace,     ///< 高炉
    Smoker,           ///< 烟熏炉
    Anvil,            ///< 铁砧
    Grindstone,       ///< 磨石
    Stonecutter,      ///< 切石机
    SmithingTable,    ///< 锻造台
    Loom,             ///< 织布机
    CartographyTable, ///< 制图台
    BrewingStand,     ///< 酿造台
    EnchantingScreen, ///< 附魔台

    // 红石
    Dispenser, ///< 发射器
    Dropper,   ///< 投掷器
    Hopper,    ///< 漏斗
    Beacon,    ///< 信标

    // 其他
    Sign,           ///< 告示牌
    CommandBlock,   ///< 命令方块
    StructureBlock, ///< 结构方块
    JigsawBlock,    ///< 拼图方块
    Bed,            ///< 床（交互）

    Count ///< 类型数量
};

/**
 * @brief 获取屏幕类型的资源位置ID
 * @param type 屏幕类型
 * @return 资源位置（如 "minecraft:crafting_table"）
 */
std::string screenTypeToId(ScreenType type);

/**
 * @brief 从资源位置ID解析屏幕类型
 * @param id 资源位置ID
 * @return 屏幕类型，如果未知返回 Unknown
 */
ScreenType screenTypeFromId(const std::string& id);

} // namespace mc
