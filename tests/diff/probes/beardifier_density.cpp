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
 * @file beardifier_density.cpp
 * @brief 差分能力：结构地形平滑（Beardifier）在结构周围的密度贡献。
 *
 * 起点差分只能证明"构件摆在哪里"，证明不了"这些构件把地形改成什么形状"。
 * Beardifier 是这两层之间唯一的桥梁：它把刚性构件、地面高度偏移与连接点换算成
 * 逐方块位置的密度偏移，因此同一份起点数据在这里再算一遍，能把
 *   - 参与计算的片段筛选（紧邻区块、投影类型、地形适配模式）
 *   - 地面高度偏移（groundLevelDelta）的符号与作用点
 *   - 连接点的源坐标与 dest 投影
 *   - 胡须核 / Bury / Encapsulate 三条分支的算术
 * 全部覆盖。采样网格由起点包围盒派生，两侧算法一致。
 *
 * 单元由 `harnesses/beardifier_density.ts` 给出，本探针不提供 `list`。
 */

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/WorldConstants.hpp"
#include "probe.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/density/Beardifier.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include "structure_probe_common.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using namespace world::gen::structure;

/// 采样网格的尺寸。沿 X/Z 覆盖起点包围盒外扩 24 格，沿 Y 覆盖同样的范围。
constexpr i32 GRID_X = 9;
constexpr i32 GRID_Y = 7;
constexpr i32 GRID_Z = 9;

/// 采样范围在起点包围盒外扩的格数（与原版 affectedBox 的膨胀量一致）。
constexpr i32 SAMPLE_MARGIN = 24;

const Structure& findStructure(const ResourceLocation& id)
{
    const Structure* structure = StructureRegistry::get(id);
    MC_ASSERT_RELEASE_MSG(structure != nullptr, "beardifier_density: unknown structure");
    return *structure;
}

/** 把网格的线性序号映射到 [min, max] 上的整数坐标（含端点）。 */
i32 sampleCoordinate(i32 index, i32 count, i32 min, i32 max)
{
    const i64 span = static_cast<i64>(max) - static_cast<i64>(min);
    return static_cast<i32>(static_cast<i64>(min) + (static_cast<i64>(index) * span) / (count - 1));
}

json boxToJson(const StructureBoundingBox& box)
{
    return json::array({box.minX(), box.minY(), box.minZ(), box.maxX(), box.maxY(), box.maxZ()});
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

    // 与 structure_start_single 完全同源：每格从 setLargeFeatureSeed 的初始状态起算。
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

    json result{{"unit", input.value("unit", "")}, {"valid", start != nullptr && start->isValid()}};
    if (start == nullptr || !start->isValid()) {
        result["affected"] = nullptr;
        result["values"] = json::array();
        return result.dump();
    }

    const i32 chunkMinX = chunkX * world::CHUNK_WIDTH;
    const i32 chunkMinZ = chunkZ * world::CHUNK_WIDTH;
    std::vector<std::pair<const StructureStart*, const Structure*>> starts{{start.get(), &structure}};
    const world::gen::density::BeardifierData data =
        world::gen::density::collectBeardifierData(starts, chunkMinX, chunkMinZ);

    const auto beardifier = world::gen::density::Beardifier(data.pieces, data.junctions);
    const auto& affected = beardifier.affectedBox();
    result["affected"] = affected.has_value() ? boxToJson(*affected) : json(nullptr);
    result["rigidCount"] = data.pieces.size();
    result["junctionCount"] = data.junctions.size();

    // 参与计算的输入原样输出：密度值对不上时，先分清是"输入不同"还是"算法不同"。
    json rigidJson = json::array();
    for (const auto& rigid : data.pieces) {
        rigidJson.push_back(json{{"box", boxToJson(rigid.box)},
            {"adaptation", static_cast<i32>(rigid.terrainAdaptation)},
            {"delta", rigid.groundLevelDelta}});
    }
    result["rigid"] = std::move(rigidJson);
    json junctionJson = json::array();
    for (const auto& junction : data.junctions) {
        junctionJson.push_back(json{{"x", junction.getSourceX()},
            {"y", junction.getSourceGroundY()},
            {"z", junction.getSourceZ()},
            {"delta", junction.getDeltaY()}});
    }
    result["junctions"] = std::move(junctionJson);

    // 采样网格按起点包围盒派生：两侧起点一致时网格一致，起点不一致会由 structure_starts
    // 直接暴露，不需要在这里重复校验。
    const auto startBox = start->getBoundingBox();
    result["startBox"] = boxToJson(startBox);
    json values = json::array();
    for (i32 iy = 0; iy < GRID_Y; ++iy) {
        const i32 y = sampleCoordinate(iy, GRID_Y, startBox.minY() - SAMPLE_MARGIN, startBox.maxY() + SAMPLE_MARGIN);
        for (i32 iz = 0; iz < GRID_Z; ++iz) {
            const i32 z =
                sampleCoordinate(iz, GRID_Z, startBox.minZ() - SAMPLE_MARGIN, startBox.maxZ() + SAMPLE_MARGIN);
            for (i32 ix = 0; ix < GRID_X; ++ix) {
                const i32 x =
                    sampleCoordinate(ix, GRID_X, startBox.minX() - SAMPLE_MARGIN, startBox.maxX() + SAMPLE_MARGIN);
                values.push_back(encodeFloat(beardifier.compute(x, y, z)));
            }
        }
    }
    result["values"] = std::move(values);
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(beardifier_density, nullptr, &runLine)

} // namespace mc::diff
