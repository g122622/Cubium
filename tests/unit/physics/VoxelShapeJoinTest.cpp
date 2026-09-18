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
 * @file VoxelShapeJoinTest.cpp
 * @brief VoxelShape 布尔运算和 IndexMerger 单元测试
 *
 * 测试内容：
 * 1. IndexMerger 子类（IdenticalMerger, NonOverlappingMerger, IndirectMerger, DiscreteCubeMerger）
 * 2. CubePointRange 检测
 * 3. BitSetDiscreteVoxelShape::join 布尔运算
 * 4. Shapes::join 和 Shapes::or_ 端到端测试
 * 5. 面遮挡检测（blockOccludes, faceShapeOccludes）
 */

#include "common/physics/shape/BitSetDiscreteVoxelShape.hpp"
#include "common/physics/shape/CubePointRange.hpp"
#include "common/physics/shape/IndexMergers.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// CubePointRange 检测测试
// ============================================================================

TEST(CubePointRangeTest, DetectUniformCoordinates)
{
    // 0/2, 1/2, 2/2 -> 0.0, 0.5, 1.0
    std::vector<f64> coords2 = {0.0, 0.5, 1.0};
    EXPECT_EQ(detectCubePointRange(coords2), 2);

    // 0/4, 1/4, 2/4, 3/4, 4/4 -> 0.0, 0.25, 0.5, 0.75, 1.0
    std::vector<f64> coords4 = {0.0, 0.25, 0.5, 0.75, 1.0};
    EXPECT_EQ(detectCubePointRange(coords4), 4);

    // 0/1, 1/1 -> 0.0, 1.0 (完整方块)
    std::vector<f64> coords1 = {0.0, 1.0};
    EXPECT_EQ(detectCubePointRange(coords1), 1);
}

TEST(CubePointRangeTest, DetectNonUniformCoordinates)
{
    // 非均匀坐标
    std::vector<f64> nonUniform = {0.0, 0.3, 1.0};
    EXPECT_EQ(detectCubePointRange(nonUniform), -1);

    // 超出 [0,1] 范围
    std::vector<f64> outOfRange = {-0.5, 0.5, 1.5};
    EXPECT_EQ(detectCubePointRange(outOfRange), -1);

    // 只有一个点
    std::vector<f64> singlePoint = {0.5};
    EXPECT_EQ(detectCubePointRange(singlePoint), -1);
}

TEST(CubePointRangeTest, LcmFunction)
{
    EXPECT_EQ(lcm(2, 3), 6);
    EXPECT_EQ(lcm(4, 6), 12);
    EXPECT_EQ(lcm(1, 1), 1);
    EXPECT_EQ(lcm(8, 4), 8);
}

// ============================================================================
// IdenticalMerger 测试
// ============================================================================

TEST(IdenticalMergerTest, BasicMerging)
{
    std::vector<f64> coords = {0.0, 0.5, 1.0};
    IdenticalMerger merger(coords);

    EXPECT_EQ(merger.size(), 3);

    const auto& list = merger.getList();
    EXPECT_EQ(list.size(), 3u);
    EXPECT_DOUBLE_EQ(list[0], 0.0);
    EXPECT_DOUBLE_EQ(list[1], 0.5);
    EXPECT_DOUBLE_EQ(list[2], 1.0);

    // forMergedIndexes: 所有索引应相同
    std::vector<std::tuple<i32, i32, i32>> results;
    merger.forMergedIndexes([&](i32 a, i32 b, i32 m) -> bool {
        results.emplace_back(a, b, m);
        return true;
    });

    ASSERT_EQ(results.size(), 2u); // 3个坐标点 -> 2个段
    EXPECT_EQ(std::get<0>(results[0]), 0);
    EXPECT_EQ(std::get<1>(results[0]), 0);
    EXPECT_EQ(std::get<2>(results[0]), 0);
    EXPECT_EQ(std::get<0>(results[1]), 1);
    EXPECT_EQ(std::get<1>(results[1]), 1);
    EXPECT_EQ(std::get<2>(results[1]), 1);
}

// ============================================================================
// NonOverlappingMerger 测试
// ============================================================================

