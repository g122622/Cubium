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

#pragma once
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include <atomic>
#include <functional>
#include <memory>

namespace mc::server {
/** @brief 生成步骤的算法执行器；运行时世界接入通过显式回调提供。 */
class ChunkStepExecutor {
public:
    /** @brief 接管生成器，并绑定维度、区域上下文和光照步骤。 */
    ChunkStepExecutor(std::unique_ptr<IChunkGenerator> generator,
        DimensionId dimension,
        std::function<void(WorldGenRegion&)> configure,
        std::function<void(world::chunk::ChunkPrimer&, WorldGenRegion&)> light);
    /** @brief 执行一个阶段的算法，调用方负责区域执行资格。 */
    void execute(world::chunk::ChunkPrimer& chunk, const world::chunk::ChunkStatus& status, WorldGenRegion& region);
    /** @brief 配置本次任务的区域；种子始终取自生成器。 */
    void configureRegion(WorldGenRegion& region) const;
    /** @brief 记录生成调度或步骤执行活动。 */
    void noteActivity();
    /** @brief 最近一次生成活动时间，仅用于 halo 回收门控。 */
    u64 lastActivity() const;

    /** @brief 生成器的领域入口。 */
    IChunkGenerator* generator() const
    {
        return m_generator.get();
    }

    /** @brief 任务创建时的维度身份。 */
    DimensionId dimension() const
    {
        return m_dimension;
    }

private:
    std::unique_ptr<IChunkGenerator> m_generator;
    DimensionId m_dimension;
    std::function<void(WorldGenRegion&)> m_configure;
    std::function<void(world::chunk::ChunkPrimer&, WorldGenRegion&)> m_light;
    std::atomic<u64> m_lastActivity{0};
};
} // namespace mc::server
