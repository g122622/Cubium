#pragma once

#include "../Structure.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include <memory>

namespace mc::world::gen::structure {

/**
 * @brief 海底废墟结构
 *
 * 简化版海底废墟，兼容冷海/暖海两类材质风格。
 */
class OceanRuinStructure : public Structure {
public:
    OceanRuinStructure();

    [[nodiscard]] const String& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    [[nodiscard]] bool canGenerate(
        IWorld& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) const override;

private:
    void initializeBiomes();

    void generateRuin(
        IWorldWriter& world,
        math::Random& rng,
        const BlockPos& origin,
        bool warmVariant) const;

    [[nodiscard]] bool isWarmBiome(BiomeId biomeId) const;

    static constexpr StructureSeparationSettings m_settings{20, 8, 14357621};  // MC 1.16.5: 14357621
    static const String m_name;
    std::vector<BiomeId> m_validBiomes;
};

} // namespace mc::world::gen::structure
