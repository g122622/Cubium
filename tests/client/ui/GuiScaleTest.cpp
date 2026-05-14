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