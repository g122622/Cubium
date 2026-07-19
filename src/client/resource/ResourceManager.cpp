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

#include "ResourceManager.hpp"
#include "ItemModelCache.hpp"
#include "common/resource/pack/FolderResourcePack.hpp"
#include "common/util/PlatformInfo.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "common/profiler/TraceEvents.hpp"
#include <cctype>

// stb_image 用于 PNG 加载（实现在 TextureAtlasBuilder.cpp 中）
#include <stb_image.h>

using namespace mc::trace;

namespace mc {

namespace {

/**
 * @brief 进程内存快照（MB），用于阶段边界打点
 *
 * commit = 提交量（PagefileUsage / VmSize / virtual_size），ws = 工作集。
 * 阶段入口/出口各采一次，用 MC_TRACE_INSTANT_EVENT 打出绝对值，
 * 相邻边界点相减即为该阶段的内存增量 Δ。
 */
struct _MemorySnapshot {
    i64 commitMB = 0;
    i64 wsMB = 0;
};

// 采样当前进程内存快照。两个口径都采，便于在 trace 中对照工作集与提交量。
_MemorySnapshot _snapshotMemory()
{
    return _MemorySnapshot{static_cast<i64>(util::PlatformInfo::getProcessCommitMB()),
        static_cast<i64>(util::PlatformInfo::getProcessMemoryMB())};
}

// 阶段内存增量输出到控制台。与 MC_TRACE_INSTANT_EVENT 并列：profiler 关闭时
// （基线测量场景）trace event 空展开，仅靠 spdlog 保留数据，故必须双写。
void _logPhaseMemory(std::string_view phase, const _MemorySnapshot& before, const _MemorySnapshot& after)
{
    const i64 commitDelta = after.commitMB - before.commitMB;
    const i64 wsDelta = after.wsMB - before.wsMB;
    spdlog::info("[MemPhase] {} | commit {}->{} (Δ{:+}MB) | ws {}->{} (Δ{:+}MB)",
        phase,
        before.commitMB,
        after.commitMB,
        commitDelta,
        before.wsMB,
        after.wsMB,
        wsDelta);
}

bool isAsciiWhitespace(char ch)
{
    switch (ch) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '\f':
        case '\v':
            return true;
        default:
            return false;
    }
}

std::string_view trimStateToken(std::string_view token)
{
    // MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::trimStateToken");

    size_t start = 0;
    size_t end = token.size();

    while (start < end && isAsciiWhitespace(token[start])) {
        ++start;
    }
    while (end > start && isAsciiWhitespace(token[end - 1])) {
        --end;
    }

    return token.substr(start, end - start);
}

std::string normalizeStateString(std::string_view stateStr)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::normalizeStateString");

    const std::string_view trimmed = trimStateToken(stateStr);
    if (trimmed.empty() || trimmed == "normal") {
        return "normal";
    }

    std::map<std::string_view, std::string_view> props;
    size_t start = 0;
    while (start < trimmed.size()) {
        size_t end = trimmed.find(',', start);
        if (end == std::string::npos) {
            end = trimmed.size();
        }

        const std::string_view token = trimStateToken(trimmed.substr(start, end - start));
        if (!token.empty()) {
            size_t eq = token.find('=');
            if (eq != std::string::npos) {
                const std::string_view key = trimStateToken(token.substr(0, eq));
                const std::string_view value = trimStateToken(token.substr(eq + 1));
                if (!key.empty()) {
                    props[key] = value;
                }
            }
        }

        start = end + 1;
    }

    if (props.empty()) {
        return "normal";
    }

    std::string normalized;
    bool first = true;
    for (const auto& [key, value] : props) {
        if (!first) {
            normalized += ",";
        }
        normalized.append(key.data(), key.size());
        normalized.push_back('=');
        normalized.append(value.data(), value.size());
        first = false;
    }

    return normalized;
}

