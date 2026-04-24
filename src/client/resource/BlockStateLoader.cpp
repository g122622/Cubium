#include "BlockStateLoader.hpp"
#include "common/resource/IResourcePack.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace {

std::vector<std::pair<mc::String, mc::String>> parseStateConditions(mc::StringView stateStr) {
    std::vector<std::pair<mc::String, mc::String>> conditions;

    if (stateStr.empty() || stateStr == "normal") {
        return conditions;
    }

    size_t start = 0;
    while (start < stateStr.size()) {
        size_t end = stateStr.find(',', start);
        if (end == mc::StringView::npos) {
            end = stateStr.size();
        }

        mc::StringView token(stateStr.data() + start, end - start);
        size_t eq = token.find('=');
        if (eq != mc::StringView::npos) {
            mc::String key(token.substr(0, eq));
            mc::String value(token.substr(eq + 1));
            if (!key.empty()) {
                conditions.emplace_back(std::move(key), std::move(value));
            }
        }

        start = end + 1;
    }

    return conditions;
}

bool matchesProperties(
    const std::vector<std::pair<mc::String, mc::String>>& conditions,
    const std::map<mc::String, mc::String>& properties)
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

Result<void> BlockStateLoader::loadFromResourcePack(IResourcePack& resourcePack) {
    m_resourcePack = &resourcePack;

    // 列出所有方块状态文件
    auto result = resourcePack.listResources(
        "assets/minecraft/blockstates", "json");

    if (result.failed()) {
        // 目录可能不存在
        return Result<void>::ok();
    }

    auto files = result.value();
    size_t loaded = 0;

    for (const auto& file : files) {
        // 提取方块名称
        // assets/minecraft/blockstates/stone.json -> stone
        String blockName;
        size_t lastSlash = file.find_last_of("/\\");
        size_t dotPos = file.find_last_of('.');

        if (lastSlash != String::npos && dotPos != String::npos && dotPos > lastSlash) {
            blockName = file.substr(lastSlash + 1, dotPos - lastSlash - 1);
        } else {
            continue;
        }

        // 读取并解析
        auto readResult = resourcePack.readTextResource(file);
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
        spdlog::info("BlockStateLoader: Loaded {} block states from '{}'",
                    loaded, resourcePack.name());
    }

    return Result<void>::ok();
}

const BlockStateDefinition* BlockStateLoader::getBlockState(
    const ResourceLocation& blockId) const
{
    auto it = m_blockStates.find(blockId);
    if (it != m_blockStates.end()) {
        return &it->second;
    }
    return nullptr;
}

const BlockStateVariant* BlockStateLoader::getVariant(
    const ResourceLocation& blockId,
    StringView stateStr) const
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
    const ResourceLocation& blockId,
    const std::map<String, String>& properties) const
{
    String stateStr = propertiesToStateStr(properties);

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

void BlockStateLoader::clearCache() {
    m_blockStates.clear();
}

std::vector<ResourceLocation> BlockStateLoader::getLoadedBlockStates() const {
    std::vector<ResourceLocation> result;
    result.reserve(m_blockStates.size());

    for (const auto& [loc, def] : m_blockStates) {
        result.push_back(loc);
    }

    return result;
}

String BlockStateLoader::propertiesToStateStr(const std::map<String, String>& properties) {
    if (properties.empty()) {
        return "normal";
    }

    String result;

    // 按键排序以保证一致性
    std::vector<std::pair<String, String>> sortedProps(
        properties.begin(), properties.end());
    std::sort(sortedProps.begin(), sortedProps.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    for (size_t i = 0; i < sortedProps.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += sortedProps[i].first + "=" + sortedProps[i].second;
    }

    return result;
}

} // namespace mc
