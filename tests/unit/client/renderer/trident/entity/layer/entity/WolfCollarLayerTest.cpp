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

/**
 * @file WolfCollarLayerTest.cpp
 * @brief WolfCollarLayer 核心逻辑单元测试
 *
 * 测试狼项圈渲染层的核心逻辑，包括：
 * - shouldRender 逻辑：根据 ClientEntity::wolfTamed() 判断是否渲染项圈
 *   （shouldRender 直接代理 wolfTamed()，因此此处直接测试 ClientEntity 状态）
 * - 颜色映射：DyeColor 索引到 RGB 颜色（来自 WolfCollarColors.hpp，无需 Vulkan）
 * - 颜色边界处理（索引 >= 16 时回退到默认红色）
 *
 * 注意：WolfCollarLayer::renderPipeline 依赖 Vulkan 渲染管线，
 * 无法在单元测试中直接实例化 WolfCollarLayer。颜色映射逻辑抽离到
 * WolfCollarColors.hpp（仅头文件）以便独立测试。
 * shouldRender 逻辑为 `return entity.wolfTamed()`，此处通过测试
 * ClientEntity::wolfTamed() 的状态切换来验证其正确性。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/layer/entity/WolfCollarColors.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Constants.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"

using namespace mc;
using namespace mc::client::renderer::entity::layer::entity;
using namespace mc::client;

namespace mc::renderer::layer::test {

/**
 * @brief WolfCollarLayer 颜色映射测试固件
 *
 * 测试 wolf_collar_colors::getCollarColorByIndex 和 getCollarColor，
 * 这些函数是 WolfCollarLayer::_getCollarColor 的核心逻辑。
 */
class WolfCollarLayerColorTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// getCollarColorByIndex 测试
// ============================================================================

/**
 * @brief 测试所有 16 种 DyeColor 的颜色映射
 *
 * 对应 MC 1.21.11 DyeColor.getTextureDiffuseColor() 的 RGB 值
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_AllDyeColors)
{
    // 验证所有 16 种颜色都能正确获取（不崩溃、不越界）
    for (u8 i = 0; i < 16; ++i) {
        Vector3f color = wolf_collar_colors::getCollarColorByIndex(i);
        // 颜色值应在 [0, 1] 范围内
        EXPECT_GE(color.x, 0.0f);
        EXPECT_LE(color.x, 1.0f);
        EXPECT_GE(color.y, 0.0f);
        EXPECT_LE(color.y, 1.0f);
        EXPECT_GE(color.z, 0.0f);
        EXPECT_LE(color.z, 1.0f);
    }
}

/**
 * @brief 测试白色（索引 0）的颜色
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_White)
{
    Vector3f color = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::White));
    EXPECT_FLOAT_EQ(color.x, 1.0f);
    EXPECT_FLOAT_EQ(color.y, 1.0f);
    EXPECT_FLOAT_EQ(color.z, 1.0f);
}

/**
 * @brief 测试红色（索引 14，默认色）的颜色
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_Red)
{
    Vector3f color = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Red));
    EXPECT_FLOAT_EQ(color.x, 0.6f);
    EXPECT_FLOAT_EQ(color.y, 0.2f);
    EXPECT_FLOAT_EQ(color.z, 0.2f);
}

/**
 * @brief 测试黑色（索引 15）的颜色
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_Black)
{
    Vector3f color = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Black));
    EXPECT_FLOAT_EQ(color.x, 0.1f);
    EXPECT_FLOAT_EQ(color.y, 0.1f);
    EXPECT_FLOAT_EQ(color.z, 0.1f);
}

/**
 * @brief 测试橙色（索引 1）的颜色
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_Orange)
{
    Vector3f color = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Orange));
    EXPECT_FLOAT_EQ(color.x, 0.85f);
    EXPECT_FLOAT_EQ(color.y, 0.5f);
    EXPECT_FLOAT_EQ(color.z, 0.2f);
}

/**
 * @brief 测试蓝色（索引 11）的颜色
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_Blue)
{
    Vector3f color = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Blue));
    EXPECT_FLOAT_EQ(color.x, 0.2f);
    EXPECT_FLOAT_EQ(color.y, 0.3f);
    EXPECT_FLOAT_EQ(color.z, 0.7f);
}

/**
 * @brief 测试索引超出范围时回退到默认红色（索引 14）
 *
 * 对应 WolfCollarLayer::_getCollarColor 中的边界处理
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_OutOfRange_FallsBackToRed)
{
    Vector3f fallbackColor = wolf_collar_colors::getCollarColorByIndex(16);
    Vector3f redColor = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Red));

    EXPECT_FLOAT_EQ(fallbackColor.x, redColor.x);
    EXPECT_FLOAT_EQ(fallbackColor.y, redColor.y);
    EXPECT_FLOAT_EQ(fallbackColor.z, redColor.z);
}

/**
 * @brief 测试大索引值也回退到默认红色
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_LargeIndex_FallsBackToRed)
{
    Vector3f fallbackColor = wolf_collar_colors::getCollarColorByIndex(255);
    Vector3f redColor = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Red));

    EXPECT_FLOAT_EQ(fallbackColor.x, redColor.x);
    EXPECT_FLOAT_EQ(fallbackColor.y, redColor.y);
    EXPECT_FLOAT_EQ(fallbackColor.z, redColor.z);
}

/**
 * @brief 测试不同 DyeColor 索引返回不同颜色
 *
 * 确保颜色映射不是全返回同一个颜色
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColorByIndex_DifferentColorsDifferent)
{
    Vector3f white = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::White));
    Vector3f red = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Red));
    Vector3f black = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Black));
    Vector3f blue = wolf_collar_colors::getCollarColorByIndex(static_cast<u8>(DyeColor::Blue));

    // 白色和红色应该不同
    EXPECT_NE(white.x, red.x);
    // 红色和黑色应该不同
    EXPECT_NE(red.x, black.x);
    // 蓝色和黑色应该不同
    EXPECT_NE(blue.z, black.z);
}

// ============================================================================
// getCollarColor (DyeColor overload) 测试
// ============================================================================

/**
 * @brief 测试 getCollarColor(DyeColor) 重载
 *
 * 该重载是 WolfCollarLayer 内部使用的接口，接受 DyeColor 枚举而非 u8 索引
 */
