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

// ============================================================================
// WorldGenRegion 区块访问窗口验证测试
//
// 复现跑图时出现的两类错误：
// 1. "missing chunk in access window" — 区块指针在窗口内为 nullptr
// 2. "chunk status below request" — 区块状态低于请求状态
//
// 这些错误发生在带 ChunkStep 校验模式的 WorldGenRegion 中，
// 当 getIChunk(x, z, requestedStatus) 被调用时进行三项检查：
//   (a) 请求状态 <= 该距离上的 allowedStatus
//   (b) 区块指针非空
//   (c) 区块实际状态 >= requestedStatus
// ============================================================================

#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <set>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

namespace {

// ============================================================================
// 辅助：创建指定数量 ChunkPrimer 并设置指定状态
// ============================================================================
struct PrimerPack {
    std::vector<std::unique_ptr<ChunkPrimer>> primers;
    std::vector<IChunk*> ptrs;
};

/**
 * @brief 创建 (2*radius+1)^2 个 ChunkPrimer，全部设为指定状态
 */
PrimerPack createPrimersAllStatus(ChunkCoord mainX, ChunkCoord mainZ, i32 radius, const ChunkStatus& status)
{
    const i32 diameter = radius * 2 + 1;
    PrimerPack pack;
    pack.primers.reserve(static_cast<size_t>(diameter * diameter));
    pack.ptrs.reserve(static_cast<size_t>(diameter * diameter));

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            auto primer = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);
            primer->setChunkStatus(status);
            pack.ptrs.push_back(primer.get());
            pack.primers.push_back(std::move(primer));
        }
    }
    return pack;
}

/**
 * @brief 创建 (2*radius+1)^2 个 ChunkPrimer，全部保持默认 EMPTY 状态
 */
PrimerPack createPrimersAllEmpty(ChunkCoord mainX, ChunkCoord mainZ, i32 radius)
{
    const i32 diameter = radius * 2 + 1;
    PrimerPack pack;
    pack.primers.reserve(static_cast<size_t>(diameter * diameter));
    pack.ptrs.reserve(static_cast<size_t>(diameter * diameter));

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            auto primer = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);
            pack.ptrs.push_back(primer.get());
            pack.primers.push_back(std::move(primer));
        }
    }
    return pack;
}

/**
 * @brief 创建带 nullptr 缺口的 PrimerPack
 * @param nullOffsets 相对 (mainX, mainZ) 的偏移列表，这些位置的指针设为 nullptr
 */
PrimerPack createPrimersWithNulls(ChunkCoord mainX,
    ChunkCoord mainZ,
    i32 radius,
    const ChunkStatus& status,
    const std::vector<std::pair<i32, i32>>& nullOffsets)
{
    const i32 diameter = radius * 2 + 1;
    PrimerPack pack;
    pack.primers.reserve(static_cast<size_t>(diameter * diameter));
    pack.ptrs.reserve(static_cast<size_t>(diameter * diameter));

    // 预计算空位集合
    std::set<std::pair<i32, i32>> nullSet(nullOffsets.begin(), nullOffsets.end());

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            if (nullSet.count({dx, dz})) {
                pack.ptrs.push_back(nullptr);
                pack.primers.push_back(nullptr); // 保持索引对齐
            } else {
                auto primer = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);
                primer->setChunkStatus(status);
                pack.ptrs.push_back(primer.get());
                pack.primers.push_back(std::move(primer));
            }
        }
    }
    return pack;
}

/**
 * @brief 创建混合状态的 PrimerPack — 中心区域为较高状态，外围为较低状态
 * @param innerRadius 内圈半径（含），内圈使用 innerStatus
 * @param outerStatus 外圈使用 outerStatus
 */