std::vector<std::pair<std::string, std::string>> parseStateConditions(std::string_view stateStr)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::parseStateConditions");

    std::vector<std::pair<std::string, std::string>> conditions;

    const std::string_view trimmed = trimStateToken(stateStr);
    if (trimmed.empty() || trimmed == "normal") {
        return conditions;
    }

    size_t start = 0;
    while (start < trimmed.size()) {
        size_t end = trimmed.find(',', start);
        if (end == std::string::npos) {
            end = trimmed.size();
        }

        const std::string_view token = trimStateToken(trimmed.substr(start, end - start));
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

bool matchConditions(const std::vector<std::pair<std::string, std::string>>& conditions,
    const std::map<std::string, std::string>& properties)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::matchConditions");

    for (const auto& [key, value] : conditions) {
        auto it = properties.find(key);
        if (it == properties.end() || it->second != value) {
            return false;
        }
    }
    return true;
}

} // namespace

Result<void> ResourceManager::addResourcePack(ResourcePackPtr resourcePack)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::addResourcePack");

    if (!resourcePack) {
        return Error(ErrorCode::InvalidArgument, "Resource pack is null");
    }

    // 初始化资源包
    auto result = resourcePack->initialize();
    if (result.failed()) {
        return result.error();
    }

    spdlog::info("ResourceManager: Added resource pack '{}'", resourcePack->name());
    m_resourcePacks.push_back(std::move(resourcePack));
    return Result<void>::ok();
}

Result<void> ResourceManager::loadAllResources()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::loadAllResources");

    // 设置模型加载器的资源包列表
    m_modelLoader.setPackRepository(m_resourcePacks);

    // 阶段1：加载方块状态（BlockStateLoader::loadFromResourcePack）
    {
        const _MemorySnapshot before = _snapshotMemory();
        for (auto& pack : m_resourcePacks) {
            static_cast<void>(m_blockStateLoader.loadFromResourcePack(*pack));
        }
        const _MemorySnapshot after = _snapshotMemory();
        _logPhaseMemory("BlockStateLoader::loadFromResourcePack", before, after);
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Resource,
            "Phase::BlockStateLoader::loadFromResourcePack",
            "commitBeforeMB",
            before.commitMB,
            "commitAfterMB",
            after.commitMB,
            "commitDeltaMB",
            after.commitMB - before.commitMB,
            "wsDeltaMB",
            after.wsMB - before.wsMB);
    }

    // 阶段2：加载方块模型（BlockModelLoader::loadFromResourcePack，按需加载）
    {
        const _MemorySnapshot before = _snapshotMemory();
        for (auto& pack : m_resourcePacks) {
            static_cast<void>(m_modelLoader.loadFromResourcePack(*pack));
        }
        const _MemorySnapshot after = _snapshotMemory();
        _logPhaseMemory("BlockModelLoader::loadFromResourcePack", before, after);
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Resource,
            "Phase::BlockModelLoader::loadFromResourcePack",
            "commitBeforeMB",
            before.commitMB,
            "commitAfterMB",
            after.commitMB,
            "commitDeltaMB",
            after.commitMB - before.commitMB,
            "wsDeltaMB",
            after.wsMB - before.wsMB);
    }

    // 阶段3：烘焙模型（_bakeAllModels）
    {
        const _MemorySnapshot before = _snapshotMemory();
        static_cast<void>(_bakeAllModels());
        const _MemorySnapshot after = _snapshotMemory();
        _logPhaseMemory("bakeAllModels", before, after);
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Resource,
            "Phase::bakeAllModels",
            "commitBeforeMB",
            before.commitMB,
            "commitAfterMB",
            after.commitMB,
            "commitDeltaMB",
            after.commitMB - before.commitMB,
            "wsDeltaMB",
            after.wsMB - before.wsMB);
    }

    // 注意：computeBlockAppearances 不在此调用——纹理区域已迁移到 AtlasManager，
    // 需由调用方在 AtlasManager 加载完 blocks atlas 后显式调用 computeBlockAppearances(regionLookup)。

    // 初始化物品模型缓存
    client::resource::ItemModelCache::instance().initialize(m_resourcePacks);

    spdlog::info("ResourceManager: Loaded {} block states, {} models",
        m_blockStateLoader.getLoadedBlockStates().size(),
        m_bakedModels.size());

    return Result<void>::ok();
}

