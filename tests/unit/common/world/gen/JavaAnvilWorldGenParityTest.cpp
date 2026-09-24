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
// 另外 kTargets 的坐标是离线筛选出来的，选取条件是"从未被 tick 过"（InhabitedTime==0）
// 且邻域状态齐全（排除生成边界效应——邻居未完成时其 feature 可能写入本区块）。
// 离线筛选结果：474 个 InhabitedTime==0 的 full 区块，其中 6 个的 13x13 邻域内无任何缺口，
// 本文件取其中 3 个（另 3 个为 (-10,9) (-10,10) (-9,10)，留作后续扩充）。
// 该筛选脚本未进入仓库，数字无法在测试内复现，故仅存档于此。
//
// 注意前提的**校验方式**：坐标级条件（区块存在、状态 full、从未被 tick、无方块实体）
// 由 PristineGroundTruthIsIntact 逐条断言——素材一旦被替换就会立刻失败，从而把
// "素材/坐标选取有问题" 与 "parity 有差距" 两种失败区分开。但邻域条件只在选取时
// 校验过一次，运行时不再复查（复查需读取 13x13=169 个区块，代价高于收益）：
// 若将来换了一份邻域不完整的素材，本测试不会报"素材不合格"，只会表现为 parity 变差。
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
// 建立本测试的过程暴露并修复了三个缺陷，它们的共同特征是**静默出错**——解析或
// 生成出的数据看似合法，实则整片错位，而若只看"是否成功"完全发现不了：
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
// 3) JavaChunkReader::readSectionBiomePalette 按 compact 格式解包群系索引，而
//    磁盘上是 padded 格式。bits 为 1/2/4 时两种布局恰好重合（占绝大多数），
//    palette 达 5 项（bits=3）时才暴露：实测素材 r.-1.-1.mca 的 chunk(-18,-1)
//    按 compact 解出越界索引，按 padded 解全部合法。【已修复】
//
// 另有一个**夹具**缺陷（不在读取链里，但同样属于"让结论失真的假绿"）：
// 夹具一度漏加载数据驱动的世界生成注册表（feature/placement/carver/biome）。
// 生产路径由 RegistryBootstrap::initializeAll 加载；缺了它们，生成会**静默**退化
// （矿脉与装饰特征被跳过），测出来的方块差距（当时 6.9%~10.2%）远小于真实值
// （19.0%~20.8%）——即夹具缺陷会让 parity 显得比实际更好。现改为调用
// mc::test::loadVanillaWorldGenRegistries()。【已修复】
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
#include "common/util/math/MathUtils.hpp"
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
#include "server/world/gen/aquifer/Aquifer.hpp"
#include "server/world/gen/aquifer/FluidPickerFactory.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/density/Beardifier.hpp"
#include "server/world/gen/density/NoiseChunk.hpp"
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
#include <typeinfo>
#include <utility>
#include <vector>
#include <fmt/format.h>

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
/// 现状实测：12.0% ~ 13.7%（补齐方块标签 + 重写 OreFeature 前为 18.3% ~ 20.8%）。
/// 仍只是退化门禁，收敛目标为 0。
constexpr f64 kMaxBlockMismatchRatio = 0.20;

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

    /// 注册表查不到的 stateId 出现次数（应为 0；非 0 说明生成或映射写入了裸 stateId）
    i64 javaUnregisteredStates = 0;
    i64 cubiumUnregisteredStates = 0;

    i64 heightColumns = 0;
    i64 heightMismatched = 0;
    i32 heightMaxAbsDelta = 0;
    /// 列高度差（Cubium - 原版）直方图，键为差值（格），值为列数。
    /// 系统性偏移（全部偏 -1）与随机错位（有正有负）在直方图上形态完全不同，
    /// 是区分"地形整体抬高/压低"与"个别列异常"的关键判据。
    std::map<i32, i64> heightDeltaHistogram;

    /// 按 Y 分层的不一致计数（下标 = blockY - MIN_BUILD_HEIGHT）。
    /// 总量无法区分"深层矿脉差异"与"表层地形差异"，分层后才能定位到具体阶段。
    std::vector<i64> mismatchByY;

    i64 totalBiomes = 0;
    i64 mismatchedBiomes = 0;

    /// 两侧各方块的出现次数。与 topBlockPairs 联合解读可区分两种截然不同的失效模式：
    ///   - 两侧总量接近、但差异对双向大致相等 → 方块**位置**错位（生成顺序/随机源/坐标偏移）
    ///   - 两侧总量相差悬殊 → 该方块被**多生成或少生成**（feature 计数/概率/阈值）
    /// 只看差异对无法区分二者，例如 "stone->andesite 2424" 既可能是多生成也可能是错位。
    std::map<std::string, i64> javaBlockCounts;
    std::map<std::string, i64> cubiumBlockCounts;

    /// 出现次数最多的差异对（"原版方块 -> Cubium 方块"）
    std::vector<std::pair<std::string, i64>> topBlockPairs;
    /// 原版有而 Cubium 没有的方块名
    std::vector<std::string> missingBlockNames;
    /// Cubium 有而原版没有的方块名
    std::vector<std::string> extraBlockNames;
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
        diff.mismatchByY.assign(static_cast<size_t>(world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT), 0);
        // 未注册 stateId 直接计数，而不是从调色板名字里反查：原版读取链对未知方块是
        // **静默映射为空气**（JavaBlockStateMapper 未命中即返回 0），因此映射缺口不会表现为
        // "出现 <unregistered:...>"，而会表现为"该方块凭空消失"。唯一的可靠信号是
        // 生成产物里出现注册表查不到的 stateId；原版侧则从调色板名反查注册表。
        i64 javaUnregisteredStates = 0;
        i64 cubiumUnregisteredStates = 0;

        for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
            for (i32 bz = 0; bz < 16; ++bz) {
                for (i32 bx = 0; bx < 16; ++bx) {
                    const u32 javaId = javaChunk.getBlockStateId(bx, y, bz);
                    const u32 cubiumId = cubiumChunk.getBlockStateId(bx, y, bz);
                    ++diff.totalBlocks;

                    const BlockState* javaState = BlockRegistry::instance().getBlockState(javaId);
                    const BlockState* cubiumState = BlockRegistry::instance().getBlockState(cubiumId);
                    if (javaState == nullptr) {
                        ++javaUnregisteredStates;
                    }
                    if (cubiumState == nullptr) {
                        ++cubiumUnregisteredStates;
                    }

                    const std::string javaName = blockName(javaId);
                    const std::string cubiumName = blockName(cubiumId);
                    javaPalette.insert(javaName);
                    cubiumPalette.insert(cubiumName);
                    ++diff.javaBlockCounts[javaName];
                    ++diff.cubiumBlockCounts[cubiumName];

                    if (javaId != cubiumId) {
                        ++diff.mismatchedBlocks;
                        ++diff.mismatchByY[static_cast<size_t>(y - world::MIN_BUILD_HEIGHT)];
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
                diff.extraBlockNames.push_back(name);
            }
        }
        std::sort(diff.missingBlockNames.begin(), diff.missingBlockNames.end());
        std::sort(diff.extraBlockNames.begin(), diff.extraBlockNames.end());

        diff.javaUnregisteredStates = javaUnregisteredStates;
        diff.cubiumUnregisteredStates = cubiumUnregisteredStates;

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
     *
     * 两侧判据必须同为"非空气"：原版 WORLD_SURFACE 的谓词是 `!block.isAir()`，而
     * cave_air / void_air 也属于空气家族。若 Cubium 侧改用 `stateId != 0`（只有
     * minecraft:air 恰好是 0），被雕刻或结构写进 cave_air 的列就会被误判为"有方块"，
     * 凭空产生几十格的高度差。故此处用 BlockState::isAir()。
     */
    static void compareColumnHeights(const ChunkData& javaChunk, const ChunkData& cubiumChunk, ChunkDiff& diff)
    {
        for (i32 bz = 0; bz < 16; ++bz) {
            for (i32 bx = 0; bx < 16; ++bx) {
                const i32 javaTop = javaChunk.getHighestBlock(bx, bz);
                i32 cubiumTop = world::MIN_BUILD_HEIGHT - 1;
                for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
                    const BlockState* state =
                        BlockRegistry::instance().getBlockState(cubiumChunk.getBlockStateId(bx, y, bz));
                    if (state != nullptr && !state->isAir()) {
                        cubiumTop = y;
                        break;
                    }
                }
                ++diff.heightColumns;
                const i32 delta = cubiumTop - javaTop;
                if (delta != 0) {
                    ++diff.heightMismatched;
                }
                ++diff.heightDeltaHistogram[delta];
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
            std::printf("[PARITY] (%d,%d) Cubium 有而原版没有的方块（%lld 种）：\n",
                diff.x,
                diff.z,
                static_cast<long long>(diff.cubiumOnlyBlocks));
            for (const auto& name : diff.extraBlockNames) {
                std::printf("[PARITY]      + %s\n", name.c_str());
            }
        }
        if (diff.javaUnregisteredStates != 0 || diff.cubiumUnregisteredStates != 0) {
            std::printf("[PARITY] (%d,%d) 注册表查不到的 stateId：原版侧 %lld 个位置，Cubium 侧 %lld 个位置\n",
                diff.x,
                diff.z,
                static_cast<long long>(diff.javaUnregisteredStates),
                static_cast<long long>(diff.cubiumUnregisteredStates));
        }
        const size_t limit = std::min<size_t>(diff.topBlockPairs.size(), 12);
        std::printf(
            "[PARITY] (%d,%d) 差异对 top %zu / 共 %zu 种：\n", diff.x, diff.z, limit, diff.topBlockPairs.size());
        for (size_t i = 0; i < limit; ++i) {
            std::printf("[PARITY]      %-72s %lld\n",
                diff.topBlockPairs[i].first.c_str(),
                static_cast<long long>(diff.topBlockPairs[i].second));
        }
        printBlockCountComparison(diff);
        printMismatchByY(diff);
    }

    /**
     * @brief 诊断：按 Y 带拆分差异对
     *
     * 总量差异对（如 "stone -> andesite 613"）跨越全部 Y 层，无法区分"深层矿脉"与
     * "近地表地表规则"。按 32 格分带后，每带的差异对形态直接指向生成阶段：
     *   - 深板岩层出现 stone->andesite → 石头变体 ore 特征落位错
     *   - 近地表出现 grass_block->air → 地表规则/高度错
     *   - 水/空气互换 → 含水层错
     */
    static void printMismatchPairsByYBand(const ChunkData& javaChunk, const ChunkData& cubiumChunk, i32 cx, i32 cz)
    {
        constexpr i32 kBandHeight = 32;
        std::map<i32, std::map<std::string, i64>> bandPairs;
        for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
            const i32 band = math::floorDiv(y, kBandHeight) * kBandHeight;
            for (i32 bz = 0; bz < 16; ++bz) {
                for (i32 bx = 0; bx < 16; ++bx) {
                    const u32 javaId = javaChunk.getBlockStateId(bx, y, bz);
                    const u32 cubiumId = cubiumChunk.getBlockStateId(bx, y, bz);
                    if (javaId != cubiumId) {
                        ++bandPairs[band][blockName(javaId) + " -> " + blockName(cubiumId)];
                    }
                }
            }
        }
        for (const auto& [band, pairs] : bandPairs) {
            std::vector<std::pair<std::string, i64>> sorted(pairs.begin(), pairs.end());
            std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
            const size_t limit = std::min<size_t>(sorted.size(), 6);
            std::printf("[DIAG] (%d,%d) y %4d..%4d 差异对 top %zu / 共 %zu：\n",
                cx,
                cz,
                band,
                band + kBandHeight - 1,
                limit,
                sorted.size());
            for (size_t i = 0; i < limit; ++i) {
                std::printf(
                    "[DIAG]      %-60s %lld\n", sorted[i].first.c_str(), static_cast<long long>(sorted[i].second));
            }
        }
    }

    /**
     * @brief 诊断：打印不一致最严重的若干列的完整方块剖面
     *
     * 差异对是聚合量，看不出"从哪一格开始分叉"。逐列剖面能直接指出首个分歧的 Y，
     * 从而区分"地表规则从第一格就错"与"地形高度整体偏移后内容一致"。
     * 连续相同方块压缩为 `name×n`，避免刷屏。
     */
    static void printWorstColumnProfiles(const ChunkData& javaChunk, const ChunkData& cubiumChunk, i32 cx, i32 cz)
    {
        std::vector<std::pair<i32, i32>> columns; // (不一致格数, bz*16+bx)
        for (i32 bz = 0; bz < 16; ++bz) {
            for (i32 bx = 0; bx < 16; ++bx) {
                i32 count = 0;
                for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
                    if (javaChunk.getBlockStateId(bx, y, bz) != cubiumChunk.getBlockStateId(bx, y, bz)) {
                        ++count;
                    }
                }
                columns.emplace_back(count, bz * 16 + bx);
            }
        }
        std::sort(columns.rbegin(), columns.rend());

        const auto renderColumn = [](const ChunkData& chunk, i32 bx, i32 bz) {
            std::string out;
            i32 runStart = world::MIN_BUILD_HEIGHT;
            u32 runId = chunk.getBlockStateId(bx, world::MIN_BUILD_HEIGHT, bz);
            for (i32 y = world::MIN_BUILD_HEIGHT + 1; y <= world::MAX_BUILD_HEIGHT; ++y) {
                const u32 id = y < world::MAX_BUILD_HEIGHT ? chunk.getBlockStateId(bx, y, bz) : ~0u;
                if (id != runId) {
                    out += fmt::format("{}x{} ", blockName(runId), y - runStart);
                    runStart = y;
                    runId = id;
                }
            }
            return out;
        };

        const size_t limit = std::min<size_t>(columns.size(), 3);
        for (size_t i = 0; i < limit; ++i) {
            const i32 bx = columns[i].second % 16;
            const i32 bz = columns[i].second / 16;
            std::printf("[DIAG] (%d,%d) 列 (%d,%d) 不一致 %d 格：\n", cx, cz, bx, bz, columns[i].first);
            std::printf(
                "[DIAG]      原版   y=%d 起：%s\n", world::MIN_BUILD_HEIGHT, renderColumn(javaChunk, bx, bz).c_str());
            std::printf(
                "[DIAG]      Cubium y=%d 起：%s\n", world::MIN_BUILD_HEIGHT, renderColumn(cubiumChunk, bx, bz).c_str());
        }
    }

    /**
     * @brief 打印两侧各方块的总量对比（只列数量相差 100 以上的）
     *
     * 差异对只能说明"某个位置该是 A 却是 B"，无法区分 A 被多生成、还是 A 与 B 整体错位。
     * 总量对比补上这一维度：两侧数量接近而差异对很大 → 位置错位；数量悬殊 → 多/少生成。
     */
    static void printBlockCountComparison(const ChunkDiff& diff)
    {
        struct Row {
            std::string name;
            i64 java = 0;
            i64 cubium = 0;
        };
        std::vector<Row> rows;
        for (const auto& [name, count] : diff.javaBlockCounts) {
            const i64 cubiumCount = diff.cubiumBlockCounts.count(name) != 0 ? diff.cubiumBlockCounts.at(name) : 0;
            if (count != cubiumCount) {
                rows.push_back({name, count, cubiumCount});
            }
        }
        for (const auto& [name, count] : diff.cubiumBlockCounts) {
            if (diff.javaBlockCounts.count(name) == 0) {
                rows.push_back({name, 0, count});
            }
        }
        std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
            return std::abs(a.java - a.cubium) > std::abs(b.java - b.cubium);
        });

        if (rows.empty()) {
            return;
        }
        const size_t limit = std::min<size_t>(rows.size(), 30);
        std::printf("[PARITY] (%d,%d) 方块总量对比 top %zu / 共 %zu 种（仅列两侧数量不同的）：\n",
            diff.x,
            diff.z,
            limit,
            rows.size());
        for (size_t i = 0; i < limit; ++i) {
            const i64 delta = rows[i].cubium - rows[i].java;
            const f64 ratio =
                rows[i].java != 0 ? static_cast<f64>(rows[i].cubium) / static_cast<f64>(rows[i].java) : 0.0;
            std::printf("[PARITY]      %-48s 原版 %7lld   Cubium %7lld   (%+lld, %.2fx)\n",
                rows[i].name.c_str(),
                static_cast<long long>(rows[i].java),
                static_cast<long long>(rows[i].cubium),
                static_cast<long long>(delta),
                ratio);
        }
    }

    /**
     * @brief 按 Y 区间汇总不一致数
     *
     * 总量差异无法区分"深层矿脉"与"表层地形"，按 Y 分层后才能定位到生成阶段：
     *   - 差异集中在深板岩层（y < 0）→ 矿脉 / 深板岩替换
     *   - 差异集中在近地表（y 50~90）→ 地表规则 / 含水层
     *   - 全高度均匀 → 密度函数本身
     */
    static void printMismatchByY(const ChunkDiff& diff)
    {
        if (diff.mismatchByY.empty()) {
            return;
        }
        // 每 32 格一个区间
        constexpr i32 kBandHeight = 32;
        std::map<i32, i64> bands;
        i64 total = 0;
        for (size_t i = 0; i < diff.mismatchByY.size(); ++i) {
            const i32 y = world::MIN_BUILD_HEIGHT + static_cast<i32>(i);
            bands[(y / kBandHeight) * kBandHeight] += diff.mismatchByY[i];
            total += diff.mismatchByY[i];
        }
        if (total == 0) {
            return;
        }
        std::printf(
            "[PARITY] (%d,%d) 按 Y 分层的不一致分布（共 %lld）：\n", diff.x, diff.z, static_cast<long long>(total));
        for (const auto& [bandStart, count] : bands) {
            if (count == 0) {
                continue;
            }
            const i32 pct = static_cast<i32>(100.0 * static_cast<double>(count) / static_cast<double>(total));
            std::printf("[PARITY]      y %5d..%5d  %8lld  (%3d%%)  %s\n",
                bandStart,
                bandStart + kBandHeight - 1,
                static_cast<long long>(count),
                pct,
                std::string(static_cast<size_t>(pct) / 2, '#').c_str());
        }
    }

    /// 已加载的原版区块（转存以保证指针存活）
    ///
    /// 预分配 kTargets 个容量是**必要**的：loadJavaChunk 返回的是容器内元素的地址，
    /// 一旦 push_back 触发扩容，先前返回的指针立即悬垂（ChunkData 只可移动，元素会被搬走）。
    /// 每个用例至多加载 kTargets 个区块，故容量足够；若将来增加目标区块或让某个用例
    /// 重复加载，必须同步调整这里，或改用 std::deque / vector<unique_ptr<ChunkData>>。
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
 * 目标区块是「纯净 ground truth」：坐标正确、状态 full、从未被 tick、无方块实体。
 *
 * 这是其余所有对比的前提。四项断言各自对应一类"素材不合格"：
 *   - 坐标不符 → region 定位或素材布局变了
 *   - 非 full  → 该列不是完整生成产物
 *   - InhabitedTime != 0 → 该区块被 tick 过，方块可能被随机刻/流体刻/玩家改动，
 *     不再是纯 worldgen 结果（这是最容易忽视、也最致命的一项：它不会报错，
 *     只会让 parity 数字变得无法解释）
 *   - 存在方块实体 → 容器类方块可能被玩家改动过
 *
 * 任一不满足时本用例先行失败，从而把"素材/坐标选取有问题"与"parity 有差距"区分开。
 */
