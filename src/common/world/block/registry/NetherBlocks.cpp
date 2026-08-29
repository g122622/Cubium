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

#include "world/block/registry/NetherBlocks.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/HarvestTool.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/blocks/HopperBlock.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/agricultural/MelonPumpkinBlocks.hpp"
#include "world/block/blocks/building/FenceBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/decorative/CampfireBlock.hpp"
#include "world/block/blocks/decorative/LanternBlock.hpp"
#include "world/block/blocks/decorative/TorchBlock.hpp"
#include "world/block/blocks/decorative/WallTorchBlock.hpp"
#include "world/block/blocks/end/ChorusFlowerBlock.hpp"
#include "world/block/blocks/end/ChorusPlantBlock.hpp"
#include "world/block/blocks/end/DragonEggBlock.hpp"
#include "world/block/blocks/end/EndGatewayBlock.hpp"
#include "world/block/blocks/end/EndPortalBlock.hpp"
#include "world/block/blocks/end/EndPortalFrameBlock.hpp"
#include "world/block/blocks/end/EndRodBlock.hpp"
#include "world/block/blocks/functional/BeaconBlock.hpp"
#include "world/block/blocks/functional/BellBlock.hpp"
#include "world/block/blocks/functional/BrewingStandBlock.hpp"
#include "world/block/blocks/functional/LodestoneBlock.hpp"
#include "world/block/blocks/functional/RespawnAnchorBlock.hpp"
#include "world/block/blocks/nether/EnderChestBlock.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/block/blocks/nether/MagmaBlock.hpp"
#include "world/block/blocks/nether/NetherPortalBlock.hpp"
#include "world/block/blocks/nether/NetherRootsBlock.hpp"
#include "world/block/blocks/nether/NetherSproutsBlock.hpp"
#include "world/block/blocks/nether/NetherWartBlock.hpp"
#include "world/block/blocks/nether/NyliumBlock.hpp"
#include "world/block/blocks/nether/SoulFireBlock.hpp"
#include "world/block/blocks/nether/SoulSandBlock.hpp"
#include "world/block/blocks/nether/TwistingVinesBlock.hpp"
#include "world/block/blocks/nether/WeepingVinesBlock.hpp"

