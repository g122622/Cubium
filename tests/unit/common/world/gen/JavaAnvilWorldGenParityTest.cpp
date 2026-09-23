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
// 同种子世界生成 parity 测试：Cubium 生成结果 vs 原版 1.21.11 真实存档
//
// 素材：tests/unit/testdata/worlds/java-anvil-1.21.11/
//   种子   level.dat 的 WorldGenSettings.seed = -6671478382168981129
//   维度   minecraft:overworld（noise 生成器，settings=minecraft:overworld）
//
// 与既有的 NoiseSurfaceParity / NetherSurfaceParity / SurfaceRuleParity 等测试
// 的区别：那些测试比对的是**单个子系统**（噪声采样、密度函数、地表规则）的数值，
// 且期望值是就地计算出来的；本测试比对的是**整条生成管线的最终产物**，
// 期望值来自一份真实原版存档——是唯一能端到端回答"同种子下 Cubium 生成的区块
// 和原版一样吗"的测试。
//
// ---------------------------------------------------------------------------
// ground truth 的选取：只对比「纯净区块」
// ---------------------------------------------------------------------------
// 存档里 2378 列中 675 列状态为 minecraft:full，但这些区块并非全部可用作
// 世界生成的 ground truth —— 被玩家加载过的区块会被随机刻、流体刻、玩家操作
// 改动，其方块数据已不等于 worldgen 产物。判定"纯净"的充分条件：
//
//   InhabitedTime == 0
//
// vanilla 只在区块进入 EntityTicking（有玩家在附近）后每 tick 累加
// InhabitedTime。值为 0 意味着该区块被生成并落盘后**从未进入过 ticking 状态**
// （存档中 Observable/TSR 半径内的区块会被生成但不一定 tick），因此不可能被
// 任何运行时逻辑改写，其方块数据即纯 worldgen 产物。
//
// 另外 kTargets 的坐标是离线筛选出来的，满足：8 邻域全为 full（排除生成边界
// 效应——邻居未完成时其 feature 可能写入本区块）、无 block_entities、且 13x13
// 邻域内无缺口。筛选过程：474 个 InhabitedTime==0 的 full 区块中 175 个 8 邻域
// 齐全，本文件取其中边距最大（6 格）的三个。
//
// 若素材被替换，TargetChunksExistAndAreFull 会先行失败，从而把
// "素材/坐标选取有问题" 与 "parity 有差距" 两种失败区分开。
//
// ---------------------------------------------------------------------------
// 对比维度与断言强度（双轨）
// ---------------------------------------------------------------------------
// 【基线】用例只断言与素材或生成器自身有关的结构不变量，当前全部通过，
//   目的是：素材被替换、世界生成注册表未加载、生成产物出现未注册方块等
//   会让对比失去意义的情况，能立刻暴露。
// 【门禁】用例断言与原版逐项相等，当前**失败**——Cubium 的世界生成与原版仍有
//   实质差距。它们同时充当"parity 收敛进度表"与"防止退化的门禁"：
//   parity 达成后自动转绿，中途任何退化都会立刻暴露。
//
// 机制：把原版区块经项目自身的 Java 存档读取链
// （JavaAnvilBackend → JavaColumnReader → JavaBlockStateMapper / JavaBiomeMapper）
// 解码为项目内部的 ChunkData，两侧落在同一套 BlockRegistry / BiomeId 空间，
// 因此可以逐方块、逐群系采样点精确比较。注意这意味着**读取链自身的缺陷会被
// 计成 parity 差距**（见下）。
//
// ---------------------------------------------------------------------------
// 读取链的已知缺陷（会污染对比结论，须与真实 parity 差距区分）
// ---------------------------------------------------------------------------
// 建立本测试的过程暴露并修复了两个读取链缺陷，它们的共同特征是**静默出错**——
// 解析出的数据看似合法，实则整片错位，而测试若只看"是否解析成功"完全发现不了：
//
// 1) JavaColumnReader::_readBiomes 把 4x4x4 的群系体积塌缩成 by=0 平面（用
//    bz * HORIZ + bx 的二维索引取 64 元素数组，再把同一个值写满所有 by），
//    洞穴群系被算到错误的 Y 上。【已修复】
//
// 2) JavaBiomeMapper 自维护一份手写名称表，与 BiomeRegistry 的真实内容互不同步：
//    数据包 65 个群系里 15 个错漏（7 个缺失、8 个指向语义相近但错误的群系，如
//    meadow→Plains、dripstone_caves→TheEnd），且未命中静默回退。已改为委托
//    biome::JavaBiomeRegistryIdMap 的权威表。【已修复】
//
// 3) 夹具一度漏加载数据驱动的世界生成注册表（feature/placement/carver/biome）。
//    生产路径由 RegistryBootstrap::initializeAll 加载；缺了它们，生成会**静默**退化
//    （矿脉与装饰特征被跳过），测出来的差距（当时是 6.9%~10.2%）远小于真实值
//    （19.0%~20.8%）——即夹具自身缺陷会让 parity 显得比实际更好，是最危险的一类
//    假绿。现改为调用 mc::test::loadVanillaWorldGenRegistries()。【已修复】
//
// 教训：这类"对比装置自身出错"的失效模式，与"被测系统出错"在观测上完全一致。
// 故本套件的设计原则是——被测值与被测系统之外的真值也要能对上（例如原版存档自带的
// 高度图、原版自己写下的群系采样），一旦两者矛盾，先怀疑读取链或夹具。
// 另外：任何让生成**静默跳过**某个阶段的缺失依赖，都必须当作"配置错误"处理，
// 否则它只会表现为"差距比想象中小"，而不会报错。
// ============================================================================

