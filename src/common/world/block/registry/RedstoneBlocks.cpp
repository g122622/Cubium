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

#include "world/block/registry/RedstoneBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/redstone/ActivatorRailBlock.hpp"
#include "world/block/blocks/redstone/DaylightDetectorBlock.hpp"
#include "world/block/blocks/redstone/DetectorRailBlock.hpp"
#include "world/block/blocks/redstone/DispenserBlock.hpp"
#include "world/block/blocks/redstone/DropperBlock.hpp"
#include "world/block/blocks/redstone/LeverBlock.hpp"
#include "world/block/blocks/redstone/MovingPistonBlock.hpp"
#include "world/block/blocks/redstone/NoteBlock.hpp"
#include "world/block/blocks/redstone/ObserverBlock.hpp"
#include "world/block/blocks/redstone/PistonBlock.hpp"
#include "world/block/blocks/redstone/PistonHeadBlock.hpp"
#include "world/block/blocks/redstone/PoweredRailBlock.hpp"
#include "world/block/blocks/redstone/RailBlock.hpp"
#include "world/block/blocks/redstone/RedstoneComparatorBlock.hpp"
#include "world/block/blocks/redstone/RedstoneLampBlock.hpp"
#include "world/block/blocks/redstone/RedstoneRepeaterBlock.hpp"
#include "world/block/blocks/redstone/RedstoneTorchBlock.hpp"
#include "world/block/blocks/redstone/RedstoneWallTorchBlock.hpp"
#include "world/block/blocks/redstone/RedstoneWireBlock.hpp"
#include "world/block/blocks/redstone/StoneButtonBlock.hpp"
#include "world/block/blocks/redstone/StonePressurePlateBlock.hpp"
#include "world/block/blocks/redstone/TargetBlock.hpp"
#include "world/block/blocks/redstone/TripWireBlock.hpp"
#include "world/block/blocks/redstone/TripWireHookBlock.hpp"
#include "world/block/blocks/redstone/WeightedPressurePlateBlock.hpp"
#include "world/block/blocks/redstone/WoodButtonBlock.hpp"
#include "world/block/blocks/redstone/WoodPressurePlateBlock.hpp"

