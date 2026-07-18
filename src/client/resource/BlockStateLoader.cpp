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

#include "BlockStateLoader.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace {

std::vector<std::pair<std::string, std::string>> parseStateConditions(std::string_view stateStr)
{
    std::vector<std::pair<std::string, std::string>> conditions;

    if (stateStr.empty() || stateStr == "normal") {
        return conditions;
    }

    size_t start = 0;
    while (start < stateStr.size()) {
        size_t end = stateStr.find(',', start);
        if (end == std::string_view::npos) {
            end = stateStr.size();
        }

        std::string_view token(stateStr.data() + start, end - start);
        size_t eq = token.find('=');
        if (eq != std::string_view::npos) {
            std::string key(token.substr(0, eq));
            std::string value(token.substr(eq + 1));
            if (!key.empty()) {
                conditions.emplace_back(std::move(key), std::move(value));
            }
        }

        start = end + 1;
    }

    return conditions;
}

bool matchesProperties(const std::vector<std::pair<std::string, std::string>>& conditions,
    const std::map<std::string, std::string>& properties)
{
    for (const auto& [key, value] : conditions) {
        auto it = properties.find(key);
        if (it == properties.end() || it->second != value) {
            return false;
        }
    }
    return true;
}

} // namespace

namespace mc {

Result<void> BlockStateLoader::loadFromResourcePack(IResourcePack& resourcePack)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "BlockStateLoader::loadFromResourcePack");

    // 列出所有方块状态文件
    auto result = resourcePack.listResources(resource::PackType::ClientResources, "minecraft/blockstates", "json");

    if (result.failed()) {
        // 目录可能不存在
        return Result<void>::ok();
    }

    auto files = result.value();
    size_t loaded = 0;

    for (const auto& file : files) {
        // 提取方块名称
        // assets/minecraft/blockstates/stone.json -> stone
        std::string blockName;
        size_t lastSlash = file.find_last_of("/\\");
        size_t dotPos = file.find_last_of('.');

        if (lastSlash != std::string::npos && dotPos != std::string::npos && dotPos > lastSlash) {
            blockName = file.substr(lastSlash + 1, dotPos - lastSlash - 1);
        } else {
            continue;
        }

        // 读取并解析
        auto readResult = resourcePack.readTextResource(resource::PackType::ClientResources, file);
        if (readResult.failed()) {
            continue;
        }

        auto parseResult = BlockStateDefinition::parse(readResult.value());
        if (parseResult.failed()) {
            continue;
        }

        ResourceLocation blockId("minecraft", blockName);
        m_blockStates[blockId] = parseResult.value();
        loaded++;
    }

    if (loaded > 0) {
        spdlog::info("BlockStateLoader: Loaded {} block states from '{}'", loaded, resourcePack.name());
    }

    return Result<void>::ok();
}

const BlockStateDefinition* BlockStateLoader::getBlockState(const ResourceLocation& blockId) const
{
    auto it = m_blockStates.find(blockId);
    if (it != m_blockStates.end()) {
        return &it->second;
    }
    return nullptr;
}

const BlockStateVariant* BlockStateLoader::getVariant(const ResourceLocation& blockId, std::string_view stateStr) const
{
    auto* def = getBlockState(blockId);
    if (!def) {
        return nullptr;
    }

    const VariantList* list = def->getVariants(stateStr);
    if (!list || list->variants.empty()) {
        return nullptr;
    }

    return &list->select();
}

const BlockStateVariant* BlockStateLoader::getVariant(
    const ResourceLocation& blockId, const std::map<std::string, std::string>& properties) const
{
    std::string stateStr = _propertiesToStateStr(properties);

    // 先尝试精确匹配
    if (const auto* variant = getVariant(blockId, stateStr)) {
        return variant;
    }

    // 回退：允许 JSON 只声明部分属性（与 Java 版匹配策略一致）
    const auto* def = getBlockState(blockId);
    if (!def) {
        return nullptr;
    }

    const VariantList* bestList = nullptr;
    size_t bestSpecificity = 0;
    bool hasMatch = false;

    for (const auto& [variantKey, list] : def->getAllVariants()) {
        const auto conditions = parseStateConditions(variantKey);
        if (!matchesProperties(conditions, properties)) {
            continue;
        }

        const size_t specificity = conditions.size();
        if (!hasMatch || specificity > bestSpecificity) {
            bestList = &list;
            bestSpecificity = specificity;
            hasMatch = true;
        }
    }

    if (!bestList || bestList->variants.empty()) {
        return nullptr;
    }

    return &bestList->select();
}

void BlockStateLoader::clearCache()
{
    m_blockStates.clear();
}

std::vector<ResourceLocation> BlockStateLoader::getLoadedBlockStates() const
{
    std::vector<ResourceLocation> result;
    result.reserve(m_blockStates.size());

    for (const auto& [loc, def] : m_blockStates) {
        result.push_back(loc);
    }

    return result;
}

std::string BlockStateLoader::_propertiesToStateStr(const std::map<std::string, std::string>& properties)
{
    if (properties.empty()) {
        return "normal";
    }

    std::string result;

    // 按键排序以保证一致性
    std::vector<std::pair<std::string, std::string>> sortedProps(properties.begin(), properties.end());
    std::sort(sortedProps.begin(), sortedProps.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    for (size_t i = 0; i < sortedProps.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += sortedProps[i].first + "=" + sortedProps[i].second;
    }

    return result;
}

} // namespace mc
