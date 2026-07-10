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

#include "SkyLightEngine.hpp"
#include "common/core/Constants.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"

#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

SkyStarLightEngine::SkyStarLightEngine()
    : StarLightEngine(true)
{ // true = 天空光照

    // 世界高度范围由基类从世界高度常量自动计算
    // m_minSection, m_maxSection, m_minLightSection, m_maxLightSection

    i32 totalLightSections = m_maxLightSection - m_minLightSection + 1;

    // 初始化 Nibble 缓存
    i32 sectionCacheSize = 5 * 5 * (totalLightSections + 2 + 2);
    m_sectionCacheSize = sectionCacheSize;
    m_sectionCache = new const ChunkSection*[static_cast<size_t>(sectionCacheSize)]();
    m_nibbleCache = new SWMRNibbleArray*[static_cast<size_t>(sectionCacheSize)]();
    m_notifyUpdateCache = new bool[static_cast<size_t>(sectionCacheSize)]();

    // 初始化 null 区块段检查缓存
    m_nullPropagationCheckCache.resize(static_cast<size_t>(totalLightSections), false);

    // 初始化队列（容量为一个区块段的方块数量）
    constexpr i32 sectionVolume = world::CHUNK_WIDTH * world::CHUNK_WIDTH * world::CHUNK_SECTION_HEIGHT;
    m_increaseQueue.resize(sectionVolume);
    m_decreaseQueue.resize(sectionVolume);

    // 初始化高度图
    m_heightMapBlockChange.fill(INT_MIN);
}

// ============================================================================
// 世界设置
// ============================================================================

void SkyStarLightEngine::setWorld(void* world)
{
    StarLightEngine::setWorld(world);
    // 缓存已在构造函数中初始化
}

// ============================================================================
// 空映射管理
// ============================================================================

const bool* SkyStarLightEngine::getEmptinessMap(const IChunk* chunk) const
{
    return chunk->getSkyEmptinessMap();
}

void SkyStarLightEngine::setEmptinessMap(const IChunk* chunk, const bool* map)
{
    // 注意: const_cast 是因为 IChunk 接口定义的 set 方法不是 const
    const_cast<IChunk*>(chunk)->setSkyEmptinessMap(map);
}

SWMRNibbleArray* const* SkyStarLightEngine::getNibblesOnChunk(const IChunk* chunk) const
{
    return chunk->getSkyNibbles();
}

void SkyStarLightEngine::setNibbles(const IChunk* chunk, SWMRNibbleArray* const* nibbles)
{
    const_cast<IChunk*>(chunk)->setSkyNibbles(nibbles);
}

bool SkyStarLightEngine::canUseChunk(const IChunk* chunk) const
{
    // 仅在可用状态且光照数据正确时使用区块
    const ChunkLoadStatus status = chunk->getStatus();
    const bool hasRequiredStatus = status == ChunkLoadStatus::Generated || status == ChunkLoadStatus::Loaded;
    return hasRequiredStatus && (m_isClientSide || chunk->isLightCorrect());
}

// ============================================================================
// Nibble 数组管理
// ============================================================================

void SkyStarLightEngine::initNibble(i32 chunkX, i32 chunkY, i32 chunkZ, bool extrude, bool initRemovedNibbles)
{
    if (chunkY < m_minLightSection || chunkY > m_maxLightSection) {
        return;
    }

    if (getChunkInCache(chunkX, chunkZ) == nullptr) {
        return;
    }

    SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (nibble == nullptr) {
        MC_ASSERT_RELEASE(initRemovedNibbles);
        if (!initRemovedNibbles) {
            return;
        }
        // 先以 NULL 状态创建，再在 initNibble(...) 中决定是否变为 UNINIT/INIT
        nibble = new SWMRNibbleArray(nullptr, true);
        setNibbleInCache(chunkX, chunkY, chunkZ, nibble);
    }

    initNibble(nibble, chunkX, chunkY, chunkZ, extrude);
}

