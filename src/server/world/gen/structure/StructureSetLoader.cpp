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

#include "StructureSetLoader.hpp"

#include "StructureSet.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/json/JsonRead.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "server/world/gen/structure/placement/ConcentricRingsStructurePlacement.hpp"
#include "server/world/gen/structure/placement/RandomSpreadStructurePlacement.hpp"
#include "server/world/gen/structure/placement/StructurePlacement.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {

Result<size_t> StructureSetLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 structure_set 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/worldgen/structure_set";
        auto listResult = dataPackList.listResources(directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取结构集合名称
            // 路径格式: namespace/worldgen/structure_set/path/to/set.json
            std::string setName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            // 移除 .json 扩展名
            if (setName.size() >= 5 && setName.substr(setName.size() - 5) == ".json") {
                setName = setName.substr(0, setName.size() - 5);
            }

            ResourceLocation location(setName);

            // 读取 JSON 内容
            auto readResult = dataPackList.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read structure set: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse structure set {}: {}", setName, parseResult.error().message());
                continue;
            }

            // 注册结构集合
            StructureSetRegistry::instance().registerSet(parseResult.value());
            ++loadedCount;
        }
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} structure sets from datapacks", loadedCount);
    }

    return loadedCount;
}

Result<size_t> StructureSetLoader::loadFromResourcePack(const IResourcePack& pack)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 structure_set 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/worldgen/structure_set";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取结构集合名称
            std::string setName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            if (setName.size() >= 5 && setName.substr(setName.size() - 5) == ".json") {
                setName = setName.substr(0, setName.size() - 5);
            }

            ResourceLocation location(setName);

            // 读取 JSON 内容
            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read structure set: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse structure set {}: {}", setName, parseResult.error().message());
                continue;
            }

            // 注册结构集合
            StructureSetRegistry::instance().registerSet(parseResult.value());
            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<std::unique_ptr<StructureSet>> StructureSetLoader::loadFromJson(
    const std::string& json, const ResourceLocation& location)
{
    auto parsedJson = util::json::parse(json);
    if (parsedJson.failed()) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + parsedJson.error().message());
    }
    auto jsonObj = std::move(parsedJson).value();
    return loadFromJson(jsonObj, location);
}

Result<std::unique_ptr<StructureSet>> StructureSetLoader::loadFromJson(
    const nlohmann::json& jsonObj, const ResourceLocation& location)
{
    // 解析 structures 数组
    if (!jsonObj.contains("structures") || !jsonObj["structures"].is_array()) {
        return Error(ErrorCode::InvalidData, "Structure set missing 'structures' array");
    }

    std::vector<StructureSelectionEntry> entries;
    for (const auto& structEntry : jsonObj["structures"]) {
        if (!structEntry.contains("structure") || !structEntry["structure"].is_string()) {
            spdlog::warn("Structure set '{}': entry missing 'structure' string, skipping", location.toString());
            continue;
        }

        ResourceLocation structId(structEntry["structure"].get<std::string>());
        i32 weight = util::json::getInt32Or(structEntry, "weight", 1);
        if (weight <= 0) {
            weight = 1;
        }

        entries.emplace_back(std::move(structId), weight);
    }

    if (entries.empty()) {
        return Error(ErrorCode::InvalidData, "Structure set has no valid entries");
    }

    // 解析 placement 对象
    if (!jsonObj.contains("placement") || !jsonObj["placement"].is_object()) {
        return Error(ErrorCode::InvalidData, "Structure set missing 'placement' object");
    }

    auto placement = _parsePlacement(jsonObj["placement"]);
    if (!placement) {
        return Error(ErrorCode::InvalidData, "Failed to parse placement for structure set");
    }

    return std::make_unique<StructureSet>(location, std::move(entries), std::move(placement));
}

