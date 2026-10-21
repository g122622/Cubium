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

#include "InstrumentLoader.hpp"

#include "common/core/Types.hpp"
#include "common/item/instrument/InstrumentRegistry.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/text/ITextComponent.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::server::item {
namespace {

/// 乐器定义所在目录（相对 data/<namespace>/）
constexpr std::string_view INSTRUMENT_DIRECTORY = "instrument";

/**
 * @brief 解析单个乐器 JSON 对象
 *
 * @param jsonObj 已解析的 JSON 对象
 * @param location 乐器资源位置
 * @return 乐器定义，或错误
 */
Result<std::unique_ptr<mc::item::instrument::InstrumentDefinition>> _parseInstrument(
    const nlohmann::json& jsonObj, const ResourceLocation& location)
{
    if (!jsonObj.is_object()) {
        return Error(ErrorCode::InvalidData, "instrument '" + location.toString() + "' is not an object");
    }
    for (const char* field : {"sound_event", "use_duration", "range", "description"}) {
        if (!jsonObj.contains(field)) {
            return Error(ErrorCode::InvalidData,
                "instrument '" + location.toString() + "' missing required field '" + field + "'");
        }
    }

    if (!jsonObj["sound_event"].is_string()) {
        return Error(ErrorCode::InvalidData, "instrument '" + location.toString() + "': sound_event must be a string");
    }
    const ResourceLocation soundEvent(jsonObj["sound_event"].get<std::string>());

    if (!jsonObj["use_duration"].is_number() || !jsonObj["range"].is_number()) {
        return Error(
            ErrorCode::InvalidData, "instrument '" + location.toString() + "': use_duration/range must be numbers");
    }
    const f32 useDuration = jsonObj["use_duration"].get<f32>();
    const f32 range = jsonObj["range"].get<f32>();
    if (!(useDuration > 0.0f)) {
        return Error(ErrorCode::InvalidData, "instrument '" + location.toString() + "': use_duration must be positive");
    }
    if (!(range > 0.0f)) {
        return Error(ErrorCode::InvalidData, "instrument '" + location.toString() + "': range must be positive");
    }

    std::unique_ptr<text::ITextComponent> description = text::ITextComponent::fromJson(jsonObj["description"]);
    if (description == nullptr) {
        return Error(ErrorCode::InvalidData, "instrument '" + location.toString() + "': invalid description");
    }

    return std::make_unique<mc::item::instrument::InstrumentDefinition>(
        soundEvent, useDuration, range, std::move(description));
}

/**
 * @brief 从一份资源列表加载全部乐器定义
 *
 * @param namespace_ 命名空间
 * @param resourcePaths 相对 <namespace>/ 的资源路径列表（形如 "instrument/<path>.json"）
 * @param readTextResource 读取文本资源的回调，失败返回空 optional
 * @return 成功注册的乐器数量
 */
template <typename ReadFn>
size_t _loadFromResourcePaths(
    std::string_view namespace_, const std::vector<std::string>& resourcePaths, ReadFn&& readTextResource)
{
    size_t loadedCount = 0;
    for (const auto& resourcePath : resourcePaths) {
        // 路径格式: instrument/<path>.json
        std::string pathName(resourcePath.substr(INSTRUMENT_DIRECTORY.size() + 1));
        pathName = pathName.substr(0, pathName.size() - 5);
        const ResourceLocation location(std::string(namespace_) + ":" + pathName);

        auto readResult = readTextResource(resourcePath);
        if (!readResult.success()) {
            spdlog::warn("Failed to read instrument: {}", resourcePath);
            continue;
        }

        std::unique_ptr<mc::item::instrument::InstrumentDefinition> instrument;
        try {
            const nlohmann::json jsonObj = nlohmann::json::parse(readResult.value());
            auto parseResult = _parseInstrument(jsonObj, location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse instrument {}: {}", location.toString(), parseResult.error().message());
                continue;
            }
            instrument = parseResult.value();
        }
        catch (const std::exception& e) {
            spdlog::warn("Failed to parse instrument {}: {}", location.toString(), e.what());
            continue;
        }

        mc::item::instrument::InstrumentRegistry::instance().registerInstrument(location, std::move(instrument));
        ++loadedCount;
    }
    return loadedCount;
}

} // namespace

Result<size_t> InstrumentLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    size_t loadedCount = 0;
    const auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        const std::string directory = namespace_ + "/" + std::string(INSTRUMENT_DIRECTORY);
        const auto listResult = dataPackList.listResources(directory, ".json");
        if (!listResult.success()) {
            continue;
        }
        loadedCount += _loadFromResourcePaths(namespace_, listResult.value(), [&dataPackList](const std::string& path) {
            return dataPackList.readTextResource(path);
        });
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} instruments from datapacks", loadedCount);
    }
    return loadedCount;
}

Result<size_t> InstrumentLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;
    const auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        const std::string directory = namespace_ + "/" + std::string(INSTRUMENT_DIRECTORY);
        const auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }
        loadedCount += _loadFromResourcePaths(namespace_, listResult.value(), [&pack](const std::string& path) {
            return pack.readTextResource(resource::PackType::ServerData, path);
        });
    }
    return loadedCount;
}

Result<std::unique_ptr<mc::item::instrument::InstrumentDefinition>> InstrumentLoader::loadFromJson(
    const nlohmann::json& jsonObj, const ResourceLocation& location)
{
    return _parseInstrument(jsonObj, location);
}

} // namespace mc::server::item