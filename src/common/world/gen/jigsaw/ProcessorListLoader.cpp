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

#include "ProcessorListLoader.hpp"

#include "JigsawLoaderUtils.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/feature/template/CappedStructureProcessor.hpp"
#include "common/world/gen/feature/template/ProtectedBlocksProcessor.hpp"
#include "common/world/gen/feature/template/RuleTest.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/jigsaw/ProcessorListRegistry.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// 导入处理器类型（来自 Template.hpp 的 template_ 命名空间）
using feature::template_::AlwaysTruePosRuleTest;
using feature::template_::AlwaysTrueRuleTest;
using feature::template_::AxisAlignedLinearPosTest;
using feature::template_::BlackstoneReplacementProcessor;
using feature::template_::BlockAgeProcessor;
using feature::template_::BlockIgnoreStructureProcessor;
using feature::template_::BlockMatchRuleTest;
using feature::template_::BlockStateMatchRuleTest;
using feature::template_::CappedStructureProcessor;
using feature::template_::GravityStructureProcessor;
using feature::template_::IntegrityProcessor;
using feature::template_::JigsawReplacementStructureProcessor;
using feature::template_::LavaSubmergingProcessor;
using feature::template_::LinearPosRuleTest;
using feature::template_::NopStructureProcessor;
using feature::template_::ProtectedBlocksProcessor;
using feature::template_::RandomBlockMatchRuleTest;
using feature::template_::RandomBlockStateMatchRuleTest;
using feature::template_::RuleEntry;
using feature::template_::RuleStructureProcessor;
using feature::template_::RuleTest;
using feature::template_::StructureProcessor;
using feature::template_::StructureProcessorList;
using feature::template_::TagMatchRuleTest;

