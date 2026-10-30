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
 * @file structure_density.cpp
 * @brief 差分能力：结构地形适配（Beardifier）的密度采样器。
 *
 * 输入一行一个场景：片段列表（包围盒 + 地形适配模式 + 地面高度偏移）、连接点列表、
 * 可选的影响范围包围盒与采样坐标。输出每个采样点处的密度偏移。
 *
 * 单元由共同语料决定（`harnesses/structure_density.ts`），因此本探针不提供 `list`：
 * 「原版有哪些场景」与「本项目有哪些场景」是同一份东西，不需要各自枚举一次。
 *
 * 这一层没有随机数、没有世界状态，输出只取决于输入；同一个场景必须永远得到同一个数。
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "probe.hpp"
#include "server/world/gen/density/Beardifier.hpp"
#include "server/world/gen/jigsaw/JigsawJunction.hpp"
#include "server/world/gen/structure/Structure.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace world::gen::density;

/**
 * @brief BEARD_KERNEL 的高斯分量：exp(-(dx² + (dy+0.5)² + dz²) / 16)
 *
 * 生产代码把这张表预先算好存成 f32；这里按定义式用 f64 现算，用来定位差异究竟来自
 * 采样公式还是来自预计算表的舍入。
 */
f64 gaussian(i32 dx, i32 dy, i32 dz)
{
    const f64 adjustedY = static_cast<f64>(dy) + 0.5;
    const f64 distSq = static_cast<f64>(dx) * dx + adjustedY * adjustedY + static_cast<f64>(dz) * dz;
    return std::exp(-distSq / 16.0);
}

/** 地形适配模式名 → 枚举；未知名字直接失败（语料与实现漂移时必须暴露）。 */
TerrainAdaptation parseAdaptation(const std::string& name)
{
    if (name == "NONE") {
        return TerrainAdaptation::None;
    }
    if (name == "BURY") {
        return TerrainAdaptation::Bury;
    }
    if (name == "BEARD_THIN") {
        return TerrainAdaptation::BeardThin;
    }
    if (name == "BEARD_BOX") {
        return TerrainAdaptation::BeardBox;
    }
    if (name == "ENCAPSULATE") {
        return TerrainAdaptation::Encapsulate;
    }
    throw std::runtime_error("structure_density: unknown terrain adaptation " + name);
}

/** 数组形式的包围盒 `[minX, minY, minZ, maxX, maxY, maxZ]`。 */
world::gen::structure::StructureBoundingBox parseBox(const json& value)
{
    return world::gen::structure::StructureBoundingBox(value.at(0).get<i32>(),
        value.at(1).get<i32>(),
        value.at(2).get<i32>(),
        value.at(3).get<i32>(),
        value.at(4).get<i32>(),
        value.at(5).get<i32>());
}

/** 一个场景解析后的形状；只在诊断路径里复用来算两趟。 */
struct Scene {
    std::vector<Beardifier::Rigid> pieces;
    std::vector<world::gen::jigsaw::JigsawJunction> junctions;
    std::optional<world::gen::structure::StructureBoundingBox> affected;
    std::vector<std::array<i32, 3>> points;
};

Scene parseScene(const json& input)
{
    Scene scene;
    for (const auto& entry : input.at("pieces")) {
        scene.pieces.push_back(Beardifier::Rigid{
            parseBox(entry.at(0)), parseAdaptation(entry.at(1).get<std::string>()), entry.at(2).get<i32>()});
    }
    for (const auto& entry : input.at("junctions")) {
        scene.junctions.emplace_back(entry.at(0).get<i32>(),
            entry.at(1).get<i32>(),
            entry.at(2).get<i32>(),
            0,
            world::gen::jigsaw::JigsawPlacementBehaviour::Rigid);
    }
    // affected 为 null 时构造的是「没有影响范围包围盒」的形态，此时任何查询点都返回 0。
    // 区块构建路径的包围盒由片段与连接点推导；这里让语料直接给出包围盒，是为了覆盖
    // 「包围盒与片段不重合」的边界情形，两种形态共用同一套采样逻辑。
    if (!input.at("affected").is_null()) {
        scene.affected = parseBox(input.at("affected"));
    }
    for (const auto& point : input.at("points")) {
        scene.points.push_back({point.at(0).get<i32>(), point.at(1).get<i32>(), point.at(2).get<i32>()});
    }
    return scene;
}