namespace mc {
namespace block_registry {

// 红石方块
Block* RedstoneBlocks::REDSTONE_WIRE = nullptr;
Block* RedstoneBlocks::REDSTONE_TORCH = nullptr;
Block* RedstoneBlocks::REDSTONE_WALL_TORCH = nullptr;
Block* RedstoneBlocks::REDSTONE_LAMP = nullptr;
Block* RedstoneBlocks::REDSTONE_REPEATER = nullptr;
Block* RedstoneBlocks::REDSTONE_COMPARATOR = nullptr;
Block* RedstoneBlocks::OBSERVER = nullptr;
Block* RedstoneBlocks::LEVER = nullptr;
Block* RedstoneBlocks::STONE_BUTTON = nullptr;
Block* RedstoneBlocks::OAK_BUTTON = nullptr;
Block* RedstoneBlocks::SPRUCE_BUTTON = nullptr;
Block* RedstoneBlocks::BIRCH_BUTTON = nullptr;
Block* RedstoneBlocks::JUNGLE_BUTTON = nullptr;
Block* RedstoneBlocks::ACACIA_BUTTON = nullptr;
Block* RedstoneBlocks::DARK_OAK_BUTTON = nullptr;
Block* RedstoneBlocks::CRIMSON_BUTTON = nullptr;
Block* RedstoneBlocks::WARPED_BUTTON = nullptr;
Block* RedstoneBlocks::MANGROVE_BUTTON = nullptr;
Block* RedstoneBlocks::CHERRY_BUTTON = nullptr;
Block* RedstoneBlocks::BAMBOO_BUTTON = nullptr;
Block* RedstoneBlocks::PALE_OAK_BUTTON = nullptr;
Block* RedstoneBlocks::POLISHED_BLACKSTONE_BUTTON = nullptr;
Block* RedstoneBlocks::STONE_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::OAK_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::SPRUCE_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::BIRCH_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::JUNGLE_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::ACACIA_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::DARK_OAK_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::CRIMSON_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::WARPED_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::MANGROVE_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::CHERRY_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::BAMBOO_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::PALE_OAK_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::POLISHED_BLACKSTONE_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::LIGHT_WEIGHTED_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::HEAVY_WEIGHTED_PRESSURE_PLATE = nullptr;
Block* RedstoneBlocks::DAYLIGHT_DETECTOR = nullptr;
Block* RedstoneBlocks::PISTON = nullptr;
Block* RedstoneBlocks::STICKY_PISTON = nullptr;
Block* RedstoneBlocks::PISTON_HEAD = nullptr;
Block* RedstoneBlocks::MOVING_PISTON = nullptr;
Block* RedstoneBlocks::DISPENSER = nullptr;
Block* RedstoneBlocks::DROPPER = nullptr;
Block* RedstoneBlocks::NOTE_BLOCK = nullptr;
Block* RedstoneBlocks::TRIPWIRE = nullptr;
Block* RedstoneBlocks::TRIPWIRE_HOOK = nullptr;
Block* RedstoneBlocks::TARGET = nullptr;

// 铁轨方块
Block* RedstoneBlocks::RAIL = nullptr;
Block* RedstoneBlocks::POWERED_RAIL = nullptr;
Block* RedstoneBlocks::DETECTOR_RAIL = nullptr;
Block* RedstoneBlocks::ACTIVATOR_RAIL = nullptr;

void registerRedstoneBlocks()
{
    auto& registry = BlockRegistry::instance();

    // 红石线
    RedstoneBlocks::REDSTONE_WIRE = &registry.registerBlock<blocks::RedstoneWireBlock>(
        ResourceLocation("minecraft:redstone_wire"), BlockProperties(Material::DECORATION).noCollision().notSolid());

    // 红石火把
    RedstoneBlocks::REDSTONE_TORCH =
        &registry.registerBlock<blocks::RedstoneTorchBlock>(ResourceLocation("minecraft:redstone_torch"),
            BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(7));

    // 墙上的红石火把
    RedstoneBlocks::REDSTONE_WALL_TORCH =
        &registry.registerBlock<blocks::RedstoneWallTorchBlock>(ResourceLocation("minecraft:redstone_wall_torch"),
            BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(7));

    // 红石灯
    RedstoneBlocks::REDSTONE_LAMP = &registry.registerBlock<blocks::RedstoneLampBlock>(
        ResourceLocation("minecraft:redstone_lamp"), BlockProperties(Material::REDSTONE_LIGHT).hardness(0.3f));

    // 红石中继器
    RedstoneBlocks::REDSTONE_REPEATER = &registry.registerBlock<blocks::RedstoneRepeaterBlock>(
        ResourceLocation("minecraft:repeater"), BlockProperties(Material::DECORATION).noCollision().notSolid());

    // 红石比较器
    RedstoneBlocks::REDSTONE_COMPARATOR = &registry.registerBlock<blocks::RedstoneComparatorBlock>(
        ResourceLocation("minecraft:comparator"), BlockProperties(Material::DECORATION).noCollision().notSolid());

    // 侦测器
    RedstoneBlocks::OBSERVER = &registry.registerBlock<blocks::ObserverBlock>(
        ResourceLocation("minecraft:observer"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));

    // 拉杆
    RedstoneBlocks::LEVER = &registry.registerBlock<blocks::LeverBlock>(
        ResourceLocation("minecraft:lever"), BlockProperties(Material::DECORATION).noCollision().notSolid());

    // 石头按钮
    RedstoneBlocks::STONE_BUTTON = &registry.registerBlock<blocks::StoneButtonBlock>(
        ResourceLocation("minecraft:stone_button"), BlockProperties(Material::ROCK).noCollision().notSolid());

    // 橡木按钮
    RedstoneBlocks::OAK_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:oak_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 云杉木按钮
    RedstoneBlocks::SPRUCE_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:spruce_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 白桦木按钮
    RedstoneBlocks::BIRCH_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:birch_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 丛林木按钮
    RedstoneBlocks::JUNGLE_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:jungle_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 金合欢木按钮
    RedstoneBlocks::ACACIA_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:acacia_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 深色橡木按钮
    RedstoneBlocks::DARK_OAK_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:dark_oak_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 绯红按钮（下界木材，不可燃）
    RedstoneBlocks::CRIMSON_BUTTON = &registry.registerBlock<blocks::WoodButtonBlock>(
        ResourceLocation("minecraft:crimson_button"), BlockProperties(Material::NETHER_WOOD).noCollision().notSolid());

    // 诡异按钮（下界木材，不可燃）
    RedstoneBlocks::WARPED_BUTTON = &registry.registerBlock<blocks::WoodButtonBlock>(
        ResourceLocation("minecraft:warped_button"), BlockProperties(Material::NETHER_WOOD).noCollision().notSolid());

    // 红树林木按钮
    RedstoneBlocks::MANGROVE_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:mangrove_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 樱花木按钮
    RedstoneBlocks::CHERRY_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:cherry_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 竹木按钮
    RedstoneBlocks::BAMBOO_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:bamboo_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 苍白橡木按钮
    RedstoneBlocks::PALE_OAK_BUTTON =
        &registry.registerBlock<blocks::WoodButtonBlock>(ResourceLocation("minecraft:pale_oak_button"),
            BlockProperties(Material::WOOD).noCollision().notSolid().flammable().ignitedByLava());

    // 磨制黑石按钮
    RedstoneBlocks::POLISHED_BLACKSTONE_BUTTON =
        &registry.registerBlock<blocks::StoneButtonBlock>(ResourceLocation("minecraft:polished_blackstone_button"),
            BlockProperties(Material::ROCK).noCollision().notSolid());

    // 石头压力板
    RedstoneBlocks::STONE_PRESSURE_PLATE =
        &registry.registerBlock<blocks::StonePressurePlateBlock>(ResourceLocation("minecraft:stone_pressure_plate"),
            BlockProperties(Material::ROCK).noCollision().notSolid().hardness(0.5f));

    // 橡木压力板
    RedstoneBlocks::OAK_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:oak_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 云杉木压力板
    RedstoneBlocks::SPRUCE_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:spruce_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 白桦木压力板
    RedstoneBlocks::BIRCH_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:birch_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 丛林木压力板
    RedstoneBlocks::JUNGLE_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:jungle_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 金合欢木压力板
    RedstoneBlocks::ACACIA_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:acacia_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 深色橡木压力板
    RedstoneBlocks::DARK_OAK_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:dark_oak_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 绯红木压力板
    RedstoneBlocks::CRIMSON_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:crimson_pressure_plate"),
            BlockProperties(Material::NETHER_WOOD).noCollision().notSolid().hardness(0.5f));