namespace {

// ============================================================================
// simdjson On-Demand 字段访问辅助函数
// ============================================================================

/**
 * @brief 从 On-Demand 对象读取可选字符串字段
 *
 * 字段缺失或非字符串返回 std::nullopt(容错)。每次调用消费一次字段访问。
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

/**
 * @brief 从 On-Demand 对象读取可选 f32 数值字段
 *
 * 字段缺失或非数值返回 defaultValue。
 */
f32 optFloat(simdjson::ondemand::object& obj, std::string_view key, f32 defaultValue)
{
    auto fieldResult = obj[key];
    if (fieldResult.error() != simdjson::SUCCESS) {
        return defaultValue;
    }
    auto dblResult = fieldResult.value().get_double();
    if (dblResult.error() != simdjson::SUCCESS) {
        return defaultValue;
    }
    return static_cast<f32>(dblResult.value());
}

/**
 * @brief 从 On-Demand 对象读取可选 i32 数值字段
 *
 * 字段缺失或非整数返回 defaultValue。
 */
i32 optInt(simdjson::ondemand::object& obj, std::string_view key, i32 defaultValue)
{
    auto fieldResult = obj[key];
    if (fieldResult.error() != simdjson::SUCCESS) {
        return defaultValue;
    }
    auto intResult = fieldResult.value().get_int64();
    if (intResult.error() != simdjson::SUCCESS) {
        return defaultValue;
    }
    return static_cast<i32>(intResult.value());
}

// ============================================================================
// 辅助函数：从方块名称和属性解析 BlockState
// ============================================================================

/**
 * @brief 从 JSON output_state 解析方块状态 ID
 *
 * JSON 格式：
 *   { "Name": "minecraft:stone" }
 *   { "Name": "minecraft:waxed_copper_bulb", "Properties": { "lit": "true", "powered": "false" } }
 *
 * @return 方块状态 ID，解析失败返回 0（空气）
 */
u32 parseOutputBlockStateId(simdjson::ondemand::object& outputState)
{
    auto name = reqString(outputState, "Name");
    if (name.empty()) {
        return 0;
    }

    ResourceLocation blockLoc(name);
    Block* block = BlockRegistry::instance().getBlock(blockLoc);
    if (block == nullptr) {
        spdlog::warn("ProcessorListLoader: unknown block '{}' in output_state, using air", name);
        return 0;
    }

    const BlockState& defaultState = block->defaultState();

    // 没有 Properties，直接使用默认状态
    auto propsResult = outputState["Properties"];
    if (propsResult.error() != simdjson::SUCCESS) {
        return defaultState.stateId();
    }
    auto propsObjResult = propsResult.value().get_object();
    if (propsObjResult.error() != simdjson::SUCCESS) {
        return defaultState.stateId();
    }
    auto props = propsObjResult.value();

    // 有 Properties，逐一设置属性
    const BlockState* currentState = &defaultState;

    for (auto field : props) {
        auto keyResult = field.unescaped_key();
        if (keyResult.error() != simdjson::SUCCESS) {
            continue;
        }
        std::string propName(keyResult.value());

        auto valResult = field.value().get_string();
        if (valResult.error() != simdjson::SUCCESS) {
            continue;
        }
        std::string propValue(valResult.value());

        const IProperty* prop = block->stateContainer().getProperty(propName);
        if (prop == nullptr) {
            spdlog::warn("ProcessorListLoader: unknown property '{}' on block '{}'", propName, name);
            continue;
        }

        auto valueIndex = prop->parseValue(propValue);
        if (!valueIndex.has_value()) {
            spdlog::warn(
                "ProcessorListLoader: invalid value '{}' for property '{}' on block '{}'", propValue, propName, name);
            continue;
        }

        currentState = &currentState->withValueIndex(*prop, *valueIndex);
    }

    return currentState->stateId();
}

/**
 * @brief 从 JSON block_state 解析方块状态指针
 *
 * 复用 parseOutputBlockStateId 解析逻辑，再通过方块注册表回查状态指针。
 * 用于 BlockStateMatchRuleTest / RandomBlockStateMatchRuleTest（持有 const BlockState*）。
 *
 * @return 方块状态指针，解析失败返回 nullptr
 */
const BlockState* parseOutputBlockState(simdjson::ondemand::object& outputState)
{
    u32 stateId = parseOutputBlockStateId(outputState);
    if (stateId == 0) {
        return nullptr;
    }
    return BlockRegistry::instance().getBlockState(stateId);
}

/**
 * @brief 从 JSON 解析输入/位置谓词 (RuleTest)
 *
 * JSON 格式：
 *   { "predicate_type": "minecraft:always_true" }
 *   { "predicate_type": "minecraft:block_match", "block": "minecraft:stone" }
 *   { "predicate_type": "minecraft:random_block_match", "block": "minecraft:stone", "probability": 0.5 }
 *   { "predicate_type": "minecraft:tag_match", "tag": "minecraft:stone_bricks" }
 *   { "predicate_type": "minecraft:block_state_match", "block_state": { "Name": "...", "Properties": {...} } }
 *   { "predicate_type": "minecraft:random_block_state_match", "block_state": {...}, "probability": 0.5 }
 */
std::unique_ptr<RuleTest> parseRuleTest(simdjson::ondemand::object& predicateObj)
{
    auto predicateType = reqString(predicateObj, "predicate_type");
    if (predicateType.empty()) {
        spdlog::warn("ProcessorListLoader: predicate missing 'predicate_type'");
        return std::make_unique<AlwaysTrueRuleTest>();
    }
    predicateType = stripMinecraftPrefix(predicateType);

    if (predicateType == "always_true") {
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "block_match") {
        auto block = optString(predicateObj, "block");
        if (block) {
            ResourceLocation blockLoc(*block);
            Block* blockPtr = BlockRegistry::instance().getBlock(blockLoc);
            if (blockPtr != nullptr) {
                return std::make_unique<BlockMatchRuleTest>(blockPtr);
            }
            spdlog::warn("ProcessorListLoader: block_match: unknown block '{}'", blockLoc.toString());
        }
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "random_block_match") {
        auto block = optString(predicateObj, "block");
        // probability 是数值字段
        auto probResult = predicateObj["probability"];
        if (block && probResult.error() == simdjson::SUCCESS) {
            auto probValResult = probResult.value().get_double();
            if (probValResult.error() == simdjson::SUCCESS) {
                ResourceLocation blockLoc(*block);
                f32 probability = static_cast<f32>(probValResult.value());
                Block* blockPtr = BlockRegistry::instance().getBlock(blockLoc);
                if (blockPtr != nullptr) {
                    return std::make_unique<RandomBlockMatchRuleTest>(blockPtr, probability);
                }
                spdlog::warn("ProcessorListLoader: random_block_match: unknown block '{}'", blockLoc.toString());
            }
        }
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "tag_match") {
        auto tag = optString(predicateObj, "tag");
        if (tag) {
            return std::make_unique<TagMatchRuleTest>(ResourceLocation(*tag));
        }
        spdlog::warn("ProcessorListLoader: tag_match: missing 'tag' field");
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "block_state_match" || predicateType == "blockstate_match") {
        auto bsResult = predicateObj["block_state"];
        if (bsResult.error() == simdjson::SUCCESS) {
            auto bsObjResult = bsResult.value().get_object();
            if (bsObjResult.error() == simdjson::SUCCESS) {
                auto bsObj = bsObjResult.value();
                const BlockState* state = parseOutputBlockState(bsObj);
                if (state != nullptr) {
                    return std::make_unique<BlockStateMatchRuleTest>(state);
                }
                spdlog::warn("ProcessorListLoader: block_state_match: unknown block state");
            }
        }
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "random_block_state_match") {
        auto bsResult = predicateObj["block_state"];
        auto probResult = predicateObj["probability"];
        if (bsResult.error() == simdjson::SUCCESS && probResult.error() == simdjson::SUCCESS) {
            auto bsObjResult = bsResult.value().get_object();
            auto probValResult = probResult.value().get_double();
            if (bsObjResult.error() == simdjson::SUCCESS && probValResult.error() == simdjson::SUCCESS) {
                auto bsObj = bsObjResult.value();
                const BlockState* state = parseOutputBlockState(bsObj);
                f32 probability = static_cast<f32>(probValResult.value());
                if (state != nullptr) {
                    return std::make_unique<RandomBlockStateMatchRuleTest>(state, probability);
                }
                spdlog::warn("ProcessorListLoader: random_block_state_match: unknown block state");
            }
        }
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    spdlog::warn("ProcessorListLoader: unknown predicate_type '{}', using always_true", predicateType);
    return std::make_unique<AlwaysTrueRuleTest>();
}

/**
 * @brief 从 JSON 解析位置谓词 (PosRuleTest)
 *
 * JSON 格式：
 *   { "predicate_type": "minecraft:always_true" }
 *   { "predicate_type": "minecraft:linear_pos", "min_chance": 0.0, "max_chance": 1.0, "min_dist": 0, "max_dist": 10 }
 *   { "predicate_type": "minecraft:axis_aligned_linear_pos", "min_chance": 0.0, "max_chance": 0.05, "min_dist": 0,
 * "max_dist": 100, "axis": "y" }
 */
std::unique_ptr<feature::template_::PosRuleTest> _parsePosRuleTest(simdjson::ondemand::object& predicateObj)
{
    auto predicateType = reqString(predicateObj, "predicate_type");
    if (predicateType.empty()) {
        spdlog::warn("ProcessorListLoader: pos_predicate missing 'predicate_type', using always_true");
        return std::make_unique<AlwaysTruePosRuleTest>();
    }
    predicateType = stripMinecraftPrefix(predicateType);

    if (predicateType == "always_true") {
        return std::make_unique<AlwaysTruePosRuleTest>();
    }

    if (predicateType == "linear_pos") {
        // min_chance/max_chance (f32, 默认 0.0), min_dist/max_dist (i32, 默认 0)
        f32 minChance = optFloat(predicateObj, "min_chance", 0.0f);
        f32 maxChance = optFloat(predicateObj, "max_chance", 0.0f);
        i32 minDist = optInt(predicateObj, "min_dist", 0);
        i32 maxDist = optInt(predicateObj, "max_dist", 0);

        // min_dist >= maxDist 时回退到 always_true
        if (minDist >= maxDist) {
            spdlog::warn(
                "ProcessorListLoader: linear_pos min_dist ({}) >= max_dist ({}), using always_true", minDist, maxDist);
            return std::make_unique<AlwaysTruePosRuleTest>();
        }

        return std::make_unique<LinearPosRuleTest>(minDist, maxDist, minChance, maxChance);
    }

    if (predicateType == "axis_aligned_linear_pos") {
        // min_chance/max_chance (f32, 默认 0.0), min_dist/max_dist (i32, 默认 0), axis (默认 Y)
        f32 minChance = optFloat(predicateObj, "min_chance", 0.0f);
        f32 maxChance = optFloat(predicateObj, "max_chance", 0.0f);
        i32 minDist = optInt(predicateObj, "min_dist", 0);
        i32 maxDist = optInt(predicateObj, "max_dist", 0);
        Axis axis = Axis::Y; // 默认轴为 Y

        auto axisStr = optString(predicateObj, "axis");
        if (axisStr) {
            if (*axisStr == "x") {
                axis = Axis::X;
            } else if (*axisStr == "y") {
                axis = Axis::Y;
            } else if (*axisStr == "z") {
                axis = Axis::Z;
            } else {
                spdlog::warn(
                    "ProcessorListLoader: axis_aligned_linear_pos unknown axis '{}', defaulting to Y", *axisStr);
            }
        }

        // min_dist >= maxDist 时回退到 always_true
        if (minDist >= maxDist) {
            spdlog::warn(
                "ProcessorListLoader: axis_aligned_linear_pos min_dist ({}) >= max_dist ({}), using always_true",
                minDist,
                maxDist);
            return std::make_unique<AlwaysTruePosRuleTest>();
        }

        return std::make_unique<AxisAlignedLinearPosTest>(minChance, maxChance, minDist, maxDist, axis);
    }

    spdlog::warn("ProcessorListLoader: unknown pos_predicate_type '{}', using always_true", predicateType);
    return std::make_unique<AlwaysTruePosRuleTest>();
}

} // namespace

Result<size_t> ProcessorListLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 processor_list 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/worldgen/processor_list";
        auto listResult = dataPackList.listResources(directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取处理器列表名称
            // 路径格式: namespace/worldgen/processor_list/path/to/list.json
            std::string listName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            // 移除 .json 扩展名
            if (listName.size() >= 5 && listName.substr(listName.size() - 5) == ".json") {
                listName = listName.substr(0, listName.size() - 5);
            }

            ResourceLocation location(listName);

            // 读取 JSON 内容（数据包版本会覆盖硬编码注册，因为数据包通常更准确）
            auto readResult = dataPackList.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read processor list: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse processor list {}: {}", listName, parseResult.error().message());
                continue;
            }

