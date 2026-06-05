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
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/decorative/CarpetBlock.hpp"
#include "world/block/blocks/decorative/StainedGlassBlock.hpp"

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

void registerColoredBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ========== 羊毛注册 (16色) ==========
    BlockProperties woolProps = BlockProperties(Material::WOOL).hardness(0.8f);

    WHITE_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:white_wool"), woolProps);
    ORANGE_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:orange_wool"), woolProps);
    MAGENTA_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:magenta_wool"), woolProps);
    LIGHT_BLUE_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_blue_wool"), woolProps);
    YELLOW_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:yellow_wool"), woolProps);
    LIME_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:lime_wool"), woolProps);
    PINK_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_wool"), woolProps);
    GRAY_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:gray_wool"), woolProps);
    LIGHT_GRAY_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_gray_wool"), woolProps);
    CYAN_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cyan_wool"), woolProps);
    PURPLE_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:purple_wool"), woolProps);
    BLUE_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:blue_wool"), woolProps);
    BROWN_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:brown_wool"), woolProps);
    GREEN_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:green_wool"), woolProps);
    RED_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:red_wool"), woolProps);
    BLACK_WOOL = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:black_wool"), woolProps);

    // ========== 地毯注册 (16色) ==========
    BlockProperties carpetProps = BlockProperties(Material::WOOL).hardness(0.1f).notSolid();

    WHITE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:white_carpet"), carpetProps);
    ORANGE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:orange_carpet"), carpetProps);
    MAGENTA_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:magenta_carpet"), carpetProps);
    LIGHT_BLUE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:light_blue_carpet"), carpetProps);
    YELLOW_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:yellow_carpet"), carpetProps);
    LIME_CARPET = &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:lime_carpet"), carpetProps);
    PINK_CARPET = &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:pink_carpet"), carpetProps);
    GRAY_CARPET = &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:gray_carpet"), carpetProps);
    LIGHT_GRAY_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:light_gray_carpet"), carpetProps);
    CYAN_CARPET = &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:cyan_carpet"), carpetProps);
    PURPLE_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:purple_carpet"), carpetProps);
    BLUE_CARPET = &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:blue_carpet"), carpetProps);
    BROWN_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:brown_carpet"), carpetProps);
    GREEN_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:green_carpet"), carpetProps);
    RED_CARPET = &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:red_carpet"), carpetProps);
    BLACK_CARPET =
        &registry.registerBlock<blocks::CarpetBlock>(ResourceLocation("minecraft:black_carpet"), carpetProps);

    // ========== 染色玻璃注册 (16色) ==========
    BlockProperties stainedGlassProps = BlockProperties(Material::GLASS).hardness(0.3f).notSolid();

    WHITE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:white_stained_glass"), stainedGlassProps, DyeColor::White);
    ORANGE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:orange_stained_glass"), stainedGlassProps, DyeColor::Orange);
    MAGENTA_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:magenta_stained_glass"), stainedGlassProps, DyeColor::Magenta);
    LIGHT_BLUE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:light_blue_stained_glass"), stainedGlassProps, DyeColor::LightBlue);
    YELLOW_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:yellow_stained_glass"), stainedGlassProps, DyeColor::Yellow);
    LIME_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:lime_stained_glass"), stainedGlassProps, DyeColor::Lime);
    PINK_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:pink_stained_glass"), stainedGlassProps, DyeColor::Pink);
    GRAY_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:gray_stained_glass"), stainedGlassProps, DyeColor::Gray);
    LIGHT_GRAY_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:light_gray_stained_glass"), stainedGlassProps, DyeColor::LightGray);
    CYAN_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:cyan_stained_glass"), stainedGlassProps, DyeColor::Cyan);
    PURPLE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:purple_stained_glass"), stainedGlassProps, DyeColor::Purple);
    BLUE_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:blue_stained_glass"), stainedGlassProps, DyeColor::Blue);
    BROWN_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:brown_stained_glass"), stainedGlassProps, DyeColor::Brown);
    GREEN_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:green_stained_glass"), stainedGlassProps, DyeColor::Green);
    RED_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:red_stained_glass"), stainedGlassProps, DyeColor::Red);
    BLACK_STAINED_GLASS = &registry.registerBlock<block::StainedGlassBlock>(
        ResourceLocation("minecraft:black_stained_glass"), stainedGlassProps, DyeColor::Black);

    // ========== 混凝土注册 (16色) ==========
    BlockProperties concreteProps = BlockProperties(Material::ROCK).hardness(1.8f).resistance(1.8f);

    WHITE_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:white_concrete"), concreteProps);
    ORANGE_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:orange_concrete"), concreteProps);
    MAGENTA_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:magenta_concrete"), concreteProps);
    LIGHT_BLUE_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_blue_concrete"), concreteProps);
    YELLOW_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:yellow_concrete"), concreteProps);
    LIME_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:lime_concrete"), concreteProps);
    PINK_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_concrete"), concreteProps);
    GRAY_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:gray_concrete"), concreteProps);
    LIGHT_GRAY_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_gray_concrete"), concreteProps);
    CYAN_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cyan_concrete"), concreteProps);
    PURPLE_CONCRETE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:purple_concrete"), concreteProps);
    BLUE_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:blue_concrete"), concreteProps);
    BROWN_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:brown_concrete"), concreteProps);
    GREEN_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:green_concrete"), concreteProps);
    RED_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:red_concrete"), concreteProps);
    BLACK_CONCRETE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:black_concrete"), concreteProps);

    // ========== 混凝土粉末注册 (16色) ==========
    BlockProperties concretePowderProps = BlockProperties(Material::SAND).hardness(0.5f);

    WHITE_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:white_concrete_powder"), concretePowderProps);
    ORANGE_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:orange_concrete_powder"), concretePowderProps);
    MAGENTA_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:magenta_concrete_powder"), concretePowderProps);
    LIGHT_BLUE_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_blue_concrete_powder"), concretePowderProps);
    YELLOW_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:yellow_concrete_powder"), concretePowderProps);
    LIME_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:lime_concrete_powder"), concretePowderProps);
    PINK_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_concrete_powder"), concretePowderProps);
    GRAY_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:gray_concrete_powder"), concretePowderProps);
    LIGHT_GRAY_CONCRETE_POWDER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_gray_concrete_powder"), concretePowderProps);
    CYAN_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cyan_concrete_powder"), concretePowderProps);
    PURPLE_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:purple_concrete_powder"), concretePowderProps);
    BLUE_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:blue_concrete_powder"), concretePowderProps);
    BROWN_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:brown_concrete_powder"), concretePowderProps);
    GREEN_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:green_concrete_powder"), concretePowderProps);
    RED_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:red_concrete_powder"), concretePowderProps);
    BLACK_CONCRETE_POWDER =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:black_concrete_powder"), concretePowderProps);

    // ========== 陶瓦注册 (16色 + 普通) ==========
    BlockProperties terracottaProps = BlockProperties(Material::ROCK).hardness(1.4f).resistance(4.2f);

    // 普通陶瓦
    TERRACOTTA = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:terracotta"), terracottaProps);

    // 染色陶瓦 (16色)
    WHITE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:white_terracotta"), terracottaProps);
    ORANGE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:orange_terracotta"), terracottaProps);
    MAGENTA_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:magenta_terracotta"), terracottaProps);
    LIGHT_BLUE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_blue_terracotta"), terracottaProps);
    YELLOW_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:yellow_terracotta"), terracottaProps);
    LIME_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:lime_terracotta"), terracottaProps);
    PINK_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_terracotta"), terracottaProps);
    GRAY_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:gray_terracotta"), terracottaProps);
    LIGHT_GRAY_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:light_gray_terracotta"), terracottaProps);
    CYAN_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cyan_terracotta"), terracottaProps);
    PURPLE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:purple_terracotta"), terracottaProps);
    BLUE_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:blue_terracotta"), terracottaProps);
    BROWN_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:brown_terracotta"), terracottaProps);
    GREEN_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:green_terracotta"), terracottaProps);
    RED_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:red_terracotta"), terracottaProps);
    BLACK_TERRACOTTA =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:black_terracotta"), terracottaProps);
}

} // namespace block_registry
} // namespace mc
