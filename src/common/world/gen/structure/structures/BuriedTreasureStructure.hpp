#pragma once

#include "../Structure.hpp"
#include <memory>

namespace mc::world::gen::structure {

/**
 * @brief 埋藏的宝藏结构片段
 */
class BuriedTreasurePiece : public StructurePiece {
public:
    BuriedTreasurePiece(i32 x, i32 y, i32 z);

    /**
     * @brief 在区块中生成片段
     */
    void generate(IWorldWriter& world, math::Random& rng,
                  i32 chunkX, i32 chunkZ,
                  const StructureBoundingBox& chunkBounds) override;

private:
    /**
     * @brief 检查位置是否在区块边界内
     */
    [[nodiscard]] bool isInBounds(i32 x, i32 y, i32 z,
                                   const StructureBoundingBox& chunkBounds) const;
};

/**
 * @brief 埋藏的宝藏结构
 *
 * 最简单的结构类型，只包含一个箱子。
 * 参考 MC 1.16.5: BuriedTreasureStructure
 */
class BuriedTreasureStructure : public Structure {
public:
    BuriedTreasureStructure() : Structure(StructureType::BuriedTreasure) {}

    [[nodiscard]] const String& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 检查是否可以生成
     * 埋藏的宝藏只在沙滩类生物群系生成，且概率较低
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) override;

    /**
     * @brief 生成埋藏的宝藏
     * 在区块中心生成一个宝藏箱子
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) const override;

private:
    // MC 1.16.5: spacing=1, separation=0, salt=0
    // 埋藏宝藏每个区块都可能生成，但概率很低
    static constexpr StructureSeparationSettings m_settings{1, 0, 0};
    static const String m_name;
    static const std::vector<BiomeId> m_validBiomes;
};

} // namespace mc::world::gen::structure
