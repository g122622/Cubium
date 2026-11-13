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

#include "StructurePlacementState.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/util/thread/ParallelFor.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>

#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/placement/RandomSpreadStructurePlacement.hpp"

using namespace mc::trace;

namespace mc::world::gen::structure {

namespace {

/**
 * @brief 结构集里是否有结构的群系标签与本维度可能群系相交
 *
 * 原版 `ChunkGeneratorStructureState.hasBiomesForStructureSet`：结构集的所有条目逐个
 * 展开群系，只要其中一个群系在本维度可能出现，该结构集就参与放置。群系标签缺失
 * （结构定义没带 biomes）按空集处理——它在该维度永远不会生成，与"完全不匹配"同义。
 */
bool hasBiomesForStructureSet(const StructureSet& structureSet, const std::vector<BiomeId>& possibleBiomes)
{
    for (const auto& entry : structureSet.entries()) {
        const Structure* structure = StructureRegistry::get(entry.structureId);
        if (structure == nullptr) {
            continue;
        }
        const biome::BiomeTag* tag = structure->biomeTag();
        if (tag == nullptr) {
            continue;
        }
        for (BiomeId biomeId : possibleBiomes) {
            if (tag->contains(biomeId)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

StructurePlacementState::StructurePlacementState(const biome::IBiomeSource& biomeSource, i64 worldSeed)
    : m_biomeSource(biomeSource)
    , m_worldSeed(worldSeed)
{}

void StructurePlacementState::addStructureSet(const StructureSet& structureSet)
{
    if (!hasBiomesForStructureSet(structureSet, m_biomeSource.possibleBiomes())) {
        return;
    }
    m_structureSets.push_back(&structureSet);
    if (const auto* concentric =
            dynamic_cast<const placement::ConcentricRingsStructurePlacement*>(&structureSet.placement())) {
        m_concentric.emplace(&structureSet, concentric);
    }
}

bool StructurePlacementState::isStructureChunk(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const
{
    // 三步按短路顺序求值：前一步不通过时后一步根本不执行，所以三个子步骤各自的追踪
    // 事件数反映的是**真实调用次数**，不是"每个区块各跑三次"。
    return isPlacementChunk(structureSet, chunkX, chunkZ) && passesFrequencyReduction(structureSet, chunkX, chunkZ) &&
        passesExclusionZone(structureSet, chunkX, chunkZ);
}

bool StructurePlacementState::isPlacementChunk(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen,
        "StructurePlacementState::isPlacementChunk",
        "set",
        structureSet.id().path(),
        "chunkX",
        chunkX,
        "chunkZ",
        chunkZ);

    const auto iter = m_concentric.find(&structureSet);
    if (iter != m_concentric.end()) {
        const auto& positions = ringPositions(*iter->second);
        for (const auto& pos : positions) {
            if (pos.x == chunkX && pos.z == chunkZ) {
                return true;
            }
        }
        return false;
    }
    // 随机散布：候选区块由网格与盐值决定，与该区块是否是候选无关地先算出候选点再比较。
    const auto& spread = static_cast<const placement::RandomSpreadStructurePlacement&>(structureSet.placement());
    const auto candidate = spread.getPotentialStructureChunk(m_worldSeed, chunkX, chunkZ);
    return candidate.x == chunkX && candidate.z == chunkZ;
}

bool StructurePlacementState::passesFrequencyReduction(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen,
        "StructurePlacementState::passesFrequencyReduction",
        "set",
        structureSet.id().path(),
        "chunkX",
        chunkX,
        "chunkZ",
        chunkZ);
    return structureSet.placement().applyAdditionalChunkRestrictions(chunkX, chunkZ, m_worldSeed);
}

bool StructurePlacementState::passesExclusionZone(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen,
        "StructurePlacementState::passesExclusionZone",
        "set",
        structureSet.id().path(),
        "chunkX",
        chunkX,
        "chunkZ",
        chunkZ);

    const auto& zone = structureSet.placement().exclusionZone();
    if (!zone.has_value()) {
        return true;
    }
    for (const auto* other : m_structureSets) {
        if (other->id() != zone->otherSetId) {
            continue;
        }
        // 原版连「本区块自身」也计入范围（中心处的再次落点同样算被占用），这里不额外排除。
        return !hasStructureChunkInRange(*other, chunkX, chunkZ, zone->chunkCount);
    }
    // 排斥区指向的结构集不在本维度：没有落点，不禁用。
    return true;
}

bool StructurePlacementState::hasStructureChunkInRange(
    const StructureSet& structureSet, i32 chunkX, i32 chunkZ, i32 chunkCount) const
{
    for (i32 x = chunkX - chunkCount; x <= chunkX + chunkCount; ++x) {
        for (i32 z = chunkZ - chunkCount; z <= chunkZ + chunkCount; ++z) {
            if (isStructureChunk(structureSet, x, z)) {
                return true;
            }
        }
    }
    return false;
}

const std::vector<world::chunk::ChunkPos>& StructurePlacementState::ringPositions(
    const placement::ConcentricRingsStructurePlacement& concentric) const
{
    // 原版 `ChunkGeneratorStructureState.generateRingPositions` 分两步：
    //
    //   1. 用 concentricRingsSeed 作种子算出每个要塞的搜索起点 (chunkX, chunkZ)；
    //   2. 对每个起点做一次生物群系搜索，找到就把坐标改写过去：
    //        Pair<BlockPos, Holder<Biome>> pair = biomeSource.findBiomeHorizontal(
    //            chunkX << 4 + 8, 0, chunkZ << 4 + 8, 112,
    //            preferredBiomes::contains, randomsource.fork(), randomState.sampler());
    //        return pair != null ? new ChunkPos(pair.first) : new ChunkPos(chunkX, chunkZ);
    //
    // concentricRingsSeed 在正常维度创建时**就是 levelSeed**：
    // `createForNormal(randomState, levelSeed, biomeSource, ...)` 把同一个
    // `p_255806_` 同时传给 levelSeed 与 concentricRingsSeed。只有超平坦走
    // `createForFlat` 才传常量 0L。此前这里写死 0，导致所有种子共用同一批搜索
    // 起点，与生物群系搜索组合后整体偏移（差分能力 structure_starts 查出 8 处
    // 要塞缺失）。
    //
    // 缓存在本状态里，键是放置规则指针。
    const auto cached = m_ringPositions.find(&concentric);
    if (cached != m_ringPositions.end()) {
        return cached->second;
    }

    // 放置规则自带一份按世界种子缓存的最终坐标表。原版把 `List<ChunkPos>` 缓存在
    // `ChunkGeneratorStructureState.ringPositions` 里，与放置规则同生命周期；
    // 而 `StructurePlacementState` 每个区块（探针里是每个单元）都会新建一个，
    // 只靠本实例的缓存会让 128 次群系搜索在每个单元重跑一遍——实测单个单元 2.3 秒，
    // 542 个单元直接跑不完。`resolveRingPositions` 是单飞的：并发到达的区块生成线程
    // 只有一个真正计算，其余排队等它的结果。
    const auto& resolved = concentric.resolveRingPositions(m_worldSeed, [this, &concentric]() {
        return _computeRingPositions(concentric);
    });

    // 复制到本实例：本实例是线程私有的，返回的引用不会因别的线程重算而失效。
    auto& stored = m_ringPositions[&concentric];
    stored = resolved;
    return stored;
}

std::vector<world::chunk::ChunkPos> StructurePlacementState::_computeRingPositions(
    const placement::ConcentricRingsStructurePlacement& concentric) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "StructurePlacementState::GenerateRingPositions");

    // 正常维度下 concentricRingsSeed == levelSeed（见上方说明），不是常量 0。
    const auto& layout = concentric.generateRingLayout(m_worldSeed);
    const auto& preferred = concentric.preferredBiomes();

    // 原版 `ChunkGeneratorStructureState.generateRingPositions` 在**同一条随机流**里
    // 边算起点边 fork：
    //
    //   RandomSource randomsource = RandomSource.create();
    //   randomsource.setSeed(concentricRingsSeed);   // 常量 0
    //   double d0 = randomsource.nextDouble() * PI * 2.0;
    //   for (...) {
    //       double d1 = ... randomsource.nextDouble() ...;   // 半径扰动
    //       RandomSource randomsource1 = randomsource.fork();// <- 同一个流上 fork
    //       ... 用 randomsource1 做生物群系搜索 ...
    //       d0 += (PI * 2) / k;
    //       if (换环) d0 += randomsource.nextDouble() * PI * 2.0;
    //   }
    //
    // fork 序列因此取决于「此前每个起点消耗了多少随机数」。此前这里把半径/角度扰动
    // 留在 `generateRingPositions` 内部、又在外面新建一条 rng 来 fork，两条流并不
    // 相干，fork 到的随机源与原版完全不同。
    //
    // 现在由 `generateRingLayout` 在同一条流上取 fork 种子并随起点一起带出。
    std::vector<world::chunk::ChunkPos> result(layout.size());

    // 128 个要塞的搜索彼此完全独立：各自一条 fork 出来的随机流，对群系源只读，
    // 写入目标按下标互不重叠。串行要跑几秒，这里按元素并行（为什么不用任务池见
    // `util::parallelFor` 的说明）。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "StructurePlacementState::SearchRingBiomes");
        util::parallelFor(static_cast<i32>(layout.size()), [this, &layout, &preferred, &result](i32 index) {
            const auto& entry = layout[static_cast<std::size_t>(index)];
            // 原版 `LegacyRandomSource.fork()` 就是 `new LegacyRandomSource(nextLong())`。
            math::JavaLegacyRandom fork(entry.forkSeed);
            // 结构生成走的是 8 参数重载，它内部固定步长 1、findClosest = false。
            const auto found =
                _findBiomeHorizontal(entry.chunk.x * 16 + 8, 0, entry.chunk.z * 16 + 8, 112, 1, preferred, fork);
            result[static_cast<std::size_t>(index)] = found.has_value()
                ? world::chunk::ChunkPos(static_cast<i32>(found->x >> 4), static_cast<i32>(found->z >> 4))
                : entry.chunk;
        });
    }

