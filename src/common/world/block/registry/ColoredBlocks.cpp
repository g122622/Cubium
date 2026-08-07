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

#include "world/block/registry/ColoredBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/ConcretePowderBlock.hpp"
#include "world/block/blocks/ShulkerBoxBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/decorative/CarpetBlock.hpp"
#include "world/block/blocks/decorative/GlazedTerracottaBlock.hpp"
#include "world/block/blocks/decorative/StainedGlassBlock.hpp"
#include "world/block/blocks/functional/BedBlock.hpp"

namespace mc {
namespace block_registry {

// 羊毛 (16色)
Block* ColoredBlocks::WHITE_WOOL = nullptr;
Block* ColoredBlocks::ORANGE_WOOL = nullptr;
Block* ColoredBlocks::MAGENTA_WOOL = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_WOOL = nullptr;
Block* ColoredBlocks::YELLOW_WOOL = nullptr;
Block* ColoredBlocks::LIME_WOOL = nullptr;
Block* ColoredBlocks::PINK_WOOL = nullptr;
Block* ColoredBlocks::GRAY_WOOL = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_WOOL = nullptr;
Block* ColoredBlocks::CYAN_WOOL = nullptr;
Block* ColoredBlocks::PURPLE_WOOL = nullptr;
Block* ColoredBlocks::BLUE_WOOL = nullptr;
Block* ColoredBlocks::BROWN_WOOL = nullptr;
Block* ColoredBlocks::GREEN_WOOL = nullptr;
Block* ColoredBlocks::RED_WOOL = nullptr;
Block* ColoredBlocks::BLACK_WOOL = nullptr;

// 地毯 (16色)
Block* ColoredBlocks::WHITE_CARPET = nullptr;
Block* ColoredBlocks::ORANGE_CARPET = nullptr;
Block* ColoredBlocks::MAGENTA_CARPET = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_CARPET = nullptr;
Block* ColoredBlocks::YELLOW_CARPET = nullptr;
Block* ColoredBlocks::LIME_CARPET = nullptr;
Block* ColoredBlocks::PINK_CARPET = nullptr;
Block* ColoredBlocks::GRAY_CARPET = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_CARPET = nullptr;
Block* ColoredBlocks::CYAN_CARPET = nullptr;
Block* ColoredBlocks::PURPLE_CARPET = nullptr;
Block* ColoredBlocks::BLUE_CARPET = nullptr;
Block* ColoredBlocks::BROWN_CARPET = nullptr;
Block* ColoredBlocks::GREEN_CARPET = nullptr;
Block* ColoredBlocks::RED_CARPET = nullptr;
Block* ColoredBlocks::BLACK_CARPET = nullptr;

// 染色玻璃 (16色)
Block* ColoredBlocks::WHITE_STAINED_GLASS = nullptr;
Block* ColoredBlocks::ORANGE_STAINED_GLASS = nullptr;
Block* ColoredBlocks::MAGENTA_STAINED_GLASS = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_STAINED_GLASS = nullptr;
Block* ColoredBlocks::YELLOW_STAINED_GLASS = nullptr;
Block* ColoredBlocks::LIME_STAINED_GLASS = nullptr;
Block* ColoredBlocks::PINK_STAINED_GLASS = nullptr;
Block* ColoredBlocks::GRAY_STAINED_GLASS = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_STAINED_GLASS = nullptr;
Block* ColoredBlocks::CYAN_STAINED_GLASS = nullptr;
Block* ColoredBlocks::PURPLE_STAINED_GLASS = nullptr;
Block* ColoredBlocks::BLUE_STAINED_GLASS = nullptr;
Block* ColoredBlocks::BROWN_STAINED_GLASS = nullptr;
Block* ColoredBlocks::GREEN_STAINED_GLASS = nullptr;
Block* ColoredBlocks::RED_STAINED_GLASS = nullptr;
Block* ColoredBlocks::BLACK_STAINED_GLASS = nullptr;

// 混凝土 (16色)
Block* ColoredBlocks::WHITE_CONCRETE = nullptr;
Block* ColoredBlocks::ORANGE_CONCRETE = nullptr;
Block* ColoredBlocks::MAGENTA_CONCRETE = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_CONCRETE = nullptr;
Block* ColoredBlocks::YELLOW_CONCRETE = nullptr;
Block* ColoredBlocks::LIME_CONCRETE = nullptr;
Block* ColoredBlocks::PINK_CONCRETE = nullptr;
Block* ColoredBlocks::GRAY_CONCRETE = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_CONCRETE = nullptr;
Block* ColoredBlocks::CYAN_CONCRETE = nullptr;
Block* ColoredBlocks::PURPLE_CONCRETE = nullptr;
Block* ColoredBlocks::BLUE_CONCRETE = nullptr;
Block* ColoredBlocks::BROWN_CONCRETE = nullptr;
Block* ColoredBlocks::GREEN_CONCRETE = nullptr;
Block* ColoredBlocks::RED_CONCRETE = nullptr;
Block* ColoredBlocks::BLACK_CONCRETE = nullptr;

// 混凝土粉末 (16色)
Block* ColoredBlocks::WHITE_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::ORANGE_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::MAGENTA_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::YELLOW_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::LIME_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::PINK_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::GRAY_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::CYAN_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::PURPLE_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::BLUE_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::BROWN_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::GREEN_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::RED_CONCRETE_POWDER = nullptr;
Block* ColoredBlocks::BLACK_CONCRETE_POWDER = nullptr;

// 陶瓦 (16色 + 普通)
Block* ColoredBlocks::WHITE_TERRACOTTA = nullptr;
Block* ColoredBlocks::ORANGE_TERRACOTTA = nullptr;
Block* ColoredBlocks::MAGENTA_TERRACOTTA = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_TERRACOTTA = nullptr;
Block* ColoredBlocks::YELLOW_TERRACOTTA = nullptr;
Block* ColoredBlocks::LIME_TERRACOTTA = nullptr;
Block* ColoredBlocks::PINK_TERRACOTTA = nullptr;
Block* ColoredBlocks::GRAY_TERRACOTTA = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_TERRACOTTA = nullptr;
Block* ColoredBlocks::CYAN_TERRACOTTA = nullptr;
Block* ColoredBlocks::PURPLE_TERRACOTTA = nullptr;
Block* ColoredBlocks::BLUE_TERRACOTTA = nullptr;
Block* ColoredBlocks::BROWN_TERRACOTTA = nullptr;
Block* ColoredBlocks::GREEN_TERRACOTTA = nullptr;
Block* ColoredBlocks::RED_TERRACOTTA = nullptr;
Block* ColoredBlocks::BLACK_TERRACOTTA = nullptr;
Block* ColoredBlocks::TERRACOTTA = nullptr;

// 釉面陶瓦 (16色)
Block* ColoredBlocks::WHITE_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::ORANGE_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::MAGENTA_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::YELLOW_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::LIME_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::PINK_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::GRAY_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::CYAN_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::PURPLE_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::BLUE_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::BROWN_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::GREEN_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::RED_GLAZED_TERRACOTTA = nullptr;
Block* ColoredBlocks::BLACK_GLAZED_TERRACOTTA = nullptr;

// 床 (16色)
Block* ColoredBlocks::WHITE_BED = nullptr;
Block* ColoredBlocks::ORANGE_BED = nullptr;
Block* ColoredBlocks::MAGENTA_BED = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_BED = nullptr;
Block* ColoredBlocks::YELLOW_BED = nullptr;
Block* ColoredBlocks::LIME_BED = nullptr;
Block* ColoredBlocks::PINK_BED = nullptr;
Block* ColoredBlocks::GRAY_BED = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_BED = nullptr;
Block* ColoredBlocks::CYAN_BED = nullptr;
Block* ColoredBlocks::PURPLE_BED = nullptr;
Block* ColoredBlocks::BLUE_BED = nullptr;
Block* ColoredBlocks::BROWN_BED = nullptr;
Block* ColoredBlocks::GREEN_BED = nullptr;
Block* ColoredBlocks::RED_BED = nullptr;
Block* ColoredBlocks::BLACK_BED = nullptr;

// 潜影盒 (16色)
Block* ColoredBlocks::WHITE_SHULKER_BOX = nullptr;
Block* ColoredBlocks::ORANGE_SHULKER_BOX = nullptr;
Block* ColoredBlocks::MAGENTA_SHULKER_BOX = nullptr;
Block* ColoredBlocks::LIGHT_BLUE_SHULKER_BOX = nullptr;
Block* ColoredBlocks::YELLOW_SHULKER_BOX = nullptr;
Block* ColoredBlocks::LIME_SHULKER_BOX = nullptr;
Block* ColoredBlocks::PINK_SHULKER_BOX = nullptr;
Block* ColoredBlocks::GRAY_SHULKER_BOX = nullptr;
Block* ColoredBlocks::LIGHT_GRAY_SHULKER_BOX = nullptr;
Block* ColoredBlocks::CYAN_SHULKER_BOX = nullptr;
Block* ColoredBlocks::PURPLE_SHULKER_BOX = nullptr;
Block* ColoredBlocks::BLUE_SHULKER_BOX = nullptr;
Block* ColoredBlocks::BROWN_SHULKER_BOX = nullptr;
Block* ColoredBlocks::GREEN_SHULKER_BOX = nullptr;
Block* ColoredBlocks::RED_SHULKER_BOX = nullptr;
Block* ColoredBlocks::BLACK_SHULKER_BOX = nullptr;

void registerColoredBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ========== 羊毛注册 (16色) ==========
    BlockProperties woolProps = BlockProperties(Material::WOOL).hardness(0.8f).flammable().ignitedByLava();

