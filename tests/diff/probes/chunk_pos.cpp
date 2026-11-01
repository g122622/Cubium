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
 * @brief 差分能力：`ChunkPos` 的区块坐标编码、区域换算与哈希（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.level.ChunkPos`：
 *
 * ```java
 * private static final long COORD_BITS = 32L;
 * private static final long COORD_MASK = 4294967295L;
 * private static final int REGION_BITS = 5;
 * public static final int REGION_SIZE = 32;
 * private static final int REGION_MASK = 31;
 * public static final int REGION_MAX_INDEX = 31;
 * public static final long INVALID_CHUNK_POS = asLong(1875066, 1875066);
 *
 * public ChunkPos(long packed) {
 *     this.x = (int)packed;
 *     this.z = (int)(packed >> 32);
 * }
 *
 * public static long asLong(int x, int z) {
 *     return x & 4294967295L | (z & 4294967295L) << 32;
 * }
 *
 * public static int getX(long packed) { return (int)(packed & 4294967295L); }
 * public static int getZ(long packed) { return (int)(packed >>> 32 & 4294967295L); }
 *
 * public static ChunkPos minFromRegion(int regionX, int regionZ) {
 *     return new ChunkPos(regionX << 5, regionZ << 5);
 * }
 *
 * public static ChunkPos maxFromRegion(int regionX, int regionZ) {
 *     return new ChunkPos((regionX << 5) + 31, (regionZ << 5) + 31);
 * }
 *
 * public static int hash(int x, int z) {
 *     int i = 1664525 * x + 1013904223;
 *     int j = 1664525 * (z ^ -559038737) + 1013904223;
 *     return i ^ j;
 * }
 *
 * // 方块坐标
 * public int getMinBlockX() { return SectionPos.sectionToBlockCoord(this.x); }
 * public int getBlockX(int offset) { return SectionPos.sectionToBlockCoord(this.x, offset); }
 * public int getMiddleBlockX() { return this.getBlockX(8); }
 * public int getMaxBlockX() { return this.getBlockX(15); }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`asLong` 用两个 32 位字段无符号拼接，与 `SectionPos` 的「窄字段 + 符号扩展」完全不同。**
 *
 *    | 类 | 字段位宽 | 有无符号扩展 | `(-1,-1)` 的编码 |
 *    |---|---|---|---|
 *    | `ChunkPos` | X 32 位、Z 32 位 | **无**（`& 0xFFFFFFFF`） | `-1L`（64 位全 1） |
 *    | `SectionPos` | X/Z 22 位、Y 20 位 | **有** | 见 [`section_pos`](./section_pos.md) |
 *
 *    `ChunkPos` 两个字段各占满 32 位，恰好铺满整个 `long`，**没有空隙也没有符号扩展余地**。
 *
 * 2. **反解 X 用 `&`，反解 Z 用 `>>>`。**
 *
 *    ```java
 *    getX(packed) = (int)(packed & 4294967295L)
 *    getZ(packed) = (int)(packed >>> 32 & 4294967295L)
 *    ```
 *
 *    低位直接掩码即可；高位必须**逻辑右移**（`>>>`）把符号位补 0，再掩码取低 32 位。若用
 *    算术右移 `>>`，负数打包值会把高 32 位补满 1，`getZ` 恒为 `-1`。
 *
 * 3. **区域是 32×32 个区块，用 `<< 5` / `+ 31` 换算。**
 *
 *    ```java
 *    minFromRegion(rx, rz) = (rx << 5,        rz << 5)
 *    maxFromRegion(rx, rz) = ((rx << 5) + 31, (rz << 5) + 31)
 *    ```
 *
 *    **`REGION_SIZE = 32` 而不是 16**——区域是区块级的 2 的幂，与方块级的分区尺寸（16）不同。
 *    `+ 31` 而不是 `+ 32`：区域 `r` 覆盖区块 `r*32 .. r*32+31`，共 32 个。
 *
 *    注意 `<< 5` 对负区域号也是正确的（左移不改变符号语义），`-1 << 5 = -32`，
 *    所以区域 `-1` 覆盖区块 `-32..-1`。
 *
 * 4. **`hash` 是两个独立的 LCG 混合后异或。**
 *
 *    ```java
 *    i = 1664525 * x + 1013904223;                    // 与 `Mth.murmurHash3Mixer` 无关
 *    j = 1664525 * (z ^ -559038737) + 1013904223;     // Z 先异或一个魔数
 *    return i ^ j;
 *    ```
 *
 *    **X 与 Z 的处理不对称**：Z 先与 `-559038737` 异或再走同样的 LCG。这保证 `(0,0)` 与
 *    `(0,1)` 之类的小坐标也能散开，不会因为 `z = 0` 让 `j` 退化成常数。
 *    Java 的 `int` 乘法溢出环绕。
 *
 * 单元格式：
 * - `asLong|<x>|<z>` —— 输出长整型（字符串）
 * - `decode|<packed>` —— 输出 `x,z`
 * - `roundtrip|<x>|<z>` —— 编码后反解
 * - `minRegion|<rx>|<rz>` / `maxRegion|<rx>|<rz>` —— 区域角点
 * - `hash|<x>|<z>` —— 哈希值
 * - `blocks|<x>|<z>` —— 输出 `minX,middleX,maxX,minZ,middleZ,maxZ`
 * - `invalid` —— 输出 `INVALID_CHUNK_POS`
 *
 * TODO: 未覆盖 `isValid`（依赖 `MAX_COORDINATE_VALUE`，而后者依赖 `ChunkPyramid` 的
 * 累积依赖数量）、`getRegionX/Z`、`getBlockZ` 系列（与 X 对称）。
 */
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

