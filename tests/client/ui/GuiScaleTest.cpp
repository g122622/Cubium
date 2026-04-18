#include "client/ui/GuiScale.hpp"

#include <gtest/gtest.h>

namespace mc::client::ui {

TEST(GuiScaleTest, AutoScaleUsesVanillaLimits)
{
    const auto state = calculateGuiScale(0, 1920, 1080);

    EXPECT_EQ(state.scaleFactor, 4);
    EXPECT_EQ(state.width, 480);
    EXPECT_EQ(state.height, 270);
}

TEST(GuiScaleTest, RequestedScaleUsesExactValueWhenSpaceAllows)
{
    const auto state = calculateGuiScale(3, 1920, 1080);

    EXPECT_EQ(state.scaleFactor, 3);
    EXPECT_EQ(state.width, 640);
    EXPECT_EQ(state.height, 360);
}

TEST(GuiScaleTest, RequestedScaleClampsToAvailableSpace)
{
    const auto state = calculateGuiScale(4, 800, 600);

    EXPECT_EQ(state.scaleFactor, 2);
    EXPECT_EQ(state.width, 400);
    EXPECT_EQ(state.height, 300);
}

TEST(GuiScaleTest, SmallWindowKeepsUsableLogicalSize)
{
    const auto state = calculateGuiScale(0, 800, 600);

    EXPECT_EQ(state.scaleFactor, 2);
    EXPECT_EQ(state.width, 400);
    EXPECT_EQ(state.height, 300);
}

} // namespace mc::client::ui