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

#include "BlockLightEngine.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/engine/BaseLightEngine.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
#include <fmt/format.h>

using namespace mc::trace;

namespace mc {

// ============================================================================
// 辅助函数
// ============================================================================

// ============================================================================
// 构造函数
// ============================================================================

BlockStarLightEngine::BlockStarLightEngine()
    : StarLightEngine(false)
{ // false = 不是天空光照

    // 世界高度范围由基类从世界高度常量自动计算
    // m_minSection, m_maxSection, m_minLightSection, m_maxLightSection

    i32 totalLightSections = m_maxLightSection - m_minLightSection + 1;

    // 初始化 Nibble 缓存
    i32 sectionCacheSize = 5 * 5 * (totalLightSections + 2 + 2);
    m_sectionCacheSize = sectionCacheSize;
    m_sectionCache = new const ChunkSection*[static_cast<size_t>(sectionCacheSize)]();
    m_nibbleCache = new SWMRNibbleArray*[static_cast<size_t>(sectionCacheSize)]();
    m_notifyUpdateCache = new bool[static_cast<size_t>(sectionCacheSize)]();

    // 初始化队列（区块段体积 = 16 * 16 * 16）
    m_increaseQueue.resize(world::CHUNK_SECTION_HEIGHT * world::CHUNK_SECTION_HEIGHT * world::CHUNK_SECTION_HEIGHT);
    m_decreaseQueue.resize(world::CHUNK_SECTION_HEIGHT * world::CHUNK_SECTION_HEIGHT * world::CHUNK_SECTION_HEIGHT);
}

// ============================================================================
// 世界设置
// ============================================================================

void BlockStarLightEngine::setWorld(void* world)
{
    StarLightEngine::setWorld(world);
    // 缓存已在构造函数中初始化
}

// ============================================================================
// 空映射管理
// ============================================================================

const bool* BlockStarLightEngine::getEmptinessMap(const IChunk* chunk) const
{
    return chunk->getBlockEmptinessMap();
}

void BlockStarLightEngine::setEmptinessMap(const IChunk* chunk, const bool* map)
{
    // 注意: const_cast 是因为 IChunk 接口定义的 set 方法不是 const
    const_cast<IChunk*>(chunk)->setBlockEmptinessMap(map);
}

SWMRNibbleArray* const* BlockStarLightEngine::getNibblesOnChunk(const IChunk* chunk) const
{
    return chunk->getBlockNibbles();
}

void BlockStarLightEngine::setNibbles(const IChunk* chunk, SWMRNibbleArray* const* nibbles)
{
    const_cast<IChunk*>(chunk)->setBlockNibbles(nibbles);
}

bool BlockStarLightEngine::canUseChunk(const IChunk* chunk) const
{
    // 区块必须处于 LIGHT 状态或之后，且光照数据正确
    ChunkLoadStatus status = chunk->getStatus();
    return status == ChunkLoadStatus::Generated || status == ChunkLoadStatus::Loaded;
}

// ============================================================================
// Nibble 数组管理
// ============================================================================

void BlockStarLightEngine::initNibble(i32 chunkX, i32 chunkY, i32 chunkZ, bool extrude, bool initRemovedNibbles)
{
    if (chunkY < m_minLightSection || chunkY > m_maxLightSection) {
        return;
    }

    if (getChunkInCache(chunkX, chunkZ) == nullptr) {
        return;
    }

    SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (nibble == nullptr) {
        if (!initRemovedNibbles) {
            // initRemovedNibbles 为 false 时 nibble 不应为 null
            return;
        }
        // 创建 UNINIT 状态的 Nibble（不是 NULL 状态）
        nibble = new SWMRNibbleArray(nullptr, false); // UNINIT 状态
        setNibbleInCache(chunkX, chunkY, chunkZ, nibble);
    } else {
        nibble->setNonNull();
    }
}

void BlockStarLightEngine::setNibbleNull(i32 chunkX, i32 chunkY, i32 chunkZ)
{
    SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (nibble != nullptr) {
        // 方块光去初始化保留数据为 Hidden 状态：方块光减亮通常因方块被移除，
        // 减亮传播需要保留数据才能正确计算；Hidden 保留数据但停止传播。
        nibble->setHidden();
    }
}

// ============================================================================
// 方块检查
// ============================================================================

void BlockStarLightEngine::checkBlock(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "BlockStarLightEngine::checkBlock",
        "Position",
        fmt::format("({}, {}, {})", worldX, worldY, worldZ));

    // 方块可以改变透明度、发光等级和传播方向

    i32 encodeOffset = m_coordinateOffset;
    i32 emittedMask = m_emittedLightMask;

    i32 currentLevel = getLightLevel(worldX, worldY, worldZ);
    const BlockState* blockState = getBlockState(worldX, worldY, worldZ);

