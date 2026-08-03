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

#include "../../chunk/IChunkGenerator.hpp"
#include "../Structure.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include <memory>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 沙漠神殿结构片段
 *
 * 在 FEATURES 阶段由 placeInChunk() 调用 generate() 写入方块。
 */
class DesertPyramidPiece final : public StructurePiece {
public:
    explicit DesertPyramidPiece(const BlockPos& startPos);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    BlockPos m_startPos;
    void _generatePyramid(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds);
};

/**
 * @brief 沙漠神殿结构
 *
 * 沙漠神殿是生成在沙漠生物群系的结构，包含隐藏的宝藏室。
 *
 * 特点：
 * - 仅在沙漠生物群系（minecraft:desert）生成
 * - 包含 4 个宝箱和可能的 TNT 陷阱
 * - 底部有隐藏的地窖
 * - 橙色陶瓦装饰
 */
class DesertPyramidStructure : public Structure {
public:
    explicit DesertPyramidStructure(ResourceLocation id);

    [[nodiscard]] const std::string& name() const override { return m_name; }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/desert_pyramid 标签，用于 O(1) 生物群系查找。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 检查是否可以生成
     *
     * 检查区块中心生物群系是否为沙漠，以及结构四角最低高度是否不低于海平面。
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成沙漠神殿起点（仅创建 StructurePiece，禁止写方块）
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static const std::string m_name;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
