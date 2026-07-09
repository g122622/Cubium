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

#include "DesertWellFeature.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BrushableBlockEntity.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

#include <array>

namespace mc::world::gen::feature::cave {

namespace {

/// MC: DesertWellFeature 常量方块状态。
const BlockState* sandState()
{
    return VanillaBlocks::getState(VanillaBlocks::SAND);
}
const BlockState* sandSlabState()
{
    return VanillaBlocks::getState(VanillaBlocks::SANDSTONE_SLAB);
}
const BlockState* sandstoneState()
{
    return VanillaBlocks::getState(VanillaBlocks::SANDSTONE);
}
const BlockState* waterState()
{
    return VanillaBlocks::getState(VanillaBlocks::WATER);
}
const BlockState* suspiciousSandState()
{
    return VanillaBlocks::getState(VanillaBlocks::SUSPICIOUS_SAND);
}

/// MC: WorldGenLevel.isEmptyBlock —— 空气（含 nullptr 视为空气）。
bool isEmptyBlock(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    return state == nullptr || state->isAir();
}

/// MC: BlockStatePredicate.forBlock(SAND) —— state 是否为 SAND。
bool isSand(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr && state->is(VanillaBlocks::SAND);
}

/// MC: placeSusSand —— 放置可疑沙并为其方块实体设置考古战利品表。
void placeSusSand(IWorld& world, const BlockPos& pos, const ResourceLocation& lootTable, i64 seed)
{
    world.setBlockState(pos, suspiciousSandState());
    BlockEntity* be = world.getBlockEntity(pos);
    if (be != nullptr && be->getType() == BlockEntityType::BrushableBlock) {
        static_cast<blockentity::BrushableBlockEntity*>(be)->setLootTable(lootTable, seed);
    }
}

} // namespace

// ============================================================================
// ConfiguredDesertWellFeature
// ============================================================================

ConfiguredDesertWellFeature::ConfiguredDesertWellFeature(const char* featureName)
    : m_name(featureName)
{}

bool ConfiguredDesertWellFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return m_feature.place(region, generator, random, pos);
}

// ============================================================================
// DesertWellFeature
// ============================================================================

bool DesertWellFeature::place(
    IWorld& world, IChunkGenerator& /*generator*/, math::Random& random, const BlockPos& origin)
{
    // MC: BuiltInLootTables.DESERT_WELL_ARCHAEOLOGY
    static const ResourceLocation DESERT_WELL_ARCHAEOLOGY("minecraft", "archaeology/desert_well");

    // blockpos = origin.above(); 然后向下找到第一个非空方块。
    BlockPos blockpos(origin.x, origin.y + 1, origin.z);
    while (isEmptyBlock(world, blockpos) && blockpos.y > world.getMinBuildHeight() + 2) {
        blockpos = BlockPos(blockpos.x, blockpos.y - 1, blockpos.z);
    }

    if (!isSand(world, blockpos)) {
        return false;
    }

    // 合法性扫描：3×3 范围下方两格都必须非空。
    for (i32 i = -2; i <= 2; ++i) {
        for (i32 j = -2; j <= 2; ++j) {
            if (isEmptyBlock(world, BlockPos(blockpos.x + i, blockpos.y - 1, blockpos.z + j)) &&
                isEmptyBlock(world, BlockPos(blockpos.x + i, blockpos.y - 2, blockpos.z + j))) {
                return false;
            }
        }
    }

    // 沙岩基座（y=-2..0，5×5）。
    for (i32 l = -2; l <= 0; ++l) {
        for (i32 i1 = -2; i1 <= 2; ++i1) {
            for (i32 k = -2; k <= 2; ++k) {
                world.setBlockState(BlockPos(blockpos.x + i1, blockpos.y + l, blockpos.z + k), sandstoneState());
            }
        }
    }

    // 中央水源 + 四向水平水源。
    world.setBlockState(blockpos, waterState());
    for (Direction direction : Directions::horizontal()) {
        world.setBlockState(blockpos.offset(direction), waterState());
    }

    // 井底沙（中心 + 四向）。
    const BlockPos blockpos1(blockpos.x, blockpos.y - 1, blockpos.z);
    world.setBlockState(blockpos1, sandState());
    for (Direction direction1 : Directions::horizontal()) {
        world.setBlockState(blockpos1.offset(direction1), sandState());
    }

    // 5×5 边框沙岩（y=+1，仅外圈）。
    for (i32 j1 = -2; j1 <= 2; ++j1) {
        for (i32 i2 = -2; i2 <= 2; ++i2) {
            if (j1 == -2 || j1 == 2 || i2 == -2 || i2 == 2) {
                world.setBlockState(BlockPos(blockpos.x + j1, blockpos.y + 1, blockpos.z + i2), sandstoneState());
            }
        }
    }

    // 四向中点沙岩台阶。
    world.setBlockState(BlockPos(blockpos.x + 2, blockpos.y + 1, blockpos.z), sandSlabState());
    world.setBlockState(BlockPos(blockpos.x - 2, blockpos.y + 1, blockpos.z), sandSlabState());
    world.setBlockState(BlockPos(blockpos.x, blockpos.y + 1, blockpos.z + 2), sandSlabState());
    world.setBlockState(BlockPos(blockpos.x, blockpos.y + 1, blockpos.z - 2), sandSlabState());

    // 顶部 3×3（y=+4）：中心沙岩，其余沙岩台阶。
    for (i32 k1 = -1; k1 <= 1; ++k1) {
        for (i32 j2 = -1; j2 <= 1; ++j2) {
            const BlockState* top = (k1 == 0 && j2 == 0) ? sandstoneState() : sandSlabState();
            world.setBlockState(BlockPos(blockpos.x + k1, blockpos.y + 4, blockpos.z + j2), top);
        }
    }

    // 四角立柱（y=+1..+3）。
    for (i32 l1 = 1; l1 <= 3; ++l1) {
        world.setBlockState(BlockPos(blockpos.x - 1, blockpos.y + l1, blockpos.z - 1), sandstoneState());
        world.setBlockState(BlockPos(blockpos.x - 1, blockpos.y + l1, blockpos.z + 1), sandstoneState());
        world.setBlockState(BlockPos(blockpos.x + 1, blockpos.y + l1, blockpos.z - 1), sandstoneState());
        world.setBlockState(BlockPos(blockpos.x + 1, blockpos.y + l1, blockpos.z + 1), sandstoneState());
    }

    // 在井底 5 个水源位（中心+四向）下方随机选 2 处放可疑沙。
    const std::array<BlockPos, 5> list = {
        blockpos, blockpos.east(), blockpos.south(), blockpos.west(), blockpos.north()};
    // MC: Util.getRandom(list, random)。loot seed 用 BlockPos.asLong() 的等价 toId()。
    {
        const BlockPos& picked = list[static_cast<size_t>(random.nextInt(static_cast<i32>(list.size())))];
        placeSusSand(world,
            BlockPos(picked.x, picked.y - 1, picked.z),
            DESERT_WELL_ARCHAEOLOGY,
            static_cast<i64>(picked.toId()));
    }
    {
        const BlockPos& picked = list[static_cast<size_t>(random.nextInt(static_cast<i32>(list.size())))];
        placeSusSand(world,
            BlockPos(picked.x, picked.y - 2, picked.z),
            DESERT_WELL_ARCHAEOLOGY,
            static_cast<i64>(picked.toId()));
    }
    return true;
}

} // namespace mc::world::gen::feature::cave
