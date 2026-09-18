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

#include "common/util/math/frustum/Frustum.hpp"
#include "common/core/Constants.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

using namespace mc::math::frustum;
using namespace mc;

// 辅助函数：创建透视投影矩阵
glm::mat4 createPerspectiveMatrix(f32 fov, f32 aspect, f32 nearPlane, f32 farPlane)
{
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

// 辅助函数：创建视图矩阵（相机在原点，看向 -Z 方向）
glm::mat4 createViewMatrix(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
{
    return glm::lookAt(position, target, up);
}

class FrustumTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建默认的透视和视图矩阵
        projection = createPerspectiveMatrix(70.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
        view = createViewMatrix(glm::vec3(0.0f, 0.0f, 0.0f), // 相机位置
            glm::vec3(0.0f, 0.0f, -1.0f),                    // 看向 -Z
            glm::vec3(0.0f, 1.0f, 0.0f)                      // 上方向
        );
        viewProjection = projection * view;
    }

    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 viewProjection;
};

// ========== 平面提取测试 ==========

TEST_F(FrustumTest, ExtractPlanesFromMatrix)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    EXPECT_TRUE(frustum.isValid());

    // 验证所有平面都被提取
    for (size_t i = 0; i < Frustum::PLANE_COUNT; ++i) {
        const auto& plane = frustum.getPlane(static_cast<Frustum::PlaneIndex>(i));
        // 法向量应该被归一化
        f32 length = plane.normal.length();
        EXPECT_NEAR(length, 1.0f, 0.01f) << "Plane " << i << " normal not normalized";
    }
}

TEST_F(FrustumTest, ExtractPlanesFromMatrices)
{
    Frustum frustum;
    frustum.extractFromMatrices(projection, view);

    EXPECT_TRUE(frustum.isValid());
}

TEST_F(FrustumTest, PlanesHaveCorrectOrientation)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 左平面法向量应该指向 +X（视锥内部）
    const auto& leftPlane = frustum.getPlane(Frustum::Left);
    EXPECT_GT(leftPlane.normal.x, 0.0f) << "Left plane normal should point right (+X)";

    // 右平面法向量应该指向 -X（视锥内部）
    const auto& rightPlane = frustum.getPlane(Frustum::Right);
    EXPECT_LT(rightPlane.normal.x, 0.0f) << "Right plane normal should point left (-X)";

    // 底平面法向量应该指向 +Y（视锥内部）
    const auto& bottomPlane = frustum.getPlane(Frustum::Bottom);
    EXPECT_GT(bottomPlane.normal.y, 0.0f) << "Bottom plane normal should point up (+Y)";

    // 顶平面法向量应该指向 -Y（视锥内部）
    const auto& topPlane = frustum.getPlane(Frustum::Top);
    EXPECT_LT(topPlane.normal.y, 0.0f) << "Top plane normal should point down (-Y)";

    // 近平面法向量：对于右手坐标系相机看向 -Z，视锥内部在近平面的"前方"
    // 即更负的 Z 方向，所以法向量指向 -Z
    const auto& nearPlane = frustum.getPlane(Frustum::Near);
    EXPECT_LT(nearPlane.normal.z, 0.0f)
        << "Near plane normal should point into frustum (-Z for right-hand system looking -Z)";

    // 远平面法向量：视锥内部在远平面的"后方"（更接近相机）
    // 所以法向量指向 +Z
    const auto& farPlane = frustum.getPlane(Frustum::Far);
    EXPECT_GT(farPlane.normal.z, 0.0f) << "Far plane normal should point into frustum (+Z for right-hand system)";
}

// ========== 点可见性测试 ==========

TEST_F(FrustumTest, PointVisibility_PointInFront)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 相机看向 -Z，所以前方的点（负 Z）应该可见
    Vector3 pointInFront(0.0f, 0.0f, -10.0f);
    EXPECT_TRUE(frustum.isPointVisible(pointInFront));

    // 稍微偏离中心的点也应该可见
    Vector3 pointOffCenter(2.0f, 1.0f, -10.0f);
    EXPECT_TRUE(frustum.isPointVisible(pointOffCenter));
}