PrimerPack createPrimersMixedStatus(ChunkCoord mainX,
    ChunkCoord mainZ,
    i32 radius,
    i32 innerRadius,
    const ChunkStatus& innerStatus,
    const ChunkStatus& outerStatus)
{
    const i32 diameter = radius * 2 + 1;
    PrimerPack pack;
    pack.primers.reserve(static_cast<size_t>(diameter * diameter));
    pack.ptrs.reserve(static_cast<size_t>(diameter * diameter));

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const i32 dist = std::max(std::abs(dx), std::abs(dz));
            auto primer = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);
            primer->setChunkStatus(dist <= innerRadius ? innerStatus : outerStatus);
            pack.ptrs.push_back(primer.get());
            pack.primers.push_back(std::move(primer));
        }
    }
    return pack;
}

// ============================================================================
// 全局 Primer 存储（防止 unique_ptr 在 region 使用期间被释放）
// ============================================================================
static std::vector<std::unique_ptr<ChunkPrimer>> g_primerStorage;

} // anonymous namespace

// ============================================================================
// 测试夹具
// ============================================================================

class WorldGenRegionAccessTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    void TearDown() override { g_primerStorage.clear(); }
};

// ============================================================================
// 1. 验证 ChunkStep 依赖结构（先确认依赖定义正确）
// ============================================================================

TEST_F(WorldGenRegionAccessTest, ChunkStep_StructureReferences_DirectDeps)
{
    // STRUCTURE_REFERENCES 的直接依赖：半径 0-8 全部要求 STRUCTURE_STARTS
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const ChunkDependencies& deps = step.directDependencies();

    // 半径应为 8
    EXPECT_EQ(deps.getRadius(), 8);
    EXPECT_EQ(step.accumulatedRadius(), 8);

    // 半径 0-8 全部要求 STRUCTURE_STARTS
    for (i32 r = 0; r <= 8; ++r) {
        const ChunkStatus* status = deps.get(r);
        ASSERT_NE(status, nullptr) << "radius " << r << " should have a dependency";
        EXPECT_EQ(*status, ChunkStatuses::STRUCTURE_STARTS) << "radius " << r << " should require STRUCTURE_STARTS";
    }

    // 半径 9+ 应返回 nullptr
    EXPECT_EQ(deps.get(9), nullptr);
}

TEST_F(WorldGenRegionAccessTest, ChunkStep_Features_DirectDeps)
{
    // FEATURES 直接依赖：半径 0=CARVERS, 1=CARVERS, 2-8=STRUCTURE_STARTS
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::FEATURES);
    const ChunkDependencies& deps = step.directDependencies();

    EXPECT_EQ(deps.getRadius(), 8);

    // 半径 0 和 1 要求 CARVERS
    ASSERT_NE(deps.get(0), nullptr);
    EXPECT_EQ(*deps.get(0), ChunkStatuses::CARVERS);
    ASSERT_NE(deps.get(1), nullptr);
    EXPECT_EQ(*deps.get(1), ChunkStatuses::CARVERS);

    // 半径 2-8 要求 STRUCTURE_STARTS
    for (i32 r = 2; r <= 8; ++r) {
        ASSERT_NE(deps.get(r), nullptr) << "radius " << r;
        EXPECT_EQ(*deps.get(r), ChunkStatuses::STRUCTURE_STARTS) << "radius " << r;
    }
}

// ============================================================================
// 2. 复现 "missing chunk in access window" 错误
//
// 错误场景：STRUCTURE_REFERENCES 步骤生成时，在 17x17 (radius=8) 窗口内
// 调用 getIChunk(ncx, ncz, STRUCTURE_STARTS)，但某些位置的 chunk 指针为 nullptr。
// 跑图日志中的典型错误：
//   [WorldGenRegion] missing chunk in access window:
//     requested=(-5, 3), center=(-7, -2), distance=5,
//     generatingStatus=structure_references, requestedStatus=structure_starts,
//     allowedStatus=structure_starts
// ============================================================================

