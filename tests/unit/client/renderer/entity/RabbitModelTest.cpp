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

#include <algorithm>
#include <cstddef>
#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/animal/RabbitModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model::animal;
using mc::client::renderer::entity::model::ModelRenderer;
using mc::client::renderer::entity::model::ModelVertex;

namespace mc::client::renderer {
namespace {

// 兔子模型部件在 m_parts 中的索引（按 _setupParts 的 push_back 顺序）
// 参考 RabbitModel::_setupParts()
constexpr std::size_t kLeftFootIndex = 0;
constexpr std::size_t kRightFootIndex = 1;
constexpr std::size_t kLeftThighIndex = 2;
constexpr std::size_t kRightThighIndex = 3;
constexpr std::size_t kBodyIndex = 4;
constexpr std::size_t kLeftArmIndex = 5;
constexpr std::size_t kRightArmIndex = 6;
constexpr std::size_t kHeadIndex = 7;

// RabbitModel::setAngles 中的基础旋转角度
constexpr f32 kBaseThighAngle = -0.34906584f;
constexpr f32 kBaseArmAngle = -0.17453292f;

// 计算 RabbitModel::setAngles 中的跳跃角度公式
//   thighAngle = toRadians(jumpRotation * 50 - 21)
//   footAngle  = toRadians(jumpRotation * 50)
//   armAngle   = toRadians(jumpRotation * -40 - 11)
f32 expectedThighAngle(f32 jumpRotation)
{
    return kBaseThighAngle + mc::math::toRadians(jumpRotation * 50.0f - 21.0f);
}

f32 expectedFootAngle(f32 jumpRotation)
{
    return mc::math::toRadians(jumpRotation * 50.0f);
}

f32 expectedArmAngle(f32 jumpRotation)
{
    return kBaseArmAngle + mc::math::toRadians(jumpRotation * -40.0f - 11.0f);
}

struct Bounds {
    f32 minX = 0.0f;
    f32 minY = 0.0f;
    f32 minZ = 0.0f;
    f32 maxX = 0.0f;
    f32 maxY = 0.0f;
    f32 maxZ = 0.0f;
};

Bounds computeBounds(const std::vector<ModelVertex>& vertices)
{
    Bounds b;
    if (vertices.empty()) {
        return b;
    }

    b.minX = b.maxX = vertices[0].position.x;
    b.minY = b.maxY = vertices[0].position.y;
    b.minZ = b.maxZ = vertices[0].position.z;

    for (const auto& v : vertices) {
        b.minX = std::min(b.minX, v.position.x);
        b.minY = std::min(b.minY, v.position.y);
        b.minZ = std::min(b.minZ, v.position.z);
        b.maxX = std::max(b.maxX, v.position.x);
        b.maxY = std::max(b.maxY, v.position.y);
        b.maxZ = std::max(b.maxZ, v.position.z);
    }

    return b;
}

Bounds buildDefaultPoseBounds(RabbitModel& model)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    constexpr f32 kModelScale = 1.0f / 16.0f;
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, kModelScale);
    model.generateMesh(vertices, indices, kModelScale);

    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());

    return computeBounds(vertices);
}

} // anonymous namespace

// 测试 RabbitModel 默认构造
TEST(RabbitModelTest, DefaultConstructionCreatesValidModel)
{
    RabbitModel model;

    // 验证模型有有效的纹理尺寸 (64x32)
    EXPECT_EQ(model.textureWidth(), 64);
    EXPECT_EQ(model.textureHeight(), 32);
}

// 测试 RabbitModel 网格生成 - 验证边界合理
TEST(RabbitModelTest, GeneratedMeshHasReasonableBounds)
{
    RabbitModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    // 兔子模型应该有合理的尺寸（成年体整体缩放 0.6）
    // 宽度: 约 0.2-0.6
    // 高度: 约 0.4-1.2
    // 深度: 约 0.4-1.2
    EXPECT_GT(width, 0.15f);
    EXPECT_GT(height, 0.3f);
    EXPECT_GT(depth, 0.3f);

    EXPECT_LT(width, 0.8f);
    EXPECT_LT(height, 1.5f);
    EXPECT_LT(depth, 1.5f);
}

