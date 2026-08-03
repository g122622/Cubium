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

#include "world/block/registry/BuildingBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/BlastFurnaceBlock.hpp"
#include "world/block/blocks/BookshelfBlock.hpp"
#include "world/block/blocks/CauldronBlock.hpp"
#include "world/block/blocks/ChestBlock.hpp"
#include "world/block/blocks/EnchantingTableBlock.hpp"
#include "world/block/blocks/FurnaceBlock.hpp"
#include "world/block/blocks/LavaCauldronBlock.hpp"
#include "world/block/blocks/LayeredCauldronBlock.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/ShulkerBoxBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/SmokerBlock.hpp"
#include "world/block/blocks/TrappedChestBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/decorative/ChainBlock.hpp"
#include "world/block/blocks/decorative/LadderBlock.hpp"
#include "world/block/blocks/decorative/PaneBlock.hpp"
#include "world/block/blocks/decorative/ScaffoldingBlock.hpp"
#include "world/block/blocks/functional/AnvilBlock.hpp"
#include "world/block/blocks/functional/BarrelBlock.hpp"
#include "world/block/blocks/functional/CakeBlock.hpp"
#include "world/block/blocks/functional/CartographyTableBlock.hpp"
#include "world/block/blocks/functional/ComposterBlock.hpp"
#include "world/block/blocks/functional/CraftingTableBlock.hpp"
#include "world/block/blocks/functional/FletchingTableBlock.hpp"
#include "world/block/blocks/functional/JukeboxBlock.hpp"
#include "world/block/blocks/functional/LecternBlock.hpp"
#include "world/block/blocks/functional/LoomBlock.hpp"
#include "world/block/blocks/functional/SmithingTableBlock.hpp"
#include "world/block/blocks/mob/InfestedBlock.hpp"
#include "world/block/blocks/redstone/TNTBlock.hpp"
#include "world/block/blocks/special/SpongeBlock.hpp"
#include "world/block/blocks/special/WetSpongeBlock.hpp"
#include "world/block/registry/BaseBlocks.hpp"
#include <optional>

