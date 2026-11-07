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

#include "probe.hpp"

#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;

using Shapes = mc::Shapes;
using mc::VoxelShape;

/**
 * @file shape_collide.cpp
 * @brief 实体与方块形状的碰撞求解（原版 `AABB.collide*` 系列）。
 *
 * `Entity.move` 的核心不是「加速度怎么积分」，而是**沿位移方向把碰撞箱收敛到贴面位置**：
 * 对每个即将撞上的形状，按移动轴求出允许的最大位移，再把剩余位移吃掉。这套逻辑决定了
 * 玩家能不能卡进缝隙、能不能站上方块边缘、斜向撞墙时是否被「滑」过去。
 *
 * 原版把它拆成 `AABB.collideX/Y/Z` 三个入口，实现在 `Shapes.collide`：
 * 先按坐标轴算出穿透深度，再按 `box.min(axis) < shape.min(axis)` 决定贴着哪一面停住。
 * 关键细节是**收敛方向**：正向移动时取形状的 `min` 面，反向移动取 `max` 面，
 * 并且要和当前位移取「更近的那个」。写反的表现是实体穿模或卡在半空。
 *
 * 探针直接调用生产的 `Shapes::collide`，不在这里重写求解过程。
 */

/// 一个形状：用方块局部坐标的包围盒描述，位置由场景决定。
struct ShapeSpec {
    f64 minX;
    f64 minY;
    f64 minZ;
    f64 maxX;
    f64 maxY;
    f64 maxZ;
};

/// 一个场景：实体碰撞箱 + 若干形状 + 移动轴与位移。
struct Scene {
    const char* name;
    /// 实体碰撞箱（世界坐标）
    f64 boxMinX, boxMinY, boxMinZ, boxMaxX, boxMaxY, boxMaxZ;
    /// 形状列表（世界坐标）
    std::vector<ShapeSpec> shapes;
    /// 沿哪个轴移动
    Axis axis;
    /// 期望位移
    f64 movement;
};

const std::vector<Scene>& scenes()
{
    static const std::vector<Scene> table = {
        // 玩家宽 0.6、高 1.8，站在地面上向左撞一整块石头
        {"wall_full_block_x_pos",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{1.0, 0.0, 0.0, 2.0, 1.0, 1.0}},
            Axis::X,
            0.5},
        // 同样一次移动，但只走一半（不足以撞上）
        {"wall_full_block_short",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{1.0, 0.0, 0.0, 2.0, 1.0, 1.0}},
            Axis::X,
            0.2},
        // 反向移动撞墙
        {"wall_full_block_x_neg",
            0.3,
            0.0,
            -0.3,
            0.9,
            1.8,
            0.3,
            {{-2.0, 0.0, 0.0, -1.0, 1.0, 1.0}},
            Axis::X,
            -0.5},
        // 斜向同时被两块夹住：只沿 X 求解时，两块都要参与
        {"pinched_two_blocks",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{-2.0, 0.0, 0.0, -1.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 2.0, 1.0, 1.0}},
            Axis::X,
            0.5},
        // 台阶：形状只占下半格，站在上面的实体水平移动不应被挡
        {"slab_below_entity",
            -0.3,
            0.5,
            -0.3,
            0.3,
            2.3,
            0.3,
            {{1.0, 0.0, 0.0, 2.0, 0.5, 1.0}},
            Axis::X,
            0.5},
        // 台阶正好卡在实体脚下高度：形状上表面 0.5 与实体底面 0.5 齐平，不应产生水平阻挡
        {"slab_flush_below",
            -0.3,
            0.5,
            -0.3,
            0.3,
            2.3,
            0.3,
            {{1.0, 0.0, 0.0, 2.0, 0.5, 1.0}},
            Axis::X,
            0.5},
        // 栅栏：形状细而高，实体从侧面撞上去
        {"fence_column",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{1.375, 0.0, 0.375, 1.625, 1.5, 0.625}},
            Axis::X,
            0.5},
        // 台阶状形状与实体箱只在 Y 上重叠一部分
        {"partial_y_overlap",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{1.0, 1.5, 0.0, 2.0, 2.5, 1.0}},
            Axis::X,
            0.5},
        // 竖直下落：撞地面
        {"fall_onto_ground",
            -0.3,
            2.0,
            -0.3,
            0.3,
            3.8,
            0.3,
            {{-1.0, 0.0, -1.0, 1.0, 1.0, 1.0}},
            Axis::Y,
            -1.5},
        // 上跳撞到天花板
        {"jump_into_ceiling",
            -0.3,
            1.0,
            -0.3,
            0.3,
            2.8,
            0.3,
            {{-1.0, 3.0, -1.0, 1.0, 4.0, 1.0}},
            Axis::Y,
            0.5},
        // 沿 Z 移动撞墙
        {"wall_full_block_z_pos",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{0.0, 0.0, 1.0, 1.0, 1.0, 2.0}},
            Axis::Z,
            0.5},
        // 空形状列表：位移应原样通过
        {"no_shapes",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {},
            Axis::X,
            0.5},
        // 位移为 0：无论形状如何都应为 0
        {"zero_movement",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{-2.0, 0.0, -2.0, 2.0, 2.0, 2.0}},
            Axis::X,
            0.0},
        // 实体的另一侧残留极小间隙：贴着但不重叠
        {"touching_no_overlap",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{0.3, 0.0, -1.0, 1.3, 1.0, 1.0}},
            Axis::X,
            0.5},
        // 已经重叠：求解应返回 0，不把实体推得更深
        {"already_overlapping",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{0.0, 0.0, -1.0, 1.0, 1.0, 1.0}},
            Axis::X,
            0.5},
        // 两个形状首尾相接：取更近的那个
        {"two_adjacent_shapes",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{1.0, 0.0, 0.0, 2.0, 1.0, 1.0}, {2.0, 0.0, 0.0, 3.0, 1.0, 1.0}},
            Axis::X,
            3.0},
        // 小位移撞上薄形状
        {"thin_shape_small_move",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{0.5, 0.0, 0.0, 0.5625, 1.0, 1.0}},
            Axis::X,
            0.1},
        // 非整齐小数位移
        {"fractional_move",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{1.0, 0.0, 0.0, 2.0, 1.0, 1.0}},
            Axis::X,
            0.3737},
        // 大位移跨越多个方块
        {"large_move",
            -0.3,
            0.0,
            -0.3,
            0.3,
            1.8,
            0.3,
            {{5.0, 0.0, 0.0, 6.0, 1.0, 1.0}},
            Axis::X,
            10.0},
    };
    return table;
}

void writeList(std::FILE* out)
{
    for (const auto& scene : scenes()) {
        std::fprintf(out, "%s\n", scene.name);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const Scene* scene = nullptr;
    for (const auto& candidate : scenes()) {
        if (requested == candidate.name) {
            scene = &candidate;
            break;
        }
    }
    if (scene == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    const AxisAlignedBB entityBox(scene->boxMinX, scene->boxMinY, scene->boxMinZ, scene->boxMaxX, scene->boxMaxY,
        scene->boxMaxZ);

    std::vector<VoxelShape> shapes;
    shapes.reserve(scene->shapes.size());
    for (const auto& spec : scene->shapes) {
        shapes.push_back(Shapes::box(spec.minX, spec.minY, spec.minZ, spec.maxX, spec.maxY, spec.maxZ));
    }

    const f64 resolved = Shapes::collide(scene->axis, entityBox, shapes, scene->movement);
    return json{{"unit", requested}, {"collided", encodeFloat(resolved)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(shape_collide, &writeList, &runLine)

} // namespace mc::diff