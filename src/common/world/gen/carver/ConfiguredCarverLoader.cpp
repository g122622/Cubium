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

#include "ConfiguredCarverLoader.hpp"

#include "CanyonCarver.hpp"
#include "CarverConfiguration.hpp"
#include "CaveCarver.hpp"
#include "ConfiguredCarverRegistry.hpp"
#include "NetherWorldCarver.hpp"
#include "WorldCarver.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/valueprovider/FloatProviderParser.hpp"
#include "common/world/gen/valueprovider/HeightProviderParser.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world::gen::carver {

namespace {

namespace valueprovider = world::gen::valueprovider;
namespace surface = world::gen::surface;

/**
 * @brief 剥离 "minecraft:" 命名空间前缀
 */
std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/**
 * @brief 解析 replaceable 标签字符串（"#minecraft:xxx"）为 BlockTag*
 *
 * MC 用 "#namespace:path" 表示方块标签；剥 # 后按 ResourceLocation 查 BlockTags。
 */
const BlockTag* parseReplaceable(const nlohmann::json& json)
{
    if (!json.is_string()) {
        return nullptr;
    }
    std::string tag = json.get<std::string>();
    if (!tag.empty() && tag[0] == '#') {
        tag = tag.substr(1);
    }
    return BlockTags::getTag(ResourceLocation::parse(tag));
}

/**
 * @brief 读取 f32 字段（不存在返回默认值）
 */
f32 readF32(const nlohmann::json& obj, const std::string& key, f32 defaultValue)
{
    if (obj.contains(key) && obj[key].is_number()) {
        return obj[key].get<f32>();
    }
    return defaultValue;
}

/**
 * @brief 读取 i32 字段（不存在返回默认值）
 */
i32 readI32(const nlohmann::json& obj, const std::string& key, i32 defaultValue)
{
    if (obj.contains(key) && obj[key].is_number_integer()) {
        return obj[key].get<i32>();
    }
    return defaultValue;
}

/**
 * @brief 解析 FloatProvider 字段（字段不存在或解析失败时返回 Error）
 */
Result<std::unique_ptr<valueprovider::FloatProvider>> parseFloatField(
    const nlohmann::json& config, const std::string& key, const ResourceLocation& id)
{
    if (!config.contains(key)) {
        return Error(ErrorCode::InvalidData,
            "configured_carver '" + id.toString() + "' missing FloatProvider field '" + key + "'");
    }
    auto result = valueprovider::FloatProviderParser::parse(config[key]);
    if (!result.success()) {
        return Error(result.error().code(),
            "configured_carver '" + id.toString() + "' field '" + key + "': " + result.error().message());
    }
    return result;
}

/**
 * @brief 解析 HeightProvider 字段（y）
 */
Result<std::unique_ptr<valueprovider::HeightProvider>> parseHeightField(
    const nlohmann::json& config, const ResourceLocation& id)
{
    if (!config.contains("y") || !config["y"].is_object()) {
        return Error(ErrorCode::InvalidData, "configured_carver '" + id.toString() + "' missing 'y' object");
    }
    auto result = valueprovider::HeightProviderParser::parse(config["y"]);
    if (!result.success()) {
        return Error(
            result.error().code(), "configured_carver '" + id.toString() + "' field 'y': " + result.error().message());
    }
    return result;
}

/**
 * @brief 解析 lava_level（VerticalAnchor）
 */
Result<surface::VerticalAnchor> parseLavaLevel(const nlohmann::json& config, const ResourceLocation& id)
{
    if (!config.contains("lava_level") || !config["lava_level"].is_object()) {
        return Error(ErrorCode::InvalidData, "configured_carver '" + id.toString() + "' missing 'lava_level' object");
    }
    auto result = valueprovider::HeightProviderParser::parseAnchor(config["lava_level"]);
    if (!result.success()) {
        return Error(result.error().code(),
            "configured_carver '" + id.toString() + "' field 'lava_level': " + result.error().message());
    }
    return result;
}

/**
 * @brief 解析 CanyonShapeConfiguration（canyon 的 shape 子对象）
 */
Result<CanyonShapeConfiguration> parseCanyonShape(const nlohmann::json& shapeJson, const ResourceLocation& id)
{
    if (!shapeJson.is_object()) {
        return Error(ErrorCode::InvalidData, "configured_carver '" + id.toString() + "' 'shape' must be an object");
    }

    auto distanceFactor = parseFloatField(shapeJson, "distance_factor", id);
    if (!distanceFactor.success()) {
        return Error(distanceFactor.error().code(), distanceFactor.error().message());
    }
    auto thickness = parseFloatField(shapeJson, "thickness", id);
    if (!thickness.success()) {
        return Error(thickness.error().code(), thickness.error().message());
    }
    auto hRadiusFactor = parseFloatField(shapeJson, "horizontal_radius_factor", id);
    if (!hRadiusFactor.success()) {
        return Error(hRadiusFactor.error().code(), hRadiusFactor.error().message());
    }

    const i32 widthSmoothness = readI32(shapeJson, "width_smoothness", 3);
    const f32 vDefaultFactor = readF32(shapeJson, "vertical_radius_default_factor", 1.0f);
    const f32 vCenterFactor = readF32(shapeJson, "vertical_radius_center_factor", 0.0f);

    return CanyonShapeConfiguration(distanceFactor.value(),
        thickness.value(),
        widthSmoothness,
        hRadiusFactor.value(),
        vDefaultFactor,
        vCenterFactor);
}

/**
 * @brief 解析 cave/cave_extra_underground/nether_cave 共用的 CaveCarverConfiguration
 */
Result<CaveCarverConfiguration> parseCaveConfig(
    const nlohmann::json& config, const ResourceLocation& id, const BlockTag* replaceable)
{
    const f32 probability = readF32(config, "probability", 0.0f);

    auto y = parseHeightField(config, id);
    if (!y.success()) {
        return Error(y.error().code(), y.error().message());
    }
    auto yScale = parseFloatField(config, "yScale", id);
    if (!yScale.success()) {
        return Error(yScale.error().code(), yScale.error().message());
    }
    auto lavaLevel = parseLavaLevel(config, id);
    if (!lavaLevel.success()) {
        return Error(lavaLevel.error().code(), lavaLevel.error().message());
    }
    auto hRadius = parseFloatField(config, "horizontal_radius_multiplier", id);
    if (!hRadius.success()) {
        return Error(hRadius.error().code(), hRadius.error().message());
    }
    auto vRadius = parseFloatField(config, "vertical_radius_multiplier", id);
    if (!vRadius.success()) {
        return Error(vRadius.error().code(), vRadius.error().message());
    }
    auto floorLevel = parseFloatField(config, "floor_level", id);
    if (!floorLevel.success()) {
        return Error(floorLevel.error().code(), floorLevel.error().message());
    }

    return CaveCarverConfiguration(probability,
        y.value(),
        yScale.value(),
        lavaLevel.value(),
        replaceable,
        hRadius.value(),
        vRadius.value(),
        floorLevel.value());
}

/**
 * @brief 解析 canyon 专用的 CanyonCarverConfiguration
 */
Result<CanyonCarverConfiguration> parseCanyonConfig(
    const nlohmann::json& config, const ResourceLocation& id, const BlockTag* replaceable)
{
    const f32 probability = readF32(config, "probability", 0.0f);

    auto y = parseHeightField(config, id);
    if (!y.success()) {
        return Error(y.error().code(), y.error().message());
    }
    auto yScale = parseFloatField(config, "yScale", id);
    if (!yScale.success()) {
        return Error(yScale.error().code(), yScale.error().message());
    }
    auto lavaLevel = parseLavaLevel(config, id);
    if (!lavaLevel.success()) {
        return Error(lavaLevel.error().code(), lavaLevel.error().message());
    }
    auto vRotation = parseFloatField(config, "vertical_rotation", id);
    if (!vRotation.success()) {
        return Error(vRotation.error().code(), vRotation.error().message());
    }

    if (!config.contains("shape") || !config["shape"].is_object()) {
        return Error(ErrorCode::InvalidData, "configured_carver '" + id.toString() + "' missing 'shape' object");
    }
    auto shape = parseCanyonShape(config["shape"], id);
    if (!shape.success()) {
        return Error(shape.error().code(), shape.error().message());
    }

    return CanyonCarverConfiguration(probability,
        y.value(),
        yScale.value(),
        lavaLevel.value(),
        replaceable,
        vRotation.value(),
        std::move(shape.value()));
}

/**
 * @brief 从资源路径推导 ResourceLocation
 */
ResourceLocation locationFromResourcePath(const std::string& ns, const std::string& directory, const std::string& path)
{
    std::string name = path.substr(directory.length() + 1); // +1 跳过 '/'
    if (name.size() >= 5 && name.substr(name.size() - 5) == ".json") {
        name = name.substr(0, name.size() - 5);
    }
    return ResourceLocation(ns, name);
}

} // namespace