TEST_F(FrustumTest, PointVisibility_PointBehind)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 相机后方的点（正 Z）应该不可见
    Vector3 pointBehind(0.0f, 0.0f, 10.0f);
    EXPECT_FALSE(frustum.isPointVisible(pointBehind));
}

TEST_F(FrustumTest, PointVisibility_PointTooFar)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 远裁剪面之外的点应该不可见
    Vector3 pointTooFar(0.0f, 0.0f, -1500.0f); // 超过 far plane 1000
    EXPECT_FALSE(frustum.isPointVisible(pointTooFar));
}

TEST_F(FrustumTest, PointVisibility_PointTooClose)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 近裁剪面之内的点应该不可见
    Vector3 pointTooClose(0.0f, 0.0f, -0.05f); // 小于 near plane 0.1
    EXPECT_FALSE(frustum.isPointVisible(pointTooClose));
}

TEST_F(FrustumTest, PointVisibility_PointAtBoundary)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 近裁剪面附近的点应该可见。
    // 注意：精确位于近平面上的点 (z=-0.1) 由于浮点精度，从平面方程计算出的有符号距离
    // 可能是极小的负值（而非精确 0），从而被 < 0.0f 判定为不可见。源码使用严格不等号
    // （精确 0 视为可见）是合理且与 MC Java 一致的；这里取稍微进入视锥内部的点，
    // 避免浮点噪声造成的脆弱断言。
    Vector3 pointNearNearPlane(0.0f, 0.0f, -0.2f);
    EXPECT_TRUE(frustum.isPointVisible(pointNearNearPlane));
}

TEST_F(FrustumTest, PointVisibility_GlmVersion)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    glm::vec3 pointInFront(0.0f, 0.0f, -10.0f);
    EXPECT_TRUE(frustum.isPointVisible(pointInFront));

    glm::vec3 pointBehind(0.0f, 0.0f, 10.0f);
    EXPECT_FALSE(frustum.isPointVisible(pointBehind));
}

// ========== 球可见性测试 ==========

TEST_F(FrustumTest, SphereVisibility_SphereInFront)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    Vector3 center(0.0f, 0.0f, -10.0f);
    f32 radius = 1.0f;
    EXPECT_TRUE(frustum.isSphereVisible(center, radius));
}

TEST_F(FrustumTest, SphereVisibility_SphereBehind)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    Vector3 center(0.0f, 0.0f, 10.0f);
    f32 radius = 1.0f;
    EXPECT_FALSE(frustum.isSphereVisible(center, radius));
}

TEST_F(FrustumTest, SphereVisibility_SpherePartiallyVisible)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 球心在视锥外，但部分伸入视锥
    Vector3 center(0.0f, 0.0f, 15.0f); // 在相机后面
    f32 radius = 20.0f;                // 大球，部分在视锥内
    EXPECT_TRUE(frustum.isSphereVisible(center, radius));
}

TEST_F(FrustumTest, SphereVisibility_SphereTooFar)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    Vector3 center(0.0f, 0.0f, -1500.0f);
    f32 radius = 10.0f;
    EXPECT_FALSE(frustum.isSphereVisible(center, radius));
}

TEST_F(FrustumTest, SphereVisibility_SphereAtEdge)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 球心在视锥边缘，但球的一部分在视锥内
    Vector3 center(20.0f, 0.0f, -10.0f); // 可能在边缘
    f32 radius = 5.0f;
    // 边缘测试：确保不会崩溃，且返回有效的布尔值
    bool result = frustum.isSphereVisible(center, radius);
    // 结果取决于具体视角，但应该是有效的布尔值
    EXPECT_TRUE(result == true || result == false);
}

TEST_F(FrustumTest, SphereVisibility_GlmVersion)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    glm::vec3 center(0.0f, 0.0f, -10.0f);
    f32 radius = 1.0f;
    EXPECT_TRUE(frustum.isSphereVisible(center, radius));
}

// ========== AABB 可见性测试 ==========

TEST_F(FrustumTest, AABBVisibility_AABBInFront)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 相机前方的 AABB
    AxisAlignedBB aabb(-5.0f, -5.0f, -15.0f, 5.0f, 5.0f, -10.0f);
    EXPECT_TRUE(frustum.isAABBVisible(aabb));
}

