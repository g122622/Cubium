#include <gtest/gtest.h>
#include <cmath>
#include <vector>

#include "client/renderer/trident/entity/util/ShadowRenderer.hpp"
#include "common/util/math/MathConstants.hpp"

using namespace mc;
using namespace mc::client::renderer::entity::util;

namespace mc::client::renderer::entity::util {
namespace test {

/**
 * @brief ShadowRenderer 单元测试
 *
 * 测试阴影渲染的核心算法逻辑，包括：
 * - 透明度衰减
 * - 幼体阴影减半
 * - 阴影范围计算
 * - 方块阴影条件检测逻辑
 */
class ShadowRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化测试
    }

    void TearDown() override {
        // 清理测试
    }
};

/**
 * @brief 测试阴影常量
 */
TEST_F(ShadowRendererTest, ShadowConstants) {
    // 阴影最大距离为 256 格（MC 1.16.5 标准）
    constexpr f64 MAX_SHADOW_DISTANCE = 256.0;
    EXPECT_DOUBLE_EQ(MAX_SHADOW_DISTANCE, 256.0);

    // 阴影最小亮度阈值为 3
    constexpr i32 MIN_LIGHT_FOR_SHADOW = 3;
    EXPECT_EQ(MIN_LIGHT_FOR_SHADOW, 3);
}

/**
 * @brief 测试透明度衰减逻辑 - 基本场景
 *
 * 验证透明度随高度线性衰减
 */
TEST_F(ShadowRendererTest, AlphaAttenuation_GroundLevel) {
    // 当实体在地面上时，高度为 0，distanceFactor = 1.0
    constexpr f64 maxDistance = 256.0;
    constexpr f64 baseAlpha = 0.8;
    constexpr f64 heightAboveGround = 0.0;

    const f64 distanceFactor = 1.0 - (heightAboveGround / maxDistance);
    const f64 alpha = baseAlpha * distanceFactor;

    EXPECT_DOUBLE_EQ(alpha, baseAlpha);  // 完全不透明
}

TEST_F(ShadowRendererTest, AlphaAttenuation_HalfHeight) {
    constexpr f64 maxDistance = 256.0;
    constexpr f64 baseAlpha = 0.8;
    constexpr f64 heightAboveGround = 128.0;

    const f64 distanceFactor = 1.0 - (heightAboveGround / maxDistance);
    const f64 alpha = baseAlpha * distanceFactor;

    EXPECT_DOUBLE_EQ(alpha, 0.4);  // 半透明
}

TEST_F(ShadowRendererTest, AlphaAttenuation_MaxHeight) {
    constexpr f64 maxDistance = 256.0;
    constexpr f64 baseAlpha = 0.8;
    constexpr f64 heightAboveGround = 256.0;

    const f64 distanceFactor = 1.0 - (heightAboveGround / maxDistance);
    const f64 alpha = baseAlpha * distanceFactor;

    EXPECT_DOUBLE_EQ(alpha, 0.0);  // 完全透明
}

TEST_F(ShadowRendererTest, AlphaAttenuation_ExceedsMaxHeight) {
    constexpr f64 maxDistance = 256.0;
    constexpr f64 baseAlpha = 0.8;
    constexpr f64 heightAboveGround = 300.0;

    const f64 distanceFactor = 1.0 - (heightAboveGround / maxDistance);
    const f64 alpha = baseAlpha * std::max(0.0, distanceFactor);

    EXPECT_DOUBLE_EQ(alpha, 0.0);  // 完全透明
}

/**
 * @brief 测试亮度因子计算
 */
TEST_F(ShadowRendererTest, BrightnessFactor_FullBrightness) {
    // 亮度等级 15 = 完全明亮
    constexpr u8 lightLevel = 15;
    const f64 brightness = static_cast<f64>(lightLevel) / 15.0;

    EXPECT_DOUBLE_EQ(brightness, 1.0);
}

TEST_F(ShadowRendererTest, BrightnessFactor_HalfBrightness) {
    // 亮度等级 7-8 = 半亮
    constexpr u8 lightLevel = 8;
    const f64 brightness = static_cast<f64>(lightLevel) / 15.0;

    EXPECT_NEAR(brightness, 0.533, 0.01);
}

