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

#include "../Structure.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 埋藏的宝藏结构片段
 */
class BuriedTreasurePiece : public StructurePiece {
public:
    BuriedTreasurePiece(i32 x, i32 y, i32 z);

    /**
     * @brief 在区块中生成片段
     */
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    /**
     * @brief 检查位置是否在区块边界内
     */
    [[nodiscard]] bool _isInBounds(i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds) const;
};

/**
 * @brief 埋藏的宝藏结构
 *
 * 最简单的结构类型，只包含一个箱子。
 */
class BuriedTreasureStructure : public Structure {
public:
    explicit BuriedTreasureStructure(ResourceLocation id)
        : Structure(std::move(id))
    {}

    [[nodiscard]] const std::string& name() const override { return m_name; }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/buried_treasure 标签，用于 O(1) 生物群系查找。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 埋藏宝藏的定位偏移
     *
     * MC 1.21.11 中埋藏宝藏使用 (9, 0, 9) 偏移，
     * 使得 /locate 命令指向区块内偏移 9 格的位置而非默认的 8 格。
     */
    [[nodiscard]] math::Vector3i locateOffset() const { return math::Vector3i(9, 0, 9); }

    [[nodiscard]] DecorationStage defaultDecorationStage() const override
    {
        return DecorationStage::UndergroundStructures;
    }

    /**
     * @brief 检查是否可以生成
     * 埋藏的宝藏只在沙滩类生物群系生成，且概率较低
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成埋藏的宝藏起点（仅创建 StructurePiece，禁止写方块）
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    // 注意：canGenerate中使用单独的salt(10387320)来计算概率种子
    static const std::string m_name;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
