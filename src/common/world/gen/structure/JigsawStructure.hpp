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

#include "../../../resource/ResourceLocation.hpp"
#include "../jigsaw/JigsawPattern.hpp"
#include "Structure.hpp"
#include "StructureBoundingBox.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

struct JigsawConfig {
    ResourceLocation startPool;
    i32 size = 7;

    JigsawConfig() = default;
    JigsawConfig(const ResourceLocation& pool, i32 s)
        : startPool(pool)
        , size(s)
    {}
};

class JigsawStructure : public Structure {
public:
    explicit JigsawStructure(
        const JigsawConfig& config, i32 startY = 0, bool nearTerrain = false, bool adjustForTerrain = false);

    const std::string& name() const override { return m_name; }
    StructureSeparationSettings separationSettings() const override { return m_settings; }
    const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    bool canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    JigsawConfig m_config;
    i32 m_startY;
    bool m_nearTerrain;
    bool m_adjustForTerrain;

    static constexpr StructureSeparationSettings m_settings{8, 4, 12345};
    static const std::string m_name;
    static const std::vector<BiomeId> m_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
