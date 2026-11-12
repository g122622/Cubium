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

#include "common/world/block/registry/vanilla/NaturalBlocks.hpp"
#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include "common/world/block/base/types/HarvestTool.hpp"
#include "common/world/block/blocks/base/FallingBlock.hpp"
#include "common/world/block/blocks/base/SimpleBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/BaseBlocks.hpp"
#include "common/world/block/registry/vanilla/BlockStatePredicates.hpp"
#include "world/block/blocks/agricultural/FarmlandBlock.hpp"
#include "world/block/blocks/coral/CoralBlock.hpp"
#include "world/block/blocks/coral/CoralBlockBlock.hpp"
#include "world/block/blocks/coral/CoralColor.hpp"
#include "world/block/blocks/coral/CoralFanBlock.hpp"
#include "world/block/blocks/coral/CoralPlantBlock.hpp"
#include "world/block/blocks/coral/CoralWallFanBlock.hpp"
#include "world/block/blocks/decorative/PiglinWallSkullBlock.hpp"
#include "world/block/blocks/decorative/PlayerHeadBlock.hpp"
#include "world/block/blocks/decorative/SkullBlock.hpp"
#include "world/block/blocks/decorative/SkullType.hpp"
#include "world/block/blocks/decorative/TorchBlock.hpp"
#include "world/block/blocks/decorative/WallSkullBlock.hpp"
#include "world/block/blocks/decorative/WallTorchBlock.hpp"
#include "world/block/blocks/decorative/WitherSkullBlock.hpp"
#include "world/block/blocks/dirt/DirtPathBlock.hpp"
#include "world/block/blocks/dirt/MyceliumBlock.hpp"
#include "world/block/blocks/dirt/SpreadableSnowyDirtBlock.hpp"
#include "world/block/blocks/garden/DryVegetationBlock.hpp"
#include "world/block/blocks/garden/WaterlilyBlock.hpp"
#include "world/block/blocks/ice/BlueIceBlock.hpp"
#include "world/block/blocks/ice/FrostedIceBlock.hpp"
#include "world/block/blocks/ice/IceBlock.hpp"
#include "world/block/blocks/ice/PackedIceBlock.hpp"
#include "world/block/blocks/mob/BeehiveBlock.hpp"
#include "world/block/blocks/mob/DriedGhastBlock.hpp"
#include "world/block/blocks/mob/TurtleEggBlock.hpp"
#include "world/block/blocks/ocean/BubbleColumnBlock.hpp"
#include "world/block/blocks/ocean/ConduitBlock.hpp"
#include "world/block/blocks/ocean/DriedKelpBlock.hpp"
#include "world/block/blocks/ocean/KelpBlock.hpp"
#include "world/block/blocks/ocean/KelpPlantBlock.hpp"
#include "world/block/blocks/ocean/SeaPickleBlock.hpp"
#include "world/block/blocks/ocean/SeagrassBlock.hpp"
#include "world/block/blocks/ocean/TallSeagrassBlock.hpp"
#include "world/block/blocks/special/HoneyBlock.hpp"
#include "world/block/blocks/special/SlimeBlock.hpp"
#include "world/block/blocks/special/WebBlock.hpp"
#include "world/block/blocks/vegetation/CactusBlock.hpp"
#include "world/block/blocks/vegetation/SugarCaneBlock.hpp"
#include "world/block/blocks/vegetation/VineBlock.hpp"

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

// 蜜脾块 / 干燥恶魂
Block* NaturalBlocks::HONEYCOMB_BLOCK = nullptr;
Block* NaturalBlocks::DRIED_GHAST = nullptr;

// 生物头颅 / 头（站立 + 墙挂变体）
Block* NaturalBlocks::SKELETON_SKULL = nullptr;
Block* NaturalBlocks::SKELETON_WALL_SKULL = nullptr;
Block* NaturalBlocks::WITHER_SKELETON_SKULL = nullptr;
Block* NaturalBlocks::WITHER_SKELETON_WALL_SKULL = nullptr;
Block* NaturalBlocks::PLAYER_HEAD = nullptr;
Block* NaturalBlocks::PLAYER_WALL_HEAD = nullptr;
Block* NaturalBlocks::ZOMBIE_HEAD = nullptr;
Block* NaturalBlocks::ZOMBIE_WALL_HEAD = nullptr;
Block* NaturalBlocks::CREEPER_HEAD = nullptr;
Block* NaturalBlocks::CREEPER_WALL_HEAD = nullptr;
Block* NaturalBlocks::DRAGON_HEAD = nullptr;
Block* NaturalBlocks::DRAGON_WALL_HEAD = nullptr;
Block* NaturalBlocks::PIGLIN_HEAD = nullptr;
Block* NaturalBlocks::PIGLIN_WALL_HEAD = nullptr;

