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

#include "world/block/registry/PaleGardenBlocks.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/DoorBlock.hpp"
#include "world/block/blocks/FenceGateBlock.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/building/FenceBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/TrapDoorBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/pale_garden/CreakingHeartBlock.hpp"
#include "world/block/blocks/pale_garden/EyeblossomBlock.hpp"
#include "world/block/blocks/pale_garden/MossyCarpetBlock.hpp"
#include "world/block/blocks/pale_garden/PaleHangingMossBlock.hpp"
#include "world/block/blocks/vegetation/LeavesBlock.hpp"
#include "world/block/blocks/vegetation/SaplingBlock.hpp"
#include "world/block/blocks/vegetation/TreeGenerators.hpp"

namespace mc {
namespace block_registry {

// 苍白橡木原木和木材
Block* PaleGardenBlocks::PALE_OAK_LOG = nullptr;
Block* PaleGardenBlocks::PALE_OAK_WOOD = nullptr;
Block* PaleGardenBlocks::STRIPPED_PALE_OAK_LOG = nullptr;
Block* PaleGardenBlocks::STRIPPED_PALE_OAK_WOOD = nullptr;

// 苍白橡木木板和树叶
Block* PaleGardenBlocks::PALE_OAK_PLANKS = nullptr;
Block* PaleGardenBlocks::PALE_OAK_LEAVES = nullptr;
Block* PaleGardenBlocks::PALE_OAK_SAPLING = nullptr;

// 苍白橡木建筑方块
Block* PaleGardenBlocks::PALE_OAK_STAIRS = nullptr;
Block* PaleGardenBlocks::PALE_OAK_SLAB = nullptr;
Block* PaleGardenBlocks::PALE_OAK_FENCE = nullptr;
Block* PaleGardenBlocks::PALE_OAK_FENCE_GATE = nullptr;
Block* PaleGardenBlocks::PALE_OAK_DOOR = nullptr;
Block* PaleGardenBlocks::PALE_OAK_TRAPDOOR = nullptr;

// 苍白苔藓
Block* PaleGardenBlocks::PALE_MOSS_BLOCK = nullptr;
Block* PaleGardenBlocks::PALE_MOSS_CARPET = nullptr;
Block* PaleGardenBlocks::PALE_HANGING_MOSS = nullptr;

// 眼眸花
Block* PaleGardenBlocks::OPEN_EYEBLOSSOM = nullptr;
Block* PaleGardenBlocks::CLOSED_EYEBLOSSOM = nullptr;

// 嘎枝之心
Block* PaleGardenBlocks::CREAKING_HEART = nullptr;

// 树脂系列
Block* PaleGardenBlocks::RESIN_CLUMP = nullptr;
Block* PaleGardenBlocks::RESIN_BLOCK = nullptr;
Block* PaleGardenBlocks::RESIN_BRICKS = nullptr;
Block* PaleGardenBlocks::CHISELED_RESIN_BRICKS = nullptr;
Block* PaleGardenBlocks::RESIN_BRICK_STAIRS = nullptr;
Block* PaleGardenBlocks::RESIN_BRICK_SLAB = nullptr;
Block* PaleGardenBlocks::RESIN_BRICK_WALL = nullptr;

void registerPaleGardenBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 苍白橡木原木和木材
    // ============================================================================

    // 苍白橡木原木
    PaleGardenBlocks::PALE_OAK_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:pale_oak_log"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(2.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .flammable()
                .ignitedByLava());

