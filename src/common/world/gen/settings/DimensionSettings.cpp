#include "DimensionSettings.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../block/BlockRegistry.hpp"

namespace mc {

DimensionSettings DimensionSettings::overworld()
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::overworld();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::WATER);
    settings.seaLevel = 63;
    return settings;
}

DimensionSettings DimensionSettings::nether()
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::nether();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::LAVA);
    settings.seaLevel = 31;  // MC 1.16.5: 熔岩海高度 Y<=31
    settings.bedrockRoof = 127;
    settings.bedrockFloor = 0;
    return settings;
}

DimensionSettings DimensionSettings::end()
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::end();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::END_STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::AIR);
    settings.seaLevel = 0;
    return settings;
}

DimensionSettings DimensionSettings::flat()
{
    DimensionSettings settings;
    settings.noise.height = 4;
    settings.noise.densityFactor = 0.0;
    settings.noise.densityOffset = 0.0;
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::AIR);
    settings.seaLevel = 0;
    return settings;
}

} // namespace mc
