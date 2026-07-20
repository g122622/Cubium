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

#include "MonsterRoomFeature.hpp"

#include "common/entity/core/EntityRegistry.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <spdlog/spdlog.h>

namespace mc::world::gen::feature {

namespace {

/// MOBS 数组：zombie 出现两次，使选取概率为 skeleton:1/4, zombie:1/2, spider:1/4。
const std::array<ResourceLocation, 4> MOBS = {
    ResourceLocation(mc::entity::EntityTypeKeys::SKELETON),
    ResourceLocation(mc::entity::EntityTypeKeys::ZOMBIE),
    ResourceLocation(mc::entity::EntityTypeKeys::ZOMBIE),
    ResourceLocation(mc::entity::EntityTypeKeys::SPIDER),
};

/// SIMPLE_DUNGEON 战利品表（项目无常量类，用字面量）。
const ResourceLocation SIMPLE_DUNGEON_LOOT("minecraft", "chests/simple_dungeon");

} // namespace

bool MonsterRoomFeature::place(WorldGenRegion& region, math::Random& random, i32 x, i32 y, i32 z)
{
    // i = 3 (未使用), j = nextInt(2)+2, k1 = nextInt(2)+2
    const i32 j = random.nextInt(2) + 2;
    const i32 k = -j - 1;
    const i32 l = j + 1;
    // i1 = -1, j1 = 4 (扫描高度上下界)
    const i32 l1 = -1;
    const i32 j1 = 4;
    const i32 k1 = random.nextInt(2) + 2;
    const i32 l1z = -k1 - 1;
    const i32 i2 = k1 + 1;
    i32 j2 = 0;

    // === 阶段 1：合法性扫描 ===
    // 地板（l2==-1）与天花板（l2==4）必须为固体；统计边界空格 j2。
    for (i32 k2 = k; k2 <= l; ++k2) {
        for (i32 l2 = l1; l2 <= j1; ++l2) {
            for (i32 i3 = l1z; i3 <= i2; ++i3) {
                const BlockPos pos(x + k2, y + l2, z + i3);
                const BlockState* state = region.getBlockState(pos);
                const bool flag = (state != nullptr) && state->isSolid();
                if (l2 == -1 && !flag) {
                    return false;
                }
                if (l2 == 4 && !flag) {
                    return false;
                }
                if ((k2 == k || k2 == l || i3 == l1z || i3 == i2) && l2 == 0) {
                    const bool hereEmpty = (state == nullptr) || state->isAir();
                    const BlockState* aboveState = region.getBlockState(pos.up());
                    const bool aboveEmpty = (aboveState == nullptr) || aboveState->isAir();
                    if (hereEmpty && aboveEmpty) {
                        ++j2;
                    }
                }
            }
        }
    }

    if (j2 < 1 || j2 > 5) {
        return false;
    }

    // === 阶段 2：建造房间 ===
    // i4 从 3 递减到 -1；边界格按规则放 CAVE_AIR / MOSSY_COBBLESTONE / COBBLESTONE；内部放 AIR。
    for (i32 k3 = k; k3 <= l; ++k3) {
        for (i32 i4 = 3; i4 >= -1; --i4) {
            for (i32 k4 = l1z; k4 <= i2; ++k4) {
                const BlockPos pos(x + k3, y + i4, z + k4);
                const BlockState* blockstate = region.getBlockState(pos);
                const bool isBoundary = (k3 == k || i4 == -1 || k4 == l1z || k3 == l || i4 == 4 || k4 == i2);
                if (isBoundary) {
                    if (pos.y >= region.getMinBuildHeight()) {
                        const BlockState* below = region.getBlockState(pos.down());
                        if (below == nullptr || !below->isSolid()) {
                            const BlockState* air = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
                            safeSetBlock(region, pos, air);
                            continue;
                        }
                    }
                    if (blockstate != nullptr && blockstate->isSolid() && !blockstate->is(VanillaBlocks::CHEST)) {
                        const BlockState* stone = nullptr;
                        if (i4 == -1 && random.nextInt(4) != 0) {
                            stone = VanillaBlocks::getState(VanillaBlocks::MOSSY_COBBLESTONE);
                        } else {
                            stone = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);
                        }
                        safeSetBlock(region, pos, stone);
                    }
                } else if (blockstate != nullptr && !blockstate->is(VanillaBlocks::CHEST) &&
                    !blockstate->is(VanillaBlocks::SPAWNER)) {
                    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
                    safeSetBlock(region, pos, air);
                }
            }
        }
    }