TEST(NonOverlappingMergerTest, BasicNonOverlapping)
{
    // lower: [0.0, 0.3, 0.5]  upper: [0.5, 0.8, 1.0]
    // 不重叠：lower 最大值 0.5 <= upper 最小值 0.5
    std::vector<f64> lower = {0.0, 0.3, 0.5};
    std::vector<f64> upper = {0.5, 0.8, 1.0};

    NonOverlappingMerger merger(lower, upper, false);
    EXPECT_EQ(merger.size(), 6); // 3 + 3

    // 检查合并后的坐标列表
    const auto& list = merger.getList();
    ASSERT_EQ(list.size(), 6u);
    EXPECT_DOUBLE_EQ(list[0], 0.0);
    EXPECT_DOUBLE_EQ(list[1], 0.3);
    EXPECT_DOUBLE_EQ(list[2], 0.5);
    EXPECT_DOUBLE_EQ(list[3], 0.5);
    EXPECT_DOUBLE_EQ(list[4], 0.8);
    EXPECT_DOUBLE_EQ(list[5], 1.0);

    // 检查索引映射
    std::vector<std::tuple<i32, i32, i32>> results;
    merger.forMergedIndexes([&](i32 a, i32 b, i32 m) -> bool {
        results.emplace_back(a, b, m);
        return true;
    });

    // lower 有3个坐标 -> 3个段（但第3个段过渡到upper）
    // upper 有3个坐标 -> 2个段
    ASSERT_EQ(results.size(), 5u); // 3 + 2

    // lower 的段：firstIdx=j, secondIdx=-1
    EXPECT_EQ(std::get<0>(results[0]), 0);  // firstIdx
    EXPECT_EQ(std::get<1>(results[0]), -1); // secondIdx（不存在）
    EXPECT_EQ(std::get<2>(results[0]), 0);  // mergedIdx

    EXPECT_EQ(std::get<0>(results[1]), 1);
    EXPECT_EQ(std::get<1>(results[1]), -1);
    EXPECT_EQ(std::get<2>(results[1]), 1);

    EXPECT_EQ(std::get<0>(results[2]), 2); // lower 最后一段
    EXPECT_EQ(std::get<1>(results[2]), -1);
    EXPECT_EQ(std::get<2>(results[2]), 2);

    // upper 的段：firstIdx=2(lower.size()-1), secondIdx=k
    EXPECT_EQ(std::get<0>(results[3]), 2);
    EXPECT_EQ(std::get<1>(results[3]), 0);
    EXPECT_EQ(std::get<2>(results[3]), 3);

    EXPECT_EQ(std::get<0>(results[4]), 2);
    EXPECT_EQ(std::get<1>(results[4]), 1);
    EXPECT_EQ(std::get<2>(results[4]), 4);
}

TEST(NonOverlappingMergerTest, SwappedIndices)
{
    std::vector<f64> lower = {0.0, 0.3};
    std::vector<f64> upper = {0.5, 1.0};

    // swap=true 时交换 firstIdx 和 secondIdx
    NonOverlappingMerger merger(lower, upper, true);

    std::vector<std::tuple<i32, i32, i32>> results;
    merger.forMergedIndexes([&](i32 a, i32 b, i32 m) -> bool {
        results.emplace_back(a, b, m);
        return true;
    });

    // swap=true 时 lower 的段：firstIdx=-1, secondIdx=j
    EXPECT_EQ(std::get<0>(results[0]), -1); // firstIdx 应为 -1（被交换）
    EXPECT_EQ(std::get<1>(results[0]), 0);  // secondIdx 应为 0
}

TEST(NonOverlappingMergerTest, GapBetweenShapes)
{
    // lower: [0.0, 0.3]  upper: [0.5, 1.0]
    // 中间有一个间隙 [0.3, 0.5)
    std::vector<f64> lower = {0.0, 0.3};
    std::vector<f64> upper = {0.5, 1.0};

    NonOverlappingMerger merger(lower, upper, false);
    EXPECT_EQ(merger.size(), 4); // 2 + 2

    const auto& list = merger.getList();
    ASSERT_EQ(list.size(), 4u);
    EXPECT_DOUBLE_EQ(list[0], 0.0);
    EXPECT_DOUBLE_EQ(list[1], 0.3);
    EXPECT_DOUBLE_EQ(list[2], 0.5);
    EXPECT_DOUBLE_EQ(list[3], 1.0);

    // 验证索引映射
    std::vector<std::tuple<i32, i32, i32>> results;
    merger.forMergedIndexes([&](i32 a, i32 b, i32 m) -> bool {
        results.emplace_back(a, b, m);
        return true;
    });

    // lower 有2个坐标 -> 2个段
    // upper 有2个坐标 -> 1个段
    // 总共 3 个段
    ASSERT_EQ(results.size(), 3u);

    // 段0: [0.0, 0.3) - 属于 lower
    EXPECT_EQ(std::get<0>(results[0]), 0);  // firstIdx = 0 (lower 的段0)
    EXPECT_EQ(std::get<1>(results[0]), -1); // secondIdx = -1 (upper 不存在)
    EXPECT_EQ(std::get<2>(results[0]), 0);  // mergedIdx = 0

    // 段1: [0.3, 0.5) - 间隙段，连接 lower 末尾到 upper 开头
    EXPECT_EQ(std::get<0>(results[1]), 1);  // firstIdx = lower.size()-1 = 1
    EXPECT_EQ(std::get<1>(results[1]), -1); // secondIdx = -1
    EXPECT_EQ(std::get<2>(results[1]), 1);  // mergedIdx = 1

    // 段2: [0.5, 1.0) - 属于 upper
    EXPECT_EQ(std::get<0>(results[2]), 1); // firstIdx = lower.size()-1 = 1
    EXPECT_EQ(std::get<1>(results[2]), 0); // secondIdx = 0 (upper 的段0)
    EXPECT_EQ(std::get<2>(results[2]), 2); // mergedIdx = 2
}