void SkyStarLightEngine::initNibble(SWMRNibbleArray* currNibble, i32 chunkX, i32 chunkY, i32 chunkZ, bool extrude)
{
    // 使用 isNullUpdating() 而不是 !isInitializedUpdating()
    // NULL 状态表示 nibble 不存在，UNINIT/INIT 状态都表示已存在（数据全零或实际数据）
    if (!currNibble->isNullUpdating()) {
        // already initialised
        return;
    }

    const bool* emptinessMap = StarLightEngine::getEmptinessMap(chunkX, chunkZ);

    // 找到最高非空区块段
    i32 lowestY = m_minLightSection - 1;
    for (i32 currY = m_maxSection; currY >= m_minSection; --currY) {
        if (emptinessMap != nullptr) {
            if (emptinessMap[currY - m_minSection]) {
                continue;
            }
        } else {
            const ChunkSection* current = getChunkSection(chunkX, currY, chunkZ);
            if (current == nullptr || current->isEmpty()) {
                continue;
            }
        }
        lowestY = currY;
        break;
    }

    if (chunkY > lowestY) {
        // 在最高非空区块段之上，设置为全亮（天空光对未遮挡列填 15）
        currNibble->setNonNull();
        currNibble->setFull();
        return;
    }

    if (extrude) {
        // 从上方非 null 区块段挤出光照
        for (i32 currY = chunkY + 1; currY <= m_maxLightSection; ++currY) {
            SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, currY, chunkZ);
            if (nibble != nullptr && !nibble->isNullUpdating()) {
                currNibble->setNonNull();
                currNibble->extrudeLower(*nibble);
                break;
            }
        }
    } else {
        currNibble->setNonNull();
    }
}

void SkyStarLightEngine::setNibbleNull(i32 chunkX, i32 chunkY, i32 chunkZ)
{
    SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (nibble != nullptr) {
        // 天空光直接清除数据：方块破坏只会增加天空光（方块阻挡消除），
        // 增亮传播可正确穿过 null 段，无需保留数据。
        nibble->setNull();
    }
}

// ============================================================================
// Null 区块段检查
// ============================================================================

void SkyStarLightEngine::rewriteNibbleCacheForSkylight(const IChunk* chunk)
{
    for (i32 index = 0; index < m_sectionCacheSize; ++index) {
        SWMRNibbleArray* nibble = m_nibbleCache[index];
        if (nibble != nullptr && nibble->isNullUpdating()) {
            // 停止在 null 区域的传播
            m_nibbleCache[index] = nullptr;
            nibble->updateVisible();
        }
    }
}

bool SkyStarLightEngine::checkNullSection(i32 chunkX, i32 chunkY, i32 chunkZ, bool extrudeInitialised)
{
    if (chunkY < m_minLightSection || chunkY > m_maxLightSection) {
        return false;
    }

    if (m_nullPropagationCheckCache[static_cast<size_t>(chunkY - m_minLightSection)]) {
        return false;
    }
    m_nullPropagationCheckCache[static_cast<size_t>(chunkY - m_minLightSection)] = true;

    // 检查水平邻居是否有非 null Nibble
    bool needInitNeighbours = false;
    for (i32 dz = -1; dz <= 1 && !needInitNeighbours; ++dz) {
        for (i32 dx = -1; dx <= 1 && !needInitNeighbours; ++dx) {
            SWMRNibbleArray* nibble = getNibbleFromCache(dx + chunkX, chunkY, dz + chunkZ);
            if (nibble != nullptr && !nibble->isNullUpdating()) {
                needInitNeighbours = true;
            }
        }
    }

    if (needInitNeighbours) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                initNibble(dx + chunkX, chunkY, dz + chunkZ, (dx | dz) == 0 ? extrudeInitialised : true, true);
            }
        }
    }

    return needInitNeighbours;
}

i32 SkyStarLightEngine::getLightLevelExtruded(i32 worldX, i32 worldY, i32 worldZ)
{
    i32 chunkX = worldX >> world::CHUNK_SHIFT;
    i32 chunkY = worldY >> world::SECTION_SHIFT;
    i32 chunkZ = worldZ >> world::CHUNK_SHIFT;

    SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (nibble != nullptr) {
        return nibble->getUpdating(worldX, worldY, worldZ);
    }

    // 向上查找非 null 区块段
    for (;;) {
        if (++chunkY > m_maxLightSection) {
            return game::MAX_LIGHT_LEVEL; // 天空光照默认15
        }

        nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
        if (nibble != nullptr) {
            return nibble->getUpdating(worldX, 0, worldZ);
        }
    }
}

// ============================================================================
// 天空光照传播
// ============================================================================

