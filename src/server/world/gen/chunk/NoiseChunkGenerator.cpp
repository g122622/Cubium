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

#include "NoiseChunkGenerator.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeGenerationSettings.hpp"
#include "common/world/biome/BiomeManager.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "server/world/gen/aquifer/FluidPickerFactory.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/carver/CarvingContext.hpp"
#include "server/world/gen/carver/CarvingMask.hpp"
#include "server/world/gen/carver/ConfiguredCarverRegistry.hpp"
#include "server/world/gen/carver/WorldCarver.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/chunk/NoiseColumn.hpp"
#include "server/world/gen/density/Beardifier.hpp"
#include "server/world/gen/density/NoiseChunk.hpp"
#include "server/world/gen/density/OreVeinifier.hpp"
#include "server/world/gen/feature/ConfiguredFeature.hpp"
#include "server/world/gen/feature/FeatureSorter.hpp"
#include "server/world/gen/jigsaw/JigsawJunction.hpp"
#include "server/world/gen/jigsaw/JigsawPiece.hpp"
#include "server/world/gen/placement/PlacedFeatureRegistry.hpp"
#include "server/world/gen/placement/PlacementRegistry.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/settings/NoiseSettings.hpp"
#include "server/world/gen/spawn/WorldGenSpawner.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/StructureSet.hpp"
#include "server/world/gen/structure/placement/StructurePlacement.hpp"
#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

// ============================================================================
// NoiseChunkGenerator 实现
// ============================================================================

NoiseChunkGenerator::NoiseChunkGenerator(DimensionSettings settings,
    std::unique_ptr<world::biome::IBiomeSource> biomeSource,
    std::shared_ptr<world::gen::RandomState> randomState)
    : BaseChunkGenerator(randomState->worldSeed(), std::move(settings))
    , m_randomState(std::move(randomState))
    , m_biomeSource(std::move(biomeSource))
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "NoiseChunkGenerator::constructor");

    // 确保生物群系注册表已初始化（默认构造路径会初始化，注入路径也需要）
    BiomeRegistry::instance().initialize();

    MC_ASSERT_RELEASE(m_biomeSource != nullptr);
    MC_ASSERT_RELEASE(m_randomState != nullptr);

    // MC 1.21: 创建 BiomeManager（Voronoi 缩放生物群系查询）
    // obfuscateSeed 使用 SHA-256 哈希世界种子，防止玩家通过生物群系模式逆向种子
    m_biomeManager =
        std::make_unique<world::biome::BiomeManager>(*m_biomeSource, world::biome::BiomeManager::obfuscateSeed(m_seed));

    _initGenerationRegistries();

    // MC 1.21: 初始化密度函数管线
    _initDensityFunctionPipeline();
}

NoiseChunkGenerator::~NoiseChunkGenerator() = default;

void NoiseChunkGenerator::clearStructureCache()
{
    if (m_structureManager) {
        m_structureManager->clearCache();
    }
}

// ============================================================================
// 初始化
// ============================================================================

void NoiseChunkGenerator::_initGenerationRegistries()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "NoiseChunkGenerator::initGenerationRegistries");

    // 结构注册表为纯数据驱动（无硬编码兜底）：由 MinecraftServer::initializeRegistries /
    // 测试夹具 WorldGenRegistryFixture 从数据包装配。本生成器不做任何兜底注册——
    // 未装载数据包时注册表为空，结构集为空，结构生成自然为空集（可观测的明确状态），
    // 而不是一份与数据包漂移、结构上无法支撑结构生成的假注册表。
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(m_seed));

    // 初始化放置器注册表
    PlacementRegistry::instance().initialize();
}

void NoiseChunkGenerator::_initDensityFunctionPipeline()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "NoiseChunkGenerator::initDensityFunctionPipeline");

    // RandomState 由外部构造并注入（与生物群系源共享同一缓存），此处不再内部创建。

    // MC 1.21: 从 NoiseSettings 读取 cell 大小，而非硬编码
    // cellWidth = sizeHorizontal * 4, cellHeight = sizeVertical * 4
    // 主世界: sizeHorizontal=1, sizeVertical=2 → cellWidth=4, cellHeight=8
    // 末地:   sizeHorizontal=2, sizeVertical=1 → cellWidth=8, cellHeight=4
    m_cellWidth = m_settings.noise.sizeHorizontal * 4;
    m_cellHeight = m_settings.noise.sizeVertical * 4;

    // MC 1.21.11: 全局流体选择器对所有维度统一，仅读取 noise_settings 的 sea_level 与
    // default_fluid（对齐 NoiseBasedChunkGenerator.createFluidPicker）。缓存后在 getHeight 与
    // _generateNoiseWithDensityFunction 中复用；aquifers_enabled=false（下界/末地）时经
    // Aquifer::createDisabled 使用。逐维度 minY 核对零行为差（见 FluidPickerFactory 注释）。
    m_globalFluidPicker = world::gen::aquifer::createFluidPicker(m_settings.seaLevel, m_settings.defaultFluid);
}

// ============================================================================
// 结构生成
// ============================================================================

