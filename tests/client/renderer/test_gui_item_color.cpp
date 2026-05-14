#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "client/renderer/trident/gui/GuiRenderer.hpp"

namespace mc::client {

TEST(GuiItemColorTest, ItemTextureColorUsesItemSamplerBranch)
{
    constexpr u32 color = GuiRenderer::ITEM_TEXTURE_COLOR;
    constexpr u32 alpha = (color >> 24) & 0xFF;

    // alpha=255 会被GUI片段着色器当作字体采样模式
    EXPECT_LT(alpha, 255u);

    // alpha必须接近1，避免物品图标被意外透明
    EXPECT_GE(alpha, 254u);
}

TEST(GuiItemColorTest, ItemTextureColorIsNotTransparent)
{
    constexpr u32 color = GuiRenderer::ITEM_TEXTURE_COLOR;
    constexpr u32 alpha = (color >> 24) & 0xFF;
    const f32 outputAlpha = static_cast<f32>(alpha) / 255.0f;

    EXPECT_GT(outputAlpha, 0.99f);
}

TEST(GuiShaderRegressionTest, FontSamplingUsesTextureAlpha)
{
    const std::filesystem::path shaderPath = std::filesystem::path(MC_SOURCE_DIR) / "shaders" / "gui.frag";
    ASSERT_TRUE(std::filesystem::exists(shaderPath)) << "Shader file not found: " << shaderPath.string();

    std::ifstream shaderFile(shaderPath, std::ios::binary);
    ASSERT_TRUE(shaderFile.is_open()) << "Failed to open shader file: " << shaderPath.string();

    std::stringstream buffer;
    buffer << shaderFile.rdbuf();
    const std::string shaderSource = buffer.str();

    EXPECT_NE(shaderSource.find("case 0u: return vec4(1.0, 1.0, 1.0, texture(fontSampler, uv).r);"), std::string::npos)
        << "Font slot should write glyph coverage to alpha channel";
}

TEST(GuiShaderRegressionTest, SolidRectMarkerStillEnabled)
{
    const std::filesystem::path shaderPath = std::filesystem::path(MC_SOURCE_DIR) / "shaders" / "gui.frag";
    ASSERT_TRUE(std::filesystem::exists(shaderPath)) << "Shader file not found: " << shaderPath.string();

    std::ifstream shaderFile(shaderPath, std::ios::binary);
    ASSERT_TRUE(shaderFile.is_open()) << "Failed to open shader file: " << shaderPath.string();

    std::stringstream buffer;
    buffer << shaderFile.rdbuf();
    const std::string shaderSource = buffer.str();

    EXPECT_NE(shaderSource.find("if (fragTexCoord.x < 0.0 || fragTexCoord.y < 0.0)"), std::string::npos)
        << "Solid rect UV marker branch must not be removed";
}

} // namespace mc::client
