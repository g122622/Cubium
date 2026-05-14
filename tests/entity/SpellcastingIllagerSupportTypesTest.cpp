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
    EXPECT_EQ(illusioner.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 0.0), 18.0);
}

} // namespace
} // namespace mc