namespace mc {
namespace block_registry {

// 下界方块
Block* NetherBlocks::SOUL_SAND = nullptr;
Block* NetherBlocks::SOUL_SOIL = nullptr;
Block* NetherBlocks::BASALT = nullptr;
Block* NetherBlocks::POLISHED_BASALT = nullptr;
Block* NetherBlocks::BLACKSTONE = nullptr;
Block* NetherBlocks::POLISHED_BLACKSTONE = nullptr;
Block* NetherBlocks::CRYING_OBSIDIAN = nullptr;
Block* NetherBlocks::RESPAWN_ANCHOR = nullptr;
Block* NetherBlocks::MAGMA = nullptr;
Block* NetherBlocks::NETHER_WART_BLOCK = nullptr;
Block* NetherBlocks::WARPED_WART_BLOCK = nullptr;
Block* NetherBlocks::FIRE = nullptr;
Block* NetherBlocks::SOUL_FIRE = nullptr;
Block* NetherBlocks::NETHER_WART = nullptr;

// 下界扩展植物方块
Block* NetherBlocks::CRIMSON_STEM = nullptr;
Block* NetherBlocks::WARPED_STEM = nullptr;
Block* NetherBlocks::STRIPPED_CRIMSON_STEM = nullptr;
Block* NetherBlocks::STRIPPED_WARPED_STEM = nullptr;
Block* NetherBlocks::CRIMSON_HYPHAE = nullptr;
Block* NetherBlocks::WARPED_HYPHAE = nullptr;
Block* NetherBlocks::STRIPPED_CRIMSON_HYPHAE = nullptr;
Block* NetherBlocks::STRIPPED_WARPED_HYPHAE = nullptr;
Block* NetherBlocks::CRIMSON_NYLIUM = nullptr;
Block* NetherBlocks::WARPED_NYLIUM = nullptr;
Block* NetherBlocks::SHROOMLIGHT = nullptr;
Block* NetherBlocks::CRIMSON_FUNGUS = nullptr;
Block* NetherBlocks::WARPED_FUNGUS = nullptr;
Block* NetherBlocks::WEEPING_VINES = nullptr;
Block* NetherBlocks::TWISTING_VINES = nullptr;
Block* NetherBlocks::WEEPING_VINES_PLANT = nullptr;
Block* NetherBlocks::TWISTING_VINES_PLANT = nullptr;
Block* NetherBlocks::CRIMSON_ROOTS = nullptr;
Block* NetherBlocks::WARPED_ROOTS = nullptr;
Block* NetherBlocks::NETHER_SPROUTS = nullptr;

// 下界木板及衍生方块
Block* NetherBlocks::CRIMSON_PLANKS = nullptr;
Block* NetherBlocks::WARPED_PLANKS = nullptr;
Block* NetherBlocks::CRIMSON_STAIRS = nullptr;
Block* NetherBlocks::WARPED_STAIRS = nullptr;
Block* NetherBlocks::CRIMSON_SLAB = nullptr;
Block* NetherBlocks::WARPED_SLAB = nullptr;
Block* NetherBlocks::CRIMSON_FENCE = nullptr;
Block* NetherBlocks::WARPED_FENCE = nullptr;

// 灵魂火把
Block* NetherBlocks::SOUL_TORCH = nullptr;
Block* NetherBlocks::SOUL_WALL_TORCH = nullptr;

// 黑石建筑方块
Block* NetherBlocks::BLACKSTONE_STAIRS = nullptr;
Block* NetherBlocks::BLACKSTONE_SLAB = nullptr;
Block* NetherBlocks::BLACKSTONE_WALL = nullptr;
Block* NetherBlocks::POLISHED_BLACKSTONE_BRICKS = nullptr;
Block* NetherBlocks::CRACKED_POLISHED_BLACKSTONE_BRICKS = nullptr;
Block* NetherBlocks::CHISELED_POLISHED_BLACKSTONE = nullptr;
Block* NetherBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS = nullptr;
Block* NetherBlocks::POLISHED_BLACKSTONE_BRICK_SLAB = nullptr;
Block* NetherBlocks::POLISHED_BLACKSTONE_BRICK_WALL = nullptr;
Block* NetherBlocks::POLISHED_BLACKSTONE_STAIRS = nullptr;
Block* NetherBlocks::POLISHED_BLACKSTONE_SLAB = nullptr;
Block* NetherBlocks::POLISHED_BLACKSTONE_WALL = nullptr;
Block* NetherBlocks::GILDED_BLACKSTONE = nullptr;

// 下界砖扩展
Block* NetherBlocks::NETHER_BRICKS = nullptr;
Block* NetherBlocks::RED_NETHER_BRICKS = nullptr;
Block* NetherBlocks::CHISELED_NETHER_BRICKS = nullptr;
Block* NetherBlocks::CRACKED_NETHER_BRICKS = nullptr;
Block* NetherBlocks::NETHER_BRICK_FENCE = nullptr;
Block* NetherBlocks::NETHER_BRICK_STAIRS = nullptr;
Block* NetherBlocks::NETHER_BRICK_SLAB = nullptr;
Block* NetherBlocks::NETHER_BRICK_WALL = nullptr;
Block* NetherBlocks::RED_NETHER_BRICK_STAIRS = nullptr;
Block* NetherBlocks::RED_NETHER_BRICK_SLAB = nullptr;
Block* NetherBlocks::RED_NETHER_BRICK_WALL = nullptr;
Block* NetherBlocks::END_STONE_BRICK_STAIRS = nullptr;
Block* NetherBlocks::END_STONE_BRICK_SLAB = nullptr;
Block* NetherBlocks::END_STONE_BRICK_WALL = nullptr;

// 磁石
Block* NetherBlocks::LODESTONE = nullptr;

// 漏斗和钟
Block* NetherBlocks::HOPPER = nullptr;
Block* NetherBlocks::BELL = nullptr;

// 末地方块
Block* NetherBlocks::END_STONE_BRICKS = nullptr;
Block* NetherBlocks::END_ROD = nullptr;
Block* NetherBlocks::CHORUS_PLANT = nullptr;
Block* NetherBlocks::CHORUS_FLOWER = nullptr;
Block* NetherBlocks::DRAGON_EGG = nullptr;

// 末地传送门系列
Block* NetherBlocks::NETHER_PORTAL = nullptr;
Block* NetherBlocks::END_PORTAL = nullptr;
Block* NetherBlocks::END_PORTAL_FRAME = nullptr;
Block* NetherBlocks::END_GATEWAY = nullptr;
Block* NetherBlocks::BEACON = nullptr;
Block* NetherBlocks::BREWING_STAND = nullptr;
Block* NetherBlocks::ENDER_CHEST = nullptr;
Block* NetherBlocks::LANTERN = nullptr;
Block* NetherBlocks::SOUL_LANTERN = nullptr;
Block* NetherBlocks::CAMPFIRE = nullptr;
Block* NetherBlocks::SOUL_CAMPFIRE = nullptr;
Block* NetherBlocks::JACK_O_LANTERN = nullptr;

void registerNetherBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 下界方块注册
    // ============================================================================

