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
 * @brief 差分能力：`VoxelShape` 的面切片（`getFaceShape`）。
 *
 * `list`：一组精心挑选的**形状描述**，覆盖面切片的各种边界情况
 * `run` ：同一形状 → 6 个方向各自的切片结果
 *
 * ## 为什么需要这一层
 *
 * 支撑判定（`face_sturdy`）最终归结为「取某个面的切片，看它覆盖了什么」。
 * 但方块层面的差分只能看到**最终结论**（true/false），一旦不对，无法区分是
 * 方块形状写错了、还是 `getFaceShape` 这套机制本身有偏差。
 *
 * 本能力把中间层单独拎出来：形状是**显式给定的**，与任何方块注册无关。
 * 于是任何差异都只可能来自 `slice` / `findIndex` / `isCubeLikeAlong` 这三个环节。
 *
 * ## 单元格式
 *
 * 归一化坐标，用 `;` 分隔多个盒子（依次 OR），每个盒子 6 个数：
 *
 * ```
 * 0,0,0,1,1,1                     单一满方块
 * 0.0625,0,0.0625,0.9375,0.09375,0.9375   贴地的扁平形状（lily_pad 的形状）
 * 0,0,0,1,0.5,1;0,0.5,0.25,1,1,0.75       两个盒子 OR
 * ```
 *
 * 覆盖的边界情况：
 * - 满方块（三轴都 cube-like）
 * - 贴地扁平（Y 只有 1 格、起点为 0）
 * - 悬空扁平（Y 只有 1 格、起点非 0）
 * - 居中细柱（X/Z 居中，Y 满高）
 * - Y 超出方块顶部（跨方块的支撑形状）
 * - 多盒 OR（网格细分）
 */

#include "common/core/Types.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/util/Direction.hpp"
#include "probe.hpp"
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::diff {

namespace {

using nlohmann::json;

/// 单元表：覆盖面切片的各种边界情况。
const char* const UNITS[] = {
    // 满方块：三轴都 cube-like，getFaceShape 应直接返回自身
    "0,0,0,1,1,1",
    // 贴地扁平：Y 只有 1 格且起点为 0（lily_pad 的形状）
    "0.0625,0,0.0625,0.9375,0.09375,0.9375",
    // 悬空扁平：Y 只有 1 格但起点非 0（灯笼顶部小柱的形状）
    "0.375,0.4375,0.375,0.625,0.5625,0.625",
    // 居中细柱，Y 满高（栅栏门支撑形状的形态）
    "0,0.3125,0.375,1,1.5,0.625",
    // 居中细柱，Y 只到 0.5（几乎所有「小摆件」方块的形态）
    "0.25,0,0.25,0.75,0.5,0.75",
    // 居中细柱，Y 满高且不触底（悬挂类形状）
    "0.375,0.25,0.375,0.625,1,0.625",
    // 宽扁平台（蛋糕、海龟蛋的形态）
    "0.0625,0,0.0625,0.9375,0.5,0.9375",
    // 两盒 OR：横板 + 中心立柱（墙面悬挂告示牌的形态）
    "0,0.875,0.375,1,1,0.625;0.0625,0,0.4375,0.9375,0.625,0.5625",
    // 两盒 OR：上小下大（台阶的形态）
    "0,0,0,1,0.5,1;0,0.5,0,0.5,1,1",
    // 极薄片（藤蔓、地毯的形态）
    "0,0,0,1,0.0625,1",
    // X 方向细条（朝向 EAST/WEST 的告示牌形态）
    "0.4375,0,0,0.5625,1,1",
    // 不覆盖中心的环形（中心挖空的形态）
    "0,0,0,1,1,0.25;0,0,0.75,1,1,1",
    // 只占一个角（形状偏移的形态）
    "0.15625,0,0.15625,0.34375,1,0.34375",
    // Y 超出方块顶部（跨方块支撑形状）
    "0,1,0.375,1,1.5,0.625",
    // X 只有 1 格且起点为 0
    "0,0.25,0.25,0.0625,0.75,0.75",
};

/// 6 个方向，顺序固定。
constexpr Direction DIRECTIONS[] = {
    Direction::Down,
    Direction::Up,
    Direction::North,
    Direction::South,
    Direction::West,
    Direction::East,
};

constexpr const char* DIRECTION_NAMES[] = {"down", "up", "north", "south", "west", "east"};

/**
 * @brief 把形状描述解析成 `VoxelShape`。
 *
 * 描述格式：`<x1>,<y1>,<z1>,<x2>,<y2>,<z2>`，多个盒子用 `;` 分隔（依次 OR）。
 */
VoxelShape parseShape(const std::string& spec)
{
    std::vector<VoxelShape> boxes;
    std::stringstream parts(spec);
    std::string part;
    while (std::getline(parts, part, ';')) {
        if (part.empty()) {
            continue;
        }
        std::stringstream nums(part);
        std::string value;
        std::vector<f64> coords;
        while (std::getline(nums, value, ',')) {
            coords.push_back(std::strtod(value.c_str(), nullptr));
        }
        if (coords.size() != 6) {
            std::fprintf(stderr, "shape_face: 每个盒子应为 6 个坐标，实际 %zu：%s\n", coords.size(), part.c_str());
            std::exit(1);
        }
        boxes.push_back(
            Shapes::create(coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]));
    }

    if (boxes.empty()) {
        return Shapes::empty();
    }
    if (boxes.size() == 1) {
        return boxes.front();
    }
    return Shapes::or_(boxes.front(), std::vector<VoxelShape>(boxes.begin() + 1, boxes.end()));
}

/// 把形状转成稳定的字符串：空形状记作 `EMPTY`，否则输出各轴的坐标列表。
std::string describe(const VoxelShape& shape)
{
    if (shape.isEmpty()) {
        return "EMPTY";
    }
    std::string out;
    char buffer[64];
    for (Axis axis : {Axis::X, Axis::Y, Axis::Z}) {
        const auto& coords = shape.getCoords(axis);
        out.push_back(axis == Axis::X ? 'X' : (axis == Axis::Y ? 'Y' : 'Z'));
        out.push_back('[');
        for (std::size_t i = 0; i < coords.size(); ++i) {
            if (i != 0) {
                out.push_back(',');
            }
            std::snprintf(buffer, sizeof(buffer), "%.6f", coords[i]);
            out += buffer;
        }
        out.push_back(']');
    }
    return out;
}

void writeList(FILE* out)
{
    for (const char* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    const std::string spec = json::parse(inputLine).at("unit").get<std::string>();
    const VoxelShape shape = parseShape(spec);

    // 用扁平字符串而不是嵌套 JSON：两侧的键序不保证一致，扁平化后才能逐字节比对。
    std::string result;
    for (std::size_t i = 0; i < 6; ++i) {
        if (i != 0) {
            result.push_back('|');
        }
        result += DIRECTION_NAMES[i];
        result.push_back('=');
        result += describe(shape.getFaceShape(DIRECTIONS[i]));
    }

    return json{{"unit", spec}, {"result", result}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("shape_face", &writeList, &runLine)

} // namespace mc::diff