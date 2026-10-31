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
 * @file PathNavigatorTest.cpp
 * @brief 路径导航器与 A* 寻路器的单元测试
 *
 * 覆盖三块内容：
 *   1. `Path` 的游标 / 裁剪 / 替换语义；
 *   2. `PathHeap` 按键值弹出（A* 的开放集）；
 *   3. 一个由方块表驱动的内存区域上，`PathFinder` 端到端算出的路径形状
 *      （直线、绕墙、爬台阶、拒绝跨越负代价地形、节点上限）。
 */

#include "common/entity/ai/pathfinding/GroundPathNavigator.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathFinder.hpp"
#include "common/entity/ai/pathfinding/PathHeap.hpp"
#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include "common/entity/ai/pathfinding/PathType.hpp"
#include "common/entity/ai/pathfinding/Region.hpp"
#include "common/entity/ai/pathfinding/WalkNodeEvaluator.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::pathfinding;

namespace {

// ============================================================================
// 测试地形
// ============================================================================

/**
 * @brief 方块表驱动的内存区域
 *
 * 未设置的格子是空气。`fillFloor` 铺一层石头地板，其余按需摆放方块。
 */
class FlatRegion final : public Region {
public:
    static u64 key(i32 x, i32 y, i32 z)
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 42) ^ (static_cast<u64>(static_cast<u32>(y)) << 21) ^
            static_cast<u64>(static_cast<u32>(z));
    }

    void setBlock(i32 x, i32 y, i32 z, const BlockState* state)
    {
        m_blocks[key(x, y, z)] = state;
    }

    /// 在 y = floorY 铺一层石头（可指定范围）。
    void fillFloor(i32 floorY, i32 radius)
    {
        for (i32 x = -radius; x <= radius; ++x) {
            for (i32 z = -radius; z <= radius; ++z) {
                setBlock(x, floorY, z, &VanillaBlocks::STONE->defaultState());
            }
        }
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto found = m_blocks.find(key(x, y, z));
        return found != m_blocks.end() ? found->second : &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        return getBlockState(x, y, z)->getFluidState();
    }

    [[nodiscard]] bool hasCollision(const void* /*entity*/, const AxisAlignedBB& box) const override
    {
        const i32 minX = static_cast<i32>(std::floor(box.minX));
        const i32 maxX = static_cast<i32>(std::ceil(box.maxX) - 1);
        const i32 minY = static_cast<i32>(std::floor(box.minY));
        const i32 maxY = static_cast<i32>(std::ceil(box.maxY) - 1);
        const i32 minZ = static_cast<i32>(std::floor(box.minZ));
        const i32 maxZ = static_cast<i32>(std::ceil(box.maxZ) - 1);
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = minZ; z <= maxZ; ++z) {
                    const BlockState* state = getBlockState(x, y, z);
                    if (!state->isAir() && !state->canBeReplaced()) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    [[nodiscard]] i32 minBuildHeight() const override
    {
        return -64;
    }

    [[nodiscard]] i32 maxBuildHeight() const override
    {
        return 320;
    }

private:
    std::unordered_map<u64, const BlockState*> m_blocks;
};

/**
 * @brief 受控的寻路实体（与生产 `MobEntity` 解耦，便于把结论钉在求值器上）
 */
class FakeEntity final : public PathfindingEntity {
public:
    FakeEntity(f32 width, f32 height, f32 stepHeight = 0.6f)
        : m_width(width)
        , m_height(height)
        , m_stepHeight(stepHeight)
    {
        m_malus.fill(UNSET);
    }

    void setMalus(PathType type, f32 malus)
    {
        m_malus[static_cast<size_t>(type)] = malus;
    }

    void setPosition(f64 x, f64 y, f64 z)
    {
        m_posX = x;
        m_posY = y;
        m_posZ = z;
    }

    [[nodiscard]] f32 pathfindingWidth() const override
    {
        return m_width;
    }

    [[nodiscard]] f32 pathfindingHeight() const override
    {
        return m_height;
    }

    [[nodiscard]] f32 pathfindingStepHeight() const override
    {
        return m_stepHeight;
    }

