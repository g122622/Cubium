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
 * @brief 差分能力：`Rotation` / `Mirror` 的方向与坐标变换（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `com.mojang.math.OctahedralGroup` 与 `net.minecraft.world.level.block.Rotation` /
 * `Mirror`：
 *
 * ```java
 * // Rotation 的 OctahedralGroup 映射
 * NONE(0, "none")                       -> OctahedralGroup.IDENTITY
 * CLOCKWISE_90(1, "clockwise_90")       -> OctahedralGroup.ROT_90_Y_NEG
 * CLOCKWISE_180(2, "180")               -> OctahedralGroup.ROT_180_FACE_XZ
 * COUNTERCLOCKWISE_90(3, "counterclockwise_90") -> OctahedralGroup.ROT_90_Y_POS
 *
 * // Rotation.rotate(Direction)
 * if (dir.getAxis() == Axis.Y) { return dir; }          // Y 轴不变
 * switch (this) {
 *     case CLOCKWISE_90        -> dir.getClockWise();
 *     case CLOCKWISE_180       -> dir.getOpposite();
 *     case COUNTERCLOCKWISE_90 -> dir.getCounterClockWise();
 *     default                  -> dir;
 * }
 *
 * // Mirror.mirror(Direction)
 * if (this == FRONT_BACK && dir.getAxis() == Axis.X) { return dir.getOpposite(); }
 * return this == LEFT_RIGHT && dir.getAxis() == Axis.Z ? dir.getOpposite() : dir;
 *
 * // Mirror.getRotation(Direction) —— 依赖轴的「等效旋转」
 * if ((this != LEFT_RIGHT || axis != Z) && (this != FRONT_BACK || axis != X)) { return NONE; }
 * return CLOCKWISE_180;
 *
 * // Mirror 的 OctahedralGroup
 * NONE("none")             -> IDENTITY
 * LEFT_RIGHT("left_right") -> INVERT_Z
 * FRONT_BACK("front_back") -> INVERT_X
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`Rotation.rotate(Direction)` 对 `Y` 轴方向原样返回**，不论旋转角度。所以 `UP`/`DOWN`
 *    在任何水平旋转下都不变——旋转只发生在 XZ 平面上。
 *
 * 2. **`Mirror.mirror(Direction)` 只翻转「垂直于镜面」的那个轴**，且**该轴上的方向取反，其他
 *    轴原样返回**：
 *    - `FRONT_BACK` 翻转 `X` 轴
 *    - `LEFT_RIGHT` 翻转 `Z` 轴
 *    - `NONE` 不翻转任何轴
 *
 *    注意 `FRONT_BACK` 翻转的是 **X** 轴，`LEFT_RIGHT` 翻转的是 **Z** 轴——名字的直觉会反过来。
 *
 * 3. **`Mirror.getRotation(axis)` 返回的是「对该轴方向的等效旋转」**：只有当镜像会翻转该轴时
 *    才返回 `CLOCKWISE_180`，否则 `NONE`。这个函数用于把「镜像 + 旋转」归一成纯旋转。
 *
 * 4. **`OctahedralGroup` 的坐标旋转是矩阵作用**，不是简单的分量交换。`ROT_90_Y_NEG` 等名字
 *    描述的是**旋转轴与方向**，`NEG`/`POS` 指绕该轴的正负向。
 *
 * 单元格式：
 * - `dirrot|<rotation>|<direction>` —— `Rotation.rotate(Direction)`
 * - `dirmirror|<mirror>|<direction>` —— `Mirror.mirror(Direction)`
 * - `mirrorrot|<mirror>|<axis>` —— `Mirror.getRotation(axis)`
 * - `posrot|<rotation>|<x>|<y>|<z>` —— `OctahedralGroup` 作用于坐标
 * - `posmirror|<mirror>|<x>|<y>|<z>` —— 同上，用镜像的 group
 * - `compose|<groupA>|<groupB>` —— `a.compose(b)` 后的坐标变换结果（作用于测试点）
 * - `inverse|<group>` —— `g.inverse()` 作用于测试点
 * - `inverts|<group>|<axis>` —— `g.inverts(axis)`
 *
 * 方向名：`down/up/north/south/west/east`；轴名：`x/y/z`。
 *
 * TODO: 未覆盖 `Rotation.getRotated`（旋转的复合）、`FrontAndTop` 的旋转、`Rotation.NONE` 等
 * 枚举的序列化标识。
 */
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 3×3 整数矩阵，按行存储。
struct Mat3 {
    i32 m[3][3]{};
};

/// 单位矩阵。
Mat3 identity()
{
    Mat3 result;
    for (i32 i = 0; i < 3; ++i) {
        result.m[i][i] = 1;
    }
    return result;
}

/// 矩阵乘向量。
std::array<i32, 3> applyMat3(const Mat3& mat, i32 x, i32 y, i32 z)
{
    return {mat.m[0][0] * x + mat.m[0][1] * y + mat.m[0][2] * z,
        mat.m[1][0] * x + mat.m[1][1] * y + mat.m[1][2] * z,
        mat.m[2][0] * x + mat.m[2][1] * y + mat.m[2][2] * z};
}

/// 原版 `OctahedralGroup` 的矩阵（只列本能力用到的几个）。
Mat3 groupMatrix(const std::string& name)
{
    if (name == "IDENTITY") {
        return identity();
    }
    if (name == "ROT_90_Y_NEG") {
        // 绕 Y 轴顺时针 90°（从上方俯视）：X -> -Z、Z -> X。
        return Mat3{{{0, 0, -1}, {0, 1, 0}, {1, 0, 0}}};
    }
    if (name == "ROT_90_Y_POS") {
        return Mat3{{{0, 0, 1}, {0, 1, 0}, {-1, 0, 0}}};
    }
    if (name == "ROT_180_FACE_XZ") {
        return Mat3{{{-1, 0, 0}, {0, 1, 0}, {0, 0, -1}}};
    }
    if (name == "INVERT_X") {
        Mat3 m = identity();
        m.m[0][0] = -1;
        return m;
    }
    if (name == "INVERT_Z") {
        Mat3 m = identity();
        m.m[2][2] = -1;
        return m;
    }
    throw std::runtime_error("block_transform: 未知 group：" + name);
}

/// `Rotation` 名 → `OctahedralGroup` 名。
std::string rotationGroupName(const std::string& rotation)
{
    if (rotation == "NONE") {
        return "IDENTITY";
    }
    if (rotation == "CLOCKWISE_90") {
        return "ROT_90_Y_NEG";
    }
    if (rotation == "CLOCKWISE_180") {
        return "ROT_180_FACE_XZ";
    }
    if (rotation == "COUNTERCLOCKWISE_90") {
        return "ROT_90_Y_POS";
    }
    throw std::runtime_error("block_transform: 未知 Rotation：" + rotation);
}

/// `Mirror` 名 → `OctahedralGroup` 名。
std::string mirrorGroupName(const std::string& mirror)
{
    if (mirror == "NONE") {
        return "IDENTITY";
    }
    if (mirror == "LEFT_RIGHT") {
        return "INVERT_Z";
    }
    if (mirror == "FRONT_BACK") {
        return "INVERT_X";
    }
    throw std::runtime_error("block_transform: 未知 Mirror：" + mirror);
}

/// 方向名 → (dx, dy, dz)。
std::array<i32, 3> directionVector(const std::string& name)
{
    if (name == "down") {
        return {0, -1, 0};
    }
    if (name == "up") {
        return {0, 1, 0};
    }
    if (name == "north") {
        return {0, 0, -1};
    }
    if (name == "south") {
        return {0, 0, 1};
    }
    if (name == "west") {
        return {-1, 0, 0};
    }
    if (name == "east") {
        return {1, 0, 0};
    }
    throw std::runtime_error("block_transform: 未知方向：" + name);
}

/// (dx, dy, dz) → 方向名。
std::string directionName(i32 dx, i32 dy, i32 dz)
{
    if (dx == 0 && dy == -1 && dz == 0) {
        return "down";
    }
    if (dx == 0 && dy == 1 && dz == 0) {
        return "up";
    }
    if (dx == 0 && dy == 0 && dz == -1) {
        return "north";
    }
    if (dx == 0 && dy == 0 && dz == 1) {
        return "south";
    }
    if (dx == -1 && dy == 0 && dz == 0) {
        return "west";
    }
    if (dx == 1 && dy == 0 && dz == 0) {
        return "east";
    }
    throw std::runtime_error("block_transform: 无法识别的方向向量");
}

/// 轴名。
std::string axisOf(const std::string& direction)
{
    if (direction == "up" || direction == "down") {
        return "y";
    }
    if (direction == "north" || direction == "south") {
        return "z";
    }
    return "x";
}

/// 原版 `Rotation.rotate(Direction)`：Y 轴不变，其余按旋转。
std::string rotationRotate(const std::string& rotation, const std::string& direction)
{
    if (axisOf(direction) == "y") {
        return direction;
    }
    if (rotation == "NONE") {
        return direction;
    }
    const auto vec = directionVector(direction);
    const Mat3 mat = groupMatrix(rotationGroupName(rotation));
    const auto out = applyMat3(mat, vec[0], vec[1], vec[2]);
    return directionName(out[0], out[1], out[2]);
}

/// 原版 `Mirror.mirror(Direction)`。
std::string mirrorDirection(const std::string& mirror, const std::string& direction)
{
    const std::string axis = axisOf(direction);
    if (mirror == "FRONT_BACK" && axis == "x") {
        const auto vec = directionVector(direction);
        return directionName(-vec[0], vec[1], vec[2]);
    }
    if (mirror == "LEFT_RIGHT" && axis == "z") {
        const auto vec = directionVector(direction);
        return directionName(vec[0], vec[1], -vec[2]);
    }
    return direction;
}

/// 原版 `Mirror.getRotation(Direction)`。
std::string mirrorRotation(const std::string& mirror, const std::string& axis)
{
    const bool flips = (mirror == "LEFT_RIGHT" && axis == "z") || (mirror == "FRONT_BACK" && axis == "x");
    return flips ? "CLOCKWISE_180" : "NONE";
}

/// 两个 3×3 矩阵相乘。
Mat3 matMul(const Mat3& a, const Mat3& b)
{
    Mat3 result;
    for (i32 i = 0; i < 3; ++i) {
        for (i32 j = 0; j < 3; ++j) {
            i32 sum = 0;
            for (i32 k = 0; k < 3; ++k) {
                sum += a.m[i][k] * b.m[k][j];
            }
            result.m[i][j] = sum;
        }
    }
    return result;
}

/// 转置（对这些正交矩阵即逆矩阵）。
Mat3 transpose(const Mat3& a)
{
    Mat3 result;
    for (i32 i = 0; i < 3; ++i) {
        for (i32 j = 0; j < 3; ++j) {
            result.m[i][j] = a.m[j][i];
        }
    }
    return result;
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

/// 矩阵作用结果的字符串形式。
std::string applyToString(const Mat3& mat, i32 x, i32 y, i32 z)
{
    const auto out = applyMat3(mat, x, y, z);
    return std::to_string(out[0]) + "," + std::to_string(out[1]) + "," + std::to_string(out[2]);
}

/// 单元表。
constexpr const char* UNITS[] = {
    // Rotation 作用于方向：Y 轴不变
    "dirrot|CLOCKWISE_90|up",
    "dirrot|CLOCKWISE_90|down",
    "dirrot|CLOCKWISE_180|up",
    "dirrot|COUNTERCLOCKWISE_90|down",
    // Rotation 作用于水平方向
    "dirrot|NONE|north",
    "dirrot|CLOCKWISE_90|north",
    "dirrot|CLOCKWISE_90|east",
    "dirrot|CLOCKWISE_90|south",
    "dirrot|CLOCKWISE_90|west",
    "dirrot|CLOCKWISE_180|north",
    "dirrot|COUNTERCLOCKWISE_90|north",
    "dirrot|COUNTERCLOCKWISE_90|east",
    // Mirror 作用于方向
    "dirmirror|NONE|east",
    "dirmirror|NONE|north",
    "dirmirror|FRONT_BACK|east",
    "dirmirror|FRONT_BACK|west",
    "dirmirror|FRONT_BACK|north",
    "dirmirror|LEFT_RIGHT|north",
    "dirmirror|LEFT_RIGHT|south",
    "dirmirror|LEFT_RIGHT|east",
    "dirmirror|FRONT_BACK|up",
    "dirmirror|LEFT_RIGHT|down",
    // Mirror.getRotation(axis)
    "mirrorrot|NONE|x",
    "mirrorrot|LEFT_RIGHT|x",
    "mirrorrot|LEFT_RIGHT|z",
    "mirrorrot|FRONT_BACK|x",
    "mirrorrot|FRONT_BACK|z",
    "mirrorrot|LEFT_RIGHT|y",
    // 坐标旋转
    "posrot|NONE|1|2|3",
    "posrot|CLOCKWISE_90|1|2|3",
    "posrot|CLOCKWISE_180|1|2|3",
    "posrot|COUNTERCLOCKWISE_90|1|2|3",
    "posrot|CLOCKWISE_90|-1|0|0",
    "posrot|CLOCKWISE_90|0|0|1",
    // 坐标镜像
    "posmirror|NONE|1|2|3",
    "posmirror|LEFT_RIGHT|1|2|3",
    "posmirror|FRONT_BACK|1|2|3",
    // 组合与逆
    "compose|ROT_90_Y_NEG|ROT_90_Y_NEG",
    "compose|ROT_90_Y_NEG|IDENTITY",
    "compose|IDENTITY|ROT_90_Y_NEG",
    "compose|INVERT_X|INVERT_Z",
    "inverse|ROT_90_Y_NEG",
    "inverse|ROT_90_Y_POS",
    "inverse|ROT_180_FACE_XZ",
    "inverse|INVERT_X",
    "inverts|INVERT_X|x",
    "inverts|INVERT_X|y",
    "inverts|ROT_180_FACE_XZ|z",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("block_transform: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "dirrot" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", rotationRotate(parts[1], parts[2])}}.dump();
    }
    if (mode == "dirmirror" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", mirrorDirection(parts[1], parts[2])}}.dump();
    }
    if (mode == "mirrorrot" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", mirrorRotation(parts[1], parts[2])}}.dump();
    }
    if (mode == "posrot" && parts.size() == 5) {
        const Mat3 mat = groupMatrix(rotationGroupName(parts[1]));
        return json{{"unit", unit},
            {"value", applyToString(mat, std::stoi(parts[2]), std::stoi(parts[3]), std::stoi(parts[4]))}}
            .dump();
    }
    if (mode == "posmirror" && parts.size() == 5) {
        const Mat3 mat = groupMatrix(mirrorGroupName(parts[1]));
        return json{{"unit", unit},
            {"value", applyToString(mat, std::stoi(parts[2]), std::stoi(parts[3]), std::stoi(parts[4]))}}
            .dump();
    }
    if (mode == "compose" && parts.size() == 3) {
        const Mat3 product = matMul(groupMatrix(parts[1]), groupMatrix(parts[2]));
        return json{{"unit", unit}, {"value", applyToString(product, 1, 2, 3)}}.dump();
    }
    if (mode == "inverse" && parts.size() == 2) {
        const Mat3 inv = transpose(groupMatrix(parts[1]));
        return json{{"unit", unit}, {"value", applyToString(inv, 1, 2, 3)}}.dump();
    }
    if (mode == "inverts" && parts.size() == 3) {
        const Mat3 mat = groupMatrix(parts[1]);
        const i32 index = parts[2] == "x" ? 0 : (parts[2] == "y" ? 1 : 2);
        return json{{"unit", unit}, {"value", mat.m[index][index] == -1 ? "true" : "false"}}.dump();
    }

    throw std::runtime_error("block_transform: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_transform, &writeList, &runLine)

} // namespace mc::diff