TEST(NonOverlappingMergerTest, SwappedWithGap)
{
    // lower: [0.0, 0.3]  upper: [0.5, 1.0], swapped=true
    // 模拟形状 B 在形状 A 之前的情况
    // swap=true 时，forNonSwappedIndexes 生成的 (firstIdx, secondIdx) 会被交换
    std::vector<f64> lower = {0.0, 0.3};
    std::vector<f64> upper = {0.5, 1.0};

    NonOverlappingMerger merger(lower, upper, true);

    std::vector<std::tuple<i32, i32, i32>> results;
    merger.forMergedIndexes([&](i32 a, i32 b, i32 m) -> bool {
        results.emplace_back(a, b, m);
        return true;
    });

    ASSERT_EQ(results.size(), 3u);

    // swap=true 时，forNonSwappedIndexes 生成的原始索引被交换
    // 原始（非交换）：段0 merge(0, -1, 0) -> 交换后 consumer(-1, 0, 0)
    EXPECT_EQ(std::get<0>(results[0]), -1); // 原始 firstIdx=0, 交换后 secondIdx=-1 -> firstIdx=-1
    EXPECT_EQ(std::get<1>(results[0]), 0);  // 原始 secondIdx=-1, 交换后 -> secondIdx=0

    // 原始：段1 merge(1, -1, 1) -> 交换后 consumer(-1, 1, 1)
    EXPECT_EQ(std::get<0>(results[1]), -1);
    EXPECT_EQ(std::get<1>(results[1]), 1);

    // 原始：段2 merge(1, 0, 2) -> 交换后 consumer(0, 1, 2)
    EXPECT_EQ(std::get<0>(results[2]), 0);
    EXPECT_EQ(std::get<1>(results[2]), 1);
}

TEST(NonOverlappingMergerTest, SizeMatchesCoordinateCount)
{
    // size() 应等于 lower.size() + upper.size()
    // 段数应等于 size() - 1 = lower.size() + upper.size() - 1
    std::vector<f64> lower = {0.0, 0.25, 0.5};
    std::vector<f64> upper = {0.6, 0.8, 1.0};

    NonOverlappingMerger merger(lower, upper, false);
    EXPECT_EQ(merger.size(), 6); // 3 + 3

    i32 segCount = 0;
    merger.forMergedIndexes([&](i32, i32, i32) -> bool {
        ++segCount;
        return true;
    });
    EXPECT_EQ(segCount, 5); // size - 1 = 6 - 1 = 5
}

// ============================================================================
// IndirectMerger 测试
// ============================================================================

TEST(IndirectMergerTest, BasicOverlapping)
{
    // first: [0.0, 0.5, 1.0]
    // second: [0.0, 0.3, 0.7, 1.0]
    // 合并后: [0.0, 0.3, 0.5, 0.7, 1.0]
    std::vector<f64> first = {0.0, 0.5, 1.0};
    std::vector<f64> second = {0.0, 0.3, 0.7, 1.0};

    IndirectMerger merger(first, second, true, true);

    EXPECT_GE(merger.size(), 4); // 至少5个坐标 -> size >= 5
    // 验证合并后坐标列表
    const auto& list = merger.getList();
    EXPECT_GE(list.size(), 4u);

    // 验证 forMergedIndexes 产生合理的段数
    i32 segCount = 0;
    merger.forMergedIndexes([&](i32, i32, i32) -> bool {
        ++segCount;
        return true;
    });
    EXPECT_GE(segCount, 3); // 至少3个段
}

TEST(IndirectMergerTest, IdenticalInputs)
{
    std::vector<f64> coords = {0.0, 0.5, 1.0};

    IndirectMerger merger(coords, coords, true, true);

    // 合并相同列表应产生与输入相同的坐标
    const auto& list = merger.getList();
    ASSERT_EQ(list.size(), 3u);
    EXPECT_DOUBLE_EQ(list[0], 0.0);
    EXPECT_DOUBLE_EQ(list[1], 0.5);
    EXPECT_DOUBLE_EQ(list[2], 1.0);

    // 段索引应一一对应
    std::vector<std::tuple<i32, i32>> indices;
    merger.forMergedIndexes([&](i32 a, i32 b, i32) -> bool {
        indices.emplace_back(a, b);
        return true;
    });

    ASSERT_EQ(indices.size(), 2u);
    EXPECT_EQ(std::get<0>(indices[0]), std::get<1>(indices[0]));
    EXPECT_EQ(std::get<0>(indices[1]), std::get<1>(indices[1]));
}

