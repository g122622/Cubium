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
 * @file structure_blocks_single.cpp
 * @brief 差分能力：单个结构在指定区块上的方块产物。
 *
 * `structure_start_single` 只比到「起点由哪些构件组成」，构件的 `generate`（原版
 * `postProcess`）真正往世界里写了什么方块完全没有覆盖。这条能力在起点的同一套随机源上
 * 继续走一步：把结构起点落在的每个区块都跑一遍构件放置，最后按坐标输出该区块内的方块。
 *
 * 世界是一个只记录写入的空世界（读到的方块永远是空气），两侧必须表现得完全一致：
 * 任何「读取地形后改变行为」的差异都会立刻暴露成方块差异，而不是被真实地形掩盖。
 *
 * 每个区块的随机源与生产一致：`setDecorationSeed(seed, chunkMinX, chunkMinZ)` 之后
 * `setFeatureSeed(decorSeed, 0, stageOrdinal)`，再交给该区块里全部相交构件依次消费。
 */

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "probe.hpp"
#include "probe_world.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "structure_probe_common.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using namespace world::gen::structure;

/**
 * @brief 只记录写入的空世界。
 *
 * 读取一律返回「没有方块」（探针侧的空气语义），写入按坐标去重后保留最后一次写入。
 * 最低高度按维度给出：`fillColumnDown` 会一路填到世界底部，写穿维度范围会引入
 * 与结构无关的差异。
 */
class RecordingWorld final : public probe::NullWorld {
public:
    explicit RecordingWorld(i32 minBuildHeight)
        : m_minBuildHeight(minBuildHeight)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto found = m_blocks.find(BlockPos(x, y, z).asLong());
        return found == m_blocks.end() ? nullptr : found->second;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z).asLong()] = state;
        return true;
    }

    [[nodiscard]] i32 getMinBuildHeight() const override
    {
        return m_minBuildHeight;
    }

    [[nodiscard]] const std::unordered_map<i64, const BlockState*>& blocks() const
    {
        return m_blocks;
    }

private:
    i32 m_minBuildHeight;
    std::unordered_map<i64, const BlockState*> m_blocks;
};

const Structure& findStructure(const ResourceLocation& id)
{
    const Structure* structure = StructureRegistry::get(id);
    MC_ASSERT_RELEASE_MSG(structure != nullptr, "structure_blocks_single: unknown structure");
    return *structure;
}

/// 状态的规范字符串：`minecraft:stone` 或 `minecraft:oak_log[axis=x]`（属性按名字排序）。
std::string stateToString(const BlockState& state)
{
    std::string out = state.getBlock().blockLocation().toString();
    std::vector<std::pair<std::string, std::string>> pairs;
    for (const auto& entry : state.values()) {
        pairs.emplace_back(entry.property->name(), entry.property->valueToString(entry.valueIndex));
    }
    if (pairs.empty()) {
        return out;
    }
    std::sort(pairs.begin(), pairs.end());
    out.push_back('[');
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        if (i != 0) {
            out.push_back(',');
        }
        out += pairs[i].first;
        out.push_back('=');
        out += pairs[i].second;
    }
    out.push_back(']');
    return out;
}