TEST_F(WorldGenRegionAccessTest, MissingChunkInAccessWindow_StructureReferences_NullAtDistance5)
{
    // 复现日志：center=(-7, -2), requested=(-5, 3), distance=5
    // STRUCTURE_REFERENCES 的直接依赖半径 0-8 全部要求 STRUCTURE_STARTS
    // distance=5 在依赖范围内（allowedStatus=STRUCTURE_STARTS），但 chunk 为 nullptr
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const i32 radius = step.accumulatedRadius(); // 8
    const ChunkCoord mainX = -7;
    const ChunkCoord mainZ = -2;

    // 在 distance=5 的位置 (-5, 3) 即 relX=2, relZ=5 放入 nullptr
    auto pack = createPrimersWithNulls(
        mainX, mainZ, radius, ChunkStatuses::STRUCTURE_STARTS, {{2, 5}}); // relX=2, relZ=5 -> world(-5, 3)

    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 调用 getIChunk 请求 (-5, 3) 处的 STRUCTURE_STARTS 状态
    // 由于该位置 chunk 为 nullptr，应触发 "missing chunk in access window" 错误
    // MC_ASSERT_RELEASE_MSG 会终止程序，所以无法直接断言
    // 我们验证前置条件：allowedStatus 确实允许 STRUCTURE_STARTS（即允许访问）
    // 以及 getChunkAt 确实返回 nullptr（chunk 缺失）
    const i32 relX = -5 - mainX;                                   // 2
    const i32 relZ = 3 - mainZ;                                    // 5
    const i32 distance = std::max(std::abs(relX), std::abs(relZ)); // 5
    EXPECT_EQ(distance, 5);

    // 验证依赖允许访问 distance=5 的位置
    const ChunkStatus* allowedStatus = step.directDependencies().get(distance);
    ASSERT_NE(allowedStatus, nullptr) << "distance=5 should be in the dependency range";
    EXPECT_TRUE(ChunkStatuses::STRUCTURE_STARTS.isOrBefore(*allowedStatus))
        << "STRUCTURE_STARTS should be allowed at distance=5";

    // 验证该位置确实为 nullptr
    EXPECT_EQ(region->getChunkAt(relX, relZ), nullptr) << "Chunk at relX=2, relZ=5 should be nullptr";

    // 无步骤模式下 getIChunk 可以返回 nullptr 而不报错（用于验证基础访问）
    // 但带步骤模式时，下面这行会导致 MC_ASSERT_RELEASE 失败：
    //   region->getIChunk(-5, 3, ChunkStatuses::STRUCTURE_STARTS);
    // 这正是日志中报告的错误
}

TEST_F(WorldGenRegionAccessTest, MissingChunkInAccessWindow_StructureReferences_MultipleNulls)
{
    // 复现日志：center=(-10, -2)，多个 requested 位置缺失
    // requested=(-9,1),(-8,1),(-7,1),(-6,1),(-5,1),(-4,1),(-3,-5)...(-3,1)
    // 这些位置在 distance 3-7 范围内，均在 STRUCTURE_REFERENCES 的依赖范围内
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const i32 radius = step.accumulatedRadius();
    const ChunkCoord mainX = -10;
    const ChunkCoord mainZ = -2;

    // 将 distance 3-7 范围内的一列区块设为 nullptr
    // 世界坐标 (-3, -5) 到 (-3, 1) 对应 relX=7, relZ=-3..3
    std::vector<std::pair<i32, i32>> nullOffsets;
    for (i32 rz = -3; rz <= 3; ++rz) {
        nullOffsets.push_back({7, rz}); // relX=7 -> world(-3, mainZ+rz)
    }
    // 同时加入 distance 3-6 的区块
    // 世界 (-9,1) -> relX=1, relZ=3; (-8,1) -> relX=2, relZ=3; etc.
    for (i32 rx = 1; rx <= 5; ++rx) {
        nullOffsets.push_back({rx, 3}); // relX=1..5, relZ=3
    }

    auto pack = createPrimersWithNulls(mainX, mainZ, radius, ChunkStatuses::STRUCTURE_STARTS, nullOffsets);
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 验证这些位置在依赖范围内且为 nullptr
    for (const auto& [relX, relZ] : nullOffsets) {
        const i32 distance = std::max(std::abs(relX), std::abs(relZ));
        const ChunkStatus* allowedStatus = step.directDependencies().get(distance);
        // 所有这些位置都应在依赖范围内
        ASSERT_NE(allowedStatus, nullptr) << "relX=" << relX << " relZ=" << relZ << " distance=" << distance;
        EXPECT_TRUE(ChunkStatuses::STRUCTURE_STARTS.isOrBefore(*allowedStatus))
            << "relX=" << relX << " relZ=" << relZ << " should allow STRUCTURE_STARTS";
        // 且这些位置确实为 nullptr
        EXPECT_EQ(region->getChunkAt(relX, relZ), nullptr)
            << "relX=" << relX << " relZ=" << relZ << " should be nullptr";
    }
}