TEST_F(WolfCollarLayerColorTest, GetCollarColor_DyeColorOverload)
{
    // 验证所有 16 种 DyeColor 都能正确获取
    for (i32 i = 0; i <= 15; ++i) {
        DyeColor color = static_cast<DyeColor>(i);
        Vector3f rgb = wolf_collar_colors::getCollarColor(color);
        // 颜色值应在 [0, 1] 范围内
        EXPECT_GE(rgb.x, 0.0f);
        EXPECT_LE(rgb.x, 1.0f);
        EXPECT_GE(rgb.y, 0.0f);
        EXPECT_LE(rgb.y, 1.0f);
        EXPECT_GE(rgb.z, 0.0f);
        EXPECT_LE(rgb.z, 1.0f);
    }

    // 验证与索引版本一致
    EXPECT_FLOAT_EQ(
        wolf_collar_colors::getCollarColor(DyeColor::White).x, wolf_collar_colors::getCollarColorByIndex(0).x);
    EXPECT_FLOAT_EQ(
        wolf_collar_colors::getCollarColor(DyeColor::Red).x, wolf_collar_colors::getCollarColorByIndex(14).x);
}

// ============================================================================
// COLLAR_COLORS 数组契约测试
// ============================================================================

/**
 * @brief 验证 COLLAR_COLORS 数组大小为 16（对应 16 种 DyeColor）
 */
TEST_F(WolfCollarLayerColorTest, CollarColorsArray_Has16Entries)
{
    // 数组大小应为 16
    EXPECT_EQ(sizeof(wolf_collar_colors::COLLAR_COLORS) / sizeof(Vector3f), 16);
}

/**
 * @brief 验证 COLLAR_COLORS 数组索引与 DyeColor 枚举值对应
 *
 * 确保 WolfCollarLayer::_getCollarColor 通过
 * `static_cast<u8>(entity.wolfCollarColor())` 索引时能拿到正确颜色
 */
TEST_F(WolfCollarLayerColorTest, CollarColorsArray_IndexMatchesDyeColorEnum)
{
    // DyeColor::White = 0
    EXPECT_FLOAT_EQ(wolf_collar_colors::COLLAR_COLORS[static_cast<u8>(DyeColor::White)].x, 1.0f);
    // DyeColor::Red = 14
    EXPECT_FLOAT_EQ(wolf_collar_colors::COLLAR_COLORS[static_cast<u8>(DyeColor::Red)].x, 0.6f);
    // DyeColor::Black = 15
    EXPECT_FLOAT_EQ(wolf_collar_colors::COLLAR_COLORS[static_cast<u8>(DyeColor::Black)].x, 0.1f);
}

// ============================================================================
// shouldRender 逻辑测试（通过 ClientEntity::wolfTamed 验证）
// ============================================================================

/**
 * @brief WolfCollarLayer shouldRender 逻辑测试固件
 *
 * WolfCollarLayer::shouldRender 实现为 `return entity.wolfTamed()`，
 * 直接代理 ClientEntity::wolfTamed()。由于 WolfCollarLayer 依赖 Vulkan
 * 渲染管线无法在单元测试中实例化，此处通过测试 ClientEntity::wolfTamed()
 * 的状态切换来验证 shouldRender 的核心逻辑。
 */
class WolfCollarLayerShouldRenderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建一个 minecraft:wolf 类型的 ClientEntity
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:wolf");
    }

    void TearDown() override { entity.reset(); }

    std::unique_ptr<ClientEntity> entity;
};

/**
 * @brief 未驯服的狼不应渲染项圈
 *
 * 对应 MC 1.21.11 WolfCollarLayer.submit() 中 collarColor == null 的检查
 * （extractRenderState 在未驯服时将 collarColor 设为 null）
 *
 * WolfCollarLayer::shouldRender 直接返回 entity.wolfTamed()，
 * 因此 wolfTamed() == false 时 shouldRender 返回 false。
 */
