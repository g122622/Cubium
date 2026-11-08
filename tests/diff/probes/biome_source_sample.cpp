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
 * @file biome_source_sample.cpp
 * @brief 差分能力：生物群系源在三维网格上的采样结果。
 *
 * 群系源是气候参数采样（六条噪声）加 RTree 最近邻的最终产物，任何一层写错都会表现为
 * 「某些位置群系不对」。结构起点差分只能在结构候选点上间接碰到它，覆盖极稀。
 * 这条能力按固定网格逐点采样同一维度的群系源：水平方向覆盖区块内的 4 个 quart 单元，
 * 竖直方向跨若干层，输出群系资源名与 oracle 逐点比对。
 *
 * 单元由 `harnesses/biome_source_sample.ts` 给出，本探针不提供 `list`。
 */

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"
#include "probe.hpp"
#include "structure_probe_common.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 水平方向每个区块采样的 quart 单元数（一个区块 = 4 个 quart 单元）。
constexpr i32 QUARTS_PER_AXIS = 4;

/// 竖直方向采样的 quart 高度层（对应方块 Y = 4 倍）。
constexpr i32 SAMPLE_QUART_LEVELS[] = {-16, 0, 16, 32, 64};

/// 一个 quart 单元的方块宽度（4 格）。
constexpr i32 BLOCKS_PER_QUART = 4;

/**
 * @brief 按某一维度的世界高度范围推导 quart 高度层
 *
 * 原版与探针必须用**完全相同**的高度层：这里是唯一的口径来源，两侧都从区间推导，
 * 不手抄坐标。overworld 的高度范围是 [-64, 320)，末地是 [0, 256)。
 */
std::string runLine(std::string_view inputLine)
{
    initializeStructureRegistries();
    // 群系注册名映射是独立初始化的一步（服务端由 RegistryBootstrap 负责），
    // 本探针直接构造群系源，必须显式建表。
    if (::mc::world::biome::JavaBiomeRegistryIdMap::instance().initialize().failed()) {
        throw std::runtime_error("biome_source_sample: java biome registry id map initialization failed");
    }

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string dimensionName = input.at("dimension").get<std::string>();
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 chunkX = input.at("chunkX").get<i32>();
    const i32 chunkZ = input.at("chunkZ").get<i32>();

    WorldSetup setup = makeWorldSetup(dimensionName, seed);
    auto biomeSource = makeBiomeSource(dimensionName, *setup.randomState);

    json biomes = json::array();
    for (const i32 quartY : SAMPLE_QUART_LEVELS) {
        for (i32 dz = 0; dz < QUARTS_PER_AXIS; ++dz) {
            for (i32 dx = 0; dx < QUARTS_PER_AXIS; ++dx) {
                const i32 quartX = (chunkX * world::CHUNK_WIDTH + dx * BLOCKS_PER_QUART) / BLOCKS_PER_QUART;
                const i32 quartZ = (chunkZ * world::CHUNK_WIDTH + dz * BLOCKS_PER_QUART) / BLOCKS_PER_QUART;
                const BiomeId biomeId = biomeSource->getNoiseBiome(quartX, quartY, quartZ);
                // 内部群系名是 1.16.5 旧名（stone_shore / mountains …），oracle 侧给的是
                // 1.21.11 资源名（stony_shore / windswept_hills …）：必须经注册名映射转译，
                // 不能直接拿 `Biome::name()` 比较，否则差异全是命名口径而不是群系本身。
                const std::optional<std::string> javaName =
                    ::mc::world::biome::JavaBiomeRegistryIdMap::instance().javaNameOf(biomeId);
                biomes.push_back(javaName.has_value()
                        ? *javaName
                        : "<unmapped:" + ::mc::world::biome::BiomeRegistry::instance().get(biomeId).name() + ">");
            }
        }
    }

    return json{{"unit", input.value("unit", "")}, {"biomes", std::move(biomes)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(biome_source_sample, nullptr, &runLine)

} // namespace mc::diff
