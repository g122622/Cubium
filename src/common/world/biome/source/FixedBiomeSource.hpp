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
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <optional>
#include <unordered_set>
#include <vector>

namespace mc {
namespace world {
namespace biome {
namespace source {

/**
 * @brief 固定生物群系源
 *
 * MC 1.21.11: FixedBiomeSource
 * 对所有位置返回同一个固定生物群系。
 * 用于超平坦世界和调试世界。
 *
 * 特点：
 * - getNoiseBiome() 始终返回固定生物群系
 * - findBiome() 仅在 predicate 匹配固定生物群系时返回结果
 * - getBiomesWithin() 始终返回只包含固定生物群系的集合
 */
class FixedBiomeSource : public IBiomeSource {
public:
    /**
     * @brief 构造固定生物群系源
     * @param seed 世界种子
     * @param biomeId 固定返回的生物群系 ID
     */
    explicit FixedBiomeSource(u64 seed, BiomeId biomeId)
        : IBiomeSource(seed)
        , m_biomeId(biomeId)
    {
        m_possibleBiomes.push_back(biomeId);
    }

    [[nodiscard]] BiomeId getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const override
    {
        (void)quartX;
        (void)quartY;
        (void)quartZ;
        return m_biomeId;
    }

    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override { return m_possibleBiomes; }

    /**
     * @brief 获取固定的生物群系 ID
     */
    [[nodiscard]] BiomeId fixedBiomeId() const { return m_biomeId; }

    /**
     * @brief 在指定范围内搜索生物群系
     *
     * 如果 predicate 接受固定生物群系，返回搜索范围内某位置；
     * 否则返回 nullopt。
     */
    [[nodiscard]] std::optional<BlockPos> findBiome(i32 centerX,
        i32 centerY,
        i32 centerZ,
        i32 radius,
        i32 step,
        const std::function<bool(BiomeId)>& predicate,
        math::Random& random,
        bool stopOnFirst) const
    {
        (void)centerY;
        (void)step;
        if (!predicate(m_biomeId)) {
            return std::nullopt;
        }

        if (stopOnFirst) {
            // MC: 返回精确的中心位置
            return BlockPos(centerX, centerY, centerZ);
        }

        // MC: 返回搜索范围内的随机位置
        const i32 rx = centerX - radius + random.nextInt(radius * 2 + 1);
        const i32 rz = centerZ - radius + random.nextInt(radius * 2 + 1);
        return BlockPos(rx, centerY, rz);
    }

    /**
     * @brief 获取指定范围内的生物群系集合
     *
     * 始终返回只包含固定生物群系的集合。
     */
    [[nodiscard]] std::unordered_set<BiomeId> getBiomesWithin(i32 x, i32 y, i32 z, i32 radius) const
    {
        (void)x;
        (void)y;
        (void)z;
        (void)radius;
        return {m_biomeId};
    }

private:
    BiomeId m_biomeId;
};

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
