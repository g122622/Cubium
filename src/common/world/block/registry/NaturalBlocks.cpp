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

#include "world/block/registry/NaturalBlocks.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/FallingBlock.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/agricultural/FarmlandBlock.hpp"
#include "world/block/blocks/coral/CoralBlock.hpp"
#include "world/block/blocks/decorative/TorchBlock.hpp"
#include "world/block/blocks/decorative/WallTorchBlock.hpp"
#include "world/block/blocks/dirt/DirtPathBlock.hpp"
#include "world/block/blocks/dirt/SpreadableSnowyDirtBlock.hpp"
#include "world/block/blocks/garden/DryVegetationBlock.hpp"
#include "world/block/blocks/garden/WaterlilyBlock.hpp"
#include "world/block/blocks/ice/IceBlock.hpp"
#include "world/block/blocks/ice/SnowBlock.hpp"
#include "world/block/blocks/mob/BeehiveBlock.hpp"
#include "world/block/blocks/mob/TurtleEggBlock.hpp"
#include "world/block/blocks/ocean/BubbleColumnBlock.hpp"
#include "world/block/blocks/ocean/ConduitBlock.hpp"
#include "world/block/blocks/ocean/DriedKelpBlock.hpp"
#include "world/block/blocks/ocean/KelpBlock.hpp"
#include "world/block/blocks/ocean/SeaPickleBlock.hpp"
#include "world/block/blocks/ocean/SeagrassBlock.hpp"
#include "world/block/blocks/ocean/TallSeagrassBlock.hpp"
#include "world/block/blocks/special/WebBlock.hpp"
#include "world/block/blocks/vegetation/BambooBlock.hpp"
#include "world/block/blocks/vegetation/CactusBlock.hpp"
#include "world/block/blocks/vegetation/SugarCaneBlock.hpp"
#include "world/block/blocks/vegetation/TallGrassBlock.hpp"
#include "world/block/blocks/vegetation/VineBlock.hpp"
#include "world/block/registry/BaseBlocks.hpp"

