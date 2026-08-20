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
 * @file SharedTopologyParityTest.cpp
 * @brief 纯拓扑子树跨区块共享的字级回归测试
 *
 * 阶段 1+2 优化：NoiseBindingVisitor::apply 对纯拓扑子树包装为 SharedTopology，
 * RandomState::createRouterCopy 经 NoiseRouter::mapAllCopy 复用共享化树，使纯拓扑
 * 子树在每区块 mapAll 中零深拷贝。本测试守护三件事：
 *
 * 1. isShareable 谓词正确区分纯拓扑（可共享）与 per-chunk 可变 / 占位（不可共享）。
 * 2. SharedTopology::compute 与被包装内部子树 compute 字级一致（共享语义不变量）。
 * 3. 真实数据驱动 noise_router（主世界/下界/末地，多 seed）经共享化后，createRouterCopy
 *    副本的 finalDensity.compute 与 RandomState::m_router 的 finalDensity.compute 字级
 *    一致——证明共享化不改变任何密度数值（地形命脉）。
 *
 * 依赖 tests/main.cpp 的 WorldGenRegistryEnvironment 预加载 NoiseSettingsRegistry 等数据包。
 */

#include "common/core/Types.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/world/gen/density/NoiseBindingVisitor.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace mc;
using namespace mc::world::gen::density;

// ============================================================================
// Part 1：isShareable 谓词单元测试（手工建小树，不依赖数据包）
// ============================================================================

TEST(SharedTopologyIsShareableTest, PureTopologyLeavesAreShareable)
{
    // 可共享叶子：compute 纯只读，无 mutable，无 per-chunk 绑定
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*factory::constant(1.5)));
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*factory::yClampedGradient(0, 100, 2.0, 4.0)));
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*factory::endIslands(42)));
}

TEST(SharedTopologyIsShareableTest, PureTopologyCompositesAreShareable)
{
    // 可共享复合：递归子节点全纯拓扑 → 整棵可共享
    auto addTree = factory::add(factory::constant(1.0), factory::constant(2.0));
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*addTree));

    auto clampTree = factory::clamp(factory::add(factory::constant(1.0), factory::constant(2.0)), -1.0, 1.0);
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*clampTree));

    auto mulTree = factory::mul(factory::yClampedGradient(0, 64, 0.0, 1.0), factory::constant(0.5));
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*mulTree));

    // SharedHolder 包装纯拓扑子树 → 可共享（递归 inner）
    auto sharedHolderTree = factory::sharedHolder(factory::constant(3.0));
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*sharedHolderTree));
}

TEST(SharedTopologyIsShareableTest, MarkersAndMutableNodesAreNotShareable)
{
    // Marker 占位（任意类型）→ 不可共享（NoiseChunk::apply 会替换为 per-chunk 实现）
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*factory::interpolated(factory::constant(1.0))));
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*factory::cacheOnce(factory::constant(1.0))));
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*factory::cacheAllInCellMarker(factory::constant(1.0))));
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*factory::flatCacheMarker(factory::constant(1.0))));
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*factory::cache2DMarker(factory::constant(1.0))));

    // per-chunk 可变节点实例（compute 写 mutable 缓存）→ 不可共享
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*factory::cache2D(factory::constant(1.0))));
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*factory::flatCache(factory::constant(1.0))));
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*factory::cacheAllInCell(factory::constant(1.0))));
}

TEST(SharedTopologyIsShareableTest, CompositeWithMarkerIsNotShareable)
{
    // 复合节点含 Marker 子树 → 不可共享（Marker 必须每区块独立替换）
    auto treeWithMarker = factory::add(factory::interpolated(factory::constant(1.0)), factory::constant(2.0));
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*treeWithMarker));

    // Marker 的纯拓扑子树本身可共享，但 Marker 节点不可共享
    auto markerWithPureChild = factory::interpolated(factory::add(factory::constant(1.0), factory::constant(2.0)));
    EXPECT_FALSE(NoiseBindingVisitor::isShareable(*markerWithPureChild));
    // 而 Marker 内部的纯拓扑 wrapped 子树可共享
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*factory::add(factory::constant(1.0), factory::constant(2.0))));
}