    ColoredBlocks::WHITE_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:white_wool"), woolProps);
    ColoredBlocks::ORANGE_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:orange_wool"), woolProps);
    ColoredBlocks::MAGENTA_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:magenta_wool"), woolProps);
    ColoredBlocks::LIGHT_BLUE_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_blue_wool"), woolProps);
    ColoredBlocks::YELLOW_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:yellow_wool"), woolProps);
    ColoredBlocks::LIME_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:lime_wool"), woolProps);
    ColoredBlocks::PINK_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_wool"), woolProps);
    ColoredBlocks::GRAY_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:gray_wool"), woolProps);
    ColoredBlocks::LIGHT_GRAY_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_gray_wool"), woolProps);
    ColoredBlocks::CYAN_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cyan_wool"), woolProps);
    ColoredBlocks::PURPLE_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:purple_wool"), woolProps);
    ColoredBlocks::BLUE_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:blue_wool"), woolProps);
    ColoredBlocks::BROWN_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:brown_wool"), woolProps);
    ColoredBlocks::GREEN_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:green_wool"), woolProps);
    ColoredBlocks::RED_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:red_wool"), woolProps);
    ColoredBlocks::BLACK_WOOL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:black_wool"), woolProps);

    // ========== 地毯注册 (16色) ==========
    BlockProperties carpetProps = BlockProperties(Material::WOOL).hardness(0.1f).notSolid().flammable().ignitedByLava();

