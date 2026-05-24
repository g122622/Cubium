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

#include "TemplatePoolLoader.hpp"

#include "../../../resource/DataPackList.hpp"
#include "../../../resource/IResourcePack.hpp"
#include "../../../resource/PackType.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "JigsawPattern.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

Result<size_t> TemplatePoolLoader::loadFromDataPackList(const resource::DataPackList& dataPackList)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 template_pool 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/worldgen/template_pool";
        auto listResult = dataPackList.listResources(directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取模板池名称
            // 路径格式: namespace/worldgen/template_pool/path/to/pool.json
            std::string poolName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            // 移除 .json 扩展名
            if (poolName.size() >= 5 && poolName.substr(poolName.size() - 5) == ".json") {
                poolName = poolName.substr(0, poolName.size() - 5);
            }

            ResourceLocation location(poolName);

            // 读取 JSON 内容
            auto readResult = dataPackList.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read template pool: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse template pool {}: {}", poolName, parseResult.error().message());
                continue;
            }

            // 注册模板池
            JigsawPatternRegistry::instance().registerPattern(parseResult.value());
            ++loadedCount;
        }
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} template pools from datapacks", loadedCount);
    }

    return loadedCount;
}

Result<size_t> TemplatePoolLoader::loadFromResourcePack(const IResourcePack& pack)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 template_pool 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/worldgen/template_pool";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取模板池名称
            std::string poolName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            if (poolName.size() >= 5 && poolName.substr(poolName.size() - 5) == ".json") {
                poolName = poolName.substr(0, poolName.size() - 5);
            }

            ResourceLocation location(poolName);

            // 读取 JSON 内容
            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read template pool: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse template pool {}: {}", poolName, parseResult.error().message());
                continue;
            }

            // 注册模板池
            JigsawPatternRegistry::instance().registerPattern(parseResult.value());
            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<std::unique_ptr<JigsawPattern>> TemplatePoolLoader::loadFromJson(
    const std::string& json, const ResourceLocation& location)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);
        return loadFromJson(jsonObj, location);
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse JSON: ") + e.what());
    }
}

Result<std::unique_ptr<JigsawPattern>> TemplatePoolLoader::loadFromJson(
    const nlohmann::json& jsonObj, const ResourceLocation& location)
{
    // 解析名称（可选，如果没有则使用 location）
    ResourceLocation name = location;
    if (jsonObj.contains("name") && jsonObj["name"].is_string()) {
        name = ResourceLocation(jsonObj["name"].get<std::string>());
    }

    // 解析回退池（可选）
    ResourceLocation fallback("minecraft", "empty");
    if (jsonObj.contains("fallback") && jsonObj["fallback"].is_string()) {
        fallback = ResourceLocation(jsonObj["fallback"].get<std::string>());
    }

    // 创建模板池
    auto pattern = std::make_unique<JigsawPattern>(name, fallback);

    // 解析元素列表
    if (!jsonObj.contains("elements") || !jsonObj["elements"].is_array()) {
        return Error(ErrorCode::InvalidData, "Template pool missing 'elements' array");
    }

    const auto& elements = jsonObj["elements"];
    for (const auto& element : elements) {
        std::unique_ptr<JigsawPiece> piece;
        i32 weight = 1;
        if (parseElement(element, piece, weight) && piece) {
            pattern->addPiece(std::move(piece), weight);
        }
    }

    if (pattern->isEmpty()) {
        return Error(ErrorCode::InvalidData, "Template pool has no valid elements");
    }

    return pattern;
}

bool TemplatePoolLoader::parseElement(
    const nlohmann::json& elementObj, std::unique_ptr<JigsawPiece>& outPiece, i32& outWeight)
{
    // 解析权重
    outWeight = 1;
    if (elementObj.contains("weight") && elementObj["weight"].is_number_integer()) {
        outWeight = elementObj["weight"].get<i32>();
        if (outWeight <= 0) {
            outWeight = 1;
        }
    }

    // 解析元素
    if (!elementObj.contains("element") || !elementObj["element"].is_object()) {
        spdlog::warn("Element missing 'element' object");
        return false;
    }

    outPiece = parseElementType(elementObj["element"]);
    return outPiece != nullptr;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::parseElementType(const nlohmann::json& elementObj)
{
    // 获取元素类型
    if (!elementObj.contains("element_type") || !elementObj["element_type"].is_string()) {
        spdlog::warn("Element missing 'element_type' string");
        return nullptr;
    }

    std::string elementType = elementObj["element_type"].get<std::string>();

    // 移除命名空间前缀（如果有）
    if (elementType.size() > 10 && elementType.substr(0, 10) == "minecraft:") {
        elementType = elementType.substr(10);
    }

    // 根据类型分发解析
    if (elementType == "single_pool_element") {
        return parseSinglePoolElement(elementObj);
    } else if (elementType == "list_pool_element") {
        return parseListPoolElement(elementObj);
    } else if (elementType == "empty_pool_element") {
        return parseEmptyPoolElement(elementObj);
    } else if (elementType == "feature_pool_element") {
        return parseFeaturePoolElement(elementObj);
    } else {
        // 未知类型，返回空元素
        spdlog::warn("Unknown pool element type: {}, using empty element", elementType);
        return EmptyJigsawPiece::instance().clone();
    }
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::parseSinglePoolElement(const nlohmann::json& elementObj)
{
    // 解析模板位置
    if (!elementObj.contains("location") || !elementObj["location"].is_string()) {
        spdlog::warn("single_pool_element missing 'location' string");
        return nullptr;
    }

    std::string location = elementObj["location"].get<std::string>();

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (elementObj.contains("projection") && elementObj["projection"].is_string()) {
        projection = parseProjection(elementObj["projection"].get<std::string>());
    }

    // 创建单个拼图块
    std::unique_ptr<JigsawPiece> piece = std::make_unique<SingleJigsawPiece>(location, projection);
    return piece;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::parseListPoolElement(const nlohmann::json& elementObj)
{
    if (!elementObj.contains("elements") || !elementObj["elements"].is_array()) {
        spdlog::warn("list_pool_element missing 'elements' array");
        return nullptr;
    }

    auto listPiece = std::make_unique<ListJigsawPiece>();

    for (const auto& subElement : elementObj["elements"]) {
        std::unique_ptr<JigsawPiece> subPiece = parseElementType(subElement);
        if (subPiece) {
            listPiece->addPiece(std::move(subPiece));
        }
    }

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (elementObj.contains("projection") && elementObj["projection"].is_string()) {
        projection = parseProjection(elementObj["projection"].get<std::string>());
    }
    listPiece->setPlacementBehaviour(projection);

    std::unique_ptr<JigsawPiece> result = std::move(listPiece);
    return result;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::parseEmptyPoolElement(const nlohmann::json& elementObj)
{
    MC_UNUSED(elementObj);
    return EmptyJigsawPiece::instance().clone();
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::parseFeaturePoolElement(const nlohmann::json& elementObj)
{
    // feature_pool_element 用于生成地物
    // 当前实现为空元素，待地物系统完善后实现
    spdlog::debug("feature_pool_element not yet implemented, using empty element");
    MC_UNUSED(elementObj);
    return EmptyJigsawPiece::instance().clone();
}

JigsawPlacementBehaviour TemplatePoolLoader::parseProjection(const std::string& projectionStr)
{
    if (projectionStr == "terrain_matching" || projectionStr == "minecraft:terrain_matching") {
        return JigsawPlacementBehaviour::TerrainMatching;
    } else {
        return JigsawPlacementBehaviour::Rigid;
    }
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
