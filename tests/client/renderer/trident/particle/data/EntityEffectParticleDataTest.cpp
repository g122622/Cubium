/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "client/renderer/trident/particle/data/EntityEffectParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <memory>
#include <glm/glm.hpp>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::data;

// ==================== EntityEffectParticleData 测试 ====================

class EntityEffectParticleDataTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        testColor = 0xFFFF0000; // 红色
    }

    u32 testColor;
};

TEST_F(EntityEffectParticleDataTest, Construction_SetsColor)
{
    EntityEffectParticleData data(testColor);

    EXPECT_EQ(data.color(), 0xFFFF0000u);
}

TEST_F(EntityEffectParticleDataTest, Construction_DefaultValue)
{
    EntityEffectParticleData data;

    // 默认白色（ARGB=0xFFFFFFFF）
    EXPECT_EQ(data.color(), 0xFFFFFFFFu);
}

TEST_F(EntityEffectParticleDataTest, GetType_ReturnsEntityEffect)
{
    EntityEffectParticleData data(testColor);

    EXPECT_EQ(data.getType(), ParticleTypeId::EntityEffect);
}

TEST_F(EntityEffectParticleDataTest, GetTypeName_ReturnsEntityEffectName)
{
    EntityEffectParticleData data(testColor);

    EXPECT_EQ(data.getTypeName(), "minecraft:entity_effect");
}

TEST_F(EntityEffectParticleDataTest, GetParameters_ContainsColor)
{
    EntityEffectParticleData data(0xFF00FF00);

    auto params = data.getParameters();

    // 参数格式: "0xAARRGGBB"
    EXPECT_NE(params.find("0xFF00FF00"), std::string::npos);
}

TEST_F(EntityEffectParticleDataTest, Clone_ReturnsIdenticalCopy)
{
    EntityEffectParticleData data(0xFF123456);

    auto cloned = data.clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getType(), ParticleTypeId::EntityEffect);
    auto* clonedEffect = dynamic_cast<EntityEffectParticleData*>(cloned.get());
    ASSERT_NE(clonedEffect, nullptr);
    EXPECT_EQ(clonedEffect->color(), 0xFF123456u);
}

TEST_F(EntityEffectParticleDataTest, ToRGBAVector_ConvertsCorrectly)
{
    // ARGB 0xFFFF0000 -> RGBA (1.0, 0.0, 0.0, 1.0)
    EntityEffectParticleData data(0xFFFF0000);

    glm::vec4 rgba = data.toRGBAVector();

    EXPECT_FLOAT_EQ(rgba.r, 1.0f);
    EXPECT_FLOAT_EQ(rgba.g, 0.0f);
    EXPECT_FLOAT_EQ(rgba.b, 0.0f);
    EXPECT_FLOAT_EQ(rgba.a, 1.0f);
}

TEST_F(EntityEffectParticleDataTest, ToRGBAVector_HandlesAllChannels)
{
    // ARGB 0xAABBCCDD -> RGBA (0xBB/255, 0xCC/255, 0xDD/255, 0xAA/255)
    EntityEffectParticleData data(0xAABBCCDD);

    glm::vec4 rgba = data.toRGBAVector();

    EXPECT_FLOAT_EQ(rgba.r, 0xBB / 255.0f);
    EXPECT_FLOAT_EQ(rgba.g, 0xCC / 255.0f);
    EXPECT_FLOAT_EQ(rgba.b, 0xDD / 255.0f);
    EXPECT_FLOAT_EQ(rgba.a, 0xAA / 255.0f);
}

TEST_F(EntityEffectParticleDataTest, FromRGBAVector_RoundTrip)
{
    EntityEffectParticleData original(0xFF80C0E0);
    glm::vec4 rgba = original.toRGBAVector();

    EntityEffectParticleData reconstructed = EntityEffectParticleData::fromRGBAVector(rgba);

    EXPECT_EQ(reconstructed.color(), 0xFF80C0E0u);
}

TEST_F(EntityEffectParticleDataTest, BellBlockEntity_StartingColor)
{
    // MC 原版 BellBlockEntity 起始颜色 16700985 = 0x00FED639
    EntityEffectParticleData data(16700985u);

    EXPECT_EQ(data.color(), 16700985u);

    // 转换为 RGBA 向量验证
    // ARGB 0x00FED639 -> RGBA (0xFE/255, 0xD6/255, 0x39/255, 0x00/255)
    glm::vec4 rgba = data.toRGBAVector();
    EXPECT_FLOAT_EQ(rgba.r, 0xFE / 255.0f);
    EXPECT_FLOAT_EQ(rgba.g, 0xD6 / 255.0f);
    EXPECT_FLOAT_EQ(rgba.b, 0x39 / 255.0f);
    EXPECT_FLOAT_EQ(rgba.a, 0x00 / 255.0f); // alpha=0
}

TEST_F(EntityEffectParticleDataTest, BellBlockEntity_ColorIncrementSequence)
{
    // 模拟 BellBlockEntity._showBellParticles 的颜色递增序列
    i32 colorCounter = 16700985;
    for (i32 k = 0; k < 15; ++k) {
        colorCounter += 5;
        const u32 expectedColor = static_cast<u32>(colorCounter);

        EntityEffectParticleData data(expectedColor);
        EXPECT_EQ(data.color(), expectedColor);
    }
}

TEST_F(EntityEffectParticleDataTest, CopyConstruction)
{
    EntityEffectParticleData original(0xFFAABBCC);
    EntityEffectParticleData copy(original);

    EXPECT_EQ(copy.color(), 0xFFAABBCCu);
    EXPECT_EQ(copy.getType(), ParticleTypeId::EntityEffect);
}

TEST_F(EntityEffectParticleDataTest, MoveConstruction)
{
    EntityEffectParticleData original(0xFFAABBCC);
    EntityEffectParticleData moved(std::move(original));

    EXPECT_EQ(moved.color(), 0xFFAABBCCu);
}

} // namespace
} // namespace mc
