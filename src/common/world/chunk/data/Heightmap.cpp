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

#include "common/world/chunk/data/Heightmap.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <array>
#include <cstddef>

namespace mc::world::chunk {

// ============================================================================
// Heightmap 实现
// ============================================================================

Heightmap::Heightmap(HeightmapType type)
    : m_type(type)
{
    // m_words 默认全 0，而编码 0 即 NO_BLOCK_SENTINEL（见 _encode），故无需显式填充。
    static_assert(BITS >= 1 && BITS <= 32, "Heightmap 位宽必须落在 (0, 32]");
    static_assert(VALUES_PER_LONG >= 1, "BITS 不得超过 64");
}

void Heightmap::rebuild(const ChunkData& chunk,
    std::array<Heightmap, HEIGHTMAP_TYPE_COUNT>& heightmaps,
    const std::array<bool, HEIGHTMAP_TYPE_COUNT>& enabled)
{
    size_t enabledCount = 0;
    for (size_t i = 0; i < HEIGHTMAP_TYPE_COUNT; ++i) {
        if (enabled[i]) {
            heightmaps[i] = Heightmap(static_cast<HeightmapType>(i));
            ++enabledCount;
        }
    }
    if (enabledCount == 0) {
        return;
    }

    // 段快照只取一次；整段为空时无需对每列逐个读取空气方块。
    const auto sections = chunk.getSections();
    i32 highestSection = world::CHUNK_SECTIONS - 1;
    while (highestSection >= 0) {
        const ChunkSection* section = sections[static_cast<size_t>(highestSection)];
        if (section != nullptr && !section->isEmpty()) {
            break;
        }
        --highestSection;
    }

    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            auto unresolved = enabled;
            size_t remaining = enabledCount;
            for (i32 sectionIndex = highestSection; sectionIndex >= 0 && remaining != 0; --sectionIndex) {
                const ChunkSection* section = sections[static_cast<size_t>(sectionIndex)];
                if (section == nullptr || section->isEmpty()) {
                    continue;
                }
                const BlockCoord bottomY = world::sectionToY(sectionIndex);
                for (i32 localY = world::CHUNK_SECTION_HEIGHT - 1; localY >= 0 && remaining != 0; --localY) {
                    const BlockState* state = section->getBlockState(x, localY, z);
                    if (state == nullptr || state->isAir()) {
                        continue;
                    }
                    for (size_t i = 0; i < HEIGHTMAP_TYPE_COUNT; ++i) {
                        if (unresolved[i] && isOpaqueForType(static_cast<HeightmapType>(i), state)) {
                            heightmaps[i].setHeight(x, z, bottomY + localY + 1);
                            unresolved[i] = false;
                            --remaining;
                        }
                    }
                }
            }
        }
    }
}

bool Heightmap::update(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) [[unlikely]] {
        return false;
    }

    const i32 index = z * mc::world::CHUNK_WIDTH + x;
    const BlockCoord currentHeight = _decode(_getEncoded(index));

    // 只有当新方块不低于当前高度且是阻挡方块时才更新。
    // currentHeight 为 NO_BLOCK_SENTINEL（MIN_BUILD_HEIGHT-1）时表示该列尚无方块，
    // 任何合法 y（>= MIN_BUILD_HEIGHT）都满足 y >= currentHeight，从而正常写入。
    if (y >= currentHeight && _isOpaque(state)) {
        _setEncoded(index, _encode(y + 1)); // 高度图存储的是 Y+1（即上方空气方块的位置）
        return true;
    }

    return false;
}

BlockCoord Heightmap::getHeight(BlockCoord x, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return NO_BLOCK_SENTINEL;
    }
    const i32 index = z * mc::world::CHUNK_WIDTH + x;
    return _decode(_getEncoded(index));
}

void Heightmap::setHeight(BlockCoord x, BlockCoord z, BlockCoord height)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return;
    }
    const i32 index = z * mc::world::CHUNK_WIDTH + x;
    _setEncoded(index, _encode(height));
}

