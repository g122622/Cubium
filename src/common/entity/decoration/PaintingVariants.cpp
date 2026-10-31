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

#include "PaintingVariants.hpp"

#include "common/resource/repository/DataPackRepository.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {

namespace {

/// 数据包里的画变体目录（相对 `data/<ns>/`）。
constexpr std::string_view PAINTING_VARIANT_DIRECTORY = "painting_variant";

/**
 * @brief 从文本组件里取翻译键
 *
 * 原版数据里 title/author 形如 `{ "color": "yellow", "translate": "painting.minecraft.kebab.title" }`。
 * 本类只保留 `translate` 的值：样式不参与玩法，翻译键才稳定可比。
 * 若组件是字面量（`{"text": "..."}`），退回取 text 内容。
 */
std::optional<std::string> componentKey(const nlohmann::json& component)
{
    if (component.is_string()) {
        return component.get<std::string>();
    }
    if (!component.is_object()) {
        return std::nullopt;
    }
    if (component.contains("translate") && component["translate"].is_string()) {
        return component["translate"].get<std::string>();
    }
    if (component.contains("text") && component["text"].is_string()) {
        return component["text"].get<std::string>();
    }
    return std::nullopt;
}

/**
 * @brief 把资源路径转成变体 ID
 *
 * `painting_variant/kebab.json` -> `minecraft:kebab`。
 */
std::optional<ResourceLocation> variantIdFromPath(const std::string& resourcePath)
{
    auto slash = resourcePath.rfind('/');
    const std::string fileName = slash == std::string::npos ? resourcePath : resourcePath.substr(slash + 1);
    if (fileName.size() <= 5 || fileName.substr(fileName.size() - 5) != ".json") {
        return std::nullopt;
    }
    const std::string name = fileName.substr(0, fileName.size() - 5);
    return ResourceLocation(name);
}

} // namespace

std::unordered_map<ResourceLocation, std::unique_ptr<PaintingVariant>>& PaintingVariants::_getVariants()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<PaintingVariant>> variants;
    return variants;
}

const PaintingVariant* PaintingVariants::get(const ResourceLocation& id)
{
    const auto& variants = _getVariants();
    auto it = variants.find(id);
    return it != variants.end() ? it->second.get() : nullptr;
}

std::size_t PaintingVariants::size()
{
    return _getVariants().size();
}

void PaintingVariants::forEachVariant(
    const std::function<void(const ResourceLocation&, const PaintingVariant&)>& callback)
{
    for (const auto& [id, variant] : _getVariants()) {
        callback(id, *variant);
    }
}

Result<std::size_t> PaintingVariants::loadFromDataPackRepository(const resource::DataPackRepository& dataPacks)
{
    auto namespacesResult = dataPacks.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return Error(ErrorCode::FileReadFailed,
            "painting_variant: 无法列出数据包命名空间：" + namespacesResult.error().toString());
    }

    auto& variants = _getVariants();
    variants.clear();
    std::size_t loaded = 0;

    // 与 GenericTagLoader 一致：资源路径必须带命名空间前缀（`<ns>/painting_variant`），
    // 只给目录名会一个文件都列不到。
    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/" + std::string(PAINTING_VARIANT_DIRECTORY);
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
                return Error(ErrorCode::InvalidData,
                    "painting_variant: 解析失败 " + resourcePath + "：" + std::string(e.what()));
            }

            const auto id = variantIdFromPath(resourcePath);
            if (!id.has_value()) {
                continue;
            }

            if (!json.contains("width") || !json.contains("height") || !json.contains("asset_id")) {
                return Error(ErrorCode::InvalidData, "painting_variant: " + resourcePath + " 缺少必填字段");
            }

            const i32 width = json["width"].get<i32>();
            const i32 height = json["height"].get<i32>();
            if (width <= 0 || height <= 0) {
                return Error(ErrorCode::InvalidData, "painting_variant: " + resourcePath + " 的宽高必须为正");
            }

            std::optional<std::string> titleKey;
            if (json.contains("title")) {
                titleKey = componentKey(json["title"]);
            }
            std::optional<std::string> authorKey;
            if (json.contains("author")) {
                authorKey = componentKey(json["author"]);
            }

            variants[*id] = std::make_unique<PaintingVariant>(
                width, height, ResourceLocation(json["asset_id"].get<std::string>()), titleKey, authorKey);
            ++loaded;
        }
    } // namespace 循环

    return loaded;
}

void PaintingVariants::clearAll()
{
    _getVariants().clear();
}

} // namespace mc
