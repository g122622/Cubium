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

// ContainerType 的 wire 取值测试。
//
// 该枚举的数值会原样作为 `open_screen` 的 menuType 上线，客户端据此决定建哪种窗口、
// 槽位怎么排。数值一旦与菜单注册表的项序不符，原版客户端会建成**另一个**窗口：既不
// 断连也不报错，只是此后所有槽位号的语义都对不上。
//
// 这类错误在本项目的自研客户端上不可见（收发两端共用同一个枚举，怎么错都自洽），
// 只在第三方客户端连入时才暴露。故必须有测试把每一项的值钉死。

#include "entity/inventory/ContainerTypeUtils.hpp"
#include "entity/inventory/ContainerTypes.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <iterator>
#include <string>

using namespace mc;

namespace {

/// 一个容器类型在 wire 上的期望取值。
struct ExpectedWireId {
    ContainerType type;
    u8 wireId;
};

/// 全部会被下发的容器类型及其在菜单注册表中的项序。
///
/// 玩家背包不在此列：原版该窗口由客户端本地构造，服务端从不下发它，故取注册表之外的值。
constexpr ExpectedWireId kExpectedWireIds[] = {
    {ContainerType::Generic9x1, 0},
    {ContainerType::Generic9x2, 1},
    {ContainerType::Generic9x3, 2},
    {ContainerType::Generic9x4, 3},
    {ContainerType::Generic9x5, 4},
    {ContainerType::Generic9x6, 5},
    {ContainerType::Generic3x3, 6},
    // 自动合成器排在发射器之后、铁砧之前——漏掉它会让其后所有类型整体前移一位。
    {ContainerType::Crafter, 7},
    {ContainerType::Anvil, 8},
    {ContainerType::Beacon, 9},
    {ContainerType::BlastFurnace, 10},
    {ContainerType::BrewingStand, 11},
    {ContainerType::Crafting, 12},
    {ContainerType::Enchantment, 13},
    {ContainerType::Furnace, 14},
    {ContainerType::Grindstone, 15},
    {ContainerType::Hopper, 16},
    {ContainerType::Lectern, 17},
    {ContainerType::Loom, 18},
    {ContainerType::Merchant, 19},
    {ContainerType::ShulkerBox, 20},
    {ContainerType::Smithing, 21},
    {ContainerType::Smoker, 22},
    {ContainerType::Cartography, 23},
    {ContainerType::Stonecutter, 24},
};

/// 玩家背包的哨兵值（不在注册表内，仅用于内部判别）。
constexpr u8 kPlayerContainerWireId = 255;

} // namespace

TEST(ContainerTypeUtilsTest, WireIdMatchesMenuRegistryOrder)
{
    for (const auto& [type, expected] : kExpectedWireIds) {
        EXPECT_EQ(ContainerTypes::toNetworkType(type), expected)
            << "容器类型 " << static_cast<int>(expected) << " 的 wire 取值不符";
        // 该值就是枚举本身的数值，故两者必须一致——客户端会把它反解回枚举。
        EXPECT_EQ(static_cast<u8>(type), expected);
    }
}

TEST(ContainerTypeUtilsTest, WireIdsAreContiguousFromZero)
{
    // 注册表项序必须是 0..N-1 连续无空洞：出现空洞说明有类型被漏配，
    // 而它之后的所有类型都会前移一位。
    constexpr size_t count = std::size(kExpectedWireIds);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(kExpectedWireIds[i].wireId, static_cast<u8>(i)) << "第 " << i << " 项的 wire 取值应为 " << i;
    }
}

TEST(ContainerTypeUtilsTest, PlayerContainerIsNotARegistryEntry)
{
    // 玩家背包不是菜单注册表项，其数值不得落在已下发的取值范围内，
    // 否则客户端会把它当成某个真实容器类型建错窗口。
    const u8 playerWireId = ContainerTypes::toNetworkType(ContainerType::Player);
    EXPECT_EQ(playerWireId, kPlayerContainerWireId);
    for (const auto& [type, wireId] : kExpectedWireIds) {
        (void)type;
        EXPECT_NE(playerWireId, wireId);
    }
}

TEST(ContainerTypeUtilsTest, EveryContainerTypeReportsSlotCount)
{
    for (const auto& [type, wireId] : kExpectedWireIds) {
        (void)wireId;
        EXPECT_GT(ContainerTypes::getSlotCount(type), 0) << "容器类型 " << static_cast<int>(type) << " 未给出槽位数";
    }
    // 玩家背包是 5 个合成格 + 1 个结果格 + 4 护甲 + 36 主背包 = 46。
    EXPECT_EQ(ContainerTypes::getSlotCount(ContainerType::Player), 46);
}

TEST(ContainerTypeUtilsTest, EveryContainerTypeHasDefaultTitle)
{
    for (const auto& [type, wireId] : kExpectedWireIds) {
        (void)wireId;
        const char* title = ContainerTypes::getDefaultTitle(type);
        ASSERT_NE(title, nullptr);
        EXPECT_GT(std::string(title).size(), 0u);
    }
}

TEST(ContainerTypeUtilsTest, ClickActionRoundTripsThroughClickType)
{
    // toClickType / toClickAction 是容器点击的业务映射，来回转换须稳定。
    const ClickAction actions[] = {
        ClickAction::Pickup,
        ClickAction::QuickMove,
        ClickAction::Swap,
        ClickAction::Clone,
        ClickAction::Throw,
        ClickAction::QuickCraft,
        ClickAction::PickupAll,
    };
    for (const ClickAction action : actions) {
        const ClickType clickType = ContainerTypes::toClickType(action, 0);
        EXPECT_EQ(ContainerTypes::toClickAction(clickType), action)
            << "点击动作 " << static_cast<int>(action) << " 往返后变了";
    }

    // 右键拾取与左键拾取映射到不同的菜单点击类型。
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Pickup, 0), ClickType::Pick);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Pickup, 1), ClickType::PickSome);
    // Q 与 Ctrl+Q 同理。
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Throw, 0), ClickType::Throw);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Throw, 1), ClickType::ThrowAll);
}