std::unique_ptr<placement::StructurePlacement> StructureSetLoader::_parsePlacement(const nlohmann::json& placementObj)
{
    if (!placementObj.contains("type") || !placementObj["type"].is_string()) {
        spdlog::warn("Placement missing 'type' string");
        return nullptr;
    }

    std::string type = placementObj["type"].get<std::string>();

    // 移除命名空间前缀
    if (type.size() > 10 && type.substr(0, 10) == "minecraft:") {
        type = type.substr(10);
    }

    if (type == "random_spread") {
        return _parseRandomSpreadPlacement(placementObj);
    } else if (type == "concentric_rings") {
        return _parseConcentricRingsPlacement(placementObj);
    } else {
        spdlog::warn("Unknown placement type: '{}'", type);
        return nullptr;
    }
}

std::unique_ptr<placement::StructurePlacement> StructureSetLoader::_parseRandomSpreadPlacement(
    const nlohmann::json& placementObj)
{
    // 解析必填参数
    const i64 salt = util::json::getInt64Or(placementObj, "salt", 0);

    const i32 spacing = util::json::getInt32Or(placementObj, "spacing", 1);

    const i32 separation = util::json::getInt32Or(placementObj, "separation", 0);

    // 解析分布类型
    placement::RandomSpreadType spreadType = placement::RandomSpreadType::Linear;
    if (placementObj.contains("spread_type") && placementObj["spread_type"].is_string()) {
        spreadType = _parseSpreadType(placementObj["spread_type"].get<std::string>());
    }

    // 解析频率缩减方法
    placement::FrequencyReductionMethod freqReduction = placement::FrequencyReductionMethod::Default;
    if (placementObj.contains("frequency_reduction_method") && placementObj["frequency_reduction_method"].is_string()) {
        freqReduction = _parseFrequencyReductionMethod(placementObj["frequency_reduction_method"].get<std::string>());
    }

    // 解析频率
    const f32 frequency = util::json::getFloat32Or(placementObj, "frequency", 1.0f);

    // 解析定位偏移
    math::Vector3i locateOffset(0, 0, 0);
    if (placementObj.contains("locate_offset") && placementObj["locate_offset"].is_array()) {
        const auto& offset = placementObj["locate_offset"];
        if (offset.size() >= 3 && offset[0].is_number_integer() && offset[1].is_number_integer() &&
            offset[2].is_number_integer()) {
            // 三个分量已确认是整数，越界时按 0 处理（本工厂无 Result 出口）
            const auto readOffset = [](const nlohmann::json& array, std::size_t index) -> i32 {
                const auto read = util::json::getInt32At(array, index);
                return read.success() ? read.value() : 0;
            };
            locateOffset = math::Vector3i(readOffset(offset, 0), readOffset(offset, 1), readOffset(offset, 2));
        }
    }

    // 解析排斥区
    auto exclusionZone = _parseExclusionZone(placementObj);

    return std::make_unique<placement::RandomSpreadStructurePlacement>(
        spacing, separation, salt, spreadType, freqReduction, frequency, locateOffset, std::move(exclusionZone));
}