i32 SkyStarLightEngine::tryPropagateSkylight(
    i32 worldX, i32 startY, i32 worldZ, bool extrudeInitialised, bool delayLightSet)
{
    i32 encodeOffset = m_coordinateOffset;
    i64 propagateDirection =
        static_cast<i64>(getEverythingButDirection(LightAxisDirection::POSITIVE_Y)); // just don't check upwards

    if (getLightLevelExtruded(worldX, startY + 1, worldZ) != game::MAX_LIGHT_LEVEL) {
        return startY;
    }

    // ensure this section is always checked
    checkNullSection(
        worldX >> world::CHUNK_SHIFT, startY >> world::SECTION_SHIFT, worldZ >> world::CHUNK_SHIFT, extrudeInitialised);

    const BlockState* above = getBlockState(worldX, startY + 1, worldZ);

    for (i32 currY = startY; currY >= (m_minLightSection << world::SECTION_SHIFT); --currY) {
        if ((currY & world::CHUNK_MASK) == world::CHUNK_MASK) {
            // ensure this section is always checked
            checkNullSection(worldX >> world::CHUNK_SHIFT,
                currY >> world::SECTION_SHIFT,
                worldZ >> world::CHUNK_SHIFT,
                extrudeInitialised);
        }
        const BlockState* current = getBlockState(worldX, currY, worldZ);

        VoxelShape fromShape = Shapes::empty();
        if (above != nullptr && above->useShapeForLightOcclusion()) {
            fromShape = Shapes::fromCollisionShape(above->getFaceOcclusionShape(Direction::Down));
            if (Shapes::faceShapeOccludes(Shapes::empty(), fromShape)) {
                // above wont let us propagate
                break;
            }
        }

        u64 flags = 0;
        if (current != nullptr && current->useShapeForLightOcclusion()) {
            const VoxelShape cullingFace = Shapes::fromCollisionShape(current->getFaceOcclusionShape(Direction::Up));
            if (Shapes::faceShapeOccludes(fromShape, cullingFace)) {
                // can't propagate here, we're done on this column
                break;
            }
            flags |= FLAG_HAS_SIDED_TRANSPARENT_BLOCKS;
        }

        const i32 opacity = current == nullptr ? 0 : current->getBlock().getOpacity(*current);

        if (opacity > 0) {
            // let the queued value (if any) handle it from here.
            break;
        }

        // 光照队列编码格式（紧凑64位）：
        // 位 0-5: X坐标（相对于编码偏移）
        // 位 6-11: Z坐标（相对于编码偏移）
        // 位 12-27: Y坐标（相对于编码偏移）
        // 位 28-31: 传播级别 (0-15)
        // 位 32-37: 传播方向位集
        appendToIncreaseQueue(((worldX + (worldZ << 6) + (currY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
            (static_cast<u64>(game::MAX_LIGHT_LEVEL) << 28) // 天空光照满级
            | (propagateDirection << 32) | flags);

        above = current;

        if (getNibbleFromCache(
                worldX >> world::CHUNK_SHIFT, currY >> world::SECTION_SHIFT, worldZ >> world::CHUNK_SHIFT) == nullptr) {
            // we skip empty sections here, as this is just an easy way of making sure the above block
            // can propagate through air.

            // nothing can propagate in null sections, remove the queue entry for it
            --m_increaseQueueInitialLength;

            // advance currY to the top of the section below
            currY = currY & ~world::CHUNK_MASK;
            // note: this value ^ is actually 1 above the top, but the loop decrements by 1 so we actually
            // end up there

            // make sure this is marked as AIR
            above = nullptr; // AIR_BLOCK_STATE
        } else if (!delayLightSet) {
            setLightLevel(worldX, currY, worldZ, game::MAX_LIGHT_LEVEL);
        }
    }

    return startY;
}

void SkyStarLightEngine::processDelayedIncreases()
{
    i32 decodeOffsetX = -m_encodeOffsetX;
    i32 decodeOffsetY = -m_encodeOffsetY;
    i32 decodeOffsetZ = -m_encodeOffsetZ;

    for (i32 i = 0; i < m_increaseQueueInitialLength; ++i) {
        u64 queueValue = m_increaseQueue[static_cast<size_t>(i)];

        i32 posX = (static_cast<i32>(queueValue) & 63) + decodeOffsetX;
        i32 posZ = ((static_cast<i32>(queueValue) >> 6) & 63) + decodeOffsetZ;
        i32 posY = ((static_cast<i32>(queueValue) >> 12) & 0xFFFF) + decodeOffsetY;
        i32 propagatedLevel = static_cast<i32>((queueValue >> 28) & 0xF);

        setLightLevel(posX, posY, posZ, propagatedLevel);
    }
}

void SkyStarLightEngine::processDelayedDecreases()
{
    i32 decodeOffsetX = -m_encodeOffsetX;
    i32 decodeOffsetY = -m_encodeOffsetY;
    i32 decodeOffsetZ = -m_encodeOffsetZ;

    for (i32 i = 0; i < m_decreaseQueueInitialLength; ++i) {
        u64 queueValue = m_decreaseQueue[static_cast<size_t>(i)];

        i32 posX = (static_cast<i32>(queueValue) & 63) + decodeOffsetX;
        i32 posZ = ((static_cast<i32>(queueValue) >> 6) & 63) + decodeOffsetZ;
        i32 posY = ((static_cast<i32>(queueValue) >> 12) & 0xFFFF) + decodeOffsetY;

        setLightLevel(posX, posY, posZ, 0);
    }
}

// ============================================================================
// 方块检查
// ============================================================================

void SkyStarLightEngine::checkBlock(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "SkyStarLightEngine::checkBlock",
        "Position",
        fmt::format("({}, {}, {})", worldX, worldY, worldZ));

    // 方块可以改变透明度和传播方向

    i32 encodeOffset = m_coordinateOffset;
    i32 currentLevel = getLightLevel(worldX, worldY, worldZ);

    if (currentLevel == game::MAX_LIGHT_LEVEL) {
        // 必须重新传播被覆盖的天空源
        // 队列编码：位 0-27 为坐标，位 28-31 为光照等级
        appendToIncreaseQueue(((worldX + (worldZ << 6) + (worldY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
            (static_cast<u64>(currentLevel & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
            FLAG_HAS_SIDED_TRANSPARENT_BLOCKS // don't know if the block is conditionally transparent
        );
    } else {
        setLightLevel(worldX, worldY, worldZ, 0);
    }

    appendToDecreaseQueue(((worldX + (worldZ << 6) + (worldY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
        (static_cast<u64>(currentLevel & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32));
}

// ============================================================================
// 光照计算
// ============================================================================

i32 SkyStarLightEngine::calculateLightValue(
    StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ, i32 expected)
{
    if (expected == game::MAX_LIGHT_LEVEL) {
        return expected;
    }

    i32 sectionOffset = m_chunkSectionIndexOffset;
    const BlockState* centerState = getBlockState(worldX, worldY, worldZ);

    // 检查中心方块是否是条件透明方块
    const BlockState* conditionallyOpaqueState = nullptr;
    i32 opacity = 1; // 默认透明度
    if (centerState != nullptr) {
        opacity = std::max(1, centerState->getBlock().getOpacity(*centerState));
        if (centerState->useShapeForLightOcclusion()) {
            conditionallyOpaqueState = centerState;
        }
    }

    i32 level = 0;

    for (LightAxisDirection dir : ALL_AXIS_DIRECTIONS) {
        i32 dx, dy, dz;
        getDirectionOffset(dir, dx, dy, dz);

        i32 offX = worldX + dx;
        i32 offY = worldY + dy;
        i32 offZ = worldZ + dz;

        i32 sectionIndex = (offX >> 4) + 5 * (offZ >> 4) + (5 * 5) * (offY >> 4) + sectionOffset;
        i32 localIndex = (offX & 15) | ((offZ & 15) << 4) | ((offY & 15) << 8);

        i32 neighbourLevel = getLightLevel(sectionIndex, localIndex);

        if ((neighbourLevel - 1) <= level) {
            // don't need to test transparency, we know it wont affect the result.
            continue;
        }

        const BlockState* neighbourState = getBlockState(offX, offY, offZ);

        // 条件透明检查
        if (neighbourState != nullptr && neighbourState->useShapeForLightOcclusion()) {
            // here the block can be conditionally opaque (i.e light cannot propagate from it), so we need to test that
            // we don't read the blockstate because most of the time this is false, so using the faster
            // known transparency lookup results in a net win
            CollisionShape neighbourFaceShape =
                neighbourState->getFaceOcclusionShape(getNMSDirection(getOppositeDirection(dir)));
            CollisionShape thisFaceShape = (conditionallyOpaqueState != nullptr)
                ? conditionallyOpaqueState->getFaceOcclusionShape(getNMSDirection(dir))
                : CollisionShape(); // 空形状

            // 使用 VoxelShape 进行精确的面遮挡检测
            VoxelShape neighbourVoxel = Shapes::fromCollisionShape(neighbourFaceShape);
            VoxelShape thisVoxel = Shapes::fromCollisionShape(thisFaceShape);

            if (Shapes::faceShapeOccludes(thisVoxel, neighbourVoxel)) {
                // not allowed to propagate
                continue;
            }
        }

        i32 calculated = neighbourLevel - opacity;
        level = std::max(calculated, level);

        if (level > expected) {
            return level;
        }
    }

    return level;
}

// ============================================================================
// 传播方块变化
// ============================================================================

void SkyStarLightEngine::propagateBlockChanges(
    StarLightLightingProvider* lightAccess, const IChunk* chunk, const std::vector<BlockPos>& positions)
{
    rewriteNibbleCacheForSkylight(chunk);
    std::fill(m_nullPropagationCheckCache.begin(), m_nullPropagationCheckCache.end(), false);

    i32 chunkX = chunk->pos().x;
    i32 chunkZ = chunk->pos().z;
    // 高度图偏移计算：用于将世界坐标映射到高度图数组索引
    // 每个区块有 CHUNK_WIDTH * CHUNK_WIDTH 个列
    i32 heightMapOffset = chunkX * -world::CHUNK_WIDTH + (chunkZ * (-world::CHUNK_WIDTH * world::CHUNK_WIDTH));

    // 设置高度图用于变化
    for (const BlockPos& pos : positions) {
        i32 index = pos.x + (pos.z << world::CHUNK_SHIFT) + heightMapOffset;
        i32 curr = m_heightMapBlockChange[static_cast<size_t>(index)];
        if (pos.y > curr) {
            m_heightMapBlockChange[static_cast<size_t>(index)] = pos.y;
        }
    }

    // 重新计算变化列的源（高度图大小为 CHUNK_WIDTH * CHUNK_WIDTH）
    constexpr i32 heightMapSize = world::CHUNK_WIDTH * world::CHUNK_WIDTH;
    for (i32 index = 0; index < heightMapSize; ++index) {
        i32 maxY = m_heightMapBlockChange[static_cast<size_t>(index)];
        if (maxY == INT_MIN) {
            continue; // 未变化
        }
        m_heightMapBlockChange[static_cast<size_t>(index)] = INT_MIN; // 恢复默认

        i32 columnX = (index & world::CHUNK_MASK) | (chunkX << world::CHUNK_SHIFT);
        i32 columnZ = (index >> world::CHUNK_SHIFT) | (chunkZ << world::CHUNK_SHIFT);

        // 尝试从上方 Y 传播天空光
        i32 maxPropagationY = tryPropagateSkylight(columnX, maxY, columnZ, true, true);

        // 移除下方所有 15 级源
        i32 encodeOffset = m_coordinateOffset;
        i32 propagateDirection = getEverythingButDirection(LightAxisDirection::POSITIVE_Y);

        if (getLightLevelExtruded(columnX, maxPropagationY, columnZ) == game::MAX_LIGHT_LEVEL) {
            checkNullSection(columnX >> world::CHUNK_SHIFT,
                maxPropagationY >> world::SECTION_SHIFT,
                columnZ >> world::CHUNK_SHIFT,
                true);

            for (i32 currY = maxPropagationY; currY >= (m_minLightSection << world::SECTION_SHIFT); --currY) {
                if ((currY & world::CHUNK_MASK) == world::CHUNK_MASK) {
                    checkNullSection(columnX >> world::CHUNK_SHIFT,
                        currY >> world::SECTION_SHIFT,
                        columnZ >> world::CHUNK_SHIFT,
                        true);
                }

                SWMRNibbleArray* nibble = getNibbleFromCache(
                    columnX >> world::CHUNK_SHIFT, currY >> world::SECTION_SHIFT, columnZ >> world::CHUNK_SHIFT);
                if (nibble == nullptr) {
                    currY = currY & ~world::CHUNK_MASK;
                    continue;
                }

                if (nibble->getUpdating(columnX, currY, columnZ) != game::MAX_LIGHT_LEVEL) {
                    break;
                }

                appendToDecreaseQueue(((columnX + (columnZ << 6) + (currY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                    (static_cast<u64>(game::MAX_LIGHT_LEVEL) << 28) | (static_cast<u64>(propagateDirection) << 32)
                    // do not set transparent blocks for the same reason we don't in the checkBlock method
                );
            }
        }
    }

    // 处理延迟的光照设置
    processDelayedIncreases();
    processDelayedDecreases();

    // 检查变化位置
    for (const BlockPos& pos : positions) {
        checkBlock(lightAccess, pos.x, pos.y, pos.z);
    }

    performLightDecrease(lightAccess);
}

// ============================================================================
// 区块照亮
// ============================================================================

void SkyStarLightEngine::lightChunk(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks)
{
    rewriteNibbleCacheForSkylight(chunk);
    std::fill(m_nullPropagationCheckCache.begin(), m_nullPropagationCheckCache.end(), false);

    i32 chunkX = chunk->pos().x;
    i32 chunkZ = chunk->pos().z;

    // 获取区块段数组
    const ChunkSection* const* sections = chunk->getSections();

    // 找到最高非空区块段
    i32 highestNonEmptySection = m_maxSection;
    while (highestNonEmptySection == (m_minSection - 1) || sections[highestNonEmptySection - m_minSection] == nullptr ||
        sections[highestNonEmptySection - m_minSection]->isEmpty()) {
        checkNullSection(chunkX, highestNonEmptySection, chunkZ, false);

        // 尝试向邻居传播全亮（空区块段需要将全亮传播到邻居）
        for (LightAxisDirection dir : ONLY_HORIZONTAL_DIRECTIONS) {
            i32 dx, dy, dz;
            getDirectionOffset(dir, dx, dy, dz);

            i32 neighbourX = chunkX + dx;
            i32 neighbourZ = chunkZ + dz;
            SWMRNibbleArray* neighbourNibble = getNibbleFromCache(neighbourX, highestNonEmptySection, neighbourZ);
            if (neighbourNibble == nullptr) {
                // 未加载的邻居，跳过
                continue;
            }

            // 计算边界传播参数
            i32 incX, incZ, startX, startZ;
            if (dx != 0) {
                // X 方向
                incX = 0;
                incZ = 1;
                startX =
                    (dx < 0) ? (chunkX << world::CHUNK_SHIFT) : ((chunkX << world::CHUNK_SHIFT) | world::CHUNK_MASK);
                startZ = chunkZ << world::CHUNK_SHIFT;
            } else {
                // Z 方向
                incX = 1;
                incZ = 0;
                startX = chunkX << world::CHUNK_SHIFT;
                startZ =
                    (dz < 0) ? (chunkZ << world::CHUNK_SHIFT) : ((chunkZ << world::CHUNK_SHIFT) | world::CHUNK_MASK);
            }

            i32 encodeOffset = m_coordinateOffset;
            i32 propagateDir = getDirectionBitset(dir);

            // 向邻居传播全亮
            for (i32 currY = highestNonEmptySection << world::SECTION_SHIFT, maxY = currY | world::CHUNK_MASK;
                currY <= maxY;
                ++currY) {
                for (i32 i = 0, currX = startX, currZ = startZ; i < world::CHUNK_WIDTH;
                    ++i, currX += incX, currZ += incZ) {
                    appendToIncreaseQueue(((currX + (currZ << 6) + (currY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
                        (static_cast<u64>(game::MAX_LIGHT_LEVEL) << 28) // 天空光照满级
                        | (static_cast<u64>(propagateDir) << 32)
                        // no transparent flag, we know for a fact there are no blocks here that could be directionally
                        // transparent (as the section is EMPTY)
                    );
                }
            }
        }

        if (highestNonEmptySection-- == (m_minSection - 1)) {
            break;
        }
    }

    if (highestNonEmptySection >= m_minSection) {
        // 填充其他源（从最高非空区块段的顶层向下传播）
        i32 minX = chunkX << world::CHUNK_SHIFT;
        i32 maxX = (chunkX << world::CHUNK_SHIFT) | world::CHUNK_MASK;
        i32 minZ = chunkZ << world::CHUNK_SHIFT;
        i32 maxZ = (chunkZ << world::CHUNK_SHIFT) | world::CHUNK_MASK;
        i32 startY = (highestNonEmptySection << world::SECTION_SHIFT) | world::CHUNK_MASK;

        for (i32 currZ = minZ; currZ <= maxZ; ++currZ) {
            for (i32 currX = minX; currX <= maxX; ++currX) {
                tryPropagateSkylight(currX, startY + 1, currZ, false, false);
            }
        }
    }
    // else: 区块是空的

    if (needsEdgeChecks) {
        // 不需要在这里传播，但可以减少边缘检查的开销
        performLightIncrease(lightAccess);

        for (i32 y = highestNonEmptySection; y >= m_minLightSection; --y) {
            checkNullSection(chunkX, y, chunkZ, false);
        }

        // 直接调基类方法
        StarLightEngine::checkChunkEdges(lightAccess, chunk, m_minLightSection, highestNonEmptySection);
    } else {
        for (i32 y = highestNonEmptySection; y >= m_minLightSection; --y) {
            checkNullSection(chunkX, y, chunkZ, false);
        }

        propagateNeighbourLevels(lightAccess, chunk, m_minLightSection, highestNonEmptySection);
        performLightIncrease(lightAccess);
    }
}

// ============================================================================
// 区块边缘检查
// ============================================================================

void SkyStarLightEngine::checkChunkEdges(
    StarLightLightingProvider* lightAccess, const IChunk* chunk, i32 fromSection, i32 toSection)
{
    std::fill(m_nullPropagationCheckCache.begin(), m_nullPropagationCheckCache.end(), false);
    rewriteNibbleCacheForSkylight(chunk);

    i32 chunkX = chunk->pos().x;
    i32 chunkZ = chunk->pos().z;

    for (i32 y = toSection; y >= fromSection; --y) {
        checkNullSection(chunkX, y, chunkZ, true);
    }

    // 调用基类方法
    StarLightEngine::checkChunkEdges(lightAccess, chunk, fromSection, toSection);
}

// ============================================================================
// WorldLightManager 接口实现
// ============================================================================

i32 SkyStarLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight)
{
    if (!updateSkyLight) {
        return maxUpdates;
    }
    return performUpdates(nullptr, maxUpdates);
}

void SkyStarLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty)
{
    // 更新区块段的空状态缓存
    // 这用于优化光照传播（空段不需要传播）
    i32 sectionY = pos.y;
    if (sectionY < m_minLightSection || sectionY > m_maxLightSection) {
        return;
    }
    i32 cacheIndex = sectionY - m_minLightSection;
    if (cacheIndex >= 0 && cacheIndex < static_cast<i32>(m_nullPropagationCheckCache.size())) {
        m_nullPropagationCheckCache[static_cast<size_t>(cacheIndex)] = isEmpty;
    }
}

u8 SkyStarLightEngine::getLightFor(i32 x, i32 y, i32 z) const
{
    return static_cast<u8>(getLightLevel(x, y, z));
}

void SkyStarLightEngine::setData(const SectionPos& pos, const NibbleArray& array, bool retain)
{
    // 设置指定区块段的光照数据
    i32 sectionY = pos.y;
    if (sectionY < m_minLightSection || sectionY > m_maxLightSection) {
        return;
    }

    SWMRNibbleArray* nibble = getNibbleFromCache(pos.x, sectionY, pos.z);
    if (nibble == nullptr) {
        nibble = new SWMRNibbleArray(nullptr, true);
        setNibbleInCache(pos.x, sectionY, pos.z, nibble);
    }

    // 从 NibbleArray 复制数据（区块段体积 = CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_SECTION_HEIGHT）
    nibble->setNonNull();
    constexpr i32 sectionVolume = world::CHUNK_WIDTH * world::CHUNK_WIDTH * world::CHUNK_SECTION_HEIGHT;
    for (i32 i = 0; i < sectionVolume; ++i) {
        nibble->set(i, array.get(i));
    }

    if (!retain) {
        nibble->setHidden();
    }
}

SWMRNibbleArray* SkyStarLightEngine::getData(const SectionPos& pos)
{
    i32 sectionY = pos.y;
    if (sectionY < m_minLightSection || sectionY > m_maxLightSection) {
        return nullptr;
    }
    return getNibbleFromCache(pos.x, sectionY, pos.z);
}

const SWMRNibbleArray* SkyStarLightEngine::getData(const SectionPos& pos) const
{
    i32 sectionY = pos.y;
    if (sectionY < m_minLightSection || sectionY > m_maxLightSection) {
        return nullptr;
    }
    return getNibbleFromCache(pos.x, sectionY, pos.z);
}

} // namespace mc