Result<std::unique_ptr<ConfiguredCarverBase>> ConfiguredCarverLoader::loadFromJson(
    const nlohmann::json& jsonObj, const ResourceLocation& id)
{
    if (!jsonObj.contains("type") || !jsonObj["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "configured_carver '" + id.toString() + "' missing 'type' string");
    }
    const std::string type = stripNamespace(jsonObj["type"].get<std::string>());

    const nlohmann::json& config = jsonObj.contains("config") ? jsonObj["config"] : nlohmann::json::object();
    if (!config.is_object()) {
        return Error(ErrorCode::InvalidData, "configured_carver '" + id.toString() + "' 'config' must be an object");
    }

    // replaceable 标签
    const BlockTag* replaceable = nullptr;
    if (config.contains("replaceable")) {
        replaceable = parseReplaceable(config["replaceable"]);
        if (replaceable == nullptr) {
            spdlog::warn("configured_carver '{}' references unknown replaceable tag", id.toString());
        }
    }

    if (type == "cave" || type == "nether_cave") {
        auto cfg = parseCaveConfig(config, id, replaceable);
        if (!cfg.success()) {
            return Error(cfg.error().code(), cfg.error().message());
        }
        if (type == "nether_cave") {
            std::unique_ptr<ConfiguredCarverBase> carver =
                std::make_unique<ConfiguredCarver<NetherWorldCarver, CaveCarverConfiguration>>(
                    std::make_unique<NetherWorldCarver>(), std::move(cfg.value()));
            return carver;
        }
        std::unique_ptr<ConfiguredCarverBase> carver =
            std::make_unique<ConfiguredCarver<CaveCarver, CaveCarverConfiguration>>(
                std::make_unique<CaveCarver>(), std::move(cfg.value()));
        return carver;
    }

    if (type == "canyon") {
        auto cfg = parseCanyonConfig(config, id, replaceable);
        if (!cfg.success()) {
            return Error(cfg.error().code(), cfg.error().message());
        }
        std::unique_ptr<ConfiguredCarverBase> carver =
            std::make_unique<ConfiguredCarver<CanyonCarver, CanyonCarverConfiguration>>(
                std::make_unique<CanyonCarver>(), std::move(cfg.value()));
        return carver;
    }

    return Error(ErrorCode::NotFound,
        "Unregistered configured_carver type: 'minecraft:" + type +
            "'. This carver type has no C++ implementation yet.");
}

Result<size_t> ConfiguredCarverLoader::loadFromDataPackRepository(const resource::DataPackRepository& repo)
{
    size_t loadedCount = 0;
    // 清空旧数据，保证重复加载不累积重复条目
    ConfiguredCarverRegistry::instance().clear();

    auto namespacesResult = repo.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/worldgen/configured_carver";
        auto listResult = repo.listResources(directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation id = locationFromResourcePath(ns, directory, resourcePath);

            auto readResult = repo.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read configured_carver: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse configured_carver {}: {}", id.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, id);
            if (!parseResult.success()) {
                spdlog::warn("Failed to load configured_carver {}: {}", id.toString(), parseResult.error().message());
                continue;
            }

            ConfiguredCarverRegistry::instance().registerCarver(parseResult.value(), id);
            ++loadedCount;
        }
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} configured_carvers from datapacks", loadedCount);
    }
    return loadedCount;
}

Result<size_t> ConfiguredCarverLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;
    // 清空旧数据，保证重复加载不累积重复条目
    ConfiguredCarverRegistry::instance().clear();

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/worldgen/configured_carver";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation id = locationFromResourcePath(ns, directory, resourcePath);

            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read configured_carver: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse configured_carver {}: {}", id.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, id);
            if (!parseResult.success()) {
                spdlog::warn("Failed to load configured_carver {}: {}", id.toString(), parseResult.error().message());
                continue;
            }

            ConfiguredCarverRegistry::instance().registerCarver(parseResult.value(), id);
            ++loadedCount;
        }
    }

    return loadedCount;
}

} // namespace world::gen::carver
} // namespace mc