NoiseChunkGenerator::StructureSelection NoiseChunkGenerator::_selectStructureStart(
    const world::gen::structure::StructureSet& structureSet, i32 chunkX, i32 chunkZ)
{
    // 加权选择 + 失败回退重抽。
    // 参考: ChunkGenerator.createStructures()
    //   WorldgenRandom worldgenrandom = new WorldgenRandom(new LegacyRandomSource(0L));
    //   worldgenrandom.setLargeFeatureSeed(levelSeed, chunkpos.x, chunkpos.z);
    // WorldgenRandom 包装 LegacyRandomSource：nextLong() 会拆成两次 next(32)，
    // 与直接在 LegacyRandomSource 上调 nextLong() 等价（LegacyRandomSource.nextLong 也是
    // (next(32)<<32)+next(32)），故这里的包装不改变数值，只是保持原版的类型形状。
    math::WorldgenRandom rng(std::make_unique<math::JavaLegacyRandom>(0ULL));
    rng.setLargeFeatureSeed(static_cast<i64>(m_seed), chunkX, chunkZ);

    // 【为何必须循环重抽】原版对多条目结构集是
    //     while (!arraylist.isEmpty()) {
    //         i32 j = worldgenrandom.nextInt(i);   // i = 当前剩余条目权重之和
    //         for (entry : arraylist) { j -= entry.weight(); if (j < 0) break; k++; }
    //         if (tryGenerateStructure(arraylist.get(k), ...)) return;   // 成功即止
    //         arraylist.remove(k);
    //         i -= selected.weight();
    //     }
    // 即：**某条目生成失败时，把它从候选里剔除并用同一个 RNG 继续重抽**。
    // 只抽一次就放弃是错的——以 mineshafts 为例（mineshaft 与 mineshaft_mesa 各权重 1），
    // 在非恶地群系里若先抽中 mineshaft_mesa，原版会回退到 mineshaft 并成功生成，
    // 而"只抽一次"的实现在 isValidBiome 处直接 continue，导致该处**整片没有矿井**。
    // 注意：RNG 在重试之间共享（不重新播种），总权重随移除递减。
    // 【单条目不抽签】原版 `list.size() == 1` 时直接 tryGenerateStructure，**不消耗任何
    // 抽签随机数**。这一点有可观测后果：多条目结构集的首次抽签会把世界生成随机源
    // `setLargeFeatureSeed` 的初始状态推进一格，而 TheEnd 的岛屿群系判定（erosion 的
    // 岛间噪声，见 `EndIslands`）对同一随机源是敏感的；抽签次数偏离原版时，
    // 末地在群系层面就开始分叉。因此单条目必须走直接尝试，与多条目分开。
    std::vector<const world::gen::structure::StructureSelectionEntry*> candidates;
    candidates.reserve(structureSet.entries().size());
    for (const auto& setEntry : structureSet.entries()) {
        candidates.push_back(&setEntry);
    }
    i32 remainingWeight = structureSet.totalWeight();
    const bool drawByWeight = candidates.size() > 1;

    while (!candidates.empty() && remainingWeight > 0) {
        size_t chosen = 0;
        if (drawByWeight) {
            i32 roll = rng.nextInt(remainingWeight);
            for (; chosen < candidates.size(); ++chosen) {
                roll -= candidates[chosen]->weight;
                if (roll < 0) {
                    break;
                }
            }
            if (chosen >= candidates.size()) {
                chosen = candidates.size() - 1; // 权重配置异常时的兜底
            }
        }

        const world::gen::structure::StructureSelectionEntry* entry = candidates[chosen];
        const auto* structure = world::gen::structure::StructureRegistry::get(entry->structureId);

        // 对齐 MC 1.21.11 StructurePlacement.isStructureChunk() 中的生物群系检查：
        // 采样噪声生物群系，检查是否匹配该结构的 biomeTag。此处使用 getNoiseBiome()
        // （四分坐标精度，无 Voronoi 缩放）。
        // TODO: 采样点与原版不一致。原版在 Structure.findValidGenerationPoint 找到的
        //   **候选生成点**（candidate generation point）处采样：
        //     biomeSource.getNoiseBiome(QuartPos.fromBlock(pos.getX()),
        //                               QuartPos.fromBlock(pos.getY()),
        //                               QuartPos.fromBlock(pos.getZ()))
        //   而本实现退化为"本区块中心"（block +8）这一固定近似点。二者在群系边界附近
        //   会给出不同群系，进而使结构在边界处生成/不生成与原版不一致。
        //   完整实现需先把生成点求出再回传校验，属于结构子系统改造，暂缓。
        // 同 _hasBiomesForStructureSet：查不到结构定义属配置错误，不得静默跳过。
        if (structure == nullptr) {
            spdlog::critical("[STRUCT] structure_set '{}' references unregistered structure '{}'",
                structureSet.id().toString(),
                entry->structureId.toString());
        }
        MC_ASSERT_RELEASE(structure != nullptr);

        // 生物群系校验。原版在"候选生成点已求出、构件尚未装配"时校验，采样点是候选点本身
        // （jigsaw 结构为起始块中心、高度为投影后的地面线），而非区块中心。
        // 候选点依赖随机数的结构（要先抽起始块）无法由调用方预先算出，改由结构自身在
        // generate() 内校验并返回 nullptr——那种情况下这里跳过预检，避免重复消耗随机数
        // 与用错误的采样点误杀。
        bool biomeOk = true;
        if (!structure->validatesBiomeOnCandidatePoint()) {
            const BiomeId biomeAtCandidate =
                getNoiseBiome((chunkX * world::CHUNK_WIDTH + 8) >> 2, 0, (chunkZ * world::CHUNK_WIDTH + 8) >> 2);
            biomeOk = structure->isValidBiome(biomeAtCandidate);
        }

        if (biomeOk) {
            // 【必须是独立的随机源】原版里"结构集内的条目选择"与"结构自身的生成"用的是
            // **两个各自 setLargeFeatureSeed 同一种子的 WorldgenRandom 实例**：
            //   - 条目选择：ChunkGenerator.createStructures 内 `wg.setLargeFeatureSeed(...)` 后
            //     用 `wg.nextInt(权重和)` 轮盘抽签，失败重抽共用同一个 wg；
            //   - 结构生成：Structure.GenerateContext.makeRandom 新建实例并
            //     `setLargeFeatureSeed(seed, cx, cz)`，**每次都从同一个初始状态开始**。
            // 若复用条目选择的那个实例，结构生成会从"抽过签"之后的状态起算，
            // 起始旋转、起始块选择等全部错位——实测正是村庄起始构件与原版不符的根因。
            math::WorldgenRandom structureRng(std::make_unique<math::JavaLegacyRandom>(0ULL));
            structureRng.setLargeFeatureSeed(static_cast<i64>(m_seed), chunkX, chunkZ);

            auto start = structure->generate(*this, structureRng, chunkX, chunkZ);
            // 【必须判 isValid】原版只有 `start.isValid()` 才登记结构起点：空的
            // StructureStart（构件数为 0，如起始模板池为空）不算生成成功，必须继续
            // 用同一个随机源重抽其余条目；若把它当成成功，会静默吞掉该结构集在本区块的
            // 其余候选，使结构整片缺失。
            if (start && start->isValid()) {
                return {entry->structureId, std::move(start)};
            }
        }

        // 失败：剔除该条目并递减总权重，继续重抽。
        remainingWeight -= entry->weight;
        candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(chosen));
    }

    return {};
}

NoiseChunkGenerator::StructureSelection NoiseChunkGenerator::_generateStructureStartsForSet(
    const world::gen::structure::StructurePlacementState& placementState,
    const world::gen::structure::StructureSet& structureSet,
    i32 chunkX,
    i32 chunkZ)
{
    // 单条目结构集：原版直接尝试生成，不做加权抽签（也就不消耗抽签随机数）。
    // 多条目结构集走 _selectStructureStart 的「抽签 — 失败剔除 — 重抽」。
    if (!placementState.isStructureChunk(structureSet, chunkX, chunkZ)) {
        return {};
    }
    return _selectStructureStart(structureSet, chunkX, chunkZ);
}

void NoiseChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateStructureStarts", "x", chunk.x(), "z", chunk.z());

    if (!m_structureManager) {
        chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
        return;
    }

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 结构集的可放置性（候选区块 / 频率 / 排斥区 / 同心环位置表）交给本维度的放置状态：
    // 排斥区要问「另一个结构集在附近有没有落点」，单个放置规则看不到别的结构集。
    world::gen::structure::StructurePlacementState placementState(*m_biomeSource, static_cast<i64>(m_seed));
    auto& structureSetRegistry = world::gen::structure::StructureSetRegistry::instance();
    for (const auto& structureSetPtr : structureSetRegistry.getAll()) {
        if (structureSetPtr) {
            // 群系不相交的结构集在这里被滤掉，与原版的快速预过滤同一口径。
            placementState.addStructureSet(*structureSetPtr);
        }
    }

    for (const auto* structureSet : placementState.structureSets()) {
        // 已有同结构集的有效起点时不重复生成（原版 createStructures 开头的前置检查）。
        bool hasExistingStart = false;
        for (const auto& entry : structureSet->entries()) {
            auto* existingStart = chunk.getStructureStart(entry.structureId);
            if (existingStart && existingStart->isValid()) {
                hasExistingStart = true;
                break;
            }
        }
        if (hasExistingStart) {
            continue;
        }

        auto selection = _generateStructureStartsForSet(placementState, *structureSet, chunkX, chunkZ);
        if (selection.start != nullptr) {
            chunk.addStructureStart(selection.structureId,
                std::shared_ptr<mc::world::gen::structure::StructureStart>(std::move(selection.start)));
        }
    }

    // 通知 StructureCheck 缓存此区块的结构引用数据
    // 对齐 MC 1.21.11 ServerLevel.onStructureStartsAvailable() 通过 structureCheck.onStructureLoad() 的调用
    {
        auto& structureCheck = m_structureManager->structureCheck();
        const u64 chunkPosId =
            (static_cast<u64>(static_cast<u32>(chunkX)) << 32) | static_cast<u64>(static_cast<u32>(chunkZ));

        // 从 ChunkPrimer 的 m_structureStarts 构建引用计数映射
        std::unordered_map<ResourceLocation, i32> refCounts;
        for (const auto& [structureId, start] : chunk.structureStarts()) {
            if (start && start->isValid()) {
                // 有效结构起点：记录其引用计数
                refCounts[structureId] = start->getRefCount();
            }
        }
        structureCheck.onStructureLoad(chunkPosId, refCounts);
    }

    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
}

