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

#include "BaseLightEngine.hpp"
#include "common/core/Types.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/LightType.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using namespace mc::trace;

namespace mc {

// ============================================================================
// 辅助函数：将 CollisionShape 转换为 VoxelShape
// ============================================================================

namespace {

std::unique_ptr<bool[]> _copyEmptinessMap(const std::vector<bool>& emptinessMap, size_t targetSize)
{
    auto rawMap = std::make_unique<bool[]>(targetSize);
    std::fill_n(rawMap.get(), targetSize, false);

    const size_t copySize = std::min(emptinessMap.size(), targetSize);
    for (size_t i = 0; i < copySize; ++i) {
        rawMap[i] = emptinessMap[i];
    }
    return rawMap;
}

} // anonymous namespace

// ============================================================================
// 静态成员初始化
// ============================================================================

std::array<std::vector<LightAxisDirection>, 64> StarLightEngine::s_oldCheckDirections;
bool StarLightEngine::s_directionsInitialized = false;

void StarLightEngine::_initializeDirections()
{
    if (s_directionsInitialized) return;

    for (i32 i = 0; i < 64; ++i) {
        std::vector<LightAxisDirection> directions;
        for (i32 bitset = i, count = 0; count < 6 && bitset != 0; ++count) {
            i32 trailing = bitset & -bitset; // 获取最低位1
            i32 index = 0;
            while ((1 << index) != trailing)
                ++index;
            directions.push_back(static_cast<LightAxisDirection>(index));
            bitset ^= trailing;
        }
        s_oldCheckDirections[static_cast<size_t>(i)] = std::move(directions);
    }
    s_directionsInitialized = true;
}

// ============================================================================
// 构造函数
// ============================================================================

StarLightEngine::StarLightEngine(bool isSkyLight)
    : m_isSkyLight(isSkyLight)
    , m_emittedLightMask(isSkyLight ? 0 : 0xF)
{

    _initializeDirections();
    m_chunkCache.fill(nullptr);
    m_emptinessMapCache.fill(nullptr);
}

// ============================================================================
// 缓存管理
// ============================================================================

void StarLightEngine::setWorld(void* world)
{
    m_world = world;
    // 子类会设置更多参数
}

void StarLightEngine::setupEncodeOffset(i32 centerX, i32 centerY, i32 centerZ)
{
    // 31 = center + encodeOffset，使中心坐标映射到 [0, 62] 范围
    m_encodeOffsetX = 31 - centerX;
    m_encodeOffsetY = -(m_minLightSection - 1) << world::SECTION_SHIFT; // 使最小光照段Y=0
    m_encodeOffsetZ = 31 - centerZ;

    // coordinateIndex = x | (z << 6) | (y << (6 + 6))
    m_coordinateOffset = m_encodeOffsetX + (m_encodeOffsetZ << 6) + (m_encodeOffsetY << 12);

    // 2 = (centerX >> CHUNK_SHIFT) + chunkOffset，使中心区块在缓存中心
    m_chunkOffsetX = 2 - (centerX >> world::CHUNK_SHIFT);
    m_chunkOffsetY = -(m_minLightSection - 1); // 最低段Y=0
    m_chunkOffsetZ = 2 - (centerZ >> world::CHUNK_SHIFT);

    // chunk index = x + (5 * z)
    m_chunkIndexOffset = m_chunkOffsetX + (5 * m_chunkOffsetZ);

    // chunk section index = x + (5 * z) + ((5 * 5) * y)
    m_chunkSectionIndexOffset = m_chunkIndexOffset + ((5 * 5) * m_chunkOffsetY);
}

void StarLightEngine::setupCaches(
    StarLightLightingProvider* lightAccess, i32 centerX, i32 centerY, i32 centerZ, bool relaxed, bool loadTwoRadius)
{
    i32 centerChunkX = centerX >> world::CHUNK_SHIFT;
    i32 centerChunkY = centerY >> world::CHUNK_SHIFT;
    i32 centerChunkZ = centerZ >> world::CHUNK_SHIFT;

    setupEncodeOffset(centerChunkX * world::CHUNK_WIDTH + 7,
        centerChunkY * world::CHUNK_WIDTH + 7,
        centerChunkZ * world::CHUNK_WIDTH + 7);

    i32 radius = loadTwoRadius ? 2 : 1;

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            i32 cx = centerChunkX + dx;
            i32 cz = centerChunkZ + dz;
            bool isTwoRadius = std::max(std::abs(dx), std::abs(dz)) == 2;

            const IChunk* chunk = lightAccess->getChunkForLight(cx, cz);
            if (chunk == nullptr) {
                if (relaxed || isTwoRadius) {
                    continue;
                }
                // 非宽松模式下一倍半径内的区块必须存在
                continue;
            }

            if (!canUseChunk(chunk)) {
                continue;
            }

            setChunkInCache(cx, cz, chunk);
            setEmptinessMapCache(cx, cz, getEmptinessMap(chunk));

            if (!isTwoRadius) {
                // 设置区块段和 Nibble 数组到缓存
                setBlocksForChunkInCache(cx, cz, chunk->getSections());
                setNibblesForChunkInCache(cx, cz, getNibblesOnChunk(chunk));
            }
        }
    }
}

void StarLightEngine::destroyCaches()
{
    m_chunkCache.fill(nullptr);
    m_emptinessMapCache.fill(nullptr);

    if (m_sectionCache != nullptr) {
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_sectionCache[i] = nullptr;
        }
    }

    if (m_nibbleCache != nullptr) {
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_nibbleCache[i] = nullptr;
        }
    }

    if (m_notifyUpdateCache != nullptr) {
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_notifyUpdateCache[i] = false;
        }
    }
}

void StarLightEngine::updateVisible(StarLightLightingProvider* lightAccess)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "StarLightEngine::updateVisible", "isSkyLight", m_isSkyLight);

    for (i32 index = 0, max = m_sectionCacheSize; index < max; ++index) {
        SWMRNibbleArray* nibble = m_nibbleCache[index];
        if (nibble == nullptr) {
            continue;
        }

        bool shouldNotify = m_notifyUpdateCache != nullptr && m_notifyUpdateCache[index];
        if (!shouldNotify && !nibble->isDirty()) {
            continue;
        }

        i32 chunkX = (index % 5) - m_chunkOffsetX;
        i32 chunkZ = ((index / 5) % 5) - m_chunkOffsetZ;
        i32 ySections = m_maxSection - m_minSection + 1;
        i32 chunkY = ((index / (5 * 5)) % (ySections + 2 + 2)) - m_chunkOffsetY;

        // 与 Moonrise 一致：只调用一次 updateVisible()，避免多次调用导致状态变化
        bool updated = nibble->updateVisible();
        if (updated || shouldNotify) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
                "InvokeMarkLightChangedCallback",
                "chunkX",
                chunkX,
                "chunkY",
                chunkY,
                "chunkZ",
                chunkZ,
                "updated",
                updated,
                "shouldNotify",
                shouldNotify);

            lightAccess->markLightChanged(
                m_isSkyLight ? LightType::SKY : LightType::BLOCK, SectionPos(chunkX, chunkY, chunkZ));
        }
    }
}

// ============================================================================
// 区块光照操作
// ============================================================================

