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
#include <simdjson.h>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

namespace {

// ============================================================================
// simdjson On-Demand 字段访问辅助函数
// ============================================================================

/**
 * @brief 从 On-Demand 对象读取可选字符串字段
 *
 * 字段缺失返回 std::nullopt;字段存在但非字符串返回 std::nullopt(容错)。
 * 注意:On-Demand 单次遍历语义下,本函数消费一次字段访问。
 */
std::optional<std::string> optString(simdjson::ondemand::object& obj, std::string_view key)
{
    auto fieldResult = obj[key];
    if (fieldResult.error() != simdjson::SUCCESS) {
        return std::nullopt;
    }
    auto strResult = fieldResult.value().get_string();
    if (strResult.error() != simdjson::SUCCESS) {
        return std::nullopt;
    }
    return std::string(strResult.value());
}

/**
 * @brief 从 On-Demand 对象读取必填字符串字段
 *
 * 字段缺失或非字符串返回空字符串。
 */
std::string reqString(simdjson::ondemand::object& obj, std::string_view key)
{
    auto fieldResult = obj[key];
    if (fieldResult.error() != simdjson::SUCCESS) {
        return {};
    }
    auto strResult = fieldResult.value().get_string();
    if (strResult.error() != simdjson::SUCCESS) {
        return {};
    }
    return std::string(strResult.value());
}

} // namespace

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
    // simdjson 需要 padded buffer(末尾 SIMDJSON_PADDING 字节)。
    // padded_string(std::string&&) 移动构造并自动补 padding。
    simdjson::padded_string padded(json);
    simdjson::ondemand::parser parser;

    auto docResult = parser.iterate(padded);
    if (docResult.error() != simdjson::SUCCESS) {
        return Error(
            ErrorCode::InvalidData, std::string("JSON parse error: ") + simdjson::error_message(docResult.error()));
    }

    try {
        return loadFromJson(docResult.value(), location);
    }
    catch (const simdjson::simdjson_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse JSON: ") + e.what());
    }
}

Result<std::unique_ptr<TemplatePool>> TemplatePoolLoader::loadFromJson(
    simdjson::ondemand::document& doc, const ResourceLocation& location)
{
    auto rootResult = doc.get_object();
    if (rootResult.error() != simdjson::SUCCESS) {
        return Error(ErrorCode::InvalidData, "Template pool JSON is not an object");
    }
    auto root = rootResult.value();

    // 解析名称（可选，如果没有则使用 location）
    ResourceLocation name = location;
    if (auto nameStr = optString(root, "name")) {
        name = ResourceLocation(*nameStr);
    }

    // 解析回退池（可选）
    ResourceLocation fallback("minecraft", "empty");
    if (auto fallbackStr = optString(root, "fallback")) {
        fallback = ResourceLocation(*fallbackStr);
    }

    // 创建模板池
    auto pool = std::make_unique<TemplatePool>(name, fallback);

    // 解析元素列表
    auto elementsResult = root["elements"];
    if (elementsResult.error() != simdjson::SUCCESS) {
        return Error(ErrorCode::InvalidData, "Template pool missing 'elements' array");
    }
    auto elementsArrResult = elementsResult.value().get_array();
    if (elementsArrResult.error() != simdjson::SUCCESS) {
        return Error(ErrorCode::InvalidData, "Template pool 'elements' is not an array");
    }
    auto elementsArr = elementsArrResult.value();

    for (auto elementVal : elementsArr) {
        auto elementObjResult = elementVal.get_object();
        if (elementObjResult.error() != simdjson::SUCCESS) {
            continue;
        }
        auto elementObj = elementObjResult.value();

        std::unique_ptr<JigsawPiece> piece;
        i32 weight = 1;
        if (_parseElement(elementObj, piece, weight) && piece) {
            pool->addPiece(std::move(piece), weight);
        }
    }

    return pool;
}