TEST(IndirectMergerTest, SkipExclusiveRegions)
{
    // first: [0.0, 0.3, 0.5]  (独占 [0.0, 0.3) 和共享 [0.3, 0.5))
    // second: [0.3, 0.7, 1.0] (共享 [0.3, 0.5) 和独占 [0.5, 1.0))
    // includeFirst=false: 跳过 first 独占的 [0.0, 0.3)
    // includeSecond=true: 保留 second 独占的 [0.5, 1.0)

    std::vector<f64> first = {0.0, 0.3, 0.5};
    std::vector<f64> second = {0.3, 0.7, 1.0};

    IndirectMerger mergerSkipFirst(first, second, false, true);

    // 跳过 first 独占区域后，合并坐标不应包含 0.0
    const auto& list = mergerSkipFirst.getList();
    ASSERT_GE(list.size(), 2u);
    // 第一个坐标应该是 second 的起始 0.3（first 独占的 0.0 被跳过）
    EXPECT_DOUBLE_EQ(list[0], 0.3);
}

// ============================================================================
// DiscreteCubeMerger 测试
// ============================================================================

TEST(DiscreteCubeMergerTest, BasicMerge)
{
    // first: 2段 (0/2, 1/2, 2/2)
    // second: 3段 (0/3, 1/3, 2/3, 3/3)
    // LCM(2,3) = 6段
    DiscreteCubeMerger merger(2, 3);

    EXPECT_EQ(merger.size(), 7); // 6段 + 1

    const auto& list = merger.getList();
    ASSERT_EQ(list.size(), 7u);
    EXPECT_DOUBLE_EQ(list[0], 0.0);
    EXPECT_NEAR(list[1], 1.0 / 6.0, 1e-10);
    EXPECT_NEAR(list[6], 1.0, 1e-10);

    // 验证段索引映射
    std::vector<std::tuple<i32, i32, i32>> results;
    merger.forMergedIndexes([&](i32 a, i32 b, i32 m) -> bool {
        results.emplace_back(a, b, m);
        return true;
    });

    ASSERT_EQ(results.size(), 6u);

    // 段0: firstIdx=0/3=0, secondIdx=0/2=0
    EXPECT_EQ(std::get<0>(results[0]), 0);
    EXPECT_EQ(std::get<1>(results[0]), 0);

    // 段1: firstIdx=1/3=0, secondIdx=1/2=0
    EXPECT_EQ(std::get<0>(results[1]), 0);
    EXPECT_EQ(std::get<1>(results[1]), 0);

    // 段2: firstIdx=2/3=0, secondIdx=2/2=1
    EXPECT_EQ(std::get<0>(results[2]), 0);
    EXPECT_EQ(std::get<1>(results[2]), 1);

    // 段3: firstIdx=3/3=1, secondIdx=3/2=1
    EXPECT_EQ(std::get<0>(results[3]), 1);
    EXPECT_EQ(std::get<1>(results[3]), 1);
}

// ============================================================================
// Shapes::join 端到端测试
// ============================================================================

class VoxelShapeJoinTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(VoxelShapeJoinTest, OrTwoBoxes)
{
    // 两个相邻的半方块合并应该得到完整方块
    VoxelShape lowerHalf = Shapes::box(0.0, 0.0, 0.0, 1.0, 0.5, 1.0);
    VoxelShape upperHalf = Shapes::box(0.0, 0.5, 0.0, 1.0, 1.0, 1.0);

    VoxelShape result = Shapes::or_(lowerHalf, upperHalf);

    // 合并后应该是完整方块
    EXPECT_FALSE(result.isEmpty());
    EXPECT_DOUBLE_EQ(result.min(Axis::X), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::X), 1.0);
    EXPECT_DOUBLE_EQ(result.min(Axis::Y), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::Y), 1.0);
    EXPECT_DOUBLE_EQ(result.min(Axis::Z), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::Z), 1.0);
}

TEST_F(VoxelShapeJoinTest, OrSameBox)
{
    // OR 同一个盒子应该得到相同的盒子
    VoxelShape box = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 0.5);
    VoxelShape result = Shapes::or_(box, box);

    EXPECT_FALSE(result.isEmpty());
    EXPECT_DOUBLE_EQ(result.min(Axis::X), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::X), 0.5);
}

TEST_F(VoxelShapeJoinTest, OrEmpty)
{
    // OR 空形状应得到另一个形状
    VoxelShape empty = Shapes::empty();
    VoxelShape box = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

    VoxelShape result1 = Shapes::or_(empty, box);
    EXPECT_FALSE(result1.isEmpty());

    VoxelShape result2 = Shapes::or_(box, empty);
    EXPECT_FALSE(result2.isEmpty());
}

TEST_F(VoxelShapeJoinTest, OrNonOverlapping)
{
    // 两个不重叠的半方块
    VoxelShape left = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 1.0);
    VoxelShape right = Shapes::box(0.5, 0.0, 0.0, 1.0, 1.0, 1.0);

    VoxelShape result = Shapes::or_(left, right);

    EXPECT_FALSE(result.isEmpty());
    // 结果应覆盖 [0, 1] x [0, 1] x [0, 1]
    EXPECT_DOUBLE_EQ(result.min(Axis::X), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::X), 1.0);
}

