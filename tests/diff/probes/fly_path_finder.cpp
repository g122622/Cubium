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
 * @file fly_path_finder.cpp
 * @brief 飞行寻路差分探针（对应 oracle `FlyPathfinderOracle`）。
 *
 * 与 `path_finder` 用同一份场景库，只把求值器换成 `FlyNodeEvaluator`：
 * 地形完全相同，因此两侧差异只可能来自飞行求值器本身（26 邻居枚举、起点枚举、
 * 飞行专用的类型判定）。
 */

#include "common/entity/ai/pathfinding/FlyNodeEvaluator.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathFinder.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "path_scene.hpp"
#include "probe.hpp"
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::diff {
namespace {

using namespace entity::ai::pathfinding;
using nlohmann::json;

void initialize()
{
    static const bool done = [] {
        VanillaBlocks::initialize();
        return true;
    }();
    MC_UNUSED(done);
}

/// 与 `path_finder` 共用同一段布局逻辑，保证两种求值器看到的是同一块地形。
Scene buildScene(const json& input)
{
    Scene scene;
    const auto& origin = input.at("origin");
    scene.originX = origin.at("x").get<i32>();
    scene.originY = origin.at("y").get<i32>();
    scene.originZ = origin.at("z").get<i32>();
    scene.sizeX = input.at("sizeX").get<i32>();
    scene.sizeY = input.at("sizeY").get<i32>();
    scene.sizeZ = input.at("sizeZ").get<i32>();

    for (const auto& cell : input.at("cells")) {
        scene.region.setBlock(scene.originX + cell.at("dx").get<i32>(),
            scene.originY + cell.at("dy").get<i32>(),
            scene.originZ + cell.at("dz").get<i32>(),
            cell.at("block").get<std::string>());
    }

    const auto& mobInput = input.at("mob");
    const i32 maxFallDistance = mobInput.contains("maxFallDistance") ? mobInput.at("maxFallDistance").get<i32>() : -1;
    scene.mob = std::make_unique<SceneMob>(mobInput.at("width").get<f32>(),
        mobInput.at("height").get<f32>(),
        mobInput.at("stepHeight").get<f32>(),
        mobInput.at("onGround").get<bool>(),
        mobInput.at("inWater").get<bool>(),
        maxFallDistance);
    const auto& position = mobInput.at("position");
    scene.mobPosition = Vector3d(static_cast<f64>(scene.originX) + position.at("x").get<f64>(),
        static_cast<f64>(scene.originY) + position.at("y").get<f64>(),
        static_cast<f64>(scene.originZ) + position.at("z").get<f64>());
    scene.mob->setPosition(scene.mobPosition.x, scene.mobPosition.y, scene.mobPosition.z);

    for (const auto& entry : mobInput.at("malus")) {
        scene.mob->setMalus(parsePathType(entry.at("type").get<std::string>()), entry.at("value").get<f32>());
    }

    const auto& flags = input.at("flags");
    scene.canPassDoors = flags.at("canPassDoors").get<bool>();
    scene.canOpenDoors = flags.at("canOpenDoors").get<bool>();
    scene.canFloat = flags.at("canFloat").get<bool>();
    scene.canWalkOverFences = flags.at("canWalkOverFences").get<bool>();
    return scene;
}

/// 路径节点条目：只比坐标与类型（代价浮点末位会因求值顺序不同而差 1 ulp）。
json nodeObject(const PathPoint& node)
{
    return json{{"x", node.x()}, {"y", node.y()}, {"z", node.z()}, {"type", getPathTypeName(node.type())}};
}

std::string runLine(std::string_view line)
{
    initialize();
    const json input = json::parse(line);
    Scene scene = buildScene(input);

    json result;
    result["unit"] = input.at("unit");

    // 1. 起点与 26 邻居枚举。
    {
        FlyNodeEvaluator evaluator;
        evaluator.prepare(scene.region, *scene.mob, scene.mobPosition);
        PathPoint* start = evaluator.getStart();
        if (start == nullptr) {
            result["start"] = nullptr;
            result["neighbors"] = json::array();
            evaluator.done();
            result["path"] = nullptr;
            return result.dump();
        }
        result["start"] =
            json{{"x", start->x()}, {"y", start->y()}, {"z", start->z()}, {"type", getPathTypeName(start->type())}};

        std::array<PathPoint*, NodeEvaluator::maxNeighbors()> buffer{};
        const i32 count = evaluator.getNeighbors(buffer.data(), start);
        json neighbors = json::array();
        for (i32 index = 0; index < count; ++index) {
            const PathPoint& node = *buffer[static_cast<size_t>(index)];
            neighbors.push_back(json{{"x", node.x()},
                {"y", node.y()},
                {"z", node.z()},
                {"type", getPathTypeName(node.type())},
                {"malus", node.costMalus()}});
        }
        result["neighbors"] = std::move(neighbors);
        evaluator.done();
    }

    // 2. 正式搜索。
    PathFinder finder(std::make_unique<FlyNodeEvaluator>(), input.at("maxVisitedNodes").get<i32>());
    std::vector<BlockPos> targets;
    for (const auto& target : input.at("targets")) {
        targets.emplace_back(scene.originX + target.at("x").get<i32>(),
            scene.originY + target.at("y").get<i32>(),
            scene.originZ + target.at("z").get<i32>());
    }
    const auto path = finder.findPath(scene.region,
        *scene.mob,
        scene.mobPosition,
        targets,
        input.at("maxPathLength").get<f32>(),
        input.at("reachRange").get<i32>(),
        input.at("maxVisitedNodesMultiplier").get<f32>());
    if (!path.has_value()) {
        result["path"] = nullptr;
        return result.dump();
    }

    json nodes = json::array();
    for (size_t index = 0; index < path->nodeCount(); ++index) {
        nodes.push_back(nodeObject(path->node(index)));
    }
    result["path"] = std::move(nodes);
    result["canReach"] = path->canReach();
    const PathPoint* end = path->endNode();
    result["endX"] = end != nullptr ? end->x() : 0;
    result["endY"] = end != nullptr ? end->y() : 0;
    result["endZ"] = end != nullptr ? end->z() : 0;
    return result.dump();
}

void writeList(FILE* out)
{
    initialize();
    MC_UNUSED(out);
}

} // namespace
MC_DIFF_DEFINE_PROBE("fly_path_finder", &writeList, &runLine)
} // namespace mc::diff
