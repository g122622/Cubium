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
 * @brief 差分能力：随机刻的坐标采样（`Level.getBlockRandomPos`，纯计算）。
 *
 * `list`：一组「参数…」单元
 * `run` ：同上一行 → { "positions": "x,y,z;x,y,z;..." }
 *
 * 复刻原版 `Level`：
 *
 * ```java
 * protected int randValue = RandomSource.create().nextInt();
 *
 * public BlockPos getBlockRandomPos(int x, int y, int z, int maxY) {
 *     this.randValue = this.randValue * 3 + 1013904223;
 *     int i = this.randValue >> 2;
 *     return new BlockPos(x + (i & 15), y + (i >> 16 & maxY), z + (i >> 8 & 15));
 * }
 * ```
 *
 * 以及 `ServerLevel.tickChunk` 的调用方式：
 *
 * ```java
 * for (int k = 0; k < randomTickSpeed; k++) {
 *     BlockPos pos = getBlockRandomPos(chunkMinX, sectionMinY, chunkMinZ, 15);
 *     ...
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`getBlockRandomPos` 不用 `Random` 对象，用独立的 `randValue` 整数状态。**
 *
 *    ```java
 *     this.randValue = this.randValue * 3 + 1013904223;
 *    ```
 *
 *    这是一个 LCG（`x <- 3x + 1013904223`），与世界的 `RandomSource` **完全隔离**。所以随机刻
 *    采样**不会消耗**方块随机刻逻辑用的随机数流，反之亦然。若把它实现成从 `Random` 取数，
 *    两者会互相干扰，导致所有随机行为错位。
 *
 * 2. **`int` 乘法会溢出环绕。** `randValue * 3` 在 32 位下必定溢出多次，Java 的 `int` 溢出是
 *    定义良好的二补数环绕。用 `int64` 计算再截断也能得到相同结果，但**不能**用「不溢出的
 *    自然数算术」——那会立刻发散。
 *
 * 3. **`>> 2` 是算术右移**，保留符号位。`randValue` 为负时高位补 1，得到的 `i` 也是负数，
 *    后续 `i & 15` 才把它转成 `0..15` 的非负值。若错用逻辑右移（`>>>`）会得到完全不同的坐标。
 *
 * 4. **三个坐标分量的取位不同步**：`X` 取 `i & 15`（低位 4 位），`Z` 取 `i >> 8 & 15`
 *    （第 8~11 位），`Y` 取 `i >> 16 & maxY`（第 16 位起，掩码由 `maxY` 决定）。**X 与 Z
 *    的位数间距是 8 而不是 4**，所以采样点在水平面上不是「相邻 4 位」的规律。
 *
 * 单元格式：`<x>|<y>|<z>|<maxY>|<初始 randValue>|<次数>`
 *
 * 输出连续 `次数` 次采样得到的坐标。
 *
 * TODO: 未覆盖 `tickPrecipitation` 的 `random.nextInt(48) == 0` 判定、`LevelChunkSection`
 * 的 `isRandomlyTicking` 门控、`randomTickSpeed` 的配置来源。
 */
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

/**
 * @brief 复刻 `getBlockRandomPos` 的单次采样。
 *
 * @param randValue 输入/输出：LCG 状态
 */
void sampleOnce(i32& randValue, i32 x, i32 y, i32 z, i32 maxY, i32& outX, i32& outY, i32& outZ)
{
    // `randValue * 3 + 1013904223` 在 32 位下溢出环绕。
    randValue = static_cast<i32>(static_cast<u32>(randValue) * 3u + 1013904223u);
    // 算术右移，保留符号位。
    const i32 i = randValue >> 2;
    outX = x + (i & 15);
    outY = y + ((i >> 16) & maxY);
    outZ = z + ((i >> 8) & 15);
}

/// 单元表。
constexpr const char* UNITS[] = {
    // 单次采样：验证三个分量的取位
    "0|0|0|15|0|1",
    "0|0|0|15|1|1",
    "0|0|0|15|-1|1",
    "0|0|0|15|12345|1",
    // 连续采样：验证 LCG 推进
    "0|0|0|15|0|2",
    "0|0|0|15|0|3",
    "0|0|0|15|0|5",
    "0|0|0|15|0|8",
    // 起点偏移：坐标是加法
    "100|64|200|15|0|3",
    "-100|-64|-200|15|0|3",
    // maxY 掩码：15 与 7
    "0|0|0|15|777|3",
    "0|0|0|7|777|3",
    "0|0|0|3|777|3",
    // 大初始值（接近 int 边界）
    "0|0|0|15|2147483647|3",
    "0|0|0|15|-2147483648|3",
    // 经典种子
    "0|0|0|15|42|4",
};

void writeList(FILE* out)
{
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

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 6) {
        throw std::runtime_error("random_tick_pos: 单元应为 6 段：<x>|<y>|<z>|<maxY>|<seed>|<count>：" + unit);
    }

    const i32 x = std::stoi(parts[0]);
    const i32 y = std::stoi(parts[1]);
    const i32 z = std::stoi(parts[2]);
    const i32 maxY = std::stoi(parts[3]);
    i32 randValue = static_cast<i32>(std::stoll(parts[4]));
    const i32 count = std::stoi(parts[5]);

    std::string positions;
    for (i32 k = 0; k < count; ++k) {
        i32 outX = 0;
        i32 outY = 0;
        i32 outZ = 0;
        sampleOnce(randValue, x, y, z, maxY, outX, outY, outZ);
        if (!positions.empty()) {
            positions += ";";
        }
        positions += std::to_string(outX) + "," + std::to_string(outY) + "," + std::to_string(outZ);
    }

    return json{{"unit", unit}, {"positions", positions}, {"randValueAfter", std::to_string(randValue)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("random_tick_pos", &writeList, &runLine)

} // namespace mc::diff
