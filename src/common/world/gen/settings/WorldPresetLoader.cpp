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

#include "common/world/gen/settings/WorldPresetLoader.hpp"

#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/gen/settings/WorldPreset.hpp"
#include "common/world/gen/settings/WorldPresetRegistry.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <string>

namespace mc {
namespace world {
namespace gen {
namespace settings {

namespace {

using json = nlohmann::json;

/// 从资源路径推导 ResourceLocation（路径格式 <ns>/worldgen/world_preset/<path>.json）
ResourceLocation locationFromResourcePath(const std::string& ns, const std::string& directory, const std::string& path)
{
    std::string name = path.substr(directory.length() + 1); // +1 跳过 '/'
    if (name.size() >= 5 && name.substr(name.size() - 5) == ".json") {
        name = name.substr(0, name.size() - 5);
    }
    return ResourceLocation(ns, name);
}

} // namespace

Result<size_t> WorldPresetLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    WorldPresetRegistry::instance().clear();
    WorldPresetRegistry::instance().markLoadedFromDatapack(false);
    size_t loadedCount = 0;

    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/worldgen/world_preset";
        auto listResult = dataPackList.listResources(directory, ".json");
        if (!listResult.success()) {
            continue;
        }
        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = locationFromResourcePath(namespace_, directory, resourcePath);
            auto readResult = dataPackList.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read world_preset: {}", resourcePath);
                continue;
            }
            json jsonObj;
            try {
                jsonObj = json::parse(readResult.value());
            }
            catch (const json::parse_error& e) {
                spdlog::warn("Failed to parse world_preset {}: {}", location.toString(), e.what());
                continue;
            }
            auto presetResult = WorldPreset::fromJson(jsonObj, location);
            if (presetResult.failed()) {
                spdlog::warn(
                    "Failed to load world_preset '{}': {}", location.toString(), presetResult.error().message());
                continue;
            }
            WorldPresetRegistry::instance().registerPreset(location, presetResult.value());
            ++loadedCount;
        }
    }

    WorldPresetRegistry::instance().markLoadedFromDatapack(true);
    spdlog::info("Loaded {} world_presets from datapacks", loadedCount);
    return loadedCount;
}

Result<size_t> WorldPresetLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/worldgen/world_preset";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }
        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = locationFromResourcePath(namespace_, directory, resourcePath);
            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read world_preset: {}", resourcePath);
                continue;
            }
            json jsonObj;
            try {
                jsonObj = json::parse(readResult.value());
            }
            catch (const json::parse_error& e) {
                spdlog::warn("Failed to parse world_preset {}: {}", location.toString(), e.what());
                continue;
            }
            auto presetResult = WorldPreset::fromJson(jsonObj, location);
            if (presetResult.failed()) {
                spdlog::warn(
                    "Failed to load world_preset '{}': {}", location.toString(), presetResult.error().message());
                continue;
            }
            WorldPresetRegistry::instance().registerPreset(location, presetResult.value());
            ++loadedCount;
        }
    }

    return loadedCount;
}

} // namespace settings
} // namespace gen
} // namespace world
} // namespace mc
