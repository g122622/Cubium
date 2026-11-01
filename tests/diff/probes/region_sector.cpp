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
 * @brief 差分能力：区域文件的区块索引与扇区寻址（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... } 等
 *
 * 复刻原版 `ChunkPos` 与 `RegionFile`：
 *
 * ```java
 * // ChunkPos
 * public int getRegionX()       { return this.x >> 5; }
 * public int getRegionZ()       { return this.z >> 5; }
 * public int getRegionLocalX()  { return this.x & 31; }
 * public int getRegionLocalZ()  { return this.z & 31; }
 *
 * // RegionFile
 * private static final int SECTOR_BYTES = 4096;
 * protected static final int SECTOR_INTS = 1024;
 *
 * private static int getOffsetIndex(ChunkPos pos) {
 *     return pos.getRegionLocalX() + pos.getRegionLocalZ() * 32;
 * }
 *
 * private static int getNumSectors(int offset) { return offset & 0xFF; }
 * private static int getSectorNumber(int offset) { return offset >> 8 & 16777215; }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **区块 → 区域的坐标换算用 `>> 5` / `& 31`，不是除法取模。** 对**负数**来说两者不同：
 *    `-1 >> 5 = -1`（区域 -1），而 `-1 / 32 = 0`（若用整除）；`-1 & 31 = 31`（局部 31），
 *    而 `-1 % 32 = -1`（负数余数）。用除法/取模会让负坐标区块落到错误的区域与槽位。
 *
 * 2. **槽位索引是 `localX + localZ * 32`**（列优先），不是 `localX * 32 + localZ`。
 *    region 文件的头表有 1024 项（32×32），每项 4 字节，共 4096 字节 = 1 个扇区。
 *
 * 3. **偏移值是「24 位扇区号 + 8 位扇区数」打包**：高 24 位是起始扇区，低 8 位是该区块占用
 *    的扇区数。所以单个区块最多占 255 个扇区（约 1 MB）。偏移 0 表示该槽位为空。
 *
 * 4. **头部占 2 个扇区**（偏移表 1 扇区 + 时间戳表 1 扇区），所以合法的数据扇区号
 *    `>= 2`。读取时若见到 `< 2` 会告警并视为损坏。
 *
 * 单元格式：`<模式>|<参数…>`
 * - `region|<chunkX>|<chunkZ>` —— 输出 regionX/regionZ/localX/localZ/offsetIndex
 * - `sector|<offset>` —— 从打包的偏移值拆出扇区号与扇区数
 * - `pack|<sector>|<count>` —— 反向打包成偏移值（用于验证位布局）
 *
 * TODO: 未覆盖 `RegionFileStorage` 的扇区分配（`usedSectors` 的首次适配搜索）、
 * `getExternalChunkPath`（`c.<x>.<z>.mcc` 外部文件命名）。
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

/// 原版 `RegionFile.SECTOR_BYTES`。
constexpr i32 SECTOR_BYTES = 4096;

/// 原版 `RegionFile.SECTOR_INTS`。
constexpr i32 SECTOR_INTS = 1024;

/// 每个区域边长的区块数（32）。
constexpr i32 REGION_CHUNK_SIZE = 32;

/// 原版 `ChunkPos.getRegionX`：算术右移。
i32 getRegionCoord(i32 chunkCoord)
{
    return chunkCoord >> 5;
}

/// 原版 `ChunkPos.getRegionLocalX`：按位与。
i32 getRegionLocal(i32 chunkCoord)
{
    return chunkCoord & 31;
}

/// 原版 `RegionFile.getOffsetIndex`：列优先 `localX + localZ * 32`。
i32 getOffsetIndex(i32 localX, i32 localZ)
{
    return localX + localZ * REGION_CHUNK_SIZE;
}

/// 原版 `RegionFile.getNumSectors`：低 8 位。
i32 getNumSectors(i32 offset)
{
    return offset & 0xFF;
}

/// 原版 `RegionFile.getSectorNumber`：高 24 位。
i32 getSectorNumber(i32 offset)
{
    return (offset >> 8) & 16777215;
}

/// 把扇区号与扇区数打包成偏移值（`getSectorNumber` / `getNumSectors` 的逆运算）。
i32 packOffset(i32 sector, i32 count)
{
    return (sector << 8) | (count & 0xFF);
}

/// 单元表。
constexpr const char* UNITS[] = {
    // 区块 -> 区域：原点与正坐标
    "region|0|0",
    "region|1|1",
    "region|31|31",
    "region|32|32",
    "region|63|63",
    "region|64|64",
    // 区块 -> 区域：负坐标（关键：>> 与 / 不同）
    "region|-1|-1",
    "region|-32|-32",
    "region|-33|-33",
    "region|-64|-64",
    // 区块 -> 区域：混合正负
    "region|100|0",
    "region|0|100",
    "region|-100|100",
    // 大坐标
    "region|1000000|-1000000",
    // 槽位索引：角点
    "region|0|0",
    "region|31|0",
    "region|0|31",
    "region|31|31",
    // 扇区拆解：偏移 0（空槽）
    "sector|0",
    // 扇区拆解：典型值
    "sector|514",
    "sector|51400",
    // 扇区拆解：扇区数上限 255
    "sector|25600",
    // 扇区拆解：最大扇区号
    "sector|-1",
    // 扇区拆解：仅扇区号
    "sector|131072",
    // 反向打包
    "pack|2|1",
    "pack|0|0",
    "pack|100|3",
    // 打包：扇区数溢出（只保留低 8 位）
    "pack|1|256",
    "pack|1|255",
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
    if (parts.size() < 2) {
        throw std::runtime_error("region_sector: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "region" && parts.size() == 3) {
        const i32 chunkX = std::stoi(parts[1]);
        const i32 chunkZ = std::stoi(parts[2]);
        const i32 localX = getRegionLocal(chunkX);
        const i32 localZ = getRegionLocal(chunkZ);
        return json{{"unit", unit},
            {"regionX", getRegionCoord(chunkX)},
            {"regionZ", getRegionCoord(chunkZ)},
            {"localX", localX},
            {"localZ", localZ},
            {"offsetIndex", getOffsetIndex(localX, localZ)}}
            .dump();
    }

    if (mode == "sector" && parts.size() == 2) {
        const i32 offset = std::stoi(parts[1]);
        return json{{"unit", unit}, {"sector", getSectorNumber(offset)}, {"count", getNumSectors(offset)}}.dump();
    }

    if (mode == "pack" && parts.size() == 3) {
        const i32 sector = std::stoi(parts[1]);
        const i32 count = std::stoi(parts[2]);
        return json{{"unit", unit}, {"offset", packOffset(sector, count)}}.dump();
    }

    throw std::runtime_error("region_sector: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("region_sector", &writeList, &runLine)

} // namespace mc::diff
