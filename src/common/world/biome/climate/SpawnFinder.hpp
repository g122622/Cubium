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

#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "world/biome/climate/ParameterTypes.hpp"
#include <vector>

// 前向声明 Sampler（SpawnFinder 取 const Sampler&，仅在 .cpp 中调用 Sampler::sample）
namespace mc::world::biome::climate {
class Sampler;
}

namespace mc::world::biome::climate {

/**
 * @brief 气候空间中的出生点查找器
 *
 * 通过径向搜索在气候参数空间中找到最佳出生点。
 *
 * 搜索策略：
 * 1. 从 (0, 0) 开始计算初始 fitness
 * 2. 粗搜索：半径 512..2048，步长 512
 * 3. 精搜索：半径 32..512，步长 32
 *
 * fitness = minParameterFitness * 2048² + distanceFromOrigin²
 * 其中 minParameterFitness 是所有 spawn target 中最小的 fitness 值，
 * depth 参数被置零。
 */
class SpawnFinder {
public:
    /**
     * @brief 搜索结果
     */
    struct Result {
        i32 x;       ///< 出生点 X 坐标
        i32 z;       ///< 出生点 Z 坐标
        i64 fitness; ///< fitness 值（越小越好）
    };

    /**
     * @brief 构造出生点查找器
     *
     * @param spawnTargets 生成目标参数列表
     * @param sampler 气候采样器
     */
    SpawnFinder(std::vector<ParameterPoint> spawnTargets, const Sampler& sampler);

    /**
     * @brief 获取搜索结果
     */
    [[nodiscard]] const Result& result() const { return m_result; }

    /**
     * @brief 静态方法：查找出生点
     *
     * @param spawnTargets 生成目标参数列表
     * @param sampler 气候采样器
     * @return 出生点坐标 (x, 0, z)
     */
    [[nodiscard]] static BlockPos findSpawnPosition(
        const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler);

private:
    static constexpr f64 MAX_RADIUS = 2048.0;
    static constexpr i64 SQUARE_MAX_RADIUS = 2048LL * 2048LL;

    /**
     * @brief 计算指定位置的 fitness
     *
     * fitness = minParameterFitness * 2048² + distanceFromOrigin²
     * depth 参数被置零。
     */
    [[nodiscard]] static Result getSpawnPositionAndFitness(
        const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler, i32 x, i32 z);

    /**
     * @brief 径向搜索
     *
     * @param spawnTargets 生成目标参数列表
     * @param sampler 气候采样器
     * @param maxRadius 最大搜索半径
     * @param stepSize 搜索步长
     */
    void radialSearch(
        const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler, f32 maxRadius, f32 stepSize);

    Result m_result;
};

} // namespace mc::world::biome::climate
