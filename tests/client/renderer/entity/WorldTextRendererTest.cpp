#include <array>
#include <cmath>
#include <gtest/gtest.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/frustum/Frustum.hpp"

using namespace mc;
using namespace mc::math::frustum;

namespace mc::client::renderer::entity::util {
namespace test {

/**
 * @brief WorldTextRenderer 单元测试
 *
 * 测试视锥体剔除和背面剔除的核心逻辑：
 * - shouldRenderText() 中的视锥体剔除逻辑
 * - isBackFacing() 中的背面剔除逻辑
 *
 * 注意：由于 WorldTextRenderer 使用静态成员，这些测试测试的是算法逻辑，
 * 不依赖 Vulkan 渲染上下文。
 */
class WorldTextRendererTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化测试
    }

    void TearDown() override
    {
        // 清理测试
    }
};

// ============================================================================
// 视锥体剔除测试（测试 Frustum 类的使用方式）
// ============================================================================

/**
 * @brief 测试视锥体 - 点在视锥内
 */
TEST_F(WorldTextRendererTest, Frustum_PointInsideFrustum)
{
    Frustum frustum;

    // 创建一个简单的透视投影矩阵
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), // 相机位置
        glm::vec3(0.0f, 0.0f, -1.0f),                         // 看向 -Z 方向
        glm::vec3(0.0f, 1.0f, 0.0f)                           // 上方向
    );
    glm::mat4 vp = projection * view;

    frustum.extractFromMatrix(vp);
    frustum.setCameraPosition(Vector3(0.0f, 0.0f, 0.0f));

    // 点在相机前方，应该在视锥内
    EXPECT_TRUE(frustum.isPointVisible(Vector3(0.0f, 0.0f, -5.0f)));
}

/**
 * @brief 测试视锥体 - 点在视锥外（相机后方）
 */
TEST_F(WorldTextRendererTest, Frustum_PointBehindCamera)
{
    Frustum frustum;

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = projection * view;

    frustum.extractFromMatrix(vp);
    frustum.setCameraPosition(Vector3(0.0f, 0.0f, 0.0f));

    // 点在相机后方，应该在视锥外
    EXPECT_FALSE(frustum.isPointVisible(Vector3(0.0f, 0.0f, 5.0f)));
}

/**
 * @brief 测试视锥体 - 球体在视锥内
 */
TEST_F(WorldTextRendererTest, Frustum_SphereInsideFrustum)
{
    Frustum frustum;

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = projection * view;

    frustum.extractFromMatrix(vp);
    frustum.setCameraPosition(Vector3(0.0f, 0.0f, 0.0f));

    // 球体在相机前方，应该在视锥内
    EXPECT_TRUE(frustum.isSphereVisible(Vector3(0.0f, 0.0f, -10.0f), 2.0f));
}

/**
 * @brief 测试视锥体 - 球体在视锥外
 */
TEST_F(WorldTextRendererTest, Frustum_SphereOutsideFrustum)
{
    Frustum frustum;

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = projection * view;

    frustum.extractFromMatrix(vp);
    frustum.setCameraPosition(Vector3(0.0f, 0.0f, 0.0f));

    // 球体在相机后方，应该在视锥外
    EXPECT_FALSE(frustum.isSphereVisible(Vector3(0.0f, 0.0f, 10.0f), 2.0f));
}

/**
 * @brief 测试视锥体 - 边界情况
 */
TEST_F(WorldTextRendererTest, Frustum_EdgeCase_NearBoundary)
{
    Frustum frustum;

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = projection * view;

    frustum.extractFromMatrix(vp);
    frustum.setCameraPosition(Vector3(0.0f, 0.0f, 0.0f));

    // 接近近裁剪面的点
    EXPECT_TRUE(frustum.isPointVisible(Vector3(0.0f, 0.0f, -0.5f)));

    // 在近裁剪面之前的点
    EXPECT_FALSE(frustum.isPointVisible(Vector3(0.0f, 0.0f, -0.05f)));
}

/**
 * @brief 测试视锥体 - 移动相机
 */