            ++loadedCount;
        }
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} processor lists from datapacks", loadedCount);
    }

    return loadedCount;
}
Result<size_t> ProcessorListLoader::loadFromResourcePack(const IResourcePack& pack)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 processor_list 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/worldgen/processor_list";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取处理器列表名称
            std::string listName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            if (listName.size() >= 5 && listName.substr(listName.size() - 5) == ".json") {
                listName = listName.substr(0, listName.size() - 5);
            }

            ResourceLocation location(listName);

            // 读取 JSON 内容（数据包版本会覆盖硬编码注册，因为数据包通常更准确）
            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read processor list: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse processor list {}: {}", listName, parseResult.error().message());
                continue;
            }

            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<void> ProcessorListLoader::loadFromJson(const std::string& json, const ResourceLocation& location)
{
    simdjson::padded_string padded(json);
    simdjson::ondemand::parser parser;

    auto docResult = parser.iterate(padded);
    if (docResult.error() != simdjson::SUCCESS) {
        return Error(
            ErrorCode::InvalidData, std::string("JSON parse error: ") + simdjson::error_message(docResult.error()));
    }

    try {
        auto rootResult = docResult.value().get_object();
        if (rootResult.error() != simdjson::SUCCESS) {
            return Error(ErrorCode::InvalidData, "Processor list JSON is not an object");
        }
        auto root = rootResult.value();
        return _loadFromJsonObj(root, location);
    }
    catch (const simdjson::simdjson_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse JSON: ") + e.what());
    }
}