    // === 阶段 3：宝箱 ===
    // 2 轮，每轮 3 次尝试；命中"空格 + 恰 1 个水平固体邻居"则放宝箱并设置战利品表，break。
    for (i32 l3 = 0; l3 < 2; ++l3) {
        for (i32 j4 = 0; j4 < 3; ++j4) {
            const i32 l4 = x + random.nextInt(j * 2 + 1) - j;
            const i32 i5 = y;
            const i32 j5 = z + random.nextInt(k1 * 2 + 1) - k1;
            const BlockPos chestPos(l4, i5, j5);
            const BlockState* here = region.getBlockState(chestPos);
            if (here == nullptr || here->isAir()) {
                i32 solidNeighbors = 0;
                for (Direction dir : Directions::horizontal()) {
                    const BlockState* neighbor = region.getBlockState(chestPos.offset(dir));
                    if (neighbor != nullptr && neighbor->isSolid()) {
                        ++solidNeighbors;
                    }
                }
                if (solidNeighbors == 1) {
                    const BlockState* chestDefault = VanillaBlocks::getState(VanillaBlocks::CHEST);
                    const BlockState* oriented =
                        structure::StructurePiece::reorientChest(region, chestPos, chestDefault);
                    safeSetBlock(region, chestPos, oriented);

                    // 设置战利品表
                    BlockEntity* blockEntity = region.getBlockEntity(chestPos);
                    if (blockEntity != nullptr &&
                        (blockEntity->getType() == BlockEntityType::Chest ||
                            blockEntity->getType() == BlockEntityType::TrappedChest)) {
                        auto* chestEntity = static_cast<blockentity::ChestEntity*>(blockEntity);
                        chestEntity->setLootTable(SIMPLE_DUNGEON_LOOT, random.nextLong());
                    }
                    break;
                }
            }
        }
    }

    // === 阶段 4：刷怪笼 ===
    // origin 处放 SPAWNER，setEntityId 从 MOBS 随机选取。
    {
        const BlockPos origin(x, y, z);
        const BlockState* spawner = VanillaBlocks::getState(VanillaBlocks::SPAWNER);
        safeSetBlock(region, origin, spawner);
        BlockEntity* blockEntity = region.getBlockEntity(origin);
        if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::MobSpawner) {
            auto* spawnerEntity = static_cast<blockentity::MobSpawnerBlockEntity*>(blockEntity);
            spawnerEntity->setEntityId(randomEntityId(random), random);
        } else {
            spdlog::error("Failed to fetch mob spawner entity at ({}, {}, {})", x, y, z);
        }
    }

    return true;
}

bool MonsterRoomFeature::safeSetBlock(WorldGenRegion& region, const BlockPos& pos, const BlockState* state)
{
    if (state == nullptr) {
        return false;
    }
    // 受 FEATURES_CANNOT_REPLACE 标签保护的方块不替换。
    const BlockState* existing = region.getBlockState(pos);
    if (existing != nullptr && BlockTags::FEATURES_CANNOT_REPLACE().contains(*existing)) {
        return false;
    }
    return region.setBlockState(pos.x, pos.y, pos.z, state);
}

ResourceLocation MonsterRoomFeature::randomEntityId(math::Random& random)
{
    // nextInt(MOBS.size()) 取下标
    const i32 idx = random.nextInt(static_cast<i32>(MOBS.size()));
    return MOBS[static_cast<size_t>(idx)];
}

ConfiguredMonsterRoomFeature::ConfiguredMonsterRoomFeature() = default;

bool ConfiguredMonsterRoomFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& pos) const
{
    return m_feature.place(region, random, pos.x, pos.y, pos.z);
}

} // namespace mc::world::gen::feature
