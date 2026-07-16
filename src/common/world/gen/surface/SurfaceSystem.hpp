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
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/gen/surface/SurfaceRule.hpp"
#include <functional>
#include <memory>

namespace mc::world::chunk {
class ChunkPrimer;
}

namespace mc::world::gen::density {
class NoiseChunk;
}

namespace mc::world::gen::noise {
class NormalNoise;
}

namespace mc::world::gen {
class RandomState;
}

namespace mc::world::gen::surface {

using ChunkPrimer = ::mc::world::chunk::ChunkPrimer;

/**
 * @brief SurfaceSystem — MC 1.21 SurfaceRules 执行器
 *
 * 遍历区块每个方块，根据 SurfaceRules 规则树替换默认方块。
 * 包含恶地石柱扩展和冰山扩展两个特殊处理。
 */
class SurfaceSystem {
public:
    /**
     * @brief 构造 SurfaceSystem
     * @param surfaceRule 表面规则树
     * @param defaultBlock 默认方块（石头等）
     * @param defaultFluid 默认流体（水等）
     * @param seaLevel 海平面高度
     * @param minY 世界最低 Y
     * @param height 世界高度
     * @param randomState RandomState 引用，用于噪声查找和随机工厂
     * @param positionalRandom 位置随机工厂（MC: noiseRandom，用于 getSurfaceDepth、clayBands、扩展等）
     */
    SurfaceSystem(std::shared_ptr<SurfaceRule> surfaceRule,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        i32 minY,
        i32 height,
        world::gen::RandomState& randomState,
        const math::PositionalRandomFactory& positionalRandom);

    /**
     * @brief 构建整个区块的表面
     * @param chunk 区块数据
     * @param getBiomeAt 获取指定位置的生物群系 ID
     * @param noiseChunk NoiseChunk 引用，用于 preliminarySurfaceLevel 查询
     */
    void buildSurface(ChunkPrimer& chunk,
        const std::function<BiomeId(i32, i32, i32)>& getBiomeAt,
        const density::NoiseChunk& noiseChunk) const;

private:
    /** 判断方块是否为"石头"（非空气、非流体） */
    bool isStone(const BlockState* state) const;

    /**
     * @brief 风蚀恶地地柱扩展（MC: SurfaceSystem.erodedBadlandsExtension）
     * 在 Eroded Badlands 生物群系中生成高耸的石柱/方山地貌。
     */
    void erodedBadlandsExtension(
        ChunkPrimer& chunk, i32 worldX, i32 worldZ, i32 surfaceY, i32 localX, i32 localZ) const;

    /**
     * @brief 冻洋冰山扩展（MC: SurfaceSystem.frozenOceanExtension）
     * 在 Frozen Ocean / Deep Frozen Ocean 生物群系中生成冰山。
     */
    void frozenOceanExtension(ChunkPrimer& chunk,
        i32 worldX,
        i32 worldZ,
        i32 surfaceY,
        i32 localX,
        i32 localZ,
        i32 minSurfaceLevel,
        bool isDeepFrozenOcean,
        BiomeId biomeId) const;

    std::shared_ptr<SurfaceRule> m_surfaceRule;
    const BlockState* m_defaultBlock;
    const BlockState* m_defaultFluid;
    i32 m_seaLevel;
    i32 m_minY;
    i32 m_height;

    // MC 1.21: RandomState 引用，用于噪声查找
    world::gen::RandomState& m_randomState;

    // 位置随机工厂（MC: noiseRandom）
    math::PositionalRandomFactory m_positionalRandom;

    // MC 1.21: 噪声生成器（从 RandomState 获取，不拥有）
    const world::gen::noise::NormalNoise* m_surfaceDepthNoise = nullptr;
    const world::gen::noise::NormalNoise* m_surfaceSecondaryNoise = nullptr;
    const world::gen::noise::NormalNoise* m_clayBandsOffsetNoise = nullptr;

    // Badlands 和冰山噪声（从 RandomState 获取，不拥有）
    const world::gen::noise::NormalNoise* m_badlandsPillarNoise = nullptr;
    const world::gen::noise::NormalNoise* m_badlandsPillarRoofNoise = nullptr;
    const world::gen::noise::NormalNoise* m_badlandsSurfaceNoise = nullptr;
    const world::gen::noise::NormalNoise* m_icebergPillarNoise = nullptr;
    const world::gen::noise::NormalNoise* m_icebergPillarRoofNoise = nullptr;
    const world::gen::noise::NormalNoise* m_icebergSurfaceNoise = nullptr;
};

} // namespace mc::world::gen::surface
