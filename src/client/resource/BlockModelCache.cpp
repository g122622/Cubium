#include "BlockModelCache.hpp"
#include "ResourceManager.hpp"

#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 初始化和重建
// ============================================================================

bool BlockModelCache::initialize(ResourceManager& resourceManager)
{
    spdlog::info("Initializing BlockModelCache...");

    m_resourceManager = &resourceManager;

    // 创建缺失模型外观
    createMissingAppearance();

    // 构建状态缓存
    buildStateCache();

    m_initialized = true;
    spdlog::info("BlockModelCache initialized: {} appearances cached", m_stateCache.size());

    return true;
}

bool BlockModelCache::rebuild(ResourceManager& resourceManager)
{
    spdlog::info("Rebuilding BlockModelCache...");

    // 清除旧缓存
    clear();

    // 重新初始化
    return initialize(resourceManager);
}

void BlockModelCache::clear()
{
    m_stateCache.clear();
    m_missingAppearance.reset();
    m_resourceManager = nullptr;
    m_initialized = false;
}

// ============================================================================
// 外观查询
// ============================================================================

const BlockAppearance* BlockModelCache::getBlockAppearance(const BlockState* state) const
{
    if (!state || !m_initialized) {
        return getMissingAppearance();
    }

    // 渲染热路径直接命中状态缓存，避免重复构建属性字符串。
    return getBlockAppearance(state->stateId());
}

const BlockAppearance* BlockModelCache::getBlockAppearance(u32 stateId) const
{
    if (!m_initialized) {
        return getMissingAppearance();
    }

    auto it = m_stateCache.find(stateId);
    if (it != m_stateCache.end()) {
        return it->second;
    }

    return getMissingAppearance();
}

const BlockAppearance* BlockModelCache::getBlockAppearance(u32 blockId, const std::string& properties) const
{
    if (!m_resourceManager || !m_initialized) {
        return getMissingAppearance();
    }

    // 获取方块
    Block* block = Block::getBlock(blockId);
    if (!block) {
        return getMissingAppearance();
    }

    // 从 ResourceManager 获取外观
    // 构建 properties map
    std::map<std::string, std::string> props;
    if (!properties.empty()) {
        // 解析 "key=value,key2=value2" 格式
        size_t start = 0;
        size_t end = 0;

        while (start < properties.size()) {
            end = properties.find(',', start);
            if (end == std::string::npos) {
                end = properties.size();
            }

            std::string_view pair(properties.data() + start, end - start);
            size_t eq = pair.find('=');
            if (eq != std::string_view::npos) {
                std::string key(pair.substr(0, eq));
                std::string value(pair.substr(eq + 1));
                props[key] = value;
            }

            start = end + 1;
        }
    }

    const BlockAppearance* appearance = m_resourceManager->getBlockAppearance(block->blockLocation(), props);

    return appearance ? appearance : getMissingAppearance();
}

const BlockAppearance* BlockModelCache::getMissingAppearance() const
{
    return m_missingAppearance.get();
}

// ============================================================================
// 私有方法
// ============================================================================

void BlockModelCache::buildStateCache()
{
    if (!m_resourceManager) {
        spdlog::warn("BlockModelCache: Cannot build cache without ResourceManager");
        return;
    }

    m_stateCache.clear();

    size_t successCount = 0;
    size_t failCount = 0;

    // 遍历所有方块状态
    Block::forEachBlockState([this, &successCount, &failCount](const BlockState& state) {
        u32 stateId = state.stateId();

        // 获取方块资源位置
        const ResourceLocation& blockLoc = state.blockLocation();

        // 获取模型键（属性字符串）
        std::string modelKey = state.toModelKey();

        // 构建 properties map
        std::map<std::string, std::string> props;
        if (!modelKey.empty()) {
            // 解析 "key=value,key2=value2" 格式
            size_t start = 0;
            size_t end = 0;

            while (start < modelKey.size()) {
                end = modelKey.find(',', start);
                if (end == std::string::npos) {
                    end = modelKey.size();
                }

                std::string_view pair(modelKey.data() + start, end - start);
                size_t eq = pair.find('=');
                if (eq != std::string_view::npos) {
                    std::string key(pair.substr(0, eq));
                    std::string value(pair.substr(eq + 1));
                    props[key] = value;
                }

                start = end + 1;
            }
        }

        // 从 ResourceManager 获取外观
        const BlockAppearance* appearance = m_resourceManager->getBlockAppearance(blockLoc, props);

        if (appearance) {
            m_stateCache[stateId] = appearance;
            ++successCount;
        } else {
            // 使用缺失模型
            m_stateCache[stateId] = m_missingAppearance.get();
            ++failCount;
        }
    });

    spdlog::info("BlockModelCache built: {} successes, {} failures", successCount, failCount);
}

void BlockModelCache::createMissingAppearance()
{
    m_missingAppearance = std::make_unique<BlockAppearance>();

    // 创建一个简单的紫黑方块作为缺失模型
    ModelElement element;
    element.from = {0.0f, 0.0f, 0.0f};
    element.to = {16.0f, 16.0f, 16.0f};

    // 创建所有面的纹理引用
    for (Direction dir : Directions::all()) {
        ModelFace modelFace;
        modelFace.texture = "#missing";
        modelFace.uv = {0.0f, 0.0f, 16.0f, 16.0f};
        element.faces[dir] = modelFace;
    }

    m_missingAppearance->elements.push_back(element);
    m_missingAppearance->xRotation = 0;
    m_missingAppearance->yRotation = 0;
    m_missingAppearance->uvLock = false;

    // 设置面纹理映射
    // 使用 DefaultTextureAtlas 中第一个位置的 UV 坐标
    // DefaultTextureAtlas: ATLAS_SIZE=256, TILE_SIZE=16, tilesPerRow=16
    // 第一个位置 (0,0) 是缺失纹理，UV 坐标是 (0, 0, 1/16, 1/16)
    constexpr f32 tileUV = 1.0f / 16.0f; // 0.0625
    TextureRegion missingRegion(0.0f, 0.0f, tileUV, tileUV);
    m_missingAppearance->faceTextures["down"] = missingRegion;
    m_missingAppearance->faceTextures["up"] = missingRegion;
    m_missingAppearance->faceTextures["north"] = missingRegion;
    m_missingAppearance->faceTextures["south"] = missingRegion;
    m_missingAppearance->faceTextures["west"] = missingRegion;
    m_missingAppearance->faceTextures["east"] = missingRegion;

    spdlog::debug("BlockModelCache: Created missing appearance with {} elements, {} faceTextures, UV=({},{},{},{})",
        m_missingAppearance->elements.size(),
        m_missingAppearance->faceTextures.size(),
        missingRegion.u0,
        missingRegion.v0,
        missingRegion.u1,
        missingRegion.v1);
}

} // namespace mc
