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
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"

namespace mc::world::chunk {

// ============================================================================
// Heightmap 实现
// ============================================================================

Heightmap::Heightmap(HeightmapType type)
    : m_type(type)
{
    m_heights.fill(NO_BLOCK_SENTINEL);
}

bool Heightmap::update(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return false;
    }

    const i32 index = z * mc::world::CHUNK_WIDTH + x;
    const BlockCoord currentHeight = m_heights[static_cast<size_t>(index)];

    // 只有当新方块不低于当前高度且是阻挡方块时才更新。
    // currentHeight 为 NO_BLOCK_SENTINEL（MIN_BUILD_HEIGHT-1）时表示该列尚无方块，
    // 任何合法 y（>= MIN_BUILD_HEIGHT）都满足 y >= currentHeight，从而正常写入。
    if (y >= currentHeight && _isOpaque(state)) {
        m_heights[static_cast<size_t>(index)] = y + 1; // 高度图存储的是 Y+1（即上方空气方块的位置）
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
    return m_heights[static_cast<size_t>(index)];
}

void Heightmap::setHeight(BlockCoord x, BlockCoord z, BlockCoord height)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return;
    }
    const i32 index = z * mc::world::CHUNK_WIDTH + x;
    m_heights[static_cast<size_t>(index)] = height;
}

void Heightmap::setData(const std::array<BlockCoord, SIZE>& data)
{
    m_heights = data;
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
            // 原版 blocksMotion() || !fluidState.isEmpty()
            // 项目用 isLiquid() 近似流体判定（仅方块自身是水/岩浆；
            // 含水方块中的源水未计入，见 TODO）
            return _blocksMotion(block, *state) || state->isLiquid();

        case HeightmapType::MotionBlockingNoLeaves:
            // 原版 (blocksMotion() || hasFluid) && !(block instanceof LeavesBlock)
            return (_blocksMotion(block, *state) || state->isLiquid()) && _isNotLeaf(block);

        case HeightmapType::LightBlocking:
            // 原版 getOpacity(state) > 0（纯挡光判定，无 isSolid 前置；
            // 前置 isSolid 会错误排除树叶等"非 solid 但 opacity>0"的方块）
            return state->getOpacity() > 0;

        default:
            return !block.isAir(*state);
    }
}

bool Heightmap::_blocksMotion(const Block& block, const BlockState& state)
{
    // 近似原版 blocksMotion() = isSolid() && block != COBWEB && block != BAMBOO_SAPLING。
    // 项目无 blocksMotion() 方法，用 isSolid + Block 指针排除蜘蛛网近似。
    // bamboo_sapling 用 Material::REPLACEABLE_PLANT（isSolid=false）天然不命中，无需特判。
    // TODO: 原版还排除 BAMBOO_SAPLING；项目该方块 isSolid 已为 false，无需显式排除。
    if (!block.isSolid(state)) {
        return false;
    }
    return &block != block_registry::NaturalBlocks::COBWEB;
}

bool Heightmap::_isNotLeaf(const Block& block)
{
    // 原版 !(block instanceof LeavesBlock)。项目按 Material::LEAVES 指针比较（轻量，无需 RTTI）。
    // 注意：项目树叶注册时带 .notSolid()，isSolid=false，已被 _blocksMotion 过滤；
    // 此处排除是防御性的，确保 NoLeaves 语义明确。
    return &block.material() != &Material::LEAVES;
}

} // namespace mc::world::chunk