std::unique_ptr<placement::StructurePlacement> StructureSetLoader::_parseConcentricRingsPlacement(
    const nlohmann::json& placementObj)
{
    const i32 distance = util::json::getInt32Or(placementObj, "distance", 32);

    const i32 spread = util::json::getInt32Or(placementObj, "spread", 3);

    const i32 count = util::json::getInt32Or(placementObj, "count", 128);

    // 解析首选生物群系（标签引用，如 "#minecraft:stronghold_biased_to"）。
    // 标签由 BiomeTagLoader 从数据包 tags/worldgen/biome/*.json 填充（须先于本加载器执行）。
    // 标签未加载或为空时 preferredBiomes 保持空（与硬编码兜底一致，要塞不做群系偏置）。
    std::vector<BiomeId> preferredBiomes;
    if (placementObj.contains("preferred_biomes")) {
        const auto& pb = placementObj["preferred_biomes"];
        if (pb.is_string()) {
            std::string tagRef = pb.get<std::string>();
            // 标签引用以 '#' 开头，去掉后得到 "namespace:path" 形式的 ResourceLocation。
            if (!tagRef.empty() && tagRef[0] == '#') {
                tagRef = tagRef.substr(1);
            }
            ResourceLocation tagLocation(tagRef);
            if (tagLocation.isValid()) {
                auto* tag = biome::BiomeTags::getTag(tagLocation);
                if (tag != nullptr) {
                    for (BiomeId id : tag->getBiomeIds()) {
                        preferredBiomes.push_back(id);
                    }
                    spdlog::info("ConcentricRings preferred_biomes: resolved {} biomes from tag '{}'",
                        preferredBiomes.size(),
                        tagLocation.toString());
                } else {
                    spdlog::warn("ConcentricRings preferred_biomes: tag '{}' not registered (empty preferredBiomes)",
                        tagLocation.toString());
                }
            }
        }
    }

    i64 salt = 0;
    if (placementObj.contains("salt") && placementObj["salt"].is_number_integer()) {
        salt = util::json::getInt64Or(placementObj, "salt", 0);
    }

    // 解析定位偏移
    math::Vector3i locateOffset(0, 0, 0);
    if (placementObj.contains("locate_offset") && placementObj["locate_offset"].is_array()) {
        const auto& offset = placementObj["locate_offset"];
        if (offset.size() >= 3 && offset[0].is_number_integer() && offset[1].is_number_integer() &&
            offset[2].is_number_integer()) {
            const auto readOffset = [](const nlohmann::json& array, std::size_t index) -> i32 {
                const auto read = util::json::getInt32At(array, index);
                return read.success() ? read.value() : 0;
            };
            locateOffset = math::Vector3i(readOffset(offset, 0), readOffset(offset, 1), readOffset(offset, 2));
        }
    }

    return std::make_unique<placement::ConcentricRingsStructurePlacement>(
        distance, spread, count, std::move(preferredBiomes), salt, locateOffset);
}

placement::FrequencyReductionMethod StructureSetLoader::_parseFrequencyReductionMethod(const std::string& method)
{
    // 移除命名空间前缀
    std::string m = method;
    if (m.size() > 10 && m.substr(0, 10) == "minecraft:") {
        m = m.substr(10);
    }

    if (m == "legacy_type_1" || m == "legacy_type_1_with_exception") {
        return placement::FrequencyReductionMethod::LegacyType1;
    } else if (m == "legacy_type_2") {
        return placement::FrequencyReductionMethod::LegacyType2;
    } else if (m == "legacy_type_3") {
        return placement::FrequencyReductionMethod::LegacyType3;
    } else {
        return placement::FrequencyReductionMethod::Default;
    }
}

placement::RandomSpreadType StructureSetLoader::_parseSpreadType(const std::string& spreadType)
{
    std::string s = spreadType;
    if (s.size() > 10 && s.substr(0, 10) == "minecraft:") {
        s = s.substr(10);
    }

    if (s == "triangular") {
        return placement::RandomSpreadType::Triangular;
    } else {
        return placement::RandomSpreadType::Linear;
    }
}

std::optional<placement::ExclusionZone> StructureSetLoader::_parseExclusionZone(const nlohmann::json& placementObj)
{
    if (!placementObj.contains("exclusion_zone") || !placementObj["exclusion_zone"].is_object()) {
        return std::nullopt;
    }

    const auto& zone = placementObj["exclusion_zone"];

    if (!zone.contains("other_set") || !zone["other_set"].is_string()) {
        spdlog::warn("Exclusion zone missing 'other_set' string");
        return std::nullopt;
    }

    ResourceLocation otherSetId(zone["other_set"].get<std::string>());

    i32 chunkCount = util::json::getInt32Or(zone, "chunk_count", 8);
    // 限制搜索半径范围 1-16
    if (chunkCount < 1) {
        chunkCount = 1;
    } else if (chunkCount > 16) {
        chunkCount = 16;
    }

    return placement::ExclusionZone{std::move(otherSetId), chunkCount};
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