    // 苍白橡木木材（全皮）
    PaleGardenBlocks::PALE_OAK_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:pale_oak_wood"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(2.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .flammable()
                .ignitedByLava());

    // 去皮苍白橡木原木
    PaleGardenBlocks::STRIPPED_PALE_OAK_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_pale_oak_log"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(2.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .flammable()
                .ignitedByLava());

    // 去皮苍白橡木木材
    PaleGardenBlocks::STRIPPED_PALE_OAK_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_pale_oak_wood"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(2.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 苍白橡木木板和树叶
    // ============================================================================

    // 苍白橡木木板
    PaleGardenBlocks::PALE_OAK_PLANKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pale_oak_planks"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .flammable()
                .ignitedByLava());

    // 苍白橡树树叶
    PaleGardenBlocks::PALE_OAK_LEAVES =
        &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:pale_oak_leaves"),
            BlockProperties(Material::LEAVES)
                .hardness(0.2f)
                .resistance(0.2f)
                .harvestTool(HarvestTool::Hoe)
                .soundType(BlockSoundTypes::LEAVES));

    // 苍白橡树树苗（带 stage 属性，可生长成苍白橡树）
    PaleGardenBlocks::PALE_OAK_SAPLING =
        &registry.registerBlock<blocks::SaplingBlock>(ResourceLocation("minecraft:pale_oak_sapling"),
            blocks::TreeGenerators::paleOakTree(),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::GRASS));

    // ============================================================================
    // 苍白橡木建筑方块
    // ============================================================================

    // 苍白橡木楼梯
    PaleGardenBlocks::PALE_OAK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:pale_oak_stairs"),
            PaleGardenBlocks::PALE_OAK_PLANKS->defaultState(),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .flammable()
                .ignitedByLava());

    // 苍白橡木台阶
    PaleGardenBlocks::PALE_OAK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:pale_oak_slab"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .flammable()
                .ignitedByLava());

    // 苍白橡木栅栏
    PaleGardenBlocks::PALE_OAK_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:pale_oak_fence"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .flammable()
                .ignitedByLava());

    // 苍白橡木栅栏门
    PaleGardenBlocks::PALE_OAK_FENCE_GATE =
        &registry.registerBlock<blocks::FenceGateBlock>(ResourceLocation("minecraft:pale_oak_fence_gate"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .notSolid()
                .flammable()
                .ignitedByLava());

    // 苍白橡木门
    PaleGardenBlocks::PALE_OAK_DOOR =
        &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:pale_oak_door"),
            BlockProperties(Material::WOOD)
                .hardness(3.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .notSolid()
                .flammable()
                .ignitedByLava(),
            false // isIron = false，木门可手动开关
        );

    // 苍白橡木活板门
    PaleGardenBlocks::PALE_OAK_TRAPDOOR =
        &registry.registerBlock<blocks::TrapDoorBlock>(ResourceLocation("minecraft:pale_oak_trapdoor"),
            BlockProperties(Material::WOOD)
                .hardness(3.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::WOOD)
                .notSolid()
                .flammable()
                .ignitedByLava(),
            false // isIron = false，木活板门可手动开关
        );

    // ============================================================================
    // 苍白苔藓
    // ============================================================================

    // 苍白苔藓块
    PaleGardenBlocks::PALE_MOSS_BLOCK =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pale_moss_block"),
            BlockProperties(Material::MOSS)
                .hardness(0.1f)
                .resistance(0.1f)
                .harvestTool(HarvestTool::Hoe)
                .soundType(BlockSoundTypes::PALE_MOSS));

    // 苍白苔藓地毯（BOTTOM + 四方向 WALL_HEIGHT 属性，可贴墙贴地）
    PaleGardenBlocks::PALE_MOSS_CARPET =
        &registry.registerBlock<blocks::MossyCarpetBlock>(ResourceLocation("minecraft:pale_moss_carpet"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.1f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::PALE_MOSS));

    // 苍白垂苔 - TIP属性
    PaleGardenBlocks::PALE_HANGING_MOSS =
        &registry.registerBlock<blocks::PaleHangingMossBlock>(ResourceLocation("minecraft:pale_hanging_moss"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .propagatesSkylightDown()
                .soundType(BlockSoundTypes::PALE_HANGING_MOSS));

    // ============================================================================
    // 嘎枝之心
    // ============================================================================

    // 嘎枝之心 - AXIS + CREAKING_HEART_STATE + NATURAL属性
    PaleGardenBlocks::CREAKING_HEART =
        &registry.registerBlock<blocks::CreakingHeartBlock>(ResourceLocation("minecraft:creaking_heart"),
            BlockProperties(Material::WOOD)
                .hardness(10.0f)
                .resistance(1.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::CREAKING_HEART)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 眼眸花
    // ============================================================================

    // 开放的眼眸花 - 发光等级1，迷之炖菜效果：失明 11秒
    PaleGardenBlocks::OPEN_EYEBLOSSOM =
        &registry.registerBlock<blocks::EyeblossomBlock>(ResourceLocation("minecraft:open_eyeblossom"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::EYEBLOSSOM),
            blocks::EyeblossomBlock::Type::Open,
            static_cast<u32>(entity::effect::EffectType::Blindness),
            11);

    // 闭合的眼眸花，迷之炖菜效果：反胃 7秒
    PaleGardenBlocks::CLOSED_EYEBLOSSOM =
        &registry.registerBlock<blocks::EyeblossomBlock>(ResourceLocation("minecraft:closed_eyeblossom"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::EYEBLOSSOM),
            blocks::EyeblossomBlock::Type::Closed,
            static_cast<u32>(entity::effect::EffectType::Nausea),
            7);

    // ============================================================================
    // 树脂系列
    // ============================================================================

    // 树脂块（附着在树上）
    PaleGardenBlocks::RESIN_CLUMP = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:resin_clump"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::RESIN));

    // 树脂块（固体）
    PaleGardenBlocks::RESIN_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:resin_block"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::RESIN));

    // 树脂砖
    PaleGardenBlocks::RESIN_BRICKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:resin_bricks"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::RESIN_BRICKS));

    // 雕刻树脂砖
    PaleGardenBlocks::CHISELED_RESIN_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_resin_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::RESIN_BRICKS));

    // 树脂砖楼梯
    PaleGardenBlocks::RESIN_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:resin_brick_stairs"),
            PaleGardenBlocks::RESIN_BRICKS->defaultState(),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::RESIN_BRICKS));

    // 树脂砖台阶
    PaleGardenBlocks::RESIN_BRICK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:resin_brick_slab"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::RESIN_BRICKS));

    // 树脂砖墙
    PaleGardenBlocks::RESIN_BRICK_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:resin_brick_wall"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::RESIN_BRICKS));
}

} // namespace block_registry
} // namespace mc