namespace mc {
namespace block_registry {

// 自然方块扩展
Block* NaturalBlocks::CLAY = nullptr;
Block* NaturalBlocks::MYCELIUM = nullptr;
Block* NaturalBlocks::GRASS_PATH = nullptr;
Block* NaturalBlocks::PACKED_ICE = nullptr;
Block* NaturalBlocks::BLUE_ICE = nullptr;
Block* NaturalBlocks::FROSTED_ICE = nullptr;
Block* NaturalBlocks::SLIME_BLOCK = nullptr;
Block* NaturalBlocks::HONEY_BLOCK = nullptr;
Block* NaturalBlocks::CACTUS = nullptr;
Block* NaturalBlocks::DEAD_BUSH = nullptr;
Block* NaturalBlocks::LILY_PAD = nullptr;
Block* NaturalBlocks::VINE = nullptr;
Block* NaturalBlocks::COBWEB = nullptr;
Block* NaturalBlocks::SUGAR_CANE = nullptr;
Block* NaturalBlocks::FARMLAND = nullptr;
Block* NaturalBlocks::RED_SAND = nullptr;
Block* NaturalBlocks::DRIED_KELP_BLOCK = nullptr;
Block* NaturalBlocks::SEA_PICKLE = nullptr;
Block* NaturalBlocks::KELP = nullptr;
Block* NaturalBlocks::KELP_PLANT = nullptr;
Block* NaturalBlocks::SEAGRASS = nullptr;
Block* NaturalBlocks::TALL_SEAGRASS = nullptr;
Block* NaturalBlocks::BUBBLE_COLUMN = nullptr;
Block* NaturalBlocks::TURTLE_EGG = nullptr;

// 珊瑚方块
Block* NaturalBlocks::DEAD_TUBE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::DEAD_BRAIN_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::DEAD_BUBBLE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::DEAD_FIRE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::DEAD_HORN_CORAL_BLOCK = nullptr;

Block* NaturalBlocks::DEAD_TUBE_CORAL_FAN = nullptr;
Block* NaturalBlocks::DEAD_BRAIN_CORAL_FAN = nullptr;
Block* NaturalBlocks::DEAD_BUBBLE_CORAL_FAN = nullptr;
Block* NaturalBlocks::DEAD_FIRE_CORAL_FAN = nullptr;
Block* NaturalBlocks::DEAD_HORN_CORAL_FAN = nullptr;

Block* NaturalBlocks::DEAD_TUBE_CORAL_WALL_FAN = nullptr;
Block* NaturalBlocks::DEAD_BRAIN_CORAL_WALL_FAN = nullptr;
Block* NaturalBlocks::DEAD_BUBBLE_CORAL_WALL_FAN = nullptr;
Block* NaturalBlocks::DEAD_FIRE_CORAL_WALL_FAN = nullptr;
Block* NaturalBlocks::DEAD_HORN_CORAL_WALL_FAN = nullptr;

Block* NaturalBlocks::TUBE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::BRAIN_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::BUBBLE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::FIRE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::HORN_CORAL_BLOCK = nullptr;

Block* NaturalBlocks::TUBE_CORAL_FAN = nullptr;
Block* NaturalBlocks::BRAIN_CORAL_FAN = nullptr;
Block* NaturalBlocks::BUBBLE_CORAL_FAN = nullptr;
Block* NaturalBlocks::FIRE_CORAL_FAN = nullptr;
Block* NaturalBlocks::HORN_CORAL_FAN = nullptr;

Block* NaturalBlocks::TUBE_CORAL_WALL_FAN = nullptr;
Block* NaturalBlocks::BRAIN_CORAL_WALL_FAN = nullptr;
Block* NaturalBlocks::BUBBLE_CORAL_WALL_FAN = nullptr;
Block* NaturalBlocks::FIRE_CORAL_WALL_FAN = nullptr;
Block* NaturalBlocks::HORN_CORAL_WALL_FAN = nullptr;

Block* NaturalBlocks::CONDUIT = nullptr;

// 蜂巢/蜂箱
Block* NaturalBlocks::BEE_NEST = nullptr;
Block* NaturalBlocks::BEEHIVE = nullptr;

// 火把
Block* NaturalBlocks::TORCH = nullptr;
Block* NaturalBlocks::WALL_TORCH = nullptr;

void registerNaturalBlocks()
{
    auto& registry = BlockRegistry::instance();

    // 粘土
    NaturalBlocks::CLAY = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:clay"), BlockProperties(Material::EARTH).hardness(0.6f));

    // 菌丝
    NaturalBlocks::MYCELIUM = &registry.registerBlock<blocks::MyceliumBlock>(ResourceLocation("minecraft:mycelium"),
        BlockProperties(Material::EARTH).hardness(0.6f).soundType(BlockSoundTypes::GRASS));

    // 草径
    NaturalBlocks::GRASS_PATH = &registry.registerBlock<blocks::DirtPathBlock>(
        ResourceLocation("minecraft:dirt_path"), BlockProperties(Material::EARTH).hardness(0.65f));

    // 浮冰 - 不透明，不融化
    NaturalBlocks::PACKED_ICE =
        &registry.registerBlock<blocks::PackedIceBlock>(ResourceLocation("minecraft:packed_ice"),
            BlockProperties(Material::ICE)
                .hardness(0.5f)
                .opacity(2)
                .propagatesSkylightDown()
                .slipperiness(physics::SLIPPERINESS_ICE));

    // 蓝冰 - 最滑的方块
    NaturalBlocks::BLUE_ICE = &registry.registerBlock<blocks::BlueIceBlock>(ResourceLocation("minecraft:blue_ice"),
        BlockProperties(Material::ICE).hardness(2.8f).resistance(2.8f).slipperiness(physics::SLIPPERINESS_BLUE_ICE));

    // 霜冰 - 由冰霜行者附魔生成的临时冰
    NaturalBlocks::FROSTED_ICE =
        &registry.registerBlock<blocks::FrostedIceBlock>(ResourceLocation("minecraft:frosted_ice"),
            BlockProperties(Material::ICE)
                .hardness(0.5f)
                .notSolid()
                .opacity(2)
                .propagatesSkylightDown()
                .slipperiness(physics::SLIPPERINESS_ICE));

    // 粘液块
    NaturalBlocks::SLIME_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:slime_block"), BlockProperties(Material::SLIME).hardness(0.0f));

    // 蜂蜜块
    NaturalBlocks::HONEY_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:honey_block"), BlockProperties(Material::HONEY).hardness(0.0f).notSolid());

    // 仙人掌
    NaturalBlocks::CACTUS = &registry.registerBlock<blocks::CactusBlock>(
        ResourceLocation("minecraft:cactus"), BlockProperties(Material::PLANT).hardness(0.4f));

    // 枯萎灌木 - 干草类植物，可生长在沙/陶瓦/泥土/耕地上（沙漠/恶地生物群系）。
    // 改用 DryVegetationBlock 走 canSurvive 闸门，修复世界生成时浮空（此前注册为 SimpleBlock 致终判失效）。
    NaturalBlocks::DEAD_BUSH = &registry.registerBlock<blocks::DryVegetationBlock>(
        ResourceLocation("minecraft:dead_bush"), BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 睡莲 - 水生植物，下方须为水/冰且上方无流体。新建 WaterlilyBlock 走 canSurvive 闸门，
    // 修复世界生成时浮空（此前注册为 SimpleBlock 致终判失效）。
    NaturalBlocks::LILY_PAD = &registry.registerBlock<blocks::WaterlilyBlock>(
        ResourceLocation("minecraft:lily_pad"), BlockProperties(Material::PLANT).noCollision().notSolid());

    // 藤蔓
    NaturalBlocks::VINE = &registry.registerBlock<blocks::VineBlock>(ResourceLocation("minecraft:vine"),
        BlockProperties(Material::REPLACEABLE_PLANT).hardness(0.2f).noCollision().notSolid());

    // 蜘蛛网
    NaturalBlocks::COBWEB = &registry.registerBlock<blocks::WebBlock>(
        ResourceLocation("minecraft:cobweb"), BlockProperties(Material::WEB).hardness(4.0f).noCollision());

    // 甘蔗
    NaturalBlocks::SUGAR_CANE =
        &registry.registerBlock<blocks::SugarCaneBlock>(ResourceLocation("minecraft:sugar_cane"),
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 耕地
    NaturalBlocks::FARMLAND = &registry.registerBlock<blocks::FarmlandBlock>(
        ResourceLocation("minecraft:farmland"), BlockProperties(Material::EARTH).hardness(0.6f));

    // 红沙
    NaturalBlocks::RED_SAND = &registry.registerBlock<blocks::FallingBlock>(
        ResourceLocation("minecraft:red_sand"), BlockProperties(Material::SAND).hardness(0.5f));

    // 干海带块
    NaturalBlocks::DRIED_KELP_BLOCK =
        &registry.registerBlock<blocks::DriedKelpBlock>(ResourceLocation("minecraft:dried_kelp_block"),
            BlockProperties(Material::PLANT).hardness(0.5f).resistance(0.5f));

    // 海泡菜
    NaturalBlocks::SEA_PICKLE =
        &registry.registerBlock<blocks::SeaPickleBlock>(ResourceLocation("minecraft:sea_pickle"),
            BlockProperties(Material::OCEAN_PLANT).noCollision().notSolid().lightLevel(6));

    // 海带顶部和海带茎
    NaturalBlocks::KELP = &registry.registerBlock<blocks::KelpBlock>(
        ResourceLocation("minecraft:kelp"), BlockProperties(Material::OCEAN_PLANT).noCollision().notSolid());
    NaturalBlocks::KELP_PLANT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:kelp_plant"), BlockProperties(Material::OCEAN_PLANT).noCollision().notSolid());

    // 海草与高海草
    NaturalBlocks::SEAGRASS = &registry.registerBlock<blocks::SeagrassBlock>(
        ResourceLocation("minecraft:seagrass"), BlockProperties(Material::SEA_GRASS).noCollision().notSolid());
    NaturalBlocks::TALL_SEAGRASS = &registry.registerBlock<blocks::TallSeagrassBlock>(
        ResourceLocation("minecraft:tall_seagrass"), BlockProperties(Material::SEA_GRASS).noCollision().notSolid());

    // 气泡柱与海龟蛋
    NaturalBlocks::BUBBLE_COLUMN =
        &registry.registerBlock<blocks::BubbleColumnBlock>(ResourceLocation("minecraft:bubble_column"),
            BlockProperties(Material::WATER).noCollision().notSolid().opacity(0).propagatesSkylightDown());
    NaturalBlocks::TURTLE_EGG =
        &registry.registerBlock<blocks::TurtleEggBlock>(ResourceLocation("minecraft:turtle_egg"),
            BlockProperties(Material::CORAL).hardness(0.5f).noCollision().notSolid());

    // 珊瑚（补齐死亡变种，便于海洋废墟/暖海装饰复用）
    const u32 deadFallbackId = BaseBlocks::AIR ? BaseBlocks::AIR->blockId() : 0;
    const BlockProperties coralBlockProps = BlockProperties(Material::CORAL).hardness(1.5f).resistance(6.0f);
    const BlockProperties coralPlantProps = BlockProperties(Material::CORAL).hardness(0.0f).noCollision().notSolid();
    const BlockProperties deadCoralBlockProps = BlockProperties(Material::CORAL).hardness(1.5f).resistance(6.0f);
    const BlockProperties deadCoralPlantProps =
        BlockProperties(Material::CORAL).hardness(0.0f).noCollision().notSolid();

    NaturalBlocks::DEAD_TUBE_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:dead_tube_coral_block"), blocks::CoralColor::Tube, deadCoralBlockProps);
    NaturalBlocks::DEAD_BRAIN_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:dead_brain_coral_block"), blocks::CoralColor::Brain, deadCoralBlockProps);
    NaturalBlocks::DEAD_BUBBLE_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:dead_bubble_coral_block"), blocks::CoralColor::Bubble, deadCoralBlockProps);
    NaturalBlocks::DEAD_FIRE_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:dead_fire_coral_block"), blocks::CoralColor::Fire, deadCoralBlockProps);
    NaturalBlocks::DEAD_HORN_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:dead_horn_coral_block"), blocks::CoralColor::Horn, deadCoralBlockProps);

    const u32 deadTubeBlockId =
        NaturalBlocks::DEAD_TUBE_CORAL_BLOCK ? NaturalBlocks::DEAD_TUBE_CORAL_BLOCK->blockId() : deadFallbackId;
    const u32 deadBrainBlockId =
        NaturalBlocks::DEAD_BRAIN_CORAL_BLOCK ? NaturalBlocks::DEAD_BRAIN_CORAL_BLOCK->blockId() : deadFallbackId;
    const u32 deadBubbleBlockId =
        NaturalBlocks::DEAD_BUBBLE_CORAL_BLOCK ? NaturalBlocks::DEAD_BUBBLE_CORAL_BLOCK->blockId() : deadFallbackId;
    const u32 deadFireBlockId =
        NaturalBlocks::DEAD_FIRE_CORAL_BLOCK ? NaturalBlocks::DEAD_FIRE_CORAL_BLOCK->blockId() : deadFallbackId;
    const u32 deadHornBlockId =
        NaturalBlocks::DEAD_HORN_CORAL_BLOCK ? NaturalBlocks::DEAD_HORN_CORAL_BLOCK->blockId() : deadFallbackId;

    NaturalBlocks::DEAD_TUBE_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_tube_coral_fan"),
            blocks::CoralColor::Tube,
            deadTubeBlockId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_BRAIN_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_brain_coral_fan"),
            blocks::CoralColor::Brain,
            deadBrainBlockId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_BUBBLE_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_bubble_coral_fan"),
            blocks::CoralColor::Bubble,
            deadBubbleBlockId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_FIRE_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_fire_coral_fan"),
            blocks::CoralColor::Fire,
            deadFireBlockId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_HORN_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_horn_coral_fan"),
            blocks::CoralColor::Horn,
            deadHornBlockId,
            deadCoralPlantProps);

    NaturalBlocks::DEAD_TUBE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_tube_coral_wall_fan"),
            blocks::CoralColor::Tube,
            deadTubeBlockId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_BRAIN_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_brain_coral_wall_fan"),
            blocks::CoralColor::Brain,
            deadBrainBlockId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_BUBBLE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_bubble_coral_wall_fan"),
            blocks::CoralColor::Bubble,
            deadBubbleBlockId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_FIRE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_fire_coral_wall_fan"),
            blocks::CoralColor::Fire,
            deadFireBlockId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_HORN_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_horn_coral_wall_fan"),
            blocks::CoralColor::Horn,
            deadHornBlockId,
            deadCoralPlantProps);

    NaturalBlocks::TUBE_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:tube_coral_block"), blocks::CoralColor::Tube, coralBlockProps);
    NaturalBlocks::BRAIN_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:brain_coral_block"), blocks::CoralColor::Brain, coralBlockProps);
    NaturalBlocks::BUBBLE_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:bubble_coral_block"), blocks::CoralColor::Bubble, coralBlockProps);
    NaturalBlocks::FIRE_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:fire_coral_block"), blocks::CoralColor::Fire, coralBlockProps);
    NaturalBlocks::HORN_CORAL_BLOCK = &registry.registerBlock<blocks::CoralBlockBlock>(
        ResourceLocation("minecraft:horn_coral_block"), blocks::CoralColor::Horn, coralBlockProps);

    NaturalBlocks::TUBE_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:tube_coral_fan"), blocks::CoralColor::Tube, deadTubeBlockId, coralPlantProps);
    NaturalBlocks::BRAIN_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:brain_coral_fan"), blocks::CoralColor::Brain, deadBrainBlockId, coralPlantProps);
    NaturalBlocks::BUBBLE_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:bubble_coral_fan"), blocks::CoralColor::Bubble, deadBubbleBlockId, coralPlantProps);
    NaturalBlocks::FIRE_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:fire_coral_fan"), blocks::CoralColor::Fire, deadFireBlockId, coralPlantProps);
    NaturalBlocks::HORN_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:horn_coral_fan"), blocks::CoralColor::Horn, deadHornBlockId, coralPlantProps);

    NaturalBlocks::TUBE_CORAL_WALL_FAN = &registry.registerBlock<blocks::CoralWallFanBlock>(
        ResourceLocation("minecraft:tube_coral_wall_fan"), blocks::CoralColor::Tube, deadTubeBlockId, coralPlantProps);
    NaturalBlocks::BRAIN_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:brain_coral_wall_fan"),
            blocks::CoralColor::Brain,
            deadBrainBlockId,
            coralPlantProps);
    NaturalBlocks::BUBBLE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:bubble_coral_wall_fan"),
            blocks::CoralColor::Bubble,
            deadBubbleBlockId,
            coralPlantProps);
    NaturalBlocks::FIRE_CORAL_WALL_FAN = &registry.registerBlock<blocks::CoralWallFanBlock>(
        ResourceLocation("minecraft:fire_coral_wall_fan"), blocks::CoralColor::Fire, deadFireBlockId, coralPlantProps);
    NaturalBlocks::HORN_CORAL_WALL_FAN = &registry.registerBlock<blocks::CoralWallFanBlock>(
        ResourceLocation("minecraft:horn_coral_wall_fan"), blocks::CoralColor::Horn, deadHornBlockId, coralPlantProps);

