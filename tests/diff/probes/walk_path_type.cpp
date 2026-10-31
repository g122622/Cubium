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
 * @file walk_path_type.cpp
 * @brief 逐格路径类型差分探针（对应 oracle `WalkPathTypeOracle`）。
 *
 * 对场景网格的每一格输出三层判定，越靠后叠加的规则越多：
 *   1. `fromState`：只看该格方块与流体（原版 `getPathTypeFromState`）；
 *   2. `static`：叠加脚下方块与 3×3×3 危险邻域（原版 `getPathTypeStatic`）；
 *   3. `ofMob`：再叠加实体尺寸、门、铁轨、栅栏与窄实体规则（原版 `getPathTypeOfMob`）。
 *
 * 三层放在同一次输出里是刻意的：一处差异能立刻定位到「哪一层的规则偏了」，
 * 而不必先怀疑是上游哪一步把输入弄脏了。
 */

#include "common/entity/ai/pathfinding/WalkNodeEvaluator.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "path_scene.hpp"
#include "probe.hpp"
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

/// 把一行输入解析成完整场景。
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
    const f32 width = mobInput.at("width").get<f32>();
    const f32 height = mobInput.at("height").get<f32>();
    const f32 stepHeight = mobInput.at("stepHeight").get<f32>();
    const bool onGround = mobInput.at("onGround").get<bool>();
    const bool inWater = mobInput.at("inWater").get<bool>();
    const i32 maxFallDistance = mobInput.contains("maxFallDistance") ? mobInput.at("maxFallDistance").get<i32>() : -1;

    scene.mob = std::make_unique<SceneMob>(width, height, stepHeight, onGround, inWater, maxFallDistance);
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

std::string runLine(std::string_view line)
{
    initialize();
    const json input = json::parse(line);
    Scene scene = buildScene(input);

    WalkNodeEvaluator evaluator;
    evaluator.setCanPassDoors(scene.canPassDoors);
    evaluator.setCanOpenDoors(scene.canOpenDoors);
    evaluator.setCanFloat(scene.canFloat);
    evaluator.setCanWalkOverFences(scene.canWalkOverFences);
    evaluator.prepare(scene.region, *scene.mob, scene.mobPosition);
    const PathfindingContext& context = *evaluator.context();

    // 输出顺序必须与 oracle 一致：dy 外层、dz 中层、dx 内层。
    json fromState = json::array();
    json staticTypes = json::array();
    json ofMob = json::array();
    for (i32 dy = 0; dy < scene.sizeY; ++dy) {
        for (i32 dz = 0; dz < scene.sizeZ; ++dz) {
            for (i32 dx = 0; dx < scene.sizeX; ++dx) {
                const i32 x = scene.originX + dx;
                const i32 y = scene.originY + dy;
                const i32 z = scene.originZ + dz;
                fromState.push_back(getPathTypeName(WalkNodeEvaluator::getPathTypeFromState(scene.region, x, y, z)));
                staticTypes.push_back(getPathTypeName(WalkNodeEvaluator::getPathTypeStatic(scene.region, x, y, z)));
                ofMob.push_back(getPathTypeName(evaluator.getPathTypeOfMob(context, x, y, z)));
            }
        }
    }
    evaluator.done();

    const i32 entityWidth = static_cast<i32>(std::floor(scene.mob->pathfindingWidth() + 1.0f));
    const i32 entityHeight = static_cast<i32>(std::floor(scene.mob->pathfindingHeight() + 1.0f));
    return json{{"unit", input.at("unit")},
        {"fromState", std::move(fromState)},
        {"static", std::move(staticTypes)},
        {"ofMob", std::move(ofMob)},
        {"fallDistance", scene.mob->pathfindingMaxFallDistance()},
        {"entityWidth", entityWidth},
        {"entityHeight", entityHeight}}
        .dump();
}

void writeList(FILE* out)
{
    initialize();
    // 单元（场景名）由编排层给出，探针不自行枚举。
    MC_UNUSED(out);
}

} // namespace
MC_DIFF_DEFINE_PROBE("walk_path_type", &writeList, &runLine)
} // namespace mc::diff