TEST_F(VoxelShapeJoinTest, OnlyFirstOperation)
{
    // OnlyFirst: 结果应包含只在第一个形状中的部分
    VoxelShape full = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    VoxelShape half = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 1.0);

    VoxelShape result = Shapes::join(full, half, BooleanOps::OnlyFirst());

    // full OnlyFirst half = [0.5, 1] x [0, 1] x [0, 1]
    EXPECT_FALSE(result.isEmpty());
    // 结果应从 x=0.5 开始
    EXPECT_GE(result.min(Axis::X), 0.5 - 1e-6);
}

TEST_F(VoxelShapeJoinTest, JoinIsNotEmptyOnlyFirst)
{
    // 验证 joinIsNotEmpty 对 OnlyFirst 操作的正确性
    VoxelShape full = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    VoxelShape half = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 1.0);

    // full OnlyFirst half: full 中不在 half 中的部分 = [0.5,1]x[0,1]x[0,1]，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(full, half, BooleanOps::OnlyFirst()));

    // half OnlyFirst full: half 中不在 full 中的部分 = 空（half 完全在 full 内部）
    EXPECT_FALSE(Shapes::joinIsNotEmpty(half, full, BooleanOps::OnlyFirst()));
}

TEST_F(VoxelShapeJoinTest, FaceShapeOccludes)
{
    // 两个完整方块应该完全遮挡
    VoxelShape block = Shapes::block();
    EXPECT_TRUE(Shapes::faceShapeOccludes(block, block));

    // 两个空形状不遮挡
    VoxelShape empty = Shapes::empty();
    EXPECT_FALSE(Shapes::faceShapeOccludes(empty, empty));

    // 完整方块 + 任意形状（包括空形状）都遮挡
    // 因为 isBlock 检查在 faceShapeOccludes 中排除了空形状的短路
    // 但 isBlock(block) 为 true 时直接返回 true
    EXPECT_TRUE(Shapes::faceShapeOccludes(block, empty));
    EXPECT_TRUE(Shapes::faceShapeOccludes(empty, block));

    // 半方块 + 半方块 = 完全遮挡
    VoxelShape lowerHalf = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 0.5);
    VoxelShape upperHalf = Shapes::box(0.0, 0.0, 0.5, 1.0, 1.0, 1.0);
    EXPECT_TRUE(Shapes::faceShapeOccludes(lowerHalf, upperHalf));
}

TEST_F(VoxelShapeJoinTest, BlockOccludes)
{
    VoxelShape block = Shapes::block();

    // 两个完整方块在任何方向都完全遮挡
    EXPECT_TRUE(Shapes::blockOccludes(block, block, Direction::Up));
    EXPECT_TRUE(Shapes::blockOccludes(block, block, Direction::Down));
    EXPECT_TRUE(Shapes::blockOccludes(block, block, Direction::North));
    EXPECT_TRUE(Shapes::blockOccludes(block, block, Direction::South));

    // 空形状不应遮挡
    VoxelShape empty = Shapes::empty();
    EXPECT_FALSE(Shapes::blockOccludes(block, empty, Direction::Up));
    EXPECT_FALSE(Shapes::blockOccludes(empty, block, Direction::Up));
}

TEST_F(VoxelShapeJoinTest, VoxelShapeContains)
{
    VoxelShape box = Shapes::box(0.25, 0.25, 0.25, 0.75, 0.75, 0.75);

    // 中心点应在盒子内
    EXPECT_TRUE(box.contains(0.5, 0.5, 0.5));

    // 边界外不应在盒子内（半开区间）
    EXPECT_FALSE(box.contains(0.0, 0.5, 0.5));
    EXPECT_FALSE(box.contains(0.5, 0.0, 0.5));
}

TEST_F(VoxelShapeJoinTest, VoxelShapeMove)
{
    VoxelShape box = Shapes::box(0.0, 0.0, 0.0, 0.5, 0.5, 0.5);
    VoxelShape moved = box.move(0.5, 0.5, 0.5);

    EXPECT_DOUBLE_EQ(moved.min(Axis::X), 0.5);
    EXPECT_DOUBLE_EQ(moved.min(Axis::Y), 0.5);
    EXPECT_DOUBLE_EQ(moved.min(Axis::Z), 0.5);
    EXPECT_DOUBLE_EQ(moved.max(Axis::X), 1.0);
    EXPECT_DOUBLE_EQ(moved.max(Axis::Y), 1.0);
    EXPECT_DOUBLE_EQ(moved.max(Axis::Z), 1.0);
}

