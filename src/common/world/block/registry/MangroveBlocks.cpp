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

#include "world/block/registry/MangroveBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/DoorBlock.hpp"
#include "world/block/blocks/FenceGateBlock.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/building/FenceBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/TrapDoorBlock.hpp"
#include "world/block/blocks/mangrove/MangrovePropaguleBlock.hpp"
#include "world/block/blocks/mangrove/MangroveRootsBlock.hpp"
#include "world/block/blocks/vegetation/LeavesBlock.hpp"

namespace mc {
namespace block_registry {

// 红树林木材系列方块
Block* MangroveBlocks::MANGROVE_LOG = nullptr;
Block* MangroveBlocks::MANGROVE_WOOD = nullptr;
Block* MangroveBlocks::STRIPPED_MANGROVE_LOG = nullptr;
Block* MangroveBlocks::STRIPPED_MANGROVE_WOOD = nullptr;
Block* MangroveBlocks::MANGROVE_PLANKS = nullptr;
Block* MangroveBlocks::MANGROVE_LEAVES = nullptr;
Block* MangroveBlocks::MANGROVE_PROPAGULE = nullptr;
Block* MangroveBlocks::MANGROVE_ROOTS = nullptr;
Block* MangroveBlocks::MUDDY_MANGROVE_ROOTS = nullptr;
Block* MangroveBlocks::MANGROVE_STAIRS = nullptr;
Block* MangroveBlocks::MANGROVE_SLAB = nullptr;
Block* MangroveBlocks::MANGROVE_FENCE = nullptr;
Block* MangroveBlocks::MANGROVE_FENCE_GATE = nullptr;
Block* MangroveBlocks::MANGROVE_DOOR = nullptr;
Block* MangroveBlocks::MANGROVE_TRAPDOOR = nullptr;

void registerMangroveBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 红树林木材系列方块注册（1.19 荒野更新）
    // ============================================================================

    // 红树原木属性 - WOOD材质, 斧有效, 硬度2.0, 抗性2.0, 可燃
    BlockProperties mangroveLogProps = BlockProperties(Material::WOOD)
                                           .hardness(2.0f)
                                           .resistance(2.0f)
                                           .harvestTool(HarvestTool::Axe)
                                           .soundType(BlockSoundTypes::WOOD)
                                           .flammable()
                                           .ignitedByLava();

    // 红树原木 - 有轴属性
    MangroveBlocks::MANGROVE_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:mangrove_log"), mangroveLogProps);

    // 红树木 - 原木的六面 bark 变体，有轴属性
    MangroveBlocks::MANGROVE_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:mangrove_wood"), mangroveLogProps);

    // 剥皮红树原木 - 有轴属性
    MangroveBlocks::STRIPPED_MANGROVE_LOG = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:stripped_mangrove_log"), mangroveLogProps);

    // 剥皮红树木 - 有轴属性
    MangroveBlocks::STRIPPED_MANGROVE_WOOD = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:stripped_mangrove_wood"), mangroveLogProps);

    // 红树木板属性 - WOOD材质, 斧有效, 硬度2.0, 抗性3.0, 可燃
    BlockProperties mangrovePlanksProps = BlockProperties(Material::WOOD)
                                              .hardness(2.0f)
                                              .resistance(3.0f)
                                              .harvestTool(HarvestTool::Axe)
                                              .soundType(BlockSoundTypes::WOOD)
                                              .flammable()
                                              .ignitedByLava();

    // 红树木板 - 基础建筑材料
    MangroveBlocks::MANGROVE_PLANKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:mangrove_planks"), mangrovePlanksProps);

    // 红树树叶属性 - LEAVES材质, 锄有效, 硬度0.2, 抗性0.2, 随机刻, 可被岩浆点燃（对齐 vanilla）
    BlockProperties mangroveLeavesProps = BlockProperties(Material::LEAVES)
                                              .hardness(0.2f)
                                              .resistance(0.2f)
                                              .harvestTool(HarvestTool::Hoe)
                                              .soundType(BlockSoundTypes::LEAVES)
                                              .tickRandomly()
                                              .ignitedByLava();

    // 红树树叶 - 有距离属性，会腐烂
    MangroveBlocks::MANGROVE_LEAVES = &registry.registerBlock<blocks::LeavesBlock>(
        ResourceLocation("minecraft:mangrove_leaves"), mangroveLeavesProps);

    // 红树胎生苗 - AGE_0_4 + HANGING + WATERLOGGED，悬挂时可生长
    MangroveBlocks::MANGROVE_PROPAGULE =
        &registry.registerBlock<blocks::MangrovePropaguleBlock>(ResourceLocation("minecraft:mangrove_propagule"),
            BlockProperties(Material::PLANT).noCollision().notSolid().soundType(BlockSoundTypes::PLANT));

