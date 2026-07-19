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

[[nodiscard]] bool parseAnimatedFrameSizeFromMcmeta(
    const std::vector<u8>& mcmetaData, u32 imageWidth, u32 imageHeight, u32& outFrameWidth, u32& outFrameHeight)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::parseAnimatedFrameSizeFromMcmeta");

    if (imageWidth == 0 || imageHeight == 0 || mcmetaData.empty()) {
        return false;
    }

    try {
        const std::string jsonText(mcmetaData.begin(), mcmetaData.end());
        const auto json = nlohmann::json::parse(jsonText);

        if (!json.is_object() || !json.contains("animation") || !json["animation"].is_object()) {
            return false;
        }

        const auto& animation = json["animation"];
        const i32 configuredWidth = animation.value("width", 0);
        const i32 configuredHeight = animation.value("height", 0);

        // 未配置时默认使用方形帧（frame = imageWidth）
        // 仅配置单边时，另一边沿用同值
        i32 frameWidth = configuredWidth;
        i32 frameHeight = configuredHeight;

        if (frameWidth <= 0 && frameHeight <= 0) {
            frameWidth = static_cast<i32>(imageWidth);
            frameHeight = static_cast<i32>(imageWidth);
        } else if (frameWidth <= 0) {
            frameWidth = frameHeight;
        } else if (frameHeight <= 0) {
            frameHeight = frameWidth;
        }

        if (frameWidth <= 0 || frameHeight <= 0) {
            return false;
        }

        if (frameWidth > static_cast<i32>(imageWidth) || frameHeight > static_cast<i32>(imageHeight)) {
            return false;
        }

        if ((imageWidth % static_cast<u32>(frameWidth)) != 0 || (imageHeight % static_cast<u32>(frameHeight)) != 0) {
            return false;
        }

        outFrameWidth = static_cast<u32>(frameWidth);
        outFrameHeight = static_cast<u32>(frameHeight);
        return true;
    }
    catch (const nlohmann::json::exception&) {
        return false;
    }
}

/**
 * @brief 动态生成 missingno 纹理（紫黑棋盘格）
 *
 * 当纹理资源包中找不到 missingno 纹理时，通过代码生成经典的
 * 16×16 紫黑棋盘格替代纹理，避免出现纹理缺失警告。
 *
 * @return 16×16 RGBA8 像素数据
 */
std::vector<u8> _generateMissingNoTexture()
{
    constexpr u32 SIZE = 16;
    constexpr u32 TILE = 8; // 每个棋盘格 8×8 像素
    constexpr u8 PURPLE_R = 128, PURPLE_G = 0, PURPLE_B = 128;
    constexpr u8 BLACK_R = 0, BLACK_G = 0, BLACK_B = 0;

    std::vector<u8> pixels(SIZE * SIZE * 4);
    for (u32 y = 0; y < SIZE; ++y) {
        for (u32 x = 0; x < SIZE; ++x) {
            const u32 idx = (y * SIZE + x) * 4;
            const bool isPurple = ((x / TILE) + (y / TILE)) % 2 == 0;
            pixels[idx + 0] = isPurple ? PURPLE_R : BLACK_R;
            pixels[idx + 1] = isPurple ? PURPLE_G : BLACK_G;
            pixels[idx + 2] = isPurple ? PURPLE_B : BLACK_B;
            pixels[idx + 3] = 255; // 完全不透明
        }
    }
    return pixels;
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

    // 注意：computeBlockAppearances 在 buildTextureAtlas 后调用
    // 因为需要纹理区域数据

    // 初始化物品模型缓存
    client::resource::ItemModelCache::instance().initialize(m_resourcePacks);

    spdlog::info("ResourceManager: Loaded {} block states, {} models",
        m_blockStateLoader.getLoadedBlockStates().size(),
        m_bakedModels.size());

    return Result<void>::ok();
}

