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

#include "world/block/registry/VegetationBlocks.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/agricultural/MelonPumpkinBlocks.hpp"
#include "world/block/blocks/vegetation/BambooBlock.hpp"
#include "world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "world/block/blocks/vegetation/FlowerBlock.hpp"
#include "world/block/blocks/vegetation/LeavesBlock.hpp"
#include "world/block/blocks/vegetation/MushroomBlock.hpp"
#include "world/block/blocks/vegetation/SaplingBlock.hpp"
#include "world/block/blocks/vegetation/SugarCaneBlock.hpp"
#include "world/block/blocks/vegetation/SweetBerryBushBlock.hpp"
#include "world/block/blocks/vegetation/TallGrassBlock.hpp"
#include "world/block/blocks/vegetation/TreeGenerators.hpp"

namespace mc {
namespace block_registry {

// 植被方块
Block* VegetationBlocks::SHORT_GRASS = nullptr;
Block* VegetationBlocks::TALL_GRASS = nullptr;
Block* VegetationBlocks::FERN = nullptr;
Block* VegetationBlocks::DANDELION = nullptr;
Block* VegetationBlocks::POPPY = nullptr;
Block* VegetationBlocks::BLUE_ORCHID = nullptr;
Block* VegetationBlocks::ALLIUM = nullptr;
Block* VegetationBlocks::AZURE_BLUET = nullptr;
Block* VegetationBlocks::RED_TULIP = nullptr;
Block* VegetationBlocks::ORANGE_TULIP = nullptr;
Block* VegetationBlocks::WHITE_TULIP = nullptr;
Block* VegetationBlocks::PINK_TULIP = nullptr;
Block* VegetationBlocks::OXEYE_DAISY = nullptr;
Block* VegetationBlocks::LILY_OF_THE_VALLEY = nullptr;
Block* VegetationBlocks::SUNFLOWER = nullptr;
Block* VegetationBlocks::LILAC = nullptr;
Block* VegetationBlocks::ROSE_BUSH = nullptr;
Block* VegetationBlocks::PEONY = nullptr;
Block* VegetationBlocks::LARGE_FERN = nullptr;
Block* VegetationBlocks::CORNFLOWER = nullptr;
Block* VegetationBlocks::WITHER_ROSE = nullptr;
Block* VegetationBlocks::BROWN_MUSHROOM = nullptr;
Block* VegetationBlocks::RED_MUSHROOM = nullptr;
Block* VegetationBlocks::BROWN_MUSHROOM_BLOCK = nullptr;
Block* VegetationBlocks::RED_MUSHROOM_BLOCK = nullptr;
Block* VegetationBlocks::MUSHROOM_STEM = nullptr;

// 树苗
Block* VegetationBlocks::OAK_SAPLING = nullptr;
Block* VegetationBlocks::SPRUCE_SAPLING = nullptr;
Block* VegetationBlocks::BIRCH_SAPLING = nullptr;
Block* VegetationBlocks::JUNGLE_SAPLING = nullptr;
Block* VegetationBlocks::ACACIA_SAPLING = nullptr;
Block* VegetationBlocks::DARK_OAK_SAPLING = nullptr;

// 南瓜和西瓜系列
Block* VegetationBlocks::MELON = nullptr;
Block* VegetationBlocks::PUMPKIN = nullptr;
Block* VegetationBlocks::CARVED_PUMPKIN = nullptr;
Block* VegetationBlocks::MELON_STEM = nullptr;
Block* VegetationBlocks::PUMPKIN_STEM = nullptr;
Block* VegetationBlocks::ATTACHED_MELON_STEM = nullptr;
Block* VegetationBlocks::ATTACHED_PUMPKIN_STEM = nullptr;

// 竹子
Block* VegetationBlocks::BAMBOO = nullptr;
Block* VegetationBlocks::BAMBOO_SAPLING = nullptr;

// 甜浆果丛
Block* VegetationBlocks::SWEET_BERRY_BUSH = nullptr;

void registerVegetationBlocks()
{
    auto& registry = BlockRegistry::instance();

    // 草和蕨的属性
    BlockProperties grassProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    // 矮草
    VegetationBlocks::SHORT_GRASS =
        &registry.registerBlock<blocks::TallGrassBlock>(ResourceLocation("minecraft:short_grass"), grassProps);

    // 高草（双格植物，half 属性区分上下半部分）
    VegetationBlocks::TALL_GRASS =
        &registry.registerBlock<blocks::DoublePlantBlock>(ResourceLocation("minecraft:tall_grass"), grassProps);

    // 蕨
    VegetationBlocks::FERN = &registry.registerBlock<blocks::FernBlock>(ResourceLocation("minecraft:fern"), grassProps);

    // 花朵属性
    BlockProperties flowerProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    // 蒲公英 → 饱和 0.35秒（7 tick，瞬间效果）
    VegetationBlocks::DANDELION = &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:dandelion"),
        flowerProps,
        static_cast<u32>(entity::effect::EffectType::Saturation),
        0);

