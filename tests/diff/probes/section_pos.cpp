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
 * @brief 差分能力：`SectionPos` 的区块坐标换算与 `long` 编码（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.core.SectionPos`：
 *
 * ```java
 * public static final int SECTION_SIZE = 16;
 * public static final int SECTION_MASK = 15;
 *
 * public static int blockToSectionCoord(int blockCoord) { return blockCoord >> 4; }
 * public static int blockToSectionCoord(double blockCoord) { return Mth.floor(blockCoord) >> 4; }
 * public static int sectionToBlockCoord(int sectionCoord) { return sectionCoord << 4; }
 * public static int sectionToBlockCoord(int sectionCoord, int offset) {
 *     return sectionToBlockCoord(sectionCoord) + offset;
 * }
 *
 * public static long asLong(int x, int y, int z) {
 *     long i = 0L;
 *     i |= (x & 4194303L) << 42;
 *     i |= (y & 1048575L) << 0;
 *     return i | (z & 4194303L) << 20;
 * }
 *
 * public static int x(long packed) { return (int)(packed << 0 >> 42); }
 * public static int y(long packed) { return (int)(packed << 44 >> 44); }
 * public static int z(long packed) { return (int)(packed << 22 >> 42); }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`blockCoord >> 4` 是算术右移，负数向下取整。**
 *
 *    | 方块坐标 | 区块坐标 |
 *    |---|---|
 *    | `15` | `0` |
 *    | `16` | `1` |
 *    | `0` | `0` |
 *    | `-1` | **`-1`** |
 *    | `-16` | **`-1`** |
 *    | `-17` | **`-2`** |
 *
 *    注意 `-1 >> 4 == -1` 而不是 `0`——`-1` 属于区块 `-1`（它覆盖方块 `-16..-1`）。若错用
 *    整除（`/ 16`）或逻辑右移，负坐标会整体错位一个区块。**这是世界生成与区块定位的核心
 *    语义。**
 *
 * 2. **`sectionToBlockCoord` 用 `<< 4`，是精确的乘 16。** 与 `>> 4` 构成互逆映射：
 *    `sectionToBlockCoord(blockToSectionCoord(x)) <= x`，且差值在 `0..15` 内。
 *
 * 3. **`asLong` 的三个字段位宽不同，且都做符号扩展。**
 *
 *    | 字段 | 位宽 | 掩码 | 位置 | 取值范围 |
 *    |---|---|---|---|---|
 *    | X | 22 位 | `4194303` (`0x3FFFFF`) | 第 42 位起 | `-2097152..2097151` |
 *    | Z | 22 位 | `4194303` | 第 20 位起 | `-2097152..2097151` |
 *    | Y | 20 位 | `1048575` (`0xFFFFF`) | 第 0 位起 | `-524288..524287` |
 *
 *    **Y 只有 20 位，X 与 Z 各 22 位。** Y 的范围刻意更小，因为世界高度远小于水平范围。
 *
 * 4. **反解用 `<< n >> m` 保留符号，不是直接掩码。**
 *
 *    ```java
 *    x(packed) = (int)(packed << 0  >> 42)   // 左移 0，右移 42
 *    z(packed) = (int)(packed << 22 >> 42)   // 先把 X 挤出去，再右移
 *    y(packed) = (int)(packed << 44 >> 44)   // 左移 44 把高位挤掉，再右移回来
 *    ```
 *
 *    若写成 `(int)((packed >>> offset) & mask)`，负数的符号位会丢失，得到的是「无符号解码」。
 *    `<<` 后再 `>>` 的做法让符号位贯穿整个移位过程。
 *
 * 单元格式：
 * - `blockToSection|<int>` —— `blockToSectionCoord(int)`
 * - `blockToSectionD|<double>` —— `blockToSectionCoord(double)`
 * - `sectionToBlock|<int>` —— `sectionToBlockCoord(int)`
 * - `sectionToBlockOff|<section>|<offset>` —— 带偏移的重载
 * - `asLong|<x>|<y>|<z>` —— 输出长整型（字符串，避免 JS 精度问题）
 * - `decode|<packed>` —— 输出 `x,y,z`
 * - `roundtrip|<x>|<y>|<z>` —— 编码后反解，验证可逆
 *
 * TODO: 未覆盖 `SectionPos.of(ChunkPos, int)`、`aroundChunk`、`cube`、`blocksInside` 等
 * 需要 `Stream`/`ChunkPos` 的方法，以及 `MIN_SECTION`/`MAX_SECTION` 常量本身。
 */
#include <cmath>
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

constexpr i32 SECTION_MASK = 15;
constexpr i32 SECTION_SIZE = 16;
constexpr i64 XZ_MASK = 4194303L; // 0x3FFFFF，22 位
constexpr i64 Y_MASK = 1048575L;  // 0xFFFFF，20 位

/// 原版 `SectionPos.blockToSectionCoord(int)`：算术右移。
i32 blockToSectionCoord(i32 blockCoord)
{
    return blockCoord >> 4;
}

/// 原版 `SectionPos.blockToSectionCoord(double)`：先 floor 再右移。
i32 blockToSectionCoordDouble(f64 blockCoord)
{
    return static_cast<i32>(static_cast<i64>(std::floor(blockCoord)) >> 4);
}

/// 原版 `SectionPos.sectionToBlockCoord(int)`。
i32 sectionToBlockCoord(i32 sectionCoord)
{
    return sectionCoord << 4;
}