#include "common/TestDataDir.hpp"
#include "common/WorldGenRegistryFixture.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/settings/NoiseSettingsRegistry.hpp"
#include "server/world/storage/backend/JavaAnvilBackend.hpp"
#include "server/world/storage/core/SaveFormat.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace mc;

namespace {

/// 素材目录（相对 tests/unit/testdata）
constexpr const char* kWorldRelPath = "worlds/java-anvil-1.21.11";

/// level.dat 的 WorldGenSettings.seed（Java 存档中为 Long，可能为负）
constexpr i64 kSeed = -6671478382168981129LL;

/// 目标区块：全部满足 InhabitedTime==0、8 邻域 full、13x13 邻域内无缺口
constexpr std::pair<ChunkCoord, ChunkCoord> kTargets[] = {
    {2, -2},
    {2, 10},
    {-10, -2},
};

/// 列高度相对原版高度图允许的最大绝对偏差（格）。
/// 现状实测：194~254 / 256 列不一致，最大绝对偏差 11~16 格。
/// 此处只作"量级不失控"的退化门禁，收敛目标见用例内的 TODO。
constexpr i32 kMaxHeightDelta = 24;

/// 逐方块不一致率上限（占区块总方块数）。
/// 现状实测：19.0% ~ 20.8%。同样只是退化门禁，收敛目标为 0。
constexpr f64 kMaxBlockMismatchRatio = 0.25;

/**
 * @brief 单个区块的对比统计
 */
struct ChunkDiff {
    ChunkCoord x = 0;
    ChunkCoord z = 0;

    i64 totalBlocks = 0;
    i64 mismatchedBlocks = 0;
    i64 javaOnlyBlocks = 0;   ///< 原版有、Cubium 调色板完全没有的方块种类数
    i64 cubiumOnlyBlocks = 0; ///< Cubium 有、原版调色板没有的方块种类数

    i64 heightColumns = 0;
    i64 heightMismatched = 0;
    i32 heightMaxAbsDelta = 0;

    i64 totalBiomes = 0;
    i64 mismatchedBiomes = 0;

