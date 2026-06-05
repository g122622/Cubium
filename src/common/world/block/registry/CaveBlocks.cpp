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

#include "world/block/registry/CaveBlocks.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/SimpleBlock.hpp"

namespace mc {
namespace block_registry {

// 紫水晶系列
Block* CaveBlocks::AMETHYST_BLOCK = nullptr;
Block* CaveBlocks::BUDDING_AMETHYST = nullptr;
Block* CaveBlocks::SMALL_AMETHYST_BUD = nullptr;
Block* CaveBlocks::MEDIUM_AMETHYST_BUD = nullptr;
Block* CaveBlocks::LARGE_AMETHYST_BUD = nullptr;
Block* CaveBlocks::AMETHYST_CLUSTER = nullptr;

// 滴水石系列
Block* CaveBlocks::DRIPSTONE_BLOCK = nullptr;
Block* CaveBlocks::POINTED_DRIPSTONE = nullptr;

// 方解石
Block* CaveBlocks::CALCITE = nullptr;

// 遮光玻璃
Block* CaveBlocks::TINTED_GLASS = nullptr;

// 苔藓系列
Block* CaveBlocks::MOSS_BLOCK = nullptr;
Block* CaveBlocks::MOSS_CARPET = nullptr;

// 杜鹃花系列
Block* CaveBlocks::AZALEA = nullptr;
Block* CaveBlocks::FLOWERING_AZALEA = nullptr;
Block* CaveBlocks::AZALEA_LEAVES = nullptr;
Block* CaveBlocks::FLOWERING_AZALEA_LEAVES = nullptr;

// 大垂滴叶系列
Block* CaveBlocks::BIG_DRIPLEAF = nullptr;
Block* CaveBlocks::BIG_DRIPLEAF_STEM = nullptr;
Block* CaveBlocks::SMALL_DRIPLEAF = nullptr;

// 其他洞穴方块
Block* CaveBlocks::HANGING_ROOTS = nullptr;
Block* CaveBlocks::ROOTED_DIRT = nullptr;
Block* CaveBlocks::SPORE_BLOSSOM = nullptr;
Block* CaveBlocks::GLOW_LICHEN = nullptr;
Block* CaveBlocks::CAVE_VINES = nullptr;
Block* CaveBlocks::CAVE_VINES_PLANT = nullptr;
Block* CaveBlocks::POWDER_SNOW = nullptr;

void registerCaveBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 紫水晶系列
    // ============================================================================

    // 紫水晶块 - 由紫水晶碎片合成
    CaveBlocks::AMETHYST_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:amethyst_block"),
        BlockProperties(Material::GLASS)
            .hardness(1.5f)
            .resistance(1.5f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::AMETHYST));