// 珊瑚方块
Block* NaturalBlocks::DEAD_TUBE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::DEAD_BRAIN_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::DEAD_BUBBLE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::DEAD_FIRE_CORAL_BLOCK = nullptr;
Block* NaturalBlocks::DEAD_HORN_CORAL_BLOCK = nullptr;

Block* NaturalBlocks::DEAD_TUBE_CORAL = nullptr;
Block* NaturalBlocks::DEAD_BRAIN_CORAL = nullptr;
Block* NaturalBlocks::DEAD_BUBBLE_CORAL = nullptr;
Block* NaturalBlocks::DEAD_FIRE_CORAL = nullptr;
Block* NaturalBlocks::DEAD_HORN_CORAL = nullptr;

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

Block* NaturalBlocks::TUBE_CORAL = nullptr;
Block* NaturalBlocks::BRAIN_CORAL = nullptr;
Block* NaturalBlocks::BUBBLE_CORAL = nullptr;
Block* NaturalBlocks::FIRE_CORAL = nullptr;
Block* NaturalBlocks::HORN_CORAL = nullptr;

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
    NaturalBlocks::CLAY = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:clay"),
        BlockProperties(Material::EARTH)
            .instrument(NoteBlockInstrument::Flute)
            .resistance(0.600000024f)
            .hardness(0.6f));

    // 菌丝
    NaturalBlocks::MYCELIUM = &registry.registerBlock<blocks::MyceliumBlock>(ResourceLocation("minecraft:mycelium"),
        BlockProperties(Material::EARTH).resistance(0.600000024f).hardness(0.6f).soundType(BlockSoundTypes::GRASS));

    // 草径
    NaturalBlocks::GRASS_PATH = &registry.registerBlock<blocks::DirtPathBlock>(ResourceLocation("minecraft:dirt_path"),
        BlockProperties(Material::EARTH)
            .resistance(0.649999976f)
            .hardness(0.65f)
            // 原版 dirtPathProperties() 显式 isSuffocating/isViewBlocking(Blocks::never)：
            // 碰撞比满格矮 1 像素，默认判据会给 false，但玩家站在上面仍会被闷、镜头仍被挡。
            .suffocating(block_registry::BlockStatePredicates::dirtPathBlocking())
            .viewBlocking(block_registry::BlockStatePredicates::dirtPathBlocking())
            .soundType(BlockSoundTypes::GRASS));

    // 浮冰 - 不透明，不融化
    // 原版 packed_ice 用最普通的 Blocks.register(name, properties) 注册（普通 Block，无子类），
    // 属性里既没有 noOcclusion() 也没有状态自定义，因此 canOcclude=true、形状为整格，
    // propagatesSkylightDown 取默认公式得 false——天空光每穿过一格浮冰衰减 1 级。
    // 早期显式调用了 propagatesSkylightDown() 且把 opacity 设成 2，两个值都与原版不符
    // （实测原版 1.21.11：propagatesSkylightDown=false、lightBlock=1）。
    NaturalBlocks::PACKED_ICE =
        &registry.registerBlock<blocks::PackedIceBlock>(ResourceLocation("minecraft:packed_ice"),
            BlockProperties(Material::ICE)
                .instrument(NoteBlockInstrument::Chime)
                .resistance(0.5f)
                .hardness(0.5f)
                .slipperiness(physics::SLIPPERINESS_ICE));

    // 蓝冰 - 最滑的方块
    NaturalBlocks::BLUE_ICE = &registry.registerBlock<blocks::BlueIceBlock>(ResourceLocation("minecraft:blue_ice"),
        BlockProperties(Material::ICE).hardness(2.8f).resistance(2.8f).slipperiness(physics::SLIPPERINESS_BLUE_ICE));

    // 霜冰 - 由冰霜行者附魔生成的临时冰
    NaturalBlocks::FROSTED_ICE =
        &registry.registerBlock<blocks::FrostedIceBlock>(ResourceLocation("minecraft:frosted_ice"),
            BlockProperties(Material::ICE)
                .resistance(0.5f)
                .hardness(0.5f)
                .notSolid()
                .noOcclusion()
                .slipperiness(physics::SLIPPERINESS_ICE));

    // 粘液块。须注册为 blocks::SlimeBlock（而非 SimpleBlock）才能启用弹跳（onLanded 反弹 Y 速度）、
    // 摔落免疫（onFallenUpon multiplier=0.0）、活塞粘连（isStickyBlock/canStickTo）与滑度 0.8
    // （构造函数设 m_slipperiness=SLIPPERINESS_SLIME）。此前误注册为 SimpleBlock 致上述虚函数分发到
    // Block 基类默认实现（getSlipperiness 返回 0.6、onFallenUpon multiplier=1.0 施加完整摔落伤害、
    // isStickyBlock 返回 false），SlimeBlock 子类沦为死代码——与红石块（BaseBlocks.cpp）同模式缺陷。
    NaturalBlocks::SLIME_BLOCK = &registry.registerBlock<blocks::SlimeBlock>(ResourceLocation("minecraft:slime_block"),
        BlockProperties(Material::SLIME).hardness(0.0f).noOcclusion().soundType(BlockSoundTypes::SLIME));

    // 蜂蜜块。须注册为 blocks::HoneyBlock（而非 SimpleBlock）才能启用摔落减伤（onFallenUpon
    // multiplier=0.2）、不弹跳着地（onLanded Y 速度归零）、减速（onEntityCollision 水平 ×0.4）、
    // 活塞粘连（isStickyBlock/canStickTo 仅粘蜂蜜块）、缩小碰撞箱（getCollisionShape 15/16 高）与
    // 不透明（isOpaque=false）。此前误注册为 SimpleBlock 致上述虚函数分发到 Block 基类默认实现
    // （onFallenUpon multiplier=1.0 完整摔落伤害、isStickyBlock/isOpaque 返回 false 碰巧部分一致但
    // 行为语义错误、getCollisionShape 返回满方块），HoneyBlock 子类沦为死代码——与红石块/粘液块
    NaturalBlocks::HONEY_BLOCK = &registry.registerBlock<blocks::HoneyBlock>(ResourceLocation("minecraft:honey_block"),
        BlockProperties(Material::HONEY).hardness(0.0f).notSolid().soundType(BlockSoundTypes::HONEY).noOcclusion());

    // 仙人掌
    NaturalBlocks::CACTUS = &registry.registerBlock<blocks::CactusBlock>(ResourceLocation("minecraft:cactus"),
        BlockProperties(Material::PLANT).resistance(0.400000006f).hardness(0.4f).soundType(BlockSoundTypes::WOOL));

    // 枯萎灌木 - 干草类植物，可生长在沙/陶瓦/泥土/耕地上（沙漠/恶地生物群系）。
    // 改用 DryVegetationBlock 走 canSurvive 闸门，修复世界生成时浮空（此前注册为 SimpleBlock 致终判失效）。
    // 可被岩浆点燃（对齐 vanilla dead_bush）
    NaturalBlocks::DEAD_BUSH =
        &registry.registerBlock<blocks::DryVegetationBlock>(ResourceLocation("minecraft:dead_bush"),
            BlockProperties(Material::REPLACEABLE_PLANT).replaceable().noCollision().notSolid().ignitedByLava(),
            // 原版 DryVegetationBlock.SHAPE = column(12.0, 0.0, 13.0) -> box(2, 0, 2, 14, 13, 14)
            CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.8125f, 0.875f));

    // 睡莲 - 水生植物，下方须为水/冰且上方无流体。新建 WaterlilyBlock 走 canSurvive 闸门，
    // 修复世界生成时浮空（此前注册为 SimpleBlock 致终判失效）。
    NaturalBlocks::LILY_PAD = &registry.registerBlock<blocks::WaterlilyBlock>(ResourceLocation("minecraft:lily_pad"),
        BlockProperties(Material::PLANT).withCollision().notSolid().noOcclusion().soundType(BlockSoundTypes::LILY_PAD));

    // 藤蔓 - 可被岩浆点燃（对齐 vanilla vine）
    NaturalBlocks::VINE = &registry.registerBlock<blocks::VineBlock>(ResourceLocation("minecraft:vine"),
        BlockProperties(Material::REPLACEABLE_PLANT)
            .replaceable()
            .resistance(0.200000003f)
            .hardness(0.2f)
            .withCollision()
            .notSolid()
            .noOcclusion()
            .ignitedByLava()
            .soundType(BlockSoundTypes::VINE));

    // 蜘蛛网
    NaturalBlocks::COBWEB = &registry.registerBlock<blocks::WebBlock>(ResourceLocation("minecraft:cobweb"),
        BlockProperties(Material::WEB)
            .forceSolidOn()
            .resistance(4.0f)
            .hardness(4.0f)
            .noCollision()
            .soundType(BlockSoundTypes::COBWEB));

    // 甘蔗
    NaturalBlocks::SUGAR_CANE =
        &registry.registerBlock<blocks::SugarCaneBlock>(ResourceLocation("minecraft:sugar_cane"),
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    // 耕地
    NaturalBlocks::FARMLAND = &registry.registerBlock<blocks::FarmlandBlock>(ResourceLocation("minecraft:farmland"),
        BlockProperties(Material::EARTH)
            .resistance(0.600000024f)
            .hardness(0.6f)
            // 原版 farmland 属性显式 isSuffocating/isViewBlocking(Blocks::never)。
            // 碰撞比满格矮 1 像素，默认判据会给 false，但原版仍把它算作窒息/挡视线方块。
            .suffocating(block_registry::BlockStatePredicates::alwaysBlocking())
            .viewBlocking(block_registry::BlockStatePredicates::alwaysBlocking()));

    // 红沙
    NaturalBlocks::RED_SAND = &registry.registerBlock<blocks::FallingBlock>(ResourceLocation("minecraft:red_sand"),
        BlockProperties(Material::SAND).instrument(NoteBlockInstrument::Snare).resistance(0.5f).hardness(0.5f));

    // 干海带块
    //
    // 借用 `Material::PLANT` 只为挖掘手感与可燃性，但它在原版是**完整的实心方块**
    // （注册成普通 `Block`，玩家能站在上面）。
    // `Material::PLANT` 经 `.solid(false)` 推导出「无碰撞」，会把形状整个短路成空，
    // 必须显式 `withCollision()` 恢复。
    // 由差分能力 face_sturdy 查出（六个面全部判错）。
    NaturalBlocks::DRIED_KELP_BLOCK =
        &registry.registerBlock<blocks::DriedKelpBlock>(ResourceLocation("minecraft:dried_kelp_block"),
            BlockProperties(Material::PLANT).resistance(2.5f).hardness(0.5f).withCollision());

    // 海泡菜
    NaturalBlocks::SEA_PICKLE =
        &registry.registerBlock<blocks::SeaPickleBlock>(ResourceLocation("minecraft:sea_pickle"),
            BlockProperties(Material::OCEAN_PLANT)
                .withCollision()
                .notSolid()
                .noOcclusion()
                .lightLevel(6)
                .soundType(BlockSoundTypes::SLIME));

    // 海带顶部和海带茎
    NaturalBlocks::KELP = &registry.registerBlock<blocks::KelpBlock>(ResourceLocation("minecraft:kelp"),
        BlockProperties(Material::OCEAN_PLANT).noCollision().notSolid().soundType(BlockSoundTypes::WET_GRASS));
    NaturalBlocks::KELP_PLANT =
        &registry.registerBlock<blocks::KelpPlantBlock>(ResourceLocation("minecraft:kelp_plant"),
            BlockProperties(Material::OCEAN_PLANT).noCollision().notSolid().soundType(BlockSoundTypes::WET_GRASS));

    // 海草与高海草
    NaturalBlocks::SEAGRASS = &registry.registerBlock<blocks::SeagrassBlock>(ResourceLocation("minecraft:seagrass"),
        BlockProperties(Material::SEA_GRASS)
            .replaceable()
            .noCollision()
            .notSolid()
            .offsetType(BlockOffsetType::XZ)
            .soundType(BlockSoundTypes::WET_GRASS));
    NaturalBlocks::TALL_SEAGRASS =
        &registry.registerBlock<blocks::TallSeagrassBlock>(ResourceLocation("minecraft:tall_seagrass"),
            BlockProperties(Material::SEA_GRASS)
                .replaceable()
                .noCollision()
                .notSolid()
                .offsetType(BlockOffsetType::XZ)
                .soundType(BlockSoundTypes::WET_GRASS));

    // 气泡柱与海龟蛋
    // 气泡柱：水柱方块。BubbleColumnBlock::getFluidState 返回水（对齐 vanilla），propagatesSkylightDown
    // 走默认公式得 false（含水不透天空光）。因此 lightBlock 必须由默认公式得 1，不能显式设 0：
    // vanilla bubble_column 的 getShape 是空形状、不是实体渲染方块，但它含水源，
    // 天空光每穿过一格衰减 1 级（实测原版 1.21.11 lightBlock=1）。
    NaturalBlocks::BUBBLE_COLUMN =
        &registry.registerBlock<blocks::BubbleColumnBlock>(ResourceLocation("minecraft:bubble_column"),
            BlockProperties(Material::WATER).replaceable().noCollision().notSolid().soundType(BlockSoundTypes::AIR));
    NaturalBlocks::TURTLE_EGG =
        &registry.registerBlock<blocks::TurtleEggBlock>(ResourceLocation("minecraft:turtle_egg"),
            BlockProperties(Material::CORAL)
                .resistance(0.5f)
                .hardness(0.5f)
                .withCollision()

                .noOcclusion()
                .soundType(BlockSoundTypes::METAL)
                .forceSolidOn());

    // 蜜脾块 - 固体装饰方块（硬度 0.6，珊瑚块音效）
    NaturalBlocks::HONEYCOMB_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:honeycomb_block"),
        BlockProperties(Material::ORGANIC).resistance(0.600000024f).hardness(0.6f).soundType(BlockSoundTypes::CORAL));

    // 干燥恶魂 - 含水方块，遇水逐级提升湿润等级，满级孵化为幼年恶魂
    NaturalBlocks::DRIED_GHAST =
        &registry.registerBlock<blocks::DriedGhastBlock>(ResourceLocation("minecraft:dried_ghast"),
            BlockProperties(Material::ORGANIC)
                .hardness(0.0f)
                .withCollision()

                .noOcclusion()
                .tickRandomly()
                .soundType(BlockSoundTypes::DRIED_GHAST)
                .forceSolidOn());

    // ============================================================================
    // 生物头颅 / 头（站立 + 墙挂变体）
    // 站立头颅持有 ROTATION_0_15 + POWERED，墙挂头颅持有 HORIZONTAL_FACING + POWERED。
    // 头颅均持有 SkullBlockEntity（存储玩家档案 / 自定义名称 / 龙首与猪灵头张嘴动画）。
    // 头颅作为音符盒乐器：NoteBlock::resolveInstrument 经 headInstrumentAbove 识别上述头颅方块，
    //   映射为对应 MOB_HEAD/CUSTOM 乐器（骷髅/凋灵骷髅/僵尸/苦力怕/龙/猪灵/玩家头）。
    // ============================================================================
    // 立式头颅（_head）注册了 noOcclusion()；墙面变体（_wall_head / _wall_skull）走原版
    // `Blocks.wallVariant(...)`，只继承掉落表与描述，**不继承** noOcclusion()，
    // 因此它们的 canOcclude 保持默认 true。早期实现让两者共用同一份属性，
    // 把墙面头颅也注册成了无遮挡。
    const BlockProperties skullProps =
        BlockProperties(Material::DECORATION).withCollision().hardness(1.0f).notSolid().noOcclusion().resistance(1.0f);
    // 墙壁头颅有碰撞箱（与原版一致）：Material::DECORATION 默认 blocksMovement 为 false，
    // 必须显式 withCollision()。
    const BlockProperties wallSkullProps =
        BlockProperties(Material::DECORATION).withCollision().hardness(1.0f).notSolid().resistance(1.0f);

    NaturalBlocks::SKELETON_SKULL = &registry.registerBlock<blocks::SkullBlock>(
        ResourceLocation("minecraft:skeleton_skull"), blocks::SkullType::Skeleton, skullProps);
    NaturalBlocks::SKELETON_WALL_SKULL = &registry.registerBlock<blocks::WallSkullBlock>(
        ResourceLocation("minecraft:skeleton_wall_skull"), blocks::SkullType::Skeleton, wallSkullProps);

    NaturalBlocks::WITHER_SKELETON_SKULL = &registry.registerBlock<blocks::WitherSkullBlock>(
        ResourceLocation("minecraft:wither_skeleton_skull"), skullProps);
    NaturalBlocks::WITHER_SKELETON_WALL_SKULL = &registry.registerBlock<blocks::WitherWallSkullBlock>(
        ResourceLocation("minecraft:wither_skeleton_wall_skull"), wallSkullProps);

    NaturalBlocks::PLAYER_HEAD =
        &registry.registerBlock<blocks::PlayerHeadBlock>(ResourceLocation("minecraft:player_head"), skullProps);
    NaturalBlocks::PLAYER_WALL_HEAD = &registry.registerBlock<blocks::PlayerWallHeadBlock>(
        ResourceLocation("minecraft:player_wall_head"), wallSkullProps);

    NaturalBlocks::ZOMBIE_HEAD = &registry.registerBlock<blocks::SkullBlock>(
        ResourceLocation("minecraft:zombie_head"), blocks::SkullType::Zombie, skullProps);
    NaturalBlocks::ZOMBIE_WALL_HEAD = &registry.registerBlock<blocks::WallSkullBlock>(
        ResourceLocation("minecraft:zombie_wall_head"), blocks::SkullType::Zombie, wallSkullProps);

    NaturalBlocks::CREEPER_HEAD = &registry.registerBlock<blocks::SkullBlock>(
        ResourceLocation("minecraft:creeper_head"), blocks::SkullType::Creeper, skullProps);
    NaturalBlocks::CREEPER_WALL_HEAD = &registry.registerBlock<blocks::WallSkullBlock>(
        ResourceLocation("minecraft:creeper_wall_head"), blocks::SkullType::Creeper, wallSkullProps);

    NaturalBlocks::DRAGON_HEAD = &registry.registerBlock<blocks::SkullBlock>(
        ResourceLocation("minecraft:dragon_head"), blocks::SkullType::Dragon, skullProps);
    NaturalBlocks::DRAGON_WALL_HEAD = &registry.registerBlock<blocks::WallSkullBlock>(
        ResourceLocation("minecraft:dragon_wall_head"), blocks::SkullType::Dragon, wallSkullProps);

    NaturalBlocks::PIGLIN_HEAD = &registry.registerBlock<blocks::SkullBlock>(
        ResourceLocation("minecraft:piglin_head"), blocks::SkullType::Piglin, skullProps);
    NaturalBlocks::PIGLIN_WALL_HEAD = &registry.registerBlock<blocks::PiglinWallSkullBlock>(
        ResourceLocation("minecraft:piglin_wall_head"), wallSkullProps);

    // 珊瑚（补齐死亡变种，便于海洋废墟/暖海装饰复用）
    const u32 deadFallbackId = BaseBlocks::AIR ? BaseBlocks::AIR->blockId() : 0;
    // 珊瑚块与珊瑚（活体）的声音组：方块体用 `SoundType.CORAL_BLOCK`，珊瑚/珊瑚扇用
    // `SoundType.WET_GRASS`——原版它们都不走材质默认的石头声。
    const BlockProperties coralBlockProps = BlockProperties(Material::CORAL)
                                                .hardness(1.5f)
                                                .resistance(6.0f)
                                                .soundType(BlockSoundTypes::CORAL_BLOCK)
                                                .instrument(NoteBlockInstrument::BaseDrum);
    const BlockProperties coralPlantProps = BlockProperties(Material::CORAL)
                                                .hardness(0.0f)
                                                .noCollision()
                                                .notSolid()
                                                .noOcclusion()
                                                .soundType(BlockSoundTypes::WET_GRASS);
    const BlockProperties deadCoralBlockProps =
        BlockProperties(Material::CORAL).hardness(1.5f).resistance(6.0f).instrument(NoteBlockInstrument::BaseDrum);
    const BlockProperties deadCoralPlantProps = BlockProperties(Material::CORAL)
                                                    .hardness(0.0f)
                                                    .noCollision()
                                                    .noOcclusion()
                                                    .forceSolidOn()
                                                    .instrument(NoteBlockInstrument::BaseDrum);

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

    // 死珊瑚方块体（dead_*_coral_block）不会因缺水而死亡（CoralBlockBlock 无 m_deadBlock 字段）。
    NaturalBlocks::DEAD_TUBE_CORAL = &registry.registerBlock<blocks::CoralPlantBlock>(
        ResourceLocation("minecraft:dead_tube_coral"), blocks::CoralColor::Tube, deadFallbackId, deadCoralPlantProps);
    NaturalBlocks::DEAD_BRAIN_CORAL = &registry.registerBlock<blocks::CoralPlantBlock>(
        ResourceLocation("minecraft:dead_brain_coral"), blocks::CoralColor::Brain, deadFallbackId, deadCoralPlantProps);
    NaturalBlocks::DEAD_BUBBLE_CORAL =
        &registry.registerBlock<blocks::CoralPlantBlock>(ResourceLocation("minecraft:dead_bubble_coral"),
            blocks::CoralColor::Bubble,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_FIRE_CORAL = &registry.registerBlock<blocks::CoralPlantBlock>(
        ResourceLocation("minecraft:dead_fire_coral"), blocks::CoralColor::Fire, deadFallbackId, deadCoralPlantProps);
    NaturalBlocks::DEAD_HORN_CORAL = &registry.registerBlock<blocks::CoralPlantBlock>(
        ResourceLocation("minecraft:dead_horn_coral"), blocks::CoralColor::Horn, deadFallbackId, deadCoralPlantProps);

    NaturalBlocks::DEAD_TUBE_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_tube_coral_fan"),
            blocks::CoralColor::Tube,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_BRAIN_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_brain_coral_fan"),
            blocks::CoralColor::Brain,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_BUBBLE_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_bubble_coral_fan"),
            blocks::CoralColor::Bubble,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_FIRE_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_fire_coral_fan"),
            blocks::CoralColor::Fire,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_HORN_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:dead_horn_coral_fan"),
            blocks::CoralColor::Horn,
            deadFallbackId,
            deadCoralPlantProps);

    // 活珊瑚扇脱水死亡应变为对应的死珊瑚扇（CoralFanBlock 构造传入的死珊瑚扇即为死亡目标方块）。
    // 此前误传 dead_*_coral_block（死珊瑚方块体）的 id，导致珊瑚扇死亡变成死珊瑚方块体。
    const u32 deadTubeFanBlockId =
        NaturalBlocks::DEAD_TUBE_CORAL_FAN ? NaturalBlocks::DEAD_TUBE_CORAL_FAN->blockId() : deadFallbackId;
    const u32 deadBrainFanBlockId =
        NaturalBlocks::DEAD_BRAIN_CORAL_FAN ? NaturalBlocks::DEAD_BRAIN_CORAL_FAN->blockId() : deadFallbackId;
    const u32 deadBubbleFanBlockId =
        NaturalBlocks::DEAD_BUBBLE_CORAL_FAN ? NaturalBlocks::DEAD_BUBBLE_CORAL_FAN->blockId() : deadFallbackId;
    const u32 deadFireFanBlockId =
        NaturalBlocks::DEAD_FIRE_CORAL_FAN ? NaturalBlocks::DEAD_FIRE_CORAL_FAN->blockId() : deadFallbackId;
    const u32 deadHornFanBlockId =
        NaturalBlocks::DEAD_HORN_CORAL_FAN ? NaturalBlocks::DEAD_HORN_CORAL_FAN->blockId() : deadFallbackId;

    NaturalBlocks::DEAD_TUBE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_tube_coral_wall_fan"),
            blocks::CoralColor::Tube,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_BRAIN_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_brain_coral_wall_fan"),
            blocks::CoralColor::Brain,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_BUBBLE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_bubble_coral_wall_fan"),
            blocks::CoralColor::Bubble,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_FIRE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_fire_coral_wall_fan"),
            blocks::CoralColor::Fire,
            deadFallbackId,
            deadCoralPlantProps);
    NaturalBlocks::DEAD_HORN_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:dead_horn_coral_wall_fan"),
            blocks::CoralColor::Horn,
            deadFallbackId,
            deadCoralPlantProps);

    // 活墙珊瑚扇脱水死亡应变为对应的死墙珊瑚扇（构造传入的死墙珊瑚扇即为死亡目标方块）。
    const u32 deadTubeWallFanBlockId =
        NaturalBlocks::DEAD_TUBE_CORAL_WALL_FAN ? NaturalBlocks::DEAD_TUBE_CORAL_WALL_FAN->blockId() : deadFallbackId;
    const u32 deadBrainWallFanBlockId =
        NaturalBlocks::DEAD_BRAIN_CORAL_WALL_FAN ? NaturalBlocks::DEAD_BRAIN_CORAL_WALL_FAN->blockId() : deadFallbackId;
    const u32 deadBubbleWallFanBlockId = NaturalBlocks::DEAD_BUBBLE_CORAL_WALL_FAN
        ? NaturalBlocks::DEAD_BUBBLE_CORAL_WALL_FAN->blockId()
        : deadFallbackId;
    const u32 deadFireWallFanBlockId =
        NaturalBlocks::DEAD_FIRE_CORAL_WALL_FAN ? NaturalBlocks::DEAD_FIRE_CORAL_WALL_FAN->blockId() : deadFallbackId;
    const u32 deadHornWallFanBlockId =
        NaturalBlocks::DEAD_HORN_CORAL_WALL_FAN ? NaturalBlocks::DEAD_HORN_CORAL_WALL_FAN->blockId() : deadFallbackId;

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

    // 活珊瑚脱水死亡应变为对应的死珊瑚（构造传入的死珊瑚即为死亡目标方块）。
    const u32 deadTubeCoralBlockId =
        NaturalBlocks::DEAD_TUBE_CORAL ? NaturalBlocks::DEAD_TUBE_CORAL->blockId() : deadFallbackId;
    const u32 deadBrainCoralBlockId =
        NaturalBlocks::DEAD_BRAIN_CORAL ? NaturalBlocks::DEAD_BRAIN_CORAL->blockId() : deadFallbackId;
    const u32 deadBubbleCoralBlockId =
        NaturalBlocks::DEAD_BUBBLE_CORAL ? NaturalBlocks::DEAD_BUBBLE_CORAL->blockId() : deadFallbackId;
    const u32 deadFireCoralBlockId =
        NaturalBlocks::DEAD_FIRE_CORAL ? NaturalBlocks::DEAD_FIRE_CORAL->blockId() : deadFallbackId;
    const u32 deadHornCoralBlockId =
        NaturalBlocks::DEAD_HORN_CORAL ? NaturalBlocks::DEAD_HORN_CORAL->blockId() : deadFallbackId;

    NaturalBlocks::TUBE_CORAL = &registry.registerBlock<blocks::CoralPlantBlock>(
        ResourceLocation("minecraft:tube_coral"), blocks::CoralColor::Tube, deadTubeCoralBlockId, coralPlantProps);
    NaturalBlocks::BRAIN_CORAL = &registry.registerBlock<blocks::CoralPlantBlock>(
        ResourceLocation("minecraft:brain_coral"), blocks::CoralColor::Brain, deadBrainCoralBlockId, coralPlantProps);
    NaturalBlocks::BUBBLE_CORAL =
        &registry.registerBlock<blocks::CoralPlantBlock>(ResourceLocation("minecraft:bubble_coral"),
            blocks::CoralColor::Bubble,
            deadBubbleCoralBlockId,
            coralPlantProps);
    NaturalBlocks::FIRE_CORAL = &registry.registerBlock<blocks::CoralPlantBlock>(
        ResourceLocation("minecraft:fire_coral"), blocks::CoralColor::Fire, deadFireCoralBlockId, coralPlantProps);
    NaturalBlocks::HORN_CORAL = &registry.registerBlock<blocks::CoralPlantBlock>(
        ResourceLocation("minecraft:horn_coral"), blocks::CoralColor::Horn, deadHornCoralBlockId, coralPlantProps);

    NaturalBlocks::TUBE_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:tube_coral_fan"), blocks::CoralColor::Tube, deadTubeFanBlockId, coralPlantProps);
    NaturalBlocks::BRAIN_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:brain_coral_fan"), blocks::CoralColor::Brain, deadBrainFanBlockId, coralPlantProps);
    NaturalBlocks::BUBBLE_CORAL_FAN =
        &registry.registerBlock<blocks::CoralFanBlock>(ResourceLocation("minecraft:bubble_coral_fan"),
            blocks::CoralColor::Bubble,
            deadBubbleFanBlockId,
            coralPlantProps);
    NaturalBlocks::FIRE_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:fire_coral_fan"), blocks::CoralColor::Fire, deadFireFanBlockId, coralPlantProps);
    NaturalBlocks::HORN_CORAL_FAN = &registry.registerBlock<blocks::CoralFanBlock>(
        ResourceLocation("minecraft:horn_coral_fan"), blocks::CoralColor::Horn, deadHornFanBlockId, coralPlantProps);

    NaturalBlocks::TUBE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:tube_coral_wall_fan"),
            blocks::CoralColor::Tube,
            deadTubeWallFanBlockId,
            coralPlantProps);
    NaturalBlocks::BRAIN_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:brain_coral_wall_fan"),
            blocks::CoralColor::Brain,
            deadBrainWallFanBlockId,
            coralPlantProps);
    NaturalBlocks::BUBBLE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:bubble_coral_wall_fan"),
            blocks::CoralColor::Bubble,
            deadBubbleWallFanBlockId,
            coralPlantProps);
    NaturalBlocks::FIRE_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:fire_coral_wall_fan"),
            blocks::CoralColor::Fire,
            deadFireWallFanBlockId,
            coralPlantProps);
    NaturalBlocks::HORN_CORAL_WALL_FAN =
        &registry.registerBlock<blocks::CoralWallFanBlock>(ResourceLocation("minecraft:horn_coral_wall_fan"),
            blocks::CoralColor::Horn,
            deadHornWallFanBlockId,
            coralPlantProps);

    // 潮涌核心 - 水下信标类方块，需要潮涌框架激活
    NaturalBlocks::CONDUIT = &registry.registerBlock<blocks::ConduitBlock>(ResourceLocation("minecraft:conduit"),
        BlockProperties(Material::GLASS)
            .instrument(NoteBlockInstrument::Hat)
            .hardness(3.0f)
            .resistance(3.0f)

            .soundType(BlockSoundTypes::STONE)
            .noOcclusion()
            .forceSolidOn());

    // ========== 火把 ==========
    // 火把 - 发光等级14，生成火焰和烟雾粒子
    // 原版火把带 OffsetType.XZ：外形按坐标哈希做水平抖动（每格最多 ±1/8 格）。
    NaturalBlocks::TORCH = &registry.registerBlock<blocks::TorchBlock>(ResourceLocation("minecraft:torch"),
        BlockProperties(Material::DECORATION)
            .noCollision()
            .notSolid()
            .lightLevel(14)
            .soundType(BlockSoundTypes::WOOD)
            .offsetType(BlockOffsetType::XZ),
        particle::ParticleTypeId::Flame);

    // 墙上的火把 - 附着在墙上，根据朝向有不同碰撞箱和粒子位置
    NaturalBlocks::WALL_TORCH = &registry.registerBlock<blocks::WallTorchBlock>(
        ResourceLocation("minecraft:wall_torch"),
        BlockProperties(Material::DECORATION).noCollision().notSolid().lightLevel(14).soundType(BlockSoundTypes::WOOD),
        particle::ParticleTypeId::Flame);

    // ========== 蜂巢/蜂箱 ==========
    // 蜂巢 - 自然生成于树上，材质较软
    NaturalBlocks::BEE_NEST = &registry.registerBlock<blocks::BeehiveBlock>(ResourceLocation("minecraft:bee_nest"),
        BlockProperties(Material::WOOD)
            .instrument(NoteBlockInstrument::Bass)
            .hardness(0.3f)
            .resistance(0.3f)
            .flammable()
            .ignitedByLava());

    // 蜂箱 - 玩家合成，用木板制作，需要斧头加速挖掘
    NaturalBlocks::BEEHIVE = &registry.registerBlock<blocks::BeehiveBlock>(ResourceLocation("minecraft:beehive"),
        BlockProperties(Material::WOOD)
            .instrument(NoteBlockInstrument::Bass)
            .hardness(0.6f)
            .resistance(0.6f)
            .flammable()
            .ignitedByLava()
            .harvestTool(HarvestTool::Axe));
}

} // namespace block_registry
} // namespace mc
