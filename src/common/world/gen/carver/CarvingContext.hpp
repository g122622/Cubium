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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"

namespace mc {

// 前向声明
class BlockState;
namespace world::gen::aquifer {
class Aquifer;
}
namespace world::gen::density {
class NoiseChunk;
}
namespace world::gen {
class RandomState;
}

// ============================================================================
// CarvingContext — 雕刻上下文
// ============================================================================

/**
 * @brief 雕刻上下文（MC 1.21 CarvingContext）
 *
 * 提供雕刻器执行时所需的世界生成信息，继承自 WorldGenerationContext
 * 并添加含水层引用、噪声区块和随机状态。
 *
 * MC 1.21 CarvingContext 字段对照：
 * - WorldGenerationContext (minY, genDepth) ✅
 * - Aquifer ✅
 * - NoiseChunk ✅ (新增)
 * - RandomState ✅ (新增)
 * - SurfaceRules.RuleSource — 暂不添加（topMaterial 已标记 @Deprecated）
 */
class CarvingContext : public world::gen::valueprovider::WorldGenerationContext {
public:
    /**
     * @brief 最小构造（向后兼容）
     * @param minGenY 最低生成高度
     * @param genDepth 生成深度
     * @param aquifer 含水层采样器引用（可为 nullptr 表示禁用含水层）
     */
    CarvingContext(i32 minGenY, i32 genDepth, world::gen::aquifer::Aquifer* aquifer)
        : WorldGenerationContext(minGenY, genDepth)
        , m_aquifer(aquifer)
        , m_noiseChunk(nullptr)
        , m_randomState(nullptr)
    {}

    /**
     * @brief MC 1.21 完整构造函数
     * @param minGenY 最低生成高度
     * @param genDepth 生成深度
     * @param aquifer 含水层采样器引用（可为 nullptr）
     * @param noiseChunk 噪声区块（可为 nullptr）
     * @param randomState 随机状态（可为 nullptr）
     */
    CarvingContext(i32 minGenY,
        i32 genDepth,
        world::gen::aquifer::Aquifer* aquifer,
        const world::gen::density::NoiseChunk* noiseChunk,
        const world::gen::RandomState* randomState)
        : WorldGenerationContext(minGenY, genDepth)
        , m_aquifer(aquifer)
        , m_noiseChunk(noiseChunk)
        , m_randomState(randomState)
    {}

    /**
     * @brief 获取含水层采样器
     */
    [[nodiscard]] world::gen::aquifer::Aquifer* aquifer() { return m_aquifer; }
    [[nodiscard]] const world::gen::aquifer::Aquifer* aquifer() const { return m_aquifer; }

    /**
     * @brief 含水层是否可用
     */
    [[nodiscard]] bool hasAquifer() const { return m_aquifer != nullptr; }

    /**
     * @brief 获取噪声区块
     *
     * MC 1.21: CarvingContext.noiseChunk()
     * 用于雕刻器查询密度值或预备表面高度。
     */
    [[nodiscard]] const world::gen::density::NoiseChunk* noiseChunk() const { return m_noiseChunk; }

    /**
     * @brief 获取随机状态
     *
     * MC 1.21: CarvingContext.randomState()
     * 用于雕刻器访问密度函数或表面规则。
     */
    [[nodiscard]] const world::gen::RandomState* randomState() const { return m_randomState; }

private:
    world::gen::aquifer::Aquifer* m_aquifer;
    const world::gen::density::NoiseChunk* m_noiseChunk;
    const world::gen::RandomState* m_randomState;
};

} // namespace mc