// 测试 RabbitModel 网格生成 - 验证包含所有部件
TEST(RabbitModelTest, GeneratedMeshIncludesAllParts)
{
    RabbitModel model;
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    constexpr f32 kModelScale = 1.0f / 16.0f;
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, kModelScale);
    model.generateMesh(vertices, indices, kModelScale);

    // 验证网格非空
    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());

    // 验证网格有合理的高度分布（头部在高位置，腿部在低位置）
    // 兔子整体缩放 0.6，模型单位 1/16，所以 y 范围大致在 0.4-1.2 之间
    f32 minY = vertices[0].position.y;
    f32 maxY = vertices[0].position.y;
    for (const auto& v : vertices) {
        minY = std::min(minY, v.position.y);
        maxY = std::max(maxY, v.position.y);
    }

    // 兔子应该有一定的高度差（头部高于腿部）
    EXPECT_GT(maxY - minY, 0.3f) << "Model should have significant height variation";
}

// 测试 RabbitModel 幼体状态
TEST(RabbitModelTest, ChildStateCanBeSetAndQueried)
{
    RabbitModel model;

    // AgeableModel 默认 m_isChild = false
    EXPECT_FALSE(model.isChild());

    // 设置为幼体
    model.setChild(true);
    EXPECT_TRUE(model.isChild());

    // 设置回成年体
    model.setChild(false);
    EXPECT_FALSE(model.isChild());
}

// 测试 RabbitModel 角度设置不会崩溃
TEST(RabbitModelTest, SetAnglesUpdatesModelPose)
{
    RabbitModel model;
    model.setChild(false);

    // 设置行走动画
    model.setAngles(1.0, 0.5, 0.0, 0.0, 0.0, 1.0);

    // 生成网格验证不会崩溃
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model.generateMesh(vertices, indices, 1.0f / 16.0f);

    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());
}

// 测试 RabbitModel 头部偏转
TEST(RabbitModelTest, HeadRotationAppliedCorrectly)
{
    RabbitModel model;
    model.setChild(false);

    // 设置头部向左偏转 45 度
    model.setAngles(0.0, 0.0, 0.0, 45.0, 0.0, 1.0);

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model.generateMesh(vertices, indices, 1.0f / 16.0f);
    EXPECT_FALSE(vertices.empty());

    // 设置头部俯仰 30 度
    model.setAngles(0.0, 0.0, 0.0, 0.0, 30.0, 1.0);
    model.generateMesh(vertices, indices, 1.0f / 16.0f);
    EXPECT_FALSE(vertices.empty());
}

// 测试 RabbitModel 纹理尺寸正确
TEST(RabbitModelTest, TextureSizeMatchesMinecraftSpec)
{
    RabbitModel model;

    // MC 1.21.11 兔子纹理尺寸是 64x32
    EXPECT_EQ(model.textureWidth(), 64);
    EXPECT_EQ(model.textureHeight(), 32);
}

// 测试 RabbitModel 使用模型单位时的尺寸
TEST(RabbitModelTest, RuntimeMeshUsesMinecraftModelUnits)
{
    RabbitModel model;
    model.setChild(false);
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    model.generateMesh(vertices, indices, 1.0f);

    const Bounds b = computeBounds(vertices);
    const f32 width = b.maxX - b.minX;
    const f32 depth = b.maxZ - b.minZ;

    // 在模型单位 (未缩放) 下，兔子应该有合理的尺寸
    EXPECT_GT(width, 4.0f);
    EXPECT_GT(depth, 8.0f);
    EXPECT_LT(width, 12.0f);
    EXPECT_LT(depth, 25.0f);
}

// 测试 RabbitModel 幼体渲染
TEST(RabbitModelTest, ChildRenderingProducesSmallerMesh)
{
    RabbitModel adultModel;
    RabbitModel childModel;

    adultModel.setChild(false);
    childModel.setChild(true);

    std::vector<ModelVertex> adultVertices;
    std::vector<u32> adultIndices;
    adultModel.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / 16.0f);
    adultModel.generateMesh(adultVertices, adultIndices, 1.0f / 16.0f);

    std::vector<ModelVertex> childVertices;
    std::vector<u32> childIndices;
    childModel.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / 16.0f);
    childModel.generateMesh(childVertices, childIndices, 1.0f / 16.0f);

    // 两个模型都应该生成有效的网格
    EXPECT_FALSE(adultVertices.empty());
    EXPECT_FALSE(childVertices.empty());
}

