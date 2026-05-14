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