    // 虞美人 → 夜视 4秒（80 tick）
    VegetationBlocks::POPPY = &registry.registerBlock<blocks::FlowerBlock>(
        ResourceLocation("minecraft:poppy"), flowerProps, static_cast<u32>(entity::effect::EffectType::NightVision), 4);

    // 兰花 → 饱和 0.35秒（7 tick，瞬间效果）
    VegetationBlocks::BLUE_ORCHID =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:blue_orchid"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::Saturation),
            0);

    // 绒球葱 → 防火 4秒（80 tick）
    VegetationBlocks::ALLIUM = &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:allium"),
        flowerProps,
        static_cast<u32>(entity::effect::EffectType::FireResistance),
        4);

    // 蓝花美耳草 → 失明 6秒（120 tick）
    VegetationBlocks::AZURE_BLUET =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:azure_bluet"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::Blindness),
            6);

    // 郁金香系列
    // 红色郁金香 → 虚弱 7秒（140 tick）
    VegetationBlocks::RED_TULIP = &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:red_tulip"),
        flowerProps,
        static_cast<u32>(entity::effect::EffectType::Weakness),
        7);
    // 橙色郁金香 → 虚弱 7秒（140 tick）
    VegetationBlocks::ORANGE_TULIP =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:orange_tulip"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::Weakness),
            7);
    // 白色郁金香 → 中毒 8秒（160 tick）
    VegetationBlocks::WHITE_TULIP =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:white_tulip"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::Poison),
            8);
    // 粉色郁金香 → 虚弱 7秒（140 tick）
    VegetationBlocks::PINK_TULIP =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:pink_tulip"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::Weakness),
            7);

    // 滨菊 → 生命恢复 6秒（120 tick）
    VegetationBlocks::OXEYE_DAISY =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:oxeye_daisy"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::Regeneration),
            6);

    // 铃兰 → 中毒 8秒（160 tick）
    VegetationBlocks::LILY_OF_THE_VALLEY =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:lily_of_the_valley"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::Poison),
            8);

    // 矢车菊 → 跳跃提升 4秒（80 tick）
    VegetationBlocks::CORNFLOWER =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:cornflower"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::JumpBoost),
            4);

    // 凋零玫瑰 → 凋零 6秒（120 tick）
    VegetationBlocks::WITHER_ROSE =
        &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:wither_rose"),
            flowerProps,
            static_cast<u32>(entity::effect::EffectType::Wither),
            6);

    // 高花属性（双高植物）
    BlockProperties tallFlowerProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    // 向日葵
    VegetationBlocks::SUNFLOWER =
        &registry.registerBlock<blocks::SunflowerBlock>(ResourceLocation("minecraft:sunflower"), tallFlowerProps);

    // 丁香
    VegetationBlocks::LILAC =
        &registry.registerBlock<blocks::LilacBlock>(ResourceLocation("minecraft:lilac"), tallFlowerProps);

    // 玫瑰丛
    VegetationBlocks::ROSE_BUSH =
        &registry.registerBlock<blocks::RoseBushBlock>(ResourceLocation("minecraft:rose_bush"), tallFlowerProps);

    // 牡丹
    VegetationBlocks::PEONY =
        &registry.registerBlock<blocks::PeonyBlock>(ResourceLocation("minecraft:peony"), tallFlowerProps);

    // 大型蕨 - 双格高的蕨类植物
    VegetationBlocks::LARGE_FERN =
        &registry.registerBlock<blocks::LargeFernBlock>(ResourceLocation("minecraft:large_fern"), tallFlowerProps);

    // 蘑菇属性
    BlockProperties mushroomProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().lightLevel(1);

    // 棕色蘑菇
    VegetationBlocks::BROWN_MUSHROOM =
        &registry.registerBlock<blocks::MushroomBlock>(ResourceLocation("minecraft:brown_mushroom"), mushroomProps);

    // 红色蘑菇
    VegetationBlocks::RED_MUSHROOM =
        &registry.registerBlock<blocks::MushroomBlock>(ResourceLocation("minecraft:red_mushroom"), mushroomProps);

    // 巨型蘑菇方块属性
    BlockProperties hugeMushroomProps = BlockProperties(Material::WOOD).hardness(0.2f);

    // 棕色蘑菇方块
    VegetationBlocks::BROWN_MUSHROOM_BLOCK = &registry.registerBlock<blocks::HugeMushroomBlock>(
        ResourceLocation("minecraft:brown_mushroom_block"), hugeMushroomProps);

    // 红色蘑菇方块
    VegetationBlocks::RED_MUSHROOM_BLOCK = &registry.registerBlock<blocks::HugeMushroomBlock>(
        ResourceLocation("minecraft:red_mushroom_block"), hugeMushroomProps);

    // 蘑菇柄
    VegetationBlocks::MUSHROOM_STEM = &registry.registerBlock<blocks::HugeMushroomBlock>(
        ResourceLocation("minecraft:mushroom_stem"), hugeMushroomProps);

    // 树苗属性
    BlockProperties saplingProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    VegetationBlocks::OAK_SAPLING = &registry.registerBlock<blocks::SaplingBlock>(
        ResourceLocation("minecraft:oak_sapling"), blocks::TreeGenerators::oakTree(), saplingProps);
    VegetationBlocks::SPRUCE_SAPLING = &registry.registerBlock<blocks::SaplingBlock>(
        ResourceLocation("minecraft:spruce_sapling"), blocks::TreeGenerators::spruceTree(), saplingProps);
    VegetationBlocks::BIRCH_SAPLING = &registry.registerBlock<blocks::SaplingBlock>(
        ResourceLocation("minecraft:birch_sapling"), blocks::TreeGenerators::birchTree(), saplingProps);
    VegetationBlocks::JUNGLE_SAPLING = &registry.registerBlock<blocks::SaplingBlock>(
        ResourceLocation("minecraft:jungle_sapling"), blocks::TreeGenerators::jungleTree(), saplingProps);
    VegetationBlocks::ACACIA_SAPLING = &registry.registerBlock<blocks::SaplingBlock>(
        ResourceLocation("minecraft:acacia_sapling"), blocks::TreeGenerators::acaciaTree(), saplingProps);
    VegetationBlocks::DARK_OAK_SAPLING = &registry.registerBlock<blocks::SaplingBlock>(
        ResourceLocation("minecraft:dark_oak_sapling"), blocks::TreeGenerators::darkOakTree(), saplingProps);

    // 竹子属性
    BlockProperties bambooProps = BlockProperties(Material::BAMBOO).hardness(1.0f).notSolid();

    // 竹子
    VegetationBlocks::BAMBOO =
        &registry.registerBlock<blocks::BambooBlock>(ResourceLocation("minecraft:bamboo"), bambooProps);

    // 竹子幼苗属性
    BlockProperties bambooSaplingProps = BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid();

    // 竹子幼苗
    VegetationBlocks::BAMBOO_SAPLING = &registry.registerBlock<blocks::BambooSaplingBlock>(
        ResourceLocation("minecraft:bamboo_sapling"), bambooSaplingProps);

    // ========== 南瓜和西瓜系列 ==========

    // 先注册雕刻南瓜 - 有朝向属性，可生成傀儡
    VegetationBlocks::CARVED_PUMPKIN = &registry.registerBlock<blocks::CarvedPumpkinBlock>(
        ResourceLocation("minecraft:carved_pumpkin"), BlockProperties(Material::EARTH).hardness(1.0f));

    // 南瓜 - 可用剪刀雕刻成雕刻南瓜
    // 注意：stem 和 attachedStem 由于循环依赖，构造时传入 nullptr，在下方茎方块注册后通过 setStem/setAttachedStem 回填
    VegetationBlocks::PUMPKIN = &registry.registerBlock<blocks::PumpkinBlock>(ResourceLocation("minecraft:pumpkin"),
        nullptr,                          // stem - 下方 PUMPKIN_STEM 注册后回填
        nullptr,                          // attachedStem - 下方 ATTACHED_PUMPKIN_STEM 注册后回填
        VegetationBlocks::CARVED_PUMPKIN, // carvedPumpkin - 已注册的雕刻南瓜
        BlockProperties(Material::EARTH).hardness(1.0f));

    // 西瓜方块
    // 注意：stem 和 attachedStem 由于循环依赖，构造时传入 nullptr，在下方茎方块注册后通过 setStem/setAttachedStem 回填
    VegetationBlocks::MELON = &registry.registerBlock<blocks::MelonBlock>(ResourceLocation("minecraft:melon"),
        nullptr, // stem - 下方 MELON_STEM 注册后回填
        nullptr, // attachedStem - 下方 ATTACHED_MELON_STEM 注册后回填
        BlockProperties(Material::EARTH).hardness(1.0f));

    // 注册茎方块（可以引用已注册的果实方块）

    // 南瓜茎
    VegetationBlocks::PUMPKIN_STEM =
        &registry.registerBlock<blocks::PumpkinStemBlock>(ResourceLocation("minecraft:pumpkin_stem"),
            static_cast<const blocks::StemGrownBlock*>(VegetationBlocks::PUMPKIN), // 果实已注册
            BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    // 连接南瓜茎（南瓜生成后茎变成的方块）
    VegetationBlocks::ATTACHED_PUMPKIN_STEM =
        &registry.registerBlock<blocks::PumpkinAttachedStemBlock>(ResourceLocation("minecraft:attached_pumpkin_stem"),
            static_cast<const blocks::StemGrownBlock*>(VegetationBlocks::PUMPKIN), // 果实已注册
            BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    // 西瓜茎
    VegetationBlocks::MELON_STEM =
        &registry.registerBlock<blocks::MelonStemBlock>(ResourceLocation("minecraft:melon_stem"),
            static_cast<const blocks::StemGrownBlock*>(VegetationBlocks::MELON), // 果实已注册
            BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    // 连接西瓜茎（西瓜生成后茎变成的方块）
    VegetationBlocks::ATTACHED_MELON_STEM =
        &registry.registerBlock<blocks::MelonAttachedStemBlock>(ResourceLocation("minecraft:attached_melon_stem"),
            static_cast<const blocks::StemGrownBlock*>(VegetationBlocks::MELON), // 果实已注册
            BlockProperties(Material::PLANT).hardness(0.0f).noCollision().notSolid());

    // 更新果实方块的茎指针（解决循环依赖）
    static_cast<blocks::PumpkinBlock*>(VegetationBlocks::PUMPKIN)->setStem(VegetationBlocks::PUMPKIN_STEM);
    static_cast<blocks::PumpkinBlock*>(VegetationBlocks::PUMPKIN)
        ->setAttachedStem(VegetationBlocks::ATTACHED_PUMPKIN_STEM);
    static_cast<blocks::MelonBlock*>(VegetationBlocks::MELON)->setStem(VegetationBlocks::MELON_STEM);
    static_cast<blocks::MelonBlock*>(VegetationBlocks::MELON)->setAttachedStem(VegetationBlocks::ATTACHED_MELON_STEM);

    // ========== 甜浆果丛 ==========

    // 甜浆果丛 - 4个生长阶段（AGE_0_3），实体碰撞减速/伤害，右键可采摘
    // 可种在草地、泥土、砂土、灰化土、耕地上
    VegetationBlocks::SWEET_BERRY_BUSH =
        &registry.registerBlock<blocks::SweetBerryBushBlock>(ResourceLocation("minecraft:sweet_berry_bush"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .soundType(BlockSoundTypes::SWEET_BERRY_BUSH));
}

} // namespace block_registry
} // namespace mc