void StarLightEngine::blocksChangedInChunk(StarLightLightingProvider* lightAccess,
    i32 chunkX,
    i32 chunkZ,
    const std::vector<BlockPos>& positions,
    const std::vector<bool>& changedSections)
{
    setupCaches(lightAccess, chunkX * world::CHUNK_WIDTH + 7, 128, chunkZ * world::CHUNK_WIDTH + 7, true, true);

    const IChunk* chunk = getChunkInCache(chunkX, chunkZ);
    if (chunk == nullptr) {
        destroyCaches();
        return;
    }

    // 处理空区块段变化
    if (!changedSections.empty()) {
        std::vector<bool> emptinessChanges(changedSections.size());
        for (size_t i = 0; i < changedSections.size(); ++i) {
            emptinessChanges[i] = changedSections[i];
        }
        std::vector<bool> result = handleEmptySectionChanges(lightAccess, chunk, emptinessChanges, false);
        if (!result.empty()) {
            auto rawMap = _copyEmptinessMap(result, static_cast<size_t>(m_maxLightSection - m_minLightSection + 1));
            setEmptinessMap(chunk, rawMap.get());
        }
    }

    // 传播方块变化
    if (!positions.empty()) {
        propagateBlockChanges(lightAccess, chunk, positions);
    }

    updateVisible(lightAccess);
    destroyCaches();
}

std::vector<bool> StarLightEngine::handleEmptySectionChanges(StarLightLightingProvider* lightAccess,
    const IChunk* chunk,
    const std::vector<bool>& emptinessChanges,
    bool isUnlit)
{
    ChunkPos chunkPos = chunk->pos();
    i32 chunkX = chunkPos.x;
    i32 chunkZ = chunkPos.z;

    std::vector<bool> chunkEmptinessMap;
    const bool* existingMap = getEmptinessMap(chunkX, chunkZ);
    bool needsInit = isUnlit || existingMap == nullptr;

    if (needsInit) {
        chunkEmptinessMap.resize(static_cast<size_t>(m_maxSection - m_minSection + 1), false);
    }

    // 更新空映射
    for (i32 sectionIndex = static_cast<i32>(emptinessChanges.size()) - 1; sectionIndex >= 0; --sectionIndex) {
        bool isEmpty = emptinessChanges[static_cast<size_t>(sectionIndex)];
        i32 sectionY = sectionIndex + m_minSection;

        if (needsInit) {
            chunkEmptinessMap[static_cast<size_t>(sectionIndex)] = isEmpty;
        }

        if (isEmpty) {
            continue;
        }

        // 非空区块段：初始化邻居 Nibble 数组
        for (i32 dz = -1; dz <= 1; ++dz) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                bool extrude = (dx | dz) != 0 || !isUnlit;
                for (i32 dy = 1; dy >= -1; --dy) {
                    initNibble(dx + chunk->x(), dy + sectionY, dz + chunk->z(), extrude, false);
                }
            }
        }
    }

    // 检查是否需要反初始化或延迟初始化
    for (i32 dz = -1; dz <= 1; ++dz) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            // 检查邻居是否都有空映射
            bool neighboursLoaded = true;
            for (i32 dz2 = -1; dz2 <= 1 && neighboursLoaded; ++dz2) {
                for (i32 dx2 = -1; dx2 <= 1 && neighboursLoaded; ++dx2) {
                    if (getEmptinessMap(dx + dx2 + chunk->x(), dz + dz2 + chunk->z()) == nullptr) {
                        neighboursLoaded = false;
                    }
                }
            }

            for (i32 sectionY = m_maxLightSection; sectionY >= m_minLightSection; --sectionY) {
                bool allEmpty = true;
                for (i32 dy2 = -1; dy2 <= 1 && allEmpty; ++dy2) {
                    for (i32 dz2 = -1; dz2 <= 1 && allEmpty; ++dz2) {
                        for (i32 dx2 = -1; dx2 <= 1 && allEmpty; ++dx2) {
                            i32 y = sectionY + dy2;
                            if (y < m_minSection || y > m_maxSection) {
                                continue;
                            }
                            const bool* emptinessMap = getEmptinessMap(dx + dx2 + chunk->x(), dz + dz2 + chunk->z());
                            if (emptinessMap != nullptr) {
                                if (!emptinessMap[static_cast<size_t>(y - m_minSection)]) {
                                    allEmpty = false;
                                }
                            } else {
                                // 没有空映射，需要检查区块段
                                const ChunkSection* section =
                                    getChunkSection(dx + dx2 + chunk->x(), y, dz + dz2 + chunk->z());
                                if (section != nullptr && !section->isEmpty()) {
                                    allEmpty = false;
                                }
                            }
                        }
                    }
                }

                if (allEmpty && neighboursLoaded) {
                    // 可以反初始化
                    setNibbleNull(dx + chunk->x(), sectionY, dz + chunk->z());
                } else if (!allEmpty) {
                    // 必须初始化
                    bool extrude = (dx | dz) != 0 || !isUnlit;
                    initNibble(dx + chunk->x(), sectionY, dz + chunk->z(), extrude, false);
                }
            }
        }
    }

    return needsInit ? chunkEmptinessMap : std::vector<bool>{};
}

void StarLightEngine::forceHandleEmptySectionChanges(
    StarLightLightingProvider* lightAccess, const IChunk* chunk, const std::vector<bool>& emptySections)
{
    // 参考 Moonrise StarLightEngine.forceHandleEmptySectionChanges
    // 用于已正确光照的区块，强制加载光照数据到缓存并处理空段变化
    ChunkPos chunkPos = chunk->pos();
    i32 chunkX = chunkPos.x;
    i32 chunkZ = chunkPos.z;

    setupCaches(lightAccess, chunkX * world::CHUNK_WIDTH + 7, 128, chunkZ * world::CHUNK_WIDTH + 7, true, true);

    // 强制将当前区块加载到缓存
    setChunkInCache(chunkX, chunkZ, chunk);
    setBlocksForChunkInCache(chunkX, chunkZ, chunk->getSections());
    setNibblesForChunkInCache(chunkX, chunkZ, getNibblesOnChunk(chunk));
    setEmptinessMapCache(chunkX, chunkZ, getEmptinessMap(chunk));

    // 处理空段变化，但不使用 unlit 模式
    std::vector<bool> result = handleEmptySectionChanges(lightAccess, chunk, emptySections, false);
    if (!result.empty()) {
        auto rawMap = _copyEmptinessMap(result, static_cast<size_t>(m_maxLightSection - m_minLightSection + 1));
        setEmptinessMap(chunk, rawMap.get());
    }

    updateVisible(lightAccess);
    destroyCaches();
}

void StarLightEngine::checkChunkEdges(StarLightLightingProvider* lightAccess, i32 chunkX, i32 chunkZ)
{
    setupCaches(lightAccess, chunkX * world::CHUNK_WIDTH + 7, 128, chunkZ * world::CHUNK_WIDTH + 7, true, false);

    const IChunk* chunk = getChunkInCache(chunkX, chunkZ);
    if (chunk != nullptr) {
        checkChunkEdges(lightAccess, chunk, m_minLightSection, m_maxLightSection);
        updateVisible(lightAccess);
    }

    destroyCaches();
}

void StarLightEngine::checkChunkEdges(
    StarLightLightingProvider* lightAccess, const IChunk* chunk, i32 fromSection, i32 toSection)
{
    ChunkPos chunkPos = chunk->pos();
    i32 chunkX = chunkPos.x;
    i32 chunkZ = chunkPos.z;

    for (i32 sectionY = toSection; sectionY >= fromSection; --sectionY) {
        checkChunkEdge(lightAccess, chunk, chunkX, sectionY, chunkZ);
    }

    performLightDecrease(lightAccess);
}