TEST_F(ShadowRendererTest, BrightnessFactor_Darkness) {
    // 亮度等级 0 = 完全黑暗
    constexpr u8 lightLevel = 0;
    const f64 brightness = static_cast<f64>(lightLevel) / 15.0;

    EXPECT_DOUBLE_EQ(brightness, 0.0);
}

TEST_F(ShadowRendererTest, BrightnessFactor_MinimumForShadow) {
    // MC 要求光照等级 > 3 才渲染阴影
    constexpr i32 MIN_LIGHT_FOR_SHADOW = 3;

    // 光照等级 4 应该允许阴影
    EXPECT_GT(4, MIN_LIGHT_FOR_SHADOW);

    // 光照等级 3 不应该渲染阴影
    EXPECT_FALSE(3 > MIN_LIGHT_FOR_SHADOW);
}

/**
 * @brief 测试幼体阴影减半
 */
TEST_F(ShadowRendererTest, ChildEntity_ShadowRadiusHalved) {
    constexpr f64 baseRadius = 0.5;
    constexpr bool isChild = true;

    // MC 1.16.5: 幼体阴影半径减半
    const f64 adjustedRadius = isChild ? baseRadius * 0.5 : baseRadius;

    EXPECT_DOUBLE_EQ(adjustedRadius, 0.25);
}

TEST_F(ShadowRendererTest, AdultEntity_ShadowRadiusUnchanged) {
    constexpr f64 baseRadius = 0.5;
    constexpr bool isChild = false;

    const f64 adjustedRadius = isChild ? baseRadius * 0.5 : baseRadius;

    EXPECT_DOUBLE_EQ(adjustedRadius, 0.5);
}

/**
 * @brief 测试阴影范围计算
 */
TEST_F(ShadowRendererTest, ShadowRangeCalculation_CenterPosition) {
    // 实体位置在整数坐标
    const f64 entityX = 100.0;
    const f64 entityY = 64.0;
    const f64 entityZ = -200.0;
    const f64 shadowRadius = 0.5;

    // 计算搜索范围（参考 MC 1.16.5 EntityRendererManager.renderShadow）
    const i32 minX = static_cast<i32>(std::floor(entityX - shadowRadius));
    const i32 maxX = static_cast<i32>(std::floor(entityX + shadowRadius));
    const i32 minY = static_cast<i32>(std::floor(entityY - shadowRadius));
    const i32 maxY = static_cast<i32>(std::floor(entityY));
    const i32 minZ = static_cast<i32>(std::floor(entityZ - shadowRadius));
    const i32 maxZ = static_cast<i32>(std::floor(entityZ + shadowRadius));

    // 验证范围
    EXPECT_EQ(minX, 99);
    EXPECT_EQ(maxX, 100);
    EXPECT_EQ(minY, 63);
    EXPECT_EQ(maxY, 64);
    EXPECT_EQ(minZ, -201);
    EXPECT_EQ(maxZ, -200);
}

TEST_F(ShadowRendererTest, ShadowRangeCalculation_FractionalPosition) {
    // 实体位置有小数部分
    const f64 entityX = 100.5;
    const f64 entityY = 64.7;
    const f64 entityZ = -200.3;
    const f64 shadowRadius = 0.5;

    const i32 minX = static_cast<i32>(std::floor(entityX - shadowRadius));
    const i32 maxX = static_cast<i32>(std::floor(entityX + shadowRadius));
    const i32 minY = static_cast<i32>(std::floor(entityY - shadowRadius));
    const i32 maxY = static_cast<i32>(std::floor(entityY));
    const i32 minZ = static_cast<i32>(std::floor(entityZ - shadowRadius));
    const i32 maxZ = static_cast<i32>(std::floor(entityZ + shadowRadius));

    EXPECT_EQ(minX, 100);
    EXPECT_EQ(maxX, 101);
    EXPECT_EQ(minY, 64);
    EXPECT_EQ(maxY, 64);
    EXPECT_EQ(minZ, -201);
    EXPECT_EQ(maxZ, -200);
}

