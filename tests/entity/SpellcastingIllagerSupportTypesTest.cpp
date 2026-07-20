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

#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/entities/monster/illager/EvokerEntity.hpp"
#include "common/entity/entities/monster/illager/IllusionerEntity.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include "common/entity/interfaces/IRangedAttackMob.hpp"

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
    EvokerEntity evoker(EntityInstanceId(1));

    evoker.setSpellType(SpellcastingIllagerEntity::SpellType::Fangs);
    evoker.setSpellTicks(40);
    EXPECT_TRUE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::Fangs);
    EXPECT_EQ(evoker.spellTicks(), 40);

    evoker.tick();
    EXPECT_EQ(evoker.spellTicks(), 39);
}

TEST(SpellcastingIllagerSupportTypesTest, IllusionerSpellcastingAndAttributesAreInitialized)
{
    IllusionerEntity illusioner(EntityInstanceId(2));

    // 验证施法状态
    EXPECT_FALSE(illusioner.isSpellcasting());

    // 设置施法状态
    illusioner.setSpellType(SpellcastingIllagerEntity::SpellType::Blindness);
    illusioner.setSpellTicks(20);
    EXPECT_TRUE(illusioner.isSpellcasting());
    EXPECT_EQ(illusioner.spellType(), SpellcastingIllagerEntity::SpellType::Blindness);
    EXPECT_EQ(illusioner.spellTicks(), 20);

    // 验证属性
    EXPECT_EQ(illusioner.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 0.0), 18.0);
    EXPECT_EQ(illusioner.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0), 32.0);
    EXPECT_EQ(illusioner.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0), 0.5);
}

TEST(SpellcastingIllagerSupportTypesTest, IllusionerImplementsIRangedAttackMob)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // 验证幻术师实现了 IRangedAttackMob 接口
    auto* rangedAttacker = dynamic_cast<entity::IRangedAttackMob*>(&illusioner);
    EXPECT_NE(rangedAttacker, nullptr);

    // 验证攻击间隔
    EXPECT_EQ(rangedAttacker->getAttackInterval(), 20);

    // 默认可以进行远程攻击
    EXPECT_TRUE(rangedAttacker->canRangedAttack());

    // 施法时不能远程攻击
    illusioner.setSpellType(SpellcastingIllagerEntity::SpellType::Blindness);
    illusioner.setSpellTicks(20);
    EXPECT_FALSE(rangedAttacker->canRangedAttack());
}

TEST(SpellcastingIllagerSupportTypesTest, IllusionerEyeHeight)
{
    IllusionerEntity illusioner(EntityInstanceId(1));
    EXPECT_FLOAT_EQ(illusioner.eyeHeight(), 1.62f);
}

} // namespace
} // namespace mc
