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
 * @file structure_starts.cpp
 * @brief 差分能力：结构起点（STRUCTURE_STARTS 阶段的产物）。
 *
 * 输入一行一个样本：维度名、世界种子、区块坐标。输出该区块里落下的全部结构起点，
 * 每个起点给出被选中的结构、整体包围盒、各个片段（顺序即装配顺序）与地面高度偏移、
 * 以及每个片段拥有的连接点。
 *
 * 这一层不写任何方块、不依赖邻区：STRUCTURE_STARTS 阶段只读本区块坐标与结构集，
 * 因此逐区块独立测量即可，与区块的处理顺序无关。
 *
 * 单元由共同语料决定（`harnesses/structure_starts.ts`），本探针不提供 `list`。
 */

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "common/core/GameDirectory.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "probe.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/BiomeLoader.hpp"
#include "server/world/gen/biome/BiomeTagLoader.hpp"
#include "server/world/gen/biome/source/EndBiomeSource.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/carver/ConfiguredCarverLoader.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/density/DensityFunctionLoader.hpp"
#include "server/world/gen/feature/ConfiguredFeatureLoader.hpp"
#include "server/world/gen/feature/FeatureTypeRegistry.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/feature/template/TemplateManagerHostBinding.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/jigsaw/ProcessorListLoader.hpp"
#include "server/world/gen/noise/NoiseLoader.hpp"
#include "server/world/gen/placement/PlacedFeatureLoader.hpp"
#include "server/world/gen/placement/PlacementRegistry.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/settings/NoiseSettingsLoader.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureDefinitionLoader.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/StructurePlacementState.hpp"
#include "server/world/gen/structure/StructureSetLoader.hpp"
#include "server/world/gen/structure/StructureTagLoader.hpp"
#include "server/world/gen/structure/pools/Pools.hpp"
#include "server/world/gen/structure/structures/EndCityStructure.hpp"
#include "server/world/gen/structure/structures/NetherFossilStructure.hpp"
#include "server/world/gen/structure/structures/OceanRuinStructure.hpp"
#include "server/world/gen/structure/structures/RuinedPortalStructure.hpp"
#include "structure_probe_common.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 按服务端启动顺序装配全部世界生成注册表。
 *
 * 实现与 `mineshaft_pieces` 共用，见 `structure_probe_common.hpp`。
 */
void initializeRegistries()
{
    initializeStructureRegistries();
}

// ============================================================================
// 维度
// ============================================================================

/**
 * @brief 按维度名构造维度设置与随机状态
 *
 * 群系源必须与生成器共享**同一个** RandomState 实例：群系源内部持有该实例的裸指针
 * （采样器指针 + 末地群系的随机状态指针），两份独立实例会让群系采样读到已释放的内存。
 */

// ============================================================================
// 摘要
// ============================================================================

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
    }
    return "NONE";
}

const char* projectionName(StructurePieceProjection projection)
{
    return projection == StructurePieceProjection::Rigid ? "rigid" : "terrain_matching";
}

/// 一个片段的摘要。字段与 oracle 声明的一一对应，多出的字段不参与比较。
json summarisePiece(const world::gen::structure::StructurePiece& piece)
{
    const auto box = piece.getBoundingBox();
    // 接线按收集顺序原样输出，与 oracle 的 `getJunctions()` 同口径。接线在父构件上按连接点
    // 遍历序追加、在子构件上只有被挂接时的那一条，两条实现都必须逐项对齐；若此处再排序，
    // 装配阶段真的把接线记到了错误的连接点上也会被掩盖掉。
    json junctions = json::array();
    for (const auto& junction : piece.getJunctions()) {
        junctions.push_back(json{{"x", junction.getSourceX()},
            {"groundY", junction.getSourceGroundY()},
            {"z", junction.getSourceZ()},
            {"deltaY", junction.getDeltaY()},
            {"dest",
                junction.getDestProjection() == world::gen::jigsaw::JigsawPlacementBehaviour::Rigid
                    ? "rigid"
                    : "terrain_matching"}});
    }
    // `template` 的语义是「拼图池元素标识」，不是「构件用到的模板文件」：oracle 侧只对
    // `PoolElementStructurePiece` 读取模板位置，要塞房间、矿井走廊、末地城构件这些
    // **程序化拼装**的非池元素片段一律输出空串。末地城的 `CityTemplate` 虽然有模板文件，
    // 但它不是池元素，因此同样输出空串——否则这条能力对末地城永远报「template 不同」，
    // 把真正的几何差异淹没掉。构件模板名由专门的能力（end_city_pieces）比对。
    const std::string templateId = piece.isJigsawPiece() ? std::string(piece.templateLocation()) : std::string();

    // 模板文件名：oracle 侧对任意 TemplateStructurePiece 都输出**完整资源位置**
    // （`TemplateStructurePiece.templateName` 存的是 `Identifier.toString()`），废墟传送门与
    // 末地城都不是池元素（template 为空），但两者都有独立的模板文件。末地城存的是裸文件名
    // （与 oracle 侧反射读到的值一致），不能自作主张补命名空间。
    const std::string templateFile = [&]() -> std::string {
        if (const auto* portal = dynamic_cast<const world::gen::structure::RuinedPortalPiece*>(&piece)) {
            return portal->templateName();
        }
        if (const auto* ruin = dynamic_cast<const world::gen::structure::OceanRuinPiece*>(&piece)) {
            return ruin->templateName();
        }
        if (const auto* city = dynamic_cast<const world::gen::structure::end_city::CityTemplate*>(&piece)) {
            // 末地城存的是裸文件名（`end_city/base_floor` 的文件名部分），与原版
            // `TemplateStructurePiece.templateName` 逐字一致，不能补命名空间。
            return city->templateName();
        }
        if (const auto* fossil = dynamic_cast<const world::gen::structure::NetherFossilPiece*>(&piece)) {
            // 化石存的是完整资源位置（`Identifier.toString()`），与 oracle 反射读到的值同口径。
            return fossil->templateName();
        }
        return std::string(piece.templateLocation());
    }();

    return json{{"templateFile", templateFile},
        {"box", {box.minX(), box.minY(), box.minZ(), box.maxX(), box.maxY(), box.maxZ()}},
        {"rotation", rotationName(piece.getRotation())},
        {"groundLevelDelta", piece.getGroundLevelDelta()},
        {"template", templateId},
        {"projection", projectionName(piece.getProjection())},
        {"junctions", std::move(junctions)}};
}

