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
 * @brief 差分能力：`BlockPos` 的位打包与坐标运算（纯计算）。
 *
 * `list`：一组「模式|x,y,z…」单元
 * `run` ：同上一行 → { "long": "...", "x": 1, "y": 2, "z": 3 } 等
 *
 * 复刻原版 `BlockPos`：
 *
 * ```java
 * public static final int PACKED_HORIZONTAL_LENGTH =
 *     1 + Mth.log2(Mth.smallestEncompassingPowerOfTwo(30000000));
 * public static final int PACKED_Y_LENGTH = 64 - 2 * PACKED_HORIZONTAL_LENGTH;
 * // => PACKED_HORIZONTAL_LENGTH = 26, PACKED_Y_LENGTH = 12
 *
 * public static long asLong(int x, int y, int z) {
 *     long i = 0L;
 *     i |= (x & PACKED_X_MASK) << X_OFFSET;   // X_OFFSET = 12 + 26 = 38
 *     i |= (y & PACKED_Y_MASK) << 0;          // Y_OFFSET = 0
 *     return i | (z & PACKED_Z_MASK) << Z_OFFSET;   // Z_OFFSET = 12
 * }
 *
 * public static int getX(long p) {
 *     return (int)(p << 64 - X_OFFSET - PACKED_HORIZONTAL_LENGTH >> 64 - PACKED_HORIZONTAL_LENGTH);
 * }
 * public static int getY(long p) { return (int)(p << 64 - PACKED_Y_LENGTH >> 64 - PACKED_Y_LENGTH); }
 * public static int getZ(long p) {
 *     return (int)(p << 64 - Z_OFFSET - PACKED_HORIZONTAL_LENGTH >> 64 - PACKED_HORIZONTAL_LENGTH);
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **位段宽度是 `X/Z: 26`、`Y: 12`**，偏移是 `X << 38`、`Y << 0`、`Z << 12`。写成
 *    `X/Y/Z` 各占 21 位是错的。
 *
 * 2. **解包用「左移再算术右移」还原符号**：`p << (64 - width) >> (64 - width)`。C++ 中必须
 *    用**有符号**右移（`i64`），用 `u64` 无符号右移会让负数坐标变成巨大的正数。
 *
 * 3. **`asLong` 先掩码再移位**，所以超范围坐标会**静默截断**（`& PACKED_X_MASK`），不是
 *    报错也不是饱和。例如 `x = 30000000 + 1` 会被掩码成 `-(MAX+1)` 对应的值。
 *
 * ## 坐标范围
 *
 * `MAX_HORIZONTAL_COORDINATE = (1 << 26) / 2 - 1 = 33554431`。超出该范围的坐标在打包时
 * 被掩码截断，解包后得到完全不同的坐标——这是原版行为，必须复刻。
 *
 * 单元格式：
 * - `pack|<x>|<y>|<z>` —— 打包成 long，再解包回 x/y/z（一并输出，验证往返）
 * - `offset|<x>|<y>|<z>|<dx>|<dy>|<dz>` —— 坐标偏移
 *
 * TODO: 未覆盖 `betweenClosed`（包围盒迭代顺序）、`distManhattan` / `distSqr` /
 * `distChessboard`（简单距离公式）、`containing`（依赖 `Mth.floor`，已有 `mth_rounding`
 * 覆盖）。
 */
#include <cstdint>
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

/// 原版 `BlockPos.PACKED_HORIZONTAL_LENGTH`。
constexpr i32 PACKED_HORIZONTAL_LENGTH = 26;

/// 原版 `BlockPos.PACKED_Y_LENGTH`。
constexpr i32 PACKED_Y_LENGTH = 64 - 2 * PACKED_HORIZONTAL_LENGTH;

constexpr i64 PACKED_X_MASK = (1LL << PACKED_HORIZONTAL_LENGTH) - 1LL;
constexpr i64 PACKED_Y_MASK = (1LL << PACKED_Y_LENGTH) - 1LL;
constexpr i64 PACKED_Z_MASK = (1LL << PACKED_HORIZONTAL_LENGTH) - 1LL;

constexpr i32 Y_OFFSET = 0;
constexpr i32 Z_OFFSET = PACKED_Y_LENGTH;
constexpr i32 X_OFFSET = PACKED_Y_LENGTH + PACKED_HORIZONTAL_LENGTH;

/// 原版 `BlockPos.MAX_HORIZONTAL_COORDINATE`。
constexpr i32 MAX_HORIZONTAL_COORDINATE = (1 << PACKED_HORIZONTAL_LENGTH) / 2 - 1;

/**
 * @brief 原版 `BlockPos.asLong(int, int, int)`。
 *
 * 注意先掩码再移位：超范围坐标会被静默截断。
 */
i64 packBlockPos(i32 x, i32 y, i32 z)
{
    i64 result = 0LL;
    result |= (static_cast<i64>(x) & PACKED_X_MASK) << X_OFFSET;
    result |= (static_cast<i64>(y) & PACKED_Y_MASK) << Y_OFFSET;
    result |= (static_cast<i64>(z) & PACKED_Z_MASK) << Z_OFFSET;
    return result;
}

/**
 * @brief 原版 `BlockPos.getX(long)`：左移后**算术**右移以还原符号。
 */
i32 unpackX(i64 packed)
{
    return static_cast<i32>((packed << (64 - X_OFFSET - PACKED_HORIZONTAL_LENGTH)) >> (64 - PACKED_HORIZONTAL_LENGTH));
}

/// 原版 `BlockPos.getY(long)`。
i32 unpackY(i64 packed)
{
    return static_cast<i32>((packed << (64 - PACKED_Y_LENGTH)) >> (64 - PACKED_Y_LENGTH));
}

/// 原版 `BlockPos.getZ(long)`。
i32 unpackZ(i64 packed)
{
    return static_cast<i32>((packed << (64 - Z_OFFSET - PACKED_HORIZONTAL_LENGTH)) >> (64 - PACKED_HORIZONTAL_LENGTH));
}

/// 单元表。
constexpr const char* UNITS[] = {
    // 原点
    "pack|0|0|0",
    // 正坐标
    "pack|1|2|3",
    "pack|100|64|200",
    // 负坐标：验证符号还原
    "pack|-1|-1|-1",
    "pack|-100|-64|-200",
    // 各轴单独为负，确认不影响其它轴
    "pack|-1|0|0",
    "pack|0|-1|0",
    "pack|0|0|-1",
    // Y 轴极值（12 位有符号：-2048..2047）
    "pack|0|2047|0",
    "pack|0|-2048|0",
    "pack|0|2048|0",
    "pack|0|-2049|0",
    // X/Z 轴极值（26 位有符号）
    "pack|33554431|0|0",
    "pack|-33554432|0|0",
    "pack|33554432|0|0",
    "pack|-33554433|0|0",
    // 三轴同时负
    "pack|-12345|-100|-54321",
    // offset
    "offset|1|2|3|1|1|1",
    "offset|1|2|3|-1|-2|-3",
    "offset|0|0|0|0|0|0",
    "offset|-5|-5|-5|5|5|5",
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
    if (parts.empty()) {
        throw std::runtime_error("blockpos_codec: 空单元");
    }

    const std::string mode = parts[0];

    if (mode == "pack" && parts.size() == 4) {
        const i32 x = std::stoi(parts[1]);
        const i32 y = std::stoi(parts[2]);
        const i32 z = std::stoi(parts[3]);
        const i64 packed = packBlockPos(x, y, z);
        // 用字符串输出 long：JSON number 在 JS 侧会退化为 double，丢失低位精度。
        return json{{"unit", unit},
            {"packed", std::to_string(packed)},
            {"x", unpackX(packed)},
            {"y", unpackY(packed)},
            {"z", unpackZ(packed)}}
            .dump();
    }

    if (mode == "offset" && parts.size() == 7) {
        const i32 x = std::stoi(parts[1]);
        const i32 y = std::stoi(parts[2]);
        const i32 z = std::stoi(parts[3]);
        const i32 dx = std::stoi(parts[4]);
        const i32 dy = std::stoi(parts[5]);
        const i32 dz = std::stoi(parts[6]);
        // 原版 `offset` 在坐标完全不变时返回 this，否则 new BlockPos(...)。
        return json{{"unit", unit}, {"x", x + dx}, {"y", y + dy}, {"z", z + dz}}.dump();
    }

    throw std::runtime_error("blockpos_codec: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(blockpos_codec, &writeList, &runLine)

} // namespace mc::diff
