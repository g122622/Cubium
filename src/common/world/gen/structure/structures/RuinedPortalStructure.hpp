#pragma once

#include "../../chunk/IChunkGenerator.hpp"
#include "../Structure.hpp"
#include <memory>

namespace mc::world::gen::structure {

/**
 * @brief 废弃传送门结构片段
 */
class RuinedPortalPiece : public StructurePiece {
public:
    RuinedPortalPiece(i32 x, i32 y, i32 z, i32 sizeX, i32 sizeY, i32 sizeZ);

    /**
     * @brief 在区块中生成片段
     */
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds) override;

private:
    /**
     * @brief 检查位置是否在区块边界内
     */
    [[nodiscard]] bool isInBounds(i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds) const;
};

/**
 * @brief 废弃传送门结构
 *
 * 废弃传送门是主世界和下界中自然生成的残破下界传送门结构。
 * 参考 MC 1.16.5: RuinedPortalStructure
 */
class RuinedPortalStructure : public Structure {
public:
    RuinedPortalStructure()
        : Structure(StructureType::RuinedPortal)
    {}

    [[nodiscard]] const std::string& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成废弃传送门
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static constexpr StructureSeparationSettings m_settings{40, 15, 34222645};
    static const std::string m_name;
    static const std::vector<BiomeId> m_validBiomes;

    /**
     * @brief 生成传送门框架
     */
    void generatePortalFrame(IWorldWriter& world, i32 x, i32 y, i32 z, math::Random& rng, bool isNether) const;
};

} // namespace mc::world::gen::structure