// ============================================================================
// 3. 复现 "chunk status below request" 错误
//
// 错误场景：FEATURES 步骤生成时，请求邻居区块的 CARVERS 状态，
// 但该区块只有 BIOMES 状态。跑图日志中的典型错误：
//   [WorldGenRegion] chunk status below request:
//     requested=(-6, 2), center=(-7, 2), distance=1,
//     generatingStatus=features, requestedStatus=carvers,
//     allowedStatus=carvers, actualStatus=biomes
// ============================================================================

TEST_F(WorldGenRegionAccessTest, ChunkStatusBelowRequest_Features_CarversNotReached)
{
    // 复现日志：center=(-7, 2), requested=(-6, 2), distance=1
    // FEATURES 的直接依赖：radius 0,1 = CARVERS, radius 2-8 = STRUCTURE_STARTS
    // distance=1 处 allowedStatus=CARVERS，但实际区块只到 BIOMES
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::FEATURES);
    const i32 radius = step.accumulatedRadius(); // 8
    const ChunkCoord mainX = -7;
    const ChunkCoord mainZ = 2;

    const i32 diameter = radius * 2 + 1;
    PrimerPack pack;
    pack.primers.reserve(static_cast<size_t>(diameter * diameter));
    pack.ptrs.reserve(static_cast<size_t>(diameter * diameter));

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const i32 dist = std::max(std::abs(dx), std::abs(dz));
            auto primer = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);

            if (dx == 1 && dz == 0) {
                // distance=1 的区块只有 BIOMES（应该有 CARVERS）
                // 世界坐标 (-6, 2) = relX=1, relZ=0
                primer->setChunkStatus(ChunkStatuses::BIOMES);
            } else if (dist <= 1) {
                primer->setChunkStatus(ChunkStatuses::CARVERS);
            } else {
                primer->setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
            }

            pack.ptrs.push_back(primer.get());
            pack.primers.push_back(std::move(primer));
        }
    }
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 验证 distance=1 处 allowedStatus = CARVERS
    const ChunkStatus* allowedStatus = step.directDependencies().get(1);
    ASSERT_NE(allowedStatus, nullptr);
    EXPECT_EQ(*allowedStatus, ChunkStatuses::CARVERS);

    // 验证该位置区块存在
    IChunk* chunk = region->getChunkAt(1, 0); // relX=1, relZ=0
    ASSERT_NE(chunk, nullptr);

    // 验证区块实际状态是 BIOMES（低于 CARVERS）
    const auto* primer = dynamic_cast<const ChunkPrimer*>(chunk);
    ASSERT_NE(primer, nullptr);
    EXPECT_EQ(primer->getChunkStatus(), ChunkStatuses::BIOMES);
    EXPECT_TRUE(ChunkStatuses::BIOMES.isBefore(ChunkStatuses::CARVERS));

    // getIChunk(-6, 2, CARVERS) 会触发 "chunk status below request"
    // region->getIChunk(-6, 2, ChunkStatuses::CARVERS); // MC_ASSERT_RELEASE
}