namespace mc {
namespace block_registry {

// 建筑方块
Block* BuildingBlocks::BRICKS = nullptr;
Block* BuildingBlocks::MOSSY_COBBLESTONE = nullptr;
Block* BuildingBlocks::BOOKSHELF = nullptr;
Block* BuildingBlocks::TNT = nullptr;
Block* BuildingBlocks::SPONGE = nullptr;
Block* BuildingBlocks::WET_SPONGE = nullptr;

// 功能方块
Block* BuildingBlocks::CRAFTING_TABLE = nullptr;
Block* BuildingBlocks::FURNACE = nullptr;
Block* BuildingBlocks::BLAST_FURNACE = nullptr;
Block* BuildingBlocks::SMOKER = nullptr;
Block* BuildingBlocks::CAULDRON = nullptr;
Block* BuildingBlocks::WATER_CAULDRON = nullptr;
Block* BuildingBlocks::LAVA_CAULDRON = nullptr;
Block* BuildingBlocks::POWDER_SNOW_CAULDRON = nullptr;
Block* BuildingBlocks::ENCHANTING_TABLE = nullptr;
Block* BuildingBlocks::CHEST = nullptr;
Block* BuildingBlocks::TRAPPED_CHEST = nullptr;
Block* BuildingBlocks::SHULKER_BOX = nullptr;
Block* BuildingBlocks::LOOM = nullptr;
Block* BuildingBlocks::BARREL = nullptr;
Block* BuildingBlocks::CARTOGRAPHY_TABLE = nullptr;
Block* BuildingBlocks::FLETCHING_TABLE = nullptr;
Block* BuildingBlocks::SMITHING_TABLE = nullptr;
Block* BuildingBlocks::COMPOSTER = nullptr;
Block* BuildingBlocks::CAKE = nullptr;
Block* BuildingBlocks::LECTERN = nullptr;
Block* BuildingBlocks::JUKEBOX = nullptr;

// 含水方块
Block* BuildingBlocks::LADDER = nullptr;
Block* BuildingBlocks::CHAIN = nullptr;
Block* BuildingBlocks::SCAFFOLDING = nullptr;
Block* BuildingBlocks::GLASS_PANE = nullptr;
Block* BuildingBlocks::IRON_BARS = nullptr;

// 石砖系列
Block* BuildingBlocks::STONE_BRICKS = nullptr;
Block* BuildingBlocks::MOSSY_STONE_BRICKS = nullptr;
Block* BuildingBlocks::CRACKED_STONE_BRICKS = nullptr;
Block* BuildingBlocks::CHISELED_STONE_BRICKS = nullptr;
Block* BuildingBlocks::STONE_BRICK_STAIRS = nullptr;
Block* BuildingBlocks::STONE_BRICK_SLAB = nullptr;
Block* BuildingBlocks::MOSSY_STONE_BRICK_STAIRS = nullptr;
Block* BuildingBlocks::MOSSY_STONE_BRICK_SLAB = nullptr;
Block* BuildingBlocks::MOSSY_STONE_BRICK_WALL = nullptr;

// 虫蚀方块系列
Block* BuildingBlocks::INFESTED_STONE = nullptr;
Block* BuildingBlocks::INFESTED_COBBLESTONE = nullptr;
Block* BuildingBlocks::INFESTED_STONE_BRICKS = nullptr;
Block* BuildingBlocks::INFESTED_MOSSY_STONE_BRICKS = nullptr;
Block* BuildingBlocks::INFESTED_CRACKED_STONE_BRICKS = nullptr;
Block* BuildingBlocks::INFESTED_CHISELED_STONE_BRICKS = nullptr;

// 石英系列
Block* BuildingBlocks::QUARTZ_BLOCK = nullptr;
Block* BuildingBlocks::CHISELED_QUARTZ_BLOCK = nullptr;
Block* BuildingBlocks::QUARTZ_PILLAR = nullptr;
Block* BuildingBlocks::SMOOTH_QUARTZ = nullptr;

// 海晶系列
Block* BuildingBlocks::PRISMARINE = nullptr;
Block* BuildingBlocks::PRISMARINE_BRICKS = nullptr;
Block* BuildingBlocks::DARK_PRISMARINE = nullptr;
Block* BuildingBlocks::PRISMARINE_STAIRS = nullptr;
Block* BuildingBlocks::PRISMARINE_BRICK_STAIRS = nullptr;
Block* BuildingBlocks::DARK_PRISMARINE_STAIRS = nullptr;
Block* BuildingBlocks::PRISMARINE_SLAB = nullptr;
Block* BuildingBlocks::PRISMARINE_BRICK_SLAB = nullptr;
Block* BuildingBlocks::DARK_PRISMARINE_SLAB = nullptr;
Block* BuildingBlocks::SEA_LANTERN = nullptr;

// 紫珀系列
Block* BuildingBlocks::PURPUR_BLOCK = nullptr;
Block* BuildingBlocks::PURPUR_PILLAR = nullptr;

// 骨块与干草块
Block* BuildingBlocks::BONE_BLOCK = nullptr;
Block* BuildingBlocks::HAY_BLOCK = nullptr;

// 铁砧系列
Block* BuildingBlocks::ANVIL = nullptr;
Block* BuildingBlocks::CHIPPED_ANVIL = nullptr;
Block* BuildingBlocks::DAMAGED_ANVIL = nullptr;

void registerBuildingBlocks()
{
    auto& registry = BlockRegistry::instance();

    // 砖块
    BuildingBlocks::BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:bricks"), BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f));

    // 苔石圆石
    BuildingBlocks::MOSSY_COBBLESTONE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:mossy_cobblestone"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f));

    // 书架
    BuildingBlocks::BOOKSHELF = &registry.registerBlock<blocks::BookshelfBlock>(ResourceLocation("minecraft:bookshelf"),
        BlockProperties(Material::WOOD).hardness(1.5f).flammable().ignitedByLava());

    // 海绵
    BuildingBlocks::SPONGE = &registry.registerBlock<blocks::SpongeBlock>(
        ResourceLocation("minecraft:sponge"), BlockProperties(Material::SPONGE).hardness(0.6f));

    // 湿海绵
    BuildingBlocks::WET_SPONGE = &registry.registerBlock<blocks::WetSpongeBlock>(
        ResourceLocation("minecraft:wet_sponge"), BlockProperties(Material::SPONGE).hardness(0.6f));

    // 工作台
    // 右键打开3x3合成界面，记录交互统计 INTERACT_WITH_CRAFTING_TABLE。
    // 参考 MC Java: CraftingTableBlock.onBlockActivated() → player.openMenu() + player.awardStat()
    BuildingBlocks::CRAFTING_TABLE =
        &registry.registerBlock<blocks::CraftingTableBlock>(ResourceLocation("minecraft:crafting_table"),
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).flammable().ignitedByLava());