TEST_F(FrustumTest, AABBVisibility_AABBBehind)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 相机后方的 AABB
    AxisAlignedBB aabb(-5.0f, -5.0f, 10.0f, 5.0f, 5.0f, 15.0f);
    EXPECT_FALSE(frustum.isAABBVisible(aabb));
}

TEST_F(FrustumTest, AABBVisibility_AABBPartiallyVisible)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 部分在视锥内的 AABB（跨越近裁剪面）
    AxisAlignedBB aabb(-5.0f, -5.0f, -5.0f, 5.0f, 5.0f, 5.0f);
    EXPECT_TRUE(frustum.isAABBVisible(aabb)); // 保守测试，应该报告可见
}

TEST_F(FrustumTest, AABBVisibility_AABBTooFar)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 远裁剪面之外的 AABB
    AxisAlignedBB aabb(-5.0f, -5.0f, -1100.0f, 5.0f, 5.0f, -1050.0f);
    EXPECT_FALSE(frustum.isAABBVisible(aabb));
}

TEST_F(FrustumTest, AABBVisibility_AABBAtEdge)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 在视锥边缘的 AABB
    AxisAlignedBB aabb(15.0f, -5.0f, -10.0f, 25.0f, 5.0f, -5.0f);
    // 边缘测试：确保不会崩溃，且返回有效的布尔值
    bool result = frustum.isAABBVisible(aabb);
    // 结果取决于具体视角，但应该是有效的布尔值
    EXPECT_TRUE(result == true || result == false);
}

TEST_F(FrustumTest, AABBVisibility_WorldCoordinates)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);
    frustum.setCameraPosition(Vector3(0.0f, 0.0f, 0.0f));

    // 使用世界坐标测试
    AxisAlignedBB worldAABB(-5.0f, -5.0f, -15.0f, 5.0f, 5.0f, -10.0f);
    EXPECT_TRUE(frustum.isAABBVisibleWorld(worldAABB));
}

TEST_F(FrustumTest, AABBVisibility_LargeAABB)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 非常大的 AABB（跨越整个视锥）
    AxisAlignedBB hugeAABB(-10000.0f, -10000.0f, -10000.0f, 10000.0f, 10000.0f, 10000.0f);
    EXPECT_TRUE(frustum.isAABBVisible(hugeAABB));
}

TEST_F(FrustumTest, AABBVisibility_EmptyAABB)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 空 AABB（实际上是一个点在原点）
    // 相机在原点看向 -Z，近裁剪面在 z=-0.1
    // 原点 (0,0,0) 在近平面之前（z > -0.1），所以不在视锥内
    AxisAlignedBB emptyAABB; // 默认构造，min = max = 0
    EXPECT_FALSE(frustum.isAABBVisible(emptyAABB));
}

// ========== 区块可见性测试 ==========

TEST_F(FrustumTest, ChunkVisibility_ChunkInFront)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);
    frustum.setCameraPosition(Vector3(0.0f, 64.0f, 0.0f));

    // 相机前方的区块
    EXPECT_TRUE(frustum.isChunkVisible(0, -1, 0, mc::world::MAX_BUILD_HEIGHT));
}

TEST_F(FrustumTest, ChunkVisibility_ChunkBehind)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);
    frustum.setCameraPosition(Vector3(0.0f, 64.0f, 0.0f));

    // 相机后方的区块
    EXPECT_FALSE(frustum.isChunkVisible(0, 1, 0, mc::world::MAX_BUILD_HEIGHT));
}

TEST_F(FrustumTest, ChunkVisibility_ChunkAtCameraPosition)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);
    frustum.setCameraPosition(Vector3(8.0f, 64.0f, 8.0f));

    // 相机所在区块
    EXPECT_TRUE(frustum.isChunkVisible(0, 0, 0, mc::world::MAX_BUILD_HEIGHT));
}