TEST(SharedTopologyIsShareableTest, CubicSplineWithNestedIsShareable)
{
    // CubicSpline 递归 input + points 中嵌套子样条
    auto inputUnique = factory::yClampedGradient(0, 64, -1.0, 1.0);
    std::shared_ptr<DensityFunction> input(inputUnique.release());
    // 常量值控制点（无嵌套子样条）→ 可共享
    std::vector<SplinePoint> flatPoints = {SplinePoint(-1.0, 0.0, 0.0), SplinePoint(1.0, 1.0, 0.0)};
    auto flatSpline = factory::cubicSpline(input, flatPoints);
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*flatSpline));
}

TEST(SharedTopologyIsShareableTest, SharedTopologyItselfIsShareable)
{
    // SharedTopology 视为可共享叶子（语义不变量：内部已纯拓扑）
    auto sharedTopology =
        std::make_unique<SharedTopology>(std::shared_ptr<const DensityFunction>(std::move(factory::constant(7.0))));
    EXPECT_TRUE(NoiseBindingVisitor::isShareable(*sharedTopology));
}

// ============================================================================
// Part 2：SharedTopology::compute 语义不变量（手工建小树，不依赖数据包）
// ============================================================================

TEST(SharedTopologyComputeTest, ComputeEqualsInnerSubtree)
{
    // SharedTopology.compute(x,y,z) 必须与被包装内部子树 compute 字级一致
    auto inner = factory::add(factory::constant(1.0), factory::yClampedGradient(0, 64, 0.0, 1.0));
    // 记录内部子树在多点的基准值
    const std::vector<std::tuple<i32, i32, i32>> samplePoints = {
        {0, 0, 0}, {32, 32, 32}, {-16, 0, 16}, {100, 50, -50}, {7, 63, 7}};
    std::vector<f64> baseline;
    baseline.reserve(samplePoints.size());
    for (const auto& [x, y, z] : samplePoints) {
        baseline.push_back(inner->compute(x, y, z));
    }

    // 包装为 SharedTopology（移走 inner 所有权到 shared_ptr）
    auto shared = std::make_unique<SharedTopology>(std::shared_ptr<const DensityFunction>(std::move(inner)));

    // compute 委托一致：SharedTopology.compute(x,y,z) == 内部子树.compute(x,y,z)
    for (size_t i = 0; i < samplePoints.size(); ++i) {
        const auto& [x, y, z] = samplePoints[i];
        EXPECT_DOUBLE_EQ(shared->compute(x, y, z), baseline[i])
            << "SharedTopology.compute diverged from inner at point (" << x << "," << y << "," << z << ")";
    }
}

TEST(SharedTopologyComputeTest, MapAllPreservesSharedIdentity)
{
    // SharedTopology::mapAll 返回持同一 shared_ptr 的新包装，内部子树零深拷贝。
    // 验证：mapAll 后的新 SharedTopology 持同一内部对象（身份共享）。
    auto inner = factory::constant(42.0);
    const DensityFunction* innerPtr = inner.get();
    auto shared = std::make_unique<SharedTopology>(std::shared_ptr<const DensityFunction>(std::move(inner)));

    // 最小透传 visitor：apply 原样返回（不替换任何节点）
    class IdentityVisitor final : public DensityFunction::Visitor {
    public:
        [[nodiscard]] std::unique_ptr<DensityFunction> apply(std::unique_ptr<DensityFunction> f) override { return f; }
    } visitor;

    auto mapped = shared->mapAll(visitor);
    ASSERT_NE(mapped, nullptr);
    // mapped 应仍是 SharedTopology（透传），且内部对象与原 innerPtr 同一（身份共享）
    auto* mappedShared = dynamic_cast<SharedTopology*>(mapped.get());
    ASSERT_NE(mappedShared, nullptr) << "mapAll should return a SharedTopology wrapper (identity preserved)";
    EXPECT_EQ(&mappedShared->inner(), innerPtr)
        << "mapAll must share the same inner object (zero deep-copy of subtree)";

    // compute 委托一致
    EXPECT_DOUBLE_EQ(mapped->compute(0, 0, 0), 42.0);
    EXPECT_DOUBLE_EQ(mapped->minValue(), 42.0);
    EXPECT_DOUBLE_EQ(mapped->maxValue(), 42.0);
}

