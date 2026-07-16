/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
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

#include "NoiseLoader.hpp"

#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace noise {

namespace {

/**
 * @brief 从资源路径推导 ResourceLocation
 *
 * 路径格式: <namespace>/worldgen/noise/<path>.json
 * ResourceLocation = <namespace>:<path>（去 .json）
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

Result<size_t> NoiseLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    // 先清空硬编码兜底，再由数据包注入。完成后 markLoadedFromDatapack() 置位，
    // 使后续 Noises::get()/has() 跳过 initialize() 兜底（避免覆盖数据驱动值）。
    Noises::clear();
    Noises::markLoadedFromDatapack(false);

    size_t loadedCount = 0;

    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/worldgen/noise";
        auto listResult = dataPackList.listResources(directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = locationFromResourcePath(namespace_, directory, resourcePath);

            auto readResult = dataPackList.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read noise: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse noise {}: {}", location.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse noise {}: {}", location.toString(), parseResult.error().message());
                continue;
            }

            const auto& params = parseResult.value();
            Noises::registerNoise(location.toString(), params.firstOctave, params.amplitudes);
            ++loadedCount;
        }
    }

    Noises::markLoadedFromDatapack(true);
    spdlog::info("Loaded {} noise parameters from datapacks", loadedCount);
    return loadedCount;
}

Result<size_t> NoiseLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/worldgen/noise";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = locationFromResourcePath(namespace_, directory, resourcePath);

            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read noise: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse noise {}: {}", location.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse noise {}: {}", location.toString(), parseResult.error().message());
                continue;
            }

            const auto& params = parseResult.value();
            Noises::registerNoise(location.toString(), params.firstOctave, params.amplitudes);
            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<NoiseParameters> NoiseLoader::loadFromJson(const nlohmann::json& jsonObj, const ResourceLocation& location)
{
    // firstOctave（必填）
    if (!jsonObj.contains("firstOctave") || !jsonObj["firstOctave"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "noise '" + location.toString() + "' missing 'firstOctave' integer");
    }
    i32 firstOctave = jsonObj["firstOctave"].get<i32>();

    // amplitudes：MC 允许数组（标准）或单 double（简写，等价于 [value]）。
    std::vector<f64> amplitudes;
    if (!jsonObj.contains("amplitudes")) {
        return Error(ErrorCode::InvalidData, "noise '" + location.toString() + "' missing 'amplitudes'");
    }
    const auto& ampNode = jsonObj["amplitudes"];
    if (ampNode.is_array()) {
        amplitudes.reserve(ampNode.size());
        for (const auto& a : ampNode) {
            if (!a.is_number()) {
                return Error(
                    ErrorCode::InvalidData, "noise '" + location.toString() + "' amplitudes contains non-number");
            }
            amplitudes.push_back(a.get<f64>());
        }
    } else if (ampNode.is_number()) {
        // 简写：单个振幅 → 单元素数组
        amplitudes.push_back(ampNode.get<f64>());
    } else {
        return Error(ErrorCode::InvalidData, "noise '" + location.toString() + "' amplitudes must be array or number");
    }

    if (amplitudes.empty()) {
        return Error(ErrorCode::InvalidData, "noise '" + location.toString() + "' amplitudes is empty");
    }

    return NoiseParameters{firstOctave, std::move(amplitudes)};
}

} // namespace noise
} // namespace gen
} // namespace world
} // namespace mc