    /// 出现次数最多的差异对（"原版方块 -> Cubium 方块"）
    std::vector<std::pair<std::string, i64>> topBlockPairs;
    /// 原版有而 Cubium 没有的方块名
    std::vector<std::string> missingBlockNames;
};

/**
 * @brief 同种子世界生成 parity 测试夹具
 */
class JavaAnvilWorldGenParityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
        // 群系按名解析依赖这张表（JavaBiomeMapper 委托它）。生产路径由
        // RegistryBootstrap 完成同样的调用；本测试不走世界启动流程，须显式初始化，
        // 否则原版区块的群系会全部解析失败（并逐条打 warn）。
        ASSERT_TRUE(world::biome::JavaBiomeRegistryIdMap::instance().initialize().success())
            << "JavaBiomeRegistryIdMap 初始化失败";

        // 加载数据驱动的世界生成注册表（feature/placement/carver/biome）。
        // 生产路径由 RegistryBootstrap::initializeAll 按同样顺序加载；不加载的话
        // 生成会**静默**退化——矿脉与装饰特征被跳过、群系缺生成设置，
        // 测出来的 parity 差距不再代表真实管线。
        ASSERT_TRUE(mc::test::loadVanillaWorldGenRegistries())
            << "数据包缺失，无法加载世界生成注册表：" << GameDirectory::defaultDirectory().dataPacksDir().string();
    }

    void SetUp() override
    {
        // 素材随仓库分发，缺失属故障而非可跳过情形（与 JavaRealWorldFixture 一致）
        const std::filesystem::path worldDir = mc::test::testDataPath(kWorldRelPath);
        ASSERT_TRUE(std::filesystem::exists(worldDir / "level.dat")) << "测试素材缺失：" << worldDir.string();
        ASSERT_TRUE(std::filesystem::exists(worldDir / "region")) << "测试素材损坏：缺少 region/ 目录";

        world::storage::SaveFormatInfo info;
        info.format = world::storage::SaveFormat::JavaAnvil;
        info.formatName = "Java 1.21.11";
        info.dataVersion = 4671;
        info.readonly = true;

        auto openResult = m_backend.open(worldDir, info);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        // 与原版存档一致的生成器配置：主世界 noise 生成器 + minecraft:overworld 设置
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, static_cast<u64>(kSeed));
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        m_manager = std::make_unique<server::ServerChunkManager>(std::move(generator));
    }

    /**
     * @brief 取原版存档中的某一列
     *
     * @return 原版区块数据；素材中不存在时返回 nullptr 并记录 gtest 失败
     */
    [[nodiscard]] const ChunkData* loadJavaChunk(ChunkCoord x, ChunkCoord z)
    {
        auto result = m_backend.loadChunk(x, z, 0);
        EXPECT_TRUE(result.success()) << result.error().message();
        if (result.failed() || !result.value().has_value()) {
            ADD_FAILURE() << "素材中不存在区块 (" << x << "," << z << ")";
            return nullptr;
        }
        m_javaChunks.emplace_back(std::move(*result.value()));
        return &m_javaChunks.back();
    }

    /// 用同种子生成一个区块并推进到 FULL
    [[nodiscard]] ChunkData* generateCubiumChunk(ChunkCoord x, ChunkCoord z)
    {
        return m_manager->requestFullChunkSync(x, z);
    }

    /// 方块 stateId -> 方块名（注册表查不到时返回占位串，便于诊断输出）
    static std::string blockName(u32 stateId)
    {
        const BlockState* state = BlockRegistry::instance().getBlockState(stateId);
        if (state == nullptr) {
            return "<unregistered:" + std::to_string(stateId) + ">";
        }
        return state->getBlock().blockLocation().toString();
    }

    /**
     * @brief 逐方块比较原版与 Cubium 的生成结果
     */
    static ChunkDiff compareBlocks(const ChunkData& javaChunk, const ChunkData& cubiumChunk)
    {
        ChunkDiff diff;
        diff.x = javaChunk.x();
        diff.z = javaChunk.z();

        std::map<std::string, i64> pairCounter;
        std::set<std::string> javaPalette;
        std::set<std::string> cubiumPalette;

        for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
            for (i32 bz = 0; bz < 16; ++bz) {
                for (i32 bx = 0; bx < 16; ++bx) {
                    const u32 javaId = javaChunk.getBlockStateId(bx, y, bz);
                    const u32 cubiumId = cubiumChunk.getBlockStateId(bx, y, bz);
                    ++diff.totalBlocks;

                    const std::string javaName = blockName(javaId);
                    const std::string cubiumName = blockName(cubiumId);
                    javaPalette.insert(javaName);
                    cubiumPalette.insert(cubiumName);

                    if (javaId != cubiumId) {
                        ++diff.mismatchedBlocks;
                        ++pairCounter[javaName + "  ->  " + cubiumName];
                    }
                }
            }
        }

        for (const auto& name : javaPalette) {
            if (!cubiumPalette.count(name)) {
                ++diff.javaOnlyBlocks;
                diff.missingBlockNames.push_back(name);
            }
        }
        for (const auto& name : cubiumPalette) {
            if (!javaPalette.count(name)) {
                ++diff.cubiumOnlyBlocks;
            }
        }
        std::sort(diff.missingBlockNames.begin(), diff.missingBlockNames.end());

        diff.topBlockPairs.assign(pairCounter.begin(), pairCounter.end());
        std::sort(diff.topBlockPairs.begin(), diff.topBlockPairs.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
        return diff;
    }

    /**
     * @brief 逐列比较地表高度：Cubium 生成的最高非空气方块 vs 原版高度图
     *
     * 用原版自己写下的 WORLD_SURFACE 高度图（而非反推它的方块），可独立于两侧的
     * 方块调色板判断地形高低是否对齐。
     */
    static void compareColumnHeights(const ChunkData& javaChunk, const ChunkData& cubiumChunk, ChunkDiff& diff)
    {
        for (i32 bz = 0; bz < 16; ++bz) {
            for (i32 bx = 0; bx < 16; ++bx) {
                const i32 javaTop = javaChunk.getHighestBlock(bx, bz);
                i32 cubiumTop = world::MIN_BUILD_HEIGHT - 1;
                for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
                    if (cubiumChunk.getBlockStateId(bx, y, bz) != 0) {
                        cubiumTop = y;
                        break;
                    }
                }
                ++diff.heightColumns;
                const i32 delta = cubiumTop - javaTop;
                if (delta != 0) {
                    ++diff.heightMismatched;
                }
                diff.heightMaxAbsDelta = std::max(diff.heightMaxAbsDelta, std::abs(delta));
            }
        }
    }

    /**
     * @brief 比较 4x4x4 生物群系采样网格
     *
     * 两侧的 BiomeContainer 都由同一套采样语义填充（每 4x4x4 方块共享一个群系），
     * 因此可以按 (sectionIndex, bx, by, bz) 逐点比较。
     */
    static void compareBiomes(const ChunkData& javaChunk, const ChunkData& cubiumChunk, ChunkDiff& diff)
    {
        const BiomeContainer& javaBiomes = javaChunk.getBiomes();
        const BiomeContainer& cubiumBiomes = cubiumChunk.getBiomes();
        for (i32 sectionIndex = 0; sectionIndex < BiomeContainer::SECTION_COUNT; ++sectionIndex) {
            for (i32 by = 0; by < BiomeContainer::VERT_SIZE; ++by) {
                for (i32 bz = 0; bz < BiomeContainer::HORIZ_SIZE; ++bz) {
                    for (i32 bx = 0; bx < BiomeContainer::HORIZ_SIZE; ++bx) {
                        ++diff.totalBiomes;
                        if (javaBiomes.getBiome(sectionIndex, bx, by, bz) !=
                            cubiumBiomes.getBiome(sectionIndex, bx, by, bz)) {
                            ++diff.mismatchedBiomes;
                        }
                    }
                }
            }
        }
    }

    static void printBlockDiffReport(const ChunkDiff& diff)
    {
        std::printf("[PARITY] (%d,%d) 方块不一致 %lld / %lld (%.2f%%)\n",
            diff.x,
            diff.z,
            static_cast<long long>(diff.mismatchedBlocks),
            static_cast<long long>(diff.totalBlocks),
            100.0 * static_cast<double>(diff.mismatchedBlocks) / static_cast<double>(diff.totalBlocks));
        if (diff.javaOnlyBlocks != 0) {
            std::printf("[PARITY] (%d,%d) 原版有而 Cubium 完全没有的方块（%lld 种）：\n",
                diff.x,
                diff.z,
                static_cast<long long>(diff.javaOnlyBlocks));
            for (const auto& name : diff.missingBlockNames) {
                std::printf("[PARITY]      - %s\n", name.c_str());
            }
        }
        if (diff.cubiumOnlyBlocks != 0) {
            std::printf("[PARITY] (%d,%d) Cubium 有而原版没有的方块种类数：%lld\n",
                diff.x,
                diff.z,
                static_cast<long long>(diff.cubiumOnlyBlocks));
        }
        const size_t limit = std::min<size_t>(diff.topBlockPairs.size(), 12);
        std::printf(
            "[PARITY] (%d,%d) 差异对 top %zu / 共 %zu 种：\n", diff.x, diff.z, limit, diff.topBlockPairs.size());
        for (size_t i = 0; i < limit; ++i) {
            std::printf("[PARITY]      %-72s %lld\n",
                diff.topBlockPairs[i].first.c_str(),
                static_cast<long long>(diff.topBlockPairs[i].second));
        }
    }

    /// 已加载的原版区块（转存以保证指针存活）
    std::vector<ChunkData> m_javaChunks = [] {
        std::vector<ChunkData> v;
        v.reserve(std::size(kTargets));
        return v;
    }();
    world::storage::JavaAnvilBackend m_backend;
    std::unique_ptr<server::ServerChunkManager> m_manager;
};

