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

#include "BlockModelCache.hpp"
#include "ResourceManager.hpp"

#include <spdlog/spdlog.h>

#include "common/profiler/TraceEvents.hpp"
#include "common/util/PlatformInfo.hpp"
#include "common/util/assert/AssertAll.hpp"

using namespace mc::trace;

namespace mc {
namespace {

/**
 * @brief 解析 "key=value,key2=value2" 格式的属性字符串
 *
 * @param properties 属性字符串
 * @return 属性键值对映射
 */
std::map<std::string, std::string> parsePropertiesString(std::string_view properties)
{
    std::map<std::string, std::string> props;
    if (properties.empty()) {
        return props;
    }

    size_t start = 0;
    while (start < properties.size()) {
        size_t end = properties.find(',', start);
        if (end == std::string_view::npos) {
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

    return props;
}

} // namespace

// ============================================================================
// 初始化和重建
// ============================================================================

bool BlockModelCache::initialize(ResourceManager& resourceManager)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "BlockModelCache::initialize");

    spdlog::info("Initializing BlockModelCache...");

    // 阶段4：BlockModelCache::initialize。注意本阶段自身只构建轻量 m_stateCache
    // （~180KB），其入口时上游 buildTextureAtlas 产出的 m_blockAppearances /
    // m_atlasResult.pixels 已驻留，故阶段 Δ 主要反映上游内存，非本缓存自身。
    const i64 commitBefore = static_cast<i64>(util::PlatformInfo::getProcessCommitMB());
    const i64 wsBefore = static_cast<i64>(util::PlatformInfo::getProcessMemoryMB());

    m_resourceManager = &resourceManager;

    // 创建缺失模型外观
    _createMissingAppearance();

    // 构建状态缓存
    _buildStateCache();

    m_initialized = true;
    spdlog::info("BlockModelCache initialized: {} appearances cached", m_stateCache.size());

    const i64 commitAfter = static_cast<i64>(util::PlatformInfo::getProcessCommitMB());
    const i64 wsAfter = static_cast<i64>(util::PlatformInfo::getProcessMemoryMB());
    spdlog::info("[MemPhase] BlockModelCache::initialize | commit {}->{} (Δ{:+}MB) | ws {}->{} (Δ{:+}MB)",
        commitBefore,
        commitAfter,
        commitAfter - commitBefore,
        wsBefore,
        wsAfter,
        wsAfter - wsBefore);
    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Resource,
        "Phase::BlockModelCache::initialize",
        "commitBeforeMB",
        commitBefore,
        "commitAfterMB",
        commitAfter,
        "commitDeltaMB",
        commitAfter - commitBefore,
        "wsDeltaMB",
        wsAfter - wsBefore);

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
    m_regionLookup = nullptr;
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
    const BlockAppearance* appearance =
        m_resourceManager->getBlockAppearance(block->blockLocation(), parsePropertiesString(properties));

    return appearance ? appearance : getMissingAppearance();
}

const BlockAppearance* BlockModelCache::getMissingAppearance() const
{
    return m_missingAppearance.get();
}

const TextureRegion* BlockModelCache::regionLookup(const ResourceLocation& textureLocation) const
{
    if (!m_regionLookup) {
        return nullptr;
    }
    return m_regionLookup(textureLocation);
}

// ============================================================================
// 私有方法
// ============================================================================

void BlockModelCache::_buildStateCache()
{
    MC_ASSERT_RELEASE(m_resourceManager != nullptr);

    m_stateCache.clear();

    size_t successCount = 0;
    size_t failCount = 0;

    // 遍历所有方块状态
    Block::forEachBlockState([this, &successCount, &failCount](const BlockState& state) {
        u32 stateId = state.stateId();

        // 获取方块资源位置
        const ResourceLocation& blockLoc = state.blockLocation();

        // 获取模型键（属性字符串）并解析为属性映射
        std::map<std::string, std::string> props = parsePropertiesString(state.toModelKey());

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

void BlockModelCache::_createMissingAppearance()
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
        element.at(dir) = modelFace;
    }

    m_missingAppearance->elements.push_back(element);
    m_missingAppearance->xRotation = 0;
    m_missingAppearance->yRotation = 0;
    m_missingAppearance->uvLock = false;

    // 设置面纹理映射
    // 使用 DefaultTextureAtlas 中第一个位置的 UV 坐标
    // DefaultTextureAtlas: ATLAS_SIZE=256, TILE_SIZE=16, tilesPerRow=16
    // 第一个位置 (0,0) 是缺失纹理，UV 坐标是 (0, 0, 1/16, 1/16)
    constexpr f32 tileUV = 1.0f / 16.0f;
    TextureRegion missingRegion(0.0f, 0.0f, tileUV, tileUV);
    ResourceLocation missingLoc("minecraft:textures/block/missing");
    for (Direction dir : Directions::all()) {
        const size_t idx = Directions::index(dir);
        m_missingAppearance->faceTextures[idx] = missingRegion;
        m_missingAppearance->faceTextureLocations[idx] = missingLoc;
    }

    // 缺失模型的粒子纹理与面纹理一致
    m_missingAppearance->particleTexture = missingRegion;
    m_missingAppearance->particleTextureLocation = missingLoc;
    m_missingAppearance->hasParticleTexture = true;
}

} // namespace mc
