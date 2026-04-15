#include <gtest/gtest.h>

#include "common/entity/entities/monster/illager/EvokerEntity.hpp"
#include "common/entity/entities/monster/illager/IllusionerEntity.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"

#include <type_traits>

namespace mc {
namespace {

TEST(SpellcastingIllagerSupportTypesTest, IllagerSpellcastersInheritSharedBase)
{
    EXPECT_TRUE((std::is_base_of_v<SpellcastingIllagerEntity, EvokerEntity>));
    EXPECT_TRUE((std::is_base_of_v<SpellcastingIllagerEntity, IllusionerEntity>));
}

TEST(SpellcastingIllagerSupportTypesTest, EvokerCastingStateUsesSharedSpellTicks)
{
    EvokerEntity evoker(LegacyEntityType::Evoker, 1);

    evoker.startCasting(2);
    EXPECT_TRUE(evoker.isCasting());
    EXPECT_EQ(evoker.getSpellType(), 2);
    EXPECT_EQ(evoker.spellTicks(), 40);

    evoker.tick();
    EXPECT_EQ(evoker.spellTicks(), 39);
}

TEST(SpellcastingIllagerSupportTypesTest, IllusionerSpellcastingAndAttributesAreInitialized)
{
    IllusionerEntity illusioner(LegacyEntityType::Illusioner, 2);

    illusioner.castBlindnessSpell();
    EXPECT_TRUE(illusioner.isCasting());
    EXPECT_EQ(illusioner.spellType(), SpellcastingIllagerEntity::SpellType::Blindness);
    EXPECT_EQ(
        illusioner.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 0.0),
        18.0);
}

} // namespace
} // namespace mc
