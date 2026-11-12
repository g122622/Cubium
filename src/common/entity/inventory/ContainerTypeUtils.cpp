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

#include "common/entity/inventory/ContainerTypeUtils.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"

namespace mc {
namespace ContainerTypes {

i32 getSlotCount(ContainerType type)
{
    switch (type) {
        case ContainerType::Generic9x1:
            return 9; // 1行箱子
        case ContainerType::Generic9x2:
            return 18; // 2行箱子
        case ContainerType::Generic9x3:
            return 27; // 3行箱子（普通大箱子）
        case ContainerType::Generic9x4:
            return 36; // 4行箱子
        case ContainerType::Generic9x5:
            return 45; // 5行箱子
        case ContainerType::Generic9x6:
            return 54; // 6行箱子（最大箱子）
        case ContainerType::Generic3x3:
            return 9; // 发射器/投掷器
        case ContainerType::Anvil:
            return 3; // 铁砧（2输入+1输出）
        case ContainerType::Beacon:
            return 1; // 信标
        case ContainerType::BlastFurnace:
            return 3; // 高炉（输入+燃料+输出）
        case ContainerType::BrewingStand:
            return 5; // 酿造台（1燃料+3药水+1材料）
        case ContainerType::Crafting:
            return 10; // 工作台（9格网格+1输出）
        case ContainerType::Enchantment:
            return 2; // 附魔台（1输入+1青金石）
        case ContainerType::Furnace:
            return 3; // 熔炉（输入+燃料+输出）
        case ContainerType::Grindstone:
            return 2; // 砂轮（输入+输出）
        case ContainerType::Hopper:
            return 5; // 漏斗
        case ContainerType::Lectern:
            return 1; // 讲台
        case ContainerType::Loom:
            return 4; // 织布机（3输入+1输出）
        case ContainerType::Merchant:
            return 3; // 村民交易（2输入+1输出）
        case ContainerType::ShulkerBox:
            return 27; // 潜影盒（与3行箱子相同）
        case ContainerType::Smithing:
            return 3; // 锻造台（2输入+1输出）
        case ContainerType::Smoker:
            return 3; // 烟熏炉（输入+燃料+输出）
        case ContainerType::Cartography:
            return 3; // 制图台（2输入+1输出）
        case ContainerType::Stonecutter:
            return 2; // 切石机（1输入+1输出）
        case ContainerType::Crafter:
            return 9; // 自动合成器（3x3合成网格）
        case ContainerType::Player:
            return 46; // 玩家背包（36背包+4护甲+5合成+1结果）
        default:
            return 0;
    }
}

const char* getDefaultTitle(ContainerType type)
{
    // 返回的是翻译键而非英文文本：容器标题以翻译组件下发，由客户端按自己的语言表本地化。
    switch (type) {
        case ContainerType::Generic9x1:
        case ContainerType::Generic9x2:
        case ContainerType::Generic9x3:
        case ContainerType::Generic9x4:
        case ContainerType::Generic9x5:
        case ContainerType::Generic9x6:
            return "container.chest";
        case ContainerType::Generic3x3:
            return "container.dispenser";
        case ContainerType::Anvil:
            return "container.repair";
        case ContainerType::Beacon:
            return "container.beacon";
        case ContainerType::BlastFurnace:
            return "container.blast_furnace";
        case ContainerType::BrewingStand:
            return "container.brewing";
        case ContainerType::Crafting:
            return "container.crafting";
        case ContainerType::Enchantment:
            return "container.enchant";
        case ContainerType::Furnace:
            return "container.furnace";
        case ContainerType::Grindstone:
            return "container.grindstone_title";
        case ContainerType::Hopper:
            return "container.hopper";
        case ContainerType::Lectern:
            return "container.lectern";
        case ContainerType::Loom:
            return "container.loom";
        case ContainerType::Merchant:
            // 原版交易界面标题是村民名（merchant.title 带职业参数），这里用村民默认名兜底。
            return "entity.minecraft.villager";
        case ContainerType::ShulkerBox:
            return "container.shulkerBox";
        case ContainerType::Smithing:
            return "container.upgrade";
        case ContainerType::Smoker:
            return "container.smoker";
        case ContainerType::Cartography:
            return "container.cartography_table";
        case ContainerType::Stonecutter:
            return "container.stonecutter";
        case ContainerType::Crafter:
            return "container.crafter";
        case ContainerType::Player:
            return "container.inventory";
        default:
            return "container.chest";
    }
}

u8 toNetworkType(ContainerType type)
{
    return static_cast<u8>(type);
}

ClickType toClickType(ClickAction action, i32 button)
{
    switch (action) {
        case ClickAction::Pickup:
            // PICKUP: button 0 = 左键拾取/放置, button 1 = 右键拾取一半/放置一个
            return (button == 0) ? ClickType::Pick : ClickType::PickSome;
        case ClickAction::QuickMove:
            return ClickType::QuickMove;
        case ClickAction::Swap:
            return ClickType::Swap;
        case ClickAction::Clone:
            return ClickType::Clone;
        case ClickAction::Throw:
            return (button == 0) ? ClickType::Throw : ClickType::ThrowAll;
        case ClickAction::QuickCraft:
            return ClickType::QuickCraft;
        case ClickAction::PickupAll:
            return ClickType::PickAll;
        default:
            return ClickType::Pick;
    }
}

ClickAction toClickAction(ClickType clickType)
{
    switch (clickType) {
        case ClickType::Pick:
        case ClickType::Place:
        case ClickType::PlaceSome:
        case ClickType::PlaceAll:
        case ClickType::PickSome:
            return ClickAction::Pickup;
        case ClickType::PickAll:
            return ClickAction::PickupAll;
        case ClickType::Throw:
        case ClickType::ThrowAll:
            return ClickAction::Throw;
        case ClickType::QuickMove:
            return ClickAction::QuickMove;
        case ClickType::QuickCraft:
            return ClickAction::QuickCraft;
        case ClickType::Clone:
            return ClickAction::Clone;
        case ClickType::Pickup:
            return ClickAction::Pickup;
        case ClickType::Swap:
            return ClickAction::Swap;
        default:
            return ClickAction::Pickup;
    }
}

} // namespace ContainerTypes
} // namespace mc