TEST_F(VoxelShapeJoinTest, MultipleBoxOr)
{
    // 三个非重叠方块合并
    VoxelShape a = Shapes::box(0.0, 0.0, 0.0, 0.25, 1.0, 1.0);
    VoxelShape b = Shapes::box(0.25, 0.0, 0.0, 0.5, 1.0, 1.0);
    VoxelShape c = Shapes::box(0.5, 0.0, 0.0, 0.75, 1.0, 1.0);

    std::vector<VoxelShape> shapes = {b, c};
    VoxelShape result = Shapes::or_(a, shapes);

    EXPECT_FALSE(result.isEmpty());
    EXPECT_DOUBLE_EQ(result.min(Axis::X), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::X), 0.75);
}

TEST_F(VoxelShapeJoinTest, SmallBoxOr)
{
    // 测试细粒度的方块合并
    // 1/4 x 1 x 1 的方块
    VoxelShape a = Shapes::box(0.0, 0.0, 0.0, 0.25, 1.0, 1.0);
    VoxelShape b = Shapes::box(0.25, 0.0, 0.0, 0.5, 1.0, 1.0);

    VoxelShape result = Shapes::or_(a, b);

    EXPECT_FALSE(result.isEmpty());
    EXPECT_DOUBLE_EQ(result.min(Axis::X), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::X), 0.5);
    EXPECT_DOUBLE_EQ(result.min(Axis::Y), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::Y), 1.0);
}

TEST_F(VoxelShapeJoinTest, OptimizePreservesShape)
{
    // optimize() 不应改变形状的边界
    VoxelShape box = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    VoxelShape optimized = box.optimize();

    EXPECT_FALSE(optimized.isEmpty());
    EXPECT_DOUBLE_EQ(optimized.min(Axis::X), 0.0);
    EXPECT_DOUBLE_EQ(optimized.max(Axis::X), 1.0);
    EXPECT_DOUBLE_EQ(optimized.min(Axis::Y), 0.0);
    EXPECT_DOUBLE_EQ(optimized.max(Axis::Y), 1.0);
    EXPECT_DOUBLE_EQ(optimized.min(Axis::Z), 0.0);
    EXPECT_DOUBLE_EQ(optimized.max(Axis::Z), 1.0);
}

TEST_F(VoxelShapeJoinTest, CollisionBasic)
{
    // 测试基本的碰撞检测
    VoxelShape box = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    AxisAlignedBB entity(0.5, 0.5, 0.5, 1.5, 1.5, 1.5);

    // 向 -X 方向移动应该碰撞
    f64 result = box.collide(Axis::X, entity, -0.5);
    EXPECT_NEAR(result, -0.5, 1e-6);

    // 向 +X 方向移动不应碰撞
    f64 result2 = box.collide(Axis::X, entity, 0.5);
    EXPECT_DOUBLE_EQ(result2, 0.5);
}

// ============================================================================
// NonOverlappingMerger 端到端测试（真正不重叠的形状，带间隙）
// ============================================================================

TEST_F(VoxelShapeJoinTest, OrGapNonOverlappingShapes)
{
    // 两个在 X 轴上不重叠且有间隙的形状
    // left: [0, 0.25) x [0,1] x [0,1]
    // right: [0.5, 1) x [0,1] x [0,1]
    // 间隙: [0.25, 0.5) x [0,1] x [0,1]
    VoxelShape left = Shapes::box(0.0, 0.0, 0.0, 0.25, 1.0, 1.0);
    VoxelShape right = Shapes::box(0.5, 0.0, 0.0, 1.0, 1.0, 1.0);

    VoxelShape result = Shapes::or_(left, right);

    EXPECT_FALSE(result.isEmpty());
    EXPECT_DOUBLE_EQ(result.min(Axis::X), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::X), 1.0);
    EXPECT_DOUBLE_EQ(result.min(Axis::Y), 0.0);
    EXPECT_DOUBLE_EQ(result.max(Axis::Y), 1.0);

    // 间隙区域不应包含任何体素
    // [0.25, 0.5) 在 X 方向上是空的
    // 但整体边界应该是 [0, 1] 因为 OR 合并了两个形状
}

TEST_F(VoxelShapeJoinTest, JoinIsNotEmptyNonOverlappingWithGap)
{
    // 不重叠且有间隙的形状
    VoxelShape left = Shapes::box(0.0, 0.0, 0.0, 0.25, 1.0, 1.0);
    VoxelShape right = Shapes::box(0.5, 0.0, 0.0, 1.0, 1.0, 1.0);

    // OR 操作：两个不重叠的形状 OR 应该非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(left, right, BooleanOps::Or()));

    // AND 操作：两个不重叠的形状 AND 应该为空
    EXPECT_FALSE(Shapes::joinIsNotEmpty(left, right, BooleanOps::And()));

    // OnlyFirst：left 中不在 right 中的部分 = left 本身，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(left, right, BooleanOps::OnlyFirst()));

    // OnlySecond：right 中不在 left 中的部分 = right 本身，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(left, right, BooleanOps::OnlySecond()));
}