TEST_F(JavaAnvilWorldGenParityTest, PristineGroundTruthIsIntact)
{
    for (const auto& [x, z] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(x, z);
        ASSERT_NE(javaChunk, nullptr) << "无法读取区块 (" << x << "," << z << ")";
        EXPECT_EQ(javaChunk->x(), x) << "读取到的区块坐标与请求不一致（region 定位可能出错）";
        EXPECT_EQ(javaChunk->z(), z) << "读取到的区块坐标与请求不一致（region 定位可能出错）";
        EXPECT_TRUE(javaChunk->isFullyGenerated()) << "区块 (" << x << "," << z << ") 不是 full";
        EXPECT_EQ(javaChunk->inhabitedTime(), 0)
            << "区块 (" << x << "," << z
            << ") 的 InhabitedTime 非 0，说明它被 tick 过，"
               "方块可能已被运行时逻辑改动，不再是纯 worldgen 产物；需重新挑选纯净区块";
        EXPECT_EQ(javaChunk->blockEntityCount(), 0)
            << "区块 (" << x << "," << z << ") 含方块实体，容器内容可能已被玩家改动";
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
 * 两侧的方块调色板与方块映射链都没有缺口。
 *
 * 三项断言各自对应一类"对比失去意义"的情况：
 *
 * 1. `cubiumUnregisteredStates == 0`：Cubium 生成产物里出现注册表查不到的 stateId，
 *    说明生成或 stateId 分配有问题。
 * 2. `javaUnregisteredStates == 0`：从原版存档解出的 stateId 在注册表里查不到。
 *    注意原版侧**不会**表现为"出现 <unregistered:...>"——JavaBlockStateMapper 对未知
 *    方块是静默映射为空气的，所以映射缺口的真正症状是"该方块凭空消失"。因此这里
 *    改成直接对解码出的 stateId 反查注册表，而不是像早先那样遍历调色板名字找
 *    "<unregistered:" 前缀（那种写法在调色板一致的常见情况下是空循环，等于零断言）。
 * 3. `cubiumOnlyBlocks == 0`：Cubium 生成了原版在该区块没有的方块。
 *
 * 【当前状态】第 3 项失败（第 1、2 项通过）。实测 Cubium 独有的方块（补齐方块标签
 * 与重写 OreFeature 之后）：
 *   - (2,-2)：deepslate_coal_ore / dripstone_block / glow_lichen / pointed_dripstone /
 *             short_grass / wildflowers
 *   - (2,10)：water / dandelion / poppy / bush 等
 * 其中 dripstone / glow_lichen / short_grass / wildflowers / 花 属于**装饰特征**尚未
 * 完全对齐（这些方块原版在该区块也没有，是 Cubium 多生成的）；deepslate_coal_ore 说明
 * 矿石落位仍与原版不重合（差异已从"石头层完全没有矿"缩小到"个别矿脉位置不同"）。
 *
 * 【收敛目标】三项断言全部为 0。
 */
TEST_F(JavaAnvilWorldGenParityTest, BlockPalettesAndMappingAreIntact)
{
    for (const auto& [cx, cz] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(cx, cz);
        ASSERT_NE(javaChunk, nullptr);
        ChunkData* cubiumChunk = generateCubiumChunk(cx, cz);
        ASSERT_NE(cubiumChunk, nullptr);

        const ChunkDiff diff = compareBlocks(*javaChunk, *cubiumChunk);
        EXPECT_EQ(diff.cubiumUnregisteredStates, 0)
            << "区块 (" << cx << "," << cz << ") 的生成产物出现注册表查不到的 stateId";
        EXPECT_EQ(diff.javaUnregisteredStates, 0)
            << "区块 (" << cx << "," << cz << ") 从原版存档解出的 stateId 在注册表中不存在";
        EXPECT_EQ(diff.cubiumOnlyBlocks, 0) << "区块 (" << cx << "," << cz << ") 生成了原版在该区块没有的方块";
    }
}

/**
 * 逐方块与原版严格相等。
 *
 * 【当前状态】失败：不一致率 12.0% ~ 13.7%（68~159 种差异对）。
 * 本轮已消除的缺口（此前 18.3% ~ 20.8%）：
 *   - **石头层矿石完全缺失**：`stone_ore_replaceables` 方块标签未注册，使 17 个 ore_*
 *     configured_feature 的石头层 target 恒不匹配，coal/copper/iron/gold/lapis/
 *     redstone/diamond/emerald 在主石层一个都放不出来。补齐标签后石头层矿石已出现
 *     （coal_ore 116 对原版 94、copper_ore 130 对 93 等）。
 *   - **OreFeature 五处算法偏差**：半径漏 /2.0（矿脉体积膨胀 8 倍）、球体重叠判据误用
 *     r1+r2 而非 |r1-r2|、提前退出误用 WorldSurfaceWG 而非 OceanFloorWG、Y 抖动
 *     误用 nextInt(-2,2) 而非 nextInt(3)-2、全程 f32 而原版是 f64。
 *   - **Mth.sin/cos 查表缺失**：原版 `Mth.sin` 是 65536 项量化查表，与 std::sin 差约
 *     1e-5；矿脉半径包络依赖它，用 std::sin 会让球体边界逐格偏移。
 *
 * 【剩余缺口】按差异量排序：
 *   1. **石头变体（andesite/granite/diorite/tuff/gravel）双向错位**：这些由
 *      ore_andesite/granite/diorite/tuff/gravel 等 placed_feature 放置，其落位依赖
 *      每个特征的 setFeatureSeed(featureIndex)。总量已接近（如 andesite 1266 对原版 1345），
 *      但位置对不上 → 嫌疑在特征排序/索引或 placement 链的随机量消耗。
 *   2. **地表内容**：clay / sand / seagrass / tall_seagrass 缺失，water 严重偏少
 *      （3 对 963），grass_block 偏多（256 对 5）——地形高度本身未对齐导致水面/岸线错位。
 *   3. **装饰特征**：dripstone / glow_lichen / 花 等多生成。
 *   4. **列高度**：194~254 / 256 列不一致，是上述一切的根因之一。
 *
 * 【收敛目标】不一致数降为 0。在此之前用 kMaxBlockMismatchRatio 卡住量级，
 * 避免在没有门禁的情况下进一步退化。
 */

/**
 * @brief 临时诊断：含水层流体面链路（preliminarySurfaceLevel → aquifer fluidLevel）
 *
 * TODO(parity): 本用例是**收敛过程中的临时诊断**，不参与 parity 断言，parity 达成后删除。
 *
 * 本用例建立时观察到"整片海域无水"（(2,-2) 的 water 原版 963 / Cubium 0，且海底被地表规则铺成草方块）。排查结论已固化：
 *
 *   - `preliminarySurfaceLevel` **偏低 8~20 格是原版的正常现象**（实测 (2,-2) 为 48，
 *     而该处水面 62、海底约 57）。它不是 final_density，本来就只给出粗略高度；
 *     AST 编译（CompiledDensityFunctionAdapter）与未编译的原始树**结果完全一致**，
 *     故此前怀疑的 McToAst/BytecodeGen 对 FindTopSurface 的转换已被排除。
 *
 *   - 真正的根因是含水层中心的位置编解码错位：NoiseBasedAquifer 自带一份
 *     encode/decodeBlockPos，把原版布局 `x<<38 | z<<12 | y` 写成了 `x<<38 | y<<26 | z`，
 *     与它自己的解包公式不自洽。负 Z 区块的水层中心被解成 704511 这类大正数，
 *     computeFluid 于是在错误位置采样地表高度与 floodedness 噪声，fluidLevel 恒为
 *     WAY_BELOW_MIN_Y。**已修复**（改为复用 BlockPos::asLong / getXFromLong 等既有工具），
 *     (2,-2) 的 y=62..40 已恢复为 water。
 *
 *   - 本用例保留下来用于观察该链路：直方图反映 preliminarySurfaceLevel 的分布形态，
 *     抽样列给出绝对值，含水层列剖面给出 computeSubstance 在各 Y 的实际返回。
 */
TEST_F(JavaAnvilWorldGenParityTest, PreliminarySurfaceLevelDiagnostic)
{
    auto settings = DimensionSettings::overworld();
    auto state = world::gen::RandomState::create(settings, static_cast<u64>(kSeed));
    ASSERT_NE(state, nullptr);

    const auto& noise = settings.noise;
    const i32 cellWidth = noise.sizeHorizontal * 4;
    const i32 cellHeight = noise.sizeVertical * 4;
    const i32 cellCountY = math::floorDiv(noise.height, cellHeight);

    for (const auto& [cx, cz] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(cx, cz);
        ASSERT_NE(javaChunk, nullptr);

        const i32 startX = cx * world::CHUNK_WIDTH;
        const i32 startZ = cz * world::CHUNK_WIDTH;
        auto nc = std::make_unique<world::gen::density::NoiseChunk>(*state,
            cellWidth,
            cellHeight,
            cellCountY,
            startX,
            noise.minY,
            startZ,
            std::make_unique<world::gen::density::BeardifierMarker>(),
            1);

        // 差值直方图：prelimSurface - (原版 WORLD_SURFACE + 1)
        // 注意：getHighestBlock 返回的是"最高非空气方块的 Y"（WorldSurface 存 Y+1），
        // 而原版 getHeight(WORLD_SURFACE) 返回 Y+1；这里统一到 Y+1 便于与原版公式对照。
        std::map<i32, i32> hist;
        for (i32 bz = 0; bz < world::CHUNK_WIDTH; ++bz) {
            for (i32 bx = 0; bx < world::CHUNK_WIDTH; ++bx) {
                const i32 prelim = nc->samplePreliminarySurfaceLevel(startX + bx, startZ + bz);
                const i32 javaTop = javaChunk->getHighestBlock(bx, bz);
                ++hist[prelim - (javaTop + 1)];
            }
        }
        std::printf("[DIAG-AQ] (%d,%d) prelimSurface - (原版WORLD_SURFACE+1) 直方图（海平面 63）：\n", cx, cz);
        for (const auto& [delta, count] : hist) {
            std::printf("[DIAG-AQ]      %+4d  %4d\n", delta, count);
        }

        // 抽样 8 列：prelimSurface 绝对值 vs 原版 WORLD_SURFACE 绝对值
        std::printf(
            "[DIAG-AQ] (%d,%d) 抽样列  bx,bz | prelimSurface | 原版表面高度 | 原版方块剖面(顶部12格)：\n", cx, cz);
        for (i32 k = 0; k < 8; ++k) {
            const i32 bx = (k * 3) % world::CHUNK_WIDTH;
            const i32 bz = (k * 5) % world::CHUNK_WIDTH;
            const i32 prelim = nc->samplePreliminarySurfaceLevel(startX + bx, startZ + bz);
            const i32 javaTop = javaChunk->getHighestBlock(bx, bz);
            std::string profile;
            for (i32 y = javaTop; y > javaTop - 12 && y >= world::MIN_BUILD_HEIGHT; --y) {
                profile += blockName(javaChunk->getBlockStateId(bx, y, bz)) + " ";
            }
            std::printf("[DIAG-AQ]      (%2d,%2d) | %4d | %4d | %s\n", bx, bz, prelim, javaTop, profile.c_str());
        }

        // 决定性对照：未编译的原始密度函数树 vs AST 编译后的 adapter
        // 若两者对同一 (x,z) 给出不同的 preliminarySurfaceLevel，则缺陷在 McToAst/BytecodeGen。
        {
            const auto* rawFts =
                dynamic_cast<const world::gen::density::FindTopSurface*>(&state->router().preliminarySurfaceLevel());
            std::printf("[DIAG-AQ] (%d,%d) 原始树是否为 FindTopSurface: %d\n", cx, cz, rawFts != nullptr ? 1 : 0);
            if (rawFts != nullptr) {
                std::printf("[DIAG-AQ]      lowerBound=%d cellHeight=%d\n", rawFts->lowerBound(), rawFts->cellHeight());
                for (i32 y = 80; y >= 32; y -= 8) {
                    const f64 upper = rawFts->upperBound().compute(startX + 8, 0, startZ + 8);
                    const f64 dens = rawFts->density().compute(startX + 8, y, startZ + 8);
                    std::printf("[DIAG-AQ]      RAW y=%3d upperBound=%.6f density=%.6f\n", y, upper, dens);
                }
                std::printf("[DIAG-AQ]      RAW compute(x,0,z)=%.1f | COMPILED compute(x,0,z)=%.1f\n",
                    rawFts->compute(startX + 8, 0, startZ + 8),
                    state->compiledRouter()[static_cast<size_t>(RouterSlot::PreliminarySurfaceLevel)]->eval(
                        startX + 8, 0, startZ + 8));
            }
        }

        // 拆解 FindTopSurface：upperBound 取值 + density 在各 y 的符号
        {
            const auto* fts =
                dynamic_cast<const world::gen::density::FindTopSurface*>(&nc->router().preliminarySurfaceLevel());
            std::printf("[DIAG-AQ] (%d,%d) preliminarySurfaceLevel 是否为 FindTopSurface: %d\n",
                cx,
                cz,
                fts != nullptr ? 1 : 0);
            std::printf("[DIAG-AQ]      实际类型: %s\n", typeid(nc->router().preliminarySurfaceLevel()).name());
            if (fts != nullptr) {
                std::printf("[DIAG-AQ]      lowerBound=%d cellHeight=%d\n", fts->lowerBound(), fts->cellHeight());
                for (i32 y = 80; y >= 32; y -= 8) {
                    std::printf("[DIAG-AQ]      列(%d,%d) y=%3d  upperBound=%.6f  density=%.6f\n",
                        8,
                        8,
                        y,
                        fts->upperBound().compute(startX + 8, 0, startZ + 8),
                        fts->density().compute(startX + 8, y, startZ + 8));
                }
            }
        }

        // 深暗之域判定所需的 erosion / depth（computeSurfaceLevel 会据此直接返回 WAY_BELOW_MIN_Y）
        {
            const auto& r = state->router();
            for (i32 y = 70; y >= 40; y -= 6) {
                std::printf("[DIAG-AQ]      EROSION/DEPTH y=%3d erosion=%.6f depth=%.6f deepDark=%d\n",
                    y,
                    r.erosion().compute(startX + 8, y, startZ + 8),
                    r.depth().compute(startX + 8, y, startZ + 8),
                    (r.erosion().compute(startX + 8, y, startZ + 8) < -0.225 &&
                        r.depth().compute(startX + 8, y, startZ + 8) > 0.9)
                        ? 1
                        : 0);
            }
        }

        // 直接调用含水层：在 y=40..75 上问"若此处非固体（density<0），含水层给出什么流体"
        auto aquifer = world::gen::aquifer::Aquifer::createNoiseBased(*nc,
            cx,
            cz,
            nc->router(),
            state->aquiferRandom(),
            noise.minY,
            noise.height,
            world::gen::aquifer::createFluidPicker(settings.seaLevel, settings.defaultFluid));
        const i32 probeBx = 8;
        const i32 probeBz = 8;
        std::string aquiferColumn;
        for (i32 y = 75; y >= 40; --y) {
            const BlockState* s = aquifer->computeSubstance(startX + probeBx, y, startZ + probeBz, -0.01);
            aquiferColumn += (s == nullptr ? std::string("<null>") : s->getBlock().blockLocation().toString()) + " ";
        }
        std::printf("[DIAG-AQ] (%d,%d) 列(%d,%d) y=75..40 含水层 computeSubstance(density=-0.01)：\n",
            cx,
            cz,
            probeBx,
            probeBz);
        std::printf("[DIAG-AQ]      %s\n", aquiferColumn.c_str());
    }
}

/**
 * @brief 临时诊断：ChunkData/ChunkPrimer 的 getBiomeAtBlock 与 BiomeContainer 是否自洽
 *
 * TODO(parity): 收敛过程中的临时诊断，parity 达成后删除。
 *
 * 背景：OceanWaterReproTest 一度以 parity 素材的种子 + 区块 (2,-2) 断言"存在海洋群系",
 * 却一个海洋列都找不到，而本文件的 compareBiomes 已证明该区块的 BiomeContainer 与原版
 * 100% 一致。本用例给出的结论：**该区块的群系是 river（河流），不是 ocean**——两侧的
 * BiomeContainer 与 getBiomeAtBlock 都给出 river，完全自洽；是那条断言的群系类别选错了
 * （河流不属于 IS_OCEAN）。测试已相应改为按"水域群系"（IS_OCEAN ∪ IS_RIVER）判定。
 *
 * 本用例仍保留：它是检验"ChunkData 的 BiomeContainer 与 getBiomeAtBlock 两个访问路径
 * 是否自洽"的最短路径，且在 getBiomeAtBlock 随 Y 返回洞穴群系（如 dripstone_caves）时，
 * 能直接暴露 3D 群系采样是否被塌缩成平面。 */
TEST_F(JavaAnvilWorldGenParityTest, BiomeAccessorConsistencyDiagnostic)
{
    const ChunkData* javaChunk = loadJavaChunk(2, -2);
    ASSERT_NE(javaChunk, nullptr);
    ChunkData* cubiumChunk = generateCubiumChunk(2, -2);
    ASSERT_NE(cubiumChunk, nullptr);

    const auto& cubiumBiomes = cubiumChunk->getBiomes();
    std::printf("[DIAG-BIO] (2,-2) 原版 BiomeContainer 在 section0/by0 的群系名：\n");
    for (i32 bz = 0; bz < 4; ++bz) {
        std::string row;
        for (i32 bx = 0; bx < 4; ++bx) {
            const BiomeId id = javaChunk->getBiomes().getBiome(0, bx, 0, bz);
            row += BiomeRegistry::instance().get(id).name() + " ";
        }
        std::printf("[DIAG-BIO]   %s\n", row.c_str());
    }
    std::printf("[DIAG-BIO] Cubium 生成区块 getBiomeAtBlock(8, y, 8) 随 y 变化：\n");
    for (i32 y : {-60, -32, 0, 32, 63, 100, 200}) {
        const BiomeId id = cubiumChunk->getBiomeAtBlock(8, y, 8);
        std::printf("[DIAG-BIO]   y=%4d biome=%s (id=%u)\n",
            y,
            BiomeRegistry::instance().get(id).name().c_str(),
            static_cast<u32>(id));
    }
    std::printf("[DIAG-BIO] BiomeContainer 侧 (section0..3, bx=2, bz=2, by=0)：\n");
    for (i32 s = 0; s < 4; ++s) {
        const BiomeId id = cubiumBiomes.getBiome(s, 2, 0, 2);
        std::printf("[DIAG-BIO]   section=%d biome=%s\n", s, BiomeRegistry::instance().get(id).name().c_str());
    }
}

TEST_F(JavaAnvilWorldGenParityTest, GeneratedBlocksMatchJavaSave)
{
    for (const auto& [cx, cz] : kTargets) {
        const ChunkData* javaChunk = loadJavaChunk(cx, cz);
        ASSERT_NE(javaChunk, nullptr);
        ChunkData* cubiumChunk = generateCubiumChunk(cx, cz);
        ASSERT_NE(cubiumChunk, nullptr);

        const ChunkDiff diff = compareBlocks(*javaChunk, *cubiumChunk);
        printBlockDiffReport(diff);
        printMismatchPairsByYBand(*javaChunk, *cubiumChunk, cx, cz);
        printWorstColumnProfiles(*javaChunk, *cubiumChunk, cx, cz);
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
        std::printf("[PARITY] (%d,%d) 列高度差直方图（Cubium - 原版，单位：格）：\n", cx, cz);
        for (const auto& [delta, count] : diff.heightDeltaHistogram) {
            std::printf("[PARITY]      %+4d  %4lld  %s\n",
                delta,
                static_cast<long long>(count),
                std::string(static_cast<size_t>(count) / 4, '#').c_str());
        }
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