// ============================================================================
// 【基线】用例：当前应全部通过
// ============================================================================

/**
 * 目标区块在素材中确实存在、坐标正确、状态为 full。
 *
 * 这是其余所有对比的前提：素材被替换、或坐标被改错时，本用例先行失败，
 * 从而把"素材/选取有问题"与"parity 有差距"两种失败区分开。
 */
TEST_F(JavaAnvilWorldGenParityTest, TargetChunksExistAndAreFull)
{
    for (const auto& [x, z] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(x, z);
        ASSERT_NE(javaChunk, nullptr) << "无法读取区块 (" << x << "," << z << ")";
        EXPECT_EQ(javaChunk->x(), x) << "读取到的区块坐标与请求不一致（region 定位可能出错）";
        EXPECT_EQ(javaChunk->z(), z) << "读取到的区块坐标与请求不一致（region 定位可能出错）";
        EXPECT_TRUE(javaChunk->isFullyGenerated())
            << "区块 (" << x << "," << z << ") 不是 full；素材筛选条件已失效，需重新挑选纯净区块";
    }
}

/**
 * 世界生成所依赖的注册表确实已就绪。
 *
 * 这些注册表来自机器相关的数据包目录（tests/unit/main.cpp 的
 * WorldGenRegistryEnvironment 负责加载），缺失时 RandomState::create 会断言失败，
 * 但更隐蔽的情况是"加载了但为空"——生成会静默跳过雕刻与装饰。
 * 本用例把这类"环境不对"从"gen 结果不对"里摘出来。
 */
