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

#include "WorldLightManager.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"
#include <algorithm>
#include <memory>
#include <string>

namespace {

/// TLS 天空光引擎池：每个 worker 线程独占一套引擎实例。
/// 对齐 Moonrise StarLightInterface 的 thread_local 引擎池——引擎无跨 operation
/// 持久状态（nibble/emptiness map 全挂 IChunk，引擎只有 per-op 缓存），故线程级
/// 单例安全复用，无引擎级锁。nibble 写串行由 UniversalWorkerPool 区域锁保证。
thread_local std::unique_ptr<mc::SkyStarLightEngine> _tlsSkyEngine;

/// TLS 方块光引擎池（语义同 _tlsSkyEngine）。
thread_local std::unique_ptr<mc::BlockStarLightEngine> _tlsBlockEngine;

} // namespace

namespace mc {

WorldLightManager::WorldLightManager(StarLightLightingProvider* provider, bool hasBlockLight, bool hasSkyLight)
    : m_provider(provider)
    , m_hasBlockLight(hasBlockLight)
    , m_hasSkyLight(hasSkyLight)
    , m_minLightSection((provider->getMinBuildHeight() >> world::SECTION_SHIFT) - 1)
    , m_maxLightSection(((provider->getMaxBuildHeight() - 1) >> world::SECTION_SHIFT) + 1)
    , m_minSection(provider->getMinBuildHeight() >> world::SECTION_SHIFT)
    , m_maxSection((provider->getMaxBuildHeight() - 1) >> world::SECTION_SHIFT)
{}

// ============================================================================
// TLS 引擎池
// ============================================================================

SkyStarLightEngine* WorldLightManager::acquireSkyLightEngine()
{
    // 首次调用惰性构造，之后该线程复用同一实例。引擎构造无参数（无跨 op 状态）。
    if (_tlsSkyEngine == nullptr) {
        _tlsSkyEngine = std::make_unique<SkyStarLightEngine>();
    }
    return _tlsSkyEngine.get();
}

void WorldLightManager::releaseSkyLightEngine(SkyStarLightEngine* /*engine*/) noexcept
{
    // no-op：对齐 Moonrise try/finally 签名，引擎留在 TLS 复用，线程退出时 unique_ptr 析构释放。
}

BlockStarLightEngine* WorldLightManager::acquireBlockLightEngine()
{
    if (_tlsBlockEngine == nullptr) {
        _tlsBlockEngine = std::make_unique<BlockStarLightEngine>();
    }
    return _tlsBlockEngine.get();
}

void WorldLightManager::releaseBlockLightEngine(BlockStarLightEngine* /*engine*/) noexcept
{
    // no-op（见 releaseSkyLightEngine）。
}

// ============================================================================
// 主线程读路径（visible 侧，无锁）
// ============================================================================

i32 WorldLightManager::getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const
{
    // 主线程读 visible 侧，不经引擎、不持锁。worker 写 updating 经区域锁串行，
    // visible 侧 atomic 发布，主线程无锁读安全（SWMRNibbleArray 双缓冲语义）。
    // 对齐 Moonrise StarLightInterface.getRawBrightness。
    i32 skyLight = 0;
    if (m_hasSkyLight) {
        skyLight = static_cast<i32>(_getSkyLightValue(pos.x, pos.y, pos.z)) - skyDarkening;
        skyLight = std::max(0, skyLight);
        // 天空光满亮时方块光不可能更高，短路返回（对齐 Moonrise）
        if (skyLight == 15) {
            return 15;
        }
    }

    i32 blockLight = 0;
    if (m_hasBlockLight) {
        const SectionPos sectionPos(
            pos.x >> world::CHUNK_SHIFT, pos.y >> world::SECTION_SHIFT, pos.z >> world::CHUNK_SHIFT);
        SWMRNibbleArray* blockNibble = _getNibble(LightType::BLOCK, sectionPos);
        if (blockNibble != nullptr) {
            blockLight = static_cast<i32>(blockNibble->getVisible(
                pos.x & world::CHUNK_MASK, pos.y & world::CHUNK_MASK, pos.z & world::CHUNK_MASK));
        }
    }

    return std::max(blockLight, skyLight);
}

u8 WorldLightManager::getBlockLight(i32 x, i32 y, i32 z) const
{
    if (!m_hasBlockLight) {
        return 0;
    }

    const SectionPos sectionPos(x >> world::CHUNK_SHIFT, y >> world::SECTION_SHIFT, z >> world::CHUNK_SHIFT);
    SWMRNibbleArray* nibble = _getNibble(LightType::BLOCK, sectionPos);
    if (nibble == nullptr) {
        return 0;
    }
    // 方块光 null nibble 返回 0（无光源），与 SWMRNibbleArray::getVisible 一致
    return nibble->getVisible(x & world::CHUNK_MASK, y & world::CHUNK_MASK, z & world::CHUNK_MASK);
}

u8 WorldLightManager::getSkyLight(i32 x, i32 y, i32 z) const
{
    if (!m_hasSkyLight) {
        return 15;
    }
    return _getSkyLightValue(x, y, z);
}

SWMRNibbleArray* WorldLightManager::getData(LightType type, const SectionPos& pos)
{
    return _getNibble(type, pos);
}

SWMRNibbleArray* WorldLightManager::_getNibble(LightType type, const SectionPos& pos) const
{
    // 主线程读：经 provider 取已加载区块，直接索引区块上的 nibble 数组。
    // 不持锁——nibble 指针挂在区块上（稳定），其 visible 侧数据 atomic 读安全。
    const i32 index = pos.y - m_minLightSection;
    if (index < 0 || index >= (m_provider->getSectionCount() + 2)) {
        return nullptr;
    }

    const IChunk* chunk = m_provider->getChunkForLight(pos.x, pos.z);
    if (chunk == nullptr) {
        return nullptr;
    }

    SWMRNibbleArray* const* nibbles = (type == LightType::SKY) ? chunk->getSkyNibbles() : chunk->getBlockNibbles();
    if (nibbles == nullptr) {
        return nullptr;
    }

    return nibbles[index];
}

u8 WorldLightManager::_getSkyLightValue(i32 x, i32 y, i32 z) const
{
    // 对齐 Moonrise StarLightInterface.getSkyLightValue：
    // null nibble（未光照段）的天空光须经 emptiness map 判断该段是否在最低非空段之上
    // （之上天空光未遮挡满亮 15），否则向上回溯首个非 null nibble 取该列天空光。
    // 区块未加载返回 15（对齐 Moonrise chunk==null 分支）。
    const i32 chunkX = x >> world::CHUNK_SHIFT;
    const i32 chunkZ = z >> world::CHUNK_SHIFT;
    const IChunk* chunk = m_provider->getChunkForLight(chunkX, chunkZ);
    if (chunk == nullptr) {
        return 15;
    }

    i32 sectionY = y >> world::SECTION_SHIFT;
    if (sectionY > m_maxLightSection) {
        return 15;
    }
    if (sectionY < m_minLightSection) {
        sectionY = m_minLightSection;
        y = sectionY << world::SECTION_SHIFT;
    }

    SWMRNibbleArray* const* nibbles = chunk->getSkyNibbles();
    if (nibbles == nullptr) {
        return 15;
    }

    SWMRNibbleArray* immediate = nibbles[sectionY - m_minLightSection];
    if (immediate != nullptr && !immediate->isNullVisible()) {
        return immediate->getVisible(x & world::CHUNK_MASK, y & world::CHUNK_MASK, z & world::CHUNK_MASK);
    }

    // null nibble：经 emptiness map 查找最低非空段
    const bool* emptinessMap = chunk->getSkyEmptinessMap();
    if (emptinessMap == nullptr) {
        return 15;
    }

    i32 lowestY = m_minLightSection - 1;
    for (i32 currY = m_maxSection; currY >= m_minSection; --currY) {
        if (emptinessMap[currY - m_minSection]) {
            continue;
        }
        lowestY = currY;
        break;
    }

    if (sectionY > lowestY) {
        return 15;
    }

    // 向上回溯首个非 null nibble，取该列天空光（对齐 Moonrise：上方未遮挡段的光照下来）
    for (i32 currY = sectionY + 1; currY <= m_maxLightSection; ++currY) {
        SWMRNibbleArray* nibble = nibbles[currY - m_minLightSection];
        if (nibble != nullptr && !nibble->isNullVisible()) {
            return nibble->getVisible(x & world::CHUNK_MASK, 0, z & world::CHUNK_MASK);
        }
    }

    return 15;
}

// ============================================================================
// 调试信息（读 visible 侧状态）
// ============================================================================

std::string WorldLightManager::getDebugInfo(LightType type, const SectionPos& pos) const
{
    // 主线程读 visible 侧状态，反映已发布的光照数据（对齐客户端可见状态）。
    // 引擎已改 TLS 池——主线程引擎实例与 worker 引擎不同，queuedUpdateSize 无意义，故不再报告 [q:N]。
    switch (type) {
        case LightType::BLOCK: {
            if (!m_hasBlockLight) {
                return "BlockLight: N/A";
            }

            const SWMRNibbleArray* nibble = _getNibble(LightType::BLOCK, pos);
            std::string sectionState;
            if (nibble == nullptr) {
                sectionState = "2"; // EMPTY - 无数据
            } else if (nibble->isNullVisible()) {
                sectionState = "2"; // EMPTY - Null 状态
            } else if (nibble->isUninitializedVisible()) {
                sectionState = "1"; // LIGHT_ONLY - 未初始化
            } else if (nibble->isHiddenVisible()) {
                sectionState = "1"; // LIGHT_ONLY - 隐藏状态
            } else if (nibble->isInitializedVisible()) {
                sectionState = "0"; // LIGHT_AND_DATA - 有完整数据
            }

            std::string result = "BlockLight:" + sectionState;

            // 附加脏标记
            if (nibble != nullptr && nibble->isDirty()) {
                result += "[dirty]";
            }

            return result;
        }
        case LightType::SKY: {
            if (!m_hasSkyLight) {
                return "SkyLight: N/A";
            }

            const SWMRNibbleArray* nibble = _getNibble(LightType::SKY, pos);
            std::string sectionState;
            if (nibble == nullptr) {
                sectionState = "2"; // EMPTY - 无数据
            } else if (nibble->isNullVisible()) {
                sectionState = "2"; // EMPTY - Null 状态
            } else if (nibble->isUninitializedVisible()) {
                sectionState = "1"; // LIGHT_ONLY - 未初始化
            } else if (nibble->isHiddenVisible()) {
                sectionState = "1"; // LIGHT_ONLY - 隐藏状态
            } else if (nibble->isInitializedVisible()) {
                sectionState = "0"; // LIGHT_AND_DATA - 有完整数据
            }

            std::string result = "SkyLight:" + sectionState;

            // 附加脏标记
            if (nibble != nullptr && nibble->isDirty()) {
                result += "[dirty]";
            }

            return result;
        }
    }
    return "Unknown";
}

} // namespace mc
