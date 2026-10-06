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

#include "GenericTagLoader.hpp"

#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <exception>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::resource::tag {

namespace {

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 标签条目的原始数据（未解析引用）
 */
struct _RawTagEntry {
    std::string id;       ///< 条目标识符（成员名或 # 标签引用）
    bool required = true; ///< 是否必须存在
};

/**
 * @brief 单个标签文件的原始解析数据（未解析引用）
 */
struct _RawTagData {
    bool replace = false;              ///< 数据包 replace 语义标志
    std::vector<_RawTagEntry> entries; ///< 原始条目列表
};

/**
 * @brief 多数据包合并后的标签数据（未解析引用）
 */
struct _MergedTagData {
    bool replace = false;              ///< 合并后的 replace 标志
    std::vector<_RawTagEntry> entries; ///< 合并后的条目列表
};

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 解析 JSON 字符串为原始标签数据（第一阶段：不解析引用）
 */
Result<_RawTagData> _parseJsonRaw(const std::string& json, const ResourceLocation& location)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);

        _RawTagData rawData;

        // 解析 replace 字段（可选，默认 false）
        if (jsonObj.contains("replace") && jsonObj["replace"].is_boolean()) {
            rawData.replace = jsonObj["replace"].get<bool>();
        }

        // 解析 values 数组
        if (!jsonObj.contains("values") || !jsonObj["values"].is_array()) {
            return Error(ErrorCode::InvalidData, "tag missing 'values' array");
        }

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                // 字符串格式: "minecraft:stone" 或 "#minecraft:base_stone_overworld"
                // 字符串格式默认 required=true
                rawData.entries.push_back({value.get<std::string>(), true});
            } else if (value.is_object()) {
                // 对象格式: {"id":"minecraft:stone","required":false}
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::error(
                        "GenericTagLoader: object entry in tag '{}' missing 'id' field, skipped", location.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::error(
                        "GenericTagLoader: object entry 'id' in tag '{}' is empty, skipped", location.toString());
                    continue;
                }

                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                rawData.entries.push_back({id, required});
            } else {
                spdlog::error(
                    "GenericTagLoader: value in tag '{}' is not a string or object, skipped", location.toString());
            }
        }

        return rawData;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("failed to parse JSON: ") + e.what());
    }
}

/**
 * @brief 解析单个条目为成员索引（解析引用阶段）
 *
 * 标签引用 (#namespace:path) 从 readTag 回调查找被引用标签的成员并展开；
 * 普通成员经 resolveMember 回调解析为成员索引。
 */
void _resolveEntry(const _RawTagEntry& entry,
    std::vector<std::size_t>& members,
    std::unordered_set<ResourceLocation>& visitedTags,
    const ResourceLocation& tagLocation,
    const TagMemberResolver& resolveMember,
    const TagMemberReader& readTag)
{
    if (entry.id.empty()) {
        // 空条目 id：解析阶段已对对象条目过滤空 id，走到这里只可能是字符串条目为 ""，属数据包异常。
        spdlog::error("GenericTagLoader: empty entry id in tag '{}', skipped", tagLocation.toString());
        return;
    }

    if (entry.id[0] == '#') {
        // 标签引用: #namespace:path
        std::string tagRef = entry.id.substr(1);
        ResourceLocation tagRefLocation = ResourceLocation::parse(tagRef);

        // 防止循环引用
        if (visitedTags.count(tagRefLocation) > 0) {
            if (entry.required) {
                spdlog::warn("GenericTagLoader: circular tag reference '{}' (required), skipped (tag: {})",
                    entry.id,
                    tagLocation.toString());
            }
            return;
        }
        visitedTags.insert(tagRefLocation);

        // 查找被引用的标签成员（被引用标签已由依赖解析保证先填充）
        std::vector<std::size_t> referencedMembers = readTag(tagRefLocation);
        if (!referencedMembers.empty()) {
            for (std::size_t member : referencedMembers) {
                members.push_back(member);
            }
        } else if (entry.required) {
            spdlog::warn("GenericTagLoader: referenced tag '{}' resolved no members (required), skipped (tag: {})",
                entry.id,
                tagLocation.toString());
        }
    } else {
        // 直接成员名
        std::size_t member = resolveMember(entry.id);
        if (member != TAG_MEMBER_NOT_FOUND) {
            members.push_back(member);
        } else if (entry.required) {
            spdlog::warn("GenericTagLoader: unknown member '{}' (required), skipped (tag: {})",
                entry.id,
                tagLocation.toString());
        }
        // required=false 时静默跳过
    }
}