    // 生成紫水晶的方块 - 随机刻会生长紫水晶簇
    CaveBlocks::BUDDING_AMETHYST = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:budding_amethyst"),
        BlockProperties(Material::GLASS)
            .hardness(1.5f)
            .resistance(1.5f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::AMETHYST)
            .tickRandomly());

    // 小紫水晶芽 - 发光等级1
    CaveBlocks::SMALL_AMETHYST_BUD =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:small_amethyst_bud"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::AMETHYST_CLUSTER)
                .lightLevel(1));

    // 中紫水晶芽 - 发光等级2
    CaveBlocks::MEDIUM_AMETHYST_BUD =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:medium_amethyst_bud"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::AMETHYST_CLUSTER)
                .lightLevel(2));

    // 大紫水晶芽 - 发光等级4
    CaveBlocks::LARGE_AMETHYST_BUD =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:large_amethyst_bud"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::AMETHYST_CLUSTER)
                .lightLevel(4));

    // 紫水晶簇 - 发光等级5，可被破坏获得紫水晶碎片
    CaveBlocks::AMETHYST_CLUSTER = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:amethyst_cluster"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(1.5f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::AMETHYST_CLUSTER)
            .lightLevel(5));

    // ============================================================================
    // 滴水石系列
    // ============================================================================

    // 滴水石块 - 由滴水石锥合成
    CaveBlocks::DRIPSTONE_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:dripstone_block"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(1.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::DRIPSTONE_BLOCK));

    // 滴水石锥 - 从钟乳石落下的石锥，可刺穿实体
    CaveBlocks::POINTED_DRIPSTONE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pointed_dripstone"),
            BlockProperties(Material::ROCK)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::POINTED_DRIPSTONE));

    // ============================================================================
    // 方解石
    // ============================================================================

    // 方解石 - 紫晶洞中的白色岩石层
    CaveBlocks::CALCITE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:calcite"),
        BlockProperties(Material::ROCK)
            .hardness(0.75f)
            .resistance(0.75f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::CALCITE));

    // ============================================================================
    // 遮光玻璃
    // ============================================================================

    // 遮光玻璃 - 不透光但透明的玻璃，由紫水晶碎片和玻璃合成
    CaveBlocks::TINTED_GLASS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:tinted_glass"),
        BlockProperties(Material::GLASS)
            .hardness(0.3f)
            .resistance(0.3f)
            .notSolid()
            .opacity(0)
            .propagatesSkylightDown()
            .soundType(BlockSoundTypes::GLASS));

    // ============================================================================
    // 苔藓系列
    // ============================================================================

    // 苔藓块 - 可使用骨粉催生周围苔藓地毯和植物
    CaveBlocks::MOSS_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:moss_block"),
        BlockProperties(Material::MOSS)
            .hardness(0.1f)
            .resistance(0.1f)
            .harvestTool(HarvestTool::Shovel)
            .soundType(BlockSoundTypes::MOSS));

    // 苔藓地毯 - 地毯类方块
    CaveBlocks::MOSS_CARPET = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:moss_carpet"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::MOSS_CARPET));

    // ============================================================================
    // 杜鹃花系列
    // ============================================================================

    // 杜鹃花 - 灌木类方块
    CaveBlocks::AZALEA = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:azalea"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::AZALEA));

    // 开花的杜鹃花 - 灌木类方块
    CaveBlocks::FLOWERING_AZALEA = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:flowering_azalea"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::FLOWERING_AZALEA));

    // 杜鹃花叶 - 树叶类方块，随机刻可枯萎消失
    CaveBlocks::AZALEA_LEAVES = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:azalea_leaves"),
        BlockProperties(Material::LEAVES)
            .hardness(0.2f)
            .resistance(0.2f)
            .soundType(BlockSoundTypes::AZALEA_LEAVES)
            .tickRandomly());

    // 开花的杜鹃花叶 - 树叶类方块，随机刻可枯萎消失
    CaveBlocks::FLOWERING_AZALEA_LEAVES =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:flowering_azalea_leaves"),
            BlockProperties(Material::LEAVES)
                .hardness(0.2f)
                .resistance(0.2f)
                .soundType(BlockSoundTypes::AZALEA_LEAVES)
                .tickRandomly());

    // ============================================================================
    // 大垂滴叶系列
    // ============================================================================

    // 大垂滴叶 - 可站立的倾斜叶片
    CaveBlocks::BIG_DRIPLEAF = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:big_dripleaf"),
        BlockProperties(Material::PLANT).hardness(0.1f).resistance(0.1f).soundType(BlockSoundTypes::BIG_DRIPLEAF));

    // 大垂滴叶茎 - 支撑茎，无碰撞
    CaveBlocks::BIG_DRIPLEAF_STEM =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:big_dripleaf_stem"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.1f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::BIG_DRIPLEAF));

    // 小垂滴叶 - 装饰植物，可放置在粘土上
    CaveBlocks::SMALL_DRIPLEAF = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:small_dripleaf"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::SMALL_DRIPLEAF));

    // ============================================================================
    // 其他洞穴方块
    // ============================================================================

    // 垂根 - 悬挂的根系装饰
    CaveBlocks::HANGING_ROOTS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:hanging_roots"),
        BlockProperties(Material::REPLACEABLE_PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::HANGING_ROOTS));

    // 缠根泥土 - 骨肥可催生垂根，使用锄可快速挖掘
    CaveBlocks::ROOTED_DIRT = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:rooted_dirt"),
        BlockProperties(Material::EARTH).hardness(0.5f).resistance(0.5f).soundType(BlockSoundTypes::ROOTED_DIRT));

    // 孢子花 - 天花板装饰植物，散发绿色粒子
    CaveBlocks::SPORE_BLOSSOM = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:spore_blossom"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::SPORE_BLOSSOM));

    // 发光地衣 - 可放置在任意面的发光苔藓类植物
    CaveBlocks::GLOW_LICHEN = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:glow_lichen"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.2f)
            .resistance(0.2f)
            .soundType(BlockSoundTypes::GLOW_LICHEN)
            .lightLevel(7));

    // 洞穴藤蔓 - 可长出发光浆果的藤蔓
    CaveBlocks::CAVE_VINES = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cave_vines"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::CAVE_VINES)
            .lightLevel(14));

    // 洞穴藤蔓植株 - 洞穴藤蔓的茎部分
    CaveBlocks::CAVE_VINES_PLANT = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cave_vines_plant"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::CAVE_VINES));

    // 粉雪 - 可替换的雪类方块，实体会陷入其中
    CaveBlocks::POWDER_SNOW = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:powder_snow"),
        BlockProperties(Material::POWDER_SNOW)
            .noCollision()
            .notSolid()
            .replaceable()
            .hardness(0.25f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::POWDER_SNOW));
}

} // namespace block_registry
} // namespace mc
