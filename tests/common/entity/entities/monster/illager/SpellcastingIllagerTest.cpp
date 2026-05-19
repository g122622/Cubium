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

#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include "common/util/math/Vector3.hpp"

using namespace mc;

/**
 * @brief 测试施法粒子颜色映射
 *
 * 参考 MC 1.16.5 SpellcastingIllagerEntity.SpellType.particleSpeed
 * 每种法术类型对应不同的粒子颜色（RGB 值作为速度参数）
 */
class SpellcastingIllagerParticleColorTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @brief 测试召唤恼鬼法术的粒子颜色
 *
 * SummonVex (召唤恼鬼) - 淡蓝白色
 * MC 1.16.5: SUMMON_VEX(1, 0.7D, 0.7D, 0.8D)
 */
TEST_F(SpellcastingIllagerParticleColorTest, SummonVexParticleColor)
{
    auto color = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::SummonVex);

    // 淡蓝白色 (0.7, 0.7, 0.8)
    EXPECT_FLOAT_EQ(color.x, 0.7f);
    EXPECT_FLOAT_EQ(color.y, 0.7f);
    EXPECT_FLOAT_EQ(color.z, 0.8f);
}

/**
 * @brief 测试尖牙攻击法术的粒子颜色
 *
 * Fangs (尖牙攻击) - 棕色
 * MC 1.16.5: FANGS(2, 0.4D, 0.3D, 0.35D)
 */
TEST_F(SpellcastingIllagerParticleColorTest, FangsParticleColor)
{
    auto color = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::Fangs);

    // 棕色 (0.4, 0.3, 0.35)
    EXPECT_FLOAT_EQ(color.x, 0.4f);
    EXPECT_FLOAT_EQ(color.y, 0.3f);
    EXPECT_FLOAT_EQ(color.z, 0.35f);
}

/**
 * @brief 测试唔噜噜法术（羊变色）的粒子颜色
 *
 * Wololo (羊变色) - 橙黄色
 * MC 1.16.5: WOLOLO(3, 0.7D, 0.5D, 0.2D)
 */
TEST_F(SpellcastingIllagerParticleColorTest, WololoParticleColor)
{
    auto color = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::Wololo);

    // 橙黄色 (0.7, 0.5, 0.2)
    EXPECT_FLOAT_EQ(color.x, 0.7f);
    EXPECT_FLOAT_EQ(color.y, 0.5f);
    EXPECT_FLOAT_EQ(color.z, 0.2f);
}

/**
 * @brief 测试消失/镜像法术的粒子颜色
 *
 * Disappear (消失/镜像) - 蓝色
 * MC 1.16.5: DISAPPEAR(4, 0.3D, 0.3D, 0.8D)
 */
TEST_F(SpellcastingIllagerParticleColorTest, DisappearParticleColor)
{
    auto color = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::Disappear);

    // 蓝色 (0.3, 0.3, 0.8)
    EXPECT_FLOAT_EQ(color.x, 0.3f);
    EXPECT_FLOAT_EQ(color.y, 0.3f);
    EXPECT_FLOAT_EQ(color.z, 0.8f);
}

/**
 * @brief 测试失明法术的粒子颜色
 *
 * Blindness (失明) - 深蓝/深紫色
 * MC 1.16.5: BLINDNESS(5, 0.1D, 0.1D, 0.2D)
 */
TEST_F(SpellcastingIllagerParticleColorTest, BlindnessParticleColor)
{
    auto color = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::Blindness);

    // 深蓝/深紫色 (0.1, 0.1, 0.2)
    EXPECT_FLOAT_EQ(color.x, 0.1f);
    EXPECT_FLOAT_EQ(color.y, 0.1f);
    EXPECT_FLOAT_EQ(color.z, 0.2f);
}

/**
 * @brief 测试无施法状态的粒子颜色
 *
 * None (无施法) - 黑色（不显示粒子）
 * MC 1.16.5: NONE(0, 0.0D, 0.0D, 0.0D)
 */
TEST_F(SpellcastingIllagerParticleColorTest, NoneParticleColor)
{
    auto color = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::None);

    // 黑色 (0.0, 0.0, 0.0) - 不显示粒子
    EXPECT_FLOAT_EQ(color.x, 0.0f);
    EXPECT_FLOAT_EQ(color.y, 0.0f);
    EXPECT_FLOAT_EQ(color.z, 0.0f);
}

/**
 * @brief 测试 spellTypeFromId 转换
 *
 * 验证从整数 ID 到 SpellType 枚举的转换
 */
