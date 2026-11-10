/**
 * @file stronghold_small_door.cpp
 * @brief 差分能力：面向小门的三个落点公式（正向 / 左门 / 右门）。
 *
 * `stronghold_pieces` 只能在整棵树里指出「第几个构件开始分叉」，而一旦某个构件的包围盒
 * 偏移一格，后续的碰撞判定全都会换一种走法，随机流随之整片错位——从构件序列根本看不出
 * 是哪一个公式错了。这条能力把三个公式单独拎出来：给定父构件的原点、朝向与尺寸，
 * 直接比对落点坐标，以及落点盒（在无碰撞的空场景里，三层嵌套各取一个候选）。
 *
 * 单元由 `harnesses/stronghold_small_door.ts` 给出，本探针不提供 `list`。
 */

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/WorldConstants.hpp"
#include "probe.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
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

/** 构件类型名（与 oracle 的 `StructurePieceType` 注册名同口径）。 */
const char* pieceTypeName(i32 type)
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
            return "unknown";
    }
}

/** 构件在原版里的 `getOrientation()`（未设过的房间类为 "null"）。 */
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

/** 找到本探针在 `pieces` 里读出的起始楼梯（永远是第一个元素）。 */
StrongholdStartStairs* startOf(const std::vector<std::unique_ptr<StructurePiece>>& pieces)
{
    for (const auto& piece : pieces) {
        if (auto* start = dynamic_cast<StrongholdStartStairs*>(piece.get())) {
            return start;
        }
    }
    return nullptr;
}

std::string runLine(std::string_view inputLine)
{
    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const Direction direction = parseDirection(input.at("direction").get<std::string>());
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 x = input.at("x").get<i32>();
    const i32 y = input.at("y").get<i32>();
    const i32 z = input.at("z").get<i32>();
    const i32 sizeX = input.at("sizeX").get<i32>();
    const i32 sizeY = input.at("sizeY").get<i32>();
    const i32 sizeZ = input.at("sizeZ").get<i32>();
    const i32 offsetX = input.at("offsetX").get<i32>();
    const i32 offsetY = input.at("offsetY").get<i32>();
    const i32 verticalOffset = input.at("verticalOffset").get<i32>();
    const i32 positionOffset = input.at("positionOffset").get<i32>();
    // 父构件自身的分支深度由 harness 指定，落点子构件的深度必须恰好是它 +1。
    const i32 parentDepth = input.at("parentDepth").get<i32>();

    struct Slot {
        const char* key;
        u64 seed;
        bool forward;
        bool left;
    };

    const Slot slots[] = {
        {"forward", seed, true, false}, {"left", seed + 1, false, true}, {"right", seed + 2, false, false}};

    // 两件的包围盒都由输入几何直接重建，只有构造期的抽取顺序需要与 oracle 保持一致：
    // 落点子构件本身也要抽若干随机数，两侧若差一步，结果就无法归因到公式上。

    json result = json{{"unit", input.value("unit", "")}};

    // 三个落点各自开一条独立随机流（种子 +1/+2 与 oracle 对齐）：落点子构件本身也要抽若干
    // 随机数，共用一条流会让后一个落点看到前一个消耗过的序列，差异就无法归因。
    const auto record = [&result](const char* key, const json& entry) {
        result[key] = entry;
    };

    for (const Slot& slot : slots) {
        // 两侧都用同一个直廊实例做父构件（构造顺序一致），朝向与盒子随后被直接改写。
        math::JavaLegacyRandom slotRng(slot.seed);
        std::vector<std::unique_ptr<StructurePiece>> pieces;
        auto slotParent = std::make_unique<StrongholdStraight>(StrongholdPieceTypes::STRAIGHT,
            slotRng,
            x,
            y,
            z,
            x + sizeX - 1,
            y + sizeY - 1,
            z + sizeZ - 1,
            x,
            y,
            z,
            sizeX,
            sizeZ,
            offsetX,
            offsetY,
            direction);
        slotParent->setExpansionDepth(parentDepth);
        // 父构件的盒子直接按 `orientBox` 展开（不一定走构造期那条随机分支）；朝向与起点另设。
        slotParent->setBoundingBox(
            StructureBoundingBox::orientBox(x, y, z, offsetX, offsetY, 0, sizeX, sizeY, sizeZ, direction));
        slotParent->setCoordBaseMode(Directions::opposite(direction));
        // 起始楼梯在父构件之后构造：构造顺序必须与 oracle 一致，否则随机流错开。
        auto startOwner = std::make_unique<StrongholdStartStairs>(slotRng, x, z, direction);
        startOwner->setExpansionDepth(0);
        startOwner->setBoundingBox(slotParent->getBoundingBox());
        startOwner->setCoordBaseMode(Directions::opposite(direction));
        pieces.push_back(std::move(startOwner));
        pieces.push_back(std::move(slotParent));
        StrongholdStraight* parentPtr = dynamic_cast<StrongholdStraight*>(pieces.back().get());

        StructurePiece* child = nullptr;
        if (slot.forward) {
            child = parentPtr->getNextComponentNormal(
                startOf(pieces), *parentPtr, pieces, slotRng, positionOffset, verticalOffset);
        } else if (slot.left) {
            child = parentPtr->getNextComponentX(
                startOf(pieces), *parentPtr, pieces, slotRng, verticalOffset, positionOffset);
        } else {
            child = parentPtr->getNextComponentZ(
                startOf(pieces), *parentPtr, pieces, slotRng, verticalOffset, positionOffset);
        }
        // 原始输出：null 或 [minX,minY,minZ,maxX,maxY,maxZ,type,depth,rotation]，与 oracle 逐项对应。
        if (child == nullptr) {
            record(slot.key, json(nullptr));
            continue;
        }
        const auto& placedBox = child->getBoundingBox();
        record(slot.key,
            json::array({placedBox.minX(),
                placedBox.minY(),
                placedBox.minZ(),
                placedBox.maxX(),
                placedBox.maxY(),
                placedBox.maxZ(),
                pieceTypeName(child->getComponentType()),
                child->expansionDepth(),
                orientationName(*child),
                rotationName(child->getRotation())}));
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(stronghold_small_door, nullptr, &runLine)

} // namespace mc::diff
