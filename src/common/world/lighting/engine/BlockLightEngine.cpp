#include "BlockLightEngine.hpp"
#include "../../../physics/collision/CollisionShape.hpp"
#include "../../../physics/shape/Shapes.hpp"
#include "../../../physics/shape/VoxelShape.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/ChunkData.hpp"
#include "../../chunk/IChunk.hpp"
#include "../IChunkLightProvider.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 辅助函数：将 CollisionShape 转换为 VoxelShape
// ============================================================================

namespace {

/**
 * @brief 将 CollisionShape 转换为 VoxelShape
 *
 * 用于面遮挡检测。对于完整方块和空形状有优化路径。
 * 参考 Moonrise 中使用 Shapes.faceShapeOccludes 的逻辑
 */
VoxelShape collisionShapeToVoxelShape(const CollisionShape& shape)
{
    if (shape.isEmpty()) {
        return Shapes::empty();
    }
    if (shape.isFullBlock()) {
        return Shapes::block();
    }
    // 对于简单盒，创建对应的 VoxelShape
    const auto& boxes = shape.boxes();
    if (boxes.empty()) {
        return Shapes::empty();
    }
    // 使用第一个碰撞盒创建 VoxelShape
    // 注意：对于复杂形状（多个盒），这只是一个近似
    const auto& box = boxes[0];
    return Shapes::box(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

BlockStarLightEngine::BlockStarLightEngine(StarLightLightingProvider* provider)
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

    // 初始化队列
    m_increaseQueue.resize(16 * 16 * 16);
    m_decreaseQueue.resize(16 * 16 * 16);

    (void)provider; // 暂时未使用
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
    // 参考 Moonrise: chunk.getPersistedStatus().isOrAfter(ChunkStatus.LIGHT) && (isClientSide ||
    // chunk.isLightCorrect())
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
            // 与 Moonrise 一致：initRemovedNibbles 为 false 时 nibble 不应为 null
            return;
        }
        // 与 Moonrise 一致：创建 UNINIT 状态的 Nibble（不是 NULL 状态）
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
        // 方块光照去初始化时设为 Hidden 状态，保持外观但停止传播
        nibble->setHidden();
    }
}

// ============================================================================
// 方块检查
// ============================================================================