Result<void> ProcessorListLoader::_loadFromJsonObj(
    simdjson::ondemand::object& jsonObj, const ResourceLocation& location)
{
    // 解析 processors 数组
    auto procsResult = jsonObj["processors"];
    if (procsResult.error() != simdjson::SUCCESS) {
        return Error(ErrorCode::InvalidData, "Processor list missing 'processors' array");
    }
    auto procsArrResult = procsResult.value().get_array();
    if (procsArrResult.error() != simdjson::SUCCESS) {
        return Error(ErrorCode::InvalidData, "Processor list 'processors' is not an array");
    }
    auto procsArr = procsArrResult.value();

    auto processorList = std::make_unique<StructureProcessorList>();

    for (auto processorVal : procsArr) {
        auto processorObjResult = processorVal.get_object();
        if (processorObjResult.error() != simdjson::SUCCESS) {
            continue;
        }
        auto processorObj = processorObjResult.value();
        auto processor = _parseProcessor(processorObj);
        if (processor) {
            processorList->addProcessor(std::move(processor));
        }
    }

    // 注册处理器列表
    ProcessorListRegistry::instance().registerList(location, *processorList);

    return Result<void>::ok();
}

std::unique_ptr<StructureProcessorList> ProcessorListLoader::parseInlineProcessorList(
    simdjson::ondemand::value& processorsValue)
{
    auto processorList = std::make_unique<StructureProcessorList>();

    auto arrResult = processorsValue.get_array();
    if (arrResult.error() != simdjson::SUCCESS) {
        return processorList; // 非数组返回空列表
    }
    auto arr = arrResult.value();

    for (auto processorVal : arr) {
        auto processorObjResult = processorVal.get_object();
        if (processorObjResult.error() != simdjson::SUCCESS) {
            continue;
        }
        auto processorObj = processorObjResult.value();
        auto processor = _parseProcessor(processorObj);
        if (processor) {
            processorList->addProcessor(std::move(processor));
        }
    }

    return processorList;
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseProcessor(simdjson::ondemand::object& processorObj)
{
    auto type = reqString(processorObj, "processor_type");
    if (type.empty()) {
        spdlog::warn("Processor missing 'processor_type' string");
        return nullptr;
    }
    type = stripMinecraftPrefix(type);

    // 根据类型分发解析
    if (type == "block_ignore") {
        return _parseBlockIgnoreProcessor(processorObj);
    } else if (type == "block_rot") {
        return _parseBlockRotProcessor(processorObj);
    } else if (type == "gravity") {
        return _parseGravityProcessor(processorObj);
    } else if (type == "jigsaw_replacement") {
        return _parseJigsawReplacementProcessor();
    } else if (type == "rule") {
        return _parseRuleProcessor(processorObj);
    } else if (type == "block_age") {
        return _parseBlockAgeProcessor(processorObj);
    } else if (type == "blackstone_replace") {
        return _parseBlackstoneReplaceProcessor();
    } else if (type == "lava_submerged_block") {
        return _parseLavaSubmergedBlockProcessor();
    } else if (type == "nop") {
        return _parseNopProcessor();
    } else if (type == "protected_blocks") {
        return _parseProtectedBlocksProcessor(processorObj);
    } else if (type == "capped") {
        return _parseCappedProcessor(processorObj);
    } else {
        spdlog::warn("Unknown processor type: '{}', skipping", type);
        return nullptr;
    }
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseBlockIgnoreProcessor(
    simdjson::ondemand::object& processorObj)
{
    // block_ignore 处理器：忽略指定方块列表
    // JSON: { "processor_type": "minecraft:block_ignore", "blocks": [...] }
    std::vector<u32> blocksToIgnore;

    auto blocksResult = processorObj["blocks"];
    if (blocksResult.error() == simdjson::SUCCESS) {
        auto blocksArrResult = blocksResult.value().get_array();
        if (blocksArrResult.error() == simdjson::SUCCESS) {
            auto blocksArr = blocksArrResult.value();
            for (auto blockEntryVal : blocksArr) {
                // 先判断元素类型:字符串(方块名)或对象(完整方块状态)
                auto typeResult = blockEntryVal.type();
                if (typeResult.error() != simdjson::SUCCESS) {
                    continue;
                }
                if (typeResult.value() == simdjson::ondemand::json_type::string) {
                    auto strResult = blockEntryVal.get_string();
                    if (strResult.error() == simdjson::SUCCESS) {
                        ResourceLocation blockLoc(std::string(strResult.value()));
                        Block* block = BlockRegistry::instance().getBlock(blockLoc);
                        if (block != nullptr) {
                            blocksToIgnore.push_back(block->defaultState().stateId());
                        } else {
                            spdlog::info(
                                "block_ignore: unknown block '{}' in ignore list, skipping", blockLoc.toString());
                        }
                    }
                } else if (typeResult.value() == simdjson::ondemand::json_type::object) {
                    auto objResult = blockEntryVal.get_object();
                    if (objResult.error() == simdjson::SUCCESS) {
                        auto blockEntryObj = objResult.value();
                        u32 stateId = parseOutputBlockStateId(blockEntryObj);
                        if (stateId != 0) {
                            blocksToIgnore.push_back(stateId);
                        }
                    }
                }
            }
        }
    }

    return std::make_unique<BlockIgnoreStructureProcessor>(blocksToIgnore);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseBlockRotProcessor(
    simdjson::ondemand::object& processorObj)
{
    // block_rot 处理器（IntegrityProcessor）：完整性衰减
    // JSON: { "processor_type": "minecraft:block_rot", "integrity": 0.5 }
    f32 integrity = optFloat(processorObj, "integrity", 1.0f);
    // 限制在 [0.0, 1.0] 范围
    if (integrity < 0.0f) {
        integrity = 0.0f;
    } else if (integrity > 1.0f) {
        integrity = 1.0f;
    }

    return std::make_unique<IntegrityProcessor>(integrity);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseGravityProcessor(
    simdjson::ondemand::object& processorObj)
{
    // gravity 处理器：重力偏移
    // JSON: { "processor_type": "minecraft:gravity", "heightmap": "WORLD_SURFACE_WG" }
    i32 heightmapType = 0; // 默认 WORLD_SURFACE_WG
    auto heightmap = optString(processorObj, "heightmap");
    if (heightmap) {
        std::string hm = stripMinecraftPrefix(*heightmap);

        if (hm == "WORLD_SURFACE_WG" || hm == "world_surface_wg") {
            heightmapType = 0;
        } else if (hm == "OCEAN_FLOOR_WG" || hm == "ocean_floor_wg") {
            heightmapType = 1;
        } else if (hm == "MOTION_BLOCKING" || hm == "motion_blocking") {
            heightmapType = 2;
        } else {
            spdlog::info("gravity: unknown heightmap '{}', defaulting to WORLD_SURFACE_WG", hm);
        }
    }

    return std::make_unique<GravityStructureProcessor>(heightmapType);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseJigsawReplacementProcessor()
{
    // jigsaw_replacement 处理器：Jigsaw 替换，无需额外配置
    return std::make_unique<JigsawReplacementStructureProcessor>();
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseRuleProcessor(simdjson::ondemand::object& processorObj)
{
    // rule 处理器：规则处理器
    // JSON: { "processor_type": "minecraft:rule", "rules": [...] }
    // 每条规则：{ "input_predicate": {...}, "location_predicate": {...}, "output_state": {...} }
    std::vector<std::unique_ptr<RuleEntry>> rules;

    auto rulesResult = processorObj["rules"];
    if (rulesResult.error() != simdjson::SUCCESS) {
        spdlog::info("rule processor: no rules array, creating empty rule processor");
        return std::make_unique<RuleStructureProcessor>(std::move(rules));
    }
    auto rulesArrResult = rulesResult.value().get_array();
    if (rulesArrResult.error() != simdjson::SUCCESS) {
        spdlog::info("rule processor: 'rules' is not an array, creating empty rule processor");
        return std::make_unique<RuleStructureProcessor>(std::move(rules));
    }
    auto rulesArr = rulesArrResult.value();

    for (auto ruleVal : rulesArr) {
        auto ruleObjResult = ruleVal.get_object();
        if (ruleObjResult.error() != simdjson::SUCCESS) {
            continue;
        }
        auto ruleObj = ruleObjResult.value();

        // 解析 input_predicate
        std::unique_ptr<RuleTest> inputPredicate;
        auto inputResult = ruleObj["input_predicate"];
        if (inputResult.error() == simdjson::SUCCESS) {
            auto inputObjResult = inputResult.value().get_object();
            if (inputObjResult.error() == simdjson::SUCCESS) {
                auto inputObj = inputObjResult.value();
                inputPredicate = parseRuleTest(inputObj);
            }
        }
        if (!inputPredicate) {
            inputPredicate = std::make_unique<AlwaysTrueRuleTest>();
        }

        // 解析 location_predicate
        std::unique_ptr<RuleTest> locationPredicate;
        auto locResult = ruleObj["location_predicate"];
        if (locResult.error() == simdjson::SUCCESS) {
            auto locObjResult = locResult.value().get_object();
            if (locObjResult.error() == simdjson::SUCCESS) {
                auto locObj = locObjResult.value();
                locationPredicate = parseRuleTest(locObj);
            }
        }
        if (!locationPredicate) {
            locationPredicate = std::make_unique<AlwaysTrueRuleTest>();
        }

        // 解析 pos_predicate（可选）
        std::unique_ptr<feature::template_::PosRuleTest> posPredicate;
        auto posResult = ruleObj["pos_predicate"];
        if (posResult.error() == simdjson::SUCCESS) {
            auto posObjResult = posResult.value().get_object();
            if (posObjResult.error() == simdjson::SUCCESS) {
                auto posObj = posObjResult.value();
                posPredicate = _parsePosRuleTest(posObj);
            }
        }
        if (!posPredicate) {
            posPredicate = std::make_unique<AlwaysTruePosRuleTest>();
        }

        // 解析 output_state
        u32 outputStateId = 0;
        auto outResult = ruleObj["output_state"];
        if (outResult.error() == simdjson::SUCCESS) {
            auto outObjResult = outResult.value().get_object();
            if (outObjResult.error() == simdjson::SUCCESS) {
                auto outObj = outObjResult.value();
                outputStateId = parseOutputBlockStateId(outObj);
            }
        }

        // 解析 output_nbt / block_entity_modifier（可选，暂不实现）
        std::optional<nbt::tags::compound_tag> outputNbt;

        auto ruleEntry = std::make_unique<RuleEntry>(
            std::move(inputPredicate), std::move(locationPredicate), std::move(posPredicate), outputStateId, outputNbt);
        rules.push_back(std::move(ruleEntry));
    }

    return std::make_unique<RuleStructureProcessor>(std::move(rules));
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseBlockAgeProcessor(
    simdjson::ondemand::object& processorObj)
{
    // block_age 处理器（苔藓化）
    // JSON: { "processor_type": "minecraft:block_age", "mossiness": 0.5 }
    f32 mossiness = optFloat(processorObj, "mossiness", 0.0f);
    if (mossiness < 0.0f) {
        mossiness = 0.0f;
    } else if (mossiness > 1.0f) {
        mossiness = 1.0f;
    }

    return std::make_unique<BlockAgeProcessor>(mossiness);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseBlackstoneReplaceProcessor()
{
    // blackstone_replace 处理器：黑石替换，无需额外配置
    return std::make_unique<BlackstoneReplacementProcessor>();
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseLavaSubmergedBlockProcessor()
{
    // lava_submerged_block 处理器：岩浆淹没，无需额外配置
    return std::make_unique<LavaSubmergingProcessor>();
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseNopProcessor()
{
    // nop 处理器：空操作
    return std::make_unique<NopStructureProcessor>();
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseProtectedBlocksProcessor(
    simdjson::ondemand::object& processorObj)
{
    // protected_blocks 处理器：保护指定标签的方块不被结构覆盖
    // JSON: { "processor_type": "minecraft:protected_blocks", "value": "#minecraft:features_cannot_replace" }
    //
    // 处理逻辑：
    // - 读取 world->getBlockState(pos) 当前世界方块状态
    // - 若世界方块在保护标签内 → 跳过放置（保留世界方块）
    // - 否则 → 正常放置模板方块
    //
    // 边界情况：
    // - 缺少 value 字段 → 使用 Nop 占位（无法构造有意义的处理器）
    // - value 不是字符串 → 使用 Nop 占位
    // - value 不以 '#' 开头 → 仍尝试解析为 ResourceLocation（向后兼容）
    // - 标签不存在 → ProtectedBlocksProcessor 在 process() 时透传（视为空标签）

    auto valueStr = optString(processorObj, "value");
    if (!valueStr || valueStr->empty()) {
        spdlog::warn("protected_blocks processor: missing or invalid 'value' field, using nop processor");
        return std::make_unique<NopStructureProcessor>();
    }

    // value 字段为标签 ID，格式为 "#<namespace>:<path>" 或 "#<path>"
    // MC 原版 codec 要求 '#' 前缀，此处剥离 '#' 后解析为 ResourceLocation
    if ((*valueStr)[0] == '#') {
        *valueStr = valueStr->substr(1);
    }

    ResourceLocation tagId(*valueStr);
    if (tagId.namespace_().empty()) {
        spdlog::warn("protected_blocks processor: invalid tag id, using nop processor");
        return std::make_unique<NopStructureProcessor>();
    }

    return std::make_unique<ProtectedBlocksProcessor>(tagId);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseCappedProcessor(simdjson::ondemand::object& processorObj)
{
    // capped 处理器：限制内部处理器应用次数的上限
    // JSON: { "processor_type": "minecraft:capped", "delegate": {...}, "limit": <IntProvider> }
    // delegate 是嵌套的处理器定义，limit 是 IntProvider（支持固定整数或随机范围）
    auto delegateResult = processorObj["delegate"];
    if (delegateResult.error() != simdjson::SUCCESS) {
        spdlog::warn("capped processor: missing or invalid 'delegate' field, using nop processor");
        return std::make_unique<NopStructureProcessor>();
    }
    auto delegateObjResult = delegateResult.value().get_object();
    if (delegateObjResult.error() != simdjson::SUCCESS) {
        spdlog::warn("capped processor: 'delegate' is not an object, using nop processor");
        return std::make_unique<NopStructureProcessor>();
    }
    auto delegateObj = delegateObjResult.value();

    auto delegateProcessor = _parseProcessor(delegateObj);
    if (!delegateProcessor) {
        spdlog::warn("capped processor: failed to parse delegate processor, using nop processor");
        return std::make_unique<NopStructureProcessor>();
    }

    // 解析 limit 参数：支持裸整数和完整 IntProvider 格式
    // MC 原版使用 IntProvider.POSITIVE_CODEC（最小值 >= 1）
    std::unique_ptr<valueprovider::IntProvider> limitProvider;
    auto limitResult = processorObj["limit"];
    if (limitResult.error() == simdjson::SUCCESS) {
        // IntProviderParser 仍基于 nlohmann::json(被 8+ 个其他 loader 复用,本次不迁移)。
        // 在此边界用 raw_json() 取 limit 字段的原始 JSON 文本,转回 nlohmann::json 传给 IntProviderParser。
        // capped 处理器在原版数据中稀有,边界开销可接受。
        auto rawResult = limitResult.value().raw_json();
        if (rawResult.error() == simdjson::SUCCESS) {
            std::string_view rawJson = rawResult.value();
            try {
                nlohmann::json limitJson = nlohmann::json::parse(rawJson);
                // POSITIVE_CODEC 要求 minValue >= 1
                auto limitParseResult = valueprovider::IntProviderParser::parse(limitJson, 1);
                if (limitParseResult.success()) {
                    limitProvider = limitParseResult.value();
                } else {
                    spdlog::warn("capped processor: failed to parse limit IntProvider: {}, using default value 4",
                        limitParseResult.error().message());
                }
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("capped processor: failed to reparse limit JSON: {}, using default value 4", e.what());
            }
        }
    }

    if (!limitProvider) {
        limitProvider = std::make_unique<valueprovider::ConstantInt>(4);
    }

    return std::make_unique<CappedStructureProcessor>(std::move(delegateProcessor), std::move(limitProvider));
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
