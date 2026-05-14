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

#include "client/ui/minecraft/targetinfo/TargetInfo.hpp"

#include <gtest/gtest.h>

namespace mc::client::ui::minecraft::targetinfo {
namespace {

TEST(TargetInfoFormatterTest, HumanizeIdentifierSplitsWords)
{
    EXPECT_EQ("Stone", humanizeIdentifier("stone"));
    EXPECT_EQ("Experience Orb", humanizeIdentifier("experience_orb"));
    EXPECT_EQ("Oak Log", humanizeIdentifier("oakLog"));
    EXPECT_EQ("XP Bar", humanizeIdentifier("XPBar"));
}

TEST(TargetInfoFormatterTest, HumanizeResourceLocationUsesPath)
{
    EXPECT_EQ("Ancient Debris", humanizeResourceLocation(ResourceLocation("minecraft:ancient_debris")));
    EXPECT_EQ("Block", humanizeResourceLocation(ResourceLocation("example_mod:block")));
}

TEST(TargetInfoFormatterTest, FormatHelpersProduceReadableStrings)
{
    EXPECT_EQ("12.35 m", formatDistance(12.345f));
    EXPECT_EQ("1, 64, -5", formatBlockPos(BlockPos(1, 64, -5)));
    EXPECT_EQ("North", formatDirection(Direction::North));
}

} // namespace
} // namespace mc::client::ui::minecraft::targetinfo