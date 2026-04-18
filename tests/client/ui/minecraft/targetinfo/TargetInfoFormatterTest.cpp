#include "client/ui/minecraft/targetinfo/TargetInfo.hpp"

#include <gtest/gtest.h>

namespace mc::client::ui::minecraft::targetinfo {
namespace {

TEST(TargetInfoFormatterTest, HumanizeIdentifierSplitsWords) {
    EXPECT_EQ("Stone", humanizeIdentifier("stone"));
    EXPECT_EQ("Experience Orb", humanizeIdentifier("experience_orb"));
    EXPECT_EQ("Oak Log", humanizeIdentifier("oakLog"));
    EXPECT_EQ("XP Bar", humanizeIdentifier("XPBar"));
}

TEST(TargetInfoFormatterTest, HumanizeResourceLocationUsesPath) {
    EXPECT_EQ("Ancient Debris", humanizeResourceLocation(ResourceLocation("minecraft:ancient_debris")));
    EXPECT_EQ("Block", humanizeResourceLocation(ResourceLocation("example_mod:block")));
}

TEST(TargetInfoFormatterTest, FormatHelpersProduceReadableStrings) {
    EXPECT_EQ("12.35 m", formatDistance(12.345f));
    EXPECT_EQ("1, 64, -5", formatBlockPos(BlockPos(1, 64, -5)));
    EXPECT_EQ("North", formatDirection(Direction::North));
}

} // namespace
} // namespace mc::client::ui::minecraft::targetinfo