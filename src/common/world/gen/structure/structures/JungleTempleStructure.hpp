#pragma once

#include "../Structure.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 丛林神庙结构
 *
 * 丛林神庙是生成在丛林生物群系的结构，包含陷阱和谜题。
 * 参考 MC 1.16.5: JungleTempleStructure
 *
 * 特点：
 * - 生成在丛林生物群系
 * - 包含谜题机关（拉杆谜题）
 * - 箭矢陷阱
 * - 隐藏宝箱
 * - 苔石和錾制石砖装饰
 */
class JungleTempleStructure : public Structure {
public:
    JungleTempleStructure();

    [[nodiscard]] const String& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) override;

    /**
     * @brief 生成丛林神庙
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) const override;

private:
    void initializeBiomes();
    void generateTemple(IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const;

    static constexpr StructureSeparationSettings m_settings{32, 8, 14357621};
    static const String m_name;
    std::vector<BiomeId> m_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
