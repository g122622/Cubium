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
 * @file jigsaw_joint_order.cpp
 * @brief 差分能力：拼图模板的连接点洗牌顺序。
 *
 * `structure_starts` 只看装配整体，一处随机数消耗不同就让整棵树从某个构件开始分叉，
 * 报告里只剩「N 处差异」这种无法定位的信号。这条能力把最小的一步单独拉出来：
 * 给定模板、旋转与随机种子，列出 `getShuffledJoints` 的结果顺序（洗牌 → 按
 * selectionPriority 降序稳定排序）。
 *
 * 洗牌消耗的随机数次数由连接点数量决定：n 个连接点做 n-1 次交换。任何一次多余的消耗
 * 都会让之后所有连接点的处理顺序错位——表现为装配结果里相邻构件的旋转互换，而构件
 * 类型、包围盒尺寸都可能仍然相同。本能力正是把这种「顺序错位」变成可逐条比对的差异。
 *
 * 输入一行一个样本：模板 id、旋转、随机种子。输出连接点顺序（含位置与优先级）。
 */

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "probe.hpp"
#include "server/world/gen/jigsaw/JigsawPiece.hpp"
#include "server/world/gen/jigsaw/JigsawTransform.hpp"
#include "server/world/gen/jigsaw/JigsawTypes.hpp"
#include "server/world/gen/jigsaw/SingleJigsawPiece.hpp"
#include "server/world/gen/jigsaw/TemplatePoolRegistry.hpp"
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
            return "UNKNOWN";
    }
}

Rotation parseRotation(std::string_view name)
{
    if (name == "NONE") {
        return Rotation::None;
    }
    if (name == "CLOCKWISE_90") {
        return Rotation::Clockwise90;
    }
    if (name == "CLOCKWISE_180") {
        return Rotation::Clockwise180;
    }
    if (name == "COUNTERCLOCKWISE_90") {
        return Rotation::CounterClockwise90;
    }
    throw std::runtime_error("jigsaw_joint_order: unknown rotation '" + std::string(name) + "'");
}

void initializeRegistries()
{
    initializeStructureRegistries();
}

std::string runLine(std::string_view inputLine)
{
    initializeRegistries();

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string templateId = input.at("template").get<std::string>();
    const Rotation rotation = parseRotation(input.at("rotation").get<std::string>());
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());

    // 与装配期完全相同的构造路径：SingleJigsawPiece 在首次访问连接点时从数据包模板解析，
    // 因此这里拿到的顺序基线与装配期一致，只差洗牌本身。
    world::gen::jigsaw::SingleJigsawPiece piece(templateId, world::gen::jigsaw::JigsawPlacementBehaviour::Rigid);

    math::WorldgenRandom rng(std::make_unique<math::JavaLegacyRandom>(seed));

    // 先按给定旋转取一次变换后的连接点，再走与装配完全相同的洗牌 + 稳定排序。
    // 旋转不改变洗牌消耗，但改变每个连接点的朝向与位置，因而改变匹配结果。
    const auto joints = piece.getShuffledJoints(rng);

    // 连接点坐标必须按原版 getJigsaws(pos, rotation) 变换到世界坐标：洗牌与排序只决定
    // **顺序**，而顺序相同、坐标方向相反（未做旋转）会让整条比对失去意义。
    const auto jointSize = piece.getSize();
    (void)jointSize;

    // 旋转后的朝向（facing + top）必须一并比对：匹配只看"正面相反、aligned 时顶面一致"，
    // 坐标与顺序完全相同而朝向旋转错一格时，装配会选到另一个旋转的候选块，
    // 表现为"同一模板、不同旋转、位置相邻"的差异，只比坐标根本定位不到。
    const auto directionName = [](Direction direction) -> const char* {
        switch (direction) {
            case Direction::Down:
                return "down";
            case Direction::Up:
                return "up";
            case Direction::North:
                return "north";
            case Direction::South:
                return "south";
            case Direction::West:
                return "west";
            case Direction::East:
                return "east";
            default:
                return "none";
        }
    };

    json result = json::array();
    for (const auto& joint : joints) {
        const BlockPos transformed = world::gen::jigsaw::JigsawTransform::transformPosition(
            joint.sourcePos, rotation, Mirror::None, BlockPos(0, 0, 0));
        const auto rotatedJoint = world::gen::jigsaw::JigsawOrientations::rotate(joint.orientation, rotation);
        result.push_back(json{{"x", transformed.x},
            {"y", transformed.y},
            {"z", transformed.z},
            {"name", joint.sourceName},
            {"target", joint.targetPool},
            {"selectionPriority", joint.selectionPriority},
            {"placementPriority", joint.placementPriority},
            {"front", directionName(world::gen::jigsaw::JigsawOrientations::getFacing(rotatedJoint))},
            {"top", directionName(world::gen::jigsaw::JigsawOrientations::getRotation(rotatedJoint))},
            {"joint", joint.jointType == world::gen::jigsaw::JigsawJointType::Rollable ? "rollable" : "aligned"}});
    }

    return json{{"unit", input.value("unit", "")}, {"rotation", rotationName(rotation)}, {"joints", std::move(result)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(jigsaw_joint_order, nullptr, &runLine)

} // namespace mc::diff
