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
 * @brief 差分能力：`BoundingBox` 的封装族与构造函数（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "minX,minY,minZ,maxX,maxY,maxZ" | "<empty>" }
 *
 * 复刻原版 `net.minecraft.world.level.levelgen.structure.BoundingBox`：
 *
 * ```java
 * public BoundingBox(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 *     this.minX = minX; ... this.maxZ = maxZ;
 *     if (maxX < minX || maxY < minY || maxZ < minZ) {
 *         Util.logAndPauseIfInIde("Invalid bounding box data, inverted bounds for: " + this);
 *         this.minX = Math.min(minX, maxX); ...   // 归一化
 *     }
 * }
 *
 * public BoundingBox(BlockPos pos) {
 *     this(pos.getX(), pos.getY(), pos.getZ(), pos.getX(), pos.getY(), pos.getZ());
 * }
 *
 * public static BoundingBox fromCorners(Vec3i a, Vec3i b) {
 *     return new BoundingBox(
 *         Math.min(a.getX(), b.getX()), Math.min(a.getY(), b.getY()), Math.min(a.getZ(), b.getZ()),
 *         Math.max(a.getX(), b.getX()), Math.max(a.getY(), b.getY()), Math.max(a.getZ(), b.getZ()));
 * }
 *
 * public static BoundingBox infinite() {
 *     return new BoundingBox(Integer.MIN_VALUE, ..., Integer.MAX_VALUE, ...);
 * }
 *
 * public static BoundingBox encapsulating(BoundingBox a, BoundingBox b) {
 *     return new BoundingBox(
 *         Math.min(a.minX, b.minX), ..., Math.max(a.maxZ, b.maxZ));
 * }
 *
 * public static Optional<BoundingBox> encapsulatingPositions(Iterable<BlockPos> positions) {
 *     Iterator<BlockPos> it = positions.iterator();
 *     if (!it.hasNext()) return Optional.empty();
 *     BoundingBox box = new BoundingBox(it.next());
 *     it.forEachRemaining(box::encapsulate);
 *     return Optional.of(box);
 * }
 *
 * public BoundingBox encapsulate(BlockPos pos) {
 *     this.minX = Math.min(this.minX, pos.getX()); ...
 *     this.maxX = Math.max(this.maxX, pos.getX()); ...
 *     return this;
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **构造器**在 min/max 反了时**会归一化**，与 `AABB` 的「无条件归一化」不同。**
 *
 *    ```java
 *    if (maxX < minX || maxY < minY || maxZ < minZ) {
 *        // 只有确认为「反的」才交换
 *    }
 *    ```
 *
 *    `AABB` 是无条件 `Math.min` / `Math.max`，两者在合法输入上结果相同，但**日志副作用不同**：
 *    `BoundingBox` 会调用 `Util.logAndPauseIfInIde`。本能力不输出日志，只比对数值。
 *
 * 2. **`fromCorners` 与构造器归一化是两套独立机制。**
 *
 *    `fromCorners` 自己先做 `Math.min` / `Math.max`，所以传给构造器的永远是**已归一化**的值，
 *    构造器的 `if` 永不触发。
 *
 * 3. **`encapsulatingPositions` 空集合返回 `Optional.empty()`，不是全零盒。**
 *
 *    ```java
 *    if (!iterator.hasNext()) return Optional.empty();
 *    ```
 *
 *    **零个位置与「一个位于原点的位置」结果完全不同**。本能力用 `"<empty>"` 表示空集合。
 *
 * 4. **`encapsulatingPositions` 的起始盒是「第一个位置折叠成的点盒」，不是 `infinite()`。**
 *
 *    `new BoundingBox(it.next())` 得到 `(p, p, p, p, p, p)`，然后用 `encapsulate` 逐个扩展。
 *
 *    若从 `infinite()` 起步，结果会被 `Integer.MIN_VALUE` / `MAX_VALUE` 撑满——**这是常见错误**。
 *
 * 单元格式：
 * - `ctor|<minX>|<minY>|<minZ>|<maxX>|<maxY>|<maxZ>` —— 构造器
 * - `fromCorners|<ax>|<ay>|<az>|<bx>|<by>|<bz>` —— `fromCorners`
 * - `infinite` —— `infinite()`（无参数）
 * - `encapBox|<a6>|<b6>` —— 两盒封装
 * - `encapPos|<x>,<y>,<z>;<x>,<y>,<z>;...` —— 位置集合封装；空集合写作 `-`
 *
 * 整数按 `i32` 处理，`Integer.MIN_VALUE` / `MAX_VALUE` 用字面量 `minint` / `maxint` 表示。
 */
#include <algorithm>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

constexpr i32 INT_MIN_VALUE = std::numeric_limits<i32>::min();
constexpr i32 INT_MAX_VALUE = std::numeric_limits<i32>::max();

/// 一个整数包围盒（闭区间）。
struct Box {
    i32 minX = 0;
    i32 minY = 0;
    i32 minZ = 0;
    i32 maxX = 0;
    i32 maxY = 0;
    i32 maxZ = 0;
};

/// 原版 `BoundingBox(int, int, int, int, int, int)`：反了则归一化。
Box makeBox(i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
{
    Box box{minX, minY, minZ, maxX, maxY, maxZ};
    if (maxX < minX || maxY < minY || maxZ < minZ) {
        box.minX = std::min(minX, maxX);
        box.minY = std::min(minY, maxY);
        box.minZ = std::min(minZ, maxZ);
        box.maxX = std::max(minX, maxX);
        box.maxY = std::max(minY, maxY);
        box.maxZ = std::max(minZ, maxZ);
    }
    return box;
}

/// 原版 `BoundingBox.encapsulate(BlockPos)`：原地扩展。
void encapsulate(Box& box, i32 x, i32 y, i32 z)
{
    box.minX = std::min(box.minX, x);
    box.minY = std::min(box.minY, y);
    box.minZ = std::min(box.minZ, z);
    box.maxX = std::max(box.maxX, x);
    box.maxY = std::max(box.maxY, y);
    box.maxZ = std::max(box.maxZ, z);
}

i32 parseInt(const std::string& text)
{
    if (text == "minint") {
        return INT_MIN_VALUE;
    }
    if (text == "maxint") {
        return INT_MAX_VALUE;
    }
    return static_cast<i32>(std::stoll(text));
}

std::string render(const Box& box)
{
    return std::to_string(box.minX) + "," + std::to_string(box.minY) + "," + std::to_string(box.minZ) + "," +
        std::to_string(box.maxX) + "," + std::to_string(box.maxY) + "," + std::to_string(box.maxZ);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // ctor：正常与反向
        "ctor|0|0|0|1|1|1",
        "ctor|5|5|5|0|0|0",
        "ctor|0|0|0|0|0|0",
        "ctor|-5|-5|-5|5|5|5",
        // ctor：只有部分轴反向
        "ctor|0|0|0|1|1|0",
        "ctor|1|0|0|0|1|1",
        // fromCorners
        "fromCorners|0|0|0|3|2|4",
        "fromCorners|3|2|4|0|0|0",
        "fromCorners|-1|-2|-3|1|2|3",
        // infinite
        "infinite",
        // encapBox
        "encapBox|0|0|0|1|1|1|2|2|2|3|3|3",
        "encapBox|0|0|0|1|1|1|0|0|0|1|1|1",
        "encapBox|-5|-5|-5|0|0|0|3|3|3|8|8|8",
        // encapPos
        "encapPos|0,0,0",
        "encapPos|0,0,0;3,2,4",
        "encapPos|3,2,4;0,0,0",
        "encapPos|-1,-2,-3;1,2,3;0,0,0",
        "encapPos|-",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
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

/// 拆分 `x,y,z;x,y,z` 形式的位置列表。
std::vector<Box> parsePositions(const std::string& text)
{
    std::vector<Box> points;
    if (text == "-") {
        return points;
    }
    std::string current;
    std::vector<std::string> parts;
    for (char ch : text) {
        if (ch == ';') {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(current);

    for (const auto& part : parts) {
        std::vector<std::string> numbers;
        std::string token;
        for (char ch : part) {
            if (ch == ',') {
                numbers.push_back(token);
                token.clear();
            } else {
                token.push_back(ch);
            }
        }
        numbers.push_back(token);
        if (numbers.size() != 3) {
            throw std::runtime_error("bounding_box_encap: 位置需要三坐标：" + part);
        }
        const i32 x = static_cast<i32>(std::stoll(numbers[0]));
        const i32 y = static_cast<i32>(std::stoll(numbers[1]));
        const i32 z = static_cast<i32>(std::stoll(numbers[2]));
        points.push_back(Box{x, y, z, x, y, z});
    }
    return points;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);

    const std::string mode = parts[0];

    if (mode == "ctor" && parts.size() == 7) {
        const Box box = makeBox(parseInt(parts[1]),
            parseInt(parts[2]),
            parseInt(parts[3]),
            parseInt(parts[4]),
            parseInt(parts[5]),
            parseInt(parts[6]));
        return json{{"unit", unit}, {"value", render(box)}}.dump();
    }
    if (mode == "fromCorners" && parts.size() == 7) {
        const i32 ax = parseInt(parts[1]);
        const i32 ay = parseInt(parts[2]);
        const i32 az = parseInt(parts[3]);
        const i32 bx = parseInt(parts[4]);
        const i32 by = parseInt(parts[5]);
        const i32 bz = parseInt(parts[6]);
        const Box box = makeBox(
            std::min(ax, bx), std::min(ay, by), std::min(az, bz), std::max(ax, bx), std::max(ay, by), std::max(az, bz));
        return json{{"unit", unit}, {"value", render(box)}}.dump();
    }
    if (mode == "infinite" && parts.size() == 1) {
        const Box box{INT_MIN_VALUE, INT_MIN_VALUE, INT_MIN_VALUE, INT_MAX_VALUE, INT_MAX_VALUE, INT_MAX_VALUE};
        return json{{"unit", unit}, {"value", render(box)}}.dump();
    }
    if (mode == "encapBox" && parts.size() == 13) {
        const Box a{parseInt(parts[1]),
            parseInt(parts[2]),
            parseInt(parts[3]),
            parseInt(parts[4]),
            parseInt(parts[5]),
            parseInt(parts[6])};
        const Box b{parseInt(parts[7]),
            parseInt(parts[8]),
            parseInt(parts[9]),
            parseInt(parts[10]),
            parseInt(parts[11]),
            parseInt(parts[12])};
        const Box box = makeBox(std::min(a.minX, b.minX),
            std::min(a.minY, b.minY),
            std::min(a.minZ, b.minZ),
            std::max(a.maxX, b.maxX),
            std::max(a.maxY, b.maxY),
            std::max(a.maxZ, b.maxZ));
        return json{{"unit", unit}, {"value", render(box)}}.dump();
    }
    if (mode == "encapPos" && parts.size() == 2) {
        const auto points = parsePositions(parts[1]);
        if (points.empty()) {
            return json{{"unit", unit}, {"value", "<empty>"}}.dump();
        }
        Box box = points.front();
        for (std::size_t i = 1; i < points.size(); ++i) {
            encapsulate(box, points[i].minX, points[i].minY, points[i].minZ);
        }
        return json{{"unit", unit}, {"value", render(box)}}.dump();
    }

    throw std::runtime_error("bounding_box_encap: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("bounding_box_encap", &writeList, &runLine)

} // namespace mc::diff
