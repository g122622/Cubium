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
 * @file JungleTempleRotationTest.cpp
 * @brief 丛林神庙布局的朝向变换测试
 *
 * 神庙方块布局用结构内局部坐标书写，由 StructurePiece::setBlockState 叠加
 * getXWithOffset/getZWithOffset 的镜像与旋转映射到世界坐标。这里用同一构件、
 * 四个不同朝向生成，断言所有写出的方块都落在该朝向的包围盒内，且四个朝向
 * 写出的方块数一致（布局没有因为朝向而丢失或多出方块）。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/structures/JungleTempleStructure.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

using namespace mc;
using namespace mc::block_registry;
using namespace mc::world::gen::structure;

namespace {

/**
 * @brief 按位置记录方块的世界（继承测试桩，补上按坐标存取）
 *
 * 神庙布局会通过动态转换读取世界（判定已有方块），因此必须是一个完整的 IWorld；
 * `BaseTestWorld` 提供其余成员的桩实现，这里只替换按坐标存取。
 */
class RecordingWorld final : public test::MapBackedTestWorld {
public:
    RecordingWorld()
        : MapBackedTestWorld(Difficulty::Normal)
    {}

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_writes.emplace_back(x, y, z);
        return MapBackedTestWorld::setBlockState(x, y, z, state);
    }

    [[nodiscard]] const std::vector<BlockPos>& writes() const
    {
        return m_writes;
    }

private:
    std::vector<BlockPos> m_writes;
};

} // namespace

class JungleTempleRotationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
};
TEST_F(JungleTempleRotationTest, AllFacingsStayInsideBoundingBox)
{
    // 起点放在区块最小角，Y 取 64（原版 SinglePieceStructure 固定高度）。
    const BlockPos startPos(0, 64, 0);

    for (const Direction direction : Directions::horizontal()) {
        math::Random rng(1234);
        JungleTemplePiece piece(startPos, direction);

        RecordingWorld world;
        // 包围盒放宽到足以覆盖整座神庙，避免边界裁剪掩盖朝向错误。
        const StructureBoundingBox chunkBounds(-40, 50, -40, 40, 90, 40);
        piece.generate(world, rng, 0, 0, chunkBounds, nullptr, nullptr);

        ASSERT_FALSE(world.writes().empty()) << "temple must place blocks for facing " << static_cast<i32>(direction);

        const StructureBoundingBox& box = piece.getBoundingBox();
        for (const BlockPos& pos : world.writes()) {
            EXPECT_TRUE(box.contains(pos.x, pos.y, pos.z))
                << "block (" << pos.x << "," << pos.y << "," << pos.z << ") outside bounding box ("
                << box.minX() << ".." << box.maxX() << ", " << box.minY() << ".." << box.maxY() << ", " << box.minZ()
                << ".." << box.maxZ() << ") for facing " << static_cast<i32>(direction);
        }
    }
}

TEST_F(JungleTempleRotationTest, DifferentFacingsReachDifferentFootprints)
{
    // 四个朝向写出的方块集合必须互不相同：若布局忽略朝向，东西向与南北向会完全重合。
    const BlockPos startPos(0, 64, 0);
    const StructureBoundingBox chunkBounds(-40, 50, -40, 40, 90, 40);

    std::vector<std::vector<std::pair<i32, i32>>> footprints;
    std::vector<size_t> blockCounts;
    for (const Direction direction : Directions::horizontal()) {
        math::Random rng(1);
        JungleTemplePiece piece(startPos, direction);
        RecordingWorld world;
        piece.generate(world, rng, 0, 0, chunkBounds, nullptr, nullptr);

        std::vector<std::pair<i32, i32>> footprint;
        footprint.reserve(world.writes().size());
        for (const BlockPos& pos : world.writes()) {
            footprint.emplace_back(pos.x, pos.z);
        }
        std::sort(footprint.begin(), footprint.end());
        footprint.erase(std::unique(footprint.begin(), footprint.end()), footprint.end());
        blockCounts.push_back(world.writes().size());
        footprints.push_back(std::move(footprint));
    }

    // 方块总量与朝向无关（只是坐标映射不同）。
    ASSERT_EQ(blockCounts.size(), 4u);
    for (size_t i = 1; i < blockCounts.size(); ++i) {
        EXPECT_EQ(blockCounts[i], blockCounts[0]);
    }

    // 南北向两两「长度沿 Z」、东西向两两「长度沿 X」；四个朝向的脚印两两不同
    // （North 正面朝 -Z、South 朝 +Z，West/East 正面朝 ∓X）。
    // Directions::horizontal() 的顺序是 {North, East, South, West}。
    EXPECT_NE(footprints[0], footprints[1]);
    EXPECT_NE(footprints[0], footprints[2]);
    EXPECT_NE(footprints[0], footprints[3]);
    EXPECT_NE(footprints[1], footprints[2]);
    EXPECT_NE(footprints[1], footprints[3]);
    EXPECT_NE(footprints[2], footprints[3]);
}