void BlockStarLightEngine::checkBlock(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ)
{
    MC_TRACE_EVENT("server.lighting",
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
        bool hasSidedTransparent = (blockState != nullptr && blockState->useShapeForLightOcclusion());

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
    // 参考 Moonrise BlockStarLightEngine.calculateLightValue
    const BlockState* centerState = getBlockState(worldX, worldY, worldZ);
    IWorld* world = lightAccess->getWorld();

    i32 level = 0;
    if (centerState != nullptr) {
        level = centerState->getBlock().getLightLevel(*centerState) & m_emittedLightMask;
    }

    // Moonrise: if (level >= (15 - 1) || level > expect)
    if (level >= 14 || level > expected) {
        return level;
    }

    i32 opacity = 1;
    if (centerState != nullptr) {
        opacity = std::max(1, centerState->getBlock().getOpacity(*centerState));
    }

    if (opacity >= 15) {
        return level;
    }

    // 检查中心方块是否是条件透明方块
    const BlockState* conditionallyOpaqueState = nullptr;
    if (centerState != nullptr && centerState->useShapeForLightOcclusion()) {
        conditionallyOpaqueState = centerState;
    }

    i32 sectionOffset = m_chunkSectionIndexOffset;

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

        // 条件透明检查 - 与 Moonrise 完全一致
        if (neighbourState != nullptr && neighbourState->useShapeForLightOcclusion()) {
            // here the block can be conditionally opaque (i.e light cannot propagate from it), so we need to test that
            // we don't read the blockstate because most of the time this is false, so using the faster
            // known transparency lookup results in a net win
            CollisionShape neighbourFace =
                neighbourState->getFaceOcclusionShape(getNMSDirection(getOppositeDirection(dir)));
            CollisionShape thisFace;
            if (conditionallyOpaqueState != nullptr) {
                thisFace = conditionallyOpaqueState->getFaceOcclusionShape(getNMSDirection(dir));
            }

            // 使用 Shapes::faceShapeOccludes 进行精确的面遮挡检测
            VoxelShape neighbourVoxel = collisionShapeToVoxelShape(neighbourFace);
            VoxelShape thisVoxel = collisionShapeToVoxelShape(thisFace);

            if (Shapes::faceShapeOccludes(thisVoxel, neighbourVoxel)) {
                // not allowed to propagate
                continue;
            }
        }

        // passed transparency,
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

std::vector<BlockPos> BlockStarLightEngine::getSources(StarLightLightingProvider* lightAccess, const IChunk* chunk)
{
    std::vector<BlockPos> sources;

    i32 offX = chunk->x() << 4;
    i32 offZ = chunk->z() << 4;

    const ChunkSection* const* sections = chunk->getSections();

    for (i32 sectionY = m_minSection; sectionY <= m_maxSection; ++sectionY) {
        i32 sectionIndex = sectionY - m_minSection;
        const ChunkSection* section = sections[sectionIndex];
        if (section == nullptr || section->isEmpty()) {
            continue;
        }

        // 检查区块段是否可能有光源（优化：如果段中没有发光方块则跳过）
        i32 offY = sectionY << 4;

        for (i32 index = 0; index < 4096; ++index) {
            const BlockState* state = section->getBlockState(index & 15, // x
                (index >> 8) & 15,                                       // y
                (index >> 4) & 15                                        // z
            );
            if (state == nullptr || state->getBlock().getLightLevel(*state) == 0) {
                continue;
            }

            i32 x = offX | (index & 15);
            i32 y = offY | ((index >> 8) & 15);
            i32 z = offZ | ((index >> 4) & 15);

            sources.emplace_back(x, y, z);
        }
    }

    return sources;
}

i32 BlockStarLightEngine::getLightEmission(
    StarLightLightingProvider* lightAccess, const BlockState* state, i32 x, i32 y, i32 z) const
{
    if (state == nullptr) {
        return 0;
    }
    return state->getBlock().getLightLevel(*state) & m_emittedLightMask;
}

// ============================================================================
// 区块照亮
// ============================================================================

void BlockStarLightEngine::lightChunk(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks)
{
    IWorld* world = lightAccess->getWorld();

    std::vector<BlockPos> positions = getSources(lightAccess, chunk);
    i32 encodeOffset = m_coordinateOffset;
    i32 emittedMask = m_emittedLightMask;

    for (const BlockPos& pos : positions) {
        const BlockState* blockState = getBlockState(pos.x, pos.y, pos.z);
        i32 emittedLight = getLightEmission(lightAccess, blockState, pos.x, pos.y, pos.z);

        if (emittedLight <= getLightLevel(pos.x, pos.y, pos.z)) {
            continue;
        }

        // 检查方块是否使用形状进行光照遮挡（条件透明）
        bool hasSidedTransparent = (blockState != nullptr && blockState->useShapeForLightOcclusion());

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
    // 在 Starlight 架构中，空映射通过区块接口管理
    // 空区块段可以跳过光照传播计算
    // 实际的 Nibble 初始化/去初始化在 handleEmptySectionChanges 中处理
    (void)pos;
    (void)isEmpty;
}

u8 BlockStarLightEngine::getLightFor(i32 x, i32 y, i32 z) const
{
    return static_cast<u8>(getLightLevel(x, y, z));
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
    nibble->setNonNull();
    const std::vector<u8>& data = array.data();
    for (i32 i = 0; i < 4096 && i < static_cast<i32>(data.size() * 2); ++i) {
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

void BlockStarLightEngine::updateEmptinessMap(i32 chunkX, i32 chunkZ, const ChunkData* chunk)
{
    MC_TRACE_EVENT("server.lighting", "BlockStarLightEngine::updateEmptinessMap");

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