void StarLightEngine::checkChunkEdge(
    StarLightLightingProvider* lightAccess, const IChunk* chunk, i32 chunkX, i32 chunkY, i32 chunkZ)
{
    SWMRNibbleArray* currNibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (currNibble == nullptr) {
        return;
    }

    for (LightAxisDirection dir : ONLY_HORIZONTAL_DIRECTIONS) {
        i32 dx, dy, dz;
        _getDirectionOffset(dir, dx, dy, dz);

        SWMRNibbleArray* neighbourNibble = getNibbleFromCache(chunkX + dx, chunkY, chunkZ + dz);
        if (neighbourNibble == nullptr) {
            continue;
        }

        if (!currNibble->isInitializedUpdating() && !neighbourNibble->isInitializedUpdating()) {
            // 两者都是零，无需检查
            continue;
        }

        // 当前区块
        i32 incX, incZ, startX, startZ;
        if (dx != 0) {
            // X 方向
            incX = 0;
            incZ = 1;
            if (dx < 0) {
                // 负方向
                startX = chunkX << world::CHUNK_SHIFT;
            } else {
                startX = (chunkX << world::CHUNK_SHIFT) | world::CHUNK_MASK;
            }
            startZ = chunkZ << world::CHUNK_SHIFT;
        } else {
            // Z 方向
            incX = 1;
            incZ = 0;
            if (dz < 0) {
                // 负方向
                startZ = chunkZ << world::CHUNK_SHIFT;
            } else {
                startZ = (chunkZ << world::CHUNK_SHIFT) | world::CHUNK_MASK;
            }
            startX = chunkX << world::CHUNK_SHIFT;
        }

        i32 centerDelayedChecks = 0;
        i32 neighbourDelayedChecks = 0;
        for (i32 currY = chunkY << world::CHUNK_SHIFT, maxY = currY | world::CHUNK_MASK; currY <= maxY; ++currY) {
            for (i32 i = 0, currX = startX, currZ = startZ; i < world::CHUNK_WIDTH; ++i, currX += incX, currZ += incZ) {
                i32 neighbourX = currX + dx;
                i32 neighbourZ = currZ + dz;

                i32 currentIndex = (currX & world::CHUNK_MASK) | ((currZ & world::CHUNK_MASK) << world::CHUNK_SHIFT) |
                    ((currY & world::CHUNK_MASK) << 8);
                i32 currentLevel = currNibble->getUpdating(currentIndex);

                i32 neighbourIndex = (neighbourX & world::CHUNK_MASK) |
                    ((neighbourZ & world::CHUNK_MASK) << world::CHUNK_SHIFT) | ((currY & world::CHUNK_MASK) << 8);
                i32 neighbourLevel = neighbourNibble->getUpdating(neighbourIndex);

                // 延迟检查，因为 checkBlock 方法会覆盖光照值，这会影响后续的 calculateLightValue 操作
                // 虽然这种行为上不显著，但会因排队更多值而影响性能

                if (calculateLightValue(lightAccess, currX, currY, currZ, currentLevel) != currentLevel) {
                    m_chunkCheckDelayedUpdatesCenter[static_cast<size_t>(centerDelayedChecks++)] = currentIndex;
                }

                if (calculateLightValue(lightAccess, neighbourX, currY, neighbourZ, neighbourLevel) != neighbourLevel) {
                    m_chunkCheckDelayedUpdatesNeighbour[static_cast<size_t>(neighbourDelayedChecks++)] = neighbourIndex;
                }
            }
        }

        i32 currentChunkOffX = chunkX << world::CHUNK_SHIFT;
        i32 currentChunkOffZ = chunkZ << world::CHUNK_SHIFT;
        i32 neighbourChunkOffX = (chunkX + dx) << world::CHUNK_SHIFT;
        i32 neighbourChunkOffZ = (chunkZ + dz) << world::CHUNK_SHIFT;
        i32 chunkOffY = chunkY << world::CHUNK_SHIFT;
        for (i32 i = 0, len = std::max(centerDelayedChecks, neighbourDelayedChecks); i < len; ++i) {
            // 尝试将邻居数据一起排队
            // index = x | (z << CHUNK_SHIFT) | (y << 8)
            if (i < centerDelayedChecks) {
                i32 value = m_chunkCheckDelayedUpdatesCenter[static_cast<size_t>(i)];
                checkBlock(lightAccess,
                    currentChunkOffX | (value & world::CHUNK_MASK),
                    chunkOffY | (value >> 8),
                    currentChunkOffZ | ((value >> 4) & 0xF));
            }
            if (i < neighbourDelayedChecks) {
                i32 value = m_chunkCheckDelayedUpdatesNeighbour[static_cast<size_t>(i)];
                checkBlock(lightAccess,
                    neighbourChunkOffX | (value & world::CHUNK_MASK),
                    chunkOffY | (value >> 8),
                    neighbourChunkOffZ | ((value >> 4) & 0xF));
            }
        }
    }
}

