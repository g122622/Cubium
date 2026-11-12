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
 * @file GravityProcessorHeightmapTest.cpp
 * @brief GravityStructureProcessor 的高度图类型解析测试
 *
 * 处理器按**读取世界的视图**替换高度图类型：`WorldGenRegion`（生成阶段）只有
 * `*_WG` 高度图可用，运行时世界读的是玩家看到的非 WG 高度图。两者取值不同时，
 * 换错会让整块模板上下错位，因此这里用一个「按类型返回不同高度」的世界桩
 * 直接断言替换发生了。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/template/Template.hpp"

#include <array>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::world::chunk;
using namespace mc::world::gen::feature::template_;

namespace {

/// 生成阶段高度图返回的高度
constexpr i32 GENERATION_SURFACE_Y = 60;
/// 运行时高度图返回的高度（故意与生成阶段不同，避免「替换与否都通过」）
constexpr i32 RUNTIME_SURFACE_Y = 64;

/**
 * @brief 按高度图类型返回不同高度的运行时世界桩
 *
 * 只实现 getHeight 的三参版本：其它成员沿用 BaseTestWorld 的桩实现。
 */
class HeightmapProbeWorld final : public test::MapBackedTestWorld {
public:
    HeightmapProbeWorld()
        : MapBackedTestWorld(Difficulty::Normal)
    {}

    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/, HeightmapType type) const override
    {
        ++m_queries;
        m_lastType = type;
        switch (type) {
            case HeightmapType::WorldSurfaceWG:
            case HeightmapType::OceanFloorWG:
                return GENERATION_SURFACE_Y;
            case HeightmapType::WorldSurface:
            case HeightmapType::OceanFloor:
                return RUNTIME_SURFACE_Y;
            default:
                return 0;
        }
    }

    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/) const override
    {
        return m_lastType == HeightmapType::WorldSurfaceWG || m_lastType == HeightmapType::OceanFloorWG
            ? GENERATION_SURFACE_Y
            : RUNTIME_SURFACE_Y;
    }

    [[nodiscard]] HeightmapType lastType() const
    {
        return m_lastType;
    }

private:
    mutable HeightmapType m_lastType = HeightmapType::WorldSurfaceWG;
    mutable i32 m_queries = 0;
};

} // namespace

class GravityProcessorHeightmapTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }

    /// 构造一次处理器调用所需的全部参数，返回处理后的世界 Y
    static i32 processWorldY(GravityStructureProcessor& processor, IWorld* world)
    {
        PlacementSettings settings;
        settings.setWorld(world);

        BlockInfo rawBlockInfo;
        rawBlockInfo.pos = BlockPos(1, 0, 1);
        rawBlockInfo.blockStateId = VanillaBlocks::STONE->defaultState().stateId();
        BlockInfo blockInfo = rawBlockInfo;

        const auto processed =
            processor.process(BlockPos(0, 0, 0), BlockPos(1, 0, 1), rawBlockInfo, blockInfo, settings);
        EXPECT_TRUE(processed.has_value());
        return processed.has_value() ? processed->pos.y : -999;
    }
};

TEST_F(GravityProcessorHeightmapTest, RuntimeWorldSwapsGenerationHeightmap)
{
    // 运行时世界（非 WorldGenRegion）：WorldSurfaceWG 必须换成 WorldSurface。
    HeightmapProbeWorld world;
    GravityStructureProcessor processor(HeightmapType::WorldSurfaceWG, 0);
    const i32 worldY = processWorldY(processor, &world);

    EXPECT_EQ(world.lastType(), HeightmapType::WorldSurface);
    // getHeight 返回「最高方块 Y」，模板放置高度是它 +1。
    EXPECT_EQ(worldY, RUNTIME_SURFACE_Y + 1);
}

TEST_F(GravityProcessorHeightmapTest, RuntimeWorldSwapsOceanFloorHeightmap)
{
    HeightmapProbeWorld world;
    GravityStructureProcessor processor(HeightmapType::OceanFloorWG, -1);
    const i32 worldY = processWorldY(processor, &world);

    EXPECT_EQ(world.lastType(), HeightmapType::OceanFloor);
    EXPECT_EQ(worldY, RUNTIME_SURFACE_Y + 1 - 1);
}

TEST_F(GravityProcessorHeightmapTest, NonWgTypesArePassedThrough)
{
    // 已经是非 WG 的类型在运行时世界下原样使用。
    HeightmapProbeWorld world;
    GravityStructureProcessor processor(HeightmapType::MotionBlocking, 0);
    processWorldY(processor, &world);

    EXPECT_EQ(world.lastType(), HeightmapType::MotionBlocking);
}

TEST_F(GravityProcessorHeightmapTest, GenerationViewKeepsWgHeightmap)
{
    // 生成阶段视图（WorldGenRegion）：保持调用方给的 WG 类型不变。
    std::vector<std::unique_ptr<ChunkPrimer>> owned;
    std::vector<IChunk*> chunks;
    owned.push_back(std::make_unique<ChunkPrimer>(0, 0));
    chunks.push_back(owned.back().get());
    WorldGenRegion region(0, 0, 0, std::move(chunks));

    GravityStructureProcessor processor(HeightmapType::WorldSurfaceWG, 0);
    // WorldGenRegion 没有可注入的高度图，这里只断言不会因为替换而崩溃且结果确定。
    PlacementSettings settings;
    settings.setWorld(&region);
    BlockInfo rawBlockInfo;
    rawBlockInfo.pos = BlockPos(1, 0, 1);
    rawBlockInfo.blockStateId = VanillaBlocks::STONE->defaultState().stateId();
    BlockInfo blockInfo = rawBlockInfo;

    const auto processed = processor.process(BlockPos(0, 0, 0), BlockPos(1, 0, 1), rawBlockInfo, blockInfo, settings);
    EXPECT_TRUE(processed.has_value());
}
