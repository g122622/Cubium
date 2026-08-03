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

#include "EmptyJigsawPiece.hpp"
#include "FeatureJigsawPiece.hpp"
#include "JigsawLoaderUtils.hpp"
#include "ListJigsawPiece.hpp"
#include "ProcessorListLoader.hpp"
#include "ProcessorListRegistry.hpp"
#include "SingleJigsawPiece.hpp"
#include "TemplatePool.hpp"
#include "TemplatePoolRegistry.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

Result<size_t> TemplatePoolLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
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
            TemplatePoolRegistry::instance().registerPool(parseResult.value());
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
            TemplatePoolRegistry::instance().registerPool(parseResult.value());
            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<std::unique_ptr<TemplatePool>> TemplatePoolLoader::loadFromJson(
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

Result<std::unique_ptr<TemplatePool>> TemplatePoolLoader::loadFromJson(
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
    auto pool = std::make_unique<TemplatePool>(name, fallback);

    // 解析元素列表
    if (!jsonObj.contains("elements") || !jsonObj["elements"].is_array()) {
        return Error(ErrorCode::InvalidData, "Template pool missing 'elements' array");
    }

    i32 legacyCount = 0;
    i32 featureCount = 0;
    i32 singleCount = 0;
    i32 listCount = 0;
    i32 emptyCount = 0;

    const auto& elements = jsonObj["elements"];
    for (const auto& element : elements) {
        std::unique_ptr<JigsawPiece> piece;
        i32 weight = 1;
        if (_parseElement(element, piece, weight) && piece) {
            // 统计元素类型（必须在 addPiece/std::move 之前，因为 move 后 piece 变为 nullptr）
            const auto& typeName = piece->getTypeName();
            if (typeName == "legacy_single_pool_element") {
                ++legacyCount;
            } else if (typeName == "single_pool_element") {
                ++singleCount;
            } else if (typeName == "list_pool_element") {
                ++listCount;
            } else if (typeName == "feature_pool_element") {
                ++featureCount;
            } else if (typeName == "empty_pool_element") {
                ++emptyCount;
            }

            pool->addPiece(std::move(piece), weight);
        }
    }

    // minecraft:empty 是原版 Jigsaw 终止符池，其 JSON 定义为 "elements": []，
    // 是合法的空元素列表（getRandomPiece 返回 nullptr、getShuffledPieces 返回空，
    // 组装器据此走 fallback 链）。其余池仍要求至少一个有效元素。
    const bool isEmptyTerminator = (name.namespace_() == "minecraft" && name.path() == "empty");
    if (pool->isEmpty() && !isEmptyTerminator) {
        return Error(ErrorCode::InvalidData, "Template pool has no valid elements");
    }

    spdlog::info("Template pool '{}': {} elements (legacy={}, single={}, list={}, feature={}, empty={})",
        name.toString(),
        pool->getTotalWeight(),
        legacyCount,
        singleCount,
        listCount,
        featureCount,
        emptyCount);

    return pool;
}

bool TemplatePoolLoader::_parseElement(
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

    outPiece = _parseElementType(elementObj["element"]);
    return outPiece != nullptr;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseElementType(const nlohmann::json& elementObj)
{
    // 获取元素类型
    if (!elementObj.contains("element_type") || !elementObj["element_type"].is_string()) {
        spdlog::warn("Element missing 'element_type' string");
        return nullptr;
    }

    std::string elementType = elementObj["element_type"].get<std::string>();
    elementType = stripMinecraftPrefix(elementType);

    // 根据类型分发解析
    if (elementType == "single_pool_element") {
        return _parseSinglePoolElement(elementObj);
    } else if (elementType == "legacy_single_pool_element") {
        return _parseLegacySinglePoolElement(elementObj);
    } else if (elementType == "list_pool_element") {
        return _parseListPoolElement(elementObj);
    } else if (elementType == "empty_pool_element") {
        return _parseEmptyPoolElement(elementObj);
    } else if (elementType == "feature_pool_element") {
        return _parseFeaturePoolElement(elementObj);
    } else {
        // 未知类型，返回空元素
        spdlog::warn("Unknown pool element type: '{}', using empty element", elementType);
        return std::make_unique<EmptyJigsawPiece>();
    }
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseSinglePoolElement(const nlohmann::json& elementObj)
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
        projection = _parseProjection(elementObj["projection"].get<std::string>());
    }

    // 解析处理器列表引用
    auto processorId = _parseProcessors(elementObj);

    // 创建单个拼图块
    auto piece = std::make_unique<SingleJigsawPiece>(location, projection, processorId);
    return piece;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseLegacySinglePoolElement(const nlohmann::json& elementObj)
{
    // 解析模板位置
    if (!elementObj.contains("location") || !elementObj["location"].is_string()) {
        spdlog::warn("legacy_single_pool_element missing 'location' string");
        return nullptr;
    }

    std::string location = elementObj["location"].get<std::string>();

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (elementObj.contains("projection") && elementObj["projection"].is_string()) {
        projection = _parseProjection(elementObj["projection"].get<std::string>());
    }

    // 解析处理器列表引用
    auto processorId = _parseProcessors(elementObj);

    // 创建 legacy 拼图块（放置时忽略空气方块）
    auto piece = std::make_unique<LegacySingleJigsawPiece>(location, projection, processorId);
    return piece;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseListPoolElement(const nlohmann::json& elementObj)
{
    if (!elementObj.contains("elements") || !elementObj["elements"].is_array()) {
        spdlog::warn("list_pool_element missing 'elements' array");
        return nullptr;
    }

    auto listPiece = std::make_unique<ListJigsawPiece>();

    for (const auto& subElement : elementObj["elements"]) {
        std::unique_ptr<JigsawPiece> subPiece = _parseElementType(subElement);
        if (subPiece) {
            listPiece->addPiece(std::move(subPiece));
        }
    }

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (elementObj.contains("projection") && elementObj["projection"].is_string()) {
        projection = _parseProjection(elementObj["projection"].get<std::string>());
    }
    listPiece->setPlacementBehaviour(projection);

    std::unique_ptr<JigsawPiece> result = std::move(listPiece);
    return result;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseEmptyPoolElement(const nlohmann::json& elementObj)
{
    MC_UNUSED(elementObj);
    // EmptyJigsawPiece 是单例，clone() 返回 nullptr（不可克隆）。
    // 返回一个临时持有的 EmptyJigsawPiece 实例：TemplatePool::addPiece 检测到 isEmpty() 后
    // 会存入单例 &EmptyJigsawPiece::instance() 指针（非拥有），随后丢弃此临时对象。
    // 对应 MC 1.21 EmptyPoolElement.getInstance() 在池中共享单例的语义。
    return std::make_unique<EmptyJigsawPiece>();
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseFeaturePoolElement(const nlohmann::json& elementObj)
{
    // 解析 feature 引用
    std::string featureId;
    if (elementObj.contains("feature") && elementObj["feature"].is_string()) {
        featureId = elementObj["feature"].get<std::string>();
    } else {
        spdlog::warn("feature_pool_element missing 'feature' string, using empty element");
        return std::make_unique<EmptyJigsawPiece>();
    }

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (elementObj.contains("projection") && elementObj["projection"].is_string()) {
        projection = _parseProjection(elementObj["projection"].get<std::string>());
    }

    // 创建 feature 拼图块
    return std::make_unique<FeatureJigsawPiece>(featureId, projection);
}

JigsawPlacementBehaviour TemplatePoolLoader::_parseProjection(const std::string& projectionStr)
{
    if (projectionStr == "terrain_matching" || projectionStr == "minecraft:terrain_matching") {
        return JigsawPlacementBehaviour::TerrainMatching;
    } else {
        return JigsawPlacementBehaviour::Rigid;
    }
}

std::optional<ResourceLocation> TemplatePoolLoader::_parseProcessors(const nlohmann::json& elementObj)
{
    if (!elementObj.contains("processors")) {
        return std::nullopt;
    }

    const auto& procs = elementObj["processors"];

    // 字符串形式: "minecraft:mossify_10_percent" — 处理器列表引用
    if (procs.is_string()) {
        return ResourceLocation(procs.get<std::string>());
    }

    // 对象形式: {"processors": [...]} — 内联处理器列表
    if (procs.is_object()) {
        if (procs.contains("processors") && procs["processors"].is_array()) {
            return _registerInlineProcessors(procs["processors"]);
        }
        return ResourceLocation("minecraft", "empty");
    }

    // 数组形式: [...] — 内联处理器列表
    if (procs.is_array()) {
        return _registerInlineProcessors(procs);
    }

    return std::nullopt;
}

std::optional<ResourceLocation> TemplatePoolLoader::_registerInlineProcessors(const nlohmann::json& processorsArray)
{
    // 空数组 → empty 处理器列表引用
    if (processorsArray.empty()) {
        return ResourceLocation("minecraft", "empty");
    }

    // 解析内联处理器数组并注册到 ProcessorListRegistry，返回合成资源位置供 SingleJigsawPiece 查找。
    // 对应 MC 1.21 SinglePoolElement 的内联 processors 列表（数据包中可直接内联处理器而非引用已注册列表）。
    auto processorList = ProcessorListLoader::parseInlineProcessorList(processorsArray);
    if (!processorList || processorList->empty()) {
        return ResourceLocation("minecraft", "empty");
    }

    // 生成唯一合成资源位置（inline_processor_list_<序号>）
    static std::atomic<u64> s_inlineCounter{0};
    const ResourceLocation inlineId(
        "minecraft", "inline_processor_list_" + std::to_string(s_inlineCounter.fetch_add(1)));
    ProcessorListRegistry::instance().registerList(inlineId, *processorList);
    return inlineId;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
