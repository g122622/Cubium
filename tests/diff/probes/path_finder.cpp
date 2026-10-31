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
 * @file path_finder.cpp
 * @brief A* 寻路差分探针（对应 oracle `PathFinderOracle`）。
 *
 * 输出三段：
 *   1. `start`：起点节点的坐标、类型与代价；
 *   2. `neighbors`：起点展开出的邻居序列（顺序与方向枚举一致）；
 *   3. `path`：`findPath` 返回的节点序列 + 是否到达 + 终点坐标。
 *
 * 邻居序列的顺序本身是比对对象：A* 在 `f` 相等时的取舍由展开顺序决定，
 * 因此两侧必须按同一顺序产出。
 */

#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathFinder.hpp"
#include "common/entity/ai/pathfinding/WalkNodeEvaluator.hpp"
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

/// 把一行输入解析成完整场景（与 `walk_path_type` 共用同一段布局逻辑）。
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

/// 按场景输入设好能力开关的求值器（两处都要设：邻居枚举与正式搜索各用一个实例）。
std::unique_ptr<WalkNodeEvaluator> makeEvaluator(const Scene& scene)
{
    auto evaluator = std::make_unique<WalkNodeEvaluator>();
    evaluator->setCanPassDoors(scene.canPassDoors);
    evaluator->setCanOpenDoors(scene.canOpenDoors);
    evaluator->setCanFloat(scene.canFloat);
    evaluator->setCanWalkOverFences(scene.canWalkOverFences);
    return evaluator;
}

/// 邻居条目：坐标 + 类型 + 该类型的代价（代价决定 A* 在两个候选之间的取舍）。
json neighborObject(const PathPoint& node)
{
    return json{{"x", node.x()},
        {"y", node.y()},
        {"z", node.z()},
        {"type", getPathTypeName(node.type())},
        {"malus", node.costMalus()}};
}

/// 路径节点条目：只比坐标与类型（代价的浮点末位会因求值顺序不同而差 1 ulp）。
json nodeObject(const PathPoint& node)
{
    return json{{"x", node.x()}, {"y", node.y()}, {"z", node.z()}, {"type", getPathTypeName(node.type())}};
}

std::string runLine(std::string_view line)
{
    initialize();
    const json input = json::parse(line);
    Scene scene = buildScene(input);
    const i32 maxVisitedNodes = input.at("maxVisitedNodes").get<i32>();

    json result;
    result["unit"] = input.at("unit");

    // 1. 邻居枚举：开会话、取起点、展开一次。
    {
        auto evaluator = makeEvaluator(scene);
        evaluator->prepare(scene.region, *scene.mob, scene.mobPosition);
        PathPoint* start = evaluator->getStart();
        if (start == nullptr) {
            result["start"] = nullptr;
            result["neighbors"] = json::array();
            evaluator->done();

            // 起点为空时仍要跑一次 findPath，让「两侧都算不出路径」这件事也被比对到。
            auto searchEvaluator = makeEvaluator(scene);
            PathFinder finder(std::move(searchEvaluator), maxVisitedNodes);
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
            result["path"] = nullptr;
            MC_UNUSED(path);
            return result.dump();
        }

        result["start"] = json{{"x", start->x()},
            {"y", start->y()},
            {"z", start->z()},
            {"type", getPathTypeName(start->type())},
            {"malus", start->costMalus()}};

        std::array<PathPoint*, NodeEvaluator::maxNeighbors()> buffer{};
        const i32 count = evaluator->getNeighbors(buffer.data(), start);
        json neighbors = json::array();
        for (i32 index = 0; index < count; ++index) {
            neighbors.push_back(neighborObject(*buffer[static_cast<size_t>(index)]));
        }
        result["neighbors"] = std::move(neighbors);
        evaluator->done();
    }

    // 2. 正式搜索。
    auto searchEvaluator = makeEvaluator(scene);
    PathFinder finder(std::move(searchEvaluator), maxVisitedNodes);
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
MC_DIFF_DEFINE_PROBE("path_finder", &writeList, &runLine)
} // namespace mc::diff