Result<AtlasBuildResult> ResourceManager::buildTextureAtlas()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::buildTextureAtlas");

    const _MemorySnapshot phaseBefore = _snapshotMemory();

    // 收集所需纹理
    auto textures = _collectRequiredTextures();

    spdlog::info("Collecting {} textures for atlas", textures.size());

    TextureAtlasBuilder builder;
    builder.setMaxSize(4096, 4096);
    builder.setPadding(0);

    // 统计
    size_t addedCount = 0;
    size_t failedCount = 0;
    std::vector<std::string> failedTextures;

    for (const auto& texLoc : textures) {
        bool added = false;

        // 构建候选路径列表：原始路径 + MC 1.12/1.13+ 路径变体
        std::vector<ResourceLocation> candidates;
        candidates.push_back(texLoc);

        std::string altPath = getAltTexturePath(texLoc.path());
        if (!altPath.empty()) {
            candidates.emplace_back(texLoc.namespace_(), std::move(altPath));
        }

        // 遍历候选路径和资源包（后添加的优先级更高）
        for (const auto& candidateLoc : candidates) {
            if (added) {
                break;
            }

            for (auto it = m_resourcePacks.rbegin(); it != m_resourcePacks.rend() && !added; ++it) {
                auto& pack = *it;
                std::string relativePath = candidateLoc.toFilePath(resource::PackType::ClientResources, "png");
                relativePath.erase(0, std::string("assets/").size());

                if (!pack->hasResource(resource::PackType::ClientResources, relativePath)) {
                    continue;
                }

                auto readResult = pack->readResource(resource::PackType::ClientResources, relativePath);
                if (!readResult.success()) {
                    continue;
                }

                int width = 0;
                int height = 0;
                int channels = 0;
                stbi_uc* pixels = stbi_load_from_memory(readResult.value().data(),
                    static_cast<int>(readResult.value().size()),
                    &width,
                    &height,
                    &channels,
                    4);

                if (pixels) {
                    std::vector<u8> pixelData(pixels, pixels + width * height * 4);
                    stbi_image_free(pixels);

                    u32 frameWidth = static_cast<u32>(width);
                    u32 frameHeight = static_cast<u32>(height);

                    const std::string mcmetaPath = relativePath + ".mcmeta";
                    if (pack->hasResource(resource::PackType::ClientResources, mcmetaPath)) {
                        const auto mcmetaResult = pack->readResource(resource::PackType::ClientResources, mcmetaPath);
                        if (mcmetaResult.success()) {
                            static_cast<void>(parseAnimatedFrameSizeFromMcmeta(mcmetaResult.value(),
                                static_cast<u32>(width),
                                static_cast<u32>(height),
                                frameWidth,
                                frameHeight));
                        }
                    }

                    // 使用原始 texLoc 作为图集键（而非候选路径），保持一致性
                    builder.addTextureFrame(
                        texLoc, pixelData, static_cast<u32>(width), static_cast<u32>(height), frameWidth, frameHeight);
                    added = true;
                    addedCount++;
                }
            }
        }

        if (!added) {
            // missingno 纹理：动态生成紫黑棋盘格替代纹理
            if (texLoc == ResourceLocation("minecraft", "textures/missingno")) {
                constexpr u32 MISSINGNO_SIZE = 16;
                auto pixelData = _generateMissingNoTexture();
                builder.addTextureFrame(
                    texLoc, pixelData, MISSINGNO_SIZE, MISSINGNO_SIZE, MISSINGNO_SIZE, MISSINGNO_SIZE);
                addedCount++;
            } else {
                failedTextures.push_back(
                    texLoc.toString() + " -> " + texLoc.toFilePath(resource::PackType::ClientResources, "png"));
                failedCount++;
            }
        }
    }

    spdlog::info("Texture atlas: {} added, {} failed", addedCount, failedCount);

    // 输出失败纹理的详细信息
    if (failedCount > 0) {
        spdlog::warn("Failed textures (first 50 of {}):", failedCount);
        for (size_t i = 0; i < std::min(failedTextures.size(), size_t(50)); ++i) {
            spdlog::warn("  - {}", failedTextures[i]);
        }
    }

    // 构建图集
    auto result = builder.build();
    if (result.failed()) {
        return result.error();
    }

    // 缓存结果
    m_atlasResult = result.value();
    m_textureRegions = m_atlasResult.regions;
    m_atlasBuilt = true;

    // 注册 MC 1.12/1.13+ 路径变体别名
    // 例如：如果图集中存在 minecraft:textures/block/stone，则同时注册
    // minecraft:textures/blocks/stone 指向同一纹理区域，反之亦然。
    // 这样无论模型使用哪种路径格式，都能通过直接查找找到纹理。
    {
        std::vector<std::pair<ResourceLocation, TextureRegion>> aliases;
        for (const auto& [loc, region] : m_textureRegions) {
            std::string altPath = getAltTexturePath(loc.path());
            if (!altPath.empty()) {
                ResourceLocation altLoc(loc.namespace_(), std::move(altPath));
                // 仅在别名不存在时添加，避免覆盖已有纹理
                if (m_textureRegions.find(altLoc) == m_textureRegions.end()) {
                    aliases.emplace_back(std::move(altLoc), region);
                }
            }
        }
        for (auto& [altLoc, region] : aliases) {
            m_textureRegions.emplace(std::move(altLoc), std::move(region));
        }
    }

    // 构建纹理图集后计算方块外观（这样纹理区域可用）
    _computeBlockAppearances();

    spdlog::info("ResourceManager: {} appearances computed", m_blockAppearances.size());

    // 外观已计算完毕，加载期中间缓存（未烘焙模型、已烘焙模型、方块状态定义）不再被运行时访问——
    // 运行时渲染走 BlockModelCache::getBlockAppearance(stateId) 直接查 m_blockAppearances。
    // 释放这三块以降低运行时驻留内存（合计数十~上百 MB）。
    const _MemorySnapshot releaseBefore = _snapshotMemory();
    m_bakedModels.clear();
    m_modelLoader.clearCache();
    m_blockStateLoader.clearCache();
    const _MemorySnapshot releaseAfter = _snapshotMemory();
    _logPhaseMemory("buildTextureAtlas::ReleaseDeadCache", releaseBefore, releaseAfter);
    // 验证死缓存释放是否反映在提交量上：若 releaseDelta≈0，说明堆未归还页面，
    // 解释了"结构优化后工作集看不出下降"的现象。
    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Resource,
        "Phase::buildTextureAtlas::ReleaseDeadCache",
        "commitBeforeMB",
        releaseBefore.commitMB,
        "commitAfterMB",
        releaseAfter.commitMB,
        "commitDeltaMB",
        releaseAfter.commitMB - releaseBefore.commitMB,
        "wsDeltaMB",
        releaseAfter.wsMB - releaseBefore.wsMB);

    const _MemorySnapshot phaseAfter = _snapshotMemory();
    _logPhaseMemory("buildTextureAtlas", phaseBefore, phaseAfter);
    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Resource,
        "Phase::buildTextureAtlas",
        "commitBeforeMB",
        phaseBefore.commitMB,
        "commitAfterMB",
        phaseAfter.commitMB,
        "commitDeltaMB",
        phaseAfter.commitMB - phaseBefore.commitMB,
        "wsDeltaMB",
        phaseAfter.wsMB - phaseBefore.wsMB);

    return m_atlasResult;
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