void NoiseChunkGenerator::generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateStructureReferences", "x", chunk.x(), "z", chunk.z());

    // MC 1.21: StructureReferences 阶段
    // 扫描以当前区块为中心的 17x17 区块范围（taskRange=8），
    // 找到所有与当前区块相交的 StructureStart，将引用添加到当前区块。
    const ChunkCoord cx = chunk.x();
    const ChunkCoord cz = chunk.z();

    for (i32 dx = -8; dx <= 8; ++dx) {
        for (i32 dz = -8; dz <= 8; ++dz) {
            const ChunkCoord ncx = cx + dx;
            const ChunkCoord ncz = cz + dz;

            const IChunk* neighbor = region.getIChunk(ncx, ncz, ChunkStatuses::STRUCTURE_STARTS);
            if (!neighbor) {
                continue;
            }

            // 获取邻居区块中与当前区块相交的结构起点
            auto intersecting = neighbor->getIntersectingStructures(cx, cz);
            for (auto& [structureId, srcX, srcZ] : intersecting) {
                chunk.addStructureReference(structureId, srcX, srcZ);

                // 对齐 Moonrise：STRUCTURE_REFERENCES 只写中心区块 + 集中式 StructureCheck 缓存。
                // 不写邻居 StructureStart（Moonrise StructureCheckMixin.incrementReference 写
                // 集中式 loadedChunksSafe，非邻居 ChunkAccess）。区域锁串行化重叠写区域，
                // 邻居 StructureStart 在本阶段只读不写。
                if (m_structureManager) {
                    const u64 srcChunkPosId =
                        (static_cast<u64>(static_cast<u32>(srcX)) << 32) | static_cast<u64>(static_cast<u32>(srcZ));
                    m_structureManager->structureCheck().incrementReference(srcChunkPosId, structureId);
                }
            }
        }
    }

    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_REFERENCES);
}

// ============================================================================
// 生物群系生成
// ============================================================================

void NoiseChunkGenerator::generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateBiomes", "x", chunk.x(), "z", chunk.z());
    (void)region;

    MC_ASSERT_RELEASE(m_randomState != nullptr);

    BiomeContainer& biomes = chunk.getBiomes();
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 使用 NoiseChunk 的缓存气候采样器填充生物群系
    // NoiseChunk.cachedClimateSampler() 使用经过 mapAll 包装的密度函数，
    // 在插值上下文中采样时利用缓存和插值优化。
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;
    const i32 startBlockY = m_settings.noise.minY;
    const i32 cellCountY = math::floorDiv(m_settings.noise.height, m_cellHeight);

    // MC 1.21: 构建 Beardifier 并传入 NoiseChunk
    // Beardifier 在 NoiseChunk 构造时集成到密度函数树中（叠加到 finalDensity 上），
    // 而非在外部逐方块计算
    // 使用 shared_ptr 因为 std::function 要求可复制的 callable
    auto beardifierDf = std::make_shared<world::gen::density::Beardifier>(_buildBeardifier(region, chunk));

    auto& noiseChunk = chunk.getOrCreateNoiseChunk([this, cellCountY, startX, startBlockY, startZ, beardifierDf]() {
        MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "CreateNoiseChunk");
        // 将 shared_ptr 中的 Beardifier 移动到 unique_ptr 中传入 NoiseChunk
        auto beardifierUnique = std::make_unique<world::gen::density::Beardifier>(std::move(*beardifierDf));
        // 方案X 阶段5-7：传 *m_randomState，NoiseChunk 从维度级编译产物 newInstance 组装区块级 router。
        auto nc = std::make_unique<world::gen::density::NoiseChunk>(*m_randomState,
            m_cellWidth,
            m_cellHeight,
            cellCountY,
            startX,
            startBlockY,
            startZ,
            std::move(beardifierUnique));
        return nc;
    });

    // 获取缓存气候采样器
    // MC 1.21.11: NoiseBasedChunkGenerator.doCreateBiomes 调用
    //   noisechunk.cachedClimateSampler(router, settings.value().spawnTarget())
    // spawnTarget 用于 Climate.Sampler.findSpawnPosition()，在区块生物群系填充阶段
    // 不影响 BiomeResolver 的查找（其使用独立 ParameterList），仅传递给采样器供出生点查询。
    auto sampler = noiseChunk.cachedClimateSampler(m_settings.spawnTarget);

    // 获取 BiomeSource 的参数列表用于生物群系查找
    auto* multiNoiseSource = dynamic_cast<world::biome::source::MultiNoiseBiomeSource*>(m_biomeSource.get());
    if (multiNoiseSource != nullptr) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateBiomes_MultiNoiseBiomeSource");

        const auto& parameters = multiNoiseSource->parameters();
        constexpr i32 HORIZ_SIZE = 4;
        constexpr i32 VERT_SIZE = 4;
        constexpr i32 SECTION_COUNT = world::CHUNK_SECTIONS;

        for (i32 section = 0; section < SECTION_COUNT; ++section) {
            for (i32 y = 0; y < VERT_SIZE; ++y) {
                for (i32 z = 0; z < HORIZ_SIZE; ++z) {
                    for (i32 x = 0; x < HORIZ_SIZE; ++x) {
                        const i32 quartX = (chunkX * HORIZ_SIZE) + x;
                        const i32 quartY = (section * VERT_SIZE) + y + math::floorDiv(world::MIN_BUILD_HEIGHT, 4);
                        const i32 quartZ = (chunkZ * HORIZ_SIZE) + z;

                        const auto target = sampler.sample(quartX, quartY, quartZ);
                        const BiomeId biome = parameters.findValue(target);
                        biomes.setBiome(section, x, y, z, biome);
                    }
                }
            }
        }
    } else {
        // 非 MultiNoiseBiomeSource（如 EndBiomeSource），使用传统路径
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.World.ChunkGen, "GenerateBiomes_TraditionalBiomeSource", "x", chunk.x(), "z", chunk.z());
        m_biomeSource->fillBiomeContainer(biomes, chunkX, chunkZ);
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::BIOMES);
}

// ============================================================================
// 噪声地形生成
// ============================================================================

void NoiseChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateNoise", "x", chunk.x(), "z", chunk.z());

    MC_ASSERT_RELEASE(m_randomState != nullptr);

    _generateNoiseWithDensityFunction(region, chunk);
}

// ============================================================================
// 地表生成
// ============================================================================