TEST_F(WolfCollarLayerShouldRenderTest, NotTamed_ReturnsFalse)
{
    entity->setWolfTamed(false);
    EXPECT_FALSE(entity->wolfTamed()); // shouldRender 代理此值
}

/**
 * @brief 驯服的狼应渲染项圈
 */
TEST_F(WolfCollarLayerShouldRenderTest, Tamed_ReturnsTrue)
{
    entity->setWolfTamed(true);
    EXPECT_TRUE(entity->wolfTamed()); // shouldRender 代理此值
}

/**
 * @brief 默认（未设置驯服状态）不应渲染项圈
 *
 * ClientEntity 默认 m_wolfTamed = false
 */
TEST_F(WolfCollarLayerShouldRenderTest, Default_ReturnsFalse)
{
    EXPECT_FALSE(entity->wolfTamed()); // shouldRender 代理此值
}

/**
 * @brief 驯服状态切换后 shouldRender 正确响应
 */
TEST_F(WolfCollarLayerShouldRenderTest, StateToggle)
{
    entity->setWolfTamed(true);
    EXPECT_TRUE(entity->wolfTamed());

    entity->setWolfTamed(false);
    EXPECT_FALSE(entity->wolfTamed());

    entity->setWolfTamed(true);
    EXPECT_TRUE(entity->wolfTamed());
}

// ============================================================================
// 集成场景测试（shouldRender + 颜色映射）
// ============================================================================

/**
 * @brief 集成场景：驯服狼 + 蓝色项圈
 *
 * 模拟玩家用蓝色染料右键已驯服狼后的状态：
 * 1. wolfTamed == true → shouldRender 返回 true
 * 2. wolfCollarColor == Blue → _getCollarColor 返回蓝色 RGB
 */
TEST_F(WolfCollarLayerShouldRenderTest, Integration_TamedBlueCollar)
{
    entity->setWolfTamed(true);
    entity->setWolfCollarColor(DyeColor::Blue);

    // shouldRender 逻辑：wolfTamed() == true
    EXPECT_TRUE(entity->wolfTamed());

    // _getCollarColor 逻辑：getCollarColor(wolfCollarColor())
    Vector3f expectedColor = wolf_collar_colors::getCollarColor(entity->wolfCollarColor());
    EXPECT_FLOAT_EQ(expectedColor.x, 0.2f);
    EXPECT_FLOAT_EQ(expectedColor.y, 0.3f);
    EXPECT_FLOAT_EQ(expectedColor.z, 0.7f);
}

/**
 * @brief 集成场景：未驯服狼不应渲染项圈（即使设置了颜色）
 */
TEST_F(WolfCollarLayerShouldRenderTest, Integration_NotTamedNoRenderEvenWithColor)
{
    entity->setWolfTamed(false);
    entity->setWolfCollarColor(DyeColor::Blue);

    // shouldRender 逻辑：wolfTamed() == false
    EXPECT_FALSE(entity->wolfTamed());

    // 但颜色映射仍然可用（MC 中 extractRenderState 在未驯服时设 collarColor 为 null，
    // 但 ClientEntity::wolfCollarColor 仍保留最后值，仅 shouldRender 会被跳过）
    Vector3f color = wolf_collar_colors::getCollarColor(entity->wolfCollarColor());
    EXPECT_FLOAT_EQ(color.z, 0.7f); // 蓝色 z 分量
}

/**
 * @brief 集成场景：默认状态（未驯服 + 红色项圈）
 */
TEST_F(WolfCollarLayerShouldRenderTest, Integration_DefaultState)
{
    // 默认状态
    EXPECT_FALSE(entity->wolfTamed());
    EXPECT_EQ(entity->wolfCollarColor(), DyeColor::Red);

    // shouldRender 逻辑：wolfTamed() == false
    EXPECT_FALSE(entity->wolfTamed());

    // 默认颜色映射为红色
    Vector3f color = wolf_collar_colors::getCollarColor(entity->wolfCollarColor());
    EXPECT_FLOAT_EQ(color.x, 0.6f);
    EXPECT_FLOAT_EQ(color.y, 0.2f);
    EXPECT_FLOAT_EQ(color.z, 0.2f);
}

/**
 * @brief 集成场景：所有 16 种 DyeColor 的项圈颜色都能正确映射
 *
 * 模拟玩家用不同染料右键已驯服狼后的项圈颜色
 */
TEST_F(WolfCollarLayerShouldRenderTest, Integration_AllDyeColorsMapCorrectly)
{
    entity->setWolfTamed(true);

    for (i32 i = 0; i <= 15; ++i) {
        DyeColor color = static_cast<DyeColor>(i);
        entity->setWolfCollarColor(color);

        // shouldRender 应该始终为 true（已驯服）
        EXPECT_TRUE(entity->wolfTamed());

        // 颜色映射应正确
        Vector3f rgb = wolf_collar_colors::getCollarColor(entity->wolfCollarColor());
        EXPECT_EQ(entity->wolfCollarColor(), color);
        EXPECT_GE(rgb.x, 0.0f);
        EXPECT_LE(rgb.x, 1.0f);
    }
}

} // namespace mc::renderer::layer::test
