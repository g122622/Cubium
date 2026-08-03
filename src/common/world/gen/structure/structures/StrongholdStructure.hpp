/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction restriction, including without limitation the rights
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

#include "../../chunk/IChunkGenerator.hpp"
#include "../Structure.hpp"
#include "StrongholdPieces.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 要塞结构
 *
 * 要塞是生成在地下的大型结构，包含末地传送门。
 *
 * 特点：
 * - 生成于地下，Y 坐标通常在 20-40
 * - 包含多个房间：图书馆、监狱、传送门房间等
 * - 有复杂的走廊连接系统
 * - 每个世界最多 65 个要塞
 */
class StrongholdStructure : public Structure {
public:
    /**
     * @brief 要塞配置
     */
    struct Config {
        i32 distance = 32; ///< 距离（环之间的距离）
        i32 spread = 3;    ///< 扩散角度
        i32 count = 65;    ///< 最大要塞数量
        i32 minY = 20;     ///< 最低 Y 坐标
        i32 maxY = 40;     ///< 最高 Y 坐标
    };

    explicit StrongholdStructure(ResourceLocation id);
    StrongholdStructure(ResourceLocation id, const Config& config);

    [[nodiscard]] const std::string& name() const noexcept override { return m_name; }

    /**
     * @brief 获取结构关联的生物群系标签
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成要塞
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 计算要塞位置
     * @param index 要塞索引 (0-64)
     * @param worldSeed 世界种子
     * @return 要塞起始区块坐标
     */
    [[nodiscard]] static std::pair<i32, i32> calculateStrongholdPos(i32 index, i64 worldSeed);

    /**
     * @brief 计算要塞所在环
     * @param index 要塞索引
     * @return 环索引 (0-7)
     */
    [[nodiscard]] static i32 getRing(i32 index) noexcept;

private:
    /**
     * @brief 使用 StrongholdPieces 生成要塞
     */
    void _generateStrongholdPieces(
        math::Random& rng, const BlockPos& startPos, std::vector<std::unique_ptr<StructurePiece>>& pieces) const;

    /**
     * @brief 递归生成走廊
     */
    void _generateCorridor(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 depth,
        StrongholdStartStairs* start) const;

    Config m_config;
    static const std::string m_name;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
