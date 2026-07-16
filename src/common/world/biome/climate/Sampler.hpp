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

// 前向声明，避免循环 include（SpawnFinder 取 const Sampler&，Sampler::findSpawnPosition 调 SpawnFinder）
namespace mc::world::biome::climate {
class SpawnFinder;
}

// 前向声明 DensityFunction（定义在 density/ 目录）
namespace mc::world::gen::density {
class DensityFunction;
}

namespace mc::world::biome::climate {

/**
 * @brief 气候采样器
 *
 * 持有 6 个密度函数引用，在任意 3D 位置采样气候参数值。
 * 密度函数的实例由 NoiseRouter 创建并持有，Sampler 仅引用。
 */
class Sampler {
public:
    /**
     * @brief 构造气候采样器
     *
     * @param temperature 温度密度函数
     * @param humidity 湿度密度函数
     * @param continentalness 大陆度密度函数
     * @param erosion 侵蚀密度函数
     * @param depth 深度密度函数
     * @param weirdness 奇异度密度函数
     */
    Sampler(const mc::world::gen::density::DensityFunction& temperature,
        const mc::world::gen::density::DensityFunction& humidity,
        const mc::world::gen::density::DensityFunction& continentalness,
        const mc::world::gen::density::DensityFunction& erosion,
        const mc::world::gen::density::DensityFunction& depth,
        const mc::world::gen::density::DensityFunction& weirdness);

    /**
     * @brief 在指定 quart 坐标处采样气候值
     *
     * quart 坐标 = block 坐标 / 4
     *
     * @param quartX X quart 坐标
     * @param quartY Y quart 坐标
     * @param quartZ Z quart 坐标
     * @return 采样得到的 TargetPoint
     */
    [[nodiscard]] TargetPoint sample(i32 quartX, i32 quartY, i32 quartZ) const;

    /**
     * @brief 获取生成目标参数列表
     *
     * 用于 SpawnFinder 计算出生点。
     */
    [[nodiscard]] const std::vector<ParameterPoint>& spawnTarget() const { return m_spawnTarget; }

    /**
     * @brief 设置生成目标参数列表
     */
    void setSpawnTarget(std::vector<ParameterPoint> target) { m_spawnTarget = std::move(target); }

    /**
     * @brief 使用气候采样器查找出生点
     *
     * 如果 spawnTarget 为空，返回 (0, 0, 0)。
     */
    [[nodiscard]] BlockPos findSpawnPosition() const;

private:
    const mc::world::gen::density::DensityFunction* m_temperature;
    const mc::world::gen::density::DensityFunction* m_humidity;
    const mc::world::gen::density::DensityFunction* m_continentalness;
    const mc::world::gen::density::DensityFunction* m_erosion;
    const mc::world::gen::density::DensityFunction* m_depth;
    const mc::world::gen::density::DensityFunction* m_weirdness;
    std::vector<ParameterPoint> m_spawnTarget;
};

} // namespace mc::world::biome::climate