    // 熔炉、高炉、烟熏炉
    // 三者共用相同的方块属性：mapColor(STONE).instrument(BASEDRUM).requiresCorrectToolForDrops()
    //   .strength(3.5F).lightLevel(litBlockEmission(13))
    // 光照等级由 AbstractFurnaceBlock::getLightLevel() 根据 LIT 属性动态返回（13/0）。
    // requiresTool + harvestTool(Pickaxe) 确保必须用镐挖掘才能掉落。
    // 参考 MC 1.21.11: Blocks.FURNACE / BLAST_FURNACE / SMOKER
    BlockProperties furnaceProps = BlockProperties(Material::ROCK)
                                       .hardness(3.5f)
                                       .resistance(3.5f)
                                       .requiresTool()
                                       .harvestTool(HarvestTool::Pickaxe)
                                       .instrument(BlockProperties::Instrument::BaseDrum);

    BuildingBlocks::FURNACE =
        &registry.registerBlock<blocks::FurnaceBlock>(ResourceLocation("minecraft:furnace"), furnaceProps);

    BuildingBlocks::BLAST_FURNACE =
        &registry.registerBlock<blocks::BlastFurnaceBlock>(ResourceLocation("minecraft:blast_furnace"), furnaceProps);

    BuildingBlocks::SMOKER =
        &registry.registerBlock<blocks::SmokerBlock>(ResourceLocation("minecraft:smoker"), furnaceProps);

    // 炼药锅（空）
    BuildingBlocks::CAULDRON = &registry.registerBlock<blocks::CauldronBlock>(ResourceLocation("minecraft:cauldron"),
        BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid());

    // 水炼药锅（分层，水位1-3）
    // 对应 MC 原版 LayeredCauldronBlock (WATER_CAULDRON)
    // 降水类型为 Rain，表示水炼药锅只在雨天被降水填充
    BuildingBlocks::WATER_CAULDRON =
        &registry.registerBlock<blocks::LayeredCauldronBlock>(ResourceLocation("minecraft:water_cauldron"),
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            world::biome::BiomeClimate::Precipitation::Rain);

    // 岩浆炼药锅 - 始终满的炼药锅，发光等级15，实体进入受岩浆伤害
    // 参考: net.minecraft.world.level.block.LavaCauldronBlock
    BuildingBlocks::LAVA_CAULDRON =
        &registry.registerBlock<blocks::LavaCauldronBlock>(ResourceLocation("minecraft:lava_cauldron"),
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid().lightLevel(15));

    // 细雪炼药锅（分层，水位1-3）
    // 对应 MC 原版 LayeredCauldronBlock (POWDER_SNOW_CAULDRON)
    // 降水类型为 Snow，表示细雪炼药锅只在雪天被降水填充
    // 实体着火进入时会将细雪炼药锅转换为水炼药锅
    BuildingBlocks::POWDER_SNOW_CAULDRON =
        &registry.registerBlock<blocks::LayeredCauldronBlock>(ResourceLocation("minecraft:powder_snow_cauldron"),
            BlockProperties(Material::IRON).hardness(2.0f).resistance(2.0f).notSolid(),
            world::biome::BiomeClimate::Precipitation::Snow);

    // 附魔台
    BuildingBlocks::ENCHANTING_TABLE =
        &registry.registerBlock<blocks::EnchantingTableBlock>(ResourceLocation("minecraft:enchanting_table"),
            BlockProperties(Material::ROCK).hardness(5.0f).resistance(1200.0f).notSolid().lightLevel(7));

