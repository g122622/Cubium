#pragma once

#include "../../chunk/IChunkGenerator.hpp"
#include "../Structure.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 沙漠神殿结构
 *
 * 沙漠神殿是生成在沙漠生物群系的结构，包含隐藏的宝藏室。
 * 参考 MC 1.16.5: DesertPyramidStructure
 *
 * 特点：
 * - 生成在沙漠生物群系
 * - 包含 4 个宝箱和可能的 TNT 陷阱
 * - 底部有隐藏的地窖
 * - 橙色陶瓦装饰
 */
class DesertPyramidStructure : public Structure {
public:
    DesertPyramidStructure();

    [[nodiscard]] const std::string& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成沙漠神殿
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    void initializeBiomes();
    void generatePyramid(IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const;

    // MC 1.16.5: spacing=32, separation=8, salt=14357617
    static constexpr StructureSeparationSettings m_settings{32, 8, 14357617};
    static const std::string m_name;
    std::vector<BiomeId> m_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