// ============================================================================
// 单行
// ============================================================================

std::string runLine(std::string_view inputLine)
{
    initializeRegistries();

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string dimensionName = input.at("dimension").get<std::string>();
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 chunkX = input.at("chunkX").get<i32>();
    const i32 chunkZ = input.at("chunkZ").get<i32>();

    // 群系源与生成器必须共享同一个 RandomState：气候采样器按值持有 6 个气候密度函数的
    // 裸指针，末地群系源还持有随机状态本身的裸指针；各自构造一份会让指针指向已释放对象。
    WorldSetup setup = makeWorldSetup(dimensionName, seed);
    auto biomeSource = makeBiomeSource(dimensionName, *setup.randomState);
    NoiseChunkGenerator generator(std::move(setup.settings), std::move(biomeSource), setup.randomState);

    // 放置状态要看到本维度的群系源，生成器自己持有同一份：这里从生成器重建一份用于判定的
    // 状态即可（判定只读群系源与结构集），二者必须是同一套群系。
    world::gen::structure::StructurePlacementState placementState(*generator.getBiomeSource(), static_cast<i64>(seed));
    auto& registry = world::gen::structure::StructureSetRegistry::instance();
    for (const auto& structureSetPtr : registry.getAll()) {
        if (structureSetPtr) {
            placementState.addStructureSet(*structureSetPtr);
        }
    }

    // 生成区域只含本区块：STRUCTURE_STARTS 的依赖半径为 0，落点扫描所需的其它结构信息
    // 全部经由结构引用拿到。region 的作用是让废墟传送门这类结构看到已放置结构对地形的
    // 整平贡献——原版经 RandomState 的 NoiseChunk 缓存在同一进程里拿到这份贡献，本实现
    // 必须显式把区块传进来。
    // region 的区块数组按「从左上到右下」的绝对顺序排列，主区块位于下标正中：大小必须是
    // 奇数（半径 1 → 9 个区块），写 1 会让相对坐标 (0,0) 落到下标 0 而取不到主区块。
    std::vector<std::unique_ptr<::mc::world::chunk::ChunkPrimer>> regionOwners;
    std::vector<IChunk*> regionChunks;
    for (i32 dz = -1; dz <= 1; ++dz) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            regionOwners.push_back(std::make_unique<::mc::world::chunk::ChunkPrimer>(chunkX + dx, chunkZ + dz));
            regionChunks.push_back(regionOwners.back().get());
        }
    }
    ::mc::WorldGenRegion region(chunkX, chunkZ, 1, std::move(regionChunks));
    ::mc::world::chunk::ChunkPrimer& primer = *regionOwners[4];

    json starts = json::array();

    for (const auto* structureSet : placementState.structureSets()) {
        NoiseChunkGenerator::StructureSelection selection =
            generator._generateStructureStartsForSet(region, placementState, *structureSet, chunkX, chunkZ);
        if (selection.start == nullptr) {
            continue;
        }
        const auto& start = *selection.start;
        const auto& bbox = start.getBoundingBox();
        json pieces = json::array();
        for (const auto& piece : start.pieces()) {
            pieces.push_back(summarisePiece(*piece));
        }
        starts.push_back(json{{"structure", selection.structureId.toString()},
            {"box", {bbox.minX(), bbox.minY(), bbox.minZ(), bbox.maxX(), bbox.maxY(), bbox.maxZ()}},
            {"pieces", std::move(pieces)}});

        // 起点必须登记回本区块再继续下一个结构集：原版 createStructures 每择出一个起点就
        // 写回区块，后续结构的落点扫描经 RandomState 的 NoiseChunk 看到它的地形整平贡献
        // （Beardifier）。不登记的话废墟传送门等结构会按未被整平的地形算高度。
        primer.addStructureStart(
            selection.structureId, std::shared_ptr<world::gen::structure::StructureStart>(std::move(selection.start)));
    }

    return json{{"unit", input.value("unit", "")}, {"starts", std::move(starts)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(structure_starts, nullptr, &runLine)

} // namespace mc::diff