void StarLightEngine::propagateNeighbourLevels(
    StarLightLightingProvider* lightAccess, const IChunk* chunk, i32 fromSection, i32 toSection)
{
    ChunkPos chunkPos = chunk->pos();
    i32 chunkX = chunkPos.x;
    i32 chunkZ = chunkPos.z;
    i32 encodeOffset = m_coordinateOffset;

    for (i32 currSectionY = toSection; currSectionY >= fromSection; --currSectionY) {
        SWMRNibbleArray* currNibble = getNibbleFromCache(chunkX, currSectionY, chunkZ);
        if (currNibble == nullptr) {
            continue;
        }
        for (LightAxisDirection dir : ONLY_HORIZONTAL_DIRECTIONS) {
            i32 dx, dy, dz;
            _getDirectionOffset(dir, dx, dy, dz);

            SWMRNibbleArray* neighbourNibble = getNibbleFromCache(chunkX + dx, currSectionY, chunkZ + dz);
            if (neighbourNibble == nullptr || !neighbourNibble->isInitializedUpdating()) {
                // 无法从 0 拉取
                continue;
            }

            // 邻居区块
            i32 incX, incZ, startX, startZ;
            if (dx != 0) {
                // X 方向
                incX = 0;
                incZ = 1;
                if (dx < 0) {
                    // 负方向
                    startX = (chunkX << world::CHUNK_SHIFT) - 1;
                } else {
                    startX = (chunkX << world::CHUNK_SHIFT) + world::CHUNK_WIDTH;
                }
                startZ = chunkZ << world::CHUNK_SHIFT;
            } else {
                // Z 方向
                incX = 1;
                incZ = 0;
                if (dz < 0) {
                    // 负方向
                    startZ = (chunkZ << world::CHUNK_SHIFT) - 1;
                } else {
                    startZ = (chunkZ << world::CHUNK_SHIFT) + world::CHUNK_WIDTH;
                }
                startX = chunkX << world::CHUNK_SHIFT;
            }

            i32 propagateDirection =
                getDirectionBitset(_getOppositeDirection(dir)); // 只想在这个方向检查向这个区块的传播
            i32 sectionOffset = m_chunkSectionIndexOffset;

            for (i32 currY = currSectionY << world::CHUNK_SHIFT, maxY = currY | world::CHUNK_MASK; currY <= maxY;
                ++currY) {
                for (i32 i = 0, currX = startX, currZ = startZ; i < world::CHUNK_WIDTH;
                    ++i, currX += incX, currZ += incZ) {
                    i32 level = neighbourNibble->getUpdating((currX & world::CHUNK_MASK) |
                        ((currZ & world::CHUNK_MASK) << world::CHUNK_SHIFT) | ((currY & world::CHUNK_MASK) << 8));

                    if (level <= 1) {
                        // 无需传播
                        continue;
                    }

                    appendToIncreaseQueue(((currX + (currZ << 6) + (currY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(level & 0xF) << 28) | (static_cast<u64>(propagateDirection) << 32) |
                        FLAG_HAS_SIDED_TRANSPARENT_BLOCKS // 不知道当前方块是否透明，必须检查
                    );
                }
            }
        }
    }
}

void StarLightEngine::lightChunk(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks)
{
    // 子类实现
}

void StarLightEngine::light(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks)
{
    if (chunk == nullptr) {
        return;
    }

    i32 chunkX = chunk->x();
    i32 chunkZ = chunk->z();
    i32 centerX = chunkX * world::CHUNK_WIDTH + 7;
    i32 centerY = 128; // 中间高度
    i32 centerZ = chunkZ * world::CHUNK_WIDTH + 7;

    // 重置队列状态（确保多次调用不会累积队列条目）
    m_increaseQueueInitialLength = 0;
    m_decreaseQueueInitialLength = 0;
    m_needsUpdate = false;

    // 初始化缓存
    setupCaches(lightAccess, centerX, centerY, centerZ, true, true);

    try {
        const i32 totalLightSections = m_maxLightSection - m_minLightSection + 1;

        // 与 Moonrise 一致：light() 使用一组"全 NULL 状态"临时 nibble 作为点亮输入。
        std::vector<SWMRNibbleArray> tempNibbles(static_cast<size_t>(totalLightSections));
        std::vector<SWMRNibbleArray*> tempNibblePtrs(static_cast<size_t>(totalLightSections), nullptr);
        for (i32 i = 0; i < totalLightSections; ++i) {
            tempNibbles[static_cast<size_t>(i)] = SWMRNibbleArray(nullptr, true);
            tempNibblePtrs[static_cast<size_t>(i)] = &tempNibbles[static_cast<size_t>(i)];
        }

        // 设置当前区块到缓存
        setChunkInCache(chunkX, chunkZ, chunk);

        // 设置区块段到缓存
        const ChunkSection* const* sections = chunk->getSections();
        if (sections != nullptr) {
            setBlocksForChunkInCache(chunkX, chunkZ, sections);
        }

        // 将临时 nibble 写入缓存（而不是直接读取 chunk 上现有 nibble）
        setNibblesForChunkInCache(chunkX, chunkZ, tempNibblePtrs.data());

        // 与 Moonrise 一致：unlit 点亮流程先按"无空映射"处理，避免旧缓存干扰 initNibble。
        setEmptinessMapCache(chunkX, chunkZ, nullptr);

        // 计算 emptySections，并在未点亮模式下先跑一次空段处理（Moonrise light() 同构流程）
        std::vector<bool> emptySections(static_cast<size_t>(m_maxSection - m_minSection + 1), true);
        for (i32 sectionY = m_minSection; sectionY <= m_maxSection; ++sectionY) {
            const ChunkSection* section =
                (sections == nullptr) ? nullptr : sections[static_cast<size_t>(sectionY - m_minSection)];
            emptySections[static_cast<size_t>(sectionY - m_minSection)] = (section == nullptr || section->isEmpty());
        }

        std::vector<bool> emptinessUpdate = handleEmptySectionChanges(lightAccess, chunk, emptySections, true);
        if (!emptinessUpdate.empty()) {
            auto rawMap =
                _copyEmptinessMap(emptinessUpdate, static_cast<size_t>(m_maxLightSection - m_minLightSection + 1));
            setEmptinessMap(chunk, rawMap.get());
            setEmptinessMapCache(chunkX, chunkZ, getEmptinessMap(chunk));
        }

        // 执行光照计算
        lightChunk(lightAccess, chunk, needsEdgeChecks);

        // 将临时 nibble 写回区块（move 语义：临时对象状态转移到区块 nibble，临时对象置 Null）
        setNibbles(chunk, tempNibblePtrs.data());

        // 重新把缓存指向区块上的 nibble（move 后区块 nibble 与临时对象是不同实例）。
        // 对齐 Moonrise light() 顺序：setNibbles 后 updateVisible 必须作用在区块 nibble 上，
        // 因 markLightChanged→getData→toByteArray 读区块 nibble 的 visible 侧，
        // 若 updateVisible 仍作用已 move 置空的临时对象则区块 nibble 永不发布 visible（既有 bug）。
        setNibblesForChunkInCache(chunkX, chunkZ, getNibblesOnChunk(chunk));

        // 更新可见数据：发布区块 nibble 的 visible 侧，并经 markLightChanged 通知主线程
        updateVisible(lightAccess);
    }
    catch (...) {
        destroyCaches();
        throw;
    }

    destroyCaches();
}

// ============================================================================
// 缓存访问方法
// ============================================================================

const IChunk* StarLightEngine::getChunkInCache(i32 chunkX, i32 chunkZ) const
{
    i32 dx = chunkX + m_chunkOffsetX;
    i32 dz = chunkZ + m_chunkOffsetZ;
    if (dx < 0 || dx >= 5 || dz < 0 || dz >= 5) {
        return nullptr;
    }
    return m_chunkCache[static_cast<size_t>(dx + dz * 5)];
}

void StarLightEngine::setChunkInCache(i32 chunkX, i32 chunkZ, const IChunk* chunk)
{
    i32 dx = chunkX + m_chunkOffsetX;
    i32 dz = chunkZ + m_chunkOffsetZ;
    if (dx >= 0 && dx < 5 && dz >= 0 && dz < 5) {
        m_chunkCache[static_cast<size_t>(dx + dz * 5)] = chunk;
    }
}

const ChunkSection* StarLightEngine::getChunkSection(i32 chunkX, i32 chunkY, i32 chunkZ) const
{
    if (m_sectionCache == nullptr) {
        return nullptr;
    }
    i32 index = chunkX + 5 * chunkZ + (5 * 5) * chunkY + m_chunkSectionIndexOffset;
    if (index < 0 || index >= m_sectionCacheSize) {
        return nullptr;
    }
    return static_cast<const ChunkSection*>(m_sectionCache[index]);
}

void StarLightEngine::setChunkSectionInCache(i32 chunkX, i32 chunkY, i32 chunkZ, const ChunkSection* section)
{
    if (m_sectionCache == nullptr) {
        return;
    }
    i32 index = chunkX + 5 * chunkZ + (5 * 5) * chunkY + m_chunkSectionIndexOffset;
    if (index >= 0 && index < m_sectionCacheSize) {
        m_sectionCache[index] = section;
    }
}

void StarLightEngine::setBlocksForChunkInCache(i32 chunkX, i32 chunkZ, const ChunkSection* const* sections)
{
    for (i32 cy = m_minLightSection; cy <= m_maxLightSection; ++cy) {
        setChunkSectionInCache(chunkX,
            cy,
            chunkZ,
            sections == nullptr ? nullptr
                                : (cy >= m_minSection && cy <= m_maxSection ? sections[cy - m_minSection] : nullptr));
    }
}

SWMRNibbleArray* StarLightEngine::getNibbleFromCache(i32 chunkX, i32 chunkY, i32 chunkZ) const
{
    if (m_nibbleCache == nullptr) {
        return nullptr;
    }
    i32 index = chunkX + 5 * chunkZ + (5 * 5) * chunkY + m_chunkSectionIndexOffset;
    if (index < 0 || index >= m_sectionCacheSize) {
        return nullptr;
    }
    return m_nibbleCache[index];
}

void StarLightEngine::setNibbleInCache(i32 chunkX, i32 chunkY, i32 chunkZ, SWMRNibbleArray* nibble)
{
    if (m_nibbleCache == nullptr) {
        return;
    }
    i32 index = chunkX + 5 * chunkZ + (5 * 5) * chunkY + m_chunkSectionIndexOffset;
    if (index >= 0 && index < m_sectionCacheSize) {
        m_nibbleCache[index] = nibble;
    }
}

void StarLightEngine::setNibblesForChunkInCache(i32 chunkX, i32 chunkZ, SWMRNibbleArray* const* nibbles)
{
    for (i32 cy = m_minLightSection; cy <= m_maxLightSection; ++cy) {
        setNibbleInCache(chunkX, cy, chunkZ, nibbles == nullptr ? nullptr : nibbles[cy - m_minLightSection]);
    }
}

const bool* StarLightEngine::getEmptinessMap(i32 chunkX, i32 chunkZ) const
{
    i32 dx = chunkX + m_chunkOffsetX;
    i32 dz = chunkZ + m_chunkOffsetZ;
    if (dx < 0 || dx >= 5 || dz < 0 || dz >= 5) {
        return nullptr;
    }
    return m_emptinessMapCache[static_cast<size_t>(dx + dz * 5)];
}

void StarLightEngine::setEmptinessMapCache(i32 chunkX, i32 chunkZ, const bool* map)
{
    i32 dx = chunkX + m_chunkOffsetX;
    i32 dz = chunkZ + m_chunkOffsetZ;
    if (dx >= 0 && dx < 5 && dz >= 0 && dz < 5) {
        m_emptinessMapCache[static_cast<size_t>(dx + dz * 5)] = map;
    }
}

const BlockState* StarLightEngine::getBlockState(i32 worldX, i32 worldY, i32 worldZ) const
{
    const ChunkSection* section =
        getChunkSection(worldX >> world::CHUNK_SHIFT, worldY >> world::CHUNK_SHIFT, worldZ >> world::CHUNK_SHIFT);
    if (section == nullptr) {
        return nullptr; // 空气
    }

    // 通过区块获取方块状态
    const IChunk* chunk = getChunkInCache(worldX >> world::CHUNK_SHIFT, worldZ >> world::CHUNK_SHIFT);
    if (chunk == nullptr) {
        return nullptr;
    }

    return chunk->getBlockState(worldX & world::CHUNK_MASK, worldY, worldZ & world::CHUNK_MASK);
}

const BlockState* StarLightEngine::getBlockState(i32 sectionIndex, i32 localIndex) const
{
    if (sectionIndex < 0 || sectionIndex >= m_sectionCacheSize || m_sectionCache == nullptr) {
        return nullptr;
    }

    const ChunkSection* section = static_cast<const ChunkSection*>(m_sectionCache[sectionIndex]);
    if (section == nullptr) {
        return nullptr;
    }

    // localIndex = x | (z << CHUNK_SHIFT) | (y << 8)
    i32 x = localIndex & world::CHUNK_MASK;
    i32 z = (localIndex >> world::CHUNK_SHIFT) & world::CHUNK_MASK;
    i32 y = (localIndex >> 8) & world::CHUNK_MASK;

    return section->getBlockState(x, y, z);
}

i32 StarLightEngine::getLightLevel(i32 worldX, i32 worldY, i32 worldZ) const
{
    SWMRNibbleArray* nibble =
        getNibbleFromCache(worldX >> world::CHUNK_SHIFT, worldY >> world::CHUNK_SHIFT, worldZ >> world::CHUNK_SHIFT);
    if (nibble == nullptr) {
        return m_isSkyLight ? 15 : 0; // 天空光照默认15，方块光照默认0
    }
    return nibble->getUpdating(worldX & world::CHUNK_MASK, worldY & world::CHUNK_MASK, worldZ & world::CHUNK_MASK);
}

i32 StarLightEngine::getLightLevel(i32 sectionIndex, i32 localIndex) const
{
    if (sectionIndex < 0 || sectionIndex >= m_sectionCacheSize || m_nibbleCache == nullptr) {
        return m_isSkyLight ? 15 : 0;
    }
    SWMRNibbleArray* nibble = m_nibbleCache[sectionIndex];
    if (nibble == nullptr) {
        return m_isSkyLight ? 15 : 0;
    }
    return nibble->getUpdating(localIndex);
}

void StarLightEngine::setLightLevel(i32 worldX, i32 worldY, i32 worldZ, i32 level)
{
    i32 sectionIndex = (worldX >> world::CHUNK_SHIFT) + 5 * (worldZ >> world::CHUNK_SHIFT) +
        (5 * 5) * (worldY >> world::CHUNK_SHIFT) + m_chunkSectionIndexOffset;
    SWMRNibbleArray* nibble = m_nibbleCache[sectionIndex];

    if (nibble != nullptr) {
        nibble->set(
            worldX & world::CHUNK_MASK, worldY & world::CHUNK_MASK, worldZ & world::CHUNK_MASK, static_cast<u8>(level));

        // 客户端需要通知相邻区块段
        if (m_notifyUpdateCache != nullptr && m_isClientSide) {
            i32 cx1 = (worldX - 1) >> world::CHUNK_SHIFT;
            i32 cx2 = (worldX + 1) >> world::CHUNK_SHIFT;
            i32 cy1 = (worldY - 1) >> world::CHUNK_SHIFT;
            i32 cy2 = (worldY + 1) >> world::CHUNK_SHIFT;
            i32 cz1 = (worldZ - 1) >> world::CHUNK_SHIFT;
            i32 cz2 = (worldZ + 1) >> world::CHUNK_SHIFT;
            for (i32 x = cx1; x <= cx2; ++x) {
                for (i32 y = cy1; y <= cy2; ++y) {
                    for (i32 z = cz1; z <= cz2; ++z) {
                        i32 idx = x + 5 * z + (5 * 5) * y + m_chunkSectionIndexOffset;
                        if (idx >= 0 && idx < m_sectionCacheSize) {
                            m_notifyUpdateCache[idx] = true;
                        }
                    }
                }
            }
        }
    }
}

void StarLightEngine::setLightLevel(i32 sectionIndex, i32 localIndex, i32 worldX, i32 worldY, i32 worldZ, i32 level)
{
    if (sectionIndex < 0 || sectionIndex >= m_sectionCacheSize || m_nibbleCache == nullptr) {
        return;
    }
    SWMRNibbleArray* nibble = m_nibbleCache[sectionIndex];
    if (nibble != nullptr) {
        nibble->set(localIndex, static_cast<u8>(level));

        if (m_notifyUpdateCache != nullptr && m_isClientSide) {
            i32 cx1 = (worldX - 1) >> world::CHUNK_SHIFT;
            i32 cx2 = (worldX + 1) >> world::CHUNK_SHIFT;
            i32 cy1 = (worldY - 1) >> world::CHUNK_SHIFT;
            i32 cy2 = (worldY + 1) >> world::CHUNK_SHIFT;
            i32 cz1 = (worldZ - 1) >> world::CHUNK_SHIFT;
            i32 cz2 = (worldZ + 1) >> world::CHUNK_SHIFT;
            for (i32 x = cx1; x <= cx2; ++x) {
                for (i32 y = cy1; y <= cy2; ++y) {
                    for (i32 z = cz1; z <= cz2; ++z) {
                        i32 idx = x + 5 * z + (5 * 5) * y + m_chunkSectionIndexOffset;
                        if (idx >= 0 && idx < m_sectionCacheSize) {
                            m_notifyUpdateCache[idx] = true;
                        }
                    }
                }
            }
        }
    }
}

void StarLightEngine::postLightUpdate(i32 worldX, i32 worldY, i32 worldZ)
{
    if (m_notifyUpdateCache == nullptr || !m_isClientSide) {
        return;
    }

    i32 cx1 = (worldX - 1) >> world::CHUNK_SHIFT;
    i32 cx2 = (worldX + 1) >> world::CHUNK_SHIFT;
    i32 cy1 = (worldY - 1) >> world::CHUNK_SHIFT;
    i32 cy2 = (worldY + 1) >> world::CHUNK_SHIFT;
    i32 cz1 = (worldZ - 1) >> world::CHUNK_SHIFT;
    i32 cz2 = (worldZ + 1) >> world::CHUNK_SHIFT;

    for (i32 x = cx1; x <= cx2; ++x) {
        for (i32 y = cy1; y <= cy2; ++y) {
            for (i32 z = cz1; z <= cz2; ++z) {
                i32 idx = x + 5 * z + (5 * 5) * y + m_chunkSectionIndexOffset;
                if (idx >= 0 && idx < m_sectionCacheSize) {
                    m_notifyUpdateCache[idx] = true;
                }
            }
        }
    }
}

// ============================================================================
// 队列操作
// ============================================================================

void StarLightEngine::appendToIncreaseQueue(u64 queueValue)
{
    if (m_increaseQueueInitialLength >= static_cast<i32>(m_increaseQueue.size())) {
        resizeIncreaseQueue();
    }
    m_increaseQueue[static_cast<size_t>(m_increaseQueueInitialLength++)] = queueValue;
    m_needsUpdate = true;
}

void StarLightEngine::appendToDecreaseQueue(u64 queueValue)
{
    if (m_decreaseQueueInitialLength >= static_cast<i32>(m_decreaseQueue.size())) {
        resizeDecreaseQueue();
    }
    m_decreaseQueue[static_cast<size_t>(m_decreaseQueueInitialLength++)] = queueValue;
    m_needsUpdate = true;
}

void StarLightEngine::resizeIncreaseQueue()
{
    size_t newSize = std::max(static_cast<size_t>(4), m_increaseQueue.size() + (m_increaseQueue.size() >> 1));
    m_increaseQueue.resize(newSize);
}

void StarLightEngine::resizeDecreaseQueue()
{
    size_t newSize = std::max(static_cast<size_t>(4), m_decreaseQueue.size() + (m_decreaseQueue.size() >> 1));
    m_decreaseQueue.resize(newSize);
}

i32 StarLightEngine::performUpdates(StarLightLightingProvider* lightAccess, i32 maxUpdates)
{
    // 先处理减亮队列
    if (m_decreaseQueueInitialLength > 0) {
        performLightDecrease(lightAccess);
    }

    // 再处理增亮队列
    if (m_increaseQueueInitialLength > 0) {
        performLightIncrease(lightAccess);
    }

    m_needsUpdate = false;
    return maxUpdates;
}

// ============================================================================
// 光照传播（基础实现，子类可扩展）
// ============================================================================

void StarLightEngine::performLightIncrease(StarLightLightingProvider* lightAccess)
{
    // 参考 Moonrise StarLightEngine.performLightIncrease
    i32 decodeOffsetX = -m_encodeOffsetX;
    i32 decodeOffsetY = -m_encodeOffsetY;
    i32 decodeOffsetZ = -m_encodeOffsetZ;
    i32 encodeOffset = m_coordinateOffset;
    i32 sectionOffset = m_chunkSectionIndexOffset;

    i32 queueReadIndex = 0;
    i32 queueLength = m_increaseQueueInitialLength;
    m_increaseQueueInitialLength = 0;

    while (queueReadIndex < queueLength) {
        u64 queueValue = m_increaseQueue[static_cast<size_t>(queueReadIndex++)];

        // 解码队列条目
        i32 posX = (static_cast<i32>(queueValue) & 63) + decodeOffsetX;
        i32 posZ = ((static_cast<i32>(queueValue) >> 6) & 63) + decodeOffsetZ;
        i32 posY = ((static_cast<i32>(queueValue) >> 12) & 0xFFFF) + decodeOffsetY;
        i32 propagatedLevel = static_cast<i32>((queueValue >> 28) & 0xF);
        i32 directionBits = static_cast<i32>((queueValue >> 32) & 0x3F);

        // 检查重检标志
        if ((queueValue & FLAG_RECHECK_LEVEL) != 0) {
            if (getLightLevel(posX, posY, posZ) != propagatedLevel) {
                // not at the level we expect, so something changed.
                continue;
            }
        } else if ((queueValue & FLAG_WRITE_LEVEL) != 0) {
            // these are used to restore block sources after a propagation decrease
            setLightLevel(posX, posY, posZ, propagatedLevel);
        }

        // 根据 FLAG_HAS_SIDED_TRANSPARENT_BLOCKS 标志选择处理路径
        if ((queueValue & FLAG_HAS_SIDED_TRANSPARENT_BLOCKS) == 0) {
            // we don't need to worry about our state here.
            const std::vector<LightAxisDirection>& directions =
                s_oldCheckDirections[static_cast<size_t>(directionBits)];
            for (LightAxisDirection propagate : directions) {
                i32 dx, dy, dz;
                _getDirectionOffset(propagate, dx, dy, dz);

                i32 offX = posX + dx;
                i32 offY = posY + dy;
                i32 offZ = posZ + dz;

                i32 sectionIndex = (offX >> world::CHUNK_SHIFT) + 5 * (offZ >> world::CHUNK_SHIFT) +
                    (5 * 5) * (offY >> world::CHUNK_SHIFT) + sectionOffset;
                i32 localIndex = (offX & world::CHUNK_MASK) | ((offZ & world::CHUNK_MASK) << world::CHUNK_SHIFT) |
                    ((offY & world::CHUNK_MASK) << 8);

                SWMRNibbleArray* currentNibble = m_nibbleCache[sectionIndex];
                i32 currentLevel;
                if (currentNibble == nullptr ||
                    (currentLevel = currentNibble->getUpdating(localIndex)) >= (propagatedLevel - 1)) {
                    continue; // already at the level we want or unloaded
                }

                const BlockState* blockState = getBlockState(sectionIndex, localIndex);
                if (blockState == nullptr) {
                    continue;
                }

                u64 flags = 0;
                if (blockState->useShapeForLightOcclusion()) {
                    // 获取遮挡面
                    CollisionShape cullingFace =
                        blockState->getFaceOcclusionShape(_getNMSDirection(_getOppositeDirection(propagate)));
                    if (cullingFace.isFullBlock()) {
                        // 完全面遮挡，无法传播
                        continue;
                    }
                    if (!cullingFace.isEmpty()) {
                        // 部分遮挡，需要后续检测
                        flags |= FLAG_HAS_SIDED_TRANSPARENT_BLOCKS;
                    }
                }

                i32 opacity = blockState->getBlock().getOpacity(*blockState);
                i32 targetLevel = propagatedLevel - std::max(1, opacity);
                if (targetLevel <= currentLevel) {
                    continue;
                }

                currentNibble->set(localIndex, static_cast<u8>(targetLevel));
                postLightUpdate(offX, offY, offZ);

                if (targetLevel > 1) {
                    if (queueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        resizeIncreaseQueue();
                    }
                    m_increaseQueue[static_cast<size_t>(queueLength++)] =
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(targetLevel & 0xF) << 28) |
                        (static_cast<u64>(_getEverythingButOppositeDirection(propagate)) << 32) | flags;
                }
            }
        } else {
            // we actually need to worry about our state here
            const BlockState* fromBlock = getBlockState(posX, posY, posZ);
            const std::vector<LightAxisDirection>& directions =
                s_oldCheckDirections[static_cast<size_t>(directionBits)];

            for (LightAxisDirection propagate : directions) {
                i32 dx, dy, dz;
                _getDirectionOffset(propagate, dx, dy, dz);

                i32 offX = posX + dx;
                i32 offY = posY + dy;
                i32 offZ = posZ + dz;

                // 检查源方块的遮挡面
                CollisionShape fromShape; // 空 shape
                if (fromBlock != nullptr && fromBlock->useShapeForLightOcclusion()) {
                    fromShape = fromBlock->getFaceOcclusionShape(_getNMSDirection(propagate));
                }

                if (!fromShape.isEmpty() && fromShape.isFullBlock()) {
                    // 源面完全遮挡，无法传播
                    continue;
                }

                i32 sectionIndex = (offX >> world::CHUNK_SHIFT) + 5 * (offZ >> world::CHUNK_SHIFT) +
                    (5 * 5) * (offY >> world::CHUNK_SHIFT) + sectionOffset;
                i32 localIndex = (offX & world::CHUNK_MASK) | ((offZ & world::CHUNK_MASK) << world::CHUNK_SHIFT) |
                    ((offY & world::CHUNK_MASK) << 8);

                SWMRNibbleArray* currentNibble = m_nibbleCache[sectionIndex];
                i32 currentLevel;
                if (currentNibble == nullptr ||
                    (currentLevel = currentNibble->getUpdating(localIndex)) >= (propagatedLevel - 1)) {
                    continue; // already at the level we want
                }

                const BlockState* blockState = getBlockState(sectionIndex, localIndex);
                if (blockState == nullptr) {
                    continue;
                }

                u64 flags = 0;
                if (blockState->useShapeForLightOcclusion()) {
                    CollisionShape cullingFace =
                        blockState->getFaceOcclusionShape(_getNMSDirection(_getOppositeDirection(propagate)));

                    // 使用 VoxelShape 进行精确遮挡检测
                    VoxelShape fromVoxel = Shapes::fromCollisionShape(fromShape);
                    VoxelShape cullingVoxel = Shapes::fromCollisionShape(cullingFace);

                    if (Shapes::faceShapeOccludes(fromVoxel, cullingVoxel)) {
                        continue;
                    }
                    flags |= FLAG_HAS_SIDED_TRANSPARENT_BLOCKS;
                }

                i32 opacity = blockState->getBlock().getOpacity(*blockState);
                i32 targetLevel = propagatedLevel - std::max(1, opacity);
                if (targetLevel <= currentLevel) {
                    continue;
                }

                currentNibble->set(localIndex, static_cast<u8>(targetLevel));
                postLightUpdate(offX, offY, offZ);

                if (targetLevel > 1) {
                    if (queueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        resizeIncreaseQueue();
                    }
                    m_increaseQueue[static_cast<size_t>(queueLength++)] =
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(targetLevel & 0xF) << 28) |
                        (static_cast<u64>(_getEverythingButOppositeDirection(propagate)) << 32) | flags;
                }
            }
        }
    }
}

