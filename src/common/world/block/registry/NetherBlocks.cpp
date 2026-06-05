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
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/decorative/CampfireBlock.hpp"
#include "world/block/blocks/decorative/LanternBlock.hpp"
#include "world/block/blocks/end/ChorusFlowerBlock.hpp"
#include "world/block/blocks/end/ChorusPlantBlock.hpp"
#include "world/block/blocks/end/DragonEggBlock.hpp"
#include "world/block/blocks/end/EndGatewayBlock.hpp"
#include "world/block/blocks/end/EndPortalBlock.hpp"
#include "world/block/blocks/end/EndPortalFrameBlock.hpp"
#include "world/block/blocks/functional/BeaconBlock.hpp"
#include "world/block/blocks/functional/BrewingStandBlock.hpp"
#include "world/block/blocks/functional/RespawnAnchorBlock.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/block/blocks/nether/MagmaBlock.hpp"
#include "world/block/blocks/nether/NetherPortalBlock.hpp"
#include "world/block/blocks/nether/NetherRootsBlock.hpp"
#include "world/block/blocks/nether/NetherSproutsBlock.hpp"
#include "world/block/blocks/nether/NetherWartBlock.hpp"
#include "world/block/blocks/nether/NyliumBlock.hpp"
#include "world/block/blocks/nether/SoulFireBlock.hpp"

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
Block* NetherBlocks::CRIMSON_ROOTS = nullptr;
Block* NetherBlocks::WARPED_ROOTS = nullptr;
Block* NetherBlocks::NETHER_SPROUTS = nullptr;

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
    SOUL_SAND = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:soul_sand"), BlockProperties(Material::SAND).hardness(0.5f));

    // 灵魂土
    SOUL_SOIL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:soul_soil"), BlockProperties(Material::EARTH).hardness(0.5f));

    // 玄武岩
    BASALT = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:basalt"), BlockProperties(Material::ROCK).hardness(1.25f).resistance(4.2f));

    // 磨制玄武岩
    POLISHED_BASALT = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:polished_basalt"),
        BlockProperties(Material::ROCK).hardness(1.25f).resistance(4.2f));

    // 黑石
    BLACKSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blackstone"), BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));

    // 磨制黑石
    POLISHED_BLACKSTONE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:polished_blackstone"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f));

    // 哭泣的黑曜石
    CRYING_OBSIDIAN = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:crying_obsidian"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f).lightLevel(10));

    // 重生锚 - 不可被活塞推动
    RESPAWN_ANCHOR = &registry.registerBlock<blocks::RespawnAnchorBlock>(ResourceLocation("minecraft:respawn_anchor"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f));

    // 火 - 普通火焰
    FIRE = &registry.registerBlock<blocks::FireBlock>(ResourceLocation("minecraft:fire"),
        BlockProperties(Material::FIRE).noCollision().hardness(0.0f).lightLevel(15).noLootTable());

    // 灵魂火 - 蓝色火焰，伤害更高
    SOUL_FIRE = &registry.registerBlock<blocks::SoulFireBlock>(ResourceLocation("minecraft:soul_fire"),
        BlockProperties(Material::FIRE).noCollision().hardness(0.0f).lightLevel(10).noLootTable());

    // 下界传送门
    NETHER_PORTAL = &registry.registerBlock<blocks::NetherPortalBlock>(ResourceLocation("minecraft:nether_portal"),
        BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(11).noLootTable());

    // 下界疣 - 作物方块
    NETHER_WART = &registry.registerBlock<blocks::NetherWartBlock>(
        ResourceLocation("minecraft:nether_wart"), BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // ============================================================================
    // 下界扩展方块注册（岩浆块、地狱疣块等）
    // ============================================================================

    // 岩浆块 - 发光3级
    // 会站在上面造成伤害，在水中产生气泡柱
    MAGMA = &registry.registerBlock<blocks::MagmaBlock>(
        ResourceLocation("minecraft:magma"), BlockProperties(Material::ROCK).hardness(0.5f).lightLevel(3));

    // 地狱疣块
    NETHER_WART_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:nether_wart_block"),
        BlockProperties(Material::ORGANIC).hardness(1.0f).resistance(1.0f).soundType(BlockSoundTypes::WART));

    // 诡异疣块
    WARPED_WART_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:warped_wart_block"),
        BlockProperties(Material::ORGANIC).hardness(1.0f).resistance(1.0f).soundType(BlockSoundTypes::WART));

    // 绯红菌柄
    CRIMSON_STEM = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:crimson_stem"), BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 诡异菌柄
    WARPED_STEM = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:warped_stem"), BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 去皮绯红菌柄
    STRIPPED_CRIMSON_STEM =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_crimson_stem"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 去皮诡异菌柄
    STRIPPED_WARPED_STEM =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_warped_stem"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 绯红菌核
    CRIMSON_HYPHAE = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:crimson_hyphae"), BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 诡异菌核
    WARPED_HYPHAE = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:warped_hyphae"), BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 去皮绯红菌核
    STRIPPED_CRIMSON_HYPHAE =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_crimson_hyphae"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 去皮诡异菌核
    STRIPPED_WARPED_HYPHAE =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_warped_hyphae"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f));

    // 绯红菌岩
    CRIMSON_NYLIUM = &registry.registerBlock<blocks::NyliumBlock>(
        ResourceLocation("minecraft:crimson_nylium"), BlockProperties(Material::ROCK).hardness(0.4f).resistance(0.4f));

    // 诡异菌岩
    WARPED_NYLIUM = &registry.registerBlock<blocks::NyliumBlock>(
        ResourceLocation("minecraft:warped_nylium"), BlockProperties(Material::ROCK).hardness(0.4f).resistance(0.4f));

    // 菌光体
    SHROOMLIGHT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:shroomlight"), BlockProperties(Material::EARTH).hardness(1.0f).lightLevel(15));

    // 绯红菌
    CRIMSON_FUNGUS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:crimson_fungus"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 诡异菌
    WARPED_FUNGUS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:warped_fungus"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 垂泪藤
    WEEPING_VINES = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:weeping_vines"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 扭曲藤
    TWISTING_VINES = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:twisting_vines"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 绯红菌索 - 下界装饰植物
    CRIMSON_ROOTS = &registry.registerBlock<blocks::NetherRootsBlock>(ResourceLocation("minecraft:crimson_roots"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().soundType(BlockSoundTypes::ROOT));

    // 诡异菌索 - 下界装饰植物
    WARPED_ROOTS = &registry.registerBlock<blocks::NetherRootsBlock>(ResourceLocation("minecraft:warped_roots"),
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().soundType(BlockSoundTypes::ROOT));

    // 下界苗 - 下界矮小装饰植物
    NETHER_SPROUTS = &registry.registerBlock<blocks::NetherSproutsBlock>(ResourceLocation("minecraft:nether_sprouts"),
        BlockProperties(Material::REPLACEABLE_PLANT)
            .noCollision()
            .notSolid()
            .soundType(BlockSoundTypes::NETHER_SPROUT));

    // ============================================================================
    // 末地方块注册
    // ============================================================================

    // 末地石砖属性
    BlockProperties endStoneBrickProps = BlockProperties(Material::ROCK).hardness(3.0f).resistance(9.0f);

    END_STONE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:end_stone_bricks"), endStoneBrickProps);

    // 末地烛 - 发光14级
    END_ROD = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:end_rod"), BlockProperties(Material::DECORATION).noCollision().lightLevel(14));

    // 末地传送门 - 穿越后传送到末地
    END_PORTAL = &registry.registerBlock<blocks::EndPortalBlock>(ResourceLocation("minecraft:end_portal"),
        BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(15).noLootTable());

    // 末地传送门框架 - 放置末影之眼激活传送门
    END_PORTAL_FRAME =
        &registry.registerBlock<blocks::EndPortalFrameBlock>(ResourceLocation("minecraft:end_portal_frame"),
            BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).lightLevel(1).noLootTable());

    // 末地折跃门 - 在末地之间传送
    END_GATEWAY = &registry.registerBlock<blocks::EndGatewayBlock>(ResourceLocation("minecraft:end_gateway"),
        BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(15).noLootTable());

    // 紫颂植物 - 末地植物
    CHORUS_PLANT = &registry.registerBlock<blocks::ChorusPlantBlock>(
        ResourceLocation("minecraft:chorus_plant"), BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 紫颂花 - 紫颂植物的顶部
    CHORUS_FLOWER = &registry.registerBlock<blocks::ChorusFlowerBlock>(
        ResourceLocation("minecraft:chorus_flower"), BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 龙蛋 - 末影龙掉落物
    DRAGON_EGG = &registry.registerBlock<blocks::DragonEggBlock>(
        ResourceLocation("minecraft:dragon_egg"), BlockProperties(Material::ROCK).hardness(3.0f).lightLevel(1));

    // 信标 - 发光15级（通过 getLightLevel）
    BEACON = &registry.registerBlock<blocks::BeaconBlock>(
        ResourceLocation("minecraft:beacon"), BlockProperties(Material::GLASS).hardness(3.0f));

    // 酿造台 - 发光1级（通过 getLightLevel）
    BREWING_STAND = &registry.registerBlock<blocks::BrewingStandBlock>(
        ResourceLocation("minecraft:brewing_stand"), BlockProperties(Material::IRON).hardness(0.5f));

    // 末影箱 - 发光7级
    ENDER_CHEST = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:ender_chest"),
        BlockProperties(Material::ROCK).hardness(22.5f).resistance(600.0f).lightLevel(7));

    // 灯笼 - 发光15级（通过构造函数参数）
    LANTERN = &registry.registerBlock<blocks::LanternBlock>(ResourceLocation("minecraft:lantern"),
        BlockProperties(Material::IRON).hardness(3.5f).resistance(3.5f),
        15 // 光照等级
    );

    // 灵魂灯笼 - 发光10级（通过构造函数参数）
    SOUL_LANTERN = &registry.registerBlock<blocks::LanternBlock>(ResourceLocation("minecraft:soul_lantern"),
        BlockProperties(Material::IRON).hardness(3.5f).resistance(3.5f),
        10 // 光照等级
    );

    // 营火 - 发光15级（点燃时，通过 getLightLevel 动态计算）
    CAMPFIRE = &registry.registerBlock<blocks::CampfireBlock>(ResourceLocation("minecraft:campfire"),
        BlockProperties(Material::WOOD).hardness(2.0f),
        15 // 点燃时光照等级
    );

    // 灵魂营火 - 发光10级（点燃时，通过 getLightLevel 动态计算）
    SOUL_CAMPFIRE = &registry.registerBlock<blocks::SoulCampfireBlock>(
        ResourceLocation("minecraft:soul_campfire"), BlockProperties(Material::WOOD).hardness(2.0f));

    // 南瓜灯 - 发光15级
    JACK_O_LANTERN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:jack_o_lantern"), BlockProperties(Material::EARTH).hardness(1.0f).lightLevel(15));
}

} // namespace block_registry
} // namespace mc
