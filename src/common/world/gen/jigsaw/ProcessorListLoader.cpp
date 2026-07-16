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
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/template/CappedStructureProcessor.hpp"
#include "common/world/gen/feature/template/ProtectedBlocksProcessor.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <nlohmann/json.hpp>
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
u32 parseOutputBlockStateId(const nlohmann::json& outputState)
{
    if (!outputState.contains("Name") || !outputState["Name"].is_string()) {
        return 0;
    }

    std::string blockName = outputState["Name"].get<std::string>();
    ResourceLocation blockLoc(blockName);

    Block* block = BlockRegistry::instance().getBlock(blockLoc);
    if (block == nullptr) {
        spdlog::warn("ProcessorListLoader: unknown block '{}' in output_state, using air", blockName);
        return 0;
    }

    const BlockState& defaultState = block->defaultState();

    // 没有 Properties，直接使用默认状态
    if (!outputState.contains("Properties") || !outputState["Properties"].is_object()) {
        return defaultState.stateId();
    }

    // 有 Properties，逐一设置属性
    const auto& props = outputState["Properties"];
    const BlockState* currentState = &defaultState;

    for (const auto& [propName, propValue] : props.items()) {
        if (!propValue.is_string()) {
            continue;
        }

        const IProperty* prop = block->stateContainer().getProperty(propName);
        if (prop == nullptr) {
            spdlog::warn("ProcessorListLoader: unknown property '{}' on block '{}'", propName, blockName);
            continue;
        }

        auto valueIndex = prop->parseValue(propValue.get<std::string>());
        if (!valueIndex.has_value()) {
            spdlog::warn("ProcessorListLoader: invalid value '{}' for property '{}' on block '{}'",
                propValue.get<std::string>(),
                propName,
                blockName);
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
const BlockState* parseOutputBlockState(const nlohmann::json& outputState)
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
std::unique_ptr<RuleTest> parseRuleTest(const nlohmann::json& predicateObj)
{
    if (!predicateObj.contains("predicate_type") || !predicateObj["predicate_type"].is_string()) {
        spdlog::warn("ProcessorListLoader: predicate missing 'predicate_type'");
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    std::string predicateType = predicateObj["predicate_type"].get<std::string>();
    predicateType = stripMinecraftPrefix(predicateType);

    if (predicateType == "always_true") {
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "block_match") {
        if (predicateObj.contains("block") && predicateObj["block"].is_string()) {
            ResourceLocation blockLoc(predicateObj["block"].get<std::string>());
            Block* block = BlockRegistry::instance().getBlock(blockLoc);
            if (block != nullptr) {
                return std::make_unique<BlockMatchRuleTest>(block);
            }
            spdlog::warn("ProcessorListLoader: block_match: unknown block '{}'", blockLoc.toString());
        }
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "random_block_match") {
        if (predicateObj.contains("block") && predicateObj["block"].is_string() &&
            predicateObj.contains("probability") && predicateObj["probability"].is_number()) {
            ResourceLocation blockLoc(predicateObj["block"].get<std::string>());
            f32 probability = predicateObj["probability"].get<f32>();
            Block* block = BlockRegistry::instance().getBlock(blockLoc);
            if (block != nullptr) {
                return std::make_unique<RandomBlockMatchRuleTest>(block, probability);
            }
            spdlog::warn("ProcessorListLoader: random_block_match: unknown block '{}'", blockLoc.toString());
        }
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "tag_match") {
        if (predicateObj.contains("tag") && predicateObj["tag"].is_string()) {
            std::string tag = predicateObj["tag"].get<std::string>();
            return std::make_unique<TagMatchRuleTest>(ResourceLocation(tag));
        }
        spdlog::warn("ProcessorListLoader: tag_match: missing 'tag' field");
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "block_state_match" || predicateType == "blockstate_match") {
        if (predicateObj.contains("block_state") && predicateObj["block_state"].is_object()) {
            const BlockState* state = parseOutputBlockState(predicateObj["block_state"]);
            if (state != nullptr) {
                return std::make_unique<BlockStateMatchRuleTest>(state);
            }
            spdlog::warn("ProcessorListLoader: block_state_match: unknown block state");
        }
        return std::make_unique<AlwaysTrueRuleTest>();
    }

    if (predicateType == "random_block_state_match") {
        if (predicateObj.contains("block_state") && predicateObj["block_state"].is_object() &&
            predicateObj.contains("probability") && predicateObj["probability"].is_number()) {
            const BlockState* state = parseOutputBlockState(predicateObj["block_state"]);
            f32 probability = predicateObj["probability"].get<f32>();
            if (state != nullptr) {
                return std::make_unique<RandomBlockStateMatchRuleTest>(state, probability);
            }
            spdlog::warn("ProcessorListLoader: random_block_state_match: unknown block state");
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
std::unique_ptr<feature::template_::PosRuleTest> _parsePosRuleTest(const nlohmann::json& predicateObj)
{
    if (!predicateObj.contains("predicate_type") || !predicateObj["predicate_type"].is_string()) {
        spdlog::warn("ProcessorListLoader: pos_predicate missing 'predicate_type', using always_true");
        return std::make_unique<AlwaysTruePosRuleTest>();
    }

    std::string predicateType = predicateObj["predicate_type"].get<std::string>();
    predicateType = stripMinecraftPrefix(predicateType);

    if (predicateType == "always_true") {
        return std::make_unique<AlwaysTruePosRuleTest>();
    }

    if (predicateType == "linear_pos") {
        // min_chance/max_chance (f32, 默认 0.0), min_dist/max_dist (i32, 默认 0)
        f32 minChance = 0.0f;
        f32 maxChance = 0.0f;
        i32 minDist = 0;
        i32 maxDist = 0;

        if (predicateObj.contains("min_chance") && predicateObj["min_chance"].is_number()) {
            minChance = predicateObj["min_chance"].get<f32>();
        }
        if (predicateObj.contains("max_chance") && predicateObj["max_chance"].is_number()) {
            maxChance = predicateObj["max_chance"].get<f32>();
        }
        if (predicateObj.contains("min_dist") && predicateObj["min_dist"].is_number()) {
            minDist = predicateObj["min_dist"].get<i32>();
        }
        if (predicateObj.contains("max_dist") && predicateObj["max_dist"].is_number()) {
            maxDist = predicateObj["max_dist"].get<i32>();
        }

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
        f32 minChance = 0.0f;
        f32 maxChance = 0.0f;
        i32 minDist = 0;
        i32 maxDist = 0;
        Axis axis = Axis::Y; // 默认轴为 Y

        if (predicateObj.contains("min_chance") && predicateObj["min_chance"].is_number()) {
            minChance = predicateObj["min_chance"].get<f32>();
        }
        if (predicateObj.contains("max_chance") && predicateObj["max_chance"].is_number()) {
            maxChance = predicateObj["max_chance"].get<f32>();
        }
        if (predicateObj.contains("min_dist") && predicateObj["min_dist"].is_number()) {
            minDist = predicateObj["min_dist"].get<i32>();
        }
        if (predicateObj.contains("max_dist") && predicateObj["max_dist"].is_number()) {
            maxDist = predicateObj["max_dist"].get<i32>();
        }
        if (predicateObj.contains("axis") && predicateObj["axis"].is_string()) {
            std::string axisStr = predicateObj["axis"].get<std::string>();
            if (axisStr == "x") {
                axis = Axis::X;
            } else if (axisStr == "y") {
                axis = Axis::Y;
            } else if (axisStr == "z") {
                axis = Axis::Z;
            } else {
                spdlog::warn(
                    "ProcessorListLoader: axis_aligned_linear_pos unknown axis '{}', defaulting to Y", axisStr);
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
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);
        return _loadFromJsonObj(jsonObj, location);
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse JSON: ") + e.what());
    }
}

Result<void> ProcessorListLoader::_loadFromJsonObj(const nlohmann::json& jsonObj, const ResourceLocation& location)
{
    // 解析 processors 数组
    if (!jsonObj.contains("processors") || !jsonObj["processors"].is_array()) {
        return Error(ErrorCode::InvalidData, "Processor list missing 'processors' array");
    }

    auto processorList = std::make_unique<StructureProcessorList>();
    i32 processorCount = 0;

    for (const auto& processorObj : jsonObj["processors"]) {
        auto processor = _parseProcessor(processorObj);
        if (processor) {
            processorList->addProcessor(std::move(processor));
            ++processorCount;
        }
    }

    // 注册处理器列表
    ProcessorListRegistry::instance().registerList(location, *processorList);

    spdlog::info("Processor list '{}': {} processors", location.toString(), processorCount);
    return Result<void>::ok();
}

std::unique_ptr<StructureProcessorList> ProcessorListLoader::parseInlineProcessorList(
    const nlohmann::json& processorsArray)
{
    auto processorList = std::make_unique<StructureProcessorList>();

    if (!processorsArray.is_array()) {
        return processorList; // 空列表
    }

    for (const auto& processorObj : processorsArray) {
        auto processor = _parseProcessor(processorObj);
        if (processor) {
            processorList->addProcessor(std::move(processor));
        }
    }

    return processorList;
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseProcessor(const nlohmann::json& processorObj)
{
    if (!processorObj.contains("processor_type") || !processorObj["processor_type"].is_string()) {
        spdlog::warn("Processor missing 'processor_type' string");
        return nullptr;
    }

    std::string type = processorObj["processor_type"].get<std::string>();
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

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseBlockIgnoreProcessor(const nlohmann::json& processorObj)
{
    // block_ignore 处理器：忽略指定方块列表
    // JSON: { "processor_type": "minecraft:block_ignore", "blocks": [...] }
    std::vector<u32> blocksToIgnore;

    if (processorObj.contains("blocks") && processorObj["blocks"].is_array()) {
        for (const auto& blockEntry : processorObj["blocks"]) {
            if (blockEntry.is_string()) {
                // 方块名称字符串，通过方块注册表解析
                ResourceLocation blockLoc(blockEntry.get<std::string>());
                Block* block = BlockRegistry::instance().getBlock(blockLoc);
                if (block != nullptr) {
                    blocksToIgnore.push_back(block->defaultState().stateId());
                } else {
                    spdlog::info("block_ignore: unknown block '{}' in ignore list, skipping", blockLoc.toString());
                }
            } else if (blockEntry.is_object()) {
                // 完整方块状态对象
                u32 stateId = parseOutputBlockStateId(blockEntry);
                if (stateId != 0) {
                    blocksToIgnore.push_back(stateId);
                }
            }
        }
    }

    return std::make_unique<BlockIgnoreStructureProcessor>(blocksToIgnore);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseBlockRotProcessor(const nlohmann::json& processorObj)
{
    // block_rot 处理器（IntegrityProcessor）：完整性衰减
    // JSON: { "processor_type": "minecraft:block_rot", "integrity": 0.5 }
    f32 integrity = 1.0f;
    if (processorObj.contains("integrity") && processorObj["integrity"].is_number()) {
        integrity = processorObj["integrity"].get<f32>();
        // 限制在 [0.0, 1.0] 范围
        if (integrity < 0.0f) {
            integrity = 0.0f;
        } else if (integrity > 1.0f) {
            integrity = 1.0f;
        }
    }

    return std::make_unique<IntegrityProcessor>(integrity);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseGravityProcessor(const nlohmann::json& processorObj)
{
    // gravity 处理器：重力偏移
    // JSON: { "processor_type": "minecraft:gravity", "heightmap": "WORLD_SURFACE_WG" }
    i32 heightmapType = 0; // 默认 WORLD_SURFACE_WG
    if (processorObj.contains("heightmap") && processorObj["heightmap"].is_string()) {
        std::string heightmap = stripMinecraftPrefix(processorObj["heightmap"].get<std::string>());

        if (heightmap == "WORLD_SURFACE_WG" || heightmap == "world_surface_wg") {
            heightmapType = 0;
        } else if (heightmap == "OCEAN_FLOOR_WG" || heightmap == "ocean_floor_wg") {
            heightmapType = 1;
        } else if (heightmap == "MOTION_BLOCKING" || heightmap == "motion_blocking") {
            heightmapType = 2;
        } else {
            spdlog::info("gravity: unknown heightmap '{}', defaulting to WORLD_SURFACE_WG", heightmap);
        }
    }

    return std::make_unique<GravityStructureProcessor>(heightmapType);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseJigsawReplacementProcessor()
{
    // jigsaw_replacement 处理器：Jigsaw 替换，无需额外配置
    return std::make_unique<JigsawReplacementStructureProcessor>();
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseRuleProcessor(const nlohmann::json& processorObj)
{
    // rule 处理器：规则处理器
    // JSON: { "processor_type": "minecraft:rule", "rules": [...] }
    // 每条规则：{ "input_predicate": {...}, "location_predicate": {...}, "output_state": {...} }
    std::vector<std::unique_ptr<RuleEntry>> rules;

    if (!processorObj.contains("rules") || !processorObj["rules"].is_array()) {
        spdlog::info("rule processor: no rules array, creating empty rule processor");
        return std::make_unique<RuleStructureProcessor>(std::move(rules));
    }

    for (const auto& ruleObj : processorObj["rules"]) {
        // 解析 input_predicate
        std::unique_ptr<RuleTest> inputPredicate;
        if (ruleObj.contains("input_predicate") && ruleObj["input_predicate"].is_object()) {
            inputPredicate = parseRuleTest(ruleObj["input_predicate"]);
        } else {
            inputPredicate = std::make_unique<AlwaysTrueRuleTest>();
        }

        // 解析 location_predicate
        std::unique_ptr<RuleTest> locationPredicate;
        if (ruleObj.contains("location_predicate") && ruleObj["location_predicate"].is_object()) {
            locationPredicate = parseRuleTest(ruleObj["location_predicate"]);
        } else {
            locationPredicate = std::make_unique<AlwaysTrueRuleTest>();
        }

        // 解析 pos_predicate（可选）
        std::unique_ptr<feature::template_::PosRuleTest> posPredicate;
        if (ruleObj.contains("pos_predicate") && ruleObj["pos_predicate"].is_object()) {
            posPredicate = _parsePosRuleTest(ruleObj["pos_predicate"]);
        } else {
            posPredicate = std::make_unique<AlwaysTruePosRuleTest>();
        }

        // 解析 output_state
        u32 outputStateId = 0;
        if (ruleObj.contains("output_state") && ruleObj["output_state"].is_object()) {
            outputStateId = parseOutputBlockStateId(ruleObj["output_state"]);
        }

        // 解析 output_nbt / block_entity_modifier（可选，暂不实现）
        std::optional<nbt::tags::compound_tag> outputNbt;

        auto ruleEntry = std::make_unique<RuleEntry>(
            std::move(inputPredicate), std::move(locationPredicate), std::move(posPredicate), outputStateId, outputNbt);
        rules.push_back(std::move(ruleEntry));
    }

    return std::make_unique<RuleStructureProcessor>(std::move(rules));
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseBlockAgeProcessor(const nlohmann::json& processorObj)
{
    // block_age 处理器（苔藓化）
    // JSON: { "processor_type": "minecraft:block_age", "mossiness": 0.5 }
    f32 mossiness = 0.0f;
    if (processorObj.contains("mossiness") && processorObj["mossiness"].is_number()) {
        mossiness = processorObj["mossiness"].get<f32>();
        if (mossiness < 0.0f) {
            mossiness = 0.0f;
        } else if (mossiness > 1.0f) {
            mossiness = 1.0f;
        }
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
    const nlohmann::json& processorObj)
{
    // protected_blocks 处理器：保护指定标签的方块不被结构覆盖
    // JSON: { "processor_type": "minecraft:protected_blocks", "value": "#minecraft:features_cannot_replace" }
    //
    // 对应 MC Java 1.21.11 net.minecraft.world.level.levelgen.structure.templatesystem.ProtectedBlockProcessor
    // value 字段为标签 ID 字符串，必须以 '#' 前缀标识（MC codec 使用 TagKey.hashedCodec 解析）
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

    if (!processorObj.contains("value") || !processorObj["value"].is_string()) {
        spdlog::warn("protected_blocks processor: missing or invalid 'value' field, using nop processor");
        return std::make_unique<NopStructureProcessor>();
    }

    std::string valueStr = processorObj["value"].get<std::string>();

    // value 字段为标签 ID，格式为 "#<namespace>:<path>" 或 "#<path>"
    // MC 原版 codec 要求 '#' 前缀，此处剥离 '#' 后解析为 ResourceLocation
    if (!valueStr.empty() && valueStr[0] == '#') {
        valueStr = valueStr.substr(1);
    }

    ResourceLocation tagId(valueStr);
    if (tagId.namespace_().empty()) {
        spdlog::warn("protected_blocks processor: invalid tag id '{}', using nop processor",
            processorObj["value"].get<std::string>());
        return std::make_unique<NopStructureProcessor>();
    }

    return std::make_unique<ProtectedBlocksProcessor>(tagId);
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseCappedProcessor(const nlohmann::json& processorObj)
{
    // capped 处理器：限制内部处理器应用次数的上限
    // JSON: { "processor_type": "minecraft:capped", "delegate": {...}, "limit": <IntProvider> }
    // delegate 是嵌套的处理器定义，limit 是 IntProvider（支持固定整数或随机范围）
    if (!processorObj.contains("delegate") || !processorObj["delegate"].is_object()) {
        spdlog::warn("capped processor: missing or invalid 'delegate' field, using nop processor");
        return std::make_unique<NopStructureProcessor>();
    }

    auto delegateProcessor = _parseProcessor(processorObj["delegate"]);
    if (!delegateProcessor) {
        spdlog::warn("capped processor: failed to parse delegate processor, using nop processor");
        return std::make_unique<NopStructureProcessor>();
    }

    // 解析 limit 参数：支持裸整数和完整 IntProvider 格式
    // MC 原版使用 IntProvider.POSITIVE_CODEC（最小值 >= 1）
    std::unique_ptr<valueprovider::IntProvider> limitProvider;
    if (processorObj.contains("limit")) {
        // POSITIVE_CODEC 要求 minValue >= 1
        auto limitResult = valueprovider::IntProviderParser::parse(processorObj["limit"], 1);
        if (limitResult.success()) {
            limitProvider = limitResult.value();
        } else {
            spdlog::warn("capped processor: failed to parse limit IntProvider: {}, using default value 4",
                limitResult.error().message());
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
