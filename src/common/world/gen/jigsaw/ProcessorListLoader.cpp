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

#include "common/resource/DataPackList.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/resource/PackType.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// 导入处理器类型，简化代码
using feature::template_::BlackstoneReplacementProcessor;
using feature::template_::BlockAgeProcessor;
using feature::template_::BlockIgnoreStructureProcessor;
using feature::template_::GravityStructureProcessor;
using feature::template_::IntegrityProcessor;
using feature::template_::JigsawReplacementStructureProcessor;
using feature::template_::LavaSubmergingProcessor;
using feature::template_::NopStructureProcessor;
using feature::template_::RuleStructureProcessor;
using feature::template_::StructureProcessor;
using feature::template_::StructureProcessorList;

Result<size_t> ProcessorListLoader::loadFromDataPackList(const resource::DataPackList& dataPackList)
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

            // 读取 JSON 内容
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

            // 读取 JSON 内容
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

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseProcessor(const nlohmann::json& processorObj)
{
    if (!processorObj.contains("processor_type") || !processorObj["processor_type"].is_string()) {
        spdlog::warn("Processor missing 'processor_type' string");
        return nullptr;
    }

    std::string type = processorObj["processor_type"].get<std::string>();

    // 移除命名空间前缀
    if (type.size() > 10 && type.substr(0, 10) == "minecraft:") {
        type = type.substr(10);
    }

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
                // 方块名称字符串，后续需要通过注册表解析为 ID
                // 目前记录为 0（空气），等待方块注册表完善后替换
                spdlog::info(
                    "block_ignore: block '{}' parsing deferred to block registry", blockEntry.get<std::string>());
            } else if (blockEntry.is_object()) {
                // 完整方块状态对象，后续解析
                spdlog::info("block_ignore: block state object parsing deferred");
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
        std::string heightmap = processorObj["heightmap"].get<std::string>();
        if (heightmap.size() > 10 && heightmap.substr(0, 10) == "minecraft:") {
            heightmap = heightmap.substr(10);
        }

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
    // 规则解析较为复杂，后续实现
    // 目前创建空规则列表的 RuleStructureProcessor
    spdlog::info("rule processor: rules parsing deferred, creating empty rule processor");

    std::vector<std::unique_ptr<feature::template_::RuleEntry>> rules;

    if (processorObj.contains("rules") && processorObj["rules"].is_array()) {
        spdlog::info("rule processor: {} rules skipped (parsing not yet implemented)", processorObj["rules"].size());
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
    // protected_blocks 处理器：保护方块
    // JSON: { "processor_type": "minecraft:protected_blocks", "value": "#minecraft:features_cannot_replace" }
    // 需要标签系统支持，目前用 nop 处理器占位
    spdlog::info("protected_blocks processor: tag parsing deferred, using nop processor");
    return std::make_unique<NopStructureProcessor>();
}

std::unique_ptr<StructureProcessor> ProcessorListLoader::_parseCappedProcessor(const nlohmann::json& processorObj)
{
    // capped 处理器：上限处理器
    // JSON: { "processor_type": "minecraft:capped", "delegate": {...}, "limit": 4 }
    // 需要递归解析 delegate，目前用 nop 处理器占位
    spdlog::info("capped processor: delegate parsing deferred, using nop processor");
    return std::make_unique<NopStructureProcessor>();
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