void NoiseChunkGenerator::buildSurface(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "BuildSurface", "x", chunk.x(), "z", chunk.z());

    MC_ASSERT_RELEASE(m_randomState != nullptr);

    // 地表规则使用区域内已生成的区块群系，并由区域进行方块分辨率的抖动缩放。
    // 不能直接取原始 quart，也不能重新计算噪声源覆盖存档中的群系数据。
    const auto getBiomeAt = [&region](i32 x, i32 y, i32 z) -> BiomeId {
        return region.getBiome(x, y, z);
    };
    // SurfaceRules.Context 直接持有 NoiseChunk 引用，
    // 通过 NoiseChunk.samplePreliminarySurfaceLevel() 查询预备表面高度
    // NoiseChunk 在 generateNoise 阶段已创建，此处直接获取
    auto* noiseChunkPtr = chunk.noiseChunk();
    if (noiseChunkPtr != nullptr) {
        m_randomState->surfaceSystem().buildSurface(chunk, getBiomeAt, *noiseChunkPtr);
    } else {
        spdlog::warn("[NoiseChunkGenerator] buildSurface: NoiseChunk is null for chunk ({}, {}). "
                     "Surface generation skipped.",
            chunk.x(),
            chunk.z());
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::SURFACE);
}

// ============================================================================
// 雕刻和特性
// ============================================================================

void NoiseChunkGenerator::applyCarvers(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "ApplyCarvers", "x", chunk.x(), "z", chunk.z());
    const ChunkCoord targetChunkX = chunk.x();
    const ChunkCoord targetChunkZ = chunk.z();

    CarvingMask& carvingMask = chunk.carvingMask();

    // 从 ChunkPrimer 缓存的 NoiseChunk 获取 Aquifer
    world::gen::aquifer::Aquifer* aquifer = nullptr;
    const world::gen::density::NoiseChunk* noiseChunkPtr = nullptr;
    if (chunk.hasNoiseChunk()) {
        noiseChunkPtr = chunk.noiseChunk();
        aquifer = const_cast<world::gen::aquifer::Aquifer*>(noiseChunkPtr->aquifer());
    }

    CarvingContext context(m_settings.noise.minY, m_settings.noise.height, aquifer, noiseChunkPtr, m_randomState.get());

    math::JavaLegacyRandom worldgenRandom;

    for (i32 dx = -8; dx <= 8; ++dx) {
        for (i32 dz = -8; dz <= 8; ++dz) {
            const ChunkCoord originChunkX = targetChunkX + dx;
            const ChunkCoord originChunkZ = targetChunkZ + dz;

            // MC 1.21.11 NoiseBasedChunkGenerator#applyCarvers：
            //   biomeSource.getNoiseBiome(QuartPos.fromBlock(chunkPos.getMinBlockX()), 0,
            //                             QuartPos.fromBlock(chunkPos.getMinBlockZ()), sampler)
            // 即以该来源区块的**最小角**换算 quart 坐标（QuartPos.fromBlock(chunkX * 16) == chunkX * 4），
            // 而非区块中心。气候噪声在 quart 分辨率上逐格变化，角点与中心（+8 格 = +2 quart）
            // 可能落在不同群系，从而给出不同的雕刻器列表，导致洞穴/流体分布整体错位。
            const i32 biomeQuartX = originChunkX * 4;
            const i32 biomeQuartZ = originChunkZ * 4;
            const BiomeId biomeId = m_biomeSource->getNoiseBiome(biomeQuartX, 0, biomeQuartZ);
            const Biome& biome = m_biomeSource->getBiomeDefinition(biomeId);
            const BiomeGenerationSettings& biomeSettings = biome.generationSettings();

            // 遍历该生物群系的所有雕刻器
            // 数据驱动：getCarvers() 返回 configured_carver 的 ResourceLocation id 列表，
            // 需经 ConfiguredCarverRegistry 解析为 const ConfiguredCarverBase*。
            const auto& carverIds = biomeSettings.getCarvers();
            for (size_t carverIndex = 0; carverIndex < carverIds.size(); ++carverIndex) {
                const ConfiguredCarverBase* configuredCarver =
                    ConfiguredCarverRegistry::instance().get(carverIds[carverIndex]);
                if (configuredCarver == nullptr) {
                    // 未注册的雕刻器 id：跳过（严格报错在加载期已处理，运行期不中断生成）
                    continue;
                }

                // MC 1.21.11: 每个雕刻器使用 carverIndex 偏移的种子
                worldgenRandom.setLargeFeatureSeed(m_seed + static_cast<u64>(carverIndex), originChunkX, originChunkZ);

                if (configuredCarver->shouldCarve(worldgenRandom, originChunkX, originChunkZ)) {
                    configuredCarver->carve(chunk,
                        context,
                        *m_biomeSource,
                        targetChunkX,
                        targetChunkZ,
                        originChunkX,
                        originChunkZ,
                        carvingMask,
                        worldgenRandom);
                }
            }
        }
    }

    chunk.setChunkStatus(ChunkStatuses::CARVERS);
}

void NoiseChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "PlaceFeatures", "x", chunk.x(), "z", chunk.z());

    chunk.primeHeightmaps(HeightmapFlag::POST_FEATURES);

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    // 构建所有可能生物群系的拓扑排序 placed_feature 列表
    std::call_once(m_featuresPerStepFlag, [this]() {
        const std::vector<BiomeId>& possibleBiomes = m_biomeSource->possibleBiomes();
        m_featuresPerStep = FeatureSorter::buildFeaturesPerStep(
            possibleBiomes,
            [](BiomeId biomeId, DecorationStage stage) -> const std::vector<ResourceLocation>& {
                const Biome& biome = BiomeRegistry::instance().get(biomeId);
                return biome.generationSettings().getFeatures(stage);
            },
            PlacedFeatureRegistry::instance());
    });

    std::unordered_set<BiomeId> sectionBiomes;
    for (ChunkCoord dz = -1; dz <= 1; ++dz) {
        for (ChunkCoord dx = -1; dx <= 1; ++dx) {
            const IChunk* neighborChunk = region.getIChunk(chunkX + dx, chunkZ + dz, ChunkStatuses::CARVERS);
            if (!neighborChunk) {
                if (dx == 0 && dz == 0) {
                    // 当前区块：遍历所有 section 的所有 4x4x4 生物群系采样点
                    for (i32 section = 0; section < world::CHUNK_SECTIONS; ++section) {
                        const i32 sectionBaseY = section * world::CHUNK_SECTION_HEIGHT + world::MIN_BUILD_HEIGHT;
                        for (i32 y = 0; y < 4; ++y) {
                            for (i32 z = 0; z < 4; ++z) {
                                for (i32 x = 0; x < 4; ++x) {
                                    sectionBiomes.insert(chunk.getBiomeAtBlock(x * 4, sectionBaseY + y * 4, z * 4));
                                }
                            }
                        }
                    }
                }
                continue;
            }

            // 邻居区块：遍历所有 section 的所有 4x4x4 生物群系采样点
            for (i32 section = 0; section < world::CHUNK_SECTIONS; ++section) {
                const i32 sectionBaseY = section * world::CHUNK_SECTION_HEIGHT + world::MIN_BUILD_HEIGHT;
                for (i32 y = 0; y < 4; ++y) {
                    for (i32 z = 0; z < 4; ++z) {
                        for (i32 x = 0; x < 4; ++x) {
                            sectionBiomes.insert(neighborChunk->getBiomeAtBlock(x * 4, sectionBaseY + y * 4, z * 4));
                        }
                    }
                }
            }
        }
    }

    // 只保留生物群系源中实际存在的生物群系（对应 Java: set.retainAll(biomeSource.possibleBiomes())）
    const std::vector<BiomeId>& possibleBiomes = m_biomeSource->possibleBiomes();
    std::unordered_set<BiomeId> possibleSet(possibleBiomes.begin(), possibleBiomes.end());
    for (auto it = sectionBiomes.begin(); it != sectionBiomes.end();) {
        if (possibleSet.find(*it) == possibleSet.end()) {
            it = sectionBiomes.erase(it);
        } else {
            ++it;
        }
    }

    // === MC 1.21: 按装饰阶段交错放置结构和特征 ===
    // 原版 ChunkGenerator.applyBiomeDecoration:
    //   WorldgenRandom worldgenrandom = new WorldgenRandom(new
    //   XoroshiroRandomSource(RandomSupport.generateUniqueSeed())); long i =
    //   worldgenrandom.setDecorationSeed(level.getSeed(), blockpos.getX(), blockpos.getZ());
    // setDecorationSeed 先 setSeed 再取两次 nextLong()，故结果与构造时的 uniqueSeed 无关，
    // 只取决于世界种子与区块原点坐标。
    //
    // 【必须用 WorldgenRandom 而非直接用 Random】WorldgenRandom 把所有位宽抽取
    // 折算成"反复调用内层 nextLong() 取高位"，于是 nextLong() 会消耗**两个**内层 nextLong
    // 并各取高 32 位，与直接在内层上调 nextLong() 完全不同。用后者会得到不同的 decorSeed，
    // 进而使所有 placed_feature 的 setFeatureSeed 种子错位。
    math::WorldgenRandom worldgenRandom(std::make_unique<math::Random>(0ULL));
    const u64 decorSeed = worldgenRandom.setDecorationSeed(m_seed, startX, startZ);
    const BlockPos chunkOrigin(startX, 0, startZ);

    // 按结构装饰阶段分组
    // MC 1.21.11: 使用跨区块结构引用而非仅当前区块的起点
    // 对应 Java: ChunkGenerator.applyBiomeDecoration() 中遍历 structureReferences
    std::map<i32,
        std::vector<std::pair<const world::gen::structure::Structure*, world::gen::structure::StructureStart*>>>
        structuresByStage;
    if (m_structureManager && chunk.hasStructureReferences()) {
        for (const auto& [structureId, refs] : chunk.structureReferences()) {
            const world::gen::structure::Structure* structure =
                world::gen::structure::StructureRegistry::get(structureId);
            if (!structure) {
                continue;
            }

            for (const auto& [refX, refZ] : refs) {
                // 从源区块获取 StructureStart
                IChunk* sourceChunk = region.getIChunk(refX, refZ, ChunkStatuses::STRUCTURE_STARTS);
                if (!sourceChunk) {
                    continue;
                }

                auto* sourcePrimer = dynamic_cast<ChunkPrimer*>(sourceChunk);
                if (!sourcePrimer) {
                    continue;
                }

                auto* start = sourcePrimer->getStructureStart(structureId);
                if (!start || !start->isValid()) {
                    continue;
                }

                const i32 stageOrdinal = static_cast<i32>(structure->decorationStage());
                structuresByStage[stageOrdinal].emplace_back(structure, start);
            }
        }
    }

    const i32 featureSteps = static_cast<i32>(m_featuresPerStep.size());
    const i32 totalSteps = std::max(static_cast<i32>(DecorationStage::Count), featureSteps);

    for (i32 stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const DecorationStage stage = DecorationStages::fromIndex(static_cast<u8>(stepIndex));
        const i32 stageOrdinal = stepIndex;

        // === 放置该阶段的结构的特征 ===
        // 对应 Java: for (Structure structure : map.getOrDefault(k, Collections.emptyList()))
        i32 structureIndex = 0;
        auto structIt = structuresByStage.find(stageOrdinal);
        if (structIt != structuresByStage.end()) {
            for (const auto& [structure, start] : structIt->second) {
                worldgenRandom.setFeatureSeed(decorSeed, structureIndex, stageOrdinal);
                structure->placeInChunk(region, chunk, *start, chunkX, chunkZ, this);
                ++structureIndex;
            }
        }

        // === 放置该阶段的生物群系特征 ===
        if (stepIndex < featureSteps) {
            const FeatureSorter::StepFeatureData& stepData = m_featuresPerStep[static_cast<size_t>(stepIndex)];
            if (stepData.features.empty()) {
                continue;
            }

            // 收集所有出现的生物群系中该阶段的特征拓扑索引
            // 对应 Java: holderset.stream().map(Holder::value).forEach(p -> intset.add(indexMapping.applyAsInt(p)))
            std::set<i32> featureIndices;
            for (BiomeId biomeId : sectionBiomes) {
                const Biome& biome = BiomeRegistry::instance().get(biomeId);
                const BiomeGenerationSettings& biomeSettings = biome.generationSettings();
                const auto& featureIds = biomeSettings.getFeatures(stage);
                for (const ResourceLocation& fid : featureIds) {
                    const i32 topoIndex = stepData.getIndex(fid);
                    if (topoIndex >= 0) {
                        featureIndices.insert(topoIndex);
                    }
                }
            }

            // 按拓扑索引排序放置特征
            // 对应 Java: i32[] aint = intset.toIntArray(); Arrays.sort(aint);
            for (i32 topoIndex : featureIndices) {
                if (topoIndex < static_cast<i32>(stepData.features.size()) && stepData.features[topoIndex] != nullptr) {
                    worldgenRandom.setFeatureSeed(decorSeed, topoIndex, stageOrdinal);
                    stepData.features[topoIndex]->place(region, chunk, *this, worldgenRandom, chunkOrigin);
                }
            }
        }
    }

    chunk.setChunkStatus(ChunkStatuses::FEATURES);
}

// ============================================================================
// 生物群系
// ============================================================================

BiomeId NoiseChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    // MC 1.21: 使用 BiomeManager 的 Voronoi 缩放查询
    // 替代旧的直接 quart 分辨率查询
    return m_biomeManager->getBiome(x, y, z);
}

BiomeId NoiseChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    return m_biomeSource->getNoiseBiome(noiseX, noiseY, noiseZ);
}

