/**
 * @file stronghold_room_crossing.cpp
 * @brief 差分能力：单个要塞房间构件（RoomCrossing）的放置公式。
 *
 * 整树比对只能指出「第几个构件开始分叉」，却分不清是房间自己算错位置，还是随机流在更早
 * 的一步就分开了。这条能力把范围收窄到一个构件：给定门的位置、朝向与随机源种子，房间的
 * 包围盒必须与原版逐坐标一致，且构造期消耗的随机数个数也必须一致——后者用「构造之后再
 * 抽一次」的返回值间接验证。
 *
 * 单元由 `harnesses/stronghold_room_crossing.ts` 给出，本探针不提供 `list`。
 */

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "probe.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/structures/StrongholdPieces.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using namespace world::gen::structure;

/** 由名字解析方向（与 oracle 的 `Direction.byName` 同口径）。 */
Direction parseDirection(std::string_view name)
{
    if (name == "NORTH") {
        return Direction::North;
    }
    if (name == "SOUTH") {
        return Direction::South;
    }
    if (name == "WEST") {
        return Direction::West;
    }
    if (name == "EAST") {
        return Direction::East;
    }
    return Direction::None;
}

/** 朝向名（与原版 `Rotation.name()` 同口径）。 */
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

/** 导入口朝向名（本项目把 setCoordBaseMode 记下的方向作为构件朝向）。 */
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

std::string runLine(std::string_view inputLine)
{
    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const Direction direction = parseDirection(input.at("direction").get<std::string>());
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 x = input.at("x").get<i32>();
    const i32 y = input.at("y").get<i32>();
    const i32 z = input.at("z").get<i32>();
    const i32 depth = input.at("depth").get<i32>();

    // 原版房间构造用的是 LegacyRandomSource（线性同余），本项目要用同一个后端才能对齐抽取序列。
    math::JavaLegacyRandom rng(seed);
    // 包围盒按原版 `orientBox(x, y, z, -4, -1, 0, 11, 7, 11, direction)` 展开：
    // 这条能力要验证的正是「同一扇门在四个朝向下落出同一个盒子」。
    const StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 11, 7, 11, direction);
    auto piece = std::make_unique<StrongholdRoomCrossing>(StrongholdPieceTypes::ROOM_CROSSING,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
    piece->setExpansionDepth(depth);

    json result = json{{"unit", input.value("unit", "")}};
    if (piece == nullptr) {
        result["placed"] = false;
        return result.dump();
    }
    result["placed"] = true;
    const auto& placedBox = piece->getBoundingBox();
    result["box"] = {
        placedBox.minX(), placedBox.minY(), placedBox.minZ(), placedBox.maxX(), placedBox.maxY(), placedBox.maxZ()};
    result["rotation"] = rotationName(piece->getRotation());
    result["orientation"] = orientationName(*piece);
    result["roomType"] = piece->roomType();
    // 构造之后再抽一次：若两侧消耗的随机数个数不同，这个值就会分叉。
    math::JavaLegacyRandom reference(seed);
    result["referenceNextInt5"] = reference.nextInt(5);
    result["nextInt5AfterConstruction"] = rng.nextInt(5);
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(stronghold_room_crossing, nullptr, &runLine)

} // namespace mc::diff
