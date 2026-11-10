/**
 * @file stronghold_pieces.cpp
 * @brief 差分能力：要塞的构件序列（逐个构件的类型、包围盒、朝向、生成深度）。
 *
 * `structure_starts` 把要塞当成一整块比较：要塞动辄两三百个构件，一处随机数消耗不同
 * 就让整棵树从某个构件开始分叉，报告里只剩「这个区块不一致」，无从定位。这条能力把
 * 要塞拆成构件级：同一份种子与区块坐标下，逐条列出每个构件的生成顺序、类型、包围盒、
 * 朝向与深度，于是「从第几个构件开始分歧」可以直接看出来，再往回追那一次抽取即可。
 *
 * 单元是「维度/种子/区块坐标」三元组，由 `harnesses/stronghold_pieces.ts` 给出，
 * 本探针不提供 `list`。
 */

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "probe.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructurePlacementState.hpp"
#include "server/world/gen/structure/StructureSet.hpp"
#include "server/world/gen/structure/structures/StrongholdPieces.hpp"
#include "structure_probe_common.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using namespace world::gen::structure;

/**
 * @brief 构件类型名（原版 `StructurePieceType` 注册 id 的全大写形式）
 *
 * 未知类型直接印成数字，让「出现了意料之外的构件」在报告里立刻可见。
 */
std::string pieceTypeName(i32 type)
{
    switch (type) {
        case StrongholdPieceTypes::STRAIGHT:
            return "SHS";
        case StrongholdPieceTypes::PRISON:
            return "SHPH";
        case StrongholdPieceTypes::LEFT_TURN:
            return "SHLT";
        case StrongholdPieceTypes::RIGHT_TURN:
            return "SHRT";
        case StrongholdPieceTypes::ROOM_CROSSING:
            return "SHRC";
        case StrongholdPieceTypes::STAIRS_STRAIGHT:
            return "SHSSD";
        case StrongholdPieceTypes::STAIRS:
            return "SHSD";
        case StrongholdPieceTypes::START_STAIRS:
            return "SHSTART";
        case StrongholdPieceTypes::CROSSING:
            return "SH5C";
        case StrongholdPieceTypes::CHEST_CORRIDOR:
            return "SHCC";
        case StrongholdPieceTypes::LIBRARY:
            return "SHLI";
        case StrongholdPieceTypes::PORTAL_ROOM:
            return "SHPR";
        case StrongholdPieceTypes::CORRIDOR:
            return "SHFC";
        default:
            return std::to_string(type);
    }
}

/**
 * @brief 构件在原版里的 `getOrientation()`
 *
 * 原版把朝向存成独立字段：构造时 `setOrientation` 写入，房间一类从不设朝向的构件为 null。
 * 本项目同位置保存的是 `setCoordBaseMode` 记下的方向，直接读取即可——
 * 用 rotation/mirror 反推会把 NORTH 与「未设朝向」混为一谈（两者旋转相同）。
 */
const char* orientationName(const StructurePiece& piece)
{
    switch (piece.orientation()) {
        case Direction::North:
            return "NORTH";
        case Direction::South:
            return "SOUTH";
        case Direction::West:
            return "WEST";
        case Direction::East:
            return "EAST";
        default:
            return "null";
    }
}

/** 朝向名：直接取构件记录的 Rotation，与 structure_starts 的口径一致。 */
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

std::string runLine(std::string_view inputLine)
{
    initializeStructureRegistries();

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string dimensionName = input.at("dimension").get<std::string>();
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const const i32 chunkX = input.at("chunkX").get<i32>();
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
    json startBox = nullptr;
    bool found = false;

    // 区域构造同 mineshaft_pieces：只用于让落点扫描看到已放置结构的地形整平贡献。
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
        if (selection.structureId.toString() != "minecraft:stronghold") {
            regionPrimer.addStructureStart(selection.structureId,
                std::shared_ptr<world::gen::structure::StructureStart>(std::move(selection.start)));
            continue;
        }
        found = true;
        // 起始楼梯的包围盒（以及整体移位量）在实体化校验之外就定了，这里是它与参照侧
        // 对齐的第一处信息：能区分「整体竖平移不同」和「装配本身分叉」。
        const auto& startBBox = selection.start->getBoundingBox();
        startBox = json::array({startBBox.minX(),
            startBBox.minY(),
            startBBox.minZ(),
            startBBox.maxX(),
            startBBox.maxY(),
            startBBox.maxZ()});
        for (const auto& piece : selection.start->pieces()) {
            const auto& bbox = piece->getBoundingBox();

            pieces.push_back(json{{"type", pieceTypeName(piece->getComponentType())},
                {"box", {bbox.minX(), bbox.minY(), bbox.minZ(), bbox.maxX(), bbox.maxY(), bbox.maxZ()}},
                {"rotation", rotationName(piece->getRotation())},
                // 原版 `getGenDepth()` 是分支深度（起始楼梯 0，逐层 +1），不是
                // `StructurePiece.genDepth` 那个用于重叠判定的标识。
                {"genDepth", piece->expansionDepth()},
                // 原版的 `getOrientation()` 是构件构造时 setOrientation 记下的朝向，
                // 未设过（房间、起始楼梯）则为 null。普通构件把朝向编码进
                // rotation + mirror，用同一张映射表反推；起始楼梯直接读自己的字段。
                {"orientation", orientationName(*piece)}});
        }
    }

    return json{{"unit", input.value("unit", "")},
        {"present", found},
        {"startBox", std::move(startBox)},
        {"pieces", std::move(pieces)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(stronghold_pieces, nullptr, &runLine)

} // namespace mc::diff
