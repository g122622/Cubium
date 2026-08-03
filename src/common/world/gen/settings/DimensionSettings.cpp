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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/climate/ParameterPointCodec.hpp"
#include "common/world/biome/source/OverworldBiomeBuilder.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/DensityFunctionLoader.hpp"
#include "common/world/gen/feature/parser/BlockStateParser.hpp"
#include "common/world/gen/settings/NoiseSettings.hpp"
#include "common/world/gen/surface/SurfaceRule.hpp"
#include "common/world/gen/surface/SurfaceRuleDeserializer.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <nlohmann/json_fwd.hpp>

namespace mc {

namespace {

using json = nlohmann::json;

/// noise_settings RL path → DimensionKind（用于补全 C++ 预设的非 JSON 字段）。
/// 用户决策：JSON 只提供 noise 4 尺寸字段，scaling/slides/densityFactor 等由预设补全。
DimensionKind dimensionKindFromPath(std::string_view path)
{
    if (path == "overworld") return DimensionKind::Overworld;
    if (path == "large_biomes") return DimensionKind::LargeBiomes;
    if (path == "amplified") return DimensionKind::Amplified;
    if (path == "nether") return DimensionKind::Nether;
    if (path == "end") return DimensionKind::End;
    if (path == "caves") return DimensionKind::Caves;
    if (path == "floating_islands") return DimensionKind::FloatingIslands;
    return DimensionKind::Overworld;
}

/// 读取必填整数字段
Result<i32> readInt(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "noise_settings: missing integer field '" + std::string(field) + "'");
    }
    return j[field].get<i32>();
}

/// 读取必填 bool 字段
Result<bool> readBool(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_boolean()) {
        return Error(ErrorCode::InvalidData, "noise_settings: missing boolean field '" + std::string(field) + "'");
    }
    return j[field].get<bool>();
}

/// noise_router JSON 键名 → RouterSlot（与 NoiseRouter 构造函数参数顺序一致）
struct RouterField {
    const char* key;
    RouterSlot slot;
};
const RouterField kRouterFields[] = {
    {"barrier", RouterSlot::Barrier},
    {"fluid_level_floodedness", RouterSlot::FluidLevelFloodedness},
    {"fluid_level_spread", RouterSlot::FluidLevelSpread},
    {"lava", RouterSlot::Lava},
    {"temperature", RouterSlot::Temperature},
    {"vegetation", RouterSlot::Vegetation},
    {"continents", RouterSlot::Continents},
    {"erosion", RouterSlot::Erosion},
    {"depth", RouterSlot::Depth},
    {"ridges", RouterSlot::Ridges},
    {"preliminary_surface_level", RouterSlot::PreliminarySurfaceLevel},
    {"final_density", RouterSlot::FinalDensity},
    {"vein_toggle", RouterSlot::VeinToggle},
    {"vein_ridged", RouterSlot::VeinRidged},
    {"vein_gap", RouterSlot::VeinGap},
};

} // namespace

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
    // MC 1.21.11: NoiseGeneratorSettings.overworld() 使用 OverworldBiomeBuilder.spawnTarget()
    settings.spawnTarget = world::biome::source::OverworldBiomeBuilder().spawnTarget();
    // 数据驱动唯一路径：C++ 预设挂规范 noise_settings RL，RandomState::create 据此查 NoiseSettingsRegistry。
    settings.m_noiseSettingsId = resource::ResourceLocation("minecraft", "overworld");
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
    // MC 1.21.11: LARGE_BIOMES 与 OVERWORLD 共用同一 spawnTarget
    settings.spawnTarget = world::biome::source::OverworldBiomeBuilder().spawnTarget();
    settings.m_noiseSettingsId = resource::ResourceLocation("minecraft", "large_biomes");
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
    // MC 1.21.11: AMPLIFIED 与 OVERWORLD 共用同一 spawnTarget
    settings.spawnTarget = world::biome::source::OverworldBiomeBuilder().spawnTarget();
    settings.m_noiseSettingsId = resource::ResourceLocation("minecraft", "amplified");
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
    settings.m_noiseSettingsId = resource::ResourceLocation("minecraft", "nether");
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
    settings.m_noiseSettingsId = resource::ResourceLocation("minecraft", "end");
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
    settings.m_noiseSettingsId = resource::ResourceLocation("minecraft", "caves");
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
    settings.m_noiseSettingsId = resource::ResourceLocation("minecraft", "floating_islands");
    return settings;
}

