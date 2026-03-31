#include "Dimension.hpp"
#include "../gen/settings/DimensionSettings.hpp"
#include "../gen/chunk/NoiseChunkGenerator.hpp"
#include "../gen/chunk/NetherChunkGenerator.hpp"
#include "../gen/chunk/EndChunkGenerator.hpp"
#include "../biome/layer/LayerUtil.hpp"
#include "../biome/provider/nether/NetherBiomeProvider.hpp"
#include "../biome/provider/end/EndBiomeProvider.hpp"

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

Dimension::Dimension(DimensionId id,
                     DimensionType type,
                     std::unique_ptr<IChunkGenerator> generator,
                     std::unique_ptr<BiomeProvider> biomeProvider)
    : m_id(id)
    , m_type(std::move(type))
    , m_generator(std::move(generator))
    , m_biomeProvider(std::move(biomeProvider))
{
}

// ============================================================================
// 更新
// ============================================================================

void Dimension::tick() {
    // 基类默认无操作
    // 子类可覆盖以实现维度特定逻辑（如末地的末影龙战斗）
}

// ============================================================================
// 工厂方法
// ============================================================================

std::unique_ptr<Dimension> Dimension::createOverworld(u64 seed) {
    DimensionType dimType = DimensionType::overworld();

    // 创建生物群系提供者
    auto biomeProvider = std::make_unique<LayerBiomeProvider>(seed, false);

    // 创建区块生成器
    auto settings = DimensionSettings::overworld();
    auto generator = std::make_unique<NoiseChunkGenerator>(seed, std::move(settings));

    auto dimension = std::make_unique<Dimension>(
        0,  // DimensionId::Overworld
        std::move(dimType),
        std::move(generator),
        std::move(biomeProvider)
    );

    // 主世界默认出生点
    dimension->m_spawnPoint = Vector3d(0.0, 64.0, 0.0);

    return dimension;
}

std::unique_ptr<Dimension> Dimension::createNether(u64 seed) {
    DimensionType dimType = DimensionType::nether();

    // 下界使用 NetherBiomeProvider
    auto biomeProvider = std::make_unique<biome::nether::NetherBiomeProvider>(seed);

    // 创建下界区块生成器
    auto settings = DimensionSettings::nether();
    auto generator = std::make_unique<NetherChunkGenerator>(seed, std::move(settings));

    auto dimension = std::make_unique<Dimension>(
        1,  // DimensionId::Nether
        std::move(dimType),
        std::move(generator),
        std::move(biomeProvider)
    );

    // 下界默认出生点
    dimension->m_spawnPoint = Vector3d(0.0, 64.0, 0.0);

    return dimension;
}

std::unique_ptr<Dimension> Dimension::createTheEnd(u64 seed) {
    DimensionType dimType = DimensionType::theEnd();

    // 末地使用 EndBiomeProvider
    auto biomeProvider = std::make_unique<biome::end::EndBiomeProvider>(seed);

    // 创建末地区块生成器
    auto settings = DimensionSettings::end();
    auto generator = std::make_unique<EndChunkGenerator>(seed, std::move(settings));

    auto dimension = std::make_unique<Dimension>(
        2,  // DimensionId::TheEnd
        std::move(dimType),
        std::move(generator),
        std::move(biomeProvider)
    );

    // 末地默认出生点（末地传送门平台位置）
    dimension->m_spawnPoint = Vector3d(100.0, 49.0, 0.0);

    return dimension;
}

} // namespace mc
