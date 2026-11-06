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
 * @file end_city_pieces.cpp
 * @brief 差分能力：末地城的构件序列（逐个构件的模板名、包围盒、旋转、生成深度）。
 *
 * 末地城完全靠「生成器递归 + 逐个构件拼接」组装，没有任何模板池，因此
 * `structure_starts` 对它只能看到整体包围盒；一处随机数消耗不同或一处邻近性判定
 * 语义不同，报告里就只剩「整体不一致」。这条能力把整座城拆到构件级：同一份种子与
 * 区块坐标下，逐条列出构件的生成顺序、模板名、包围盒、旋转与生成深度，
 * 于是「从第几个构件开始分歧」可以直接看出来。
 *
 * 1.21.11 的末地城有一个特殊之处：`recursiveChildren` 在把一批构件提交进主列表之前，
 * 会用一次 `nextInt()` 的返回值覆盖这批构件的 `genDepth`，然后只与 `genDepth` 不同的
 * 已有构件比对包围盒。桥段的 `genDepth` 被设成 -1 表示豁免。这条规则决定了整座城的
 * 形状（重叠的兄弟构件允许共存、桥可以贴着塔出发），所以本能力把 `genDepth` 一并输出。
 *
 * 单元是「维度/种子/区块坐标」三元组，由 `harnesses/end_city_pieces.ts` 给出，
 * 本探针不提供 `list`。
 */

#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "probe.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructurePlacementState.hpp"
#include "server/world/gen/structure/StructureSet.hpp"
#include "structure_probe_common.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/** 旋转名：与 oracle 侧的 `Rotation` 枚举名一致。 */
const char* rotationName(Rotation rotation)
{
    switch (rotation) {
        case Rotation::None:
            return "NONE";
        case Rotation::Clockwise90:
            return "CLOCKWISE_90";
        case Rotation::Clockwise180:
            return "CLOCKWISE_180";
        case Rotation::CounterClockwise90:
            return "COUNTERCLOCKWISE_90";
        default:
            return "NONE";
    }
}

/**
 * @brief 构件名
 *
 * 构件标识在 `CityTemplate` 里存的是完整资源位置（`minecraft:end_city/<name>`），而
 * oracle 侧读到的是原版 `TemplateStructurePiece.templateName`（只有 `<name>`，前缀由
 * `makeTemplateLocation()` 补齐）。两侧都去掉命名空间与目录，只比构件名本身：
 * `end_city/` 前缀是常量，比它不会增加任何信息，却会让每条差异多一行噪声。
 */
std::string pieceTemplateName(std::string_view templateLocation)
{
    constexpr std::string_view kDirectory = "end_city/";
    const size_t slash = templateLocation.rfind('/');
    const size_t directory = templateLocation.rfind(kDirectory);
    const size_t begin = (directory != std::string_view::npos) ? directory + kDirectory.size() : slash + 1;
    return std::string(templateLocation.substr(begin));
}

std::string runLine(std::string_view inputLine)
{
    initializeStructureRegistries();

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string dimensionName = input.at("dimension").get<std::string>();
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 chunkX = input.at("chunkX").get<i32>();
    const i32 chunkZ = input.at("chunkZ").get<i32>();

    WorldSetup setup = makeWorldSetup(dimensionName, seed);
    auto biomeSource = makeBiomeSource(dimensionName, *setup.randomState);
    NoiseChunkGenerator generator(std::move(setup.settings), std::move(biomeSource), setup.randomState);

    world::gen::structure::StructurePlacementState placementState(*generator.getBiomeSource(), static_cast<i64>(seed));
    auto& registry = world::gen::structure::StructureSetRegistry::instance();
    for (const auto& structureSetPtr : registry.getAll()) {
        if (structureSetPtr) {
            placementState.addStructureSet(*structureSetPtr);
        }
    }

    json pieces = json::array();
    bool found = false;

    // 结构起点阶段的依赖半径为 0。region 的区块数组按「从左上到右下」的绝对顺序排列，
    // 主区块位于下标正中：大小必须是奇数（半径 1 → 9 个区块），写 1 会让主区块取不到。
    // region 的作用是让落点扫描看到已放置结构对地形的整平贡献（原版经 RandomState 的
    // NoiseChunk 缓存在同一进程内取得）。
    std::vector<std::unique_ptr<::mc::world::chunk::ChunkPrimer>> regionOwners;
    std::vector<IChunk*> regionChunks;
    for (i32 dz = -1; dz <= 1; ++dz) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            regionOwners.push_back(std::make_unique<::mc::world::chunk::ChunkPrimer>(chunkX + dx, chunkZ + dz));
            regionChunks.push_back(regionOwners.back().get());
        }
    }
    ::mc::WorldGenRegion region(chunkX, chunkZ, 1, std::move(regionChunks));
    ::mc::world::chunk::ChunkPrimer& regionPrimer = *regionOwners[4];

    for (const auto* structureSet : placementState.structureSets()) {
        NoiseChunkGenerator::StructureSelection selection =
            generator._generateStructureStartsForSet(region, placementState, *structureSet, chunkX, chunkZ);
        if (selection.start == nullptr) {
            continue;
        }
        if (selection.structureId.toString() != "minecraft:end_city") {
            // 非目标结构也要登记回本区块：后续结构集的落点扫描需要看到它的地形整平贡献。
            regionPrimer.addStructureStart(selection.structureId,
                std::shared_ptr<world::gen::structure::StructureStart>(std::move(selection.start)));
            continue;
        }
        found = true;
        for (const auto& piece : selection.start->pieces()) {
            const auto& bbox = piece->getBoundingBox();
            pieces.push_back(json{{"template", pieceTemplateName(piece->templateLocation())},
                {"box", {bbox.minX(), bbox.minY(), bbox.minZ(), bbox.maxX(), bbox.maxY(), bbox.maxZ()}},
                {"rotation", rotationName(piece->getRotation())},
                {"genDepth", piece->genDepth()}});
        }
    }

    return json{{"unit", input.value("unit", "")}, {"present", found}, {"pieces", std::move(pieces)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(end_city_pieces, nullptr, &runLine)

} // namespace mc::diff