TEST_F(WorldGenRegionAccessTest, ChunkStatusBelowRequest_Noise_BiomesNotReached)
{
    // NOISE 步骤要求 radius 0=BIOMES，radius 1-8=STRUCTURE_STARTS
    // 如果 radius 0 的区块只有 STRUCTURE_REFERENCES 而没有 BIOMES
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::NOISE);
    const i32 radius = step.accumulatedRadius();
    const ChunkCoord mainX = 0;
    const ChunkCoord mainZ = 0;

    const i32 diameter = radius * 2 + 1;
    PrimerPack pack;
    pack.primers.reserve(static_cast<size_t>(diameter * diameter));
    pack.ptrs.reserve(static_cast<size_t>(diameter * diameter));

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            auto primer = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);

            if (dx == 0 && dz == 0) {
                // 中心区块只有 STRUCTURE_REFERENCES，缺少 BIOMES
                primer->setChunkStatus(ChunkStatuses::STRUCTURE_REFERENCES);
            } else {
                primer->setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
            }

            pack.ptrs.push_back(primer.get());
            pack.primers.push_back(std::move(primer));
        }
    }
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 验证 NOISE 步骤在 radius 0 要求 BIOMES
    const ChunkStatus* allowedCenter = step.directDependencies().get(0);
    ASSERT_NE(allowedCenter, nullptr);
    EXPECT_EQ(*allowedCenter, ChunkStatuses::BIOMES);

    // 验证中心区块只有 STRUCTURE_REFERENCES（< BIOMES）
    IChunk* center = region->getChunkAt(0, 0);
    ASSERT_NE(center, nullptr);
    const auto* primer = dynamic_cast<const ChunkPrimer*>(center);
    ASSERT_NE(primer, nullptr);
    EXPECT_EQ(primer->getChunkStatus(), ChunkStatuses::STRUCTURE_REFERENCES);
    EXPECT_TRUE(ChunkStatuses::STRUCTURE_REFERENCES.isBefore(ChunkStatuses::BIOMES));

    // getIChunk(0, 0, BIOMES) 会触发 "chunk status below request"
    // region->getIChunk(0, 0, ChunkStatuses::BIOMES); // MC_ASSERT_RELEASE
}

// ============================================================================
// 4. 正确访问场景（无错误）—— 验证正确构造的 region 可以正常访问
// ============================================================================

TEST_F(WorldGenRegionAccessTest, CorrectAccess_StructureReferences_AllChunksPresent)
{
    // STRUCTURE_REFERENCES 步骤，所有 17x17 区块都有 STRUCTURE_STARTS 状态
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const i32 radius = step.accumulatedRadius();
    const ChunkCoord mainX = 0;
    const ChunkCoord mainZ = 0;

    auto pack = createPrimersAllStatus(mainX, mainZ, radius, ChunkStatuses::STRUCTURE_STARTS);
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 遍历所有依赖范围内的区块，验证都能正常访问
    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const i32 distance = std::max(std::abs(dx), std::abs(dz));
            const ChunkStatus* allowedStatus = step.directDependencies().get(distance);
            if (allowedStatus == nullptr) {
                continue;
            }
            const IChunk* chunk = region->getIChunk(mainX + dx, mainZ + dz, *allowedStatus);
            ASSERT_NE(chunk, nullptr) << "chunk at (" << mainX + dx << "," << mainZ + dz << ") dist=" << distance;
        }
    }
}

TEST_F(WorldGenRegionAccessTest, CorrectAccess_Features_AllChunksCorrectStatus)
{
    // FEATURES 步骤，内圈 distance 0-1 = CARVERS，外圈 2-8 = STRUCTURE_STARTS
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::FEATURES);
    const i32 radius = step.accumulatedRadius();
    const ChunkCoord mainX = 0;
    const ChunkCoord mainZ = 0;

    auto pack =
        createPrimersMixedStatus(mainX, mainZ, radius, 1, ChunkStatuses::CARVERS, ChunkStatuses::STRUCTURE_STARTS);
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const i32 distance = std::max(std::abs(dx), std::abs(dz));
            const ChunkStatus* allowedStatus = step.directDependencies().get(distance);
            if (allowedStatus == nullptr) {
                continue;
            }
            const IChunk* chunk = region->getIChunk(mainX + dx, mainZ + dz, *allowedStatus);
            ASSERT_NE(chunk, nullptr) << "chunk at (" << mainX + dx << "," << mainZ + dz << ") dist=" << distance;
        }
    }
}

// ============================================================================
// 5. 验证访问越界（超出依赖范围）的检测
// ============================================================================

