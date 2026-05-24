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

#include "../../feature/template/Template.hpp"
#include "../Structure.hpp"
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 下界化石结构
 *
 * 在下界生成的巨大骨化石结构。
 * 由骨块构成的随机形状，增添下界的荒凉感。
 *
 * 参考: MC 1.16.5 NetherFossilStructure.java
 */
class NetherFossilStructure : public Structure {
public:
    NetherFossilStructure();
    ~NetherFossilStructure() override = default;

    [[nodiscard]] const std::string& name() const override { return s_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return s_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return s_validBiomes; }

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static const std::string s_name;
    static constexpr StructureSeparationSettings s_settings{2, 1, 14357921};
    static const std::vector<BiomeId> s_validBiomes;
};

/**
 * @brief 下界化石结构片段
 */
class NetherFossilPiece : public StructurePiece {
public:
    NetherFossilPiece(const BlockPos& pos, i32 fossilType, feature::template_::Rotation rotation);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds) override;

private:
    void generateFossil(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds);

    i32 m_fossilType;
    feature::template_::Rotation m_rotation;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