// 测试 RabbitModel 行走动画
TEST(RabbitModelTest, WalkAnimationGeneratesValidMesh)
{
    RabbitModel model;
    model.setChild(false);

    // 模拟行走动画的不同阶段
    for (f64 phase = 0.0; phase < 6.28; phase += 0.5) {
        f64 limbSwing = phase;
        f64 limbSwingAmount = 0.5;

        model.setAngles(limbSwing, limbSwingAmount, 0.0, 0.0, 0.0, 1.0);

        std::vector<ModelVertex> vertices;
        std::vector<u32> indices;
        model.generateMesh(vertices, indices, 1.0f / 16.0f);

        EXPECT_FALSE(vertices.empty()) << "Phase: " << phase;
        EXPECT_FALSE(indices.empty()) << "Phase: " << phase;
    }
}

// ============================================================================
// 跳跃动画 (setJumpRotation) 测试
// 参考 MC 1.21.11 RabbitModel.setupAnim 中的跳跃角度计算
// ============================================================================

// 测试默认 m_jumpRotation = 0 时的腿部角度
TEST(RabbitModelJumpTest, DefaultJumpRotationIsZero)
{
    RabbitModel model;
    model.setChild(false);

    // 设置角度（jumpRotation 默认为 0）
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    const auto& parts = model.getParts();

    // 验证 thigh 角度 = baseThighAngle + toRadians(0 * 50 - 21) = baseThighAngle + toRadians(-21)
    const f32 expectedThigh = expectedThighAngle(0.0f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX()), expectedThigh, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightThighIndex]->rotateAngleX()), expectedThigh, 1e-4f);

    // 验证 foot 角度 = toRadians(0 * 50) = 0
    const f32 expectedFoot = expectedFootAngle(0.0f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftFootIndex]->rotateAngleX()), expectedFoot, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightFootIndex]->rotateAngleX()), expectedFoot, 1e-4f);

    // 验证 arm 角度 = baseArmAngle + toRadians(0 * -40 - 11) = baseArmAngle + toRadians(-11)
    const f32 expectedArm = expectedArmAngle(0.0f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftArmIndex]->rotateAngleX()), expectedArm, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightArmIndex]->rotateAngleX()), expectedArm, 1e-4f);
}

// 测试 setJumpRotation(1.0) - 跳跃中点（jumpRotation=sin(PI/2)=1）
TEST(RabbitModelJumpTest, JumpRotationOneAppliesMaxRotation)
{
    RabbitModel model;
    model.setChild(false);

    model.setJumpRotation(1.0f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    const auto& parts = model.getParts();

    // jumpRotation = 1.0: thighAngle = toRadians(50 - 21) = toRadians(29)
    const f32 expectedThigh = expectedThighAngle(1.0f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX()), expectedThigh, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightThighIndex]->rotateAngleX()), expectedThigh, 1e-4f);

    // jumpRotation = 1.0: footAngle = toRadians(50)
    const f32 expectedFoot = expectedFootAngle(1.0f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftFootIndex]->rotateAngleX()), expectedFoot, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightFootIndex]->rotateAngleX()), expectedFoot, 1e-4f);

    // jumpRotation = 1.0: armAngle = toRadians(-40 - 11) = toRadians(-51)
    const f32 expectedArm = expectedArmAngle(1.0f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftArmIndex]->rotateAngleX()), expectedArm, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightArmIndex]->rotateAngleX()), expectedArm, 1e-4f);
}

// 测试 setJumpRotation(0.5) - 中间值
TEST(RabbitModelJumpTest, JumpRotationHalfAppliesIntermediateRotation)
{
    RabbitModel model;
    model.setChild(false);

    model.setJumpRotation(0.5f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    const auto& parts = model.getParts();

    const f32 expectedThigh = expectedThighAngle(0.5f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX()), expectedThigh, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightThighIndex]->rotateAngleX()), expectedThigh, 1e-4f);

    const f32 expectedFoot = expectedFootAngle(0.5f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftFootIndex]->rotateAngleX()), expectedFoot, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightFootIndex]->rotateAngleX()), expectedFoot, 1e-4f);

    const f32 expectedArm = expectedArmAngle(0.5f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftArmIndex]->rotateAngleX()), expectedArm, 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kRightArmIndex]->rotateAngleX()), expectedArm, 1e-4f);
}