/// 原版 `SectionPos.sectionToBlockCoord(int, int)`。
i32 sectionToBlockCoordOffset(i32 sectionCoord, i32 offset)
{
    return sectionToBlockCoord(sectionCoord) + offset;
}

/// 原版 `SectionPos.asLong(int, int, int)`。
i64 asLong(i32 x, i32 y, i32 z)
{
    i64 packed = 0;
    packed |= (static_cast<i64>(x) & XZ_MASK) << 42;
    packed |= (static_cast<i64>(y) & Y_MASK) << 0;
    packed |= (static_cast<i64>(z) & XZ_MASK) << 20;
    return packed;
}

/// 原版 `SectionPos.x(long)`：`(int)(packed << 0 >> 42)`。
i32 decodeX(i64 packed)
{
    return static_cast<i32>(packed << 0 >> 42);
}

/// 原版 `SectionPos.y(long)`：`(int)(packed << 44 >> 44)`。
i32 decodeY(i64 packed)
{
    return static_cast<i32>(packed << 44 >> 44);
}

/// 原版 `SectionPos.z(long)`：`(int)(packed << 22 >> 42)`。
i32 decodeZ(i64 packed)
{
    return static_cast<i32>(packed << 22 >> 42);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // blockToSection：正数
        "blockToSection|0",
        "blockToSection|15",
        "blockToSection|16",
        "blockToSection|31",
        "blockToSection|32",
        // blockToSection：负数（向下取整）
        "blockToSection|-1",
        "blockToSection|-15",
        "blockToSection|-16",
        "blockToSection|-17",
        "blockToSection|-32",
        "blockToSection|-33",
        // blockToSection：极值
        "blockToSection|2147483647",
        "blockToSection|-2147483648",
        // blockToSection(double)
        "blockToSectionD|0.0",
        "blockToSectionD|15.9",
        "blockToSectionD|-0.1",
        "blockToSectionD|-1.0",
        "blockToSectionD|-16.5",
        "blockToSectionD|16.0",
        // sectionToBlock
        "sectionToBlock|0",
        "sectionToBlock|1",
        "sectionToBlock|-1",
        "sectionToBlock|-2",
        "sectionToBlock|100",
        // 带偏移
        "sectionToBlockOff|0|0",
        "sectionToBlockOff|0|15",
        "sectionToBlockOff|-1|8",
        "sectionToBlockOff|2|100",
        // asLong
        "asLong|0|0|0",
        "asLong|1|2|3",
        "asLong|-1|-1|-1",
        "asLong|-5|10|-20",
        "asLong|100|200|-300",
        // decode
        "decode|0",
        "decode|4398046511104",
        // roundtrip
        "roundtrip|1|2|3",
        "roundtrip|-1|-1|-1",
        "roundtrip|-5|10|-20",
        "roundtrip|2097151|524287|2097151",
        "roundtrip|-2097152|-524288|-2097152",
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

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("section_pos: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "blockToSection" && parts.size() == 2) {
        const i32 value = static_cast<i32>(std::stoll(parts[1]));
        return json{{"unit", unit}, {"value", blockToSectionCoord(value)}}.dump();
    }
    if (mode == "blockToSectionD" && parts.size() == 2) {
        const f64 value = std::stod(parts[1]);
        return json{{"unit", unit}, {"value", blockToSectionCoordDouble(value)}}.dump();
    }
    if (mode == "sectionToBlock" && parts.size() == 2) {
        const i32 value = static_cast<i32>(std::stoll(parts[1]));
        return json{{"unit", unit}, {"value", sectionToBlockCoord(value)}}.dump();
    }
    if (mode == "sectionToBlockOff" && parts.size() == 3) {
        const i32 section = static_cast<i32>(std::stoll(parts[1]));
        const i32 offset = std::stoi(parts[2]);
        return json{{"unit", unit}, {"value", sectionToBlockCoordOffset(section, offset)}}.dump();
    }
    if (mode == "asLong" && parts.size() == 4) {
        const i64 packed = asLong(static_cast<i32>(std::stoll(parts[1])),
            static_cast<i32>(std::stoll(parts[2])),
            static_cast<i32>(std::stoll(parts[3])));
        // 长整型必须按字符串输出——JS 安全整数上限是 2^53。
        return json{{"unit", unit}, {"value", std::to_string(packed)}}.dump();
    }
    if (mode == "decode" && parts.size() == 2) {
        const i64 packed = std::stoll(parts[1]);
        const std::string decoded = std::to_string(decodeX(packed)) + "," + std::to_string(decodeY(packed)) + "," +
            std::to_string(decodeZ(packed));
        return json{{"unit", unit}, {"value", decoded}}.dump();
    }
    if (mode == "roundtrip" && parts.size() == 4) {
        const i32 x = static_cast<i32>(std::stoll(parts[1]));
        const i32 y = static_cast<i32>(std::stoll(parts[2]));
        const i32 z = static_cast<i32>(std::stoll(parts[3]));
        const i64 packed = asLong(x, y, z);
        const std::string decoded = std::to_string(decodeX(packed)) + "," + std::to_string(decodeY(packed)) + "," +
            std::to_string(decodeZ(packed));
        return json{{"unit", unit}, {"value", decoded}}.dump();
    }

    throw std::runtime_error("section_pos: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(section_pos, &writeList, &runLine)

} // namespace mc::diff