/**
 * @brief 将合并后的标签数据解析并写入目标标签
 */
void _resolveAndFill(const ResourceLocation& location,
    const _MergedTagData& data,
    const TagMemberResolver& resolveMember,
    const TagMemberReader& readTag,
    const TagFiller& fillTag)
{
    if (data.entries.empty()) {
        // 空标签（可能因 replace 清空）仍须写入以应用 replace 语义。
        // 数据包侧出现空标签极不寻常（vanilla 无），明确告警以便定位。
        spdlog::warn("GenericTagLoader: tag '{}' has NO entries; writing empty tag", location.toString());
        fillTag(location, {}, data.replace);
        return;
    }

    std::vector<std::size_t> members;
    std::unordered_set<ResourceLocation> visitedTags;
    visitedTags.insert(location); // 防止自引用

    for (const auto& entry : data.entries) {
        _resolveEntry(entry, members, visitedTags, location, resolveMember, readTag);
    }

    if (members.empty()) {
        // 所有条目都解析失败：写出的标签为空，依赖它的标签也会连带为空，明确 error 而非 info。
        spdlog::error("GenericTagLoader: tag '{}' resolved ZERO members (all entries unresolved); "
                      "dependent tags will inherit empty members",
            location.toString());
    }

    fillTag(location, members, data.replace);
}

/**
 * @brief 递归解析标签及其依赖（第二阶段：带依赖顺序的解析）
 *
 * 当标签 A 引用 #B 时，需要先确保 B 的内容已被填充，
 * 否则 A 从 B 引用到的成员列表为空。
 */
void _resolveWithDependencies(const ResourceLocation& location,
    std::unordered_map<ResourceLocation, _MergedTagData>& mergedTags,
    std::unordered_set<ResourceLocation>& resolved,
    std::unordered_set<ResourceLocation>& resolving,
    const TagMemberResolver& resolveMember,
    const TagMemberReader& readTag,
    const TagFiller& fillTag)
{
    // 已经解析过，无需重复处理
    if (resolved.count(location) > 0) {
        return;
    }

    // 不在本次数据包加载范围内，可能是内置标签或不存在
    auto it = mergedTags.find(location);
    if (it == mergedTags.end()) {
        return;
    }

    // 检测循环依赖
    if (resolving.count(location) > 0) {
        spdlog::warn("GenericTagLoader: circular tag dependency detected '{}', skipped", location.toString());
        return;
    }

    resolving.insert(location);

    const auto& data = it->second;

    // 先递归解析所有 # 标签引用的依赖
    for (const auto& entry : data.entries) {
        if (!entry.id.empty() && entry.id[0] == '#') {
            std::string tagRef = entry.id.substr(1);
            ResourceLocation tagRefLocation = ResourceLocation::parse(tagRef);
            _resolveWithDependencies(tagRefLocation, mergedTags, resolved, resolving, resolveMember, readTag, fillTag);
        }
    }

    // 所有依赖已解析，现在解析当前标签
    _resolveAndFill(location, data, resolveMember, readTag, fillTag);

    resolving.erase(location);
    resolved.insert(location);
}

/**
 * @brief 从路径提取标签名称
 *
 * 路径格式: namespace/tags/block/subdir/xxx.json -> namespace:subdir/xxx
 */
ResourceLocation _extractTagLocation(
    const std::string& namespace_, std::string_view directory, const std::string& resourcePath)
{
    std::string tagName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
    // 移除 .json 扩展名
    if (tagName.size() >= 5 && tagName.substr(tagName.size() - 5) == ".json") {
        tagName = tagName.substr(0, tagName.size() - 5);
    }
    return ResourceLocation(tagName);
}

/**
 * @brief 第二阶段公共收尾：注册空占位、按依赖顺序解析并填充
 */