    // 红树根 - WATERLOGGED属性，非固体，可燃
    MangroveBlocks::MANGROVE_ROOTS =
        &registry.registerBlock<blocks::MangroveRootsBlock>(ResourceLocation("minecraft:mangrove_roots"),
            BlockProperties(Material::WOOD)
                .notSolid()
                .hardness(0.7f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::MANGROVE_ROOTS)
                .flammable()
                .ignitedByLava());

    // 沾泥红树根属性 - EARTH材质, 锹有效, 硬度0.7, 抗性0.7
    BlockProperties muddyMangroveRootsProps = BlockProperties(Material::EARTH)
                                                  .hardness(0.7f)
                                                  .resistance(0.7f)
                                                  .harvestTool(HarvestTool::Shovel)
                                                  .soundType(BlockSoundTypes::MUDDY_MANGROVE_ROOTS);

    // 沾泥红树根 - 有轴属性，泥巴和红树根的混合物
    MangroveBlocks::MUDDY_MANGROVE_ROOTS = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:muddy_mangrove_roots"), muddyMangroveRootsProps);

    // ========== 红树木制建筑方块 ==========

    // 楼梯属性 - WOOD材质, 硬度2.0, 抗性3.0, 可燃
    BlockProperties mangroveStairsProps = BlockProperties(Material::WOOD)
                                              .hardness(2.0f)
                                              .resistance(3.0f)
                                              .harvestTool(HarvestTool::Axe)
                                              .flammable()
                                              .ignitedByLava();

    // 红树木楼梯 - 使用红树木板作为源方块
    MangroveBlocks::MANGROVE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:mangrove_stairs"),
            MangroveBlocks::MANGROVE_PLANKS->defaultState(),
            mangroveStairsProps);

    // 台阶属性 - WOOD材质, 硬度2.0, 抗性3.0, 可燃
    BlockProperties mangroveSlabProps = BlockProperties(Material::WOOD)
                                            .hardness(2.0f)
                                            .resistance(3.0f)
                                            .harvestTool(HarvestTool::Axe)
                                            .flammable()
                                            .ignitedByLava();

    // 红树木台阶
    MangroveBlocks::MANGROVE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:mangrove_slab"), mangroveSlabProps);

    // 栅栏属性 - WOOD材质, 硬度2.0, 抗性3.0, 可燃
    BlockProperties mangroveFenceProps = BlockProperties(Material::WOOD)
                                             .hardness(2.0f)
                                             .resistance(3.0f)
                                             .harvestTool(HarvestTool::Axe)
                                             .flammable()
                                             .ignitedByLava();

    // 红树木栅栏
    MangroveBlocks::MANGROVE_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:mangrove_fence"), mangroveFenceProps);

    // 栅栏门属性 - WOOD材质, 非固体, 硬度2.0, 抗性3.0, 可燃
    BlockProperties mangroveFenceGateProps = BlockProperties(Material::WOOD)
                                                 .hardness(2.0f)
                                                 .resistance(3.0f)
                                                 .notSolid()
                                                 .harvestTool(HarvestTool::Axe)
                                                 .flammable()
                                                 .ignitedByLava();

    // 红树木栅栏门
    MangroveBlocks::MANGROVE_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:mangrove_fence_gate"), mangroveFenceGateProps);

    // 门属性 - WOOD材质, 非固体, 硬度3.0, 抗性3.0, 可燃, 非铁门
    BlockProperties mangroveDoorProps = BlockProperties(Material::WOOD)
                                            .hardness(3.0f)
                                            .resistance(3.0f)
                                            .notSolid()
                                            .harvestTool(HarvestTool::Axe)
                                            .flammable()
                                            .ignitedByLava();

    // 红树木门 - isIron = false
    MangroveBlocks::MANGROVE_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:mangrove_door"), mangroveDoorProps, false);

    // 活板门属性 - WOOD材质, 非固体, 硬度3.0, 抗性3.0, 可燃, 非铁活板门
    BlockProperties mangroveTrapdoorProps = BlockProperties(Material::WOOD)
                                                .hardness(3.0f)
                                                .resistance(3.0f)
                                                .notSolid()
                                                .harvestTool(HarvestTool::Axe)
                                                .flammable()
                                                .ignitedByLava();

    // 红树木活板门 - isIron = false
    MangroveBlocks::MANGROVE_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:mangrove_trapdoor"), mangroveTrapdoorProps, false);
}

} // namespace block_registry
} // namespace mc