bool TemplatePoolLoader::_parseElement(
    simdjson::ondemand::object& elementObj, std::unique_ptr<JigsawPiece>& outPiece, i32& outWeight)
{
    // 解析权重
    outWeight = 1;
    auto weightResult = elementObj["weight"];
    if (weightResult.error() == simdjson::SUCCESS) {
        auto intResult = weightResult.value().get_int64();
        if (intResult.error() == simdjson::SUCCESS) {
            outWeight = static_cast<i32>(intResult.value());
            if (outWeight <= 0) {
                outWeight = 1;
            }
        }
    }

    // 解析元素
    auto elementResult = elementObj["element"];
    if (elementResult.error() != simdjson::SUCCESS) {
        spdlog::warn("Element missing 'element' object");
        return false;
    }
    auto elementInnerResult = elementResult.value().get_object();
    if (elementInnerResult.error() != simdjson::SUCCESS) {
        spdlog::warn("Element 'element' is not an object");
        return false;
    }
    auto elementInner = elementInnerResult.value();

    outPiece = _parseElementType(elementInner);
    return outPiece != nullptr;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseElementType(simdjson::ondemand::object& elementObj)
{
    // 获取元素类型
    auto typeStr = reqString(elementObj, "element_type");
    if (typeStr.empty()) {
        spdlog::warn("Element missing 'element_type' string");
        return nullptr;
    }
    typeStr = stripMinecraftPrefix(typeStr);

    // 根据类型分发解析
    if (typeStr == "single_pool_element") {
        return _parseSinglePoolElement(elementObj);
    } else if (typeStr == "legacy_single_pool_element") {
        return _parseLegacySinglePoolElement(elementObj);
    } else if (typeStr == "list_pool_element") {
        return _parseListPoolElement(elementObj);
    } else if (typeStr == "empty_pool_element") {
        return _parseEmptyPoolElement();
    } else if (typeStr == "feature_pool_element") {
        return _parseFeaturePoolElement(elementObj);
    } else {
        // 未知类型，返回空元素
        spdlog::warn("Unknown pool element type: '{}', using empty element", typeStr);
        return std::make_unique<EmptyJigsawPiece>();
    }
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseSinglePoolElement(simdjson::ondemand::object& elementObj)
{
    // 解析模板位置
    auto location = reqString(elementObj, "location");
    if (location.empty()) {
        spdlog::warn("single_pool_element missing 'location' string");
        return nullptr;
    }

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (auto projStr = optString(elementObj, "projection")) {
        projection = _parseProjection(*projStr);
    }

    // 解析处理器列表引用
    auto processorId = _parseProcessors(elementObj);

    // 创建单个拼图块
    auto piece = std::make_unique<SingleJigsawPiece>(location, projection, processorId);
    return piece;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseLegacySinglePoolElement(simdjson::ondemand::object& elementObj)
{
    // 解析模板位置
    auto location = reqString(elementObj, "location");
    if (location.empty()) {
        spdlog::warn("legacy_single_pool_element missing 'location' string");
        return nullptr;
    }

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (auto projStr = optString(elementObj, "projection")) {
        projection = _parseProjection(*projStr);
    }

    // 解析处理器列表引用
    auto processorId = _parseProcessors(elementObj);

    // 创建 legacy 拼图块（放置时忽略空气方块）
    auto piece = std::make_unique<LegacySingleJigsawPiece>(location, projection, processorId);
    return piece;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseListPoolElement(simdjson::ondemand::object& elementObj)
{
    auto elementsResult = elementObj["elements"];
    if (elementsResult.error() != simdjson::SUCCESS) {
        spdlog::warn("list_pool_element missing 'elements' array");
        return nullptr;
    }
    auto elementsArrResult = elementsResult.value().get_array();
    if (elementsArrResult.error() != simdjson::SUCCESS) {
        spdlog::warn("list_pool_element 'elements' is not an array");
        return nullptr;
    }
    auto elementsArr = elementsArrResult.value();

    auto listPiece = std::make_unique<ListJigsawPiece>();

    for (auto subElementVal : elementsArr) {
        auto subElementObjResult = subElementVal.get_object();
        if (subElementObjResult.error() != simdjson::SUCCESS) {
            continue;
        }
        auto subElementObj = subElementObjResult.value();
        std::unique_ptr<JigsawPiece> subPiece = _parseElementType(subElementObj);
        if (subPiece) {
            listPiece->addPiece(std::move(subPiece));
        }
    }

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (auto projStr = optString(elementObj, "projection")) {
        projection = _parseProjection(*projStr);
    }
    listPiece->setPlacementBehaviour(projection);

    std::unique_ptr<JigsawPiece> result = std::move(listPiece);
    return result;
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseEmptyPoolElement()
{
    // EmptyJigsawPiece 是单例，clone() 返回 nullptr（不可克隆）。
    // 返回一个临时持有的 EmptyJigsawPiece 实例：TemplatePool::addPiece 检测到 isEmpty() 后
    // 会存入单例 &EmptyJigsawPiece::instance() 指针（非拥有），随后丢弃此临时对象。
    // 对应 MC 1.21 EmptyPoolElement.getInstance() 在池中共享单例的语义。
    return std::make_unique<EmptyJigsawPiece>();
}

std::unique_ptr<JigsawPiece> TemplatePoolLoader::_parseFeaturePoolElement(simdjson::ondemand::object& elementObj)
{
    // 解析 feature 引用
    auto featureId = reqString(elementObj, "feature");
    if (featureId.empty()) {
        spdlog::warn("feature_pool_element missing 'feature' string, using empty element");
        return std::make_unique<EmptyJigsawPiece>();
    }

    // 解析投影类型
    JigsawPlacementBehaviour projection = JigsawPlacementBehaviour::Rigid;
    if (auto projStr = optString(elementObj, "projection")) {
        projection = _parseProjection(*projStr);
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

std::optional<ResourceLocation> TemplatePoolLoader::_parseProcessors(simdjson::ondemand::object& elementObj)
{
    // processors 字段可以是:
    // - 字符串: "minecraft:mossify_10_percent"(处理器列表引用)
    // - 对象: {"processors": [...]}(内联处理器列表)
    // - 数组: [...](内联处理器列表)
    // 用 type() 分派:不消耗 value,确定类型后传给对应处理函数。
    auto procsResult = elementObj["processors"];
    if (procsResult.error() != simdjson::SUCCESS) {
        return std::nullopt;
    }
    auto procsValue = procsResult.value();

    auto typeResult = procsValue.type();
    if (typeResult.error() != simdjson::SUCCESS) {
        return std::nullopt;
    }

    switch (typeResult.value()) {
        case simdjson::ondemand::json_type::string: {
            auto strResult = procsValue.get_string();
            if (strResult.error() != simdjson::SUCCESS) {
                return std::nullopt;
            }
            return ResourceLocation(std::string(strResult.value()));
        }
        case simdjson::ondemand::json_type::object: {
            auto objResult = procsValue.get_object();
            if (objResult.error() != simdjson::SUCCESS) {
                return ResourceLocation("minecraft", "empty");
            }
            auto innerObj = objResult.value();
            auto innerProcsResult = innerObj["processors"];
            if (innerProcsResult.error() == simdjson::SUCCESS) {
                auto innerProcsValue = innerProcsResult.value();
                return _registerInlineProcessors(innerProcsValue);
            }
            return ResourceLocation("minecraft", "empty");
        }
        case simdjson::ondemand::json_type::array: {
            return _registerInlineProcessors(procsValue);
        }
        default:
            return std::nullopt;
    }
}

std::optional<ResourceLocation> TemplatePoolLoader::_registerInlineProcessors(
    simdjson::ondemand::value& processorsValue)
{
    // 解析内联处理器数组并注册到 ProcessorListRegistry,返回合成资源位置供 SingleJigsawPiece 查找。
    // 对应 MC 1.21 SinglePoolElement 的内联 processors 列表(数据包中可直接内联处理器而非引用已注册列表)。
    // 空数组或解析失败返回 minecraft:empty。
    auto processorList = ProcessorListLoader::parseInlineProcessorList(processorsValue);
    if (!processorList || processorList->empty()) {
        return ResourceLocation("minecraft", "empty");
    }

    // 生成唯一合成资源位置(inline_processor_list_<序号>)
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
