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

#include "DimensionSettings.hpp"
#include "common/core/Constants.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {

DimensionSettings DimensionSettings::overworld() noexcept
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::overworld();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::WATER);
    settings.seaLevel = world::SEA_LEVEL;
    settings.dimensionKind = DimensionKind::Overworld;
    settings.oreVeinsEnabled = true;
    settings.disableMobGeneration = false;
    return settings;
}

DimensionSettings DimensionSettings::largeBiomesPreset() noexcept
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::overworld();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::WATER);
    settings.seaLevel = world::SEA_LEVEL;
    settings.dimensionKind = DimensionKind::LargeBiomes;
    settings.largeBiomes = true;
    settings.oreVeinsEnabled = true;
    settings.disableMobGeneration = false;
    return settings;
}

DimensionSettings DimensionSettings::amplified() noexcept
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::amplified();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::WATER);
    settings.seaLevel = world::SEA_LEVEL;
    settings.dimensionKind = DimensionKind::Amplified;
    settings.oreVeinsEnabled = true;
    settings.disableMobGeneration = false;
    return settings;
}

DimensionSettings DimensionSettings::nether() noexcept
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::nether();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::LAVA);
    settings.seaLevel = 32;
    settings.dimensionKind = DimensionKind::Nether;
    settings.oreVeinsEnabled = false;
    settings.disableMobGeneration = false;
    return settings;
}

DimensionSettings DimensionSettings::end() noexcept
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::end();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::END_STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::AIR);
    settings.seaLevel = 0;
    settings.dimensionKind = DimensionKind::End;
    settings.oreVeinsEnabled = false;
    settings.disableMobGeneration = true;
    return settings;
}

DimensionSettings DimensionSettings::caves() noexcept
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::caves();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::WATER);
    settings.seaLevel = 32;
    settings.dimensionKind = DimensionKind::Caves;
    settings.oreVeinsEnabled = false;
    settings.disableMobGeneration = false;
    return settings;
}

DimensionSettings DimensionSettings::floatingIslands() noexcept
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::floatingIslands();
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::WATER);
    settings.seaLevel = -64;
    settings.dimensionKind = DimensionKind::FloatingIslands;
    settings.oreVeinsEnabled = false;
    settings.disableMobGeneration = false;
    return settings;
}

DimensionSettings DimensionSettings::flat() noexcept
{
    DimensionSettings settings;
    settings.noise.height = 4;
    settings.noise.densityFactor = 0.0;
    settings.noise.densityOffset = 0.0;
    settings.defaultBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    settings.defaultFluid = VanillaBlocks::getState(VanillaBlocks::AIR);
    settings.seaLevel = 0;
    settings.dimensionKind = DimensionKind::Flat;
    return settings;
}

} // namespace mc