    // 箱子 - 含水方块
    BuildingBlocks::CHEST = &registry.registerBlock<blocks::ChestBlock>(ResourceLocation("minecraft:chest"),
        BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).notSolid().flammable().ignitedByLava());

    // 梯子 - 含水方块
    BuildingBlocks::LADDER = &registry.registerBlock<blocks::LadderBlock>(ResourceLocation("minecraft:ladder"),
        BlockProperties(Material::WOOD).hardness(0.4f).notSolid().flammable().ignitedByLava());

    // 锁链 - 含水方块 (MC 1.21+ 重命名为 iron_chain)
    BuildingBlocks::CHAIN = &registry.registerBlock<blocks::ChainBlock>(ResourceLocation("minecraft:iron_chain"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f).notSolid());

    // 脚手架 - 含水方块
    BuildingBlocks::SCAFFOLDING = &registry.registerBlock<blocks::ScaffoldingBlock>(
        ResourceLocation("minecraft:scaffolding"), BlockProperties(Material::DECORATION).hardness(0.0f).notSolid());

    // 玻璃板 - 含水方块
    BuildingBlocks::GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:glass_pane"), BlockProperties(Material::GLASS).hardness(0.3f).notSolid());

    // 铁栏杆 - 含水方块
    BuildingBlocks::IRON_BARS = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:iron_bars"), BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f));