    [[nodiscard]] i32 pathfindingMaxFallDistance() const override
    {
        return 3;
    }

    [[nodiscard]] f32 pathfindingMalus(PathType type) const override
    {
        const f32 stored = m_malus[static_cast<size_t>(type)];
        return stored == UNSET ? getPathMalus(type) : stored;
    }

    [[nodiscard]] AxisAlignedBB pathfindingBoundingBox() const override
    {
        const f32 half = m_width / 2.0f;
        return AxisAlignedBB(static_cast<f32>(m_posX) - half,
            static_cast<f32>(m_posY),
            static_cast<f32>(m_posZ) - half,
            static_cast<f32>(m_posX) + half,
            static_cast<f32>(m_posY) + m_height,
            static_cast<f32>(m_posZ) + half);
    }

    [[nodiscard]] bool pathfindingOnGround() const override
    {
        return true;
    }

    [[nodiscard]] bool pathfindingInWater() const override
    {
        return false;
    }

    [[nodiscard]] bool pathfindingCanStandOnFluid(const fluid::FluidState* /*fluidState*/) const override
    {
        return false;
    }

private:
    static constexpr f32 UNSET = 12345.0f;

    f32 m_width;
    f32 m_height;
    f32 m_stepHeight;
    f64 m_posX = 0.5;
    f64 m_posY = 0.0;
    f64 m_posZ = 0.5;
    std::array<f32, pathTypeCount()> m_malus{};
};

/**
 * @brief 在地板上跑一次 A*
 *
 * @param region 地形（调用方负责铺好 `floorY` 的一层地板）
 * @param entity 寻路实体（位置默认落在 (0.5, floorY+1, 0.5)）
 * @param target 目标方块坐标
 * @param floorY 地板所在的 Y
 * @return 搜索出的路径；不连通时为空
 */
std::optional<Path> findPathOn(
    FlatRegion& region, FakeEntity& entity, const BlockPos& target, i32 floorY, i32 maxVisitedNodes = 512)
{
    entity.setPosition(0.5, static_cast<f64>(floorY + 1), 0.5);
    WalkNodeEvaluator evaluator;
    PathFinder finder(std::make_unique<WalkNodeEvaluator>(), maxVisitedNodes);
    const Vector3d startPosition(0.5, static_cast<f64>(floorY + 1), 0.5);
    return finder.findPath(region, entity, startPosition, {target}, 64.0f, 1, 1.0f);
}

} // namespace

// ============================================================================
// Path
// ============================================================================

namespace {

Path makeLinePath(i32 count)
{
    std::vector<PathPoint> nodes;
    nodes.reserve(static_cast<size_t>(count));
    for (i32 index = 0; index < count; ++index) {
        nodes.emplace_back(index, 0, 0);
    }
    return Path(std::move(nodes), BlockPos(count - 1, 0, 0), true);
}

} // namespace

TEST(NavigatorPathTest, EmptyPath)
{
    const Path path;

    EXPECT_TRUE(path.empty());
    EXPECT_EQ(path.nodeCount(), 0u);
    EXPECT_TRUE(path.isDone());
}

TEST(PathTest, NodeAccess)
{
    const Path path = makeLinePath(3);

    EXPECT_EQ(path.node(0).x(), 0);
    EXPECT_EQ(path.node(2).x(), 2);
    EXPECT_EQ(path.nodePos(1), BlockPos(1, 0, 0));
}

TEST(PathTest, ReplaceNode)
{
    Path path = makeLinePath(3);

    path.replaceNode(1, PathPoint(7, 8, 9));
    EXPECT_EQ(path.node(1).x(), 7);
    EXPECT_EQ(path.node(1).y(), 8);
    EXPECT_EQ(path.node(1).z(), 9);

    // 其他节点不受影响。
    EXPECT_EQ(path.node(0).x(), 0);
    EXPECT_EQ(path.node(2).x(), 2);
}

TEST(PathTest, ReplaceNodeOutOfRangeIsNoOp)
{
    Path path = makeLinePath(2);

    path.replaceNode(5, PathPoint(10, 10, 10));
    EXPECT_EQ(path.nodeCount(), 2u);
    EXPECT_EQ(path.node(1).x(), 1);
}