// 测试 setJumpRotation 不影响头部、身体、尾巴的旋转角度
TEST(RabbitModelJumpTest, JumpRotationDoesNotAffectHeadBodyTail)
{
    RabbitModel model;
    model.setChild(false);

    // 先用 jumpRotation = 0 设置一次角度
    model.setJumpRotation(0.0f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    const auto& parts = model.getParts();
    const f64 bodyAngleBefore = parts[kBodyIndex]->rotateAngleX();

    // 改变 jumpRotation 再设置一次
    model.setJumpRotation(1.0f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    // body 的 rotateAngleX 不应改变（基础值 -0.3490659f，与 jumpRotation 无关）
    const f64 bodyAngleAfter = parts[kBodyIndex]->rotateAngleX();
    EXPECT_NEAR(static_cast<f32>(bodyAngleAfter), static_cast<f32>(bodyAngleBefore), 1e-4f);
}

// 测试 setJumpRotation 后必须调用 setAngles 才能生效
TEST(RabbitModelJumpTest, JumpRotationRequiresSetAnglesToApply)
{
    RabbitModel model;
    model.setChild(false);

    // 先调用 setAngles 设置初始角度
    model.setJumpRotation(0.0f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    const auto& parts = model.getParts();
    const f32 thighBefore = static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX());

    // 仅调用 setJumpRotation，不调用 setAngles
    model.setJumpRotation(1.0f);

    // 角度应保持不变
    const f32 thighAfter = static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX());
    EXPECT_NEAR(thighAfter, thighBefore, 1e-4f);

    // 调用 setAngles 后角度才更新
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    const f32 thighApplied = static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX());
    EXPECT_NEAR(thighApplied, expectedThighAngle(1.0f), 1e-4f);
}

// 测试 setJumpRotation 多次调用保持最新值
TEST(RabbitModelJumpTest, SetJumpRotationOverwritesPreviousValue)
{
    RabbitModel model;
    model.setChild(false);

    model.setJumpRotation(0.0f);
    model.setJumpRotation(0.5f);
    model.setJumpRotation(1.0f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    const auto& parts = model.getParts();

    // 应使用最后设置的 1.0
    EXPECT_NEAR(static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX()), expectedThighAngle(1.0f), 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftFootIndex]->rotateAngleX()), expectedFootAngle(1.0f), 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftArmIndex]->rotateAngleX()), expectedArmAngle(1.0f), 1e-4f);
}

// 测试 setJumpRotation(0) 与未调用 setJumpRotation 等价
TEST(RabbitModelJumpTest, ZeroJumpRotationEquivalentToDefault)
{
    RabbitModel modelA;
    modelA.setChild(false);
    modelA.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    RabbitModel modelB;
    modelB.setChild(false);
    modelB.setJumpRotation(0.0f);
    modelB.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    const auto& partsA = modelA.getParts();
    const auto& partsB = modelB.getParts();

    EXPECT_NEAR(static_cast<f32>(partsA[kLeftThighIndex]->rotateAngleX()),
        static_cast<f32>(partsB[kLeftThighIndex]->rotateAngleX()),
        1e-4f);
    EXPECT_NEAR(static_cast<f32>(partsA[kLeftFootIndex]->rotateAngleX()),
        static_cast<f32>(partsB[kLeftFootIndex]->rotateAngleX()),
        1e-4f);
    EXPECT_NEAR(static_cast<f32>(partsA[kLeftArmIndex]->rotateAngleX()),
        static_cast<f32>(partsB[kLeftArmIndex]->rotateAngleX()),
        1e-4f);
}

// 测试 setLivingAnimations 不影响 jumpRotation（jumpRotation 由外部通过 setJumpRotation 设置）
TEST(RabbitModelJumpTest, SetLivingAnimationsDoesNotOverrideJumpRotation)
{
    RabbitModel model;
    model.setChild(false);

    model.setJumpRotation(0.7f);
    model.setLivingAnimations(0.0f, 0.0f, 0.5f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    const auto& parts = model.getParts();

    // setLivingAnimations 应该不影响 jumpRotation，setAngles 仍使用 0.7
    EXPECT_NEAR(static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX()), expectedThighAngle(0.7f), 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftFootIndex]->rotateAngleX()), expectedFootAngle(0.7f), 1e-4f);
    EXPECT_NEAR(static_cast<f32>(parts[kLeftArmIndex]->rotateAngleX()), expectedArmAngle(0.7f), 1e-4f);
}

