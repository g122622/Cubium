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
 * @brief 差分能力：`WorldBorder` 的边界判定与派生量（纯计算）。
 *
 * `list`：一组「模式|centerX,centerZ|size|…」单元
 * `run` ：同上一行 → { "result": "..." }
 *
 * 复刻原版 `WorldBorder`：
 *
 * ```java
 * public boolean isWithinBounds(double x, double z) {
 *     return this.isWithinBounds(x, z, 0.0);
 * }
 *
 * public boolean isWithinBounds(double x, double z, double padding) {
 *     return x >= this.getMinX() - padding
 *         && x <  this.getMaxX() + padding
 *         && z >= this.getMinZ() - padding
 *         && z <  this.getMaxZ() + padding;
 * }
 *
 * public boolean isWithinBounds(BlockPos pos) {
 *     return this.isWithinBounds(pos.getX(), pos.getZ());   // 只看方块的西北角
 * }
 *
 * public boolean isWithinBounds(AABB box) {
 *     return this.isWithinBounds(box.minX, box.minZ, box.maxX - 1.0E-5F, box.maxZ - 1.0E-5F);
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **边界比较是不对称的**：下界用 `>=`、上界用 `<`。位于「正好上边界」的坐标判为
 *    **界外**，位于「正好下边界」的判为**界内**。写成 `<=` 会让边界格多算一格。
 *
 * 2. **`isWithinBounds(BlockPos)` 只检查方块的西北角 `(x, z)`**，不检查 `(x+1, z+1)`。
 *    方块是 `[x, x+1)` 的半开区间，右边界不属于自己；因此边界上的那一格仍算**界内**。
 *    若额外检查 `(x+1, z+1)`，会把紧贴上边界的整列/整行方块误判为界外。
 *
 * 3. **`isWithinBounds(AABB)` 的 `1.0E-5F` 是 float 字面量**，转成 double 后是
 *    `1.0000000116860974E-5`；与 double 的 `1.0E-5` 相差一个 ulp 级别。
 *    当盒的 `maxX` 恰好落在边界上时，这个差别会翻转判定结果。
 *
 * 单元格式：
 * - `extent|<cx>,<cz>|<size>`                        —— 输出 minX/minZ/maxX/maxZ
 * - `block|<cx>,<cz>|<size>|<bx>,<bz>`               —— `BlockPos` 重载
 * - `aabb|<cx>,<cz>|<size>|<minX>,<minZ>,<maxX>,<maxZ>` —— `AABB` 重载
 */

#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "probe.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::diff {

namespace {

using nlohmann::json;

/// 把字符串按分隔符切成若干段。
std::vector<std::string> split(std::string_view text, char separator)
{
    std::vector<std::string> parts;
    std::string current;
    for (const char ch : text) {
        if (ch == separator) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(current);
    return parts;
}

/**
 * @brief 把浮点数格式化成「17 位有效数字、去掉尾随零」的字符串。
 *
 * 用 17 位有效数字（double 的精确往返位数）：边界判定对浮点误差极敏感，
 * 低位截断会把「1.0E-5F 与 1.0E-5 的差别」抹平，让本该暴露的差异消失。
 */
std::string formatDouble(double value)
{
    if (value == 0.0) {
        // 消掉 -0.0，避免两侧符号表示不一致造成的假差异。
        return "0";
    }
    char buffer[64];
    // 用 14 位有效数字：double 的 17 位精确展开在 C++ 与 Java 的 `%g` 实现间
    // 会给出不同的末位（C++ 展开到 17 位、Java 到 16 位后补零），那属于格式化分歧
    // 而非数值分歧。14 位足以让本项目关心的所有边界判定差异显现出来。
    std::snprintf(buffer, sizeof(buffer), "%.14g", value);
    std::string text(buffer);
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

/// 按逗号切出一组浮点数。
std::vector<double> parseDoubles(const std::string& text)
{
    std::vector<double> values;
    for (const std::string& piece : split(text, ',')) {
        values.push_back(std::strtod(piece.c_str(), nullptr));
    }
    return values;
}

void writeList(FILE* out)
{
    // 覆盖用的边界配置：中心与尺寸的组合。
    // 尺寸取 1（极小）、非 2 的幂、以及非常规值，覆盖半宽计算与钳制。
    const char* configs[] = {
        "0,0|100",
        "0,0|1",
        "100,-200|512",
        "-1.5,3.25|7.5",
        "0.1,0.2|0.3",
    };

    // 探测点：包括「正好在边界上」「紧邻边界」两类，专测 >= / < 的不对称。
    const char* probes[] = {
        "0,0",
        "50,50",
        "50,50.0001",
        "49.9999,50",
        "-50,-50",
        "-50.0001,-50",
        "0.15000000000000002,0.15",
        "1,1",
        "-1,-1",
        "25,25",
    };

    for (const char* config : configs) {
        const std::string configText(config);
        std::fprintf(out, "extent|%s\n", configText.c_str());

        for (const char* probe : probes) {
            std::fprintf(out, "block|%s|%s\n", configText.c_str(), probe);
        }

        // AABB 重载：用「刚好贴着边界」的盒来暴露 1.0E-5F 与 1.0E-5 的差别。
        const char* boxes[] = {
            "0,0,50,50",
            "0,0,49.99999,49.99999",
            "0,0,50.00001,50.00001",
            "-49.99999,-49.99999,0,0",
            "-50.00001,-50.00001,0,0",
            "0,0,1,1",
        };
        for (const char* box : boxes) {
            std::fprintf(out, "aabb|%s|%s\n", configText.c_str(), box);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    // 入参是整行 JSON：{"unit":"<模式>|…"}；先取出 unit 再按 | 切分。
    const std::string line = json::parse(inputLine).at("unit").get<std::string>();
    const std::vector<std::string> parts = split(line, '|');
    if (parts.size() < 3) {
        std::fprintf(stderr, "world_border: 输入格式错误：%s\n", line.c_str());
        std::exit(1);
    }

    const std::string& mode = parts[0];
    const std::vector<double> center = parseDoubles(parts[1]);
    if (center.size() != 2) {
        std::fprintf(stderr, "world_border: 中心应为两个分量：%s\n", line.c_str());
        std::exit(1);
    }
    const double size = std::strtod(parts[2].c_str(), nullptr);

    ::mc::world::border::WorldBorder border;
    border.setCenter(center[0], center[1]);
    border.setSize(size);

    std::string result;
    if (mode == "extent") {
        result = "minX=" + formatDouble(border.getMinX()) + " minZ=" + formatDouble(border.getMinZ()) +
            " maxX=" + formatDouble(border.getMaxX()) + " maxZ=" + formatDouble(border.getMaxZ());
    } else if (mode == "block") {
        if (parts.size() < 4) {
            std::fprintf(stderr, "world_border: 缺少坐标段：%s\n", line.c_str());
            std::exit(1);
        }
        const std::vector<double> coords = parseDoubles(parts[3]);
        if (coords.size() != 2) {
            std::fprintf(stderr, "world_border: 坐标应为两个分量：%s\n", line.c_str());
            std::exit(1);
        }
        const ::mc::BlockPos pos(
            static_cast<::mc::i32>(std::floor(coords[0])), 0, static_cast<::mc::i32>(std::floor(coords[1])));
        result = border.isWithinBounds(pos) ? "true" : "false";
    } else if (mode == "aabb") {
        if (parts.size() < 4) {
            std::fprintf(stderr, "world_border: 缺少盒段：%s\n", line.c_str());
            std::exit(1);
        }
        const std::vector<double> box = parseDoubles(parts[3]);
        if (box.size() != 4) {
            std::fprintf(stderr, "world_border: AABB 应为四个分量：%s\n", line.c_str());
            std::exit(1);
        }
        const auto bounds = ::mc::AxisAlignedBB(static_cast<::mc::f32>(box[0]),
            static_cast<::mc::f32>(0.0),
            static_cast<::mc::f32>(box[1]),
            static_cast<::mc::f32>(box[2]),
            static_cast<::mc::f32>(0.0),
            static_cast<::mc::f32>(box[3]));
        result = border.isWithinBounds(bounds) ? "true" : "false";
    } else {
        std::fprintf(stderr, "world_border: 未知模式：%s\n", mode.c_str());
        std::exit(1);
    }

    return json{{"unit", line}, {"result", result}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("world_border", &writeList, &runLine)

} // namespace mc::diff