TEST(NavigatorPathTest, TruncateNodes)
{
    Path path = makeLinePath(5);
    EXPECT_EQ(path.nodeCount(), 5u);

    path.truncateNodes(2);
    EXPECT_EQ(path.nodeCount(), 2u);
    EXPECT_EQ(path.node(1).x(), 1);
}

TEST(PathTest, TruncateToZeroEmptiesPath)
{
    Path path = makeLinePath(4);

    path.truncateNodes(0);
    EXPECT_TRUE(path.empty());
    EXPECT_EQ(path.endNode(), nullptr);
}

TEST(PathTest, TruncateBeyondSizeKeepsPath)
{
    Path path = makeLinePath(3);

    path.truncateNodes(10);
    EXPECT_EQ(path.nodeCount(), 3u);
}

TEST(PathTest, CopyIsIndependent)
{
    const Path original = makeLinePath(3);
    Path copy = original.copy();

    copy.truncateNodes(1);
    EXPECT_EQ(copy.nodeCount(), 1u);
    EXPECT_EQ(original.nodeCount(), 3u);
}

TEST(NavigatorPathTest, EntityPosAtNodeCentresOnBlock)
{
    const Path path = makeLinePath(2);
    const Vector3d pos = path.entityPosAtNode(0.6f, 0);

    // 宽度 0.6：中心偏移 floor(0.6 + 1) * 0.5 = 0.5。
    EXPECT_FLOAT_EQ(static_cast<f32>(pos.x), 0.5f);
    EXPECT_FLOAT_EQ(static_cast<f32>(pos.z), 0.5f);
}

// ============================================================================
// PathHeap（A* 开放集）
// ============================================================================

TEST(NavigatorPathHeapTest, PopsInCostOrder)
{
    PathHeap heap;
    std::vector<std::unique_ptr<PathPoint>> points;

    const f32 costs[5] = {7.0f, 1.0f, 9.0f, 3.0f, 5.0f};
    for (i32 index = 0; index < 5; ++index) {
        points.push_back(std::make_unique<PathPoint>(index, 0, 0));
        points.back()->setTotalCost(costs[index]);
        heap.insert(points.back().get());
    }

    f32 previous = -1.0f;
    while (!heap.empty()) {
        const PathPoint* point = heap.pop();
        EXPECT_GE(point->totalCost(), previous);
        previous = point->totalCost();
    }
}

TEST(NavigatorPathHeapTest, ChangeCostReorders)
{
    PathHeap heap;
    PathPoint first(0, 0, 0);
    PathPoint second(0, 0, 0);
    first.setTotalCost(10.0f);
    second.setTotalCost(5.0f);
    heap.insert(&first);
    heap.insert(&second);

    heap.changeCost(&first, 1.0f);
    EXPECT_EQ(heap.pop(), &first);
}

// ============================================================================
// A* 端到端
// ============================================================================

class PathFinderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool initialized = false;
        if (!initialized) {
            VanillaBlocks::initialize();
            initialized = true;
        }
    }
};

TEST_F(PathFinderTest, StraightLineOnFlatGround)
{
    FlatRegion region;
    region.fillFloor(-1, 8);
    FakeEntity entity(0.6f, 1.8f);

    const auto path = findPathOn(region, entity, BlockPos(4, 0, 0), -1);
    ASSERT_TRUE(path.has_value());
    EXPECT_TRUE(path->canReach());
    // 起点 (0,0,0)、终点 (4,0,0)：路径终点必须正好落在目标上，节点数不超过曼哈顿步数 + 1。
    EXPECT_LE(path->nodeCount(), 5u);
    // 到达判定用曼哈顿距离 <= reachRange(1)，因此终点可能停在目标相邻的一格上。
    EXPECT_LE(path->endNode()->distanceManhattan(PathPoint(4, 0, 0)), 1.0f);
    EXPECT_EQ(path->target(), BlockPos(4, 0, 0));
}