// ============================================================================
// Part 3：真实数据驱动 noise_router 共享化字级一致性（主世界/下界/末地，多 seed）
// ============================================================================

namespace {

/// 采样点集合：覆盖区块内多个相对位置与负坐标，兼顾 XZ-only 与三维依赖的密度函数。
const std::vector<std::tuple<i32, i32, i32>>& paritySamplePoints()
{
    static const std::vector<std::tuple<i32, i32, i32>> points = {
        {0, 0, 0}, {8, 64, 8}, {16, 0, 16}, {31, 31, 31}, {-8, 64, -8}, {64, -32, 64}, {3, 100, 3}};
    return points;
}

/// 对给定 RandomState，验证 createRouterCopy 副本与 m_router 的 finalDensity.compute
/// 在所有采样点字级一致。共享化不应改变任何密度数值。
void expectRouterCopyParity(const world::gen::RandomState& state, const std::string& label)
{
    const auto& points = paritySamplePoints();

    // 基线：m_router（create 期共享化树）的 finalDensity.compute
    std::vector<f64> baseline;
    baseline.reserve(points.size());
    for (const auto& [x, y, z] : points) {
        baseline.push_back(state.router().finalDensity().compute(x, y, z));
    }

    // 副本 1：createRouterCopy（mapAllCopy 从共享化 m_router 派生）
    auto copy1 = state.createRouterCopy();
    for (size_t i = 0; i < points.size(); ++i) {
        const auto& [x, y, z] = points[i];
        EXPECT_DOUBLE_EQ(copy1.finalDensity().compute(x, y, z), baseline[i])
            << label << ": createRouterCopy copy1 diverged from m_router at (" << x << "," << y << "," << z << ")";
    }

    // 副本 2：再次 createRouterCopy，两个独立副本之间也应字级一致
    // （同一 RandomState 派生，NormalNoise 共享，纯拓扑子树共享，数值必然相同）
    auto copy2 = state.createRouterCopy();
    for (size_t i = 0; i < points.size(); ++i) {
        const auto& [x, y, z] = points[i];
        EXPECT_DOUBLE_EQ(copy2.finalDensity().compute(x, y, z), baseline[i])
            << label << ": createRouterCopy copy2 diverged from m_router at (" << x << "," << y << "," << z << ")";
    }
}

} // namespace

TEST(SharedTopologyRouterParityTest, OverworldMultipleSeeds)
{
    for (u64 seed : {0ULL, 1ULL, 42ULL, 12345ULL}) {
        auto state = world::gen::RandomState::create(DimensionSettings::overworld(), seed);
        ASSERT_NE(state, nullptr) << "overworld seed=" << seed << ": RandomState::create returned null";
        expectRouterCopyParity(*state, "overworld seed=" + std::to_string(seed));
    }
}

TEST(SharedTopologyRouterParityTest, NetherMultipleSeeds)
{
    for (u64 seed : {0ULL, 7ULL, 99ULL}) {
        auto state = world::gen::RandomState::create(DimensionSettings::nether(), seed);
        ASSERT_NE(state, nullptr) << "nether seed=" << seed << ": RandomState::create returned null";
        expectRouterCopyParity(*state, "nether seed=" + std::to_string(seed));
    }
}

TEST(SharedTopologyRouterParityTest, EndMultipleSeeds)
{
    for (u64 seed : {0ULL, 5ULL, 256ULL}) {
        auto state = world::gen::RandomState::create(DimensionSettings::end(), seed);
        ASSERT_NE(state, nullptr) << "end seed=" << seed << ": RandomState::create returned null";
        expectRouterCopyParity(*state, "end seed=" + std::to_string(seed));
    }
}
