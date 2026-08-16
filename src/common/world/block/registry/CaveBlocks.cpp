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

#include "world/block/registry/CaveBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/cave/AmethystBlock.hpp"
#include "world/block/blocks/cave/AmethystClusterBlock.hpp"
#include "world/block/blocks/cave/AzaleaBlock.hpp"
#include "world/block/blocks/cave/BigDripleafBlock.hpp"
#include "world/block/blocks/cave/BigDripleafStemBlock.hpp"
#include "world/block/blocks/cave/BuddingAmethystBlock.hpp"
#include "world/block/blocks/cave/CaveVinesBlock.hpp"
#include "world/block/blocks/cave/CaveVinesPlantBlock.hpp"
#include "world/block/blocks/cave/GlowLichenBlock.hpp"
#include "world/block/blocks/cave/HangingRootsBlock.hpp"
#include "world/block/blocks/cave/MossBlock.hpp"
#include "world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "world/block/blocks/cave/PowderSnowBlock.hpp"
#include "world/block/blocks/cave/RootedDirtBlock.hpp"
#include "world/block/blocks/cave/SmallDripleafBlock.hpp"
#include "world/block/blocks/cave/SporeBlossomBlock.hpp"
#include "world/block/blocks/decorative/CarpetBlock.hpp"
#include "world/block/blocks/vegetation/LeavesBlock.hpp"
#include "world/block/blocks/vegetation/TreeGenerators.hpp"

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

    // 紫水晶块 - 投掷物击中时播放风铃音效
    CaveBlocks::AMETHYST_BLOCK =
        &registry.registerBlock<blocks::AmethystBlock>(ResourceLocation("minecraft:amethyst_block"),
            BlockProperties(Material::GLASS)
                .hardness(1.5f)
                .resistance(1.5f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool()
                .soundType(BlockSoundTypes::AMETHYST));

    // 紫水晶母岩 - 随机刻下有1/5概率生长紫水晶芽
    CaveBlocks::BUDDING_AMETHYST =
        &registry.registerBlock<blocks::BuddingAmethystBlock>(ResourceLocation("minecraft:budding_amethyst"),
            BlockProperties(Material::GLASS)
                .hardness(1.5f)
                .resistance(1.5f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool()
                .soundType(BlockSoundTypes::AMETHYST)
                .tickRandomly());

    // 小紫水晶芽 - 发光等级1，FACING+WATERLOGGED，高度2像素宽度4像素
    CaveBlocks::SMALL_AMETHYST_BUD =
        &registry.registerBlock<blocks::AmethystClusterBlock>(ResourceLocation("minecraft:small_amethyst_bud"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::AMETHYST_CLUSTER)
                .lightLevel(1),
            2.0f,
            4.0f);

    // 中紫水晶芽 - 发光等级2，高度4像素宽度6像素
    CaveBlocks::MEDIUM_AMETHYST_BUD =
        &registry.registerBlock<blocks::AmethystClusterBlock>(ResourceLocation("minecraft:medium_amethyst_bud"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::AMETHYST_CLUSTER)
                .lightLevel(2),
            4.0f,
            6.0f);

    // 大紫水晶芽 - 发光等级4，高度5像素宽度8像素
    CaveBlocks::LARGE_AMETHYST_BUD =
        &registry.registerBlock<blocks::AmethystClusterBlock>(ResourceLocation("minecraft:large_amethyst_bud"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::AMETHYST_CLUSTER)
                .lightLevel(4),
            5.0f,
            8.0f);

    // 紫水晶簇 - 发光等级5，高度7像素宽度9像素
    CaveBlocks::AMETHYST_CLUSTER =
        &registry.registerBlock<blocks::AmethystClusterBlock>(ResourceLocation("minecraft:amethyst_cluster"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(1.5f)
                .resistance(1.5f)
                .soundType(BlockSoundTypes::AMETHYST_CLUSTER)
                .lightLevel(5),
            7.0f,
            9.0f);

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

    // 滴水石锥 - VERTICAL_DIRECTION + DRIPSTONE_THICKNESS + WATERLOGGED
    // 不使用 noCollision()：滴水石锥在原版（Java/基岩）中具有锥形碰撞箱（PointedDripstoneBlock::getShape
    // 已按厚度实现 0.375~0.75 宽、1.0 高的中心柱），实体会被石笋阻挡并落在其上触发 onFallenUpon
    // 石笋摔落伤害。先前误用 noCollision() 使 getCollisionShape 永远返回空，导致实体穿过滴石、
    // onFallenUpon 永不触发（石笋摔落伤害整条链路失效）。notSolid() 保留：滴石不作为通用固体支撑面
    // （isSolidSide 仍为 false），其放置支撑由 isValidPointedDripstonePlacement 专用逻辑判定。
    // Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_滴水石锥.txt（碰撞箱宽度/高度；石笋摔落伤害）
    CaveBlocks::POINTED_DRIPSTONE =
        &registry.registerBlock<blocks::PointedDripstoneBlock>(ResourceLocation("minecraft:pointed_dripstone"),
            BlockProperties(Material::ROCK)
                .notSolid()
                .hardness(1.5f)
                .resistance(3.0f)
                .soundType(BlockSoundTypes::POINTED_DRIPSTONE)
                .tickRandomly());

    // ============================================================================
    // 方解石
    // ============================================================================

    CaveBlocks::CALCITE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:calcite"),
        BlockProperties(Material::ROCK)
            .hardness(0.75f)
            .resistance(0.75f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .soundType(BlockSoundTypes::CALCITE));

    // ============================================================================
    // 遮光玻璃 - 透明但不透光（阻挡所有光线，不传播天空光）
    // ============================================================================

    CaveBlocks::TINTED_GLASS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:tinted_glass"),
        BlockProperties(Material::GLASS).hardness(0.3f).resistance(0.3f).notSolid().soundType(BlockSoundTypes::GLASS));

    // ============================================================================
    // 苔藓系列
    // ============================================================================

    // 苔藓块 - 可使用骨粉催生周围苔藓地毯和植物
    CaveBlocks::MOSS_BLOCK = &registry.registerBlock<blocks::MossBlock>(ResourceLocation("minecraft:moss_block"),
        BlockProperties(Material::MOSS)
            .hardness(0.1f)
            .resistance(0.1f)
            .harvestTool(HarvestTool::Shovel)
            .requiresTool()
            .soundType(BlockSoundTypes::MOSS));

    // 苔藓地毯 - 地毯类方块
    // 用 CarpetBlock（与普通羊毛地毯同类）：getShape 返回 1/16 薄板（SimpleBox，非 FullBlock），
    // 使 Block::propagatesSkylightDown 默认公式得 true，对齐 vanilla MossyCarpetBlock#propagatesSkylightDown=true
    // （苔藓地毯透传天空光，opacity=0）。此前误用 SimpleBlock（getShape=fullBlock）致默认公式得 false、
    // 天空光被错误阻断。
    CaveBlocks::MOSS_CARPET = &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:moss_carpet"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.1f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::MOSS_CARPET));

    // ============================================================================
    // 杜鹃花系列
    // ============================================================================

    // 杜鹃花 - 灌木类方块
    CaveBlocks::AZALEA = &registry.registerBlock<blocks::AzaleaBlock>(ResourceLocation("minecraft:azalea"),
        blocks::TreeGenerators::azaleaTree(),
        BlockProperties(Material::PLANT).hardness(0.0f).resistance(0.0f).soundType(BlockSoundTypes::AZALEA));

    // 开花的杜鹃花
    CaveBlocks::FLOWERING_AZALEA = &registry.registerBlock<blocks::FloweringAzaleaBlock>(
        ResourceLocation("minecraft:flowering_azalea"),
        blocks::TreeGenerators::azaleaTree(),
        BlockProperties(Material::PLANT).hardness(0.0f).resistance(0.0f).soundType(BlockSoundTypes::FLOWERING_AZALEA));

    // 杜鹃花叶 - 树叶类方块，使用LeavesBlock实现距离衰减
    CaveBlocks::AZALEA_LEAVES =
        &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:azalea_leaves"),
            BlockProperties(Material::LEAVES)
                .hardness(0.2f)
                .resistance(0.2f)
                .soundType(BlockSoundTypes::AZALEA_LEAVES)
                .tickRandomly());

    // 开花的杜鹃花叶
    CaveBlocks::FLOWERING_AZALEA_LEAVES =
        &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:flowering_azalea_leaves"),
            BlockProperties(Material::LEAVES)
                .hardness(0.2f)
                .resistance(0.2f)
                .soundType(BlockSoundTypes::AZALEA_LEAVES)
                .tickRandomly());

    // ============================================================================
    // 大垂滴叶系列
    // ============================================================================

    // 大垂滴叶 - HORIZONTAL_FACING + TILT + WATERLOGGED
    CaveBlocks::BIG_DRIPLEAF =
        &registry.registerBlock<blocks::BigDripleafBlock>(ResourceLocation("minecraft:big_dripleaf"),
            BlockProperties(Material::PLANT).hardness(0.1f).resistance(0.1f).soundType(BlockSoundTypes::BIG_DRIPLEAF));

    // 大垂滴叶茎 - HORIZONTAL_FACING + WATERLOGGED，无碰撞
    CaveBlocks::BIG_DRIPLEAF_STEM =
        &registry.registerBlock<blocks::BigDripleafStemBlock>(ResourceLocation("minecraft:big_dripleaf_stem"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.1f)
                .resistance(0.1f)
                .soundType(BlockSoundTypes::BIG_DRIPLEAF));

    // 小垂滴叶 - HORIZONTAL_FACING + DOUBLE_BLOCK_HALF + WATERLOGGED
    CaveBlocks::SMALL_DRIPLEAF =
        &registry.registerBlock<blocks::SmallDripleafBlock>(ResourceLocation("minecraft:small_dripleaf"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::SMALL_DRIPLEAF));

    // ============================================================================
    // 其他洞穴方块
    // ============================================================================

    // 垂根 - 悬挂的根系装饰，WATERLOGGED
    CaveBlocks::HANGING_ROOTS =
        &registry.registerBlock<blocks::HangingRootsBlock>(ResourceLocation("minecraft:hanging_roots"),
            BlockProperties(Material::REPLACEABLE_PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::HANGING_ROOTS));

    // 缠根泥土 - 骨粉可催生垂根
    CaveBlocks::ROOTED_DIRT =
        &registry.registerBlock<blocks::RootedDirtBlock>(ResourceLocation("minecraft:rooted_dirt"),
            BlockProperties(Material::EARTH).hardness(0.5f).resistance(0.5f).soundType(BlockSoundTypes::ROOTED_DIRT));

    // 孢子花 - 天花板装饰植物
    CaveBlocks::SPORE_BLOSSOM =
        &registry.registerBlock<blocks::SporeBlossomBlock>(ResourceLocation("minecraft:spore_blossom"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::SPORE_BLOSSOM));

    // 发光地衣 - 可放置在任意面，6面布尔属性+WATERLOGGED，发光等级7
    CaveBlocks::GLOW_LICHEN =
        &registry.registerBlock<blocks::GlowLichenBlock>(ResourceLocation("minecraft:glow_lichen"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.2f)
                .resistance(0.2f)
                .soundType(BlockSoundTypes::GLOW_LICHEN));

    // 洞穴藤蔓 - AGE_0_25 + BERRIES，有浆果时发光等级14
    CaveBlocks::CAVE_VINES = &registry.registerBlock<blocks::CaveVinesBlock>(ResourceLocation("minecraft:cave_vines"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::CAVE_VINES)
            .tickRandomly());

    // 洞穴藤蔓植株 - BERRIES属性
    CaveBlocks::CAVE_VINES_PLANT =
        &registry.registerBlock<blocks::CaveVinesPlantBlock>(ResourceLocation("minecraft:cave_vines_plant"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::CAVE_VINES));

    // 粉雪 - 无碰撞，实体会陷入
    CaveBlocks::POWDER_SNOW =
        &registry.registerBlock<blocks::PowderSnowBlock>(ResourceLocation("minecraft:powder_snow"),
            BlockProperties(Material::POWDER_SNOW)
                .notSolid()
                .replaceable()
                .hardness(0.25f)
                .resistance(0.25f)
                .soundType(BlockSoundTypes::POWDER_SNOW));
}

} // namespace block_registry
} // namespace mc