TEST_F(PathFinderTest, WallForcesDetour)
{
    FlatRegion region;
    region.fillFloor(-1, 8);
    // 在 x = 2 处立一堵墙，只留 z = 2 的缺口。
    for (i32 z = -3; z <= 3; ++z) {
        if (z == 2) {
            continue;
        }
        region.setBlock(2, 0, z, &VanillaBlocks::STONE->defaultState());
        region.setBlock(2, 1, z, &VanillaBlocks::STONE->defaultState());
    }
    FakeEntity entity(0.6f, 1.8f);

    const auto path = findPathOn(region, entity, BlockPos(5, 0, 0), -1);
    ASSERT_TRUE(path.has_value());
    EXPECT_TRUE(path->canReach());
    // 必须绕过墙：路径里应出现 z 不为 0 的节点。
    bool hasDetour = false;
    for (const PathPoint& node : path->nodes()) {
        if (node.z() != 0) {
            hasDetour = true;
            break;
        }
    }
    EXPECT_TRUE(hasDetour);
}

TEST_F(PathFinderTest, StepUpIsAllowed)
{
    FlatRegion region;
    region.fillFloor(-1, 8);
    // 在 x >= 3 放一层台阶面（y = 0 的实心方块）。
    for (i32 x = 3; x <= 8; ++x) {
        for (i32 z = -8; z <= 8; ++z) {
            region.setBlock(x, 0, z, &VanillaBlocks::STONE->defaultState());
        }
    }
    FakeEntity entity(0.6f, 1.8f);

    const auto path = findPathOn(region, entity, BlockPos(5, 1, 0), -1);
    ASSERT_TRUE(path.has_value());
    EXPECT_TRUE(path->canReach());
    EXPECT_EQ(path->endNode()->y(), 1);
}

TEST_F(PathFinderTest, NegativeMalusAreaIsAvoided)
{
    FlatRegion region;
    region.fillFloor(-1, 8);
    // 把 x = 2 一整列设为水；实体声明水与水边缘都不可通行。
    for (i32 z = -8; z <= 8; ++z) {
        region.setBlock(2, 0, z, &VanillaBlocks::WATER->defaultState());
    }
    FakeEntity entity(0.6f, 1.8f);
    entity.setMalus(PathType::Water, -1.0f);
    entity.setMalus(PathType::WaterBorder, -1.0f);

    // 目标在水的另一侧：因为整列被封死，只能返回「尽力而为」的路径（未到达），
    // 且路径绝不能踏进水格。
    const auto path = findPathOn(region, entity, BlockPos(5, 0, 0), -1);
    ASSERT_TRUE(path.has_value());
    EXPECT_FALSE(path->canReach());
    for (const PathPoint& node : path->nodes()) {
        EXPECT_NE(node.x(), 2);
    }
}

TEST_F(PathFinderTest, VisitedNodeLimitStopsSearch)
{
    FlatRegion region;
    region.fillFloor(-1, 8);
    FakeEntity entity(0.6f, 1.8f);
    entity.setPosition(0.5, 0.0, 0.5);

    // 只给 1 个节点的预算：循环在取出起点之前就达到上限，展开的节点数为 0。
    PathFinder finder(std::make_unique<WalkNodeEvaluator>(), 1);
    const auto path = finder.findPath(region, entity, Vector3d(0.5, 0.0, 0.5), {BlockPos(6, 0, 0)}, 64.0f, 1, 1.0f);
    ASSERT_TRUE(path.has_value());
    EXPECT_FALSE(path->canReach());
    EXPECT_EQ(finder.lastSearchedNodes(), 1);
}

TEST_F(PathFinderTest, UnreachableTargetYieldsPartialPath)
{
    FlatRegion region;
    region.fillFloor(-1, 3);
    // 地板只有 [-3, 3]，目标在远处没有地面的位置：走不过去。
    FakeEntity entity(0.6f, 1.8f);

    const auto path = findPathOn(region, entity, BlockPos(40, 0, 0), -1);
    // 与原版一致：未到达时仍返回「走得最远」的那条路径，只是 canReach() 为 false。
    ASSERT_TRUE(path.has_value());
    EXPECT_FALSE(path->canReach());
    EXPECT_LT(path->distanceToTarget(), 40.0f);
}