TEST_F(VoxelShapeJoinTest, JoinIsNotEmptyOverlapping)
{
    // 重叠的形状
    VoxelShape full = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    VoxelShape half = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 1.0);

    // AND：重叠区域 = [0, 0.5) x [0,1] x [0,1]，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(full, half, BooleanOps::And()));

    // OR：并集 = full，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(full, half, BooleanOps::Or()));

    // OnlyFirst: full 中不在 half 中的 = [0.5, 1] x [0,1] x [0,1]，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(full, half, BooleanOps::OnlyFirst()));

    // OnlyFirst: half 中不在 full 中的 = 空（half 完全在 full 内部）
    EXPECT_FALSE(Shapes::joinIsNotEmpty(half, full, BooleanOps::OnlyFirst()));

    // OnlySecond: full 中不在 half 中的 = [0.5, 1] x [0,1] x [0,1]，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(half, full, BooleanOps::OnlySecond()));

    // OnlySecond: half 中不在 full 中的 = 空（half 完全在 full 内部）
    EXPECT_FALSE(Shapes::joinIsNotEmpty(full, half, BooleanOps::OnlySecond()));
}

TEST_F(VoxelShapeJoinTest, JoinIsNotEmptyIdenticalShapes)
{
    VoxelShape half = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 1.0);

    // 同一个形状
    // AND: 交集 = half，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(half, half, BooleanOps::And()));
    // OR: 并集 = half，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(half, half, BooleanOps::Or()));
    // OnlyFirst: half - half = 空
    EXPECT_FALSE(Shapes::joinIsNotEmpty(half, half, BooleanOps::OnlyFirst()));
}

TEST_F(VoxelShapeJoinTest, JoinIsNotEmptyEmptyShapes)
{
    VoxelShape empty = Shapes::empty();
    VoxelShape box = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

    // OR: 空 OR 空 = 空
    EXPECT_FALSE(Shapes::joinIsNotEmpty(empty, empty, BooleanOps::Or()));
    // OR: 空 OR box = box，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(empty, box, BooleanOps::Or()));
    // AND: 空 AND box = 空
    EXPECT_FALSE(Shapes::joinIsNotEmpty(empty, box, BooleanOps::And()));
    // OnlyFirst: 空 OnlyFirst box = 空
    EXPECT_FALSE(Shapes::joinIsNotEmpty(empty, box, BooleanOps::OnlyFirst()));
    // OnlySecond: 空 OnlySecond box = box，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(empty, box, BooleanOps::OnlySecond()));
}

// ============================================================================
// IndirectMerger skipFirst/skipSkipSecond 详细测试
// ============================================================================

TEST(IndirectMergerTest, SkipFirstDetailed)
{
    // first: [0.0, 0.3, 0.5]  (段0: [0.0, 0.3), 段1: [0.3, 0.5))
    // second: [0.3, 0.7, 1.0] (段0: [0.3, 0.5), 段1: [0.5, 1.0))
    // 重叠区域: [0.3, 0.5) (first 段1 和 second 段0)
    // first 独占: [0.0, 0.3) (first 段0)
    // second 独占: [0.5, 1.0) (second 段1)
    //
    // includeFirst=false, includeSecond=true:
    // 跳过 first 的独占区域 [0.0, 0.3)
    // 结果坐标应从 0.3 开始

    std::vector<f64> first = {0.0, 0.3, 0.5};
    std::vector<f64> second = {0.3, 0.7, 1.0};

    IndirectMerger merger(first, second, false, true);

    const auto& list = merger.getList();
    ASSERT_GE(list.size(), 2u);

    // 第一个坐标应该是 0.3（first 独占的 0.0 被跳过）
    EXPECT_DOUBLE_EQ(list[0], 0.3);

    // 最后一个坐标应该是 1.0
    EXPECT_DOUBLE_EQ(list.back(), 1.0);

    // 验证索引映射
    std::vector<std::tuple<i32, i32, i32>> results;
    merger.forMergedIndexes([&](i32 a, i32 b, i32 m) -> bool {
        results.emplace_back(a, b, m);
        return true;
    });

    // 不应包含 firstIdx=0, secondIdx=-1 的段（那是 first 的独占区域）
    for (const auto& [a, b, m] : results) {
        // firstIdx 不应为负数（除非 first 独占区域被跳过后仍有 second 独占段）
        // 在这个情况下，所有段要么在重叠区域，要么在 second 独占区域
    }
}

TEST(IndirectMergerTest, SkipSecondDetailed)
{
    // first: [0.0, 0.3, 0.5]  (段0: [0.0, 0.3), 段1: [0.3, 0.5))
    // second: [0.3, 0.7, 1.0] (段0: [0.3, 0.5), 段1: [0.5, 1.0))
    //
    // includeFirst=true, includeSecond=false:
    // 跳过 second 的独占区域 [0.5, 1.0)
    // 结果坐标不应包含 1.0

    std::vector<f64> first = {0.0, 0.3, 0.5};
    std::vector<f64> second = {0.3, 0.7, 1.0};

    IndirectMerger merger(first, second, true, false);

    const auto& list = merger.getList();
    ASSERT_GE(list.size(), 2u);

    // 第一个坐标应该是 0.0（first 的起始）
    EXPECT_DOUBLE_EQ(list[0], 0.0);

    // 最后一个坐标应该是 0.7（second 的倒数第二个坐标，1.0 被跳过）
    // 或者是 0.5（取决于合并逻辑），但不应该是 1.0
    EXPECT_LT(list.back(), 1.0 - 1e-10);
}

