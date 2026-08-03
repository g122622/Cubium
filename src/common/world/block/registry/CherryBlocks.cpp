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
 */

#include "world/block/registry/CherryBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/blocks/DoorBlock.hpp"
#include "world/block/blocks/FenceGateBlock.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/building/FenceBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/TrapDoorBlock.hpp"
#include "world/block/blocks/vegetation/LeavesBlock.hpp"
#include "world/block/blocks/vegetation/SaplingBlock.hpp"
#include "world/block/blocks/vegetation/TreeGenerators.hpp"

namespace mc {
namespace block_registry {

// 樱花原木和木材
Block* CherryBlocks::CHERRY_LOG = nullptr;
Block* CherryBlocks::CHERRY_WOOD = nullptr;
Block* CherryBlocks::STRIPPED_CHERRY_LOG = nullptr;
Block* CherryBlocks::STRIPPED_CHERRY_WOOD = nullptr;

// 樱花木板系列
Block* CherryBlocks::CHERRY_PLANKS = nullptr;
Block* CherryBlocks::CHERRY_STAIRS = nullptr;
Block* CherryBlocks::CHERRY_SLAB = nullptr;
Block* CherryBlocks::CHERRY_FENCE = nullptr;
Block* CherryBlocks::CHERRY_FENCE_GATE = nullptr;
Block* CherryBlocks::CHERRY_DOOR = nullptr;
Block* CherryBlocks::CHERRY_TRAPDOOR = nullptr;

// 樱花植物
Block* CherryBlocks::CHERRY_LEAVES = nullptr;
Block* CherryBlocks::CHERRY_SAPLING = nullptr;

void registerCherryBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 樱花原木和木材
    // ============================================================================

    // 樱花原木
    CherryBlocks::CHERRY_LOG = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:cherry_log"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(2.0f)
            .soundType(BlockSoundTypes::CHERRY_WOOD)
            .flammable()
            .ignitedByLava());

    // 樱花木材
    CherryBlocks::CHERRY_WOOD = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:cherry_wood"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(2.0f)
            .soundType(BlockSoundTypes::CHERRY_WOOD)
            .flammable()
            .ignitedByLava());

    // 去皮樱花原木
    CherryBlocks::STRIPPED_CHERRY_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_cherry_log"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(2.0f)
                .soundType(BlockSoundTypes::CHERRY_WOOD)
                .flammable()
                .ignitedByLava());

    // 去皮樱花木材
    CherryBlocks::STRIPPED_CHERRY_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_cherry_wood"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(2.0f)
                .soundType(BlockSoundTypes::CHERRY_WOOD)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 樱花木板系列
    // ============================================================================

    // 樱花木板
    CherryBlocks::CHERRY_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cherry_planks"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::CHERRY_WOOD)
            .flammable()
            .ignitedByLava());

    // 樱花楼梯
    CherryBlocks::CHERRY_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:cherry_stairs"),
            CherryBlocks::CHERRY_PLANKS->defaultState(),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::CHERRY_WOOD)
                .flammable()
                .ignitedByLava());

    // 樱花台阶
    CherryBlocks::CHERRY_SLAB = &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:cherry_slab"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::CHERRY_WOOD)
            .flammable()
            .ignitedByLava());

    // 樱花栅栏
    CherryBlocks::CHERRY_FENCE = &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:cherry_fence"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::CHERRY_WOOD)
            .flammable()
            .ignitedByLava());

    // 樱花栅栏门
    CherryBlocks::CHERRY_FENCE_GATE =
        &registry.registerBlock<blocks::FenceGateBlock>(ResourceLocation("minecraft:cherry_fence_gate"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::CHERRY_WOOD)
                .notSolid()
                .flammable()
                .ignitedByLava());

    // 樱花门
    CherryBlocks::CHERRY_DOOR = &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:cherry_door"),
        BlockProperties(Material::WOOD)
            .hardness(3.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::CHERRY_WOOD)
            .notSolid()
            .flammable()
            .ignitedByLava(),
        false // 不是铁门
    );

    // 樱花活板门
    CherryBlocks::CHERRY_TRAPDOOR =
        &registry.registerBlock<blocks::TrapDoorBlock>(ResourceLocation("minecraft:cherry_trapdoor"),
            BlockProperties(Material::WOOD)
                .hardness(3.0f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::CHERRY_WOOD)
                .flammable()
                .ignitedByLava(),
            false // 不是铁活板门
        );

    // ============================================================================
    // 樱花植物
    // ============================================================================

    // 樱花树叶
    CherryBlocks::CHERRY_LEAVES = &registry.registerBlock<blocks::LeavesBlock>(
        ResourceLocation("minecraft:cherry_leaves"),
        BlockProperties(Material::LEAVES).hardness(0.2f).resistance(0.2f).soundType(BlockSoundTypes::CHERRY_LEAVES));

    // 樱花树苗
    CherryBlocks::CHERRY_SAPLING =
        &registry.registerBlock<blocks::SaplingBlock>(ResourceLocation("minecraft:cherry_sapling"),
            blocks::TreeGenerators::cherryTree(),
            BlockProperties(Material::REPLACEABLE_PLANT)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CHERRY_SAPLING));
}

} // namespace block_registry
} // namespace mc