void _finalize(std::unordered_map<ResourceLocation, _MergedTagData>& mergedTags,
    const TagMemberResolver& resolveMember,
    const TagMemberReader& readTag,
    const TagFiller& fillTag)
{
    // 2a. 先注册所有尚不存在的标签（空标签），确保标签引用可以找到目标
    for (const auto& [location, data] : mergedTags) {
        fillTag(location, {}, false);
    }

    // 2b. 按依赖顺序解析所有标签的引用并填充内容
    std::unordered_set<ResourceLocation> resolved;
    std::unordered_set<ResourceLocation> resolving;
    for (auto& [location, data] : mergedTags) {
        _resolveWithDependencies(location, mergedTags, resolved, resolving, resolveMember, readTag, fillTag);
    }
}

/**
 * @brief 将同一资源路径的一个 JSON 内容合并进 mergedTags
 */
void _mergeInto(std::unordered_map<ResourceLocation, _MergedTagData>& mergedTags,
    const ResourceLocation& location,
    const _RawTagData& rawData)
{
    auto& existing = mergedTags[location];
    if (rawData.replace) {
        // replace=true：清空已有条目，使用当前数据包的内容
        existing.replace = true;
        existing.entries.clear();
    }
    for (const auto& entry : rawData.entries) {
        existing.entries.push_back(entry);
    }
}

} // namespace

Result<size_t> GenericTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList,
    std::string_view tagDirectory,
    const TagMemberResolver& resolveMember,
    const TagMemberReader& readTag,
    const TagFiller& fillTag)
{
    size_t loadedCount = 0;

    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        // 枚举命名空间失败：整个目录的标签都不会被加载，静默返回 0 会让上层误以为"无标签"，
        // 明确 error 报告失败原因。
        spdlog::error("GenericTagLoader: getResourceNamespaces failed for '{}': {}; NO tags loaded",
            std::string(tagDirectory),
            namespacesResult.error().message());
        return loadedCount;
    }

    // 第一阶段：解析所有 JSON 文件，收集原始条目数据（不解析 # 标签引用）
    std::unordered_map<ResourceLocation, _MergedTagData> mergedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/" + std::string(tagDirectory);
        auto stacksResult = dataPackList.listResourceStacks(directory, ".json");

        if (!stacksResult.success()) {
            continue;
        }

        for (auto& [resourcePath, versions] : stacksResult.value()) {
            ResourceLocation location = _extractTagLocation(namespace_, directory, resourcePath);

            // listResourceStacks 返回的版本按优先级从高到低排序，
            // 标签加载语义是先处理低优先级数据包，后处理高优先级数据包，
            // 故逆序遍历。
            for (auto it = versions.rbegin(); it != versions.rend(); ++it) {
                auto& version = *it;

                auto parseResult = _parseJsonRaw(version.content, location);
                if (!parseResult.success()) {
                    spdlog::error("GenericTagLoader: failed to parse tag {} (from datapack {}): {}",
                        location.toString(),
                        version.packName,
                        parseResult.error().message());
                    continue;
                }

                _mergeInto(mergedTags, location, parseResult.value());
            }
        }
    }

    // 第二阶段：注册空占位并解析引用
    _finalize(mergedTags, resolveMember, readTag, fillTag);
    loadedCount = mergedTags.size();

    if (loadedCount > 0) {
        spdlog::info("GenericTagLoader: loaded {} tags from '{}'", loadedCount, std::string(tagDirectory));
    }

    return loadedCount;
}

Result<size_t> GenericTagLoader::loadFromResourcePack(const resource::IResourcePack& pack,
    std::string_view tagDirectory,
    const TagMemberResolver& resolveMember,
    const TagMemberReader& readTag,
    const TagFiller& fillTag)
{
    size_t loadedCount = 0;

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        spdlog::error("GenericTagLoader: getResourceNamespaces failed for '{}': {}; NO tags loaded",
            std::string(tagDirectory),
            namespacesResult.error().message());
        return loadedCount;
    }

    std::unordered_map<ResourceLocation, _MergedTagData> mergedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/" + std::string(tagDirectory);
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = _extractTagLocation(namespace_, directory, resourcePath);

            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::error("GenericTagLoader: failed to read tag file: {}", resourcePath);
                continue;
            }

            auto parseResult = _parseJsonRaw(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::error(
                    "GenericTagLoader: failed to parse tag {}: {}", location.toString(), parseResult.error().message());
                continue;
            }

            _mergeInto(mergedTags, location, parseResult.value());
        }
    }

    _finalize(mergedTags, resolveMember, readTag, fillTag);
    loadedCount = mergedTags.size();

    return loadedCount;
}

} // namespace mc::resource::tag
