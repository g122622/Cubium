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
 */

#include "WorldPreset.hpp"

#include "common/util/assert/AssertAll.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <string>

namespace mc {
namespace world {
namespace gen {
namespace settings {

namespace {

using json = nlohmann::json;

/// 解析 biome_source 对象 → BiomeSourceType + 相关 RL 字段。
///   multi_noise: { type:"minecraft:multi_noise", preset:"minecraft:overworld"|"minecraft:nether" }
///   the_end:     { type:"minecraft:the_end" }
///   fixed:       { type:"minecraft:fixed", biome:"minecraft:plains" }
Result<WorldPresetGenerator> parseBiomeSource(const json& biomeSource, const ResourceLocation& id)
{
    WorldPresetGenerator gen;
    if (!biomeSource.is_object() || !biomeSource.contains("type") || !biomeSource["type"].is_string()) {
        return Error(
            ErrorCode::InvalidData, "world_preset '" + id.toString() + "': biome_source missing 'type' string");
    }
    const std::string bsType = biomeSource["type"].get<std::string>();
    // 去掉 "minecraft:" 前缀做类型分发（兼容自定义命名空间下的同名 type）
    const std::string bsPath = bsType.find(':') != std::string::npos ? bsType.substr(bsType.find(':') + 1) : bsType;

    if (bsPath == "multi_noise") {
        gen.biomeSourceType = WorldPresetGenerator::BiomeSourceType::MultiNoise;
        if (!biomeSource.contains("preset") || !biomeSource["preset"].is_string()) {
            return Error(ErrorCode::InvalidData,
                "world_preset '" + id.toString() + "': multi_noise biome_source missing 'preset' string");
        }
        gen.multiNoisePreset = ResourceLocation::parse(biomeSource["preset"].get<std::string>());
    } else if (bsPath == "the_end") {
        gen.biomeSourceType = WorldPresetGenerator::BiomeSourceType::TheEnd;
    } else if (bsPath == "fixed") {
        gen.biomeSourceType = WorldPresetGenerator::BiomeSourceType::Fixed;
        if (!biomeSource.contains("biome") || !biomeSource["biome"].is_string()) {
            return Error(ErrorCode::InvalidData,
                "world_preset '" + id.toString() + "': fixed biome_source missing 'biome' string");
        }
        gen.fixedBiome = ResourceLocation::parse(biomeSource["biome"].get<std::string>());
    } else {
        return Error(ErrorCode::InvalidData,
            "world_preset '" + id.toString() + "': unsupported biome_source type '" + bsType + "'");
    }
    return gen;
}

/// 解析 generator 对象：type + biome_source + settings（settings 形态随 type 变化）
Result<WorldPresetGenerator> parseGenerator(const json& generator, const ResourceLocation& id)
{
    if (!generator.is_object() || !generator.contains("type") || !generator["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "world_preset '" + id.toString() + "': generator missing 'type' string");
    }
    const std::string genType = generator["type"].get<std::string>();
    const std::string genPath =
        genType.find(':') != std::string::npos ? genType.substr(genType.find(':') + 1) : genType;

    WorldPresetGenerator gen;
    if (genPath == "noise") {
        gen.type = WorldPresetGenerator::Type::Noise;
        // biome_source（noise 生成器必有）
        if (!generator.contains("biome_source") || !generator["biome_source"].is_object()) {
            return Error(ErrorCode::InvalidData,
                "world_preset '" + id.toString() + "': noise generator missing 'biome_source' object");
        }
        auto bsResult = parseBiomeSource(generator["biome_source"], id);
        if (bsResult.failed()) return bsResult.error();
        gen = bsResult.value();
        gen.type = WorldPresetGenerator::Type::Noise;
        // settings = noise_settings 资源位置（字符串 RL）
        if (!generator.contains("settings") || !generator["settings"].is_string()) {
            return Error(ErrorCode::InvalidData,
                "world_preset '" + id.toString() + "': noise generator missing 'settings' string RL");
        }
        gen.noiseSettings = ResourceLocation::parse(generator["settings"].get<std::string>());
    } else if (genPath == "flat") {
        gen.type = WorldPresetGenerator::Type::Flat;
        // settings 是内联对象（{biome, layers, features, lakes, structure_overrides}），无 display 包装层
        if (!generator.contains("settings") || !generator["settings"].is_object()) {
            return Error(ErrorCode::InvalidData,
                "world_preset '" + id.toString() + "': flat generator missing 'settings' object");
        }
        auto flatResult = FlatLevelGeneratorSettings::fromSettingsObject(generator["settings"], id);
        if (flatResult.failed()) return flatResult.error();
        gen.flatSettings = flatResult.value();
    } else if (genPath == "debug") {
        gen.type = WorldPresetGenerator::Type::Debug;
        // debug 生成器无 biome_source / settings
    } else {
        return Error(ErrorCode::InvalidData,
            "world_preset '" + id.toString() + "': unsupported generator type '" + genType + "'");
    }
    return gen;
}

} // namespace

Result<WorldPreset> WorldPreset::fromJson(const json& root, const ResourceLocation& id)
{
    if (!root.is_object() || !root.contains("dimensions") || !root["dimensions"].is_object()) {
        return Error(ErrorCode::InvalidData, "world_preset '" + id.toString() + "' missing 'dimensions' object");
    }

    WorldPreset preset;
    for (const auto& [dimKey, dimJson] : root["dimensions"].items()) {
        const ResourceLocation dimRl = ResourceLocation::parse(dimKey);
        if (!dimJson.is_object()) {
            return Error(ErrorCode::InvalidData,
                "world_preset '" + id.toString() + "': dimension '" + dimKey + "' is not an object");
        }
        WorldPresetDimension dim;
        // type = dimension_type 资源位置（minecraft:overworld/the_nether/the_end）
        if (!dimJson.contains("type") || !dimJson["type"].is_string()) {
            return Error(ErrorCode::InvalidData,
                "world_preset '" + id.toString() + "': dimension '" + dimKey + "' missing 'type' string");
        }
        dim.dimensionType = ResourceLocation::parse(dimJson["type"].get<std::string>());

        // generator
        if (!dimJson.contains("generator") || !dimJson["generator"].is_object()) {
            return Error(ErrorCode::InvalidData,
                "world_preset '" + id.toString() + "': dimension '" + dimKey + "' missing 'generator' object");
        }
        auto genResult = parseGenerator(dimJson["generator"], id);
        if (genResult.failed()) return genResult.error();
        dim.generator = genResult.value();

        preset.dimensions.emplace(dimRl, std::move(dim));
    }
    return preset;
}

} // namespace settings
} // namespace gen
} // namespace world
} // namespace mc
