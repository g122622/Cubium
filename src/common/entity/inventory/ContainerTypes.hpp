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
 * @brief 玩家背包容器ID
 *
 * 玩家背包在网络同步中固定使用 containerId=0（不复用自增计数器），
 * 用于创造/合成菜单构造与容器点击路由判别。
 */
namespace inventory {
constexpr ContainerId PLAYER_CONTAINER_ID = 0;
} // namespace inventory

/**
 * @brief 容器ID的网络传输类型
 *
 * 网络包中使用 u8 传输容器ID。
 */
using ContainerIdU8 = u8;

/**
 * @brief 容器类型枚举
 *
 * 定义游戏中所有容器窗口类型，用于网络同步和客户端渲染。
 *
 * **数值即 vanilla 菜单注册表的项序**，会被 `toNetworkType` 原样作为 `open_screen` 的
 * menuType 上线。客户端拿到该 id 后据此建窗口、定槽位布局——填错会让它建成另一个窗口，
 * 且既不断连也不报错，只是槽位语义整体错位。故：
 *   - 不得调整既有项的数值；
 *   - 新增类型必须插到与其注册表项序一致的位置，不能追加到末尾。
 *
 * 玩家背包不是注册表项（原版由客户端本地构造，服务端从不下发），故取注册表之外的值。
 */
enum class ContainerType : u8 {
    Generic9x1 = 0,    // 单行箱子
    Generic9x2 = 1,    // 双行箱子
    Generic9x3 = 2,    // 三行箱子（普通大箱子）
    Generic9x4 = 3,    // 四行箱子
    Generic9x5 = 4,    // 五行箱子
    Generic9x6 = 5,    // 六行箱子（最大箱子）
    Generic3x3 = 6,    // 发射器/投掷器
    Crafter = 7,       // 自动合成器
    Anvil = 8,         // 铁砧
    Beacon = 9,        // 信标
    BlastFurnace = 10, // 高炉
    BrewingStand = 11, // 酿造台
    Crafting = 12,     // 工作台
    Enchantment = 13,  // 附魔台
    Furnace = 14,      // 熔炉
    Grindstone = 15,   // 砂轮
    Hopper = 16,       // 漏斗
    Lectern = 17,      // 讲台
    Loom = 18,         // 织布机
    Merchant = 19,     // 村民交易
    ShulkerBox = 20,   // 潜影盒
    Smithing = 21,     // 锻造台
    Smoker = 22,       // 烟熏炉
    Cartography = 23,  // 制图台
    Stonecutter = 24,  // 切石机
    // 玩家背包使用特殊值
    Player = 255 // 玩家背包（特殊类型）
};

/**
 * @brief 点击操作类型（网络包中使用）
 *
 * 定义容器交互时的点击操作类型，用于网络同步。
 */
enum class ClickAction : u8 {
    Pickup = 0,     // 拾取/放置（左键或右键）
    QuickMove = 1,  // Shift+点击快速移动
    Swap = 2,       // 数字键交换（1-9交换快捷栏，40交换副手）
    Clone = 3,      // 创造模式中键复制
    Throw = 4,      // Q键丢弃（Ctrl+Q丢弃整组）
    QuickCraft = 5, // 拖拽分发
    PickupAll = 6   // 双击拾取全部相同物品
};

/**
 * @brief 槽位覆写协议点击动作
 *
 * 用于 Item::overrideStackedOnOther 和 Item::overrideOtherStackedOnMe
 * 区分左键（Primary）和右键（Secondary）点击。
 *
 * 对应 MC 1.21.11 net.minecraft.world.inventory.ClickAction。
 * 注意：此枚举与 ClickAction（网络包用）语义不同，不可混用。
 */
enum class SlotClickAction : u8 {
    Primary = 0,  ///< 左键
    Secondary = 1 ///< 右键
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