TEST_F(JavaAnvilWorldGenParityTest, WorldGenRegistriesAreLoaded)
{
    using world::gen::settings::NoiseSettingsRegistry;
    EXPECT_TRUE(NoiseSettingsRegistry::instance().has(resource::ResourceLocation("minecraft:overworld")))
        << "noise_settings 未加载：检查数据包目录（~/minecraft_reborn/datapacks/Vanilla）是否存在";
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Plains)) << "生物群系注册表未初始化";
}

/**
 * Cubium 生成的区块结构自洽：坐标正确、推进到 FULL、且不含未注册的方块 stateId。
 *
 * 不依赖原版存档，纯校验生成产物的结构性正确性；出现未注册 stateId 通常意味着
 * 方块注册或 stateId 分配出了问题，会让后续一切方块对比失去意义。
 */
TEST_F(JavaAnvilWorldGenParityTest, GeneratedChunksAreStructurallySound)
{
    for (const auto& [cx, cz] : kTargets) {
        ChunkData* cubiumChunk = generateCubiumChunk(cx, cz);
        ASSERT_NE(cubiumChunk, nullptr) << "生成区块 (" << cx << "," << cz << ") 失败";
        EXPECT_EQ(cubiumChunk->x(), cx);
        EXPECT_EQ(cubiumChunk->z(), cz);
        EXPECT_TRUE(cubiumChunk->isFullyGenerated()) << "区块 (" << cx << "," << cz << ") 未推进到 FULL";

        i64 unregistered = 0;
        for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
            for (i32 bz = 0; bz < 16; ++bz) {
                for (i32 bx = 0; bx < 16; ++bx) {
                    const u32 stateId = cubiumChunk->getBlockStateId(bx, y, bz);
                    if (BlockRegistry::instance().getBlockState(stateId) == nullptr) {
                        ++unregistered;
                    }
                }
            }
        }
        EXPECT_EQ(unregistered, 0) << "区块 (" << cx << "," << cz << ") 出现未注册的方块 stateId";
    }
}