    ColoredBlocks::WHITE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:white_carpet"), carpetProps);
    ColoredBlocks::ORANGE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:orange_carpet"), carpetProps);
    ColoredBlocks::MAGENTA_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:magenta_carpet"), carpetProps);
    ColoredBlocks::LIGHT_BLUE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:light_blue_carpet"), carpetProps);
    ColoredBlocks::YELLOW_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:yellow_carpet"), carpetProps);
    ColoredBlocks::LIME_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:lime_carpet"), carpetProps);
    ColoredBlocks::PINK_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:pink_carpet"), carpetProps);
    ColoredBlocks::GRAY_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:gray_carpet"), carpetProps);
    ColoredBlocks::LIGHT_GRAY_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:light_gray_carpet"), carpetProps);
    ColoredBlocks::CYAN_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:cyan_carpet"), carpetProps);
    ColoredBlocks::PURPLE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:purple_carpet"), carpetProps);
    ColoredBlocks::BLUE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:blue_carpet"), carpetProps);
    ColoredBlocks::BROWN_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:brown_carpet"), carpetProps);
    ColoredBlocks::GREEN_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:green_carpet"), carpetProps);
    ColoredBlocks::RED_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:red_carpet"), carpetProps);
    ColoredBlocks::BLACK_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:black_carpet"), carpetProps);

    // ========== 染色玻璃注册 (16色) ==========
    BlockProperties stainedGlassProps = BlockProperties(Material::GLASS).hardness(0.3f).notSolid();

