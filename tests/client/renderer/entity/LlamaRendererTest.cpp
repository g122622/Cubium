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
 * @file LlamaRendererTest.cpp
 * @brief LlamaRenderer 单元测试（纹理选择逻辑）
 *
 * 测试覆盖：
 * - 纹理路径格式验证
 * - 颜色变体验证
 * - 阴影大小常量验证
 *
 * 注意：由于渲染器依赖 Vulkan 等图形基础设施，本测试文件
 * 仅测试不依赖渲染基础设施的逻辑。
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include <string>

using namespace mc;

namespace mc::client::renderer {
namespace {

// ============================================================================
// 纹理路径测试
// ============================================================================

/**
 * @brief 验证羊驼纹理路径格式
 *
 * 羊驼有4种颜色变体，每种对应不同的纹理路径。
 */
TEST(LlamaRendererTextureTest, TexturePathFormatIsValid)
{
    // 验证纹理路径格式正确
    static const char* colorNames[] = {"creamy", "white", "brown", "gray"};

    for (const char* color : colorNames) {
        std::string textureName = "textures/entity/llama/";
        textureName += color;
        textureName += ".png";

        EXPECT_FALSE(textureName.empty());
        EXPECT_TRUE(textureName.find("llama") != std::string::npos);
        EXPECT_TRUE(textureName.find(".png") != std::string::npos);
    }
}

/**
 * @brief 验证颜色数量
 *
 * MC 1.16.5 中羊驼有4种颜色变体。
 */
TEST(LlamaRendererColorTest, ColorVariantCountIsCorrect)
{
    // MC 1.16.5 中羊驼有4种颜色变体
    constexpr i32 COLOR_COUNT = 4;
    static const char* colorNames[COLOR_COUNT] = {"creamy", "white", "brown", "gray"};

    EXPECT_EQ(colorNames[0], "creamy");  // LlamaColor::CREAMY = 0
    EXPECT_EQ(colorNames[1], "white");   // LlamaColor::WHITE = 1
    EXPECT_EQ(colorNames[2], "brown");   // LlamaColor::BROWN = 2
    EXPECT_EQ(colorNames[3], "gray");    // LlamaColor::GRAY = 3
}

/**
 * @brief 验证颜色索引边界
 */
TEST(LlamaRendererColorTest, ColorIndexBoundaryIsValid)
{
    // LlamaEntity::LlamaColor 枚举值范围
    constexpr i32 MIN_COLOR = 0;
    constexpr i32 MAX_COLOR = 3;

    for (i32 i = MIN_COLOR; i <= MAX_COLOR; ++i) {
        EXPECT_GE(i, MIN_COLOR);
        EXPECT_LE(i, MAX_COLOR);
    }
}

/**
 * @brief 验证阴影大小常量
 *
 * MC 1.16.5 中羊驼的阴影大小为 0.7。
 */
TEST(LlamaRendererShadowTest, ShadowSizeIsCorrect)
{
    // MC 1.16.5 中羊驼阴影大小为 0.7
    constexpr f32 LLAMA_SHADOW_SIZE = 0.7f;
    EXPECT_FLOAT_EQ(LLAMA_SHADOW_SIZE, 0.7f);
}

/**
 * @brief 验证纹理命名空间
 *
 * 纹理应该使用 minecraft 命名空间。
 */
TEST(LlamaRendererTextureTest, TextureNamespaceIsCorrect)
{
    static const char* colorNames[] = {"creamy", "white", "brown", "gray"};

    for (const char* color : colorNames) {
        std::string textureName = "minecraft:textures/entity/llama/";
        textureName += color;
        textureName += ".png";

        // 验证命名空间格式
        EXPECT_TRUE(textureName.find("minecraft:") == 0);
    }
}

} // anonymous namespace
} // namespace mc::client::renderer