    // 陷阱箱 - 含水方块
    BuildingBlocks::TRAPPED_CHEST =
        &registry.registerBlock<blocks::TrappedChestBlock>(ResourceLocation("minecraft:trapped_chest"),
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).notSolid().flammable().ignitedByLava());

    // 潜影盒（无色变体）
    BuildingBlocks::SHULKER_BOX =
        &registry.registerBlock<blocks::ShulkerBoxBlock>(ResourceLocation("minecraft:shulker_box"),
            std::nullopt,
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).notSolid());

    // 织布机
    BuildingBlocks::LOOM = &registry.registerBlock<blocks::LoomBlock>(ResourceLocation("minecraft:loom"),
        BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).flammable().ignitedByLava());

    // 木桶
    BuildingBlocks::BARREL = &registry.registerBlock<blocks::BarrelBlock>(ResourceLocation("minecraft:barrel"),
        BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).flammable().ignitedByLava());

    // 制图台
    BuildingBlocks::CARTOGRAPHY_TABLE =
        &registry.registerBlock<blocks::CartographyTableBlock>(ResourceLocation("minecraft:cartography_table"),
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).flammable().ignitedByLava());

    // 制箭台
    BuildingBlocks::FLETCHING_TABLE =
        &registry.registerBlock<blocks::FletchingTableBlock>(ResourceLocation("minecraft:fletching_table"),
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).flammable().ignitedByLava());

    // 锻造台
    BuildingBlocks::SMITHING_TABLE =
        &registry.registerBlock<blocks::SmithingTableBlock>(ResourceLocation("minecraft:smithing_table"),
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).flammable().ignitedByLava());

    // 堆肥桶
    BuildingBlocks::COMPOSTER = &registry.registerBlock<blocks::ComposterBlock>(ResourceLocation("minecraft:composter"),
        BlockProperties(Material::WOOD).hardness(0.6f).flammable().ignitedByLava());

    // 蛋糕
    BuildingBlocks::CAKE = &registry.registerBlock<blocks::CakeBlock>(
        ResourceLocation("minecraft:cake"), BlockProperties(Material::CAKE).hardness(0.5f).notSolid());

    // 讲台
    BuildingBlocks::LECTERN = &registry.registerBlock<blocks::LecternBlock>(ResourceLocation("minecraft:lectern"),
        BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).flammable().ignitedByLava());

    // 唱片机
    BuildingBlocks::JUKEBOX = &registry.registerBlock<blocks::JukeboxBlock>(ResourceLocation("minecraft:jukebox"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(6.0f).flammable().ignitedByLava());

    // TNT
    BuildingBlocks::TNT = &registry.registerBlock<blocks::TNTBlock>(
        ResourceLocation("minecraft:tnt"), BlockProperties(Material::TNT).hardness(0.0f));

    // ========== 石砖系列 ==========
    BlockProperties stoneBrickProps = BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f);

    BuildingBlocks::STONE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:stone_bricks"), stoneBrickProps);
    BuildingBlocks::MOSSY_STONE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:mossy_stone_bricks"), stoneBrickProps);
    BuildingBlocks::CRACKED_STONE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cracked_stone_bricks"), stoneBrickProps);
    BuildingBlocks::CHISELED_STONE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_stone_bricks"), stoneBrickProps);

    // 石砖楼梯和台阶
    BuildingBlocks::STONE_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:stone_brick_stairs"),
            BuildingBlocks::STONE_BRICKS->defaultState(),
            stoneBrickProps);

    BuildingBlocks::STONE_BRICK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:stone_brick_slab"), stoneBrickProps);

    // 苔藓石砖楼梯、台阶、墙
    BuildingBlocks::MOSSY_STONE_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:mossy_stone_brick_stairs"),
            BuildingBlocks::MOSSY_STONE_BRICKS->defaultState(),
            stoneBrickProps);

    BuildingBlocks::MOSSY_STONE_BRICK_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:mossy_stone_brick_slab"), stoneBrickProps);

    BuildingBlocks::MOSSY_STONE_BRICK_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:mossy_stone_brick_wall"), stoneBrickProps);

    // ========== 虫蚀方块系列 ==========
    BlockProperties infestedProps = BlockProperties(Material::EARTH).hardness(0.0f).resistance(0.75f);

    BuildingBlocks::INFESTED_STONE = &registry.registerBlock<blocks::InfestedBlock>(
        ResourceLocation("minecraft:infested_stone"), BaseBlocks::STONE->blockId(), infestedProps);
    BuildingBlocks::INFESTED_COBBLESTONE = &registry.registerBlock<blocks::InfestedBlock>(
        ResourceLocation("minecraft:infested_cobblestone"), BaseBlocks::COBBLESTONE->blockId(), infestedProps);
    BuildingBlocks::INFESTED_STONE_BRICKS = &registry.registerBlock<blocks::InfestedBlock>(
        ResourceLocation("minecraft:infested_stone_bricks"), BuildingBlocks::STONE_BRICKS->blockId(), infestedProps);
    BuildingBlocks::INFESTED_MOSSY_STONE_BRICKS =
        &registry.registerBlock<blocks::InfestedBlock>(ResourceLocation("minecraft:infested_mossy_stone_bricks"),
            BuildingBlocks::MOSSY_STONE_BRICKS->blockId(),
            infestedProps);
    BuildingBlocks::INFESTED_CRACKED_STONE_BRICKS =
        &registry.registerBlock<blocks::InfestedBlock>(ResourceLocation("minecraft:infested_cracked_stone_bricks"),
            BuildingBlocks::CRACKED_STONE_BRICKS->blockId(),
            infestedProps);
    BuildingBlocks::INFESTED_CHISELED_STONE_BRICKS =
        &registry.registerBlock<blocks::InfestedBlock>(ResourceLocation("minecraft:infested_chiseled_stone_bricks"),
            BuildingBlocks::CHISELED_STONE_BRICKS->blockId(),
            infestedProps);

    // 注册虫蚀方块映射
    blocks::InfestedBlock::registerInfestedBlock(
        BaseBlocks::STONE->blockId(), BuildingBlocks::INFESTED_STONE->blockId());
    blocks::InfestedBlock::registerInfestedBlock(
        BaseBlocks::COBBLESTONE->blockId(), BuildingBlocks::INFESTED_COBBLESTONE->blockId());
    blocks::InfestedBlock::registerInfestedBlock(
        BuildingBlocks::STONE_BRICKS->blockId(), BuildingBlocks::INFESTED_STONE_BRICKS->blockId());
    blocks::InfestedBlock::registerInfestedBlock(
        BuildingBlocks::MOSSY_STONE_BRICKS->blockId(), BuildingBlocks::INFESTED_MOSSY_STONE_BRICKS->blockId());
    blocks::InfestedBlock::registerInfestedBlock(
        BuildingBlocks::CRACKED_STONE_BRICKS->blockId(), BuildingBlocks::INFESTED_CRACKED_STONE_BRICKS->blockId());
    blocks::InfestedBlock::registerInfestedBlock(
        BuildingBlocks::CHISELED_STONE_BRICKS->blockId(), BuildingBlocks::INFESTED_CHISELED_STONE_BRICKS->blockId());

    // 初始化映射表
    blocks::InfestedBlock::initializeMappings();

    // ========== 石英系列 ==========
    BlockProperties quartzProps = BlockProperties(Material::ROCK).hardness(0.8f);

    BuildingBlocks::QUARTZ_BLOCK =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:quartz_block"), quartzProps);
    BuildingBlocks::CHISELED_QUARTZ_BLOCK =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_quartz_block"), quartzProps);

    // 石英柱 - 有轴属性
    BuildingBlocks::QUARTZ_PILLAR =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:quartz_pillar"), quartzProps);

    // 平滑石英
    BuildingBlocks::SMOOTH_QUARTZ = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:smooth_quartz"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // ========== 海晶系列 ==========
    BlockProperties prismarineProps = BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f);

    BuildingBlocks::PRISMARINE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:prismarine"), prismarineProps);
    BuildingBlocks::PRISMARINE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:prismarine_bricks"), prismarineProps);
    BuildingBlocks::DARK_PRISMARINE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:dark_prismarine"), prismarineProps);

    // 海晶楼梯
    BuildingBlocks::PRISMARINE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:prismarine_stairs"),
            BuildingBlocks::PRISMARINE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));
    BuildingBlocks::PRISMARINE_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:prismarine_brick_stairs"),
            BuildingBlocks::PRISMARINE_BRICKS->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));
    BuildingBlocks::DARK_PRISMARINE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:dark_prismarine_stairs"),
            BuildingBlocks::DARK_PRISMARINE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 海晶台阶
    BuildingBlocks::PRISMARINE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:prismarine_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));
    BuildingBlocks::PRISMARINE_BRICK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:prismarine_brick_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));
    BuildingBlocks::DARK_PRISMARINE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:dark_prismarine_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 海晶灯 - 发光15级
    BuildingBlocks::SEA_LANTERN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sea_lantern"), BlockProperties(Material::GLASS).hardness(0.3f).lightLevel(15));

    // ========== 紫珀系列 ==========
    BlockProperties purpurProps = BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f);

    BuildingBlocks::PURPUR_BLOCK =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:purpur_block"), purpurProps);

    // 紫珀柱 - 有轴属性
    BuildingBlocks::PURPUR_PILLAR =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:purpur_pillar"), purpurProps);

    // ========== 骨块和干草块 ==========
    // 骨块 - 有轴属性
    BuildingBlocks::BONE_BLOCK = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:bone_block"), BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));

    // 干草块 - 有轴属性
    BuildingBlocks::HAY_BLOCK = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:hay_block"),
        BlockProperties(Material::EARTH).hardness(0.5f).flammable().ignitedByLava());

    // ========== 铁砧系列 ==========
    // 铁砧属性: 硬度5.0, 爆炸抗性1200.0, 铁砧材质音效
    BlockProperties anvilProps =
        BlockProperties(Material::ANVIL).hardness(5.0f).resistance(1200.0f).soundType(BlockSoundTypes::ANVIL);

    // 铁砧（完好）
    BuildingBlocks::ANVIL =
        &registry.registerBlock<blocks::AnvilBlock>(ResourceLocation("minecraft:anvil"), anvilProps);

    // 铁砧（轻微损坏）
    BuildingBlocks::CHIPPED_ANVIL =
        &registry.registerBlock<blocks::AnvilBlock>(ResourceLocation("minecraft:chipped_anvil"), anvilProps);

    // 铁砧（严重损坏）
    BuildingBlocks::DAMAGED_ANVIL =
        &registry.registerBlock<blocks::AnvilBlock>(ResourceLocation("minecraft:damaged_anvil"), anvilProps);
}

} // namespace block_registry
} // namespace mc