constexpr i64 COORD_MASK = 4294967295L;
constexpr i32 REGION_SIZE = 32;

/// 原版 `ChunkPos.asLong`：两个 32 位字段无符号拼接。
i64 asLong(i32 x, i32 z)
{
    return (static_cast<i64>(x) & COORD_MASK) | ((static_cast<i64>(z) & COORD_MASK) << 32);
}

/// 原版 `ChunkPos.getX`：取低 32 位。
i32 getX(i64 packed)
{
    return static_cast<i32>(packed & COORD_MASK);
}

/// 原版 `ChunkPos.getZ`：逻辑右移 32 位再取低 32 位。
i32 getZ(i64 packed)
{
    return static_cast<i32>(static_cast<u64>(packed) >> 32 & static_cast<u64>(COORD_MASK));
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

/// 原版 `ChunkPos.hash`：两个 LCG 混合后异或。
i32 chunkHash(i32 x, i32 z)
{
    const u32 i = static_cast<u32>(1664525) * static_cast<u32>(x) + 1013904223u;
    const u32 j = static_cast<u32>(1664525) * (static_cast<u32>(z) ^ static_cast<u32>(-559038737)) + 1013904223u;
    return static_cast<i32>(i ^ j);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // asLong：基本
        "asLong|0|0",
        "asLong|1|0",
        "asLong|0|1",
        "asLong|1|2",
        // asLong：负数（无符号拼接）
        "asLong|-1|-1",
        "asLong|-1|0",
        "asLong|0|-1",
        "asLong|-5|-20",
        // asLong：大值
        "asLong|1875066|1875066",
        "asLong|2147483647|-2147483648",
        // decode
        "decode|0",
        "decode|1",
        "decode|4294967296",
        "decode|-1",
        // roundtrip
        "roundtrip|1|2",
        "roundtrip|-1|-1",
        "roundtrip|-5|-20",
        "roundtrip|2147483647|-2147483648",
        // 区域角点
        "minRegion|0|0",
        "maxRegion|0|0",
        "minRegion|1|2",
        "maxRegion|1|2",
        "minRegion|-1|-1",
        "maxRegion|-1|-1",
        // hash
        "hash|0|0",
        "hash|0|1",
        "hash|1|0",
        "hash|1|2",
        "hash|-1|-1",
        // 方块边界
        "blocks|0|0",
        "blocks|1|2",
        "blocks|-1|-1",
        // 常量
        "invalid",
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
    if (parts.empty()) {
        throw std::runtime_error("chunk_pos: 空单元");
    }

    const std::string mode = parts[0];

    if (mode == "asLong" && parts.size() == 3) {
        const i64 packed = asLong(std::stoi(parts[1]), std::stoi(parts[2]));
        return json{{"unit", unit}, {"value", std::to_string(packed)}}.dump();
    }
    if (mode == "decode" && parts.size() == 2) {
        const i64 packed = std::stoll(parts[1]);
        const std::string value = std::to_string(getX(packed)) + "," + std::to_string(getZ(packed));
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "roundtrip" && parts.size() == 3) {
        const i32 x = std::stoi(parts[1]);
        const i32 z = std::stoi(parts[2]);
        const i64 packed = asLong(x, z);
        const std::string value = std::to_string(getX(packed)) + "," + std::to_string(getZ(packed));
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "minRegion" && parts.size() == 3) {
        const i32 rx = std::stoi(parts[1]) << 5;
        const i32 rz = std::stoi(parts[2]) << 5;
        return json{{"unit", unit}, {"value", std::to_string(rx) + "," + std::to_string(rz)}}.dump();
    }
    if (mode == "maxRegion" && parts.size() == 3) {
        const i32 rx = (std::stoi(parts[1]) << 5) + (REGION_SIZE - 1);
        const i32 rz = (std::stoi(parts[2]) << 5) + (REGION_SIZE - 1);
        return json{{"unit", unit}, {"value", std::to_string(rx) + "," + std::to_string(rz)}}.dump();
    }
    if (mode == "hash" && parts.size() == 3) {
        const i32 value = chunkHash(std::stoi(parts[1]), std::stoi(parts[2]));
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "blocks" && parts.size() == 3) {
        const i32 x = std::stoi(parts[1]);
        const i32 z = std::stoi(parts[2]);
        const std::string value = std::to_string(sectionToBlockCoord(x)) + "," +
            std::to_string(sectionToBlockCoordOffset(x, 8)) + "," + std::to_string(sectionToBlockCoordOffset(x, 15)) +
            "," + std::to_string(sectionToBlockCoord(z)) + "," + std::to_string(sectionToBlockCoordOffset(z, 8)) + "," +
            std::to_string(sectionToBlockCoordOffset(z, 15));
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "invalid" && parts.size() == 1) {
        const i64 packed = asLong(1875066, 1875066);
        return json{{"unit", unit}, {"value", std::to_string(packed)}}.dump();
    }

    throw std::runtime_error("chunk_pos: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("chunk_pos", &writeList, &runLine)

} // namespace mc::diff