i32 NoiseChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    MC_ASSERT_RELEASE(m_randomState != nullptr);

    // iterateNoiseColumn — 创建单列 NoiseChunk 采样高度
    // 与直接逐方块采样相比，cell 插值方式与实际区块生成管线完全一致
    const NoiseSettings& noise = m_settings.noise;
    const i32 minY = noise.minY;
    const i32 cellHeight = m_cellHeight;
    const i32 cellWidth = m_cellWidth;
    const i32 cellCountY = math::floorDiv(noise.height, cellHeight);

    // 对齐坐标到 cell 网格
    const i32 cellX = math::floorDiv(x, cellWidth);
    const i32 cellZ = math::floorDiv(z, cellWidth);
    const i32 alignedX = cellX * cellWidth;
    const i32 alignedZ = cellZ * cellWidth;
    const f64 deltaX = static_cast<f64>(x - alignedX) / static_cast<f64>(cellWidth);
    const f64 deltaZ = static_cast<f64>(z - alignedZ) / static_cast<f64>(cellWidth);

    // 创建单列 NoiseChunk（cellCountXZ=1）
    // MC 1.21: iterateNoiseColumn 传入 cellCountXZ=1，与区块生成的 cellCountXZ=4 不同
    // cellCountXZ=1 使得 NoiseInterpolator 只分配 2 个 Z 切片而非 5 个
    // 方案X 阶段5-7：传 *m_randomState，从维度级编译产物 newInstance 组装区块级 router。
    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*m_randomState,
        cellWidth,
        cellHeight,
        cellCountY,
        alignedX,
        minY,
        alignedZ,
        std::make_unique<world::gen::density::BeardifierMarker>(),
        1); // cellCountXZ = 1 for single-column query

    // 单列查询也需要实际含水层和矿脉规则，不能用海平面填充代替。
    _configureNoiseChunkMaterials(
        *noiseChunk, math::floorDiv(alignedX, world::CHUNK_WIDTH), math::floorDiv(alignedZ, world::CHUNK_WIDTH));

    noiseChunk->initializeForFirstCellX();
    noiseChunk->advanceCellX(0);

    // 判定逻辑统一委托 Heightmap::isOpaqueForType，避免与 Heightmap 内部判定复制漂移。
    auto matchesHeightmap = [type](const BlockState* state) -> bool {
        return Heightmap::isOpaqueForType(type, state);
    };

    for (i32 cellY = cellCountY - 1; cellY >= 0; --cellY) {
        // MC 1.21: iterateNoiseColumn 使用 selectCellYZ 而非 selectCellXYZ
        // 因为 advanceCellX(0) 已经设置了 X 方向的 slice 数据，
        // 只需选择 YZ 方向的 cell 即可
        noiseChunk->selectCellYZ(cellY, 0);

        for (i32 inCellY = cellHeight - 1; inCellY >= 0; --inCellY) {
            const i32 blockY = (math::floorDiv(minY, cellHeight) + cellY) * cellHeight + inCellY;
            const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellHeight);
            noiseChunk->updateForY(blockY, yLerp);
            noiseChunk->updateForX(x, deltaX);

            noiseChunk->updateForZ(z, deltaZ);
            const f64 density = noiseChunk->finalDensity().compute(x, blockY, z);

            // 使用 BlockStateFiller 链确定方块状态
            // 原版 NoiseBasedChunkGenerator.iterateNoiseColumn：
            //   BlockState blockstate = noisechunk.getInterpolatedState();
            //   BlockState blockstate1 = blockstate == null ? defaultBlock : blockstate;
            // 【必须无条件替换】规则链未命中时 getInterpolatedState 返回 null（与密度的正负无关），
            // 原版一律当作**默认方块（石头）**参与高度图判定。若像此前那样只在 density > 0 时替换，
            // 密度 <= 0 且规则未命中的格子会被判为"非固体"，使该列的高度查询结果偏低——
            // 而装配期的高度查询（getFirstFreeHeight）正是靠这个判定定位地面线，
            // 结果就是构件贴地高度偏差、碰撞判定翻转、随机数序列整体错位。
            const BlockState* blockState = noiseChunk->getInterpolatedState(density);
            if (blockState == nullptr) {
                blockState = m_settings.defaultBlock;
            }

            if (matchesHeightmap(blockState)) {
                noiseChunk->stopInterpolation();
                return blockY + 1;
            }
        }
    }

    // MC 1.21.11: NoiseChunk.stopInterpolation()
    // 在高度查询完成后标记插值循环结束
    noiseChunk->stopInterpolation();

    return minY;
}

NoiseColumn NoiseChunkGenerator::getBaseColumn(i32 x, i32 z) const
{
    if (!m_randomState) {
        return NoiseColumn(m_settings.noise.minY, m_settings.noise.height);
    }

    // MC 1.21: NoiseBasedChunkGenerator.getBaseColumn()
    // 使用 iterateNoiseColumn 的列模式，填充完整垂直列的方块状态
    const NoiseSettings& noise = m_settings.noise;
    const i32 minY = noise.minY;
    const i32 cellHeight = m_cellHeight;
    const i32 cellWidth = m_cellWidth;
    const i32 cellCountY = math::floorDiv(noise.height, cellHeight);

    // 对齐坐标到 cell 网格
    const i32 cellX = math::floorDiv(x, cellWidth);
    const i32 cellZ = math::floorDiv(z, cellWidth);
    const i32 alignedX = cellX * cellWidth;
    const i32 alignedZ = cellZ * cellWidth;
    const f64 deltaX = static_cast<f64>(x - alignedX) / static_cast<f64>(cellWidth);
    const f64 deltaZ = static_cast<f64>(z - alignedZ) / static_cast<f64>(cellWidth);

    // 创建单列 NoiseChunk（使用 BeardifierMarker 零贡献，与 getHeight 一致）
    // MC 1.21: iterateNoiseColumn 传入 cellCountXZ=1
    // 方案X 阶段5-7：传 *m_randomState，从维度级编译产物 newInstance 组装区块级 router。
    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*m_randomState,
        cellWidth,
        cellHeight,
        cellCountY,
        alignedX,
        minY,
        alignedZ,
        std::make_unique<world::gen::density::BeardifierMarker>(),
        1); // cellCountXZ = 1 for single-column query

    // 单列查询也需要实际含水层和矿脉规则，不能用海平面填充代替。
    _configureNoiseChunkMaterials(
        *noiseChunk, math::floorDiv(alignedX, world::CHUNK_WIDTH), math::floorDiv(alignedZ, world::CHUNK_WIDTH));

    noiseChunk->initializeForFirstCellX();
    noiseChunk->advanceCellX(0);

    // MC 1.21: allocate BlockState[] array for the full column
    NoiseColumn column(minY, noise.height);

    for (i32 cellY = cellCountY - 1; cellY >= 0; --cellY) {
        noiseChunk->selectCellYZ(cellY, 0);

        for (i32 inCellY = cellHeight - 1; inCellY >= 0; --inCellY) {
            const i32 blockY = (math::floorDiv(minY, cellHeight) + cellY) * cellHeight + inCellY;
            const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellHeight);
            noiseChunk->updateForY(blockY, yLerp);
            noiseChunk->updateForX(x, deltaX);

            noiseChunk->updateForZ(z, deltaZ);
            const f64 density = noiseChunk->finalDensity().compute(x, blockY, z);

            const BlockState* blockState = noiseChunk->getInterpolatedState(density);
            if (blockState == nullptr) {
                blockState = m_settings.defaultBlock;
            }

            // MC 1.21: ablockstate[l2 * cellHeight + i3] = blockstate1
            const i32 idx = cellY * cellHeight + inCellY;
            column.setBlock(minY + idx, blockState);
        }
    }

    noiseChunk->stopInterpolation();
    return column;
}