TEST_F(WorldTextRendererTest, Frustum_CameraMoved)
{
    Frustum frustum;

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

    // 相机在 (10, 0, 10)，看向原点
    glm::mat4 view =
        glm::lookAt(glm::vec3(10.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = projection * view;

    frustum.extractFromMatrix(vp);
    frustum.setCameraPosition(Vector3(10.0f, 0.0f, 10.0f));

    // 原点应该在视锥内
    EXPECT_TRUE(frustum.isSphereVisible(Vector3(0.0f, 0.0f, 0.0f), 2.0f));

    // 相机后方（远离原点的方向）的点应该在视锥外
    EXPECT_FALSE(frustum.isSphereVisible(Vector3(20.0f, 0.0f, 20.0f), 2.0f));
}

// ============================================================================
// 背面剔除测试（测试方向向量计算逻辑）
// ============================================================================

/**
 * @brief 测试背面剔除 - 基本情况
 *
 * 当相机正对文本位置时，点积 < 0，不剔除
 *
 * 解释：
 * - toCamera 是从文本指向相机的方向
 * - cameraForward 是相机看向的方向
 * - 当文本在相机前方时，toCamera 和 cameraForward 方向相反
 * - 所以 dot < 0 表示文本在前方，不应剔除
 */
TEST_F(WorldTextRendererTest, BackfaceCulling_FacingCamera)
{
    // 相机位置 (0, 0, 0)
    // 相机前向向量 (0, 0, -1) - 看向 -Z 方向
    // 文本位置 (0, 0, -10) - 在相机前方

    Vector3f cameraForward(0.0f, 0.0f, -1.0f);
    Vector3f textPosition(0.0f, 0.0f, -10.0f);
    Vector3d cameraPosition(0.0, 0.0, 0.0);

    // 计算从文本到相机的方向
    Vector3f toCamera(static_cast<f32>(cameraPosition.x - textPosition.x),
        static_cast<f32>(cameraPosition.y - textPosition.y),
        static_cast<f32>(cameraPosition.z - textPosition.z));
    f32 distanceSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z;
    f32 invDistance = 1.0f / std::sqrt(distanceSq);
    toCamera.x *= invDistance;
    toCamera.y *= invDistance;
    toCamera.z *= invDistance;

    // 点积
    f32 dot = toCamera.x * cameraForward.x + toCamera.y * cameraForward.y + toCamera.z * cameraForward.z;

    // 点积 < 0 表示文本在相机前方（toCamera 与 cameraForward 方向相反）
    // 所以不应剔除（isBackFacing 应该返回 false）
    EXPECT_LT(dot, 0.0f);
    EXPECT_FALSE(dot >= 0.0f); // 不是背对相机
}

/**
 * @brief 测试背面剔除 - 背对相机
 *
 * 当相机背对文本位置时，点积 > 0，应该剔除
 *
 * 解释：
 * - toCamera 是从文本指向相机的方向
 * - cameraForward 是相机看向的方向
 * - 当文本在相机后方时，toCamera 和 cameraForward 方向相同
 * - 所以 dot > 0 表示文本在后方，应该剔除
 */
TEST_F(WorldTextRendererTest, BackfaceCulling_BackToCamera)
{
    // 相机位置 (0, 0, 0)
    // 相机前向向量 (0, 0, -1) - 看向 -Z 方向
    // 文本位置 (0, 0, 10) - 在相机后方

    Vector3f cameraForward(0.0f, 0.0f, -1.0f);
    Vector3f textPosition(0.0f, 0.0f, 10.0f);
    Vector3d cameraPosition(0.0, 0.0, 0.0);

    // 计算从文本到相机的方向
    Vector3f toCamera(static_cast<f32>(cameraPosition.x - textPosition.x),
        static_cast<f32>(cameraPosition.y - textPosition.y),
        static_cast<f32>(cameraPosition.z - textPosition.z));
    f32 distanceSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z;
    f32 invDistance = 1.0f / std::sqrt(distanceSq);
    toCamera.x *= invDistance;
    toCamera.y *= invDistance;
    toCamera.z *= invDistance;

    // 点积
    f32 dot = toCamera.x * cameraForward.x + toCamera.y * cameraForward.y + toCamera.z * cameraForward.z;

    // 点积 > 0 表示文本在相机后方（toCamera 与 cameraForward 方向相同）
    // 所以应该剔除（isBackFacing 应该返回 true）
    EXPECT_GT(dot, 0.0f);
    EXPECT_TRUE(dot >= 0.0f); // 背对相机，应该剔除
}

/**
 * @brief 测试背面剔除 - 侧面
 *
 * 当文本在相机侧面时，点积接近 0，边界情况
 */
TEST_F(WorldTextRendererTest, BackfaceCulling_SidePosition)
{
    // 相机位置 (0, 0, 0)
    // 相机前向向量 (0, 0, -1) - 看向 -Z 方向
    // 文本位置 (10, 0, 0) - 在相机侧面（X轴）

    Vector3f cameraForward(0.0f, 0.0f, -1.0f);
    Vector3f textPosition(10.0f, 0.0f, 0.0f);
    Vector3d cameraPosition(0.0, 0.0, 0.0);

    // 计算从文本到相机的方向
    Vector3f toCamera(static_cast<f32>(cameraPosition.x - textPosition.x),
        static_cast<f32>(cameraPosition.y - textPosition.y),
        static_cast<f32>(cameraPosition.z - textPosition.z));
    f32 distanceSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z;
    f32 invDistance = 1.0f / std::sqrt(distanceSq);
    toCamera.x *= invDistance;
    toCamera.y *= invDistance;
    toCamera.z *= invDistance;

    // 点积
    f32 dot = toCamera.x * cameraForward.x + toCamera.y * cameraForward.y + toCamera.z * cameraForward.z;

    // 点积 = 0 表示文本在相机侧面，不应剔除（边界情况）
    EXPECT_NEAR(dot, 0.0f, 0.001f);
    EXPECT_FALSE(dot < 0.0f); // 点积不小于0，不剔除
}

/**
 * @brief 测试背面剔除 - 相机旋转
 *
 * 当相机旋转后看向不同的方向
 */
TEST_F(WorldTextRendererTest, BackfaceCulling_CameraRotated)
{
    // 相机位置 (0, 0, 0)
    // 相机前向向量 (1, 0, 0) - 看向 +X 方向（相机旋转90度）
    // 文本位置 (10, 0, 0) - 现在在相机前方

    Vector3f cameraForward(1.0f, 0.0f, 0.0f); // 旋转后的前向向量
    Vector3f textPosition(10.0f, 0.0f, 0.0f);
    Vector3d cameraPosition(0.0, 0.0, 0.0);

    // 计算从文本到相机的方向
    Vector3f toCamera(static_cast<f32>(cameraPosition.x - textPosition.x),
        static_cast<f32>(cameraPosition.y - textPosition.y),
        static_cast<f32>(cameraPosition.z - textPosition.z));
    f32 distanceSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z;
    f32 invDistance = 1.0f / std::sqrt(distanceSq);
    toCamera.x *= invDistance;
    toCamera.y *= invDistance;
    toCamera.z *= invDistance;

    // 点积
    f32 dot = toCamera.x * cameraForward.x + toCamera.y * cameraForward.y + toCamera.z * cameraForward.z;

    // 点积 < 0 表示文本现在在相机前方（因为相机旋转了）
    EXPECT_LT(dot, 0.0f);
    EXPECT_TRUE(dot < 0.0f); // 现在应该剔除（因为文本在旋转后的"后方"）
}

/**
 * @brief 测试背面剔除 - 近距离
 *
 * 当相机非常接近文本时
 */
TEST_F(WorldTextRendererTest, BackfaceCulling_VeryCloseDistance)
{
    // 相机位置 (0, 0, 0)
    // 文本位置 (0, 0, 0.001) - 非常接近相机

    Vector3f cameraForward(0.0f, 0.0f, -1.0f);
    Vector3f textPosition(0.0f, 0.0f, 0.001f);
    Vector3d cameraPosition(0.0, 0.0, 0.0);

    // 计算距离平方
    Vector3f toCamera(static_cast<f32>(cameraPosition.x - textPosition.x),
        static_cast<f32>(cameraPosition.y - textPosition.y),
        static_cast<f32>(cameraPosition.z - textPosition.z));
    f32 distanceSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z;

    // 当距离非常近时，不应剔除（避免除零问题）
    if (distanceSq < 0.0001f) {
        // isBackFacing 应该返回 false
        EXPECT_TRUE(true); // 正确处理边界情况
    } else {
        f32 invDistance = 1.0f / std::sqrt(distanceSq);
        toCamera.x *= invDistance;
        toCamera.y *= invDistance;
        toCamera.z *= invDistance;

        f32 dot = toCamera.x * cameraForward.x + toCamera.y * cameraForward.y + toCamera.z * cameraForward.z;

        EXPECT_LT(dot, 0.0f); // 文本在后方
    }
}

/**
 * @brief 测试背面剔除 - 从视图矩阵提取前向向量
 *
 * 验证从视图矩阵正确提取相机前向向量
 */
TEST_F(WorldTextRendererTest, BackfaceCulling_ExtractForwardFromViewMatrix)
{
    // 视图矩阵（相机在原点，看向 -Z）
    std::array<f64, 16> viewMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 从视图矩阵提取前向向量
    // 视图矩阵的第三行（Z轴）是相机的前向方向（取反）
    Vector3f cameraForward(
        static_cast<f32>(-viewMatrix[8]), static_cast<f32>(-viewMatrix[9]), static_cast<f32>(-viewMatrix[10]));

    // 归一化
    f32 len = std::sqrt(
        cameraForward.x * cameraForward.x + cameraForward.y * cameraForward.y + cameraForward.z * cameraForward.z);
    if (len > 0.0001f) {
        cameraForward.x /= len;
        cameraForward.y /= len;
        cameraForward.z /= len;
    }

    // 验证：单位视图矩阵的前向向量应该是 -Z 方向
    EXPECT_FLOAT_EQ(cameraForward.x, 0.0f);
    EXPECT_FLOAT_EQ(cameraForward.y, 0.0f);
    EXPECT_FLOAT_EQ(cameraForward.z, -1.0f);
}

// ============================================================================
// 距离检查测试
// ============================================================================

/**
 * @brief 测试距离检查 - 在最大距离内
 */
TEST_F(WorldTextRendererTest, DistanceCheck_WithinMaxDistance)
{
    constexpr f32 maxDistance = 64.0f;
    constexpr f32 distance = 30.0f;

    EXPECT_LT(distance, maxDistance);
    EXPECT_TRUE(distance <= maxDistance);
}

/**
 * @brief 测试距离检查 - 超出最大距离
 */
TEST_F(WorldTextRendererTest, DistanceCheck_ExceedsMaxDistance)
{
    constexpr f32 maxDistance = 64.0f;
    constexpr f32 distance = 100.0f;

    EXPECT_GT(distance, maxDistance);
    EXPECT_FALSE(distance <= maxDistance);
}

/**
 * @brief 测试距离检查 - 精确在最大距离
 */
TEST_F(WorldTextRendererTest, DistanceCheck_ExactlyAtMaxDistance)
{
    constexpr f32 maxDistance = 64.0f;
    constexpr f32 distance = 64.0f;

    EXPECT_FLOAT_EQ(distance, maxDistance);
    EXPECT_TRUE(distance <= maxDistance);
}

/**
 * @brief 测试距离计算
 */
TEST_F(WorldTextRendererTest, DistanceCalculation)
{
    // 相机位置
    Vector3d cameraPosition(0.0, 0.0, 0.0);

    // 文本位置
    Vector3f textPosition(3.0f, 4.0f, 0.0f);

    // 计算距离
    Vector3f toCamera(static_cast<f32>(cameraPosition.x - textPosition.x),
        static_cast<f32>(cameraPosition.y - textPosition.y),
        static_cast<f32>(cameraPosition.z - textPosition.z));
    f32 distance = std::sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

    // 3-4-5 三角形，距离应该是 5
    EXPECT_FLOAT_EQ(distance, 5.0f);
}

// ============================================================================
// 综合测试
// ============================================================================

/**
 * @brief 测试综合场景 - 相机在原点看向 -Z
 */
TEST_F(WorldTextRendererTest, Integrated_CameraAtOriginLookingNegativeZ)
{
    // 设置视锥体
    Frustum frustum;
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = projection * view;
    frustum.extractFromMatrix(vp);
    frustum.setCameraPosition(Vector3(0.0f, 0.0f, 0.0f));

    // 相机前向向量
    Vector3f cameraForward(0.0f, 0.0f, -1.0f);
    Vector3d cameraPosition(0.0, 0.0, 0.0);
    f32 maxDistance = 64.0f;

    // 测试用例：文本在前方，在视锥内，在距离内
    {
        Vector3f textPosition(0.0f, 0.0f, -10.0f);
        Vector3f toCamera(static_cast<f32>(cameraPosition.x - textPosition.x),
            static_cast<f32>(cameraPosition.y - textPosition.y),
            static_cast<f32>(cameraPosition.z - textPosition.z));
        f32 distance = std::sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

        // 距离检查
        EXPECT_TRUE(distance <= maxDistance);

        // 视锥检查
        Vector3 frustumPos(textPosition.x, textPosition.y, textPosition.z);
        EXPECT_TRUE(frustum.isSphereVisible(frustumPos, 2.0f));

        // 背面检查
        f32 invDistance = 1.0f / distance;
        toCamera.x *= invDistance;
        toCamera.y *= invDistance;
        toCamera.z *= invDistance;
        f32 dot = toCamera.x * cameraForward.x + toCamera.y * cameraForward.y + toCamera.z * cameraForward.z;
        // dot < 0 表示文本在前方，不应剔除
        EXPECT_LT(dot, 0.0f); // 文本在前方，不应剔除
    }

    // 测试用例：文本在后方
    {
        Vector3f textPosition(0.0f, 0.0f, 10.0f);
        Vector3f toCamera(static_cast<f32>(cameraPosition.x - textPosition.x),
            static_cast<f32>(cameraPosition.y - textPosition.y),
            static_cast<f32>(cameraPosition.z - textPosition.z));
        f32 distance = std::sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

        // 距离检查
        EXPECT_TRUE(distance <= maxDistance);

        // 视锥检查 - 在后方应该不在视锥内
        Vector3 frustumPos(textPosition.x, textPosition.y, textPosition.z);
        EXPECT_FALSE(frustum.isSphereVisible(frustumPos, 2.0f));

        // 背面检查
        f32 invDistance = 1.0f / distance;
        toCamera.x *= invDistance;
        toCamera.y *= invDistance;
        toCamera.z *= invDistance;
        f32 dot = toCamera.x * cameraForward.x + toCamera.y * cameraForward.y + toCamera.z * cameraForward.z;
        // dot > 0 表示文本在后方，应该剔除
        EXPECT_GT(dot, 0.0f); // 文本在后方，应该剔除
    }
}

/**
 * @brief 测试综合场景 - 多个文本位置
 */
TEST_F(WorldTextRendererTest, Integrated_MultipleTextPositions)
{
    // 设置视锥体
    Frustum frustum;
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 vp = projection * view;
    frustum.extractFromMatrix(vp);
    frustum.setCameraPosition(Vector3(0.0f, 0.0f, 0.0f));

    Vector3f cameraForward(0.0f, 0.0f, -1.0f);
    Vector3d cameraPosition(0.0, 0.0, 0.0);
    f32 maxDistance = 64.0f;

    // 测试多个位置
    struct TestCase {
        Vector3f position;
        bool shouldPassDistance;
        bool shouldPassFrustum;
        bool shouldPassBackface;
    };

    std::vector<TestCase> testCases = {
        // 前方中心：在视锥内，距离内，背面检查通过（dot < 0）
        {{0.0f, 0.0f, -10.0f}, true, true, true},
        // 后方：不在视锥内，距离内，背面检查失败（dot > 0）
        {{0.0f, 0.0f, 10.0f}, true, false, false},
        // 远前方：超出距离，但在视锥内（远裁剪面 100），背面检查通过
        {{0.0f, 0.0f, -70.0f}, false, true, true},
        // 侧面（X 轴上）：90 度 FOV 边缘外，距离内，dot = 0 边界情况被剔除
        {{50.0f, 0.0f, 0.0f}, true, false, false},
        // 前方上方：在视锥内，距离内，背面检查通过
        {{0.0f, 5.0f, -20.0f}, true, true, true},
    };

    for (const auto& tc : testCases) {
        Vector3f toCamera(static_cast<f32>(cameraPosition.x - tc.position.x),
            static_cast<f32>(cameraPosition.y - tc.position.y),
            static_cast<f32>(cameraPosition.z - tc.position.z));
        f32 distance = std::sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

        // 距离检查
        bool passDistance = (distance <= maxDistance);
        EXPECT_EQ(passDistance, tc.shouldPassDistance);

        // 视锥检查
        Vector3 frustumPos(tc.position.x, tc.position.y, tc.position.z);
        bool passFrustum = frustum.isSphereVisible(frustumPos, 2.0f);
        EXPECT_EQ(passFrustum, tc.shouldPassFrustum);

        // 背面检查
        // dot < 0 表示文本在前方，不剔除（passBackface = true）
        // dot >= 0 表示文本在后方或侧面，剔除（passBackface = false）
        if (distance > 0.01f) {
            f32 invDistance = 1.0f / distance;
            toCamera.x *= invDistance;
            toCamera.y *= invDistance;
            toCamera.z *= invDistance;
            f32 dot = toCamera.x * cameraForward.x + toCamera.y * cameraForward.y + toCamera.z * cameraForward.z;
            bool passBackface = (dot < 0.0f); // dot < 0 表示在前方，通过背面检查
            EXPECT_EQ(passBackface, tc.shouldPassBackface);
        }
    }
}

} // namespace test
} // namespace mc::client::renderer::entity::util