    // 潮涌核心 - 水下信标类方块，需要潮涌框架激活
    NaturalBlocks::CONDUIT = &registry.registerBlock<blocks::ConduitBlock>(ResourceLocation("minecraft:conduit"),
        BlockProperties(Material::GLASS).hardness(3.0f).resistance(3.0f).notSolid());

    // ========== 火把 ==========
    // 火把 - 发光等级14，生成火焰和烟雾粒子
    NaturalBlocks::TORCH = &registry.registerBlock<blocks::TorchBlock>(ResourceLocation("minecraft:torch"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(14),
        particle::ParticleTypeId::Flame);

    // 墙上的火把 - 附着在墙上，根据朝向有不同碰撞箱和粒子位置
    NaturalBlocks::WALL_TORCH =
        &registry.registerBlock<blocks::WallTorchBlock>(ResourceLocation("minecraft:wall_torch"),
            BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(14),
            particle::ParticleTypeId::Flame);

    // ========== 蜂巢/蜂箱 ==========
    // 蜂巢 - 自然生成于树上，材质较软
    NaturalBlocks::BEE_NEST = &registry.registerBlock<blocks::BeehiveBlock>(ResourceLocation("minecraft:bee_nest"),
        BlockProperties(Material::WOOD).hardness(0.3f).resistance(0.3f).flammable().ignitedByLava());

    // 蜂箱 - 玩家合成，用木板制作，需要斧头加速挖掘
    NaturalBlocks::BEEHIVE = &registry.registerBlock<blocks::BeehiveBlock>(ResourceLocation("minecraft:beehive"),
        BlockProperties(Material::WOOD)
            .hardness(0.6f)
            .resistance(0.6f)
            .flammable()
            .ignitedByLava()
            .harvestTool(HarvestTool::Axe));
}

} // namespace block_registry
} // namespace mc