    // 灵魂沙
    // speedFactor=0.4: 实体走在上面会被减速（Block.speedFactor）
    // 配合 MOVEMENT_EFFICIENCY 属性，灵魂疾行附魔可以将减速效果抵消
    // SoulSandBlock: 上方有水源时生成涌流气泡柱（DRAG=false，向上推动），与岩浆块的涡流相反。
    NetherBlocks::SOUL_SAND = &registry.registerBlock<blocks::SoulSandBlock>(
        ResourceLocation("minecraft:soul_sand"), BlockProperties(Material::SAND).hardness(0.5f).speedFactor(0.4f));

    // 灵魂土
    NetherBlocks::SOUL_SOIL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:soul_soil"), BlockProperties(Material::EARTH).hardness(0.5f));

    // 玄武岩
    NetherBlocks::BASALT = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:basalt"), BlockProperties(Material::ROCK).hardness(1.25f).resistance(4.2f));

    // 磨制玄武岩
    NetherBlocks::POLISHED_BASALT =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:polished_basalt"),
            BlockProperties(Material::ROCK).hardness(1.25f).resistance(4.2f));

    // 黑石
    NetherBlocks::BLACKSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blackstone"), BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));

    // 磨制黑石
    NetherBlocks::POLISHED_BLACKSTONE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:polished_blackstone"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f));

    // 哭泣的黑曜石
    NetherBlocks::CRYING_OBSIDIAN = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:crying_obsidian"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f).lightLevel(10));