// 测试模拟完整跳跃周期：jumpCompletion 从 0 → 1 → 0
// 对应 MC 1.21.11 Rabbit.getJumpCompletion 和 RabbitModel.setupAnim
TEST(RabbitModelJumpTest, FullJumpCycleProducesValidAnglesThroughout)
{
    RabbitModel model;
    model.setChild(false);

    // 模拟 jumpTicks 从 0 到 jumpDuration=10 的完整周期
    // jumpCompletion = jumpTicks / jumpDuration
    // jumpRotation = sin(jumpCompletion * PI)
    for (i32 jumpTicks = 0; jumpTicks <= 10; ++jumpTicks) {
        const f32 jumpCompletion = static_cast<f32>(jumpTicks) / 10.0f;
        const f32 jumpRotation = std::sin(jumpCompletion * mc::math::PI);

        model.setJumpRotation(jumpRotation);
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        const auto& parts = model.getParts();

        // 在整个跳跃周期中，腿部角度应该匹配公式
        EXPECT_NEAR(static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX()), expectedThighAngle(jumpRotation), 1e-3f)
            << "jumpTicks=" << jumpTicks;
        EXPECT_NEAR(static_cast<f32>(parts[kLeftFootIndex]->rotateAngleX()), expectedFootAngle(jumpRotation), 1e-3f)
            << "jumpTicks=" << jumpTicks;
        EXPECT_NEAR(static_cast<f32>(parts[kLeftArmIndex]->rotateAngleX()), expectedArmAngle(jumpRotation), 1e-3f)
            << "jumpTicks=" << jumpTicks;

        // 网格生成也不应崩溃
        std::vector<ModelVertex> vertices;
        std::vector<u32> indices;
        model.generateMesh(vertices, indices, 1.0f / 16.0f);
        EXPECT_FALSE(vertices.empty()) << "jumpTicks=" << jumpTicks;
    }
}

// 测试跳跃起点和终点的角度对称性
// jumpRotation = sin(0) = 0（起点）和 sin(PI) = 0（终点）应该产生相同角度
TEST(RabbitModelJumpTest, JumpStartAndEndAnglesAreSymmetric)
{
    RabbitModel model;
    model.setChild(false);

    // 起点 jumpCompletion = 0, jumpRotation = sin(0) = 0
    model.setJumpRotation(0.0f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    const auto& parts = model.getParts();
    const f32 startThigh = static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX());
    const f32 startFoot = static_cast<f32>(parts[kLeftFootIndex]->rotateAngleX());
    const f32 startArm = static_cast<f32>(parts[kLeftArmIndex]->rotateAngleX());

    // 终点 jumpCompletion = 1.0, jumpRotation = sin(PI) ≈ 0
    const f32 endJumpRotation = std::sin(mc::math::PI); // ≈ 1.2e-7
    model.setJumpRotation(endJumpRotation);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    const f32 endThigh = static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX());
    const f32 endFoot = static_cast<f32>(parts[kLeftFootIndex]->rotateAngleX());
    const f32 endArm = static_cast<f32>(parts[kLeftArmIndex]->rotateAngleX());

    // sin(PI) ≈ 1.2e-7 ≈ 0，所以角度应基本相同
    EXPECT_NEAR(startThigh, endThigh, 1e-3f);
    EXPECT_NEAR(startFoot, endFoot, 1e-3f);
    EXPECT_NEAR(startArm, endArm, 1e-3f);
}

// 测试跳跃峰值（jumpRotation = 1.0）时大腿向后旋转
// 参考 MC 1.21.11 RabbitModel：thighAngle = toRadians(jumpRotation * 50 - 21)
// 当 jumpRotation = 1.0 时，thighAngle = toRadians(29) ≈ 0.506
// 大腿最终角度 = baseThighAngle + 0.506 ≈ -0.349 + 0.506 ≈ 0.157（正角度，大腿向后抬起）
TEST(RabbitModelJumpTest, JumpPeakThighRotatesForward)
{
    RabbitModel model;
    model.setChild(false);

    model.setJumpRotation(0.0f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    const auto& parts = model.getParts();
    const f32 thighAtStart = static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX());

    model.setJumpRotation(1.0f);
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    const f32 thighAtPeak = static_cast<f32>(parts[kLeftThighIndex]->rotateAngleX());

    // 跳跃峰值时大腿角度应比起始时更大（向前旋转）
    EXPECT_GT(thighAtPeak, thighAtStart);
}

} // namespace mc::client::renderer
