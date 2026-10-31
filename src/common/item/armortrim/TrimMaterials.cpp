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

#include "TrimMaterials.hpp"
#include "TrimMaterial.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include <optional>
#include <utility>
#include <nlohmann/json.hpp>

namespace mc {

namespace {

constexpr const char* TRIM_MATERIAL_DIRECTORY = "trim_material";

std::unordered_map<ResourceLocation, std::unique_ptr<TrimMaterial>>& _getMaterials()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<TrimMaterial>> materials;
    return materials;
}

std::optional<ResourceLocation> _materialIdFromPath(const std::string& resourcePath)
{
    auto slash = resourcePath.rfind('/');
    const std::string fileName = slash == std::string::npos ? resourcePath : resourcePath.substr(slash + 1);
    if (fileName.size() <= 5 || fileName.substr(fileName.size() - 5) != ".json") {
        return std::nullopt;
    }
    return ResourceLocation(fileName.substr(0, fileName.size() - 5));
}

} // namespace

const TrimMaterial* TrimMaterials::get(const ResourceLocation& id)
{
    auto& materials = _getMaterials();
    auto it = materials.find(id);
    return it == materials.end() ? nullptr : it->second.get();
}

const TrimMaterial* TrimMaterials::get(const std::string& id)
{
    return get(ResourceLocation(id));
}

std::size_t TrimMaterials::size()
{
    return _getMaterials().size();
}

void TrimMaterials::forEachMaterial(const std::function<void(const ResourceLocation&, const TrimMaterial&)>& callback)
{
    for (const auto& [id, material] : _getMaterials()) {
        if (material) {
            callback(id, *material);
        }
    }
}

void TrimMaterials::clearAll()
{
    _getMaterials().clear();
}

Result<std::size_t> TrimMaterials::loadFromDataPackRepository(const resource::DataPackRepository& dataPacks)
{
    auto namespacesResult = dataPacks.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return Error(
            ErrorCode::FileReadFailed, "trim_material: 无法列出数据包命名空间：" + namespacesResult.error().toString());
    }

    auto& materials = _getMaterials();
    materials.clear();
    std::size_t loaded = 0;

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/" + std::string(TRIM_MATERIAL_DIRECTORY);
        auto stacksResult = dataPacks.listResourceStacks(directory, ".json");
        if (!stacksResult.success()) {
            continue;
        }

        for (const auto& [resourcePath, versions] : stacksResult.value()) {
            if (versions.empty()) {
                continue;
            }
            const auto& content = versions.back();

            nlohmann::json json;
            try {
                json = nlohmann::json::parse(content.content);
            }
            catch (const std::exception& e) {
                return Error(
                    ErrorCode::InvalidData, "trim_material: 解析失败 " + resourcePath + "：" + std::string(e.what()));
            }

            const auto id = _materialIdFromPath(resourcePath);
            if (!id.has_value()) {
                continue;
            }

            if (!json.is_object()) {
                return Error(ErrorCode::InvalidData, "trim_material: 顶层不是对象 " + resourcePath);
            }

            const auto assetNameIt = json.find("asset_name");
            if (assetNameIt == json.end() || !assetNameIt->is_string()) {
                return Error(ErrorCode::InvalidData, "trim_material: 缺少 asset_name " + resourcePath);
            }

            const auto descIt = json.find("description");
            if (descIt == json.end() || !descIt->is_object()) {
                return Error(ErrorCode::InvalidData, "trim_material: 缺少 description " + resourcePath);
            }
            const auto translateIt = descIt->find("translate");
            if (translateIt == descIt->end() || !translateIt->is_string()) {
                return Error(ErrorCode::InvalidData, "trim_material: description 缺少 translate " + resourcePath);
            }
            const auto colorIt = descIt->find("color");
            const std::string color =
                (colorIt != descIt->end() && colorIt->is_string()) ? colorIt->get<std::string>() : "";

            std::unordered_map<ResourceLocation, std::string> overrides;
            const auto overridesIt = json.find("override_armor_assets");
            // 原版 `override_armor_assets` 是 optionalFieldOf(..., Map.of())，缺省即空表。
            if (overridesIt != json.end() && overridesIt->is_object()) {
                for (const auto& [key, value] : overridesIt->items()) {
                    if (value.is_string()) {
                        overrides.emplace(ResourceLocation(key), value.get<std::string>());
                    }
                }
            }

            materials[*id] = std::make_unique<TrimMaterial>(
                assetNameIt->get<std::string>(), translateIt->get<std::string>(), color, std::move(overrides));
            ++loaded;
        }
    }

    return loaded;
}

} // namespace mc