const BlockAppearance* ResourceManager::getBlockAppearance(
    const ResourceLocation& blockId, const std::map<std::string, std::string>& properties) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::getBlockAppearance");

    // 构建缓存键
    std::string cacheKey = blockId.toString();
    if (!properties.empty()) {
        cacheKey += "?";
        bool first = true;
        for (const auto& [key, value] : properties) {
            if (!first) cacheKey += ",";
            cacheKey += key + "=" + value;
            first = false;
        }
    }

    auto it = m_blockAppearances.find(cacheKey);
    if (it != m_blockAppearances.end()) {
        return &it->second;
    }

    // 回退匹配：允许方块状态定义只约束部分属性
    // 性能优化：m_blockAppearances 是有序 map，使用 lower_bound 将扫描范围
    // 缩小到同一 blockId 前缀区间，避免全表遍历。
    const std::string blockPrefix = blockId.toString();
    const size_t blockPrefixSize = blockPrefix.size();

    const BlockAppearance* bestMatch = nullptr;
    const BlockAppearance* firstBlockAppearance = nullptr;
    size_t bestSpecificity = 0;

    auto itRange = m_blockAppearances.lower_bound(blockPrefix);
    for (; itRange != m_blockAppearances.end(); ++itRange) {
        const auto& appearanceKey = itRange->first;
        const auto& appearance = itRange->second;

        // 已经离开 blockPrefix 前缀范围，直接结束扫描
        if (appearanceKey.compare(0, blockPrefixSize, blockPrefix) != 0) {
            break;
        }

        // 只接受两类 key：
        // 1) "namespace:block"
        // 2) "namespace:block?prop=value,..."
        const bool isExactKey = appearanceKey.size() == blockPrefixSize;
        const bool isStateKey = appearanceKey.size() > blockPrefixSize && appearanceKey[blockPrefixSize] == '?';
        if (!isExactKey && !isStateKey) {
            continue;
        }

        if (!firstBlockAppearance) {
            firstBlockAppearance = &appearance;
        }

        std::string_view statePart = "normal";
        if (isStateKey && appearanceKey.size() > blockPrefixSize + 1) {
            statePart = std::string_view(
                appearanceKey.data() + blockPrefixSize + 1, appearanceKey.size() - blockPrefixSize - 1);
        }

        const auto conditions = parseStateConditions(statePart);
        if (!matchConditions(conditions, properties)) {
            continue;
        }

        const size_t specificity = conditions.size();
        if (!bestMatch || specificity > bestSpecificity) {
            bestMatch = &appearance;
            bestSpecificity = specificity;
        }
    }

    if (bestMatch) {
        return bestMatch;
    }

    if (firstBlockAppearance) {
        return firstBlockAppearance;
    }

    return nullptr;
}

Result<DecodedTexture> ResourceManager::loadTextureRGBA(const ResourceLocation& textureLocation) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::loadTextureRGBA");

    if (m_resourcePacks.empty()) {
        return Error(ErrorCode::NotFound, "No resource packs available for texture: " + textureLocation.toString());
    }

    for (auto packIt = m_resourcePacks.rbegin(); packIt != m_resourcePacks.rend(); ++packIt) {
        const auto& pack = *packIt;
        std::string relativePath = textureLocation.toFilePath(resource::PackType::ClientResources, "png");
        relativePath.erase(0, std::string("assets/").size());
        if (!pack->hasResource(resource::PackType::ClientResources, relativePath)) {
            continue;
        }

        const auto readResult = pack->readResource(resource::PackType::ClientResources, relativePath);
        if (readResult.failed()) {
            continue;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load_from_memory(
            readResult.value().data(), static_cast<int>(readResult.value().size()), &width, &height, &channels, 4);

        if (pixels == nullptr || width <= 0 || height <= 0) {
            if (pixels != nullptr) {
                stbi_image_free(pixels);
            }
            continue;
        }

        DecodedTexture decoded{};
        decoded.width = static_cast<u32>(width);
        decoded.height = static_cast<u32>(height);
        decoded.pixels.assign(
            pixels, pixels + (static_cast<size_t>(decoded.width) * static_cast<size_t>(decoded.height) * 4));
        stbi_image_free(pixels);

        return decoded;
    }

    return Error(ErrorCode::NotFound, "Texture not found in any resource pack: " + textureLocation.toString());
}