    // 获取发光等级
    i32 emittedLevel = 0;
    if (blockState != nullptr) {
        emittedLevel = blockState->getBlock().getLightLevel(*blockState) & emittedMask;
    }

    setLightLevel(worldX, worldY, worldZ, emittedLevel);

    // 如果有发射光，添加到增亮队列
    if (emittedLevel != 0) {
        // 检查方块是否使用形状进行光照遮挡（条件透明）
        bool hasSidedTransparent = (blockState != nullptr && blockState->isConditionallyFullOpaque());

        appendToIncreaseQueue(((worldX + (worldZ << 6) + (worldY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
            (static_cast<u64>(emittedLevel & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
            (hasSidedTransparent ? FLAG_HAS_SIDED_TRANSPARENT_BLOCKS : 0));
    }

    // 添加到减亮队列
    appendToDecreaseQueue(((worldX + (worldZ << 6) + (worldY << 12) + encodeOffset) & ((1LL << 28) - 1)) |
        (static_cast<u64>(currentLevel & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32));
}

// ============================================================================
// 光照计算
// ============================================================================

i32 BlockStarLightEngine::calculateLightValue(
    StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ, i32 expected)
{
    const BlockState* centerState = getBlockState(worldX, worldY, worldZ);

    i32 level = 0;
    if (centerState != nullptr) {
        level = centerState->getBlock().getLightLevel(*centerState) & m_emittedLightMask;
    }

    // 如果光源等级已经达到最大值减1，或者超过预期值，直接返回
    if (level >= (game::MAX_LIGHT_LEVEL - 1) || level > expected) {
        return level;
    }

    i32 opacity = 1;
    if (centerState != nullptr) {
        opacity = std::max(1, centerState->getBlock().getOpacity(*centerState));
    }

    // 如果不透明度达到最大值，无法传播光线
    if (opacity >= game::MAX_LIGHT_LEVEL) {
        return level;
    }

    // 检查中心方块是否是条件透明方块
    const BlockState* conditionallyOpaqueState = nullptr;
    if (centerState != nullptr && centerState->isConditionallyFullOpaque()) {
        conditionallyOpaqueState = centerState;
    }

    i32 sectionOffset = m_chunkSectionIndexOffset;

    for (LightAxisDirection dir : ALL_AXIS_DIRECTIONS) {
        i32 dx, dy, dz;
        _getDirectionOffset(dir, dx, dy, dz);

        i32 offX = worldX + dx;
        i32 offY = worldY + dy;
        i32 offZ = worldZ + dz;

        i32 sectionIndex = (offX >> world::SECTION_SHIFT) + 5 * (offZ >> world::SECTION_SHIFT) +
            (5 * 5) * (offY >> world::SECTION_SHIFT) + sectionOffset;
        i32 localIndex = (offX & world::CHUNK_MASK) | ((offZ & world::CHUNK_MASK) << world::SECTION_SHIFT) |
            ((offY & world::CHUNK_MASK) << 8);

        i32 neighbourLevel = getLightLevel(sectionIndex, localIndex);

        if ((neighbourLevel - 1) <= level) {
            // don't need to test transparency, we know it wont affect the result.
            continue;
        }

        const BlockState* neighbourState = getBlockState(offX, offY, offZ);

        // 条件透明检查
        if (neighbourState != nullptr && neighbourState->isConditionallyFullOpaque()) {
            // 方块可能是条件透明的（光线无法从中传播），需要检测
            // 大多数情况下这是 false，所以使用更快的透明度查找是值得的
            CollisionShape neighbourFace =
                neighbourState->getFaceOcclusionShape(_getNMSDirection(_getOppositeDirection(dir)));
            CollisionShape thisFace;
            if (conditionallyOpaqueState != nullptr) {
                thisFace = conditionallyOpaqueState->getFaceOcclusionShape(_getNMSDirection(dir));
            }

            // 使用 Shapes::faceShapeOccludes 进行精确的面遮挡检测
            VoxelShape neighbourVoxel = Shapes::fromCollisionShape(neighbourFace);
            VoxelShape thisVoxel = Shapes::fromCollisionShape(thisFace);

            if (Shapes::faceShapeOccludes(thisVoxel, neighbourVoxel)) {
                // 遮挡，不允许传播
                continue;
            }
        }

        // 通过透明度检查，计算传播后的光照等级
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

void BlockStarLightEngine::propagateBlockChanges(
    StarLightLightingProvider* lightAccess, const IChunk* chunk, const std::vector<BlockPos>& positions)
{
    for (const BlockPos& pos : positions) {
        checkBlock(lightAccess, pos.x, pos.y, pos.z);
    }

    performLightDecrease(lightAccess);
}

// ============================================================================
// 光源获取
// ============================================================================

std::vector<BlockPos> BlockStarLightEngine::_getSources(StarLightLightingProvider* lightAccess, const IChunk* chunk)
{
    std::vector<BlockPos> sources;

    i32 offX = chunk->x() << world::SECTION_SHIFT;
    i32 offZ = chunk->z() << world::SECTION_SHIFT;

    const auto sectionsArr = chunk->getSections();

    for (i32 sectionY = m_minSection; sectionY <= m_maxSection; ++sectionY) {
        i32 sectionIndex = sectionY - m_minSection;
        const ChunkSection* section = sectionsArr[static_cast<size_t>(sectionIndex)];
        if (section == nullptr || section->isEmpty()) {
            continue;
        }

        // 检查区块段是否可能有光源（优化：如果段中没有发光方块则跳过）
        i32 offY = sectionY << world::SECTION_SHIFT;

        constexpr i32 SECTION_VOLUME =
            world::CHUNK_SECTION_HEIGHT * world::CHUNK_SECTION_HEIGHT * world::CHUNK_SECTION_HEIGHT;
        for (i32 index = 0; index < SECTION_VOLUME; ++index) {
            const BlockState* state = section->getBlockState(index & world::CHUNK_MASK, // x
                (index >> 8) & world::CHUNK_MASK,                                       // y
                (index >> 4) & world::CHUNK_MASK                                        // z
            );
            if (state == nullptr || state->getBlock().getLightLevel(*state) == 0) {
                continue;
            }

            i32 x = offX | (index & world::CHUNK_MASK);
            i32 y = offY | ((index >> 8) & world::CHUNK_MASK);
            i32 z = offZ | ((index >> 4) & world::CHUNK_MASK);

            sources.emplace_back(x, y, z);
        }
    }

    return sources;
}

i32 BlockStarLightEngine::_getLightEmission(
    StarLightLightingProvider* lightAccess, const BlockState* state, i32 x, i32 y, i32 z) const
{
    MC_UNUSED(lightAccess);
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    if (state == nullptr) {
        return 0;
    }
    // 方块光源始终按方块自身亮度发射：方块光无需区块列启用门控，
    // 列启用语义仅属于天空光（SkyStarLightEngine::initNibble 控制未遮挡列填 15）。
    return state->getBlock().getLightLevel(*state) & m_emittedLightMask;
}

// ============================================================================
// 区块照亮
// ============================================================================

void BlockStarLightEngine::lightChunk(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks)
{
    std::vector<BlockPos> positions = _getSources(lightAccess, chunk);
    i32 encodeOffset = m_coordinateOffset;
    i32 emittedMask = m_emittedLightMask;

    for (const BlockPos& pos : positions) {
        const BlockState* blockState = getBlockState(pos.x, pos.y, pos.z);
        i32 emittedLight = _getLightEmission(lightAccess, blockState, pos.x, pos.y, pos.z);

        if (emittedLight <= getLightLevel(pos.x, pos.y, pos.z)) {
            continue;
        }

        // 检查方块是否使用形状进行光照遮挡（条件透明）
        bool hasSidedTransparent = (blockState != nullptr && blockState->isConditionallyFullOpaque());

        appendToIncreaseQueue(((pos.x + (pos.z << 6) + (pos.y << 12) + encodeOffset) & ((1LL << 28) - 1)) |
            (static_cast<u64>(emittedLight & 0xF) << 28) | (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
            (hasSidedTransparent ? FLAG_HAS_SIDED_TRANSPARENT_BLOCKS : 0));

        setLightLevel(pos.x, pos.y, pos.z, emittedLight);
    }

    if (needsEdgeChecks) {
        performLightIncrease(lightAccess);
        i32 chunkX = chunk->pos().x;
        i32 chunkZ = chunk->pos().z;
        checkChunkEdges(lightAccess, chunk, m_minLightSection, m_maxLightSection);
    } else {
        propagateNeighbourLevels(lightAccess, chunk, m_minLightSection, m_maxLightSection);
        performLightIncrease(lightAccess);
    }
}

// ============================================================================
// WorldLightManager 接口实现
// ============================================================================

void BlockStarLightEngine::onBlockEmissionIncrease(
    StarLightLightingProvider* lightAccess, i32 x, i32 y, i32 z, i32 lightLevel)
{
    // 方块发光等级增加，需要重新检查该位置
    (void)lightLevel;
    checkBlock(lightAccess, x, y, z);
}

i32 BlockStarLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight)
{
    if (!updateBlockLight) {
        return maxUpdates;
    }
    return performUpdates(nullptr, maxUpdates);
}

void BlockStarLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty)
{
    // 区块段状态更新
    // 空映射通过区块接口管理，空区块段可以跳过光照传播计算
    // 实际的 Nibble 初始化/去初始化在 handleEmptySectionChanges 中处理
    (void)pos;
    (void)isEmpty;
}

void BlockStarLightEngine::setData(const SectionPos& pos, const NibbleArray& array, bool retain)
{
    // 设置指定区块段的光照数据（从存档加载）
    i32 chunkY = pos.y;
    if (chunkY < m_minLightSection || chunkY > m_maxLightSection) {
        return;
    }

    // 获取缓存中的区块
    const IChunk* chunk = getChunkInCache(pos.x, pos.z);
    if (chunk == nullptr) {
        return;
    }

    // 获取区块的 Nibble 数组
    SWMRNibbleArray* const* nibbles = getNibblesOnChunk(chunk);
    if (nibbles == nullptr) {
        return;
    }

    // 计算索引
    i32 index = chunkY - m_minLightSection;
    i32 totalSections = m_maxLightSection - m_minLightSection + 1;
    if (index < 0 || index >= totalSections) {
        return;
    }

    SWMRNibbleArray* nibble = nibbles[index];
    if (nibble == nullptr) {
        return;
    }

    // 从 NibbleArray 复制数据到 SWMRNibbleArray
    // 区块段体积 = 16 * 16 * 16 = 4096
    nibble->setNonNull();
    const std::vector<u8>& data = array.data();
    constexpr i32 SECTION_VOLUME =
        world::CHUNK_SECTION_HEIGHT * world::CHUNK_SECTION_HEIGHT * world::CHUNK_SECTION_HEIGHT;
    for (i32 i = 0; i < SECTION_VOLUME && i < static_cast<i32>(data.size() * 2); ++i) {
        nibble->set(i, array.get(i));
    }

    if (!retain) {
        nibble->updateVisible();
    }

    // 同时更新缓存
    setNibbleInCache(pos.x, chunkY, pos.z, nibble);
}

SWMRNibbleArray* BlockStarLightEngine::getData(const SectionPos& pos)
{
    // 获取指定区块段的光照数据（用于保存）
    i32 chunkY = pos.y;
    if (chunkY < m_minLightSection || chunkY > m_maxLightSection) {
        return nullptr;
    }

    // 首先尝试从缓存获取
    SWMRNibbleArray* cached = getNibbleFromCache(pos.x, chunkY, pos.z);
    if (cached != nullptr) {
        return cached;
    }

    // 如果缓存中没有，尝试从区块获取
    const IChunk* chunk = getChunkInCache(pos.x, pos.z);
    if (chunk == nullptr) {
        return nullptr;
    }

    SWMRNibbleArray* const* nibbles = getNibblesOnChunk(chunk);
    if (nibbles == nullptr) {
        return nullptr;
    }

    i32 index = chunkY - m_minLightSection;
    i32 totalSections = m_maxLightSection - m_minLightSection + 1;
    if (index < 0 || index >= totalSections) {
        return nullptr;
    }

    return nibbles[index];
}

const SWMRNibbleArray* BlockStarLightEngine::getData(const SectionPos& pos) const
{
    i32 chunkY = pos.y;
    if (chunkY < m_minLightSection || chunkY > m_maxLightSection) {
        return nullptr;
    }

    // 首先尝试从缓存获取
    const SWMRNibbleArray* cached = getNibbleFromCache(pos.x, chunkY, pos.z);
    if (cached != nullptr) {
        return cached;
    }

    // 如果缓存中没有，尝试从区块获取
    const IChunk* chunk = getChunkInCache(pos.x, pos.z);
    if (chunk == nullptr) {
        return nullptr;
    }

    SWMRNibbleArray* const* nibbles = getNibblesOnChunk(chunk);
    if (nibbles == nullptr) {
        return nullptr;
    }

    i32 index = chunkY - m_minLightSection;
    i32 totalSections = m_maxLightSection - m_minLightSection + 1;
    if (index < 0 || index >= totalSections) {
        return nullptr;
    }

    return nibbles[index];
}

void BlockStarLightEngine::updateEmptinessMap(i32 chunkX, i32 chunkZ, const ChunkData* chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "BlockStarLightEngine::updateEmptinessMap");

    if (chunk == nullptr) {
        return;
    }

    // 计算区块段数量
    i32 totalSections = m_maxSection - m_minSection + 1;
    if (totalSections <= 0) {
        return;
    }

    // 更新每个区块段的空状态
    for (i32 sectionY = m_minSection; sectionY <= m_maxSection; ++sectionY) {
        i32 sectionIndex = sectionY - m_minSection;
        const ChunkSection* section = chunk->getSection(sectionIndex);
        bool isEmpty = (section == nullptr || section->isEmpty());
        updateSectionStatus(SectionPos(chunkX, sectionY, chunkZ), isEmpty);
    }
}

} // namespace mc