const TextureRegion* ResourceManager::getTextureRegion(const ResourceLocation& textureLocation) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::getTextureRegion");

    // 直接委托给 _findTextureRegion（已包含 MC 1.12/1.13+ 路径变体兼容查找）
    return _findTextureRegion(textureLocation);
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
    m_textureRegions.clear();
    m_atlasResult = AtlasBuildResult();
    m_atlasBuilt = false;

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
    m_textureRegions.clear();
    m_atlasResult = AtlasBuildResult();
    m_atlasBuilt = false;

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

void ResourceManager::_computeBlockAppearances()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::computeBlockAppearances");

    const _MemorySnapshot before = _snapshotMemory();

    // 获取所有方块状态
    auto blockStates = m_blockStateLoader.getLoadedBlockStates();

    u32 totalAppearances = 0;
    u32 appearancesWithTextures = 0;

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

                    // 使用 compat 层查找纹理区域
                    const TextureRegion* region = _findTextureRegion(fullTexLoc);

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
                const TextureRegion* particleRegion = _findTextureRegion(fullParticleTexLoc);
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

std::string ResourceManager::getAltTexturePath(const std::string& path)
{
    constexpr std::string_view blockModern = "textures/block/";
    constexpr std::string_view blockLegacy = "textures/blocks/";
    constexpr std::string_view itemModern = "textures/item/";
    constexpr std::string_view itemLegacy = "textures/items/";

    if (path.size() > blockModern.size() && path.compare(0, blockModern.size(), blockModern) == 0) {
        // Modern (1.13+) -> Legacy (1.12): textures/block/ -> textures/blocks/
        return "textures/blocks/" + path.substr(blockModern.size());
    }
    if (path.size() > blockLegacy.size() && path.compare(0, blockLegacy.size(), blockLegacy) == 0) {
        // Legacy (1.12) -> Modern (1.13+): textures/blocks/ -> textures/block/
        return "textures/block/" + path.substr(blockLegacy.size());
    }
    if (path.size() > itemModern.size() && path.compare(0, itemModern.size(), itemModern) == 0) {
        // Modern (1.13+) -> Legacy (1.12): textures/item/ -> textures/items/
        return "textures/items/" + path.substr(itemModern.size());
    }
    if (path.size() > itemLegacy.size() && path.compare(0, itemLegacy.size(), itemLegacy) == 0) {
        // Legacy (1.12) -> Modern (1.13+): textures/items/ -> textures/item/
        return "textures/item/" + path.substr(itemLegacy.size());
    }

    // 实体纹理路径变体：MC 1.13+ 子目录格式 <-> MC 1.12 扁平格式
    // 例如：textures/entity/pig/pig.png -> textures/entity/pig.png
    //       textures/entity/pig.png    -> textures/entity/pig/pig.png
    //       textures/entity/pig/pig    -> textures/entity/pig
    //       textures/entity/pig        -> textures/entity/pig/pig
    constexpr std::string_view entityPrefix = "textures/entity/";
    if (path.size() > entityPrefix.size() && path.compare(0, entityPrefix.size(), entityPrefix) == 0) {
        std::string_view afterPrefix(path.data() + entityPrefix.size(), path.size() - entityPrefix.size());
        auto slashPos = afterPrefix.find('/');
        if (slashPos != std::string_view::npos) {
            // 子目录格式：textures/entity/<name>/<filename>
            // 检查 <name> 与 <filename> 是否相同（不含扩展名）
            std::string_view dirName = afterPrefix.substr(0, slashPos);
            std::string_view fileName = afterPrefix.substr(slashPos + 1);
            // 提取扩展名（如 .png）以便在转换后保留
            std::string_view extension;
            auto dotPos = fileName.rfind('.');
            if (dotPos != std::string_view::npos) {
                extension = fileName.substr(dotPos);
                fileName = fileName.substr(0, dotPos);
            }
            if (dirName == fileName) {
                // textures/entity/<name>/<name>[.ext] -> textures/entity/<name>[.ext]
                return std::string(entityPrefix) + std::string(dirName) + std::string(extension);
            }
        } else {
            // 扁平格式：textures/entity/<name>[.ext]
            // -> textures/entity/<name>/<name>[.ext]
            std::string_view namePart = afterPrefix;
            std::string_view extension;
            auto dotPos = afterPrefix.rfind('.');
            if (dotPos != std::string_view::npos) {
                namePart = afterPrefix.substr(0, dotPos);
                extension = afterPrefix.substr(dotPos);
            }
            return std::string(entityPrefix) + std::string(namePart) + "/" + std::string(namePart) +
                std::string(extension);
        }
    }

    return {};
}