void ResourceManager::clear()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::clear");

    m_resourcePacks.clear();
    m_modelLoader.clearCache();
    m_blockStateLoader.clearCache();
    m_bakedModels.clear();
    m_blockAppearances.clear();

    // 清理物品模型缓存
    client::resource::ItemModelCache::instance().cleanup();
}

void ResourceManager::clearResourcePacks()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::clearResourcePacks");

    m_resourcePacks.clear();
}

Result<void> ResourceManager::reload()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::reload");

    // 清除缓存但保留资源包列表
    m_modelLoader.clearCache();
    m_blockStateLoader.clearCache();
    m_bakedModels.clear();
    m_blockAppearances.clear();

    // 重新加载所有资源
    return loadAllResources();
}

Result<void> ResourceManager::_bakeAllModels()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::bakeAllModels");

    // 获取所有方块状态
    auto blockStates = m_blockStateLoader.getLoadedBlockStates();

    size_t successCount = 0;
    size_t failCount = 0;

    for (const auto& blockId : blockStates) {
        const auto* def = m_blockStateLoader.getBlockState(blockId);
        if (!def) continue;

        // 烘焙所有变体的模型
        for (const auto& [stateStr, variantList] : def->getAllVariants()) {
            for (const auto& variant : variantList.variants) {
                if (!m_bakedModels.count(variant.model)) {
                    auto result = m_modelLoader.bakeModel(variant.model);
                    if (result.success()) {
                        m_bakedModels[variant.model] = result.value();
                        successCount++;
                    } else {
                        if (failCount < 50) {
                            spdlog::warn("Failed to bake model '{}' for block '{}': {}",
                                variant.model.toString(),
                                blockId.toString(),
                                result.error().toString());
                        } else if (failCount == 50) {
                            spdlog::warn("More model bake failures... (suppressed)");
                        }
                        failCount++;
                    }
                }
            }
        }
    }

    if (failCount > 0) {
        spdlog::warn("ResourceManager: Baked {} models ({} failed)", successCount, failCount);
    } else {
        spdlog::info("ResourceManager: Baked {} models successfully", successCount);
    }

    return Result<void>::ok();
}

