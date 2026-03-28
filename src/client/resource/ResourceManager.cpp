#include "ResourceManager.hpp"
#include "../common/resource/FolderResourcePack.hpp"
#include "../common/resource/compat/TextureMapper.hpp"
#include "../common/resource/compat/ResourceMapper.hpp"
#include <spdlog/spdlog.h>

#include <cctype>

// stb_image 用于 PNG 加载（实现在 TextureAtlasBuilder.cpp 中）
#include <stb_image.h>

namespace mc {
namespace resource {
namespace compat {
    // Import for easy access
    using TextureMapper = mc::resource::compat::TextureMapper;
}}}

namespace mc {

namespace {

String trimStateToken(StringView token) {
    size_t start = 0;
    size_t end = token.size();

    while (start < end && std::isspace(static_cast<unsigned char>(token[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(token[end - 1]))) {
        --end;
    }

    return String(token.substr(start, end - start));
}

String normalizeStateString(StringView stateStr) {
    String trimmed = trimStateToken(stateStr);
    if (trimmed.empty() || trimmed == "normal") {
        return "normal";
    }

    std::map<String, String> props;
    size_t start = 0;
    while (start < trimmed.size()) {
        size_t end = trimmed.find(',', start);
        if (end == String::npos) {
            end = trimmed.size();
        }

        String token = trimStateToken(StringView(trimmed.data() + start, end - start));
        if (!token.empty()) {
            size_t eq = token.find('=');
            if (eq != String::npos) {
                String key = trimStateToken(StringView(token.data(), eq));
                String value = trimStateToken(StringView(token.data() + eq + 1, token.size() - eq - 1));
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

    String normalized;
    bool first = true;
    for (const auto& [key, value] : props) {
        if (!first) {
            normalized += ",";
        }
        normalized += key + "=" + value;
        first = false;
    }

    return normalized;
}

std::vector<std::pair<String, String>> parseStateConditions(StringView stateStr) {
    std::vector<std::pair<String, String>> conditions;

    const String normalized = normalizeStateString(stateStr);
    if (normalized.empty() || normalized == "normal") {
        return conditions;
    }

    size_t start = 0;
    while (start < normalized.size()) {
        size_t end = normalized.find(',', start);
        if (end == String::npos) {
            end = normalized.size();
        }

        StringView token(normalized.data() + start, end - start);
        size_t eq = token.find('=');
        if (eq != StringView::npos) {
            String key(token.substr(0, eq));
            String value(token.substr(eq + 1));
            if (!key.empty()) {
                conditions.emplace_back(std::move(key), std::move(value));
            }
        }

        start = end + 1;
    }

    return conditions;
}

bool matchConditions(
    const std::vector<std::pair<String, String>>& conditions,
    const std::map<String, String>& properties)
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

Result<void> ResourceManager::addResourcePack(ResourcePackPtr resourcePack) {
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

Result<void> ResourceManager::loadAllResources() {
    // 设置模型加载器的资源包列表
    m_modelLoader.setResourcePackList(m_resourcePacks);

    // 加载方块状态
    for (auto& pack : m_resourcePacks) {
        m_blockStateLoader.loadFromResourcePack(*pack);
    }

    // 加载模型 (按需加载，不预加载)
    for (auto& pack : m_resourcePacks) {
        m_modelLoader.loadFromResourcePack(*pack);
    }

    // 烘焙模型
    bakeAllModels();

    // 注意：computeBlockAppearances 在 buildTextureAtlas 后调用
    // 因为需要纹理区域数据

    spdlog::info("ResourceManager: Loaded {} block states, {} models",
                m_blockStateLoader.getLoadedBlockStates().size(),
                m_bakedModels.size());

    return Result<void>::ok();
}

Result<AtlasBuildResult> ResourceManager::buildTextureAtlas() {
    // 收集所需纹理
    auto textures = collectRequiredTextures();

    spdlog::info("Collecting {} textures for atlas", textures.size());

    TextureAtlasBuilder builder;
    builder.setMaxSize(4096, 4096);
    builder.setPadding(0);

    // 使用 compat 层的 TextureMapper 进行路径映射
    const auto& texMapper = resource::compat::TextureMapper::instance();

    // 统计
    size_t addedCount = 0;
    size_t variantCount = 0;
    size_t failedCount = 0;
    std::vector<String> failedTextures;

    for (const auto& texLoc : textures) {
        bool added = false;

        // 获取纹理路径的所有变体（包括 block/blocks 路径互换和名称映射）
        String texPath = texLoc.path();
        auto pathVariants = texMapper.getPathVariants(texPath);

        // 遍历资源包（后添加的优先级更高）
        for (auto it = m_resourcePacks.rbegin(); it != m_resourcePacks.rend() && !added; ++it) {
            auto& pack = *it;

            // 尝试所有路径变体
            for (const auto& variantPath : pathVariants) {
                ResourceLocation variantLoc(texLoc.namespace_(), variantPath);
                String filePath = variantLoc.toFilePath("png");

                if (pack->hasResource(filePath)) {
                    auto readResult = pack->readResource(filePath);
                    if (readResult.success()) {
                        int width, height, channels;
                        stbi_uc* pixels = stbi_load_from_memory(
                            readResult.value().data(),
                            static_cast<int>(readResult.value().size()),
                            &width, &height, &channels, 4);

                        if (pixels) {
                            std::vector<u8> pixelData(pixels, pixels + width * height * 4);
                            stbi_image_free(pixels);

                            // 用原始请求名称注册
                            builder.addTexture(texLoc, pixelData,
                                               static_cast<u32>(width),
                                               static_cast<u32>(height));
                            added = true;
                            addedCount++;

                            // 检查是否使用了变体路径
                            if (variantPath != texPath) {
                                variantCount++;
                            }
                            break;
                        }
                    }
                }
            }
        }

        if (!added) {
            failedTextures.push_back(texLoc.toString() + " -> " + texLoc.toFilePath("png"));
            failedCount++;
        }
    }

    spdlog::info("Texture atlas: {} added ({} via variant mapping), {} failed",
                 addedCount, variantCount, failedCount);

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

    // 构建纹理图集后计算方块外观（这样纹理区域可用）
    computeBlockAppearances();

    spdlog::info("ResourceManager: {} appearances computed", m_blockAppearances.size());

    return m_atlasResult;
}

const BlockAppearance* ResourceManager::getBlockAppearance(
    const ResourceLocation& blockId,
    const std::map<String, String>& properties) const
{
    // 构建缓存键
    String cacheKey = blockId.toString();
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
    const String blockPrefix = blockId.toString();
    const String blockWithQueryPrefix = blockPrefix + "?";

    const BlockAppearance* bestMatch = nullptr;
    const BlockAppearance* firstBlockAppearance = nullptr;
    size_t bestSpecificity = 0;

    for (const auto& [appearanceKey, appearance] : m_blockAppearances) {
        if (appearanceKey != blockPrefix && appearanceKey.rfind(blockWithQueryPrefix, 0) != 0) {
            continue;
        }

        if (!firstBlockAppearance) {
            firstBlockAppearance = &appearance;
        }

        StringView statePart = "normal";
        if (appearanceKey.size() > blockWithQueryPrefix.size() &&
            appearanceKey.rfind(blockWithQueryPrefix, 0) == 0) {
            statePart = StringView(appearanceKey.data() + blockWithQueryPrefix.size(),
                                   appearanceKey.size() - blockWithQueryPrefix.size());
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

const TextureRegion* ResourceManager::getTextureRegion(
    const ResourceLocation& textureLocation) const
{
    // 先尝试直接查找
    auto it = m_textureRegions.find(textureLocation);
    if (it != m_textureRegions.end()) {
        return &it->second;
    }

    // 使用 compat 层尝试路径变体
    const auto& texMapper = resource::compat::TextureMapper::instance();
    auto pathVariants = texMapper.getPathVariants(textureLocation.path());

    for (const auto& variantPath : pathVariants) {
        ResourceLocation variantLoc(textureLocation.namespace_(), variantPath);
        it = m_textureRegions.find(variantLoc);
        if (it != m_textureRegions.end()) {
            return &it->second;
        }
    }

    return nullptr;
}

Result<DecodedTexture> ResourceManager::loadTextureRGBA(
    const ResourceLocation& textureLocation) const
{
    if (m_resourcePacks.empty()) {
        return Error(ErrorCode::NotFound,
                     "No resource packs available for texture: " + textureLocation.toString());
    }

    const auto& texMapper = resource::compat::TextureMapper::instance();
    const auto pathVariants = texMapper.getPathVariants(textureLocation.path());

    for (auto packIt = m_resourcePacks.rbegin(); packIt != m_resourcePacks.rend(); ++packIt) {
        const auto& pack = *packIt;
        for (const auto& variantPath : pathVariants) {
            const ResourceLocation variantLocation(textureLocation.namespace_(), variantPath);
            const String filePath = variantLocation.toFilePath("png");
            if (!pack->hasResource(filePath)) {
                continue;
            }

            const auto readResult = pack->readResource(filePath);
            if (readResult.failed()) {
                continue;
            }

            int width = 0;
            int height = 0;
            int channels = 0;
            stbi_uc* pixels = stbi_load_from_memory(
                readResult.value().data(),
                static_cast<int>(readResult.value().size()),
                &width,
                &height,
                &channels,
                4);

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
                pixels,
                pixels + (static_cast<size_t>(decoded.width) * static_cast<size_t>(decoded.height) * 4));
            stbi_image_free(pixels);

            return decoded;
        }
    }

    return Error(ErrorCode::NotFound,
                 "Texture not found in any resource pack: " + textureLocation.toString());
}

const BakedBlockModel* ResourceManager::getBakedModel(
    const ResourceLocation& modelLocation)
{
    // 检查缓存
    auto it = m_bakedModels.find(modelLocation);
    if (it != m_bakedModels.end()) {
        return &it->second;
    }

    // 烘焙模型
    // 首先需要设置资源包
    if (!m_resourcePacks.empty()) {
        m_modelLoader.loadFromResourcePack(*m_resourcePacks[0]);
    }

    auto result = m_modelLoader.bakeModel(modelLocation);
    if (result.failed()) {
        return nullptr;
    }

    m_bakedModels[modelLocation] = result.value();
    return &m_bakedModels[modelLocation];
}

const BlockStateDefinition* ResourceManager::getBlockState(
    const ResourceLocation& blockId) const
{
    return m_blockStateLoader.getBlockState(blockId);
}

void ResourceManager::clear() {
    m_resourcePacks.clear();
    m_modelLoader.clearCache();
    m_blockStateLoader.clearCache();
    m_bakedModels.clear();
    m_blockAppearances.clear();
    m_textureRegions.clear();
    m_atlasResult = AtlasBuildResult();
    m_atlasBuilt = false;
}

void ResourceManager::clearResourcePacks() {
    m_resourcePacks.clear();
}

Result<void> ResourceManager::reload() {
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

Result<void> ResourceManager::bakeAllModels() {
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
                                         variant.model.toString(), blockId.toString(),
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
        spdlog::warn("ResourceManager: Baked {} models ({} failed)",
                     successCount, failCount);
    } else {
        spdlog::info("ResourceManager: Baked {} models successfully", successCount);
    }

    return Result<void>::ok();
}

void ResourceManager::computeBlockAppearances() {
    // 获取所有方块状态
    auto blockStates = m_blockStateLoader.getLoadedBlockStates();

    // 使用 compat 层进行纹理路径映射
    const auto& texMapper = resource::compat::TextureMapper::instance();

    u32 totalAppearances = 0;
    u32 appearancesWithTextures = 0;

    for (const auto& blockId : blockStates) {
        const auto* def = m_blockStateLoader.getBlockState(blockId);
        if (!def) continue;

        // 处理每个状态变体
        for (const auto& [stateStr, variantList] : def->getAllVariants()) {
            // 构建缓存键
            String cacheKey = blockId.toString();
            const String normalizedState = normalizeStateString(stateStr);
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
                for (const auto& [dir, face] : element.faces) {
                    String dirStr = directionToString(dir);

                    // 保留 tintindex（仅记录有着色需求的面）
                    if (face.tintIndex >= 0 && !appearance.faceTintIndices.count(dirStr)) {
                        appearance.faceTintIndices[dirStr] = face.tintIndex;
                    }

                    ResourceLocation texLoc = bakedModel.resolveTexture(face.texture);

                    // 转换纹理路径为完整的 textures/ 路径
                    ResourceLocation fullTexLoc = texturePathToLocation(texLoc.path());

                    // 使用 compat 层查找纹理区域
                    const TextureRegion* region = findTextureRegion(fullTexLoc);

                    if (region) {
                        // 保留首层纹理用于兼容旧逻辑
                        if (!appearance.faceTextures.count(dirStr)) {
                            appearance.faceTextures[dirStr] = *region;
                        }

                        // 收集全部层，支持草方块侧面 overlay 等多层模型
                        appearance.faceTextureLayers[dirStr].push_back(
                            BlockAppearance::FaceTextureLayer{*region, face.tintIndex}
                        );
                    }
                }
            }

            totalAppearances++;
            if (!appearance.faceTextures.empty()) {
                appearancesWithTextures++;
            }

            m_blockAppearances[cacheKey] = std::move(appearance);
        }
    }

    spdlog::info("computeBlockAppearances: {} total, {} with textures",
                 totalAppearances, appearancesWithTextures);
}

const TextureRegion* ResourceManager::findTextureRegion(const ResourceLocation& texLoc) const {
    // 1. 尝试原始路径
    auto it = m_textureRegions.find(texLoc);
    if (it != m_textureRegions.end()) {
        return &it->second;
    }

    // 2. 使用 compat 层尝试所有路径变体
    const auto& texMapper = resource::compat::TextureMapper::instance();
    auto pathVariants = texMapper.getPathVariants(texLoc.path());

    for (const auto& variantPath : pathVariants) {
        ResourceLocation variantLoc(texLoc.namespace_(), variantPath);
        it = m_textureRegions.find(variantLoc);
        if (it != m_textureRegions.end()) {
            return &it->second;
        }
    }

    return nullptr;
}

std::set<ResourceLocation> ResourceManager::collectRequiredTextures() const {
    std::set<ResourceLocation> textures;

    // 从烘焙模型收集纹理
    for (const auto& [loc, model] : m_bakedModels) {
        for (const auto& [name, texLoc] : model.textures) {
            // 跳过纹理变量引用（以 # 开头的值）
            String texPath = texLoc.path();
            if (!texPath.empty() && texPath[0] == '#') {
                // 纹理变量引用，不应该出现在烘焙模型中
                spdlog::warn("Texture variable reference found in baked model {}: {}={}",
                            loc.toString(), name, texPath);
                continue;
            }

            // 转换为纹理路径
            ResourceLocation textureLoc = texturePathToLocation(texPath);
            textures.insert(textureLoc);
        }
    }

    return textures;
}

ResourceLocation ResourceManager::texturePathToLocation(StringView path) {
    // 处理纹理路径
    // 例如: "blocks/stone" -> "minecraft:textures/blocks/stone"
    String p(path);

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

IResourcePack* ResourceManager::getFirstResourcePack() {
    if (m_resourcePacks.empty()) {
        return nullptr;
    }
    return m_resourcePacks[0].get();
}

IResourcePack* ResourceManager::getResourcePack(size_t index) {
    if (index >= m_resourcePacks.size()) {
        return nullptr;
    }
    return m_resourcePacks[index].get();
}

} // namespace mc
