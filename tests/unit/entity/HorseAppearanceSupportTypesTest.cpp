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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/passive/horse/CoatColors.hpp"
#include "common/entity/entities/passive/horse/CoatTypes.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"

namespace mc {
namespace {

TEST(CoatColorsTest, NormalizesIdsLikeVanilla)
{
    EXPECT_EQ(getCoatColorById(0), CoatColors::White);
    EXPECT_EQ(getCoatColorById(6), CoatColors::DarkBrown);
    EXPECT_EQ(getCoatColorById(7), CoatColors::White);
    EXPECT_EQ(getCoatColorById(-1), CoatColors::DarkBrown);

    EXPECT_EQ(getCoatColorId(CoatColors::Gray), 5);
    EXPECT_STREQ(getCoatColorName(CoatColors::Chestnut), "chestnut");
}

TEST(CoatTypesTest, NormalizesIdsLikeVanilla)
{
    EXPECT_EQ(getCoatTypeById(0), CoatTypes::None);
    EXPECT_EQ(getCoatTypeById(4), CoatTypes::BlackDots);
    EXPECT_EQ(getCoatTypeById(5), CoatTypes::None);
    EXPECT_EQ(getCoatTypeById(-1), CoatTypes::BlackDots);

    EXPECT_EQ(getCoatTypeId(CoatTypes::WhiteDots), 3);
    EXPECT_STREQ(getCoatTypeName(CoatTypes::WhiteField), "whitefield");
}

TEST(HorseAppearanceSupportTypesTest, HorseStoresPackedVariantWithSupportTypes)
{
    HorseEntity horse(EntityInstanceId(1), mc::test::testEcsRegistry());

    horse.setColor(CoatColors::Black);
    horse.setMarking(CoatTypes::WhiteDots);

    EXPECT_EQ(horse.getVariant(), 0x0304);

    horse.setVariant(0x0206);
    EXPECT_EQ(horse.getColor(), CoatColors::DarkBrown);
    EXPECT_EQ(horse.getMarking(), CoatTypes::WhiteField);
}

} // namespace
} // namespace mc