TEST_F(WorldGenRegionAccessTest, InvalidAccess_BeyondDependencyRange)
{
    // STRUCTURE_STARTS 只依赖 EMPTY at radius 0
    // accumulatedRadius=0，窗口只有 1x1
    // 请求任何非中心区块都应返回 nullptr (不在窗口内)
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_STARTS);
    const i32 radius = step.accumulatedRadius(); // 0
    const ChunkCoord mainX = 0;
    const ChunkCoord mainZ = 0;

    auto pack = createPrimersAllStatus(mainX, mainZ, radius, ChunkStatuses::EMPTY);
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 中心区块可以正常访问
    const IChunk* center = region->getIChunk(0, 0, ChunkStatuses::EMPTY);
    ASSERT_NE(center, nullptr);

    // 超出窗口范围的区块不存在
    EXPECT_EQ(region->getChunkAt(1, 0), nullptr);
    EXPECT_EQ(region->getChunkAt(0, 1), nullptr);
    EXPECT_EQ(region->getChunkAt(-1, 0), nullptr);
}

// ============================================================================
// 6. 模拟 generateStructureReferences 的完整访问模式
//    复现跑图时最密集的访问场景
// ============================================================================

TEST_F(WorldGenRegionAccessTest, StructureReferencesFullScan_AllChunksPresent)
{
    // 模拟 NoiseChunkGenerator::generateStructureReferences 的访问模式：
    // 遍历 dx=-8..8, dz=-8..8，对每个邻居调用
    //   region.getIChunk(ncx, ncz, ChunkStatuses::STRUCTURE_STARTS)
    // 这是跑图时触发 "missing chunk" 最多的场景
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const i32 radius = step.accumulatedRadius();
    const ChunkCoord mainX = -7;
    const ChunkCoord mainZ = -2;

    // 所有区块都有 STRUCTURE_STARTS，模拟正常情况
    auto pack = createPrimersAllStatus(mainX, mainZ, radius, ChunkStatuses::STRUCTURE_STARTS);
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 模拟 generateStructureReferences 的完整扫描
    i32 accessCount = 0;
    for (i32 dz = -8; dz <= 8; ++dz) {
        for (i32 dx = -8; dx <= 8; ++dx) {
            const ChunkCoord ncx = mainX + dx;
            const ChunkCoord ncz = mainZ + dz;
            const IChunk* chunk = region->getIChunk(ncx, ncz, ChunkStatuses::STRUCTURE_STARTS);
            ASSERT_NE(chunk, nullptr) << "generateStructureReferences scan: chunk at (" << ncx << "," << ncz
                                      << ") missing";
            ++accessCount;
        }
    }
    // 17x17 = 289 次访问
    EXPECT_EQ(accessCount, 289);
}

TEST_F(WorldGenRegionAccessTest, StructureReferencesFullScan_WithNullsAtBoundary)
{
    // 模拟跑图时边缘区块缺失的情况
    // 日志中 distance=7-8 的区块大量缺失
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const i32 radius = step.accumulatedRadius();
    const ChunkCoord mainX = -7;
    const ChunkCoord mainZ = -2;

    // 在 distance 7-8 处放入 nullptr，模拟边缘区块未就绪
    std::vector<std::pair<i32, i32>> nullOffsets;
    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const i32 dist = std::max(std::abs(dx), std::abs(dz));
            if (dist >= 7) {
                nullOffsets.push_back({dx, dz});
            }
        }
    }

    auto pack = createPrimersWithNulls(mainX, mainZ, radius, ChunkStatuses::STRUCTURE_STARTS, nullOffsets);
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 验证所有 distance 7-8 位置的区块为 nullptr
    i32 nullCount = 0;
    for (const auto& [relX, relZ] : nullOffsets) {
        EXPECT_EQ(region->getChunkAt(relX, relZ), nullptr)
            << "distance >= 7 chunk at relX=" << relX << " relZ=" << relZ << " should be nullptr";
        ++nullCount;
    }
    EXPECT_GT(nullCount, 0) << "Should have some null chunks at boundary";

    // 验证 distance 0-6 的区块仍然存在
    for (i32 dz = -6; dz <= 6; ++dz) {
        for (i32 dx = -6; dx <= 6; ++dx) {
            if (std::max(std::abs(dx), std::abs(dz)) <= 6) {
                EXPECT_NE(region->getChunkAt(dx, dz), nullptr)
                    << "distance <= 6 chunk at relX=" << dx << " relZ=" << dz << " should exist";
            }
        }
    }

    // 在正常代码中，generateStructureReferences 遍历 -8..8 会访问这些 nullptr 区块
    // 对 distance >= 7 的位置调用 getIChunk 会触发 "missing chunk in access window"
}