    return result;
}

std::optional<BlockPos> StructurePlacementState::_findBiomeHorizontal(i32 centerX,
    i32 centerY,
    i32 centerZ,
    i32 radius,
    i32 spread,
    const std::vector<BiomeId>& preferred,
    math::IRandom& rng) const
{
    // 逐行对照原版 `BiomeSource.findBiomeHorizontal`（结构生成走的是
    // `findClosest = false` 的那个重载，即最后一个布尔参数为 false）：
    //
    //   int i  = QuartPos.fromBlock(centerX);
    //   int j  = QuartPos.fromBlock(centerZ);
    //   int k  = QuartPos.fromBlock(radius);
    //   int l  = QuartPos.fromBlock(centerY);
    //   int j1 = findClosest ? 0 : k;          // <- false 时**从最大半径开始**
    //   int k1 = j1;
    //   while (k1 <= k) {
    //       for (int l1 = -k1; l1 <= k1; l1 += spread)
    //           for (int i2 = -k1; i2 <= k1; i2 += spread) {
    //               if (findClosest) { ...只扫边缘... }   // <- false 时不筛，扫满整个方形
    //               ...
    //           }
    //       k1 += spread;                       // <- 步长是 spread，不是 1
    //   }
    //
    // 这里此前有三处偏差：环从 0 向外递增（应是从最大半径向内递减）、
    // 只扫环边缘（findClosest=false 时应扫满整个方形）、步长固定为 1（应为 spread）。
    // 三者叠加使采样顺序与原版完全不同，蓄水池抽样最终选出的坐标自然也完全不同。
    if (preferred.empty()) {
        return std::nullopt;
    }

    auto isPreferred = [&preferred](BiomeId id) {
        return std::find(preferred.begin(), preferred.end(), id) != preferred.end();
    };

    const i32 centerQuartX = centerX >> 2;
    const i32 centerQuartZ = centerZ >> 2;
    const i32 quartY = centerY >> 2;
    const i32 quartRadius = radius >> 2;
    // 原版结构生成的步长就是 placement 的 spread（要塞为 3）。防御性地取至少 1，
    // 避免配置异常时死循环。
    const i32 step = spread > 0 ? spread : 1;

    std::optional<BlockPos> chosen;
    i32 seen = 0;

    // findClosest=false 时原版的 `j1 = k`（k 是半径换算成 quart 后的值），而
    // `while (k1 <= k)` 的步长也是 spread，因此该循环**只执行一次**、k1 恒等于 k：
    // 整个搜索是**一遍** [-k, k]² 的方形扫描，不是逐圈由外到内。
    //
    // 此前这里写成了「按 ring 逐层从外到内」的嵌套循环，同一批坐标会被反复采样，
    // 蓄水池抽样的次数因此翻倍（半径越大越严重），最终选出的坐标与原版不同——
    // 差分能力 structure_starts 表现为要塞位置整体偏移。
    for (i32 dz = -quartRadius; dz <= quartRadius; dz += step) {
        for (i32 dx = -quartRadius; dx <= quartRadius; dx += step) {
            const i32 quartX = centerQuartX + dx;
            const i32 quartZ = centerQuartZ + dz;
            if (!isPreferred(m_biomeSource.getNoiseBiome(quartX, quartY, quartZ))) {
                continue;
            }
            if (!chosen.has_value() || rng.nextInt(seen + 1) == 0) {
                chosen = BlockPos(static_cast<BlockCoord>(quartX << 2),
                    static_cast<BlockCoord>(centerY),
                    static_cast<BlockCoord>(quartZ << 2));
            }
            ++seen;
        }
    }
    return chosen;
}

} // namespace mc::world::gen::structure