DimensionSettings DimensionSettings::flat() noexcept
{
    // 超平坦世界使用 FlatChunkGenerator（不走 NoiseChunkGenerator / RandomState::create 路径），
    // 故不挂 noise_settings RL（flat 无对应 vanilla noise_settings JSON）。
    // 阶段4（改造点 D）由 ServerDimensionManager flat 分支直接构造 FlatChunkGenerator。
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

Result<DimensionSettings> DimensionSettings::fromJson(const json& root, const resource::ResourceLocation& id)
{
    if (!root.is_object()) {
        return Error(ErrorCode::InvalidData, "noise_settings root must be an object");
    }

    // 1. RL path → dimensionKind，取对应 C++ 预设作为基底（补全 scaling/slides/densityFactor 等非 JSON 字段）
    const DimensionKind kind = dimensionKindFromPath(id.path());
    DimensionSettings settings;
    switch (kind) {
        case DimensionKind::LargeBiomes:
            settings = largeBiomesPreset();
            break;
        case DimensionKind::Amplified:
            settings = amplified();
            break;
        case DimensionKind::Nether:
            settings = nether();
            break;
        case DimensionKind::End:
            settings = end();
            break;
        case DimensionKind::Caves:
            settings = caves();
            break;
        case DimensionKind::FloatingIslands:
            settings = floatingIslands();
            break;
        case DimensionKind::Overworld:
        default:
            settings = overworld();
            break;
    }
    settings.dimensionKind = kind;
    settings.m_noiseSettingsId = id;
    // spawnTarget 由预设带入（overworld/large/amplified 共用 OverworldBiomeBuilder.spawnTarget()，
    // 其余为空），若 JSON 显式提供 spawn_target 则覆盖。

    // 2. noise 4 尺寸字段（JSON 提供），其余字段（scaling/slides/densityFactor 等）保留预设
    if (!root.contains("noise") || !root["noise"].is_object()) {
        return Error(ErrorCode::InvalidData, "noise_settings: missing 'noise' object");
    }
    const json& noiseObj = root["noise"];
    auto minY = readInt(noiseObj, "min_y");
    if (minY.failed()) return minY.error();
    auto height = readInt(noiseObj, "height");
    if (height.failed()) return height.error();
    auto sizeH = readInt(noiseObj, "size_horizontal");
    if (sizeH.failed()) return sizeH.error();
    auto sizeV = readInt(noiseObj, "size_vertical");
    if (sizeV.failed()) return sizeV.error();
    // 用户决策：JSON 只提供 4 尺寸字段，scaling/slides/densityFactor/densityOffset/
    // simplexSurfaceNoise/randomDensityOffset/isAmplified/aquifersEnabled/useLegacyRandomSource
    // 全部由 dimensionKind 对应的 C++ 预设 noise 带入（上面基底已设），仅覆盖 4 尺寸。
    settings.noise.minY = minY.value();
    settings.noise.height = height.value();
    settings.noise.sizeHorizontal = sizeH.value();
    settings.noise.sizeVertical = sizeV.value();

    // 3. default_block / default_fluid（覆盖预设）
    if (root.contains("default_block")) {
        auto block = world::gen::feature::parser::BlockStateParser::parse(root["default_block"]);
        if (block.failed()) {
            return Error(
                ErrorCode::InvalidData, "noise_settings: failed to parse default_block: " + block.error().message());
        }
        settings.defaultBlock = block.value();
    }
    if (root.contains("default_fluid")) {
        auto fluid = world::gen::feature::parser::BlockStateParser::parse(root["default_fluid"]);
        if (fluid.failed()) {
            return Error(
                ErrorCode::InvalidData, "noise_settings: failed to parse default_fluid: " + fluid.error().message());
        }
        settings.defaultFluid = fluid.value();
    }

    // 4. 标量字段（覆盖预设；缺省保留预设值）
    if (root.contains("sea_level")) {
        auto seaLevel = readInt(root, "sea_level");
        if (seaLevel.failed()) return seaLevel.error();
        settings.seaLevel = seaLevel.value();
    }
    if (root.contains("disable_mob_generation")) {
        auto v = readBool(root, "disable_mob_generation");
        if (v.failed()) return v.error();
        settings.disableMobGeneration = v.value();
    }
    if (root.contains("ore_veins_enabled")) {
        auto v = readBool(root, "ore_veins_enabled");
        if (v.failed()) return v.error();
        settings.oreVeinsEnabled = v.value();
    }
    if (root.contains("aquifers_enabled")) {
        auto v = readBool(root, "aquifers_enabled");
        if (v.failed()) return v.error();
        settings.noise.aquifersEnabled = v.value();
    }
    if (root.contains("legacy_random_source")) {
        auto v = readBool(root, "legacy_random_source");
        if (v.failed()) return v.error();
        settings.noise.useLegacyRandomSource = v.value();
    }

    // 5. noise_router 15 字段（DF Holder，字符串 RL / 内联对象 / 裸数字；噪声叶子为 UnboundNoiseLeaf 占位）
    if (!root.contains("noise_router") || !root["noise_router"].is_object()) {
        return Error(ErrorCode::InvalidData, "noise_settings: missing 'noise_router' object");
    }
    const json& routerObj = root["noise_router"];
    for (const auto& field : kRouterFields) {
        if (!routerObj.contains(field.key)) {
            return Error(
                ErrorCode::InvalidData, "noise_settings: noise_router missing field '" + std::string(field.key) + "'");
        }
        auto df = world::gen::density::DensityFunctionLoader::resolveHolderElement(routerObj[field.key]);
        if (df.failed()) {
            return Error(ErrorCode::InvalidData,
                "noise_settings: failed to parse noise_router." + std::string(field.key) + ": " + df.error().message());
        }
        settings.m_routerDfs[static_cast<size_t>(field.slot)] =
            std::shared_ptr<world::gen::density::DensityFunction>(df.value().release());
    }

    // 6. surface_rule（数据驱动 SurfaceRule 树；RandomState::create 断言非空，所有 noise_settings 必须提供）
    if (root.contains("surface_rule")) {
        auto rule = world::gen::surface::SurfaceRuleDeserializer::fromJson(root["surface_rule"]);
        if (rule.failed()) {
            return Error(
                ErrorCode::InvalidData, "noise_settings: failed to parse surface_rule: " + rule.error().message());
        }
        settings.m_surfaceRule = std::shared_ptr<world::gen::surface::SurfaceRule>(rule.value().release());
    }

    // 7. spawn_target（数据驱动；覆盖预设 spawnTarget）
    if (root.contains("spawn_target")) {
        auto spawnTarget = world::biome::climate::ParameterPointCodec::fromJsonArray(root["spawn_target"]);
        if (spawnTarget.failed()) {
            return Error(ErrorCode::InvalidData,
                "noise_settings: failed to parse spawn_target: " + spawnTarget.error().message());
        }
        settings.spawnTarget = spawnTarget.value();
    }

    return settings;
}

} // namespace mc
