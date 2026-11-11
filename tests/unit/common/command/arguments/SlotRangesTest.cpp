#include <gtest/gtest.h>

#include "common/command/arguments/SlotRanges.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

using mc::i32;
using mc::command::SlotRanges;

namespace {

bool containsId(const std::vector<i32>& ids, i32 id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

} // namespace

// container 族：0..53，且 container.* 覆盖全部 54 个槽位。
TEST(SlotRangesTest, ContainerRangeCoversFiftyFourSlots)
{
    EXPECT_EQ(SlotRanges::nameToIds("container.0"), std::vector<i32>{0});
    EXPECT_EQ(SlotRanges::nameToIds("container.53"), std::vector<i32>{53});

    const auto all = SlotRanges::nameToIds("container.*");
    ASSERT_EQ(all.size(), 54u);
    EXPECT_EQ(all.front(), 0);
    EXPECT_EQ(all.back(), 53);
}

// 编号越界必须解析失败，而不是被截断成某个有效槽位。
TEST(SlotRangesTest, ContainerRangeRejectsOutOfRangeIndex)
{
    EXPECT_TRUE(SlotRanges::nameToIds("container.54").empty());
    EXPECT_FALSE(SlotRanges::isKnownName("container.54"));
}

// hotbar 是 0..8，与 container 的前 9 个槽位编号重叠——这是 vanilla 的既有设计，不是缺陷。
TEST(SlotRangesTest, HotbarAndContainerOverlapByDesign)
{
    EXPECT_EQ(SlotRanges::nameToIds("hotbar.0"), std::vector<i32>{0});
    EXPECT_EQ(SlotRanges::nameToIds("hotbar.*").size(), 9u);
    EXPECT_EQ(SlotRanges::nameToIds("hotbar.8"), std::vector<i32>{8});
    // 同一个索引可以由不同区间名给出
    EXPECT_EQ(SlotRanges::nameToIds("container.8"), SlotRanges::nameToIds("hotbar.8"));
}

// inventory 是 9..35（不含快捷栏）。
TEST(SlotRangesTest, InventoryRangeStartsAtNine)
{
    EXPECT_EQ(SlotRanges::nameToIds("inventory.0"), std::vector<i32>{9});
    const auto all = SlotRanges::nameToIds("inventory.*");
    ASSERT_EQ(all.size(), 27u);
    EXPECT_EQ(all.front(), 9);
    EXPECT_EQ(all.back(), 35);
}

// 装备槽位采用**本项目**编号（与 vanilla 有偏移），这里钉住取值，防止将来被"对齐 vanilla"改坏。
TEST(SlotRangesTest, EquipmentSlotsUseProjectNumbering)
{
    EXPECT_EQ(SlotRanges::nameToIds("weapon"), std::vector<i32>{98});
    EXPECT_EQ(SlotRanges::nameToIds("weapon.mainhand"), std::vector<i32>{98});
    EXPECT_EQ(SlotRanges::nameToIds("weapon.offhand"), std::vector<i32>{99});

    EXPECT_EQ(SlotRanges::nameToIds("armor.feet"), std::vector<i32>{100});
    EXPECT_EQ(SlotRanges::nameToIds("armor.legs"), std::vector<i32>{101});
    EXPECT_EQ(SlotRanges::nameToIds("armor.chest"), std::vector<i32>{102});
    EXPECT_EQ(SlotRanges::nameToIds("armor.head"), std::vector<i32>{103});
    EXPECT_EQ(SlotRanges::nameToIds("armor.body"), std::vector<i32>{105});
    EXPECT_EQ(SlotRanges::nameToIds("saddle"), std::vector<i32>{106});
}

// weapon.* / armor.* 是多槽组，成员与单槽名一致。
TEST(SlotRangesTest, GroupRangesListTheirMembers)
{
    const auto weapons = SlotRanges::nameToIds("weapon.*");
    ASSERT_EQ(weapons.size(), 2u);
    EXPECT_TRUE(containsId(weapons, 98));
    EXPECT_TRUE(containsId(weapons, 99));

    const auto armor = SlotRanges::nameToIds("armor.*");
    ASSERT_EQ(armor.size(), 5u);
    for (const i32 expected : {100, 101, 102, 103, 105}) {
        EXPECT_TRUE(containsId(armor, expected)) << "armor.* must contain " << expected;
    }
}

// 各族基准：enderchest 200、villager 300、horse 500、player.crafting 500。
TEST(SlotRangesTest, FamilyBasesMatchVanillaSizing)
{
    EXPECT_EQ(SlotRanges::nameToIds("enderchest.0"), std::vector<i32>{200});
    EXPECT_EQ(SlotRanges::nameToIds("enderchest.*").size(), 27u);

    EXPECT_EQ(SlotRanges::nameToIds("villager.0"), std::vector<i32>{300});
    EXPECT_EQ(SlotRanges::nameToIds("villager.*").size(), 8u);

    EXPECT_EQ(SlotRanges::nameToIds("horse.0"), std::vector<i32>{500});
    EXPECT_EQ(SlotRanges::nameToIds("horse.*").size(), 15u);

    EXPECT_EQ(SlotRanges::nameToIds("player.crafting.0"), std::vector<i32>{500});
    EXPECT_EQ(SlotRanges::nameToIds("player.crafting.*").size(), 4u);
}

// 499 同时被 horse.chest 与 player.cursor 使用（vanilla 也如此）；按目标类型区分由调用方负责。
TEST(SlotRangesTest, HorseChestAndPlayerCursorShareIndex)
{
    EXPECT_EQ(SlotRanges::nameToIds("horse.chest"), std::vector<i32>{499});
    EXPECT_EQ(SlotRanges::nameToIds("player.cursor"), std::vector<i32>{499});
}

// 未知名字返回空，且 isKnownName 为 false —— 调用方据此报"未知槽位"。
TEST(SlotRangesTest, UnknownNamesAreRejected)
{
    for (const char* name : {"", "container", "container.", "Container.0", "armor", "weapon.both", "hotbar.9"}) {
        EXPECT_TRUE(SlotRanges::nameToIds(name).empty()) << "unexpected match for '" << name << "'";
        EXPECT_FALSE(SlotRanges::isKnownName(name)) << "unexpected known name '" << name << "'";
    }
}

// allNames 必须与 nameToIds 一致，且无重复（否则命令补全会出现重复项）。
TEST(SlotRangesTest, AllNamesAreConsistentAndUnique)
{
    const auto& names = SlotRanges::allNames();
    EXPECT_FALSE(names.empty());

    std::set<std::string> unique(names.begin(), names.end());
    EXPECT_EQ(unique.size(), names.size()) << "allNames must not contain duplicates";

    for (const auto& name : names) {
        EXPECT_FALSE(SlotRanges::nameToIds(name).empty()) << "name '" << name << "' resolves to nothing";
    }

    // 抽查几个必须存在的名字
    for (const char* expected : {"contents", "container.*", "weapon.mainhand", "armor.head", "player.crafting.3"}) {
        EXPECT_TRUE(unique.count(expected) == 1) << "missing expected name '" << expected << "'";
    }
}
