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

#include "common/world/biome/BiomeSource.hpp"
#include "common/world/biome/Biomes.hpp"
#include <memory>

namespace mc::world::gen::density {
class EndIslands;
}

namespace mc::world::biome::source {

/**
 * @brief 末地生物群系源（MC 1.18+）
 *
 * 末地使用专用的生物群系选择算法，不同于 MultiNoiseBiomeSource。
 * 中央岛屿（距原点64格内）固定为 THE_END 生物群系，
 * 外围岛屿使用 EndIslands 密度函数判断生物群系类型。
 */
class EndBiomeSource : public BiomeSource {
public:
    explicit EndBiomeSource(u64 seed);
    ~EndBiomeSource() override;

    EndBiomeSource(const EndBiomeSource&) = delete;
    EndBiomeSource& operator=(const EndBiomeSource&) = delete;
    EndBiomeSource(EndBiomeSource&&) noexcept;
    EndBiomeSource& operator=(EndBiomeSource&&) noexcept;

    [[nodiscard]] BiomeId getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const override;
    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override;
    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) override;

private:
    /**
     * @brief 判断是否在中央岛屿范围内
     * @param blockX 方块 X 坐标
     * @param blockZ 方块 Z 坐标
     * @return 如果在中央岛屿范围内返回 true
     */
    [[nodiscard]] static bool isInCentralIsland(i32 blockX, i32 blockZ);

    std::unique_ptr<gen::density::EndIslands> m_islandNoise;
    std::vector<BiomeId> m_possibleBiomes;
};

} // namespace mc::world::biome::source