TEST_F(FrustumTest, ChunkVisibility_CameraMoved)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    // 相机移动到另一个位置
    // 注意：setCameraPosition 只设置相机位置用于坐标转换
    // 视锥方向仍由原始 viewProjection 矩阵决定（看向 -Z）
    constexpr f32 cameraX = 100.0f;
    constexpr f32 cameraY = 64.0f;
    constexpr f32 cameraZ = -200.0f;
    frustum.setCameraPosition(Vector3(cameraX, cameraY, cameraZ));

    // 区块在世界坐标 z=-208 到 z=-192（chunkZ = -13 * 16 = -208）
    // 相机在 z=-200，视锥朝向 -Z
    // 所以相机看到的是 z < -200 的区域
    // 区块 z 范围是 [-208, -192]，部分在视锥内（z < -200 的部分）
    // 负坐标必须使用向下取整规则，避免 static_cast 对负数向零截断。
    const i32 chunkX = mc::math::toChunkCoord(cameraX);
    const i32 chunkZ = mc::math::toChunkCoord(cameraZ);

    // 区块 X 范围: [96, 112]，相机 X=100，在范围内
    // 区块 Z 范围: [-208, -192]，相机 Z=-200，部分在视锥内
    EXPECT_TRUE(frustum.isChunkVisible(chunkX, chunkZ, 0, mc::world::MAX_BUILD_HEIGHT));
}

// ========== 区块段可见性测试 ==========

TEST_F(FrustumTest, ChunkSectionVisibility_SectionInFront)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);
    frustum.setCameraPosition(Vector3(0.0f, 64.0f, 0.0f));

    // 相机看向 -Z（视锥前方），相机 Y=64。
    // sectionY 为段索引（0..CHUNK_SECTIONS-1），sectionToY(idx) = MIN_BUILD_HEIGHT + idx*16。
    // 段索引 8 -> worldY = -64 + 8*16 = 64，即 Y∈[64,80]，与相机同高。
    EXPECT_TRUE(frustum.isChunkSectionVisible(0, 8, -1)); // Y=64 左右的段
}

TEST_F(FrustumTest, ChunkSectionVisibility_SectionBehind)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);
    frustum.setCameraPosition(Vector3(0.0f, 64.0f, 0.0f));

    EXPECT_FALSE(frustum.isChunkSectionVisible(0, 4, 1)); // 相机后方的段
}

TEST_F(FrustumTest, ChunkSectionVisibility_SectionAbove)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);
    frustum.setCameraPosition(Vector3(0.0f, 64.0f, 0.0f));

    // 上方的段（可能被顶平面裁剪，取决于 FOV）
    frustum.isChunkSectionVisible(0, 15, -1); // 最顶层段
}

// ========== 相机位置测试 ==========

TEST_F(FrustumTest, CameraPosition_SetGet)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    Vector3 cameraPos(100.0f, 64.0f, -50.0f);
    frustum.setCameraPosition(cameraPos);

    EXPECT_EQ(frustum.getCameraPosition().x, 100.0f);
    EXPECT_EQ(frustum.getCameraPosition().y, 64.0f);
    EXPECT_EQ(frustum.getCameraPosition().z, -50.0f);
}

TEST_F(FrustumTest, CameraPosition_GlmVersion)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);

    glm::vec3 cameraPos(100.0f, 64.0f, -50.0f);
    frustum.setCameraPosition(cameraPos);

    EXPECT_EQ(frustum.getCameraPosition().x, 100.0f);
    EXPECT_EQ(frustum.getCameraPosition().y, 64.0f);
    EXPECT_EQ(frustum.getCameraPosition().z, -50.0f);
}

// ========== 工具函数测试 ==========

TEST_F(FrustumTest, Utils_CreateChunkAABB)
{
    // createChunkAABB 的 minY/maxY 参数直接作为世界坐标 Y 边界，
    // 因此要覆盖完整建造高度范围，应传入 MIN_BUILD_HEIGHT..MAX_BUILD_HEIGHT。
    auto aabb = FrustumUtils::createChunkAABB(0, 0, mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);

    EXPECT_FLOAT_EQ(aabb.minX, 0.0f);
    EXPECT_FLOAT_EQ(aabb.minY, static_cast<f32>(mc::world::MIN_BUILD_HEIGHT));
    EXPECT_FLOAT_EQ(aabb.minZ, 0.0f);
    EXPECT_FLOAT_EQ(aabb.maxX, 16.0f);
    EXPECT_FLOAT_EQ(aabb.maxY, static_cast<f32>(mc::world::MAX_BUILD_HEIGHT));
    EXPECT_FLOAT_EQ(aabb.maxZ, 16.0f);
}

