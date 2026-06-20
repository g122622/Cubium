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

#include "../settings/FlatLevelGeneratorSettings.hpp"
#include "IChunkGenerator.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"

namespace mc {

/**
 * @brief 平坦世界区块生成器
 *
 * 生成平坦层次的世界地形。
 *
 * 特点：
 * - 使用 FlatLevelGeneratorSettings 定义层配置
 * - generateNoise: 逐层填充方块（基岩、泥土、草方块等）
 * - buildSurface/applyCarvers/spawnInitialMobs: 空操作
 * - 所有位置返回固定生物群系
 *
 * MC 默认配置：
 * - 1x Bedrock + 2x Dirt + 1x Grass Block
 * - 生物群系: Plains
 * - 海平面: -63（低于世界底部，无水）
 */
class FlatChunkGenerator : public BaseChunkGenerator {
public:
    /**
     * @brief 构造平坦世界区块生成器
     * @param seed 世界种子
     * @param settings 平坦世界生成设置
     */
    FlatChunkGenerator(u64 seed, FlatLevelGeneratorSettings settings);

    ~FlatChunkGenerator() override = default;

    // === IChunkGenerator 接口 ===

    void generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateNoise(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void buildSurface(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) override;
    i32 spawnInitialMobs(
        WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities) override;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z, HeightmapType type) const override;
    [[nodiscard]] i32 getGroundHeight() const override;

    [[nodiscard]] world::biome::IBiomeSource* getBiomeSource() override { return m_biomeSource.get(); }
    [[nodiscard]] const world::biome::IBiomeSource* getBiomeSource() const override { return m_biomeSource.get(); }

    [[nodiscard]] i32 getGenDepth() const override { return world::CHUNK_HEIGHT; }
    [[nodiscard]] i32 getMinY() const override { return 0; }
    [[nodiscard]] i32 seaLevel() const override { return -63; }

    /**
     * @brief 获取指定位置的基础列方块状态
     *
     * 返回展开层列表中的方块状态，null 替换为空气。
     */
    [[nodiscard]] NoiseColumn getBaseColumn(i32 x, i32 z) const override;

private:
    FlatLevelGeneratorSettings m_flatSettings;
    std::unique_ptr<world::biome::IBiomeSource> m_biomeSource;
};

} // namespace mc
