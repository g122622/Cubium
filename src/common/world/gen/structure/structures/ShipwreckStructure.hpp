#pragma once

#include "../Structure.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include <memory>

namespace mc::world::gen::structure {

/**
 * @brief 沉船结构
 *
 * 简化版沉船，用于补齐主世界水域结构链路。
 */
class ShipwreckStructure : public Structure {
public:
    ShipwreckStructure();

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

    void generateShipwreck(
        IWorldWriter& world,
        math::Random& rng,
        const BlockPos& origin) const;

    static constexpr StructureSeparationSettings m_settings{24, 4, 165745296};
    static const String m_name;
    std::vector<BiomeId> m_validBiomes;
};

} // namespace mc::world::gen::structure