const TextureRegion* ResourceManager::_findTextureRegion(const ResourceLocation& texLoc) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::findTextureRegion");

    // 1. 尝试原始路径
    auto it = m_textureRegions.find(texLoc);
    if (it != m_textureRegions.end()) {
        return &it->second;
    }

    // 2. 尝试 MC 1.12/1.13+ 路径变体兼容查找
    //    MC 1.13+ 使用 textures/block/ 和 textures/item/（单数）
    //    MC 1.12  使用 textures/blocks/ 和 textures/items/（复数）
    //    当原始路径未找到时，自动尝试对应的另一种路径形式。
    std::string altPath = getAltTexturePath(texLoc.path());
    if (!altPath.empty()) {
        ResourceLocation altLoc(texLoc.namespace_(), std::move(altPath));
        it = m_textureRegions.find(altLoc);
        if (it != m_textureRegions.end()) {
            return &it->second;
        }
    }

    return nullptr;
}

std::set<ResourceLocation> ResourceManager::_collectRequiredTextures() const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ResourceManager::collectRequiredTextures");

    std::set<ResourceLocation> textures;

    // 从烘焙模型收集纹理
    for (const auto& [loc, model] : m_bakedModels) {
        for (const auto& [name, texLoc] : model.textures) {
            // 跳过纹理变量引用（以 # 开头的值）
            std::string texPath = texLoc.path();
            if (!texPath.empty() && texPath[0] == '#') {
                // 纹理变量引用，不应该出现在烘焙模型中
                spdlog::warn(
                    "Texture variable reference found in baked model {}: {}={}", loc.toString(), name, texPath);
                continue;
            }

            // 转换为纹理路径
            ResourceLocation textureLoc = _texturePathToLocation(texPath);
            textures.insert(textureLoc);
        }
    }

    return textures;
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
