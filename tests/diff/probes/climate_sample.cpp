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
 * @file climate_sample.cpp
 * @brief 差分能力：气候采样器（六条气候噪声）在三维网格上的取值。
 *
 * 群系采样只有「最终选中哪个群系」一层结果，一旦错位就分不清是气候噪声算错、
 * 还是参数表/RTree 最近邻选错。这条能力把采样器单独拎出来：给定维度与种子，
 * 在固定网格上输出六条气候参数的**量化值**（与参数表同一口径），逐点比对。
 *
 * 量化值用整数：两侧都按 `Climate.quantizeCoord` 的同一规则折算，比较因此是精确的，
 * 不会被浮点末位差异干扰。
 *
 * 单元由 `harnesses/climate_sample.ts` 给出，本探针不提供 `list`。
 */

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "common/world/biome/climate/Sampler.hpp"
#include "probe.hpp"
#include "server/world/gen/RandomState.hpp"
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

std::string runLine(std::string_view inputLine)
{
    initializeStructureRegistries();

    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const std::string dimensionName = input.at("dimension").get<std::string>();
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());
    const i32 chunkX = input.at("chunkX").get<i32>();
    const i32 chunkZ = input.at("chunkZ").get<i32>();

    WorldSetup setup = makeWorldSetup(dimensionName, seed);
    const world::biome::climate::Sampler& sampler = setup.randomState->sampler();

    json samples = json::array();
    for (const i32 quartY : SAMPLE_QUART_LEVELS) {
        for (i32 dz = 0; dz < QUARTS_PER_AXIS; ++dz) {
            for (i32 dx = 0; dx < QUARTS_PER_AXIS; ++dx) {
                const i32 quartX = chunkX * (world::CHUNK_WIDTH / BLOCKS_PER_QUART) + dx;
                const i32 quartZ = chunkZ * (world::CHUNK_WIDTH / BLOCKS_PER_QUART) + dz;
                const world::biome::climate::TargetPoint point = sampler.sample(quartX, quartY, quartZ);
                samples.push_back(json::array({point.temperature,
                    point.humidity,
                    point.continentalness,
                    point.erosion,
                    point.depth,
                    point.weirdness}));
            }
        }
    }

    return json{{"unit", input.value("unit", "")}, {"samples", std::move(samples)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(climate_sample, nullptr, &runLine)

} // namespace mc::diff