    // 重生锚 - 不可被活塞推动
    NetherBlocks::RESPAWN_ANCHOR =
        &registry.registerBlock<blocks::RespawnAnchorBlock>(ResourceLocation("minecraft:respawn_anchor"),
            BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f));

    // 火 - 普通火焰
    NetherBlocks::FIRE = &registry.registerBlock<blocks::FireBlock>(ResourceLocation("minecraft:fire"),
        BlockProperties(Material::FIRE).noCollision().hardness(0.0f).lightLevel(15).noLootTable());

    // 灵魂火 - 蓝色火焰，伤害更高
    NetherBlocks::SOUL_FIRE = &registry.registerBlock<blocks::SoulFireBlock>(ResourceLocation("minecraft:soul_fire"),
        BlockProperties(Material::FIRE).noCollision().hardness(0.0f).lightLevel(10).noLootTable());

    // 下界传送门
    NetherBlocks::NETHER_PORTAL =
        &registry.registerBlock<blocks::NetherPortalBlock>(ResourceLocation("minecraft:nether_portal"),
            BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(11).noLootTable());

    // 下界疣 - 作物方块
    NetherBlocks::NETHER_WART = &registry.registerBlock<blocks::NetherWartBlock>(
        ResourceLocation("minecraft:nether_wart"), BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // ============================================================================
    // 下界扩展方块注册（岩浆块、地狱疣块等）
    // ============================================================================

    // 岩浆块 - 发光3级
    // 会站在上面造成伤害，在水中产生气泡柱
    NetherBlocks::MAGMA = &registry.registerBlock<blocks::MagmaBlock>(
        ResourceLocation("minecraft:magma_block"), BlockProperties(Material::ROCK).hardness(0.5f).lightLevel(3));

    // 地狱疣块
    NetherBlocks::NETHER_WART_BLOCK =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:nether_wart_block"),
            BlockProperties(Material::ORGANIC).hardness(1.0f).resistance(1.0f).soundType(BlockSoundTypes::WART));

    // 诡异疣块
    NetherBlocks::WARPED_WART_BLOCK =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:warped_wart_block"),
            BlockProperties(Material::ORGANIC).hardness(1.0f).resistance(1.0f).soundType(BlockSoundTypes::WART));

    // 绯红菌柄
    NetherBlocks::CRIMSON_STEM = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:crimson_stem"), BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 诡异菌柄
    NetherBlocks::WARPED_STEM = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:warped_stem"), BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 去皮绯红菌柄
    NetherBlocks::STRIPPED_CRIMSON_STEM =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_crimson_stem"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 去皮诡异菌柄
    NetherBlocks::STRIPPED_WARPED_STEM =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_warped_stem"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 绯红菌核
    NetherBlocks::CRIMSON_HYPHAE = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:crimson_hyphae"), BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 诡异菌核
    NetherBlocks::WARPED_HYPHAE = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:warped_hyphae"), BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 去皮绯红菌核
    NetherBlocks::STRIPPED_CRIMSON_HYPHAE =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_crimson_hyphae"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 去皮诡异菌核
    NetherBlocks::STRIPPED_WARPED_HYPHAE =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_warped_hyphae"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 绯红菌岩
    NetherBlocks::CRIMSON_NYLIUM = &registry.registerBlock<blocks::NyliumBlock>(
        ResourceLocation("minecraft:crimson_nylium"), BlockProperties(Material::ROCK).hardness(0.4f).resistance(0.4f));

    // 诡异菌岩
    NetherBlocks::WARPED_NYLIUM = &registry.registerBlock<blocks::NyliumBlock>(
        ResourceLocation("minecraft:warped_nylium"), BlockProperties(Material::ROCK).hardness(0.4f).resistance(0.4f));

    // 菌光体
    NetherBlocks::SHROOMLIGHT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:shroomlight"), BlockProperties(Material::EARTH).hardness(1.0f).lightLevel(15));

    // 绯红菌
    NetherBlocks::CRIMSON_FUNGUS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:crimson_fungus"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 诡异菌
    NetherBlocks::WARPED_FUNGUS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:warped_fungus"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 垂泪藤 - 向下生长的藤蔓头部（AGE_0_25）
    NetherBlocks::WEEPING_VINES =
        &registry.registerBlock<blocks::WeepingVinesBlock>(ResourceLocation("minecraft:weeping_vines"),
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 扭曲藤 - 向上生长的藤蔓头部（AGE_0_25）
    NetherBlocks::TWISTING_VINES =
        &registry.registerBlock<blocks::TwistingVinesBlock>(ResourceLocation("minecraft:twisting_vines"),
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 垂泪藤植株 - 垂泪藤的茎部分（不生长）
    NetherBlocks::WEEPING_VINES_PLANT =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:weeping_vines_plant"),
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 扭曲藤植株 - 扭曲藤的茎部分（不生长）
    NetherBlocks::TWISTING_VINES_PLANT =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:twisting_vines_plant"),
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 绯红菌索 - 下界装饰植物
    NetherBlocks::CRIMSON_ROOTS =
        &registry.registerBlock<blocks::NetherRootsBlock>(ResourceLocation("minecraft:crimson_roots"),
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().soundType(BlockSoundTypes::ROOT));

    // 诡异菌索 - 下界装饰植物
    NetherBlocks::WARPED_ROOTS =
        &registry.registerBlock<blocks::NetherRootsBlock>(ResourceLocation("minecraft:warped_roots"),
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().soundType(BlockSoundTypes::ROOT));

    // 下界苗 - 下界矮小装饰植物
    NetherBlocks::NETHER_SPROUTS =
        &registry.registerBlock<blocks::NetherSproutsBlock>(ResourceLocation("minecraft:nether_sprouts"),
            BlockProperties(Material::REPLACEABLE_PLANT)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::NETHER_SPROUT));

    // ============================================================================
    // 下界木板及衍生方块
    // ============================================================================

    // 绯红木板 - 下界木材，不可燃，不免疫岩浆点燃
    NetherBlocks::CRIMSON_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:crimson_planks"),
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(3.0f).soundType(BlockSoundTypes::NETHER_WOOD));

    // 诡异木板 - 下界木材，不可燃，不免疫岩浆点燃
    NetherBlocks::WARPED_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:warped_planks"),
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(3.0f).soundType(BlockSoundTypes::NETHER_WOOD));

    // 绯红楼梯
    NetherBlocks::CRIMSON_STAIRS = &registry.registerBlock<blocks::StairsBlock>(
        ResourceLocation("minecraft:crimson_stairs"),
        NetherBlocks::CRIMSON_PLANKS->defaultState(),
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(3.0f).soundType(BlockSoundTypes::NETHER_WOOD));

    // 诡异楼梯
    NetherBlocks::WARPED_STAIRS = &registry.registerBlock<blocks::StairsBlock>(
        ResourceLocation("minecraft:warped_stairs"),
        NetherBlocks::WARPED_PLANKS->defaultState(),
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(3.0f).soundType(BlockSoundTypes::NETHER_WOOD));

    // 绯红台阶
    NetherBlocks::CRIMSON_SLAB = &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:crimson_slab"),
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(3.0f).soundType(BlockSoundTypes::NETHER_WOOD));

    // 诡异台阶
    NetherBlocks::WARPED_SLAB = &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:warped_slab"),
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(3.0f).soundType(BlockSoundTypes::NETHER_WOOD));

    // 绯红栅栏 - 木质栅栏，可与所有木质栅栏和栅栏门连接
    NetherBlocks::CRIMSON_FENCE = &registry.registerBlock<blocks::FenceBlock>(
        ResourceLocation("minecraft:crimson_fence"),
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(3.0f).soundType(BlockSoundTypes::NETHER_WOOD));

    // 诡异栅栏 - 木质栅栏，可与所有木质栅栏和栅栏门连接
    NetherBlocks::WARPED_FENCE = &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:warped_fence"),
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(3.0f).soundType(BlockSoundTypes::NETHER_WOOD));

    // 灵魂火把 - 发光等级10，蓝色火焰
    NetherBlocks::SOUL_TORCH = &registry.registerBlock<blocks::TorchBlock>(ResourceLocation("minecraft:soul_torch"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(10),
        particle::ParticleTypeId::SoulFireFlame);

    // 墙上的灵魂火把
    NetherBlocks::SOUL_WALL_TORCH =
        &registry.registerBlock<blocks::WallTorchBlock>(ResourceLocation("minecraft:soul_wall_torch"),
            BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(10),
            particle::ParticleTypeId::SoulFireFlame);

    // ============================================================================
    // 黑石建筑方块
    // ============================================================================

    // 黑石楼梯
    NetherBlocks::BLACKSTONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:blackstone_stairs"),
            NetherBlocks::BLACKSTONE->defaultState(),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 黑石台阶
    NetherBlocks::BLACKSTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:blackstone_slab"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 黑石墙
    NetherBlocks::BLACKSTONE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:blackstone_wall"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 磨制黑石砖
    NetherBlocks::POLISHED_BLACKSTONE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:polished_blackstone_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 裂纹磨制黑石砖
    NetherBlocks::CRACKED_POLISHED_BLACKSTONE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cracked_polished_blackstone_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 雕纹磨制黑石
    NetherBlocks::CHISELED_POLISHED_BLACKSTONE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_polished_blackstone"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 磨制黑石砖楼梯
    NetherBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:polished_blackstone_brick_stairs"),
            NetherBlocks::POLISHED_BLACKSTONE_BRICKS->defaultState(),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 磨制黑石砖台阶
    NetherBlocks::POLISHED_BLACKSTONE_BRICK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:polished_blackstone_brick_slab"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 磨制黑石砖墙
    NetherBlocks::POLISHED_BLACKSTONE_BRICK_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:polished_blackstone_brick_wall"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 磨制黑石楼梯
    NetherBlocks::POLISHED_BLACKSTONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:polished_blackstone_stairs"),
            NetherBlocks::POLISHED_BLACKSTONE->defaultState(),
            BlockProperties(Material::ROCK)
                .hardness(2.0f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 磨制黑石台阶
    NetherBlocks::POLISHED_BLACKSTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:polished_blackstone_slab"),
            BlockProperties(Material::ROCK)
                .hardness(2.0f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 磨制黑石墙
    NetherBlocks::POLISHED_BLACKSTONE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:polished_blackstone_wall"),
            BlockProperties(Material::ROCK)
                .hardness(2.0f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 镶金黑石 - 可掉落金粒
    NetherBlocks::GILDED_BLACKSTONE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:gilded_blackstone"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool()
                .soundType(BlockSoundTypes::GILDED_BLACKSTONE));

    // ============================================================================
    // 下界砖扩展
    // ============================================================================

    // 下界砖
    NetherBlocks::NETHER_BRICKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:nether_bricks"),
        BlockProperties(Material::ROCK)
            .hardness(2.0f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool());

    // 红色下界砖
    NetherBlocks::RED_NETHER_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:red_nether_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(2.0f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 雕纹下界砖
    NetherBlocks::CHISELED_NETHER_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_nether_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(2.0f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 裂纹下界砖
    NetherBlocks::CRACKED_NETHER_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cracked_nether_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(2.0f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 下界砖栅栏 - 使用 FenceBlock，与木质栅栏同类但材质为石头，不可燃
    // 只能与下界砖栅栏互连（不能与木质栅栏连接），通过 FENCES/WOODEN_FENCES 标签区分
    NetherBlocks::NETHER_BRICK_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:nether_brick_fence"),
            BlockProperties(Material::ROCK)
                .hardness(2.0f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .requiresTool());

    // 下界砖楼梯、台阶、墙
    BlockProperties netherBrickVariantProps = BlockProperties(Material::ROCK)
                                                  .hardness(2.0f)
                                                  .resistance(6.0f)
                                                  .harvestTool(HarvestTool::Pickaxe)
                                                  .requiresTool();

    NetherBlocks::NETHER_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:nether_brick_stairs"),
            NetherBlocks::NETHER_BRICKS->defaultState(),
            netherBrickVariantProps);

    NetherBlocks::NETHER_BRICK_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:nether_brick_slab"), netherBrickVariantProps);

    NetherBlocks::NETHER_BRICK_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:nether_brick_wall"), netherBrickVariantProps);

    // 红色下界砖楼梯、台阶、墙
    BlockProperties redNetherBrickVariantProps = BlockProperties(Material::ROCK)
                                                     .hardness(2.0f)
                                                     .resistance(6.0f)
                                                     .harvestTool(HarvestTool::Pickaxe)
                                                     .requiresTool();

    NetherBlocks::RED_NETHER_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:red_nether_brick_stairs"),
            NetherBlocks::RED_NETHER_BRICKS->defaultState(),
            redNetherBrickVariantProps);

    NetherBlocks::RED_NETHER_BRICK_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:red_nether_brick_slab"), redNetherBrickVariantProps);

    NetherBlocks::RED_NETHER_BRICK_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:red_nether_brick_wall"), redNetherBrickVariantProps);

    // 末地石砖
    NetherBlocks::END_STONE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:end_stone_bricks"),
            BlockProperties(Material::ROCK).hardness(3.0f).resistance(9.0f));

    // 末地石砖楼梯、台阶、墙
    BlockProperties endStoneBrickVariantProps = BlockProperties(Material::ROCK)
                                                    .hardness(3.0f)
                                                    .resistance(9.0f)
                                                    .harvestTool(HarvestTool::Pickaxe)
                                                    .requiresTool();

    NetherBlocks::END_STONE_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:end_stone_brick_stairs"),
            NetherBlocks::END_STONE_BRICKS->defaultState(),
            endStoneBrickVariantProps);

    NetherBlocks::END_STONE_BRICK_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:end_stone_brick_slab"), endStoneBrickVariantProps);

    NetherBlocks::END_STONE_BRICK_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:end_stone_brick_wall"), endStoneBrickVariantProps);

    // ============================================================================
    // 磁石
    // ============================================================================

    // 磁石 - 可重置指南针指向
    NetherBlocks::LODESTONE = &registry.registerBlock<blocks::LodestoneBlock>(ResourceLocation("minecraft:lodestone"),
        BlockProperties(Material::ROCK)
            .hardness(3.5f)
            .resistance(3.5f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool());

    // ============================================================================
    // 漏斗和钟
    // ============================================================================

    // 漏斗 - 物品传输方块
    NetherBlocks::HOPPER = &registry.registerBlock<blocks::HopperBlock>(ResourceLocation("minecraft:hopper"),
        BlockProperties(Material::IRON)
            .hardness(3.0f)
            .resistance(4.8f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool());

    // 钟 - 村庄报时方块
    NetherBlocks::BELL = &registry.registerBlock<blocks::BellBlock>(ResourceLocation("minecraft:bell"),
        BlockProperties(Material::IRON)
            .hardness(5.0f)
            .resistance(5.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool());

    // ============================================================================
    // 末地方块注册
    // ============================================================================

    // 末地烛 - 发光14级，6 向朝向
    NetherBlocks::END_ROD = &registry.registerBlock<blocks::EndRodBlock>(
        ResourceLocation("minecraft:end_rod"), BlockProperties(Material::DECORATION).noCollision().lightLevel(14));

    // 末地传送门 - 穿越后传送到末地
    NetherBlocks::END_PORTAL = &registry.registerBlock<blocks::EndPortalBlock>(ResourceLocation("minecraft:end_portal"),
        BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(15).noLootTable());

    // 末地传送门框架 - 放置末影之眼激活传送门
    NetherBlocks::END_PORTAL_FRAME =
        &registry.registerBlock<blocks::EndPortalFrameBlock>(ResourceLocation("minecraft:end_portal_frame"),
            BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).lightLevel(1).noLootTable());

    // 末地折跃门 - 在末地之间传送
    NetherBlocks::END_GATEWAY =
        &registry.registerBlock<blocks::EndGatewayBlock>(ResourceLocation("minecraft:end_gateway"),
            BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(15).noLootTable());

    // 紫颂植物 - 末地植物
    NetherBlocks::CHORUS_PLANT = &registry.registerBlock<blocks::ChorusPlantBlock>(
        ResourceLocation("minecraft:chorus_plant"), BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 紫颂花 - 紫颂植物的顶部
    NetherBlocks::CHORUS_FLOWER = &registry.registerBlock<blocks::ChorusFlowerBlock>(
        ResourceLocation("minecraft:chorus_flower"), BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 龙蛋 - 末影龙掉落物
    NetherBlocks::DRAGON_EGG = &registry.registerBlock<blocks::DragonEggBlock>(
        ResourceLocation("minecraft:dragon_egg"), BlockProperties(Material::ROCK).hardness(3.0f).lightLevel(1));

    // 信标 - 发光15级（通过 getLightLevel）
    NetherBlocks::BEACON = &registry.registerBlock<blocks::BeaconBlock>(
        ResourceLocation("minecraft:beacon"), BlockProperties(Material::GLASS).hardness(3.0f));

    // 酿造台 - 发光1级（通过 getLightLevel）
    NetherBlocks::BREWING_STAND = &registry.registerBlock<blocks::BrewingStandBlock>(
        ResourceLocation("minecraft:brewing_stand"), BlockProperties(Material::IRON).hardness(0.5f));

    // 末影箱 - 发光7级
    // 右键打开末影箱界面（物品存储在玩家数据中），记录统计 OPEN_ENDERCHEST。
    // 方块上方有红石导体时无法打开。支持含水放置和水平朝向。
    NetherBlocks::ENDER_CHEST =
        &registry.registerBlock<blocks::EnderChestBlock>(ResourceLocation("minecraft:ender_chest"),
            BlockProperties(Material::ROCK).hardness(22.5f).resistance(600.0f).lightLevel(7).notSolid());

    // 灯笼 - 发光15级（通过构造函数参数）
    NetherBlocks::LANTERN = &registry.registerBlock<blocks::LanternBlock>(ResourceLocation("minecraft:lantern"),
        BlockProperties(Material::IRON).hardness(3.5f).resistance(3.5f),
        15 // 光照等级
    );

    // 灵魂灯笼 - 发光10级（通过构造函数参数）
    NetherBlocks::SOUL_LANTERN =
        &registry.registerBlock<blocks::LanternBlock>(ResourceLocation("minecraft:soul_lantern"),
            BlockProperties(Material::IRON).hardness(3.5f).resistance(3.5f),
            10 // 光照等级
        );

    // 营火 - 发光15级（点燃时，通过 getLightLevel 动态计算）。可被岩浆点燃（对齐 vanilla）
    NetherBlocks::CAMPFIRE = &registry.registerBlock<blocks::CampfireBlock>(ResourceLocation("minecraft:campfire"),
        BlockProperties(Material::WOOD).hardness(2.0f).ignitedByLava(),
        15 // 点燃时光照等级
    );

    // 灵魂营火 - 发光10级（点燃时，通过 getLightLevel 动态计算）。可被岩浆点燃（对齐 vanilla）
    NetherBlocks::SOUL_CAMPFIRE = &registry.registerBlock<blocks::SoulCampfireBlock>(
        ResourceLocation("minecraft:soul_campfire"), BlockProperties(Material::WOOD).hardness(2.0f).ignitedByLava());

    // 南瓜灯 - 发光15级，支持 FACING 属性和傀儡生成
    NetherBlocks::JACK_O_LANTERN = &registry.registerBlock<blocks::JackOLanternBlock>(
        ResourceLocation("minecraft:jack_o_lantern"), BlockProperties(Material::EARTH).hardness(1.0f).lightLevel(15));
}

} // namespace block_registry
} // namespace mc