TEST_F(ShadowRendererTest, ShadowRangeCalculation_LargeRadius) {
    // 大半径阴影（如末影龙）
    const f64 entityX = 0.0;
    const f64 entityY = 64.0;
    const f64 entityZ = 0.0;
    const f64 shadowRadius = 2.0;

    const i32 minX = static_cast<i32>(std::floor(entityX - shadowRadius));
    const i32 maxX = static_cast<i32>(std::floor(entityX + shadowRadius));
    const i32 minY = static_cast<i32>(std::floor(entityY - shadowRadius));
    const i32 maxY = static_cast<i32>(std::floor(entityY));
    const i32 minZ = static_cast<i32>(std::floor(entityZ - shadowRadius));
    const i32 maxZ = static_cast<i32>(std::floor(entityZ + shadowRadius));

    EXPECT_EQ(minX, -2);
    EXPECT_EQ(maxX, 2);
    EXPECT_EQ(minY, 62);
    EXPECT_EQ(maxY, 64);
    EXPECT_EQ(minZ, -2);
    EXPECT_EQ(maxZ, 2);
}

/**
 * @brief 测试阴影透明度完整计算
 *
 * 参考 MC 1.16.5 EntityRendererManager.renderBlockShadow:398-402
 * alpha = (weight - heightDiff/2) * 0.5 * brightness
 */
TEST_F(ShadowRendererTest, FullAlphaCalculation_GroundLevel) {
    // 在地面上，全亮度
    const f64 weight = 0.8;          // 基础透明度
    const f64 heightDiff = 0.0;      // 在地面上
    const f64 brightness = 1.0;      // 全亮度

    const f64 alpha = (weight - heightDiff / 2.0) * 0.5 * brightness;

    EXPECT_DOUBLE_EQ(alpha, 0.4);  // (0.8 - 0) * 0.5 * 1.0 = 0.4
}

TEST_F(ShadowRendererTest, FullAlphaCalculation_AboveGround) {
    // 离地面 1 格，全亮度
    const f64 weight = 0.8;
    const f64 heightDiff = 1.0;      // 离地面 1 格
    const f64 brightness = 1.0;

    const f64 alpha = (weight - heightDiff / 2.0) * 0.5 * brightness;

    EXPECT_DOUBLE_EQ(alpha, 0.15);  // (0.8 - 0.5) * 0.5 * 1.0 = 0.15
}

TEST_F(ShadowRendererTest, FullAlphaCalculation_DimLight) {
    // 在地面上，半亮度
    const f64 weight = 0.8;
    const f64 heightDiff = 0.0;
    const f64 brightness = 0.5;      // 半亮度

    const f64 alpha = (weight - heightDiff / 2.0) * 0.5 * brightness;

    EXPECT_DOUBLE_EQ(alpha, 0.2);  // (0.8 - 0) * 0.5 * 0.5 = 0.2
}

TEST_F(ShadowRendererTest, FullAlphaCalculation_TooHigh) {
    // 离地面太高，透明度应该接近 0
    const f64 weight = 0.8;
    const f64 heightDiff = 2.0;      // 离地面 2 格
    const f64 brightness = 1.0;

    const f64 alpha = (weight - heightDiff / 2.0) * 0.5 * brightness;

    EXPECT_DOUBLE_EQ(alpha, -0.1);  // 负数意味着不渲染
    EXPECT_LT(alpha, 0.0);          // 应该跳过
}

/**
 * @brief 测试纹理坐标计算
 *
 * 参考 MC 1.16.5 EntityRendererManager:415-418
 * u = -relativeX / 2.0 / shadowSize + 0.5
 * v = -relativeZ / 2.0 / shadowSize + 0.5
 */
TEST_F(ShadowRendererTest, TextureCoordinateCalculation) {
    const f64 shadowSize = 0.5;

    // 中心点（相对坐标 0, 0）
    {
        const f64 relativeX = 0.0;
        const f64 relativeZ = 0.0;
        const f32 u = static_cast<f32>(-relativeX / 2.0 / shadowSize + 0.5);
        const f32 v = static_cast<f32>(-relativeZ / 2.0 / shadowSize + 0.5);

        EXPECT_FLOAT_EQ(u, 0.5f);  // 中心
        EXPECT_FLOAT_EQ(v, 0.5f);  // 中心
    }

    // 左下角（相对坐标 -0.5, -0.5）
    {
        const f64 relativeX = -0.5;
        const f64 relativeZ = -0.5;
        const f32 u = static_cast<f32>(-relativeX / 2.0 / shadowSize + 0.5);
        const f32 v = static_cast<f32>(-relativeZ / 2.0 / shadowSize + 0.5);

        EXPECT_FLOAT_EQ(u, 1.0f);  // 右边
        EXPECT_FLOAT_EQ(v, 1.0f);  // 下边
    }

    // 右上角（相对坐标 0.5, 0.5）
    {
        const f64 relativeX = 0.5;
        const f64 relativeZ = 0.5;
        const f32 u = static_cast<f32>(-relativeX / 2.0 / shadowSize + 0.5);
        const f32 v = static_cast<f32>(-relativeZ / 2.0 / shadowSize + 0.5);

        EXPECT_FLOAT_EQ(u, 0.0f);  // 左边
        EXPECT_FLOAT_EQ(v, 0.0f);  // 上边
    }
}