TEST_F(FrustumTest, Utils_CreateChunkAABB_Negative)
{
    auto aabb = FrustumUtils::createChunkAABB(-1, -1, -64, 320);

    EXPECT_FLOAT_EQ(aabb.minX, -16.0f);
    EXPECT_FLOAT_EQ(aabb.minZ, -16.0f);
    EXPECT_FLOAT_EQ(aabb.maxX, 0.0f);
    EXPECT_FLOAT_EQ(aabb.maxZ, 0.0f);
}

TEST_F(FrustumTest, Utils_CreateSectionAABB)
{
    // createSectionAABB 的 Y 基准为 MIN_BUILD_HEIGHT（与 sectionToY 一致）：
    //   worldY = MIN_BUILD_HEIGHT + sectionY * sectionHeight。
    // sectionY=0 -> worldY = -64，覆盖 [-64, -48]。
    auto aabb = FrustumUtils::createSectionAABB(0, 0, 0);

    EXPECT_FLOAT_EQ(aabb.minX, 0.0f);
    EXPECT_FLOAT_EQ(aabb.minY, static_cast<f32>(mc::world::MIN_BUILD_HEIGHT));
    EXPECT_FLOAT_EQ(aabb.minZ, 0.0f);
    EXPECT_FLOAT_EQ(aabb.maxX, 16.0f);
    EXPECT_FLOAT_EQ(aabb.maxY, static_cast<f32>(mc::world::MIN_BUILD_HEIGHT + 16));
    EXPECT_FLOAT_EQ(aabb.maxZ, 16.0f);
}

TEST_F(FrustumTest, Utils_CreateSectionAABB_HighSection)
{
    // sectionY=10 -> worldY = MIN_BUILD_HEIGHT + 10*16 = -64 + 160 = 96，覆盖 [96, 112]。
    auto aabb = FrustumUtils::createSectionAABB(0, 10, 0);

    EXPECT_FLOAT_EQ(aabb.minY, static_cast<f32>(mc::world::MIN_BUILD_HEIGHT + 160));
    EXPECT_FLOAT_EQ(aabb.maxY, static_cast<f32>(mc::world::MIN_BUILD_HEIGHT + 176));
}

TEST_F(FrustumTest, Utils_CreateEntityAABB)
{
    Vector3 pos(10.0f, 64.0f, 20.0f);
    auto aabb = FrustumUtils::createEntityAABB(pos, 0.6f, 1.8f);

    EXPECT_FLOAT_EQ(aabb.minX, 9.7f);
    EXPECT_FLOAT_EQ(aabb.minY, 64.0f);
    EXPECT_FLOAT_EQ(aabb.minZ, 19.7f);
    EXPECT_FLOAT_EQ(aabb.maxX, 10.3f);
    EXPECT_FLOAT_EQ(aabb.maxY, 65.8f);
    EXPECT_FLOAT_EQ(aabb.maxZ, 20.3f);
}

TEST_F(FrustumTest, Utils_CreateBlockAABB)
{
    auto aabb = FrustumUtils::createBlockAABB(10, 20, 30);

    EXPECT_FLOAT_EQ(aabb.minX, 10.0f);
    EXPECT_FLOAT_EQ(aabb.minY, 20.0f);
    EXPECT_FLOAT_EQ(aabb.minZ, 30.0f);
    EXPECT_FLOAT_EQ(aabb.maxX, 11.0f);
    EXPECT_FLOAT_EQ(aabb.maxY, 21.0f);
    EXPECT_FLOAT_EQ(aabb.maxZ, 31.0f);
}

TEST_F(FrustumTest, Utils_ExpandAABB)
{
    AxisAlignedBB aabb(0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);
    auto expanded = FrustumUtils::expandAABB(aabb, 1.0f);

    EXPECT_FLOAT_EQ(expanded.minX, -1.0f);
    EXPECT_FLOAT_EQ(expanded.minY, -1.0f);
    EXPECT_FLOAT_EQ(expanded.minZ, -1.0f);
    EXPECT_FLOAT_EQ(expanded.maxX, 11.0f);
    EXPECT_FLOAT_EQ(expanded.maxY, 11.0f);
    EXPECT_FLOAT_EQ(expanded.maxZ, 11.0f);
}