std::string runLine(std::string_view inputLine)
{
    initializeStructureRegistries();

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string dimensionName = input.at("dimension").get<std::string>();
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 chunkX = input.at("chunkX").get<i32>();
    const i32 chunkZ = input.at("chunkZ").get<i32>();
    const ResourceLocation structureId(input.at("structure").get<std::string>());

    WorldSetup setup = makeWorldSetup(dimensionName, seed);
    auto biomeSource = makeBiomeSource(dimensionName, *setup.randomState);
    NoiseChunkGenerator generator(std::move(setup.settings), std::move(biomeSource), setup.randomState);

    // 起点生成：与 structure_start_single 完全同源（每格从同一个初始状态起算）。
    math::WorldgenRandom structureRng(std::make_unique<math::JavaLegacyRandom>(0ULL));
    structureRng.setLargeFeatureSeed(static_cast<i64>(seed), chunkX, chunkZ);

    std::vector<std::unique_ptr<::mc::world::chunk::ChunkPrimer>> regionOwners;
    std::vector<IChunk*> regionChunks;
    for (i32 dz = -1; dz <= 1; ++dz) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            regionOwners.push_back(std::make_unique<::mc::world::chunk::ChunkPrimer>(chunkX + dx, chunkZ + dz));
            regionChunks.push_back(regionOwners.back().get());
        }
    }
    ::mc::WorldGenRegion region(chunkX, chunkZ, 1, std::move(regionChunks));

    const Structure& structure = findStructure(structureId);
    auto start = structure.generate(generator, &region, structureRng, chunkX, chunkZ);
    if (start == nullptr || !start->isValid()) {
        return json{{"unit", input.value("unit", "")}, {"valid", false}, {"blocks", json::array()}}.dump();
    }

    // 起点覆盖的区块范围：原版对每个区块各跑一次 `StructureStart.placeInChunk`。
    const auto& startBox = start->getBoundingBox();
    const i32 minChunkX = startBox.minX() >> world::CHUNK_SHIFT;
    const i32 maxChunkX = startBox.maxX() >> world::CHUNK_SHIFT;
    const i32 minChunkZ = startBox.minZ() >> world::CHUNK_SHIFT;
    const i32 maxChunkZ = startBox.maxZ() >> world::CHUNK_SHIFT;

    RecordingWorld world(generator.getMinY());
    ::mc::world::chunk::ChunkPrimer chunk(chunkX, chunkZ);

    const i32 minY = generator.getMinY();
    const i32 maxY = minY + generator.getGenDepth() - 1;
    const i32 stageOrdinal = static_cast<i32>(structure.decorationStage());

    for (i32 cz = minChunkZ; cz <= maxChunkZ; ++cz) {
        for (i32 cx = minChunkX; cx <= maxChunkX; ++cx) {
            // 原版 `ChunkGenerator.applyBiomeDecoration`：每个区块一份随机源，
            // 装饰种子按该区块最小角派生，特征种子按结构在本阶段的位置派生。
            math::WorldgenRandom rng(std::make_unique<math::Random>(0ULL));
            const u64 decorSeed = rng.setDecorationSeed(seed, cx * world::CHUNK_WIDTH, cz * world::CHUNK_WIDTH);
            rng.setFeatureSeed(decorSeed, 0, stageOrdinal);

            // 原版 `getWritableArea`：y ∈ [minY + 1, maxY]，水平方向整个区块。
            const StructureBoundingBox writable(cx * world::CHUNK_WIDTH,
                minY + 1,
                cz * world::CHUNK_WIDTH,
                cx * world::CHUNK_WIDTH + world::CHUNK_WIDTH - 1,
                maxY,
                cz * world::CHUNK_WIDTH + world::CHUNK_WIDTH - 1);

            for (const auto& piece : start->pieces()) {
                const auto box = piece->getBoundingBox();
                if (!box.intersects(writable)) {
                    continue;
                }
                piece->generate(world, rng, cx, cz, writable, &chunk, &generator);
            }
        }
    }

    std::vector<std::pair<BlockPos, const BlockState*>> sorted;
    sorted.reserve(world.blocks().size());
    for (const auto& [key, state] : world.blocks()) {
        sorted.emplace_back(BlockPos::fromLong(key), state);
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& left, const auto& right) {
        const auto& a = left.first;
        const auto& b = right.first;
        if (a.x != b.x) {
            return a.x < b.x;
        }
        if (a.y != b.y) {
            return a.y < b.y;
        }
        return a.z < b.z;
    });

    json blocks = json::array();
    for (const auto& [pos, state] : sorted) {
        blocks.push_back(json{pos.x, pos.y, pos.z, stateToString(*state)});
    }

    return json{{"unit", input.value("unit", "")}, {"valid", true}, {"blocks", std::move(blocks)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(structure_blocks_single, nullptr, &runLine)

} // namespace mc::diff