i32 NoiseChunkGenerator::spawnInitialMobs(
    WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.World.ChunkGen, "NoiseChunkGenerator::spawnInitialMobs", "x", chunk.x(), "z", chunk.z());

    // MC 1.21.11: 如果 disableMobGeneration 为 true，跳过生物生成
    if (m_settings.disableMobGeneration) {
        return 0;
    }

    // 使用 WorldGenSpawner 放置被动动物
    if (!m_worldGenSpawner || !m_worldGenSpawner->isEnabled()) {
        spdlog::warn("[NoiseChunkGenerator] WorldGenSpawner is not enabled. Skipping initial mob spawning.");
        return 0;
    }

    // 获取区块中心位置的生物群系
    // MC 1.21.11: 在区块中心的最大 Y 处采样
    // Java: p_64379_.getBiome(chunkpos.getWorldPosition().atY(p_64379_.getMaxY()))
    // 通过 WorldGenRegion 的 BiomeManager 查询（带 Voronoi 缩放）
    const i32 sampleX = (chunk.x() << 4) + 8;
    const i32 sampleZ = (chunk.z() << 4) + 8;
    const i32 sampleY = region.getMaxBuildHeight() - 1;
    const BiomeId biomeId = m_biomeManager->getBiome(sampleX, sampleY, sampleZ);
    const Biome& biome = m_biomeSource->getBiomeDefinition(biomeId);

    // MC 1.21.11: WorldgenRandom.setDecorationSeed(worldSeed, blockX, blockZ)
    // 算法：setSeed(worldSeed), nextLong()|1 -> l, nextLong()|1 -> j,
    //       k = blockX * l + blockZ * j ^ worldSeed, setSeed(k)
    // 必须使用 JavaLegacyRandom 以匹配 MC 的 LegacyRandomSource 种子序列
    math::JavaLegacyRandom rng;
    const u64 decorSeed = rng.setDecorationSeed(m_seed, chunk.x() << 4, chunk.z() << 4);

    return m_worldGenSpawner->spawnInitialMobs(region, biome, chunk.x(), chunk.z(), *this, rng, outEntities);
}

void NoiseChunkGenerator::_configureNoiseChunkMaterials(
    world::gen::density::NoiseChunk& noiseChunk, ChunkCoord chunkX, ChunkCoord chunkZ) const
{
    MC_ASSERT_RELEASE(noiseChunk.aquifer() == nullptr);
    // 填充器引用的随机工厂由维度级 RandomState 持有，必须覆盖整个采样期。
    auto aquifer = m_settings.noise.aquifersEnabled ? world::gen::aquifer::Aquifer::createNoiseBased(noiseChunk,
                                                          chunkX,
                                                          chunkZ,
                                                          noiseChunk.router(),
                                                          m_randomState->aquiferRandom(),
                                                          m_settings.noise.minY,
                                                          m_settings.noise.height,
                                                          m_globalFluidPicker)
                                                    : world::gen::aquifer::Aquifer::createDisabled(m_globalFluidPicker);
    auto* aquiferPtr = aquifer.get();
    noiseChunk.setAquifer(std::move(aquifer));
    std::vector<std::unique_ptr<world::gen::density::BlockStateFiller>> fillers;
    fillers.push_back(std::make_unique<world::gen::density::AquiferFiller>(*aquiferPtr));
    // 矿脉是否启用与含水层独立。
    if (m_settings.oreVeinsEnabled) {
        auto& router = noiseChunk.router();
        fillers.push_back(std::make_unique<world::gen::density::OreVeinifier>(
            router.veinToggle(), router.veinRidged(), router.veinGap(), m_randomState->oreRandom()));
    }
    noiseChunk.setBlockStateRule(std::make_unique<world::gen::density::MaterialRuleList>(std::move(fillers)));
}