void ResourceManager::computeBlockAppearances(
    const std::function<const TextureRegion*(const ResourceLocation&)>& regionLookup)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::computeBlockAppearances");

    const _MemorySnapshot before = _snapshotMemory();

    // 获取所有方块状态
    auto blockStates = m_blockStateLoader.getLoadedBlockStates();

    u32 totalAppearances = 0;
    u32 appearancesWithTextures = 0;

    // regionLookup 可能为空（AtlasManager 未加载或单元测试环境无 Vulkan），
    // 此时跳过纹理区域查询——方块外观仍构建，只是面纹理/粒子纹理为空。
    const bool hasLookup = static_cast<bool>(regionLookup);

    for (const auto& blockId : blockStates) {
        const auto* def = m_blockStateLoader.getBlockState(blockId);
        if (!def) continue;

        // 处理每个状态变体
        for (const auto& [stateStr, variantList] : def->getAllVariants()) {
            // 构建缓存键
            std::string cacheKey = blockId.toString();
            const std::string normalizedState = normalizeStateString(stateStr);
            if (normalizedState != "normal" && !normalizedState.empty()) {
                cacheKey += "?" + normalizedState;
            }

            // 选择第一个变体 (或默认变体)
            if (variantList.variants.empty()) continue;

            const auto& variant = variantList.variants[0];

            // 获取烘焙模型
            auto it = m_bakedModels.find(variant.model);
            if (it == m_bakedModels.end()) continue;

            const auto& bakedModel = it->second;

            // 创建外观
            BlockAppearance appearance;
            appearance.elements = bakedModel.elements;
            appearance.xRotation = variant.x;
            appearance.yRotation = variant.y;
            appearance.uvLock = variant.uvLock;

            // 解析面纹理
            for (const auto& element : bakedModel.elements) {
                for (size_t i = 0; i < element.faces.size(); ++i) {
                    const auto& faceOpt = element.faces[i];
                    if (!faceOpt.has_value()) {
                        continue;
                    }
                    const auto& face = *faceOpt;
                    const Direction dir = Directions::fromIndex(i);
                    const size_t idx = Directions::index(dir);

                    // 保留 tintindex（仅记录有着色需求的面）
                    if (face.tintIndex >= 0 && !appearance.faceTintIndices[idx]) {
                        appearance.faceTintIndices[idx] = face.tintIndex;
                    }

                    ResourceLocation texLoc = bakedModel.resolveTexture(face.texture);

                    // 转换纹理路径为完整的 textures/ 路径
                    ResourceLocation fullTexLoc = _texturePathToLocation(texLoc.path());

                    // 使用注入的 regionLookup 查找纹理区域（查 AtlasManager 的 blocks atlas regions）
                    const TextureRegion* region = hasLookup ? regionLookup(fullTexLoc) : nullptr;

                    if (region) {
                        // 保留首层纹理用于兼容旧逻辑
                        if (!appearance.faceTextures[idx]) {
                            appearance.faceTextures[idx] = *region;
                            appearance.faceTextureLocations[idx] = fullTexLoc;
                        }

                        // 收集全部层，支持草方块侧面 overlay 等多层模型
                        if (!appearance.faceTextureLayers[idx]) {
                            appearance.faceTextureLayers[idx].emplace();
                        }
                        appearance.faceTextureLayers[idx]->push_back(
                            BlockAppearance::FaceTextureLayer{*region, face.tintIndex});
                    }
                }
            }

            // 解析粒子纹理（模型 JSON 中 textures.particle 字段）
            auto particleIt = bakedModel.textures.find("particle");
            if (particleIt != bakedModel.textures.end()) {
                // particleIt->second 已经被 resolveTextureReferences 解析为实际的 ResourceLocation
                ResourceLocation fullParticleTexLoc = _texturePathToLocation(particleIt->second.path());
                const TextureRegion* particleRegion = hasLookup ? regionLookup(fullParticleTexLoc) : nullptr;
                if (particleRegion) {
                    appearance.particleTexture = *particleRegion;
                    appearance.particleTextureLocation = fullParticleTexLoc;
                    appearance.hasParticleTexture = true;
                }
            }

            totalAppearances++;
            if (appearance.hasAnyFaceTexture()) {
                appearancesWithTextures++;
            }

            m_blockAppearances[cacheKey] = std::move(appearance);
        }
    }

    spdlog::info("computeBlockAppearances: {} total, {} with textures", totalAppearances, appearancesWithTextures);

    const _MemorySnapshot after = _snapshotMemory();
    _logPhaseMemory("computeBlockAppearances", before, after);
    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Resource,
        "Phase::computeBlockAppearances",
        "commitBeforeMB",
        before.commitMB,
        "commitAfterMB",
        after.commitMB,
        "commitDeltaMB",
        after.commitMB - before.commitMB,
        "wsDeltaMB",
        after.wsMB - before.wsMB);
}

ResourceLocation ResourceManager::_texturePathToLocation(std::string_view path)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::texturePathToLocation");

    // 处理纹理路径
    // 例如: "blocks/stone" -> "minecraft:textures/blocks/stone"
    std::string p(path);

    // 移除前导的 "#"
    if (!p.empty() && p[0] == '#') {
        p = p.substr(1);
    }

    // 添加 textures/ 前缀
    if (p.find("textures/") != 0 && p.find("textures\\") != 0) {
        p = "textures/" + p;
    }

    return ResourceLocation(p);
}

IResourcePack* ResourceManager::getFirstResourcePack()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::getFirstResourcePack");

    if (m_resourcePacks.empty()) {
        return nullptr;
    }
    return m_resourcePacks[0].get();
}

IResourcePack* ResourceManager::getResourcePack(size_t index)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::getResourcePack");

    if (index >= m_resourcePacks.size()) {
        return nullptr;
    }
    return m_resourcePacks[index].get();
}

} // namespace mc
