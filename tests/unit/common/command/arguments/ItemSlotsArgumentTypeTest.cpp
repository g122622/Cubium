#include <gtest/gtest.h>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/SlotRanges.hpp"
#include "common/command/errors/CommandErrors.hpp"

#include <string>

using mc::command::ItemSlotsArgumentType;

// 已知槽位名解析成功，且原样返回（调用方再经 SlotRanges 展开成索引）。
TEST(ItemSlotsArgumentTypeTest, ParsesKnownSlotNames)
{
    for (const char* name : {"container.*", "container.5", "weapon", "weapon.mainhand", "armor.head", "hotbar.3"}) {
        mc::command::StringReader reader(name);
        ItemSlotsArgumentType type;
        EXPECT_EQ(type.parse(reader).value(), std::string(name)) << "failed for '" << name << "'";
    }
}

// 未知槽位名必须在**解析期**报错。原版 SlotsArgument 同样如此；
// 若不校验，`container.99` 这类笔误会一路解析成功、执行期静默变成空槽位集合。
TEST(ItemSlotsArgumentTypeTest, RejectsUnknownSlotNames)
{
    for (const char* name : {"container.99", "not_a_slot", "Container.0", "armor.wings"}) {
        mc::command::StringReader reader(name);
        ItemSlotsArgumentType type;
        EXPECT_TRUE(type.parse(reader).failed()) << "should reject '" << name << "'";
    }
}

// 解析失败后游标必须回到起点，否则上层重试解析会从中间开始。
TEST(ItemSlotsArgumentTypeTest, CursorResetsOnUnknownSlot)
{
    mc::command::StringReader reader("container.99");
    ItemSlotsArgumentType type;
    const int start = reader.getCursor();
    EXPECT_TRUE(type.parse(reader).failed());
    EXPECT_EQ(reader.getCursor(), start);
}