    ColoredBlocks::WHITE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:white_stained_glass"), stainedGlassProps, DyeColor::White);
    ColoredBlocks::ORANGE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:orange_stained_glass"), stainedGlassProps, DyeColor::Orange);
    ColoredBlocks::MAGENTA_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:magenta_stained_glass"), stainedGlassProps, DyeColor::Magenta);
    ColoredBlocks::LIGHT_BLUE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:light_blue_stained_glass"), stainedGlassProps, DyeColor::LightBlue);
    ColoredBlocks::YELLOW_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:yellow_stained_glass"), stainedGlassProps, DyeColor::Yellow);
    ColoredBlocks::LIME_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:lime_stained_glass"), stainedGlassProps, DyeColor::Lime);
    ColoredBlocks::PINK_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:pink_stained_glass"), stainedGlassProps, DyeColor::Pink);
    ColoredBlocks::GRAY_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:gray_stained_glass"), stainedGlassProps, DyeColor::Gray);
    ColoredBlocks::LIGHT_GRAY_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:light_gray_stained_glass"), stainedGlassProps, DyeColor::LightGray);
    ColoredBlocks::CYAN_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:cyan_stained_glass"), stainedGlassProps, DyeColor::Cyan);
    ColoredBlocks::PURPLE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:purple_stained_glass"), stainedGlassProps, DyeColor::Purple);
    ColoredBlocks::BLUE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:blue_stained_glass"), stainedGlassProps, DyeColor::Blue);
    ColoredBlocks::BROWN_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:brown_stained_glass"), stainedGlassProps, DyeColor::Brown);
    ColoredBlocks::GREEN_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:green_stained_glass"), stainedGlassProps, DyeColor::Green);
    ColoredBlocks::RED_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:red_stained_glass"), stainedGlassProps, DyeColor::Red);
    ColoredBlocks::BLACK_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:black_stained_glass"), stainedGlassProps, DyeColor::Black);

    // ========== 混凝土注册 (16色) ==========
    BlockProperties concreteProps = BlockProperties(Material::ROCK).hardness(1.8f).resistance(1.8f);

    ColoredBlocks::WHITE_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:white_concrete"), concreteProps);
    ColoredBlocks::ORANGE_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:orange_concrete"), concreteProps);
    ColoredBlocks::MAGENTA_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:magenta_concrete"), concreteProps);
    ColoredBlocks::LIGHT_BLUE_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_blue_concrete"), concreteProps);
    ColoredBlocks::YELLOW_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:yellow_concrete"), concreteProps);
    ColoredBlocks::LIME_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:lime_concrete"), concreteProps);
    ColoredBlocks::PINK_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_concrete"), concreteProps);
    ColoredBlocks::GRAY_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:gray_concrete"), concreteProps);
    ColoredBlocks::LIGHT_GRAY_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_gray_concrete"), concreteProps);
    ColoredBlocks::CYAN_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cyan_concrete"), concreteProps);
    ColoredBlocks::PURPLE_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:purple_concrete"), concreteProps);
    ColoredBlocks::BLUE_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:blue_concrete"), concreteProps);
    ColoredBlocks::BROWN_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:brown_concrete"), concreteProps);
    ColoredBlocks::GREEN_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:green_concrete"), concreteProps);
    ColoredBlocks::RED_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:red_concrete"), concreteProps);
    ColoredBlocks::BLACK_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:black_concrete"), concreteProps);

    // ========== 混凝土粉末注册 (16色) ==========
    // 混凝土粉末是下落方块，接触水会固化为对应颜色的混凝土
    BlockProperties concretePowderProps = BlockProperties(Material::SAND).hardness(0.5f);

    ColoredBlocks::WHITE_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:white_concrete_powder"), concretePowderProps, ColoredBlocks::WHITE_CONCRETE);
    ColoredBlocks::ORANGE_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:orange_concrete_powder"), concretePowderProps, ColoredBlocks::ORANGE_CONCRETE);
    ColoredBlocks::MAGENTA_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:magenta_concrete_powder"), concretePowderProps, ColoredBlocks::MAGENTA_CONCRETE);
    ColoredBlocks::LIGHT_BLUE_CONCRETE_POWDER =
        &registry.registerBlock<blocks::ConcretePowderBlock>(ResourceLocation("minecraft:light_blue_concrete_powder"),
            concretePowderProps,
            ColoredBlocks::LIGHT_BLUE_CONCRETE);
    ColoredBlocks::YELLOW_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:yellow_concrete_powder"), concretePowderProps, ColoredBlocks::YELLOW_CONCRETE);
    ColoredBlocks::LIME_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:lime_concrete_powder"), concretePowderProps, ColoredBlocks::LIME_CONCRETE);
    ColoredBlocks::PINK_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:pink_concrete_powder"), concretePowderProps, ColoredBlocks::PINK_CONCRETE);
    ColoredBlocks::GRAY_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:gray_concrete_powder"), concretePowderProps, ColoredBlocks::GRAY_CONCRETE);
    ColoredBlocks::LIGHT_GRAY_CONCRETE_POWDER =
        &registry.registerBlock<blocks::ConcretePowderBlock>(ResourceLocation("minecraft:light_gray_concrete_powder"),
            concretePowderProps,
            ColoredBlocks::LIGHT_GRAY_CONCRETE);
    ColoredBlocks::CYAN_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:cyan_concrete_powder"), concretePowderProps, ColoredBlocks::CYAN_CONCRETE);
    ColoredBlocks::PURPLE_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:purple_concrete_powder"), concretePowderProps, ColoredBlocks::PURPLE_CONCRETE);
    ColoredBlocks::BLUE_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:blue_concrete_powder"), concretePowderProps, ColoredBlocks::BLUE_CONCRETE);
    ColoredBlocks::BROWN_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:brown_concrete_powder"), concretePowderProps, ColoredBlocks::BROWN_CONCRETE);
    ColoredBlocks::GREEN_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:green_concrete_powder"), concretePowderProps, ColoredBlocks::GREEN_CONCRETE);
    ColoredBlocks::RED_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:red_concrete_powder"), concretePowderProps, ColoredBlocks::RED_CONCRETE);
    ColoredBlocks::BLACK_CONCRETE_POWDER = &registry.registerBlock<blocks::ConcretePowderBlock>(
        ResourceLocation("minecraft:black_concrete_powder"), concretePowderProps, ColoredBlocks::BLACK_CONCRETE);