    // 诡异木压力板
    RedstoneBlocks::WARPED_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:warped_pressure_plate"),
            BlockProperties(Material::NETHER_WOOD).noCollision().notSolid().hardness(0.5f));

    // 红树木压力板
    RedstoneBlocks::MANGROVE_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:mangrove_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 樱花木压力板
    RedstoneBlocks::CHERRY_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:cherry_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 竹木压力板
    RedstoneBlocks::BAMBOO_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:bamboo_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 苍白橡木压力板
    RedstoneBlocks::PALE_OAK_PRESSURE_PLATE =
        &registry.registerBlock<blocks::WoodPressurePlateBlock>(ResourceLocation("minecraft:pale_oak_pressure_plate"),
            BlockProperties(Material::WOOD).noCollision().notSolid().hardness(0.5f).flammable().ignitedByLava());

    // 磨制黑石压力板
    RedstoneBlocks::POLISHED_BLACKSTONE_PRESSURE_PLATE = &registry.registerBlock<blocks::StonePressurePlateBlock>(
        ResourceLocation("minecraft:polished_blackstone_pressure_plate"),
        BlockProperties(Material::ROCK).noCollision().notSolid().hardness(0.5f));

    // 轻质测重压力板
    RedstoneBlocks::LIGHT_WEIGHTED_PRESSURE_PLATE = &registry.registerBlock<blocks::WeightedPressurePlateBlock>(
        ResourceLocation("minecraft:light_weighted_pressure_plate"),
        BlockProperties(Material::IRON).noCollision().notSolid().hardness(0.5f),
        blocks::WeightedPressurePlateBlock::Sensitivity::Light);