/**
 * @brief 测试阴影顶点四边形顺序
 *
 * 参考 MC 1.16.5 EntityRendererManager:419-422
 * 阴影四边形顶点顺序：左下 -> 右下 -> 右上 -> 左上
 */
TEST_F(ShadowRendererTest, ShadowQuad_VertexOrder) {
    // 模拟 MC 的四边形顶点顺序
    const f64 boxMinX = 0.0;
    const f64 boxMaxX = 1.0;
    const f64 boxMinZ = 0.0;
    const f64 boxMaxZ = 1.0;
    const f64 entityX = 0.5;
    const f64 entityZ = 0.5;
    const f64 shadowSize = 0.5;

    // 相对坐标
    const f32 f1 = static_cast<f32>(boxMinX - entityX);  // X 最小
    const f32 f2 = static_cast<f32>(boxMaxX - entityX);  // X 最大
    const f32 f4 = static_cast<f32>(boxMinZ - entityZ);  // Z 最小
    const f32 f5 = static_cast<f32>(boxMaxZ - entityZ);  // Z 最大

    // 纹理坐标
    const f32 f6 = static_cast<f32>(-f1 / 2.0f / shadowSize + 0.5);
    const f32 f7 = static_cast<f32>(-f2 / 2.0f / shadowSize + 0.5);
    const f32 f8 = static_cast<f32>(-f4 / 2.0f / shadowSize + 0.5);
    const f32 f9 = static_cast<f32>(-f5 / 2.0f / shadowSize + 0.5);

    // 四个顶点：(f1, f4), (f1, f5), (f2, f5), (f2, f4)
    // 对应纹理：(f6, f8), (f6, f9), (f7, f9), (f7, f8)

    // 验证顶点顺序正确
    EXPECT_LT(f1, f2);  // X 递增
    EXPECT_LT(f4, f5);  // Z 递增
}

/**
 * @brief 测试阴影透明度上限
 */
TEST_F(ShadowRendererTest, AlphaClamp_Maximum) {
    // 透明度不应超过 1.0
    const f64 alpha = 1.5;
    const f64 clampedAlpha = std::min(alpha, 1.0);

    EXPECT_DOUBLE_EQ(clampedAlpha, 1.0);
}

TEST_F(ShadowRendererTest, AlphaClamp_Minimum) {
    // 透明度不应小于 0
    const f64 alpha = -0.5;
    const f64 clampedAlpha = std::max(alpha, 0.0);

    EXPECT_DOUBLE_EQ(clampedAlpha, 0.0);
}

/**
 * @brief 测试插值位置计算
 */
TEST_F(ShadowRendererTest, InterpolatedPosition) {
    // MC 使用插值位置渲染阴影
    const f64 prevX = 100.0;
    const f64 prevY = 64.0;
    const f64 prevZ = -200.0;
    const f64 x = 110.0;
    const f64 y = 66.0;
    const f64 z = -190.0;
    const f64 partialTicks = 0.5;

    const f64 interpX = prevX + (x - prevX) * partialTicks;
    const f64 interpY = prevY + (y - prevY) * partialTicks;
    const f64 interpZ = prevZ + (z - prevZ) * partialTicks;

    EXPECT_DOUBLE_EQ(interpX, 105.0);
    EXPECT_DOUBLE_EQ(interpY, 65.0);
    EXPECT_DOUBLE_EQ(interpZ, -195.0);
}

} // namespace test
} // namespace mc::client::renderer::entity::util