    // ========== 陶瓦注册 (16色 + 普通) ==========
    BlockProperties terracottaProps = BlockProperties(Material::ROCK).hardness(1.4f).resistance(4.2f);

    // 普通陶瓦
    ColoredBlocks::TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:terracotta"), terracottaProps);

    // 染色陶瓦 (16色)
    ColoredBlocks::WHITE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:white_terracotta"), terracottaProps);
    ColoredBlocks::ORANGE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:orange_terracotta"), terracottaProps);
    ColoredBlocks::MAGENTA_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:magenta_terracotta"), terracottaProps);
    ColoredBlocks::LIGHT_BLUE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_blue_terracotta"), terracottaProps);
    ColoredBlocks::YELLOW_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:yellow_terracotta"), terracottaProps);
    ColoredBlocks::LIME_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:lime_terracotta"), terracottaProps);
    ColoredBlocks::PINK_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_terracotta"), terracottaProps);
    ColoredBlocks::GRAY_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:gray_terracotta"), terracottaProps);
    ColoredBlocks::LIGHT_GRAY_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_gray_terracotta"), terracottaProps);
    ColoredBlocks::CYAN_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cyan_terracotta"), terracottaProps);
    ColoredBlocks::PURPLE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:purple_terracotta"), terracottaProps);
    ColoredBlocks::BLUE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:blue_terracotta"), terracottaProps);
    ColoredBlocks::BROWN_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:brown_terracotta"), terracottaProps);
    ColoredBlocks::GREEN_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:green_terracotta"), terracottaProps);
    ColoredBlocks::RED_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:red_terracotta"), terracottaProps);
    ColoredBlocks::BLACK_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:black_terracotta"), terracottaProps);

    // ========== 釉面陶瓦注册 (16色) ==========
    // GlazedTerracottaBlock 带 HORIZONTAL_FACING 属性（可旋转、不可被活塞拉动）。
    // 硬度/阻抗与普通陶瓦一致（vanilla 1.4 / 4.2），材质 ROCK。
    // TODO: vanilla 釉面陶瓦不可被活塞推动（PistonBlockBehavior），项目活塞体系暂未区分，留待补全。
    BlockProperties glazedTerracottaProps = BlockProperties(Material::ROCK).hardness(1.4f).resistance(4.2f);
    ColoredBlocks::WHITE_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:white_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::ORANGE_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:orange_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::MAGENTA_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:magenta_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::LIGHT_BLUE_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:light_blue_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::YELLOW_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:yellow_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::LIME_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:lime_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::PINK_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:pink_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::GRAY_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:gray_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::LIGHT_GRAY_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:light_gray_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::CYAN_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:cyan_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::PURPLE_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:purple_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::BLUE_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:blue_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::BROWN_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:brown_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::GREEN_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:green_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::RED_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:red_glazed_terracotta"), glazedTerracottaProps);
    ColoredBlocks::BLACK_GLAZED_TERRACOTTA = &registry.registerBlock<blocks::GlazedTerracottaBlock>(
        ResourceLocation("minecraft:black_glazed_terracotta"), glazedTerracottaProps);

    // ========== 床注册 (16色) ==========
    // 床：羊毛材质，硬度0.2，不阻挡光线，可被 lava 点燃，被活塞推动时销毁
    BlockProperties bedProps = BlockProperties(Material::WOOL).hardness(0.2f).notSolid().ignitedByLava();