void Heightmap::setData(const std::array<BlockCoord, Heightmap::SIZE>& data)
{
    for (i32 index = 0; index < Heightmap::SIZE; ++index) {
        _setEncoded(index, _encode(data[static_cast<size_t>(index)]));
    }
}

void Heightmap::setAll(BlockCoord value)
{
    const i32 encoded = _encode(value);
    // 同一编码值铺满全部字：先把单个字内各槽位填好，再整字复制。
    u64 word = 0;
    for (i32 i = 0; i < VALUES_PER_LONG; ++i) {
        word |= (static_cast<u64>(encoded) & VALUE_MASK) << (i * BITS);
    }
    m_words.fill(word);
}

std::array<BlockCoord, Heightmap::SIZE> Heightmap::getData() const
{
    std::array<BlockCoord, SIZE> data{};
    for (i32 index = 0; index < SIZE; ++index) {
        data[static_cast<size_t>(index)] = _decode(_getEncoded(index));
    }
    return data;
}

bool Heightmap::_isOpaque(const BlockState* state) const
{
    return isOpaqueForType(m_type, state);
}

bool Heightmap::isOpaqueForType(HeightmapType type, const BlockState* state)
{
    if (!state) {
        return false;
    }

    const Block& block = state->owner();

    switch (type) {
        case HeightmapType::WorldSurface:
        case HeightmapType::WorldSurfaceWG:
            // 原版 NOT_AIR
            return !block.isAir(*state);

        case HeightmapType::OceanFloor:
        case HeightmapType::OceanFloorWG:
            // 原版 MATERIAL_MOTION_BLOCKING = blocksMotion()
            return _blocksMotion(block, *state);

        case HeightmapType::MotionBlocking:
            // 原版 blocksMotion() || !getFluidState().isEmpty()。
            // 必须查流体状态而不是 isLiquid()：含水方块（台阶、栅栏、箱子等）自身不是
            // 液体，但持有一格源水，同样应当计入 MOTION_BLOCKING 高度图。
            return _blocksMotion(block, *state) || _hasFluid(block, *state);

        case HeightmapType::MotionBlockingNoLeaves:
            // 原版 (blocksMotion() || hasFluid) && !(block instanceof LeavesBlock)
            return (_blocksMotion(block, *state) || _hasFluid(block, *state)) && _isNotLeaf(block);

        case HeightmapType::LightBlocking:
            // 原版 getOpacity(state) > 0（纯挡光判定，无 isSolid 前置；
            // 前置 isSolid 会错误排除树叶等"非 solid 但 opacity>0"的方块）
            return state->getOpacity() > 0;

        default:
            return !block.isAir(*state);
    }
}

bool Heightmap::_hasFluid(const Block& block, const BlockState& state)
{
    // 原版判据是 !getFluidState().isEmpty()：含水方块的 fluidState 非空，
    // 因此比 isLiquid() 更准确。getFluidState 返回空指针表示无流体。
    const fluid::FluidState* fluidState = block.getFluidState(state);
    return fluidState != nullptr && !fluidState->isEmpty();
}

bool Heightmap::_blocksMotion(const Block& block, const BlockState& state)
{
    // 直接走 `Block::blocksMotion`，不再本地近似：该方法排除了蜘蛛网与竹笋，
    // 与原版 `BlockStateBase#blocksMotion` 一致。
    return block.blocksMotion(state);
}

bool Heightmap::_isNotLeaf(const Block& block)
{
    // 原版 !(block instanceof LeavesBlock)。项目按 Material::LEAVES 指针比较（轻量，无需 RTTI）。
    // 注意：项目树叶注册时带 .notSolid()，isSolid=false，已被 _blocksMotion 过滤；
    // 此处排除是防御性的，确保 NoLeaves 语义明确。
    return &block.material() != &Material::LEAVES;
}

} // namespace mc::world::chunk
