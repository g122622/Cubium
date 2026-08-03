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

#include "world/block/registry/BambooBlocks.hpp"
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

namespace mc {
namespace block_registry {

// 竹木方块
Block* BambooBlocks::BAMBOO_BLOCK = nullptr;
Block* BambooBlocks::STRIPPED_BAMBOO_BLOCK = nullptr;

// 竹木木板系列
Block* BambooBlocks::BAMBOO_PLANKS = nullptr;
Block* BambooBlocks::BAMBOO_MOSAIC = nullptr;

// 竹木楼梯
Block* BambooBlocks::BAMBOO_STAIRS = nullptr;
Block* BambooBlocks::BAMBOO_MOSAIC_STAIRS = nullptr;

// 竹木台阶
Block* BambooBlocks::BAMBOO_SLAB = nullptr;
Block* BambooBlocks::BAMBOO_MOSAIC_SLAB = nullptr;

// 竹木栅栏和门
Block* BambooBlocks::BAMBOO_FENCE = nullptr;
Block* BambooBlocks::BAMBOO_FENCE_GATE = nullptr;
Block* BambooBlocks::BAMBOO_DOOR = nullptr;
Block* BambooBlocks::BAMBOO_TRAPDOOR = nullptr;

void registerBambooBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 竹木方块
    // ============================================================================

    // 竹块（由竹子合成的方块，有轴属性）
    BambooBlocks::BAMBOO_BLOCK = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:bamboo_block"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(2.0f)
            .soundType(BlockSoundTypes::BAMBOO_WOOD)
            .flammable()
            .ignitedByLava());

    // 去皮竹块
    BambooBlocks::STRIPPED_BAMBOO_BLOCK =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_bamboo_block"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(2.0f)
                .soundType(BlockSoundTypes::BAMBOO_WOOD)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 竹木木板系列
    // ============================================================================

    // 竹木板
    BambooBlocks::BAMBOO_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:bamboo_planks"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::BAMBOO_WOOD)
            .flammable()
            .ignitedByLava());

    // 竹马赛克
    BambooBlocks::BAMBOO_MOSAIC = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:bamboo_mosaic"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::BAMBOO_WOOD)
            .flammable()
            .ignitedByLava());

    // ============================================================================
    // 竹木楼梯
    // ============================================================================

    // 竹楼梯
    BambooBlocks::BAMBOO_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:bamboo_stairs"),
            BambooBlocks::BAMBOO_PLANKS->defaultState(),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::BAMBOO_WOOD)
                .flammable()
                .ignitedByLava());

    // 竹马赛克楼梯
    BambooBlocks::BAMBOO_MOSAIC_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:bamboo_mosaic_stairs"),
            BambooBlocks::BAMBOO_MOSAIC->defaultState(),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::BAMBOO_WOOD)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 竹木台阶
    // ============================================================================

    // 竹台阶
    BambooBlocks::BAMBOO_SLAB = &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:bamboo_slab"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::BAMBOO_WOOD)
            .flammable()
            .ignitedByLava());

    // 竹马赛克台阶
    BambooBlocks::BAMBOO_MOSAIC_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:bamboo_mosaic_slab"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::BAMBOO_WOOD)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 竹木栅栏和门
    // ============================================================================

    // 竹栅栏
    BambooBlocks::BAMBOO_FENCE = &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:bamboo_fence"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::BAMBOO_WOOD)
            .flammable()
            .ignitedByLava());

    // 竹栅栏门
    BambooBlocks::BAMBOO_FENCE_GATE =
        &registry.registerBlock<blocks::FenceGateBlock>(ResourceLocation("minecraft:bamboo_fence_gate"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::BAMBOO_WOOD)
                .notSolid()
                .flammable()
                .ignitedByLava());

    // 竹门
    BambooBlocks::BAMBOO_DOOR = &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:bamboo_door"),
        BlockProperties(Material::WOOD)
            .hardness(3.0f)
            .resistance(3.0f)
            .soundType(BlockSoundTypes::BAMBOO_WOOD)
            .notSolid()
            .flammable()
            .ignitedByLava(),
        false // 不是铁门
    );

    // 竹活板门
    BambooBlocks::BAMBOO_TRAPDOOR =
        &registry.registerBlock<blocks::TrapDoorBlock>(ResourceLocation("minecraft:bamboo_trapdoor"),
            BlockProperties(Material::WOOD)
                .hardness(3.0f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::BAMBOO_WOOD)
                .flammable()
                .ignitedByLava(),
            false // 不是铁活板门
        );
}

} // namespace block_registry
} // namespace mc