void StarLightEngine::performLightDecrease(StarLightLightingProvider* lightAccess)
{
    // 参考 Moonrise StarLightEngine.performLightDecrease
    i32 decodeOffsetX = -m_encodeOffsetX;
    i32 decodeOffsetY = -m_encodeOffsetY;
    i32 decodeOffsetZ = -m_encodeOffsetZ;
    i32 encodeOffset = m_coordinateOffset;
    i32 sectionOffset = m_chunkSectionIndexOffset;
    i32 emittedMask = m_emittedLightMask;

    i32 queueReadIndex = 0;
    i32 queueLength = m_decreaseQueueInitialLength;
    m_decreaseQueueInitialLength = 0;
    i32 increaseQueueLength = m_increaseQueueInitialLength;

    while (queueReadIndex < queueLength) {
        u64 queueValue = m_decreaseQueue[static_cast<size_t>(queueReadIndex++)];

        i32 posX = (static_cast<i32>(queueValue) & 63) + decodeOffsetX;
        i32 posZ = ((static_cast<i32>(queueValue) >> 6) & 63) + decodeOffsetZ;
        i32 posY = ((static_cast<i32>(queueValue) >> 12) & 0xFFFF) + decodeOffsetY;
        i32 propagatedLevel = static_cast<i32>((queueValue >> 28) & 0xF);
        i32 directionBits = static_cast<i32>((queueValue >> 32) & 0x3F);

        // 根据 FLAG_HAS_SIDED_TRANSPARENT_BLOCKS 标志选择处理路径
        if ((queueValue & FLAG_HAS_SIDED_TRANSPARENT_BLOCKS) == 0) {
            // we don't need to worry about our state here.
            const std::vector<LightAxisDirection>& directions =
                s_oldCheckDirections[static_cast<size_t>(directionBits)];
            for (LightAxisDirection propagate : directions) {
                i32 dx, dy, dz;
                _getDirectionOffset(propagate, dx, dy, dz);

                i32 offX = posX + dx;
                i32 offY = posY + dy;
                i32 offZ = posZ + dz;

                i32 sectionIndex = (offX >> world::CHUNK_SHIFT) + 5 * (offZ >> world::CHUNK_SHIFT) +
                    (5 * 5) * (offY >> world::CHUNK_SHIFT) + sectionOffset;
                i32 localIndex = (offX & world::CHUNK_MASK) | ((offZ & world::CHUNK_MASK) << world::CHUNK_SHIFT) |
                    ((offY & world::CHUNK_MASK) << 8);

                SWMRNibbleArray* currentNibble = m_nibbleCache[sectionIndex];
                i32 lightLevel;

                if (currentNibble == nullptr || (lightLevel = currentNibble->getUpdating(localIndex)) == 0) {
                    // already at lowest (or unloaded), nothing we can do
                    continue;
                }

                const BlockState* blockState = getBlockState(sectionIndex, localIndex);
                if (blockState == nullptr) {
                    continue;
                }

                u64 flags = 0;
                if (blockState->useShapeForLightOcclusion()) {
                    CollisionShape cullingFace =
                        blockState->getFaceOcclusionShape(_getNMSDirection(_getOppositeDirection(propagate)));
                    if (cullingFace.isFullBlock()) {
                        // 完全面遮挡
                        continue;
                    }
                    if (!cullingFace.isEmpty()) {
                        flags |= FLAG_HAS_SIDED_TRANSPARENT_BLOCKS;
                    }
                }

                i32 opacity = blockState->getBlock().getOpacity(*blockState);
                i32 targetLevel = std::max(0, propagatedLevel - std::max(1, opacity));

                if (lightLevel > targetLevel) {
                    // it looks like another source propagated here, so re-propagate it
                    if (increaseQueueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        resizeIncreaseQueue();
                    }
                    m_increaseQueue[static_cast<size_t>(increaseQueueLength++)] =
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(lightLevel & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
                        (FLAG_RECHECK_LEVEL | flags);
                    continue;
                }

                i32 emittedLight = 0;
                if (blockState != nullptr) {
                    emittedLight = blockState->getBlock().getLightLevel(*blockState) & emittedMask;
                }
                if (emittedLight != 0) {
                    // re-propagate source
                    // note: do not set recheck level, or else the propagation will fail
                    if (increaseQueueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        resizeIncreaseQueue();
                    }
                    m_increaseQueue[static_cast<size_t>(increaseQueueLength++)] =
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(emittedLight & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
                        (flags | FLAG_WRITE_LEVEL);
                }

                currentNibble->set(localIndex, 0);
                postLightUpdate(offX, offY, offZ);

                if (targetLevel > 0) {
                    if (queueLength >= static_cast<i32>(m_decreaseQueue.size())) {
                        resizeDecreaseQueue();
                    }
                    m_decreaseQueue[static_cast<size_t>(queueLength++)] =
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(targetLevel & 0xF) << 28) |
                        (static_cast<u64>(_getEverythingButOppositeDirection(propagate)) << 32) | flags;
                }
            }
        } else {
            // we actually need to worry about our state here
            const BlockState* fromBlock = getBlockState(posX, posY, posZ);
            const std::vector<LightAxisDirection>& directions =
                s_oldCheckDirections[static_cast<size_t>(directionBits)];

            for (LightAxisDirection propagate : directions) {
                i32 dx, dy, dz;
                _getDirectionOffset(propagate, dx, dy, dz);

                i32 offX = posX + dx;
                i32 offY = posY + dy;
                i32 offZ = posZ + dz;

                // 检查源方块的遮挡面
                CollisionShape fromShape; // 空 shape
                if (fromBlock != nullptr && fromBlock->useShapeForLightOcclusion()) {
                    fromShape = fromBlock->getFaceOcclusionShape(_getNMSDirection(propagate));
                }

                if (!fromShape.isEmpty() && fromShape.isFullBlock()) {
                    // 源面完全遮挡
                    continue;
                }

                i32 sectionIndex = (offX >> world::CHUNK_SHIFT) + 5 * (offZ >> world::CHUNK_SHIFT) +
                    (5 * 5) * (offY >> world::CHUNK_SHIFT) + sectionOffset;
                i32 localIndex = (offX & world::CHUNK_MASK) | ((offZ & world::CHUNK_MASK) << world::CHUNK_SHIFT) |
                    ((offY & world::CHUNK_MASK) << 8);

                SWMRNibbleArray* currentNibble = m_nibbleCache[sectionIndex];
                i32 lightLevel;

                if (currentNibble == nullptr || (lightLevel = currentNibble->getUpdating(localIndex)) == 0) {
                    // already at lowest (or unloaded), nothing we can do
                    continue;
                }

                const BlockState* blockState = getBlockState(sectionIndex, localIndex);
                if (blockState == nullptr) {
                    continue;
                }

                u64 flags = 0;
                if (blockState->useShapeForLightOcclusion()) {
                    CollisionShape cullingFace =
                        blockState->getFaceOcclusionShape(_getNMSDirection(_getOppositeDirection(propagate)));

                    // 使用 VoxelShape 进行精确遮挡检测
                    VoxelShape fromVoxel = Shapes::fromCollisionShape(fromShape);
                    VoxelShape cullingVoxel = Shapes::fromCollisionShape(cullingFace);

                    if (Shapes::faceShapeOccludes(fromVoxel, cullingVoxel)) {
                        continue;
                    }
                    flags |= FLAG_HAS_SIDED_TRANSPARENT_BLOCKS;
                }

                i32 opacity = blockState->getBlock().getOpacity(*blockState);
                i32 targetLevel = std::max(0, propagatedLevel - std::max(1, opacity));

                if (lightLevel > targetLevel) {
                    // it looks like another source propagated here, so re-propagate it
                    if (increaseQueueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        resizeIncreaseQueue();
                    }
                    m_increaseQueue[static_cast<size_t>(increaseQueueLength++)] =
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(lightLevel & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
                        (FLAG_RECHECK_LEVEL | flags);
                    continue;
                }

                i32 emittedLight = 0;
                if (blockState != nullptr) {
                    emittedLight = blockState->getBlock().getLightLevel(*blockState) & emittedMask;
                }
                if (emittedLight != 0) {
                    // re-propagate source
                    // note: do not set recheck level, or else the propagation will fail
                    if (increaseQueueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        resizeIncreaseQueue();
                    }
                    m_increaseQueue[static_cast<size_t>(increaseQueueLength++)] =
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(emittedLight & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
                        (flags | FLAG_WRITE_LEVEL);
                }

                currentNibble->set(localIndex, 0);
                postLightUpdate(offX, offY, offZ);

                if (targetLevel > 0) {
                    if (queueLength >= static_cast<i32>(m_decreaseQueue.size())) {
                        resizeDecreaseQueue();
                    }
                    m_decreaseQueue[static_cast<size_t>(queueLength++)] =
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(targetLevel & 0xF) << 28) |
                        (static_cast<u64>(_getEverythingButOppositeDirection(propagate)) << 32) | flags;
                }
            }
        }
    }

    // propagate sources we clobbered
    m_increaseQueueInitialLength = increaseQueueLength;
    performLightIncrease(lightAccess);
}

// ============================================================================
// 公共接口实现
// ============================================================================

i32 StarLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight)
{
    // 参数由子类决定是否使用，基类忽略
    (void)updateSkyLight;
    (void)updateBlockLight;
    return maxUpdates; // 子类重写
}

void StarLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty)
{
    // 子类实现
    (void)pos;
    (void)isEmpty;
}

