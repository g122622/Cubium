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

#include "TrimPatterns.hpp"
#include "TrimPattern.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include <optional>
#include <utility>
#include <nlohmann/json.hpp>

namespace mc {

namespace {

constexpr const char* TRIM_PATTERN_DIRECTORY = "trim_pattern";

std::unordered_map<ResourceLocation, std::unique_ptr<TrimPattern>>& _getPatterns()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<TrimPattern>> patterns;
    return patterns;
}

/**
 * @brief 从资源路径推出图案 ID
 *
 * `listResourceStacks` 返回的键形如 `minecraft/coast.json`，去掉扩展名即为 ID。
 * 原版这里没有子目录，但加载器不假设这一点（取最后一段作为文件名）。
 */
std::optional<ResourceLocation> _patternIdFromPath(const std::string& resourcePath)
{
    auto slash = resourcePath.rfind('/');
    const std::string fileName = slash == std::string::npos ? resourcePath : resourcePath.substr(slash + 1);
    if (fileName.size() <= 5 || fileName.substr(fileName.size() - 5) != ".json") {
        return std::nullopt;
    }
    return ResourceLocation(fileName.substr(0, fileName.size() - 5));
}

} // namespace

const TrimPattern* TrimPatterns::get(const ResourceLocation& id)
{
    auto& patterns = _getPatterns();
    auto it = patterns.find(id);
    return it == patterns.end() ? nullptr : it->second.get();
}

const TrimPattern* TrimPatterns::get(const std::string& id)
{
    return get(ResourceLocation(id));
}

std::size_t TrimPatterns::size()
{
    return _getPatterns().size();
}

void TrimPatterns::forEachPattern(const std::function<void(const ResourceLocation&, const TrimPattern&)>& callback)
{
    for (const auto& [id, pattern] : _getPatterns()) {
        if (pattern) {
            callback(id, *pattern);
        }
    }
}

void TrimPatterns::clearAll()
{
    _getPatterns().clear();
}

Result<std::size_t> TrimPatterns::loadFromDataPackRepository(const resource::DataPackRepository& dataPacks)
{
    auto namespacesResult = dataPacks.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return Error(
            ErrorCode::FileReadFailed, "trim_pattern: 无法列出数据包命名空间：" + namespacesResult.error().toString());
    }

    auto& patterns = _getPatterns();
    patterns.clear();
    std::size_t loaded = 0;

    // 与 GenericTagLoader 一致：资源路径必须带命名空间前缀（`<ns>/trim_pattern`），
    // 只给目录名会一个文件都列不到。
    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/" + std::string(TRIM_PATTERN_DIRECTORY);
        auto stacksResult = dataPacks.listResourceStacks(directory, ".json");
        if (!stacksResult.success()) {
            continue;
        }

        for (const auto& [resourcePath, versions] : stacksResult.value()) {
            if (versions.empty()) {
                continue;
            }
            // 多数据包叠加时取最后一个（优先级最高）的版本，与原版「后者覆盖前者」一致。
            const auto& content = versions.back();

            nlohmann::json json;
            try {
                json = nlohmann::json::parse(content.content);
            }
            catch (const std::exception& e) {
                // 单个文件坏掉不应让整批加载失败，但必须报出来而不是静默跳过。
                return Error(
                    ErrorCode::InvalidData, "trim_pattern: 解析失败 " + resourcePath + "：" + std::string(e.what()));
            }

            const auto id = _patternIdFromPath(resourcePath);
            if (!id.has_value()) {
                continue;
            }

            if (!json.is_object()) {
                return Error(ErrorCode::InvalidData, "trim_pattern: 顶层不是对象 " + resourcePath);
            }

            const auto assetIdIt = json.find("asset_id");
            if (assetIdIt == json.end() || !assetIdIt->is_string()) {
                return Error(ErrorCode::InvalidData, "trim_pattern: 缺少 asset_id " + resourcePath);
            }

            const auto descIt = json.find("description");
            if (descIt == json.end() || !descIt->is_object()) {
                return Error(ErrorCode::InvalidData, "trim_pattern: 缺少 description " + resourcePath);
            }
            const auto translateIt = descIt->find("translate");
            if (translateIt == descIt->end() || !translateIt->is_string()) {
                return Error(ErrorCode::InvalidData, "trim_pattern: description 缺少 translate " + resourcePath);
            }

            const auto decalIt = json.find("decal");
            // 原版 `decal` 字段有默认值 false（OptionalFieldCodec），缺省时按 false 处理。
            const bool decal = decalIt != json.end() && decalIt->is_boolean() && decalIt->get<bool>();

            patterns[*id] = std::make_unique<TrimPattern>(
                ResourceLocation(assetIdIt->get<std::string>()), translateIt->get<std::string>(), decal);
            ++loaded;
        }
    }

    return loaded;
}

} // namespace mc
