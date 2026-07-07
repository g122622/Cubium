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

#include "ConfiguredFeatureLoader.hpp"

#include "ConfiguredFeature.hpp"
#include "ConfiguredFeatureRegistry.hpp"
#include "FeatureTypeRegistry.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world::gen::feature {

namespace {

/**
 * @brief 从资源路径推导 ResourceLocation
 *
 * 路径格式: <namespace>/worldgen/configured_feature/<path>.json
 * ResourceLocation = <namespace>:<path>（去 .json）
 */
ResourceLocation locationFromResourcePath(const std::string& ns, const std::string& directory, const std::string& path)
{
    // path 形如 "minecraft/worldgen/configured_feature/monster_room.json"
    // 取 directory 之后的部分作为 name，再去掉 .json
    std::string name = path.substr(directory.length() + 1); // +1 跳过 '/'
    if (name.size() >= 5 && name.substr(name.size() - 5) == ".json") {
        name = name.substr(0, name.size() - 5);
    }
    return ResourceLocation(ns, name);
}

} // namespace

Result<size_t> ConfiguredFeatureLoader::loadFromDataPackRepository(const resource::DataPackRepository& repo)
{
    size_t loadedCount = 0;
    // 清空旧数据，保证重复加载（重启世界/测试 fixture 复用）不累积重复条目
    ConfiguredFeatureRegistry::instance().clear();

    auto namespacesResult = repo.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/worldgen/configured_feature";
        auto listResult = repo.listResources(directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation id = locationFromResourcePath(ns, directory, resourcePath);

            auto readResult = repo.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read configured_feature: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse configured_feature {}: {}", id.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, id);
            if (!parseResult.success()) {
                spdlog::warn("Failed to load configured_feature {}: {}", id.toString(), parseResult.error().message());
                continue;
            }

            ConfiguredFeatureRegistry::instance().registerFeature(parseResult.value(), id);
            ++loadedCount;
        }
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} configured_features from datapacks", loadedCount);
    }
    return loadedCount;
}

Result<size_t> ConfiguredFeatureLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;
    // 清空旧数据，保证重复加载不累积重复条目
    ConfiguredFeatureRegistry::instance().clear();

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/worldgen/configured_feature";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation id = locationFromResourcePath(ns, directory, resourcePath);

            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read configured_feature: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse configured_feature {}: {}", id.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, id);
            if (!parseResult.success()) {
                spdlog::warn("Failed to load configured_feature {}: {}", id.toString(), parseResult.error().message());
                continue;
            }

            ConfiguredFeatureRegistry::instance().registerFeature(parseResult.value(), id);
            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<std::unique_ptr<ConfiguredFeatureBase>> ConfiguredFeatureLoader::loadFromJson(
    const nlohmann::json& jsonObj, const ResourceLocation& id)
{
    if (!jsonObj.contains("type") || !jsonObj["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "configured_feature '" + id.toString() + "' missing 'type' string");
    }

    const std::string type = jsonObj["type"].get<std::string>();

    // config 可选（如 monster_room 的 config 为空对象）
    const nlohmann::json& configJson = jsonObj.contains("config") ? jsonObj["config"] : nlohmann::json::object();

    auto createResult = FeatureTypeRegistry::instance().create(type, configJson);
    if (!createResult.success()) {
        return Error(createResult.error().code(), createResult.error().message());
    }

    auto feature = createResult.value();
    feature->setId(id);
    return feature;
}

} // namespace world::gen::feature
} // namespace mc