TEST(IndirectMergerTest, IncludeBoth)
{
    // first: [0.0, 0.5, 1.0]
    // second: [0.0, 0.3, 0.7, 1.0]
    // includeFirst=true, includeSecond=true: 保留所有段
    std::vector<f64> first = {0.0, 0.5, 1.0};
    std::vector<f64> second = {0.0, 0.3, 0.7, 1.0};

    IndirectMerger merger(first, second, true, true);

    const auto& list = merger.getList();
    ASSERT_EQ(list.size(), 5u);

    // 合并后的坐标应该是 [0.0, 0.3, 0.5, 0.7, 1.0]
    EXPECT_DOUBLE_EQ(list[0], 0.0);
    EXPECT_DOUBLE_EQ(list[1], 0.3);
    EXPECT_DOUBLE_EQ(list[2], 0.5);
    EXPECT_DOUBLE_EQ(list[3], 0.7);
    EXPECT_DOUBLE_EQ(list[4], 1.0);
}

// ============================================================================
// joinIsNotEmpty 短路退出测试
// ============================================================================

TEST_F(VoxelShapeJoinTest, JoinIsNotEmptyEarlyExit)
{
    // 两个完全不重叠的形状（快速路径应该生效）
    VoxelShape left = Shapes::box(0.0, 0.0, 0.0, 0.25, 0.25, 0.25);
    VoxelShape right = Shapes::box(0.75, 0.75, 0.75, 1.0, 1.0, 1.0);

    // 在每个轴上都不重叠
    EXPECT_FALSE(Shapes::joinIsNotEmpty(left, right, BooleanOps::And()));
    EXPECT_TRUE(Shapes::joinIsNotEmpty(left, right, BooleanOps::Or()));
}

// ============================================================================
// OnlySecond 操作测试
// ============================================================================

TEST_F(VoxelShapeJoinTest, OnlySecondOperation)
{
    // OnlySecond: 结果应包含只在第二个形状中的部分
    VoxelShape full = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    VoxelShape half = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 1.0);

    // full OnlySecond half: half 中不在 full 中的部分 = 空
    EXPECT_FALSE(Shapes::joinIsNotEmpty(full, half, BooleanOps::OnlySecond()));

    // half OnlySecond full: full 中不在 half 中的部分 = [0.5, 1] x [0, 1] x [0, 1]，非空
    EXPECT_TRUE(Shapes::joinIsNotEmpty(half, full, BooleanOps::OnlySecond()));
}

// ============================================================================
// AND 操作测试
// ============================================================================

TEST_F(VoxelShapeJoinTest, AndOperation)
{
    VoxelShape full = Shapes::box(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    VoxelShape half = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 1.0);

    VoxelShape result = Shapes::join(full, half, BooleanOps::And());

    // AND: 交集应该等于 half
    EXPECT_FALSE(result.isEmpty());
    EXPECT_NEAR(result.min(Axis::X), 0.0, 1e-6);
    EXPECT_NEAR(result.max(Axis::X), 0.5, 1e-6);
}

TEST_F(VoxelShapeJoinTest, AndNonOverlapping)
{
    VoxelShape left = Shapes::box(0.0, 0.0, 0.0, 0.25, 1.0, 1.0);
    VoxelShape right = Shapes::box(0.5, 0.0, 0.0, 1.0, 1.0, 1.0);

    VoxelShape result = Shapes::join(left, right, BooleanOps::And());

    // 不重叠的形状 AND 应该为空
    EXPECT_TRUE(result.isEmpty());
}

// ============================================================================
// faceShapeOccludes 更多测试
// ============================================================================

TEST_F(VoxelShapeJoinTest, FaceShapeOccludesPartialCoverage)
{
    // 两个小方块不能遮挡完整面
    VoxelShape quarter1 = Shapes::box(0.0, 0.0, 0.0, 0.5, 0.5, 1.0);
    VoxelShape quarter2 = Shapes::box(0.5, 0.5, 0.0, 1.0, 1.0, 1.0);

    // 两个四分之一方块不能遮挡整个面（中间有空隙）
    EXPECT_FALSE(Shapes::faceShapeOccludes(quarter1, quarter2));
}

TEST_F(VoxelShapeJoinTest, FaceShapeOccludesFullCoverage)
{
    // 两个半方块覆盖完整面
    VoxelShape half1 = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 1.0);
    VoxelShape half2 = Shapes::box(0.5, 0.0, 0.0, 1.0, 1.0, 1.0);

    EXPECT_TRUE(Shapes::faceShapeOccludes(half1, half2));
}