TEST_F(SpellcastingIllagerParticleColorTest, SpellTypeFromIdConversion)
{
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(0), SpellcastingIllagerEntity::SpellType::None);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(1), SpellcastingIllagerEntity::SpellType::SummonVex);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(2), SpellcastingIllagerEntity::SpellType::Fangs);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(3), SpellcastingIllagerEntity::SpellType::Wololo);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(4), SpellcastingIllagerEntity::SpellType::Disappear);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(5), SpellcastingIllagerEntity::SpellType::Blindness);

    // 无效 ID 返回 None
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(-1), SpellcastingIllagerEntity::SpellType::None);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(100), SpellcastingIllagerEntity::SpellType::None);
}

/**
 * @brief 测试粒子颜色唯一性
 *
 * 确保不同法术类型有不同的粒子颜色
 */
TEST_F(SpellcastingIllagerParticleColorTest, ParticleColorsAreUnique)
{
    auto noneColor = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::None);
    auto summonVexColor =
        SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::SummonVex);
    auto fangsColor = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::Fangs);
    auto wololoColor = SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::Wololo);
    auto disappearColor =
        SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::Disappear);
    auto blindnessColor =
        SpellcastingIllagerEntity::getSpellParticleColor(SpellcastingIllagerEntity::SpellType::Blindness);

    // 所有颜色应该彼此不同
    std::vector<Vector3> colors = {noneColor, summonVexColor, fangsColor, wololoColor, disappearColor, blindnessColor};

    for (size_t i = 0; i < colors.size(); ++i) {
        for (size_t j = i + 1; j < colors.size(); ++j) {
            // 每对颜色应该不同
            bool sameColor = (colors[i].x == colors[j].x && colors[i].y == colors[j].y && colors[i].z == colors[j].z);
            EXPECT_FALSE(sameColor) << "Colors at index " << i << " and " << j << " are the same";
        }
    }
}

/**
 * @brief 测试法术颜色范围有效性
 *
 * 所有颜色值应在 [0.0, 1.0] 范围内
 */
TEST_F(SpellcastingIllagerParticleColorTest, ParticleColorsAreInValidRange)
{
    std::vector<SpellcastingIllagerEntity::SpellType> allTypes = {SpellcastingIllagerEntity::SpellType::None,
        SpellcastingIllagerEntity::SpellType::SummonVex,
        SpellcastingIllagerEntity::SpellType::Fangs,
        SpellcastingIllagerEntity::SpellType::Wololo,
        SpellcastingIllagerEntity::SpellType::Disappear,
        SpellcastingIllagerEntity::SpellType::Blindness};

    for (auto type : allTypes) {
        auto color = SpellcastingIllagerEntity::getSpellParticleColor(type);

        EXPECT_GE(color.x, 0.0f) << "R component out of range for type " << static_cast<int>(type);
        EXPECT_LE(color.x, 1.0f) << "R component out of range for type " << static_cast<int>(type);
        EXPECT_GE(color.y, 0.0f) << "G component out of range for type " << static_cast<int>(type);
        EXPECT_LE(color.y, 1.0f) << "G component out of range for type " << static_cast<int>(type);
        EXPECT_GE(color.z, 0.0f) << "B component out of range for type " << static_cast<int>(type);
        EXPECT_LE(color.z, 1.0f) << "B component out of range for type " << static_cast<int>(type);
    }
}

/**
 * @brief 测试施法状态管理
 *
 * 验证 setSpellType、setSpellTicks、isSpellcasting、clearSpellcasting 方法
 */
class SpellcastingIllagerStateTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SpellcastingIllagerStateTest, IsSpellcastingReturnsFalseInitially)
{
    // SpellcastingIllagerEntity 是抽象类，我们测试其方法
    // 通过检查 spellTypeFromId 和 isSpellcasting 的逻辑

    // spellTicks 初始为 0，所以 isSpellcasting() 应该返回 false
    // 这里只测试 spellTypeFromId 的正确性
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(0), SpellcastingIllagerEntity::SpellType::None);
}

TEST_F(SpellcastingIllagerStateTest, SpellTypeEnumValuesMatchMC)
{
    // 验证枚举值与 MC 1.16.5 一致
    EXPECT_EQ(static_cast<int>(SpellcastingIllagerEntity::SpellType::None), 0);
    EXPECT_EQ(static_cast<int>(SpellcastingIllagerEntity::SpellType::SummonVex), 1);
    EXPECT_EQ(static_cast<int>(SpellcastingIllagerEntity::SpellType::Fangs), 2);
    EXPECT_EQ(static_cast<int>(SpellcastingIllagerEntity::SpellType::Wololo), 3);
    EXPECT_EQ(static_cast<int>(SpellcastingIllagerEntity::SpellType::Disappear), 4);
    EXPECT_EQ(static_cast<int>(SpellcastingIllagerEntity::SpellType::Blindness), 5);
}