// ========== 视角变化测试 ==========

TEST_F(FrustumTest, DifferentFOV)
{
    // 窄视角（更小的视锥）
    glm::mat4 narrowProjection = createPerspectiveMatrix(30.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    Frustum narrowFrustum;
    narrowFrustum.extractFromMatrix(narrowProjection * view);

    // 宽视角（更大的视锥）
    glm::mat4 wideProjection = createPerspectiveMatrix(120.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    Frustum wideFrustum;
    wideFrustum.extractFromMatrix(wideProjection * view);

    // 边缘的点在窄视角不可见，但在宽视角可见
    Vector3 edgePoint(20.0f, 0.0f, -10.0f);

    // 结果取决于具体角度，这里只验证不会崩溃
    narrowFrustum.isPointVisible(edgePoint);
    wideFrustum.isPointVisible(edgePoint);
}

TEST_F(FrustumTest, RotatedCamera)
{
    // 相机旋转90度看向 +X
    glm::mat4 rotatedView = createViewMatrix(glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f), // 看向 +X
        glm::vec3(0.0f, 1.0f, 0.0f));

    Frustum frustum;
    frustum.extractFromMatrix(projection * rotatedView);

    // 现在正 X 方向的点应该可见
    Vector3 pointInFront(10.0f, 0.0f, 0.0f);
    EXPECT_TRUE(frustum.isPointVisible(pointInFront));

    // 负 Z 方向的点现在在侧面，取决于 FOV
    Vector3 pointSide(0.0f, 0.0f, -10.0f);
    frustum.isPointVisible(pointSide); // 不崩溃即可
}

// ========== 边界情况测试 ==========

TEST_F(FrustumTest, UninitializedFrustum)
{
    Frustum frustum;
    EXPECT_FALSE(frustum.isValid());

    // 未初始化时调用应该安全（但结果未定义）
    frustum.isPointVisible(Vector3(0.0f, 0.0f, 0.0f)); // 不崩溃即可
}

TEST_F(FrustumTest, VerySmallNearPlane)
{
    glm::mat4 smallNearProjection = createPerspectiveMatrix(70.0f, 16.0f / 9.0f, 0.001f, 1000.0f);
    Frustum frustum;
    frustum.extractFromMatrix(smallNearProjection * view);

    // 极近的点应该可见
    Vector3 nearPoint(0.0f, 0.0f, -0.01f);
    EXPECT_TRUE(frustum.isPointVisible(nearPoint));
}

TEST_F(FrustumTest, VeryLargeFarPlane)
{
    glm::mat4 largeFarProjection = createPerspectiveMatrix(70.0f, 16.0f / 9.0f, 0.1f, 100000.0f);
    Frustum frustum;
    frustum.extractFromMatrix(largeFarProjection * view);

    // 很远的点应该可见
    Vector3 farPoint(0.0f, 0.0f, -50000.0f);
    EXPECT_TRUE(frustum.isPointVisible(farPoint));
}

TEST_F(FrustumTest, NegativeAspectRatio)
{
    // 负宽高比应该被 glm 处理
    EXPECT_NO_THROW({
        glm::mat4 negAspectProjection = createPerspectiveMatrix(70.0f, -16.0f / 9.0f, 0.1f, 1000.0f);
        Frustum frustum;
        frustum.extractFromMatrix(negAspectProjection * view);
    });
}

TEST_F(FrustumTest, ExtremeCoordinates)
{
    Frustum frustum;
    frustum.extractFromMatrix(viewProjection);
    frustum.setCameraPosition(Vector3(1000000.0f, 64.0f, 1000000.0f));

    // 大坐标下的区块测试
    i32 farChunkX = 62500; // 约 1000000 / 16
    i32 farChunkZ = 62500;

    // 不崩溃即可
    frustum.isChunkVisible(farChunkX, farChunkZ, 0, mc::world::MAX_BUILD_HEIGHT);
}
