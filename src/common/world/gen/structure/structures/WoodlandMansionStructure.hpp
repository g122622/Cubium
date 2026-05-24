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
 * @brief 林地府邸结构
 *
 * 在黑森林生物群系生成的大型府邸结构。
 * 包含多个房间、走廊和楼梯，内有掠夺者和唤魔者。
 *
 * 参考: MC 1.16.5 WoodlandMansionStructure.java
 */
class WoodlandMansionStructure : public Structure {
public:
    WoodlandMansionStructure();
    ~WoodlandMansionStructure() override = default;

    [[nodiscard]] const std::string& name() const override { return s_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return s_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return s_validBiomes; }

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static const std::string s_name;
    static constexpr StructureSeparationSettings s_settings{80, 20, 10387319};
    static const std::vector<BiomeId> s_validBiomes;
};

/**
 * @brief 林地府邸结构片段
 */
class WoodlandMansionPiece : public StructurePiece {
public:
    WoodlandMansionPiece(const BlockPos& pos, feature::template_::Rotation rotation);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds) override;

private:
    void generateMansion(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds);
    void generateFloor(IWorldWriter& world, i32 floorY, const StructureBoundingBox& bounds);
    void generateRoof(IWorldWriter& world, const StructureBoundingBox& bounds);

    feature::template_::Rotation m_rotation;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