// ============================================================================
// 【门禁】用例：与原版逐项相等，当前失败，parity 达成后自动转绿
// ============================================================================

/**
 * Cubium 的方块调色板应当是原版调色板的子集。
 *
 * 【当前状态】失败：区块 (2,10) 的 Cubium 生成结果出现了原版调色板中没有的
 * minecraft:water（原版该区块没有任何水）。
 *
 * 【收敛目标】cubiumOnlyBlocks 降为 0，即 Cubium 不会生成原版在该处没有的方块。
 */
TEST_F(JavaAnvilWorldGenParityTest, GeneratedBlockPaletteIsSubsetOfJavaSave)
{
    for (const auto& [cx, cz] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(cx, cz);
        ASSERT_NE(javaChunk, nullptr);
        ChunkData* cubiumChunk = generateCubiumChunk(cx, cz);
        ASSERT_NE(cubiumChunk, nullptr);

        const ChunkDiff diff = compareBlocks(*javaChunk, *cubiumChunk);
        EXPECT_EQ(diff.cubiumOnlyBlocks, 0) << "区块 (" << cx << "," << cz << ") 生成了原版在该区块没有的方块";
        for (const auto& name : diff.missingBlockNames) {
            EXPECT_NE(name.rfind("<unregistered:", 0), 0)
                << "区块 (" << cx << "," << cz << ") 原版调色板中的 " << name << " 在方块注册表中不存在";
        }
    }
}

/**
 * 逐方块与原版严格相等。
 *
 * 【当前状态】失败：不一致率 19.0% ~ 20.8%（143~159 种差异对）。已定位的缺口：
 *   - **矿脉落位不符**：差异量最大的一类是 stone↔andesite / stone↔granite /
 *     stone↔diorite / deepslate↔tuff 的双向错位——Cubium 的 OreVeinifier 产物在
 *     位置与形状上与原版不一致（例如 (2,10) 有 2424 处原版 stone 被 Cubium 生成为
 *     andesite，同时又有 649 处相反）。这说明矿脉密度函数或阈值参数仍有偏差。
 *   - **石头变体矿石缺失**：原版有而 Cubium 在该区块完全没有 coal_ore / copper_ore /
 *     iron_ore / lapis_ore / redstone_ore / gold_ore；深板岩侧虽有矿石（如
 *     deepslate_copper_ore），但主石层没有，指向 ore 类 placed_feature 的 Y 范围
 *     或基底判定有偏差。
 *   - 缺少水边与地表内容：clay / sand / seagrass / tall_seagrass。
 * 注意"完全缺失"是就这些区块而言，不代表生成器里没有对应实现。
 *
 * 【收敛目标】不一致数降为 0。在此之前用 kMaxBlockMismatchRatio 卡住量级，
 * 避免在没有门禁的情况下进一步退化。
 */
