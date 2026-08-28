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
 */

#include "world/biome/climate/SpawnFinder.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "common/world/block/BlockPos.hpp"
#include "world/biome/climate/Sampler.hpp"

#include <cmath>
#include <limits>
#include <vector>

using namespace mc::trace;

namespace mc::world::biome::climate {

SpawnFinder::SpawnFinder(std::vector<ParameterPoint> spawnTargets, const Sampler& sampler)
    : m_result(getSpawnPositionAndFitness(spawnTargets, sampler, 0, 0))
{
    // 1. 计算初始位置 (0,0) 的 fitness
    // 2. 粗搜索: 半径 512..2048, 步长 512
    // 3. 精搜索: 半径 32..512, 步长 32
    radialSearch(spawnTargets, sampler, 2048.0f, 512.0f);
    radialSearch(spawnTargets, sampler, 512.0f, 32.0f);
}

BlockPos SpawnFinder::findSpawnPosition(const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler)
{
    // 父级 ServerWorld::initializeWorldSpawn 已带 trace；此处作为 subpart 量化气候空间径向搜索耗时
    // （SpawnFinder 构造期执行粗搜索半径2048/步512 + 精搜索半径512/步32，是新世界出生点定位的主要开销）。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "SpawnFinder::findSpawnPosition");

    if (spawnTargets.empty()) {
        return BlockPos(0, 0, 0);
    }
    SpawnFinder finder(spawnTargets, sampler);
    return BlockPos(finder.result().x, 0, finder.result().z);
}

SpawnFinder::Result SpawnFinder::getSpawnPositionAndFitness(
    const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler, i32 x, i32 z)
{
    // 1. 在 (x, 0, z) 处采样气候
    // 2. 将 depth 置零（出生点在表面）
    // 3. 计算所有 spawn target 中最小的 fitness
    // 4. fitness = minParameterFitness * 2048² + distanceFromOrigin²

    const i32 quartX = x >> 2; // QuartPos.fromBlock(x)
    const i32 quartZ = z >> 2; // QuartPos.fromBlock(z)
    const TargetPoint sampled = sampler.sample(quartX, 0, quartZ);

    // 将 depth 置零（出生点在表面）
    const TargetPoint targetWithZeroDepth = {
        sampled.temperature, sampled.humidity, sampled.continentalness, sampled.erosion, 0, sampled.weirdness};

    // 计算所有 spawn target 中最小的 fitness
    i64 minFitness = std::numeric_limits<i64>::max();
    for (const auto& point : spawnTargets) {
        const i64 fitness = point.fitness(targetWithZeroDepth);
        if (fitness < minFitness) {
            minFitness = fitness;
        }
    }

    // fitness = minParameterFitness * 2048² + distanceFromOrigin²
    const i64 distanceSquared = static_cast<i64>(x) * static_cast<i64>(x) + static_cast<i64>(z) * static_cast<i64>(z);
    const i64 totalFitness = minFitness * SQUARE_MAX_RADIUS + distanceSquared;

    return {x, z, totalFitness};
}

void SpawnFinder::radialSearch(
    const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler, f32 maxRadius, f32 stepSize)
{
    // 螺旋搜索：角度递增，半径递增
    // 角度步进 = stepSize / currentRadius（大半径时步进更细，小半径时更粗）
    // 每当角度累加超过 2π，重置角度并增大半径
    f32 angle = 0.0f;
    f32 radius = stepSize;
    i32 currentX = m_result.x;
    i32 currentZ = m_result.z;

    while (radius <= maxRadius) {
        const i32 testX = currentX + static_cast<i32>(std::sin(static_cast<f64>(angle)) * static_cast<f64>(radius));
        const i32 testZ = currentZ + static_cast<i32>(std::cos(static_cast<f64>(angle)) * static_cast<f64>(radius));
        const Result candidate = getSpawnPositionAndFitness(spawnTargets, sampler, testX, testZ);

        if (candidate.fitness < m_result.fitness) {
            m_result = candidate;
            currentX = candidate.x;
            currentZ = candidate.z;
        }

        angle += stepSize / radius;
        if (angle > mc::math::TWO_PI) {
            angle = 0.0f;
            radius += stepSize;
        }
    }
}

} // namespace mc::world::biome::climate