/**
 * @brief 用 f64 现算的高斯核复现一遍采样公式（诊断用）
 *
 * 生产实现把高斯核预先算成 f32 表存起来；这一趟用定义式在 f64 下现算，其余步骤与
 * `Beardifier::compute` 逐行对应。拿它与 `samples` 对照，即可判断两处差异究竟来自公式
 * 还是只来自核表的舍入。
 */
f64 sampleWithExactKernel(const Scene& scene, i32 blockX, i32 blockY, i32 blockZ)
{
    if (!scene.affected.has_value()) {
        return 0.0;
    }
    const auto& box = *scene.affected;
    if (blockX < box.minX() || blockX > box.maxX() || blockY < box.minY() || blockY > box.maxY() ||
        blockZ < box.minZ() || blockZ > box.maxZ()) {
        return 0.0;
    }

    const auto beard = [](i32 dx, i32 dy, i32 dz, i32 baseDy) -> f64 {
        const i32 i = dx + 12;
        const i32 j = dy + 12;
        const i32 k = dz + 12;
        if (i < 0 || i >= 24 || j < 0 || j >= 24 || k < 0 || k >= 24) {
            return 0.0;
        }
        const f64 adjustedY = static_cast<f64>(baseDy) + 0.5;
        const f64 distSq = static_cast<f64>(dx) * dx + adjustedY * adjustedY + static_cast<f64>(dz) * dz;
        const f64 gaussianValue = gaussian(dx, dy, dz);
        const f64 beardValue = -adjustedY * math::fastInvSqrt(distSq / 2.0) / 2.0;
        return beardValue * gaussianValue;
    };
    const auto bury = [](f64 dx, f64 dy, f64 dz) -> f64 {
        const f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (distance >= 6.0) {
            return 0.0;
        }
        if (distance <= 0.0) {
            return 1.0;
        }
        return (6.0 - distance) / 6.0;
    };

    f64 total = 0.0;
    for (const auto& rigid : scene.pieces) {
        const auto& rigidBox = rigid.box;
        const i32 dx = std::max(0, std::max(rigidBox.minX() - blockX, blockX - rigidBox.maxX()));
        const i32 dz = std::max(0, std::max(rigidBox.minZ() - blockZ, blockZ - rigidBox.maxZ()));
        const i32 baseY = rigidBox.minY() + rigid.groundLevelDelta;
        const i32 dy = blockY - baseY;
        switch (rigid.terrainAdaptation) {
            case TerrainAdaptation::None:
                break;
            case TerrainAdaptation::Bury:
                total += bury(static_cast<f64>(dx), static_cast<f64>(dy) / 2.0, static_cast<f64>(dz));
                break;
            case TerrainAdaptation::BeardThin:
            case TerrainAdaptation::BeardBox: {
                const i32 vertical = (rigid.terrainAdaptation == TerrainAdaptation::BeardBox)
                    ? std::max(0, std::max(baseY - blockY, blockY - rigidBox.maxY()))
                    : dy;
                total += beard(dx, vertical, dz, dy) * 0.8;
                break;
            }
            case TerrainAdaptation::Encapsulate: {
                const i32 vertical = std::max(0, std::max(rigidBox.minY() - blockY, blockY - rigidBox.maxY()));
                total += bury(static_cast<f64>(dx) / 2.0, static_cast<f64>(vertical) / 2.0, static_cast<f64>(dz) / 2.0) * 0.8;
                break;
            }
        }
    }
    for (const auto& junction : scene.junctions) {
        const i32 jx = blockX - junction.getSourceX();
        const i32 jy = blockY - junction.getSourceGroundY();
        const i32 jz = blockZ - junction.getSourceZ();
        total += beard(jx, jy, jz, jy) * 0.4;
    }
    return total;
}

std::string runLine(std::string_view inputLine)
{
    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const Scene scene = parseScene(input);

    const Beardifier sampler = Beardifier::fromRigidPieces(scene.pieces, scene.junctions, scene.affected);

    json samples = json::array();
    json exactSamples = json::array();
    for (const auto& point : scene.points) {
        samples.push_back(sampler.compute(point[0], point[1], point[2]));
        exactSamples.push_back(sampleWithExactKernel(scene, point[0], point[1], point[2]));
    }

    return json{{"unit", input.value("unit", "")},
        {"samples", std::move(samples)},
        // 诊断：用 f64 现算的核替换预计算表的结果，不参与比对（oracle 不声明该字段）。
        {"exactKernelSamples", std::move(exactSamples)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("structure_density", nullptr, &runLine)

} // namespace mc::diff