// ============================================================================
// 条件透明检查
// ============================================================================

bool StarLightEngine::isFaceOccluded(
    const BlockState* fromState, const BlockState* toState, LightAxisDirection direction)
{
    // 空气不遮挡任何面
    if (fromState == nullptr || toState == nullptr) {
        return false;
    }

    // 检查任一方块是否需要形状遮挡检测
    bool fromUseShape = useShapeForLightOcclusion(fromState);
    bool toUseShape = useShapeForLightOcclusion(toState);

    // 如果都不需要形状检测，使用传统的不透明度检查
    if (!fromUseShape && !toUseShape) {
        // 完整不透明方块会遮挡
        // 注意：对于普通方块，不透明度检查在调用方已经完成
        return false;
    }

    // 获取面遮挡方向
    Direction fromDir = _getNMSDirection(direction);
    Direction toDir = _getNMSDirection(_getOppositeDirection(direction));

    // 获取遮挡形状
    const CollisionShape& fromCollisionShape = fromState->getOcclusionShape();
    const CollisionShape& toCollisionShape = toState->getOcclusionShape();

    // 转换为 VoxelShape 进行面遮挡检测
    VoxelShape fromShape = Shapes::fromCollisionShape(fromCollisionShape);
    VoxelShape toShape = Shapes::fromCollisionShape(toCollisionShape);

    // 使用 Shapes 类进行面遮挡检测
    return Shapes::blockOccludes(fromShape, toShape, fromDir);
}

bool StarLightEngine::useShapeForLightOcclusion(const BlockState* state)
{
    if (state == nullptr) {
        return false;
    }
    return state->useShapeForLightOcclusion();
}

// ============================================================================
// 静态工具方法
// ============================================================================

std::vector<SWMRNibbleArray*> StarLightEngine::getFilledEmptyLight(i32 totalLightSections)
{
    std::vector<SWMRNibbleArray*> ret;
    ret.reserve(static_cast<size_t>(totalLightSections));

    for (i32 i = 0; i < totalLightSections; ++i) {
        ret.push_back(new SWMRNibbleArray(nullptr, true)); // Null 状态
    }

    return ret;
}

} // namespace mc
