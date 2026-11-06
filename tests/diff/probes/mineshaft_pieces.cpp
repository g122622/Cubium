/**
 * @file mineshaft_pieces.cpp
 * @brief 差分能力：废弃矿井的构件序列（逐个构件的类型、包围盒、朝向、生成深度）。
 *
 * `structure_starts` 只能看到「整座矿井对不对」，一处随机数消耗不同就让整棵树分叉，
 * 报告里只剩「47 处差异」这种无法定位的信号。这条能力把矿井拆成构件级：
 * 同一份种子与区块坐标下，逐条列出每个构件的生成顺序、类型、包围盒、朝向与深度，
 * 于是「从第几个构件开始分歧」可以直接看出来，再往回追那一次抽取即可。
 *
 * 单元是「维度/种子/区块坐标」三元组，由 `harnesses/mineshaft_pieces.ts` 给出，
 * 本探针不提供 `list`。
 */

#include <cstddef>
#include <cstdio>
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
#include "server/world/gen/structure/structures/MineshaftStructure.hpp"
#include "structure_probe_common.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 构件类型名。
 *
 * 与 `MineshaftPieceTypes` 的常量一一对应；未知类型直接印成数字，
 * 让「出现了意料之外的构件」在报告里立刻可见，而不是被默认成一个合法名字。
 */
std::string pieceTypeName(i32 type)
{
    switch (type) {
        case 60:
            return "room";
        case 61:
            return "corridor";
        case 62:
            return "crossing";
        case 63:
            return "stairs";
        default:
            return std::to_string(type);
    }
}

/** 朝向名：MineshaftPiece 的方向用 0=北 1=南 2=西 3=东 记录。 */
const char* directionName(i32 direction)
{
    switch (direction) {
        case 0:
            return "NORTH";
        case 1:
            return "SOUTH";
        case 2:
            return "WEST";
        case 3:
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
        default:
            return "NONE";
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
        if (selection.structureId.toString() != "minecraft:mineshaft") {
            // 非目标结构也要登记回本区块：后续结构集的落点扫描需要看到它的地形整平贡献。
            regionPrimer.addStructureStart(selection.structureId,
                std::shared_ptr<world::gen::structure::StructureStart>(std::move(selection.start)));
            continue;
        }
        found = true;
        for (const auto& piece : selection.start->pieces()) {
            const auto& bbox = piece->getBoundingBox();
            const auto type = static_cast<i32>(piece->getComponentType());
            const auto* mineshaftPiece = (type >= 60 && type <= 63)
                ? static_cast<const world::gen::structure::MineshaftPiece*>(piece.get())
                : nullptr;
            pieces.push_back(json{{"type", pieceTypeName(static_cast<i32>(piece->getComponentType()))},
                {"box", {bbox.minX(), bbox.minY(), bbox.minZ(), bbox.maxX(), bbox.maxY(), bbox.maxZ()}},
                {"rotation", rotationName(piece->getRotation())},
                {"componentType", static_cast<i32>(piece->getComponentType())},
                {"genDepth", mineshaftPiece ? mineshaftPiece->genDepth() : -1},
                // 原版只有走廊(61)与楼梯(63)在构造里调用 setOrientation；交叉点(62)与
                // 房间(60)都不调用，它们的 getOrientation() 是 null。这里必须逐类对齐，
                // 不能一律取 direction()。
                {"orientation", (type == 61 || type == 63) ? directionName(mineshaftPiece->direction()) : "null"},
                // 交叉点单独的「方向字段」：原版 getOrientation() 对它返回 null，但
                // `MineShaftCrossing.addChildren` 实际按构造时传入的方向选出口表，
                // 因此它决定了下一批构件的坐标。两侧必须逐条一致。
                {"crossDirection", type == 62 ? directionName(mineshaftPiece->direction()) : "null"}});
        }
    }

    return json{{"unit", input.value("unit", "")}, {"present", found}, {"pieces", std::move(pieces)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mineshaft_pieces, nullptr, &runLine)

} // namespace mc::diff
