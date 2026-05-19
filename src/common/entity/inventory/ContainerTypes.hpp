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

#include "../../core/Types.hpp"

namespace mc {

/**
 * @brief 容器ID类型
 *
 * 用于标识打开的容器窗口，同步客户端和服务端状态。
 * 内部使用 i32，网络传输使用 u8。
 */
using ContainerId = i32;

/**
 * @brief 无效容器ID
 */
constexpr ContainerId INVALID_CONTAINER_ID = -1;

/**
 * @brief 容器ID的网络传输类型
 *
 * 网络包中使用 u8 传输容器ID。
 */
using ContainerIdU8 = u8;

/**
 * @brief 容器类型枚举
 *
 * 对应 MC 1.16.5 Registry.MENU 注册顺序
 * 参考: net.minecraft.inventory.container.ContainerType
 */
enum class ContainerType : u8 {
    Generic9x1 = 0,    // generic_9x1 - 单行箱子
    Generic9x2 = 1,    // generic_9x2 - 双行箱子
    Generic9x3 = 2,    // generic_9x3 - 三行箱子（普通大箱子）
    Generic9x4 = 3,    // generic_9x4 - 四行箱子
    Generic9x5 = 4,    // generic_9x5 - 五行箱子
    Generic9x6 = 5,    // generic_9x6 - 六行箱子（最大箱子）
    Generic3x3 = 6,    // generic_3x3 - 发射器/投掷器
    Anvil = 7,         // anvil - 铁砧
    Beacon = 8,        // beacon - 信标
    BlastFurnace = 9,  // blast_furnace - 高炉
    BrewingStand = 10, // brewing_stand - 酿造台
    Crafting = 11,     // crafting - 工作台
    Enchantment = 12,  // enchantment - 附魔台
    Furnace = 13,      // furnace - 熔炉
    Grindstone = 14,   // grindstone - 砂轮
    Hopper = 15,       // hopper - 漏斗
    Lectern = 16,      // lectern - 讲台
    Loom = 17,         // loom - 织布机
    Merchant = 18,     // merchant - 村民交易
    ShulkerBox = 19,   // shulker_box - 潜影盒
    Smithing = 20,     // smithing - 锻造台
    Smoker = 21,       // smoker - 烟熏炉
    Cartography = 22,  // cartography_table - 制图台
    Stonecutter = 23,  // stonecutter - 切石机
    // 玩家背包没有注册在 Registry.MENU 中，使用特殊值
    Player = 255 // 玩家背包（特殊类型）
};

/**
 * @brief 点击操作类型（网络包中使用）
 *
 * 对应 MC 1.16.5 协议中的 ClickType 枚举
 * 参考: net.minecraft.inventory.container.ClickType
 */
enum class ClickAction : u8 {
    Pickup = 0,     // PICKUP - 拾取/放置（左键或右键）
    QuickMove = 1,  // QUICK_MOVE - Shift+点击快速移动
    Swap = 2,       // SWAP - 数字键交换（1-9交换快捷栏，40交换副手）
    Clone = 3,      // CLONE - 创造模式中键复制
    Throw = 4,      // THROW - Q键丢弃（Ctrl+Q丢弃整组）
    QuickCraft = 5, // QUICK_CRAFT - 拖拽分发
    PickupAll = 6   // PICKUP_ALL - 双击拾取全部相同物品
};

/**
 * @brief 容器动作类型
 *
 * 定义玩家与容器交互的操作类型
 */
enum class ContainerAction : u8 {
    Click,        ///< 点击槽位
    ShiftClick,   ///< Shift+点击（快速移动）
    HotbarSwap,   ///< 数字键交换
    CreativePick, ///< 创造模式选取
    DoubleClick,  ///< 双击（合并相同物品）
    Drag,         ///< 拖动分发
    Throw,        ///< 丢弃物品
};

/**
 * @brief 点击类型
 *
 * 定义容器菜单中的点击操作类型，用于内部处理逻辑
 * 与ClickAction对应，但更细化操作类型
 */
enum class ClickType : u8 {
    Pick,       ///< 拾取（左键空手点击槽位）
    PickAll,    ///< 全部拾取（双击槽位）
    PickSome,   ///< 部分拾取（右键拾取一半）
    Place,      ///< 放置（左键放置全部）
    PlaceSome,  ///< 部分放置（右键放置一个）
    PlaceAll,   ///< 全部放置
    Throw,      ///< 丢弃（Q键丢弃一个）
    ThrowAll,   ///< 全部丢弃（Ctrl+Q丢弃整组）
    QuickMove,  ///< 快速移动（Shift+点击）
    QuickCraft, ///< 快速合成（拖拽分发）
    Clone,      ///< 克隆（创造模式中键）
    Pickup,     ///< 拖拽拾取
    Swap        ///< 数字键交换
};

} // namespace mc