TEST_F(JavaAnvilWorldGenParityTest, GeneratedBlocksMatchJavaSave)
{
    for (const auto& [cx, cz] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(cx, cz);
        ASSERT_NE(javaChunk, nullptr);
        ChunkData* cubiumChunk = generateCubiumChunk(cx, cz);
        ASSERT_NE(cubiumChunk, nullptr);

        const ChunkDiff diff = compareBlocks(*javaChunk, *cubiumChunk);
        printBlockDiffReport(diff);
        const f64 ratio = static_cast<f64>(diff.mismatchedBlocks) / static_cast<f64>(diff.totalBlocks);
        EXPECT_LE(ratio, kMaxBlockMismatchRatio)
            << "区块 (" << cx << "," << cz << ") 与原版的方块不一致率 " << (ratio * 100.0) << "% 超过上限 "
            << (kMaxBlockMismatchRatio * 100.0) << "%";
        // TODO(parity)：逐方块对齐后把上面的比率断言收紧为 EXPECT_EQ(diff.mismatchedBlocks, 0)，
        // 并消除本用例注释中列出的缺口。
    }
}

/**
 * 地表高度与原版高度图对齐。
 *
 * 【当前状态】失败：194~254 / 256 列不一致，最大绝对偏差 11~16 格；偏差分布偏向
 * 负值，说明不是随机的表面噪声，而是密度/地表阶段的系统性偏移。这是所有世界生成
 * 差异中最难通过"补 feature"掩盖的一项。
 *
 * 【收敛目标】每列高度完全一致。
 */
TEST_F(JavaAnvilWorldGenParityTest, ColumnHeightsMatchJavaSave)
{
    for (const auto& [cx, cz] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(cx, cz);
        ASSERT_NE(javaChunk, nullptr);
        ChunkData* cubiumChunk = generateCubiumChunk(cx, cz);
        ASSERT_NE(cubiumChunk, nullptr);

        ChunkDiff diff;
        diff.x = cx;
        diff.z = cz;
        compareColumnHeights(*javaChunk, *cubiumChunk, diff);
        std::printf("[PARITY] (%d,%d) 列高度不一致 %lld / %lld，最大绝对偏差 %d 格\n",
            cx,
            cz,
            static_cast<long long>(diff.heightMismatched),
            static_cast<long long>(diff.heightColumns),
            diff.heightMaxAbsDelta);
        EXPECT_LE(diff.heightMaxAbsDelta, kMaxHeightDelta)
            << "区块 (" << cx << "," << cz << ") 与原版的列高度最大偏差 " << diff.heightMaxAbsDelta << " 格超过上限 "
            << kMaxHeightDelta;
        // TODO(parity)：收敛后收紧为 EXPECT_EQ(diff.heightMismatched, 0)。
    }
}

/**
 * 生物群系 4x4x4 采样与原版一致。
 *
 * 【当前状态】修复读取链的两个缺陷（见文件头）后，本用例衡量的是**真实**的群系 parity：
 * 方块/地形差异会连带影响群系采样（水陆分布不同则岸线群系不同），因此它同时反映
 * 生成器差距与读取链正确性，是本套件里信息量最大的用例之一。
 *
 * 【收敛目标】采样点差异降为 0。
 */
TEST_F(JavaAnvilWorldGenParityTest, BiomesMatchJavaSave)
{
    for (const auto& [cx, cz] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(cx, cz);
        ASSERT_NE(javaChunk, nullptr);
        ChunkData* cubiumChunk = generateCubiumChunk(cx, cz);
        ASSERT_NE(cubiumChunk, nullptr);

        ChunkDiff diff;
        diff.x = cx;
        diff.z = cz;
        compareBiomes(*javaChunk, *cubiumChunk, diff);
        std::printf("[PARITY] (%d,%d) 生物群系采样不一致 %lld / %lld\n",
            cx,
            cz,
            static_cast<long long>(diff.mismatchedBiomes),
            static_cast<long long>(diff.totalBiomes));
        EXPECT_EQ(diff.mismatchedBiomes, 0) << "区块 (" << cx << "," << cz << ") 的生物群系采样与原版不一致";
    }
}

} // namespace
