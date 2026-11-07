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
 * @file structure_start_single.cpp
 * @brief 差分能力：单个结构在指定区块上的起点（起点位置 + 构件盒）。
 *
 * `structure_starts` 把整个结构集的选择、权重抽签和平行放置混在一起，一旦起点错位，
 * 既可能是候选判定错、也可能是某个结构多抽或少抽了随机数。这条能力绕开结构集：
 * 给定结构 id、世界种子与区块坐标，直接用这一格的 `WorldgenRandom` 调用该结构的
 * `generate`，输出起点的生成原点（第一个构件的包围盒最小角或构件列表）与全部构件。
 *
 * 单元由 `harnesses/structure_start_single.ts` 给出，本探针不提供 `list`。
 */

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/WorldConstants.hpp"
#include "probe.hpp"
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
 * @brief 按名字查结构实例
 *
 * 结构注册表由 `initializeStructureRegistries()` 从数据包装配，名字必须逐字一致；
 * 查不到说明数据包缺该结构或名字写错，属于配置错误，直接失败。
 */
const Structure& findStructure(const ResourceLocation& id)
{
    const Structure* structure = StructureRegistry::get(id);
    MC_ASSERT_RELEASE_MSG(structure != nullptr, "structure_start_single: unknown structure");
    return *structure;
}

/**
 * @brief 构件类型名（目前只有要塞片段有可比的注册名）
 *
 * 要塞片段没有模板、包围盒尺寸又高度重复，只比包围盒时"错一个片段"与"整体偏移"
 * 无法区分。原版用注册名（shs/sh5c/shpr…），这里给出同一口径的短名。
 */
std::string pieceTypeName(i32 type)
{
    switch (type) {
        case 100:
            return "shs"; // Straight
        case 101:
            return "shph"; // PrisonHall
        case 102:
            return "shlt"; // LeftTurn
        case 103:
            return "shrt"; // RightTurn
        case 104:
            return "shrc"; // RoomCrossing
        case 105:
            return "shssd"; // StraightStairsDown
        case 106:
            return "shsd"; // StairsDown
        case 107:
            return "shstart"; // StartPiece
        case 108:
            return "sh5c"; // FiveCrossing
        case 109:
            return "shcc"; // ChestCorridor
        case 110:
            return "shli"; // Library
        case 111:
            return "shpr"; // PortalRoom
        case 112:
            return "shfc"; // FillerCorridor
        default:
            return "";
    }
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

    // 结构生成用的随机源：与结构集条目选择时的实例无关，每格从 `setLargeFeatureSeed` 的
    // 初始状态起算（原版 Structure.GenerationContext.makeRandom 即如此）。
    math::WorldgenRandom structureRng(std::make_unique<math::JavaLegacyRandom>(0ULL));
    structureRng.setLargeFeatureSeed(static_cast<i64>(seed), chunkX, chunkZ);

    // 生成区域只含本区块的 3x3 邻居：起点生成阶段的依赖半径为 0，但废墟传送门这类结构
    // 需要经 region 读取地形整平贡献，因此仍与 `structure_starts` 用同样的区域形状。
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

    json pieces = json::array();
    if (start != nullptr) {
        for (const auto& piece : start->pieces()) {
            const auto& box = piece->getBoundingBox();
            // 模板名让"同一尺寸但选错模板/旋转"的差异一眼可见：只比包围盒时这类差异
            // 会被误读成位置偏差。非拼图构件的模板名为空串。
            pieces.push_back(json{{"box", {box.minX(), box.minY(), box.minZ(), box.maxX(), box.maxY(), box.maxZ()}},
                {"rotation", static_cast<i32>(piece->getRotation())},
                {"template", std::string(piece->templateLocation())},
                {"pieceType", pieceTypeName(piece->type())}});
        }
    }

    return json{{"unit", input.value("unit", "")},
        {"valid", start != nullptr && start->isValid()},
        {"pieces", std::move(pieces)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(structure_start_single, nullptr, &runLine)

} // namespace mc::diff
