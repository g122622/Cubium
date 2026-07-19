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

#include <memory>

#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 沼泽小屋（女巫小屋）
 *
 * 在沼泽生物群系中生成的女巫小屋结构。
 * 包含炼药台、炼药锅和蘑菇盆栽。
 * 女巫可以在小屋内生成。
 */
class SwampHutStructure : public Structure {
public:
    explicit SwampHutStructure(ResourceLocation id);
    ~SwampHutStructure() override = default;

    [[nodiscard]] const std::string& name() const override { return s_name; }
    [[nodiscard]] const SpawnOverrides* spawnOverrides() const override { return &s_spawnOverrides; }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/swamp_hut 标签，用于 O(1) 生物群系查找。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static const std::string s_name;
    static const SpawnOverrides s_spawnOverrides;
};

/**
 * @brief 沼泽小屋结构片段
 */
class SwampHutPiece : public StructurePiece {
public:
    SwampHutPiece(const BlockPos& pos, feature::template_::Rotation rotation);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    void _generateHut(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds);
    void _generateFloor(IWorldWriter& world, const StructureBoundingBox& bounds);
    void _generateWalls(IWorldWriter& world, const StructureBoundingBox& bounds);
    void _generateRoof(IWorldWriter& world, const StructureBoundingBox& bounds);
    void _generateInterior(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds);
    void _generatePillars(IWorldWriter& world, const StructureBoundingBox& bounds);

    feature::template_::Rotation m_rotation;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
