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
 * @brief 差分能力：`BoundingBox` 的相交、包含与合并判定（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.level.levelgen.structure.BoundingBox`：
 *
 * ```java
 * public BoundingBox(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 *     this.minX = minX; ... this.maxZ = maxZ;
 *     if (maxX < minX || maxY < minY || maxZ < minZ) {
 *         // 边界反向 -> 自动归一化
 *         this.minX = Math.min(minX, maxX); ...
 *         this.maxX = Math.max(minX, maxX); ...
 *     }
 * }
 *
 * public static BoundingBox fromCorners(Vec3i a, Vec3i b) {
 *     return new BoundingBox(min(a.x, b.x), ..., max(a.x, b.x), ...);
 * }
 *
 * public boolean intersects(BoundingBox other) {
 *     return this.maxX >= other.minX && this.minX <= other.maxX
 *         && this.maxZ >= other.minZ && this.minZ <= other.maxZ
 *         && this.maxY >= other.minY && this.minY <= other.maxY;
 * }
 *
 * public boolean isInside(int x, int y, int z) {
 *     return x >= this.minX && x <= this.maxX
 *         && z >= this.minZ && z <= this.maxZ
 *         && y >= this.minY && y <= this.maxY;
 * }
 *
 * public static BoundingBox encapsulating(BoundingBox a, BoundingBox b) {
 *     return new BoundingBox(min(a.minX, b.minX), ..., max(a.maxX, b.maxX), ...);
 * }
 *
 * public BoundingBox move(int dx, int dy, int dz) {
 *     this.minX += dx; ... this.maxZ += dz;
 *     return this;   // 原地修改！
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **所有判定都用闭区间（`>=` / `<=`）。** 边界相接的两个盒子**算相交**：
 *    `[0,5]` 与 `[5,10]` 在 X 上满足 `5 >= 5 && 0 <= 10`，相交为真。这与 AABB（浮点）的
 *    语义不同——那边是开区间，相接不算重叠。
 *
 * 2. **`intersects` 的六个条件之间是 `&&`，缺一不可。** 只要任一轴完全分离就不相交。注意
 *    早期版本曾有 `intersects(int, int, int, int)` 的四参数重载（只比较 X 与 Z，忽略 Y），
 *    用于平面判定——两套语义不能混用。
 *
 * 3. **构造器对反向边界自动归一化，不抛异常。** 传 `(5, 0, 0, 0, 0, 0)` 会得到 `[0,5]`。
 *    这与 [`aabb_ops`](./aabb_ops.md) 里浮点 `AABB` 的归一化是同一类行为。
 *
 * 4. **`move` 是原地修改并返回 `this`，不是返回新对象。** 若调用方以为它是纯函数，
 *    ```java
 *    BoundingBox moved = box.move(1, 0, 0);   // box 自身也被改了
 *    ```
 *    原对象会被一并修改。这是结构生成代码里常见的坑。
 *
 * 单元格式：
 * - `inter|<盒A>|<盒B>` —— 输出 `true`/`false`
 * - `inside|<盒>|<x>|<y>|<z>` —— 输出 `true`/`false`
 * - `encap|<盒A>|<盒B>` —— 输出合并后的盒子
 * - `move|<盒>|<dx>|<dy>|<dz>` —— 输出移动后的盒子
 * - `corners|<ax>|<ay>|<az>|<bx>|<by>|<bz>` —— `fromCorners` 的结果
 * - `ctor|<minX>|<minY>|<minZ>|<maxX>|<maxY>|<maxZ>` —— 构造后的盒子（验证归一化）
 *
 * 盒子格式：`minX,minY,minZ,maxX,maxY,maxZ`（6 个逗号分隔的整数）。
 *
 * TODO: 未覆盖 `orientBox`（结构旋转与镜像，依赖 `Rotation`/`Mirror`，见
 * [`block_transform`](./block_transform.md)）、`encapsulatingPositions`、`inflate`、
 * `isInside(Vec3i)` 的等价重载。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `BoundingBox`。
struct Box {
    i32 minX = 0;
    i32 minY = 0;
    i32 minZ = 0;
    i32 maxX = 0;
    i32 maxY = 0;
    i32 maxZ = 0;
};

/// 原版构造器：边界反向时自动归一化。
Box makeBox(i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
{
    Box box;
    if (maxX < minX || maxY < minY || maxZ < minZ) {
        box.minX = std::min(minX, maxX);
        box.minY = std::min(minY, maxY);
        box.minZ = std::min(minZ, maxZ);
        box.maxX = std::max(minX, maxX);
        box.maxY = std::max(minY, maxY);
        box.maxZ = std::max(minZ, maxZ);
    } else {
        box = Box{minX, minY, minZ, maxX, maxY, maxZ};
    }
    return box;
}

/// 原版 `fromCorners`。
Box fromCorners(i32 ax, i32 ay, i32 az, i32 bx, i32 by, i32 bz)
{
    return makeBox(
        std::min(ax, bx), std::min(ay, by), std::min(az, bz), std::max(ax, bx), std::max(ay, by), std::max(az, bz));
}

/// 原版 `intersects(BoundingBox)`：闭区间，六条件全 `&&`。
bool intersects(const Box& a, const Box& b)
{
    return a.maxX >= b.minX && a.minX <= b.maxX && a.maxZ >= b.minZ && a.minZ <= b.maxZ && a.maxY >= b.minY &&
        a.minY <= b.maxY;
}

/// 原版 `isInside(int, int, int)`：闭区间。
bool isInside(const Box& box, i32 x, i32 y, i32 z)
{
    return x >= box.minX && x <= box.maxX && z >= box.minZ && z <= box.maxZ && y >= box.minY && y <= box.maxY;
}

/// 原版 `encapsulating`。
Box encapsulating(const Box& a, const Box& b)
{
    return makeBox(std::min(a.minX, b.minX),
        std::min(a.minY, b.minY),
        std::min(a.minZ, b.minZ),
        std::max(a.maxX, b.maxX),
        std::max(a.maxY, b.maxY),
        std::max(a.maxZ, b.maxZ));
}

/// 原版 `move`：原地修改。
void move(Box& box, i32 dx, i32 dy, i32 dz)
{
    box.minX += dx;
    box.minY += dy;
    box.minZ += dz;
    box.maxX += dx;
    box.maxY += dy;
    box.maxZ += dz;
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

std::vector<std::string> splitComma(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

Box parseBox(const std::string& text)
{
    const auto parts = splitComma(text);
    if (parts.size() != 6) {
        throw std::runtime_error("bounding_box: 盒子应为 6 个逗号分隔的整数：" + text);
    }
    return makeBox(std::stoi(parts[0]),
        std::stoi(parts[1]),
        std::stoi(parts[2]),
        std::stoi(parts[3]),
        std::stoi(parts[4]),
        std::stoi(parts[5]));
}

std::string boxToString(const Box& box)
{
    return std::to_string(box.minX) + "," + std::to_string(box.minY) + "," + std::to_string(box.minZ) + "," +
        std::to_string(box.maxX) + "," + std::to_string(box.maxY) + "," + std::to_string(box.maxZ);
}

/// 单元表。
constexpr const char* UNITS[] = {
    // 相交：完全分离
    "inter|0,0,0,1,1,1|2,0,0,3,1,1",
    "inter|0,0,0,1,1,1|0,2,0,1,3,1",
    "inter|0,0,0,1,1,1|0,0,2,1,1,3",
    // 相交：边界相接（闭区间 -> 相交）
    "inter|0,0,0,1,1,1|1,0,0,2,1,1",
    "inter|0,0,0,1,1,1|0,1,0,1,2,1",
    "inter|0,0,0,1,1,1|0,0,1,1,1,2",
    // 相交：包含关系
    "inter|0,0,0,10,10,10|2,2,2,3,3,3",
    "inter|2,2,2,3,3,3|0,0,0,10,10,10",
    // 相交：完全相同
    "inter|0,0,0,5,5,5|0,0,0,5,5,5",
    // 相交：只在一个轴上重叠
    "inter|0,0,0,5,5,5|4,10,10,9,15,15",
    // isInside：内部 / 边界 / 外部
    "inside|0,0,0,5,5,5|2|2|2",
    "inside|0,0,0,5,5,5|0|0|0",
    "inside|0,0,0,5,5,5|5|5|5",
    "inside|0,0,0,5,5,5|6|2|2",
    "inside|0,0,0,5,5,5|2|-1|2",
    "inside|0,0,0,5,5,5|2|2|6",
    // encapsulating
    "encap|0,0,0,1,1,1|5,5,5,6,6,6",
    "encap|0,0,0,10,10,10|2,2,2,3,3,3",
    "encap|0,0,0,5,5,5|0,0,0,5,5,5",
    // move
    "move|0,0,0,5,5,5|1|2|3",
    "move|0,0,0,5,5,5|0|0|0",
    "move|0,0,0,5,5,5|-10|-10|-10",
    // fromCorners：顺序无关
    "corners|0|0|0|5|5|5",
    "corners|5|5|5|0|0|0",
    "corners|5|0|3|0|5|1",
    // 构造器归一化：单轴反向
    "ctor|5|0|0|0|0|0",
    "ctor|0|0|0|5|0|0",
    // 构造器归一化：三轴反向
    "ctor|5|5|5|0|0|0",
    // 构造器：正常输入不归一化
    "ctor|1|2|3|4|5|6",
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
    if (parts.size() < 3) {
        throw std::runtime_error("bounding_box: 单元至少三段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "inter" && parts.size() == 3) {
        const Box a = parseBox(parts[1]);
        const Box b = parseBox(parts[2]);
        return json{{"unit", unit}, {"value", intersects(a, b) ? "true" : "false"}}.dump();
    }
    if (mode == "inside" && parts.size() == 5) {
        const Box box = parseBox(parts[1]);
        const bool result = isInside(box, std::stoi(parts[2]), std::stoi(parts[3]), std::stoi(parts[4]));
        return json{{"unit", unit}, {"value", result ? "true" : "false"}}.dump();
    }
    if (mode == "encap" && parts.size() == 3) {
        const Box merged = encapsulating(parseBox(parts[1]), parseBox(parts[2]));
        return json{{"unit", unit}, {"value", boxToString(merged)}}.dump();
    }
    if (mode == "move" && parts.size() == 5) {
        Box box = parseBox(parts[1]);
        move(box, std::stoi(parts[2]), std::stoi(parts[3]), std::stoi(parts[4]));
        return json{{"unit", unit}, {"value", boxToString(box)}}.dump();
    }
    if (mode == "corners" && parts.size() == 7) {
        const Box box = fromCorners(std::stoi(parts[1]),
            std::stoi(parts[2]),
            std::stoi(parts[3]),
            std::stoi(parts[4]),
            std::stoi(parts[5]),
            std::stoi(parts[6]));
        return json{{"unit", unit}, {"value", boxToString(box)}}.dump();
    }
    if (mode == "ctor" && parts.size() == 7) {
        const Box box = makeBox(std::stoi(parts[1]),
            std::stoi(parts[2]),
            std::stoi(parts[3]),
            std::stoi(parts[4]),
            std::stoi(parts[5]),
            std::stoi(parts[6]));
        return json{{"unit", unit}, {"value", boxToString(box)}}.dump();
    }

    throw std::runtime_error("bounding_box: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("bounding_box", &writeList, &runLine)

} // namespace mc::diff