// ============================================================================
// 7. 跑图日志精确复现：center=(-7, -2), distance=5,7 缺失
// ============================================================================

TEST_F(WorldGenRegionAccessTest, ReproduceLog_CenterNeg7Neg2_Distance5Missing)
{
    // 精确复现日志第一组错误：
    //   requested=(-5, 3), center=(-7, -2), distance=5
    //   requested=(-4, 3), center=(-7, -2), distance=5
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const i32 radius = step.accumulatedRadius();
    const ChunkCoord mainX = -7;
    const ChunkCoord mainZ = -2;

    // 在 distance=5 的位置 (-5,3) 和 (-4,3) 放入 nullptr
    // relX = -5 - (-7) = 2, relZ = 3 - (-2) = 5
    // relX = -4 - (-7) = 3, relZ = 3 - (-2) = 5
    auto pack = createPrimersWithNulls(mainX, mainZ, radius, ChunkStatuses::STRUCTURE_STARTS, {{2, 5}, {3, 5}});
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 精确验证日志中的两个坐标
    // requested=(-5, 3): relX=2, relZ=5, distance=5
    EXPECT_EQ(region->getChunkAt(2, 5), nullptr);
    EXPECT_NE(step.directDependencies().get(5), nullptr); // allowedStatus 存在

    // requested=(-4, 3): relX=3, relZ=5, distance=5
    EXPECT_EQ(region->getChunkAt(3, 5), nullptr);
}

TEST_F(WorldGenRegionAccessTest, ReproduceLog_CenterNeg7Neg2_Distance7Missing)
{
    // 精确复现日志第二组错误：
    //   center=(-7, -2), distance=7
    //   requested=(0, -4),(0, -3),(0, -2),(0, -1),(0, 0),(0, 1),(0, 2)
    //   relX = 0 - (-7) = 7, relZ = z - (-2)
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const i32 radius = step.accumulatedRadius();
    const ChunkCoord mainX = -7;
    const ChunkCoord mainZ = -2;

    // 在 relX=7, relZ=-2..2 放入 nullptr
    std::vector<std::pair<i32, i32>> nullOffsets;
    for (i32 rz = -2; rz <= 4; ++rz) {
        nullOffsets.push_back({7, rz});
    }

    auto pack = createPrimersWithNulls(mainX, mainZ, radius, ChunkStatuses::STRUCTURE_STARTS, nullOffsets);
    g_primerStorage = std::move(pack.primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(pack.ptrs), 0);

    // 验证日志中的坐标
    // requested=(0, -4): relX=7, relZ=-2, distance=7
    // requested=(0, -3): relX=7, relZ=-1, distance=7
    // requested=(0, -2): relX=7, relZ=0, distance=7
    // ... 到 requested=(0, 2): relX=7, relZ=4, distance=7
    const std::vector<std::pair<i32, i32>> loggedPositions = {
        {0, -4}, {0, -3}, {0, -2}, {0, -1}, {0, 0}, {0, 1}, {0, 2}};

    for (const auto& [wx, wz] : loggedPositions) {
        const i32 relX = wx - mainX;
        const i32 relZ = wz - mainZ;
        const i32 distance = std::max(std::abs(relX), std::abs(relZ));
        EXPECT_EQ(distance, 7) << "world(" << wx << "," << wz << ") should be at distance 7";
        EXPECT_EQ(region->getChunkAt(relX, relZ), nullptr) << "world(" << wx << "," << wz << ") should be nullptr";
        EXPECT_NE(step.directDependencies().get(distance), nullptr) << "distance=7 should have allowedStatus";
    }
}