void NoiseChunkGenerator::_generateNoiseWithDensityFunction(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateNoise_DF", "x", chunk.x(), "z", chunk.z());

    if (!m_randomState) {
        spdlog::warn("[NoiseChunkGenerator] generateNoise: RandomState is null for chunk ({}, {}). "
                     "Noise generation skipped.",
            chunk.x(),
            chunk.z());
        chunk.setChunkStatus(ChunkStatuses::NOISE);
        return;
    }

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    // MC 1.21: 通过 ChunkPrimer 缓存 NoiseChunk，确保 biomes/noise/surface/carvers 阶段共享
    const i32 startBlockY = m_settings.noise.minY;
    const i32 cellCountY = math::floorDiv(m_settings.noise.height, m_cellHeight);
    // subpart: 获取或创建 NoiseChunk（含路由器副本深拷贝与 Beardifier 集成）
    // 用 IIFE 包裹，使 SCOPED_EVENT 生命周期精确覆盖 getOrCreateNoiseChunk 调用，
    // 同时让 noiseChunk 引用正常逃逸到外层作用域供后续 subpart 使用。
    auto& noiseChunk = [&]() -> world::gen::density::NoiseChunk& {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.World.ChunkGen, "GenerateNoise_DF::CreateNoiseChunk", "x", chunk.x(), "z", chunk.z());
        return chunk.getOrCreateNoiseChunk([this, cellCountY, startX, startBlockY, startZ, &region, &chunk]() {
            // MC 1.21: NoiseChunk 拥有自己的路由器副本，mapAll() 会将 Marker 替换为区块特定实现
            // Beardifier 在构造时集成到密度函数树中（叠加到 finalDensity 上）
            // 方案X 阶段5-7：传 *m_randomState，从维度级编译产物 newInstance 组装区块级 router。
            auto beardifierDf = std::make_unique<world::gen::density::Beardifier>(_buildBeardifier(region, chunk));
            auto nc = std::make_unique<world::gen::density::NoiseChunk>(*m_randomState,
                m_cellWidth,
                m_cellHeight,
                cellCountY,
                startX,
                startBlockY,
                startZ,
                std::move(beardifierDf));
            return nc;
        });
    }();

    // MC 1.21: 含水层采样器和方块状态规则链必须在 NoiseChunk 上设置。
    // 【重要】不能放在 getOrCreateNoiseChunk 的 factory lambda 中：generateBiomes 阶段会先创建
    // NoiseChunk（用于气候采样），getOrCreateNoiseChunk 会缓存该实例，导致 generateNoise 阶段
    // 传入的 factory lambda 不会执行，含水层/方块状态规则链永远不会被设置。
    // 这里在获取 NoiseChunk 之后设置，并用 aquifer()==nullptr 守卫确保只设置一次。
    if (noiseChunk.aquifer() == nullptr) {
        _configureNoiseChunkMaterials(noiseChunk, chunkX, chunkZ);
    }

    // MC 1.21: Beardifier 已集成到 NoiseChunk 密度函数树中，
    // 无需在外部逐方块计算，finalDensity().compute() 已包含 Beardifier 贡献

    // subpart: cell 插值与方块填充主循环（密度计算、材质判定、高度图更新、含水层后处理标记）
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.World.ChunkGen, "GenerateNoise_DF::FillNoiseCells", "x", chunk.x(), "z", chunk.z());

    const auto& cellConfig = noiseChunk.cellConfig();
    noiseChunk.initializeForFirstCellX();

    for (i32 cellX = 0; cellX < cellConfig.cellCountXZ; ++cellX) {
        noiseChunk.advanceCellX(cellX);

        for (i32 cellZ = 0; cellZ < cellConfig.cellCountXZ; ++cellZ) {
            for (i32 cellY = cellConfig.cellCountY - 1; cellY >= 0; --cellY) {
                noiseChunk.selectCellXYZ(cellX, cellY, cellZ);

                for (i32 inCellY = cellConfig.cellHeight - 1; inCellY >= 0; --inCellY) {
                    const i32 blockY = (noiseChunk.firstCellY() + cellY) * cellConfig.cellHeight + inCellY;
                    const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellConfig.cellHeight);
                    noiseChunk.updateForY(blockY, yLerp);

                    for (i32 inCellX = 0; inCellX < cellConfig.cellWidth; ++inCellX) {
                        const i32 localX = cellX * cellConfig.cellWidth + inCellX;
                        const i32 blockX = startX + localX;
                        const f64 xLerp = static_cast<f64>(inCellX) / static_cast<f64>(cellConfig.cellWidth);
                        noiseChunk.updateForX(blockX, xLerp);

                        for (i32 inCellZ = 0; inCellZ < cellConfig.cellWidth; ++inCellZ) {
                            const i32 localZ = cellZ * cellConfig.cellWidth + inCellZ;
                            const i32 blockZ = startZ + localZ;
                            const f64 zLerp = static_cast<f64>(inCellZ) / static_cast<f64>(cellConfig.cellWidth);

                            // MC 1.21: updateForZ 设置 inCellZ 并更新插值器状态
                            // 密度通过 finalDensity().compute() 获取
                            // finalDensity 已包含 Beardifier 贡献（在 NoiseChunk 构造时叠加到密度函数树中）
                            noiseChunk.updateForZ(blockZ, zLerp);
                            const f64 density = noiseChunk.finalDensity().compute(blockX, blockY, blockZ);

                            // density > 0 → 固体（石头），density <= 0 → 空气/流体
                            // Aquifer 在 density > 0 时返回 nullptr（表示固体）
                            // Aquifer 在 density <= 0 时返回流体/空气 BlockState，或 nullptr（表示空气）
                            // nullptr 且 density > 0 → 使用默认方块（石头）
                            // nullptr 且 density <= 0 → 空气（不放置任何方块）
                            // 原版 fillFromNoise 同样是无条件替换（blockstate == null → defaultBlock）
                            const BlockState* blockState = noiseChunk.getInterpolatedState(density);
                            if (blockState == nullptr) {
                                blockState = m_settings.defaultBlock;
                            }

                            if (blockState != nullptr && !blockState->isAir()) {
                                // 高度图更新由 chunk.setBlockState 内部的
                                // _updateHeightmapsForCurrentStatus 完成（NOISE 阶段 m_persistedStatus
                                // 仍为 BIOMES，flags=PRE_FEATURES，恰好覆盖 WorldSurfaceWG/OceanFloorWG），
                                // 此处无需再显式调用 updateHeightmap（原 MC 1.21 fillFromNoise 也无此调用）。
                                chunk.setBlockState(localX, blockY, localZ, blockState);
                                // MC 1.21: 含水层边界处流体方块需标记后处理
                                // MC 使用 !blockstate.getFluidState().isEmpty()，即包含含水方块
                                if (noiseChunk.aquifer() != nullptr &&
                                    noiseChunk.aquifer()->shouldScheduleFluidUpdate()) {
                                    const auto* fluidState = blockState->getFluidState();
                                    if (fluidState != nullptr && !fluidState->isEmpty()) {
                                        chunk.markPosForPostprocessing(localX, blockY, localZ);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        noiseChunk.swapSlices();
    }

    // MC 1.21.11: NoiseChunk.stopInterpolation()
    // 在噪声填充完成后标记插值循环结束，防止后续对插值器的意外采样
    noiseChunk.stopInterpolation();

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::NOISE);
}

// ============================================================================
// Beardifier
// ============================================================================

world::gen::density::BeardifierData world::gen::density::collectBeardifierData(
    const std::vector<std::pair<const world::gen::structure::StructureStart*, const world::gen::structure::Structure*>>&
        starts,
    i32 chunkMinX,
    i32 chunkMinZ)
{
    BeardifierData data;

    // 片段与连接点只有在「与当前区块的距离不超过核半径」时才参与：再远的位置核权重恒为 0，
    // 提前过滤可以避免结构很大的时候把无关片段也带进遍历。
    constexpr i32 KERNEL_RADIUS = 12;
    const i32 maxX = chunkMinX + world::CHUNK_WIDTH - 1 + KERNEL_RADIUS;
    const i32 maxZ = chunkMinZ + world::CHUNK_WIDTH - 1 + KERNEL_RADIUS;

    for (const auto& [start, structure] : starts) {
        if (start == nullptr || !start->isValid()) {
            continue;
        }

        const auto terrainAdaptation =
            (structure != nullptr) ? structure->terrainAdaptation() : TerrainAdaptation::None;

        // 没有地形适配的结构不影响地形，整条结构起点都不参与。
        if (terrainAdaptation == TerrainAdaptation::None) {
            continue;
        }

        for (const auto& piece : start->pieces()) {
            if (!piece) {
                continue;
            }

            const auto box = piece->getBoundingBox();
            if (box.maxX() < chunkMinX - KERNEL_RADIUS || box.minX() > maxX || box.maxZ() < chunkMinZ - KERNEL_RADIUS ||
                box.minZ() > maxZ) {
                continue;
            }

            // 地形匹配投影的 Jigsaw 片段由地面高度对齐，不作为固定体积，只贡献连接点；
            // 刚性投影的 Jigsaw 片段与非 Jigsaw 片段都按固定体积参与，后者地面高度偏移恒为 0。
            const bool jigsaw = piece->isJigsawPiece();
            const bool rigidPiece = !jigsaw || piece->getProjection() == StructurePieceProjection::Rigid;

            if (rigidPiece) {
                const i32 groundLevelDelta = jigsaw ? piece->getGroundLevelDelta() : 0;
                data.pieces.push_back(Beardifier::Rigid{box, terrainAdaptation, groundLevelDelta});
            }

            for (const auto& junction : piece->getJunctions()) {
                const i32 jx = junction.getSourceX();
                const i32 jz = junction.getSourceZ();
                if (jx > chunkMinX - KERNEL_RADIUS && jx < maxX && jz > chunkMinZ - KERNEL_RADIUS && jz < maxZ) {
                    data.junctions.push_back(junction);
                }
            }
        }
    }

    return data;
}

world::gen::density::Beardifier NoiseChunkGenerator::_buildBeardifier(WorldGenRegion& region, ChunkPrimer& chunk) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "BuildBeardifier", "x", chunk.x(), "z", chunk.z());

    const i32 chunkMinX = chunk.x() * world::CHUNK_WIDTH;
    const i32 chunkMinZ = chunk.z() * world::CHUNK_WIDTH;

    // 结构引用已在 STRUCTURE_REFERENCES 阶段填充，包含源自邻居区块但延伸到当前区块的结构；
    // 引用机制本身已包含当前区块的结构起点，因此不需要额外遍历 structureStarts。
    std::vector<std::pair<const world::gen::structure::StructureStart*, const world::gen::structure::Structure*>>
        starts;
    std::unordered_set<const world::gen::structure::StructureStart*> processedStarts;

    if (chunk.hasStructureReferences()) {
        for (const auto& [structureId, refs] : chunk.structureReferences()) {
            const auto* structure = world::gen::structure::StructureRegistry::get(structureId);
            for (const auto& [refX, refZ] : refs) {
                IChunk* sourceChunk = region.getIChunk(refX, refZ, ChunkStatuses::STRUCTURE_STARTS);
                if (!sourceChunk) {
                    continue;
                }

                auto* sourcePrimer = dynamic_cast<ChunkPrimer*>(sourceChunk);
                if (!sourcePrimer) {
                    continue;
                }

                auto* start = sourcePrimer->getStructureStart(structureId);
                if (!start || !start->isValid()) {
                    continue;
                }

                // 去重：同一个 StructureStart 可能通过多个引用条目被多次发现
                if (processedStarts.count(start) != 0) {
                    continue;
                }
                processedStarts.insert(start);
                starts.emplace_back(start, structure);
            }
        }
    }

    auto data = world::gen::density::collectBeardifierData(starts, chunkMinX, chunkMinZ);
    return world::gen::density::Beardifier(std::move(data.pieces), std::move(data.junctions));
}

} // namespace mc