    ColoredBlocks::WHITE_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:white_bed"), DyeColor::White, bedProps);
    ColoredBlocks::ORANGE_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:orange_bed"), DyeColor::Orange, bedProps);
    ColoredBlocks::MAGENTA_BED = &registry.registerBlock<blocks::BedBlock>(
        ResourceLocation("minecraft:magenta_bed"), DyeColor::Magenta, bedProps);
    ColoredBlocks::LIGHT_BLUE_BED = &registry.registerBlock<blocks::BedBlock>(
        ResourceLocation("minecraft:light_blue_bed"), DyeColor::LightBlue, bedProps);
    ColoredBlocks::YELLOW_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:yellow_bed"), DyeColor::Yellow, bedProps);
    ColoredBlocks::LIME_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:lime_bed"), DyeColor::Lime, bedProps);
    ColoredBlocks::PINK_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:pink_bed"), DyeColor::Pink, bedProps);
    ColoredBlocks::GRAY_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:gray_bed"), DyeColor::Gray, bedProps);
    ColoredBlocks::LIGHT_GRAY_BED = &registry.registerBlock<blocks::BedBlock>(
        ResourceLocation("minecraft:light_gray_bed"), DyeColor::LightGray, bedProps);
    ColoredBlocks::CYAN_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:cyan_bed"), DyeColor::Cyan, bedProps);
    ColoredBlocks::PURPLE_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:purple_bed"), DyeColor::Purple, bedProps);
    ColoredBlocks::BLUE_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:blue_bed"), DyeColor::Blue, bedProps);
    ColoredBlocks::BROWN_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:brown_bed"), DyeColor::Brown, bedProps);
    ColoredBlocks::GREEN_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:green_bed"), DyeColor::Green, bedProps);
    ColoredBlocks::RED_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:red_bed"), DyeColor::Red, bedProps);
    ColoredBlocks::BLACK_BED =
        &registry.registerBlock<blocks::BedBlock>(ResourceLocation("minecraft:black_bed"), DyeColor::Black, bedProps);

    // ========== 潜影盒注册 (16色) ==========
    // 潜影盒：木质材质，硬度2.0，抗爆2.0，非固体（动态碰撞箱）
    BlockProperties shulkerBoxProps = BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).notSolid();

    ColoredBlocks::WHITE_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:white_shulker_box"), DyeColor::White, shulkerBoxProps);
    ColoredBlocks::ORANGE_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:orange_shulker_box"), DyeColor::Orange, shulkerBoxProps);
    ColoredBlocks::MAGENTA_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:magenta_shulker_box"), DyeColor::Magenta, shulkerBoxProps);
    ColoredBlocks::LIGHT_BLUE_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:light_blue_shulker_box"), DyeColor::LightBlue, shulkerBoxProps);
    ColoredBlocks::YELLOW_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:yellow_shulker_box"), DyeColor::Yellow, shulkerBoxProps);
    ColoredBlocks::LIME_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:lime_shulker_box"), DyeColor::Lime, shulkerBoxProps);
    ColoredBlocks::PINK_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:pink_shulker_box"), DyeColor::Pink, shulkerBoxProps);
    ColoredBlocks::GRAY_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:gray_shulker_box"), DyeColor::Gray, shulkerBoxProps);
    ColoredBlocks::LIGHT_GRAY_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:light_gray_shulker_box"), DyeColor::LightGray, shulkerBoxProps);
    ColoredBlocks::CYAN_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:cyan_shulker_box"), DyeColor::Cyan, shulkerBoxProps);
    ColoredBlocks::PURPLE_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:purple_shulker_box"), DyeColor::Purple, shulkerBoxProps);
    ColoredBlocks::BLUE_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:blue_shulker_box"), DyeColor::Blue, shulkerBoxProps);
    ColoredBlocks::BROWN_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:brown_shulker_box"), DyeColor::Brown, shulkerBoxProps);
    ColoredBlocks::GREEN_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:green_shulker_box"), DyeColor::Green, shulkerBoxProps);
    ColoredBlocks::RED_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:red_shulker_box"), DyeColor::Red, shulkerBoxProps);
    ColoredBlocks::BLACK_SHULKER_BOX = &registry.registerBlock<blocks::ShulkerBoxBlock>(
        ResourceLocation("minecraft:black_shulker_box"), DyeColor::Black, shulkerBoxProps);
}

} // namespace block_registry
} // namespace mc