    // 重质测重压力板
    RedstoneBlocks::HEAVY_WEIGHTED_PRESSURE_PLATE = &registry.registerBlock<blocks::WeightedPressurePlateBlock>(
        ResourceLocation("minecraft:heavy_weighted_pressure_plate"),
        BlockProperties(Material::IRON).noCollision().notSolid().hardness(0.5f),
        blocks::WeightedPressurePlateBlock::Sensitivity::Heavy);

    // 日光探测器
    RedstoneBlocks::DAYLIGHT_DETECTOR =
        &registry.registerBlock<blocks::DaylightDetectorBlock>(ResourceLocation("minecraft:daylight_detector"),
            BlockProperties(Material::WOOD).hardness(0.2f).flammable().ignitedByLava());

    // 活塞
    RedstoneBlocks::PISTON = &registry.registerBlock<blocks::PistonBlock>(ResourceLocation("minecraft:piston"),
        BlockProperties(Material::PISTON).hardness(0.5f).resistance(0.5f),
        false // not sticky
    );

    // 粘性活塞
    RedstoneBlocks::STICKY_PISTON =
        &registry.registerBlock<blocks::PistonBlock>(ResourceLocation("minecraft:sticky_piston"),
            BlockProperties(Material::PISTON).hardness(0.5f).resistance(0.5f),
            true // sticky
        );

    // 活塞头
    RedstoneBlocks::PISTON_HEAD =
        &registry.registerBlock<blocks::PistonHeadBlock>(ResourceLocation("minecraft:piston_head"),
            BlockProperties(Material::PISTON).hardness(0.5f).resistance(0.5f).noLootTable());

    // 移动中的活塞
    RedstoneBlocks::MOVING_PISTON =
        &registry.registerBlock<blocks::MovingPistonBlock>(ResourceLocation("minecraft:moving_piston"),
            BlockProperties(Material::PISTON).hardness(-1.0f).resistance(-1.0f).noLootTable());

    // 发射器
    RedstoneBlocks::DISPENSER = &registry.registerBlock<blocks::DispenserBlock>(
        ResourceLocation("minecraft:dispenser"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));

    // 投掷器
    RedstoneBlocks::DROPPER = &registry.registerBlock<blocks::DropperBlock>(
        ResourceLocation("minecraft:dropper"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));

    // 音符盒
    RedstoneBlocks::NOTE_BLOCK = &registry.registerBlock<blocks::NoteBlock>(ResourceLocation("minecraft:note_block"),
        BlockProperties(Material::WOOD).hardness(0.8f).flammable().ignitedByLava());

    // 标靶
    RedstoneBlocks::TARGET = &registry.registerBlock<blocks::TargetBlock>(
        ResourceLocation("minecraft:target"), BlockProperties(Material::WOOL).hardness(0.5f));

    // 绊线
    RedstoneBlocks::TRIPWIRE = &registry.registerBlock<blocks::TripWireBlock>(
        ResourceLocation("minecraft:tripwire"), BlockProperties(Material::DECORATION).noCollision().notSolid());

    // 绊线钩
    RedstoneBlocks::TRIPWIRE_HOOK = &registry.registerBlock<blocks::TripWireHookBlock>(
        ResourceLocation("minecraft:tripwire_hook"), BlockProperties(Material::DECORATION).noCollision().notSolid());

    // 普通铁轨
    RedstoneBlocks::RAIL = &registry.registerBlock<blocks::RailBlock>(ResourceLocation("minecraft:rail"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.7f));

    // 动力铁轨
    RedstoneBlocks::POWERED_RAIL =
        &registry.registerBlock<blocks::PoweredRailBlock>(ResourceLocation("minecraft:powered_rail"),
            BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.7f));

    // 探测铁轨
    RedstoneBlocks::DETECTOR_RAIL =
        &registry.registerBlock<blocks::DetectorRailBlock>(ResourceLocation("minecraft:detector_rail"),
            BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.7f));

    // 激活铁轨
    RedstoneBlocks::ACTIVATOR_RAIL =
        &registry.registerBlock<blocks::ActivatorRailBlock>(ResourceLocation("minecraft:activator_rail"),
            BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.7f));
}

} // namespace block_registry
} // namespace mc
