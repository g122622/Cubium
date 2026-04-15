#include <gtest/gtest.h>

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
    HorseEntity horse(LegacyEntityType::Horse, 1);

    horse.setColor(CoatColors::Black);
    horse.setMarking(CoatTypes::WhiteDots);

    EXPECT_EQ(horse.getVariant(), 0x0304);

    horse.setVariant(0x0206);
    EXPECT_EQ(horse.getColor(), CoatColors::DarkBrown);
    EXPECT_EQ(horse.getMarking(), CoatTypes::WhiteField);
}

} // namespace
} // namespace mc
