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
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
class RandomState;
namespace density {
class EndIslands;
} // namespace density
} // namespace gen
} // namespace world
} // namespace mc

namespace mc {
namespace world {
namespace biome {
namespace source {

/**
 * @brief 末地生物群系源
 *
 * 末地使用专用的生物群系选择算法，不同于 MultiNoiseBiomeSource。
 * 中央岛屿（距原点64格内）固定为 THE_END 生物群系，
 * 外围岛屿使用 EndIslands 密度函数判断生物群系类型。
 *
 * 接收 RandomState 以与 NoiseChunkGenerator 统一构造来源（末地无 NormalNoise 叶子，
 * rs 仅用于 API 统一及获取世界种子）。
 */
class EndBiomeSource : public IBiomeSource {
public:
    explicit EndBiomeSource(const gen::RandomState& rs);
    ~EndBiomeSource() override;

    EndBiomeSource(const EndBiomeSource&) = delete;
    EndBiomeSource& operator=(const EndBiomeSource&) = delete;
    EndBiomeSource(EndBiomeSource&&) noexcept;
    EndBiomeSource& operator=(EndBiomeSource&&) noexcept;

    [[nodiscard]] BiomeId getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const override;
    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override;

private:
    /**
     * @brief 判断是否在中央岛屿范围内
     * @param blockX 方块 X 坐标
     * @param blockZ 方块 Z 坐标
     * @return 如果在中央岛屿范围内返回 true
     *
     * 使用区块坐标判断: chunkX² + chunkZ² <= 4096（64区块半径）
     */
    [[nodiscard]] static bool isInCentralIsland(i32 blockX, i32 blockZ);

    std::unique_ptr<gen::density::EndIslands> m_islandNoise;
};

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
