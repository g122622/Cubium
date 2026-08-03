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

#include "world/block/registry/CandleBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/decorative/CandleBlock.hpp"
#include "world/block/blocks/functional/CandleCakeBlock.hpp"

namespace mc {
namespace block_registry {

// ============================================================================
// 蜡烛方块
// ============================================================================

Block* CandleBlocks::CANDLE = nullptr;
Block* CandleBlocks::WHITE_CANDLE = nullptr;
Block* CandleBlocks::ORANGE_CANDLE = nullptr;
Block* CandleBlocks::MAGENTA_CANDLE = nullptr;
Block* CandleBlocks::LIGHT_BLUE_CANDLE = nullptr;
Block* CandleBlocks::YELLOW_CANDLE = nullptr;
Block* CandleBlocks::LIME_CANDLE = nullptr;
Block* CandleBlocks::PINK_CANDLE = nullptr;
Block* CandleBlocks::GRAY_CANDLE = nullptr;
Block* CandleBlocks::LIGHT_GRAY_CANDLE = nullptr;
Block* CandleBlocks::CYAN_CANDLE = nullptr;
Block* CandleBlocks::PURPLE_CANDLE = nullptr;
Block* CandleBlocks::BLUE_CANDLE = nullptr;
Block* CandleBlocks::BROWN_CANDLE = nullptr;
Block* CandleBlocks::GREEN_CANDLE = nullptr;
Block* CandleBlocks::RED_CANDLE = nullptr;
Block* CandleBlocks::BLACK_CANDLE = nullptr;

// ============================================================================
// 蜡烛蛋糕方块
// ============================================================================

Block* CandleBlocks::CANDLE_CAKE = nullptr;
Block* CandleBlocks::WHITE_CANDLE_CAKE = nullptr;
Block* CandleBlocks::ORANGE_CANDLE_CAKE = nullptr;
Block* CandleBlocks::MAGENTA_CANDLE_CAKE = nullptr;
Block* CandleBlocks::LIGHT_BLUE_CANDLE_CAKE = nullptr;
Block* CandleBlocks::YELLOW_CANDLE_CAKE = nullptr;
Block* CandleBlocks::LIME_CANDLE_CAKE = nullptr;
Block* CandleBlocks::PINK_CANDLE_CAKE = nullptr;
Block* CandleBlocks::GRAY_CANDLE_CAKE = nullptr;
Block* CandleBlocks::LIGHT_GRAY_CANDLE_CAKE = nullptr;
Block* CandleBlocks::CYAN_CANDLE_CAKE = nullptr;
Block* CandleBlocks::PURPLE_CANDLE_CAKE = nullptr;
Block* CandleBlocks::BLUE_CANDLE_CAKE = nullptr;
Block* CandleBlocks::BROWN_CANDLE_CAKE = nullptr;
Block* CandleBlocks::GREEN_CANDLE_CAKE = nullptr;
Block* CandleBlocks::RED_CANDLE_CAKE = nullptr;
Block* CandleBlocks::BLACK_CANDLE_CAKE = nullptr;

void registerCandleBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 蜡烛方块（Material::DECORATION, noCollision, notSolid, CANDLE声音, hardness=0.1, resistance=0.1）
    // ============================================================================

    CandleBlocks::CANDLE = &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:candle"),
        BlockProperties(Material::DECORATION)
            .noCollision()
            .notSolid()
            .soundType(BlockSoundTypes::CANDLE)
            .hardness(0.1f)
            .resistance(0.1f));

    CandleBlocks::WHITE_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:white_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::ORANGE_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:orange_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::MAGENTA_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:magenta_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::LIGHT_BLUE_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:light_blue_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::YELLOW_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:yellow_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::LIME_CANDLE = &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:lime_candle"),
        BlockProperties(Material::DECORATION)
            .noCollision()
            .notSolid()
            .soundType(BlockSoundTypes::CANDLE)
            .hardness(0.1f)
            .resistance(0.1f));

    CandleBlocks::PINK_CANDLE = &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:pink_candle"),
        BlockProperties(Material::DECORATION)
            .noCollision()
            .notSolid()
            .soundType(BlockSoundTypes::CANDLE)
            .hardness(0.1f)
            .resistance(0.1f));

    CandleBlocks::GRAY_CANDLE = &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:gray_candle"),
        BlockProperties(Material::DECORATION)
            .noCollision()
            .notSolid()
            .soundType(BlockSoundTypes::CANDLE)
            .hardness(0.1f)
            .resistance(0.1f));

    CandleBlocks::LIGHT_GRAY_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:light_gray_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::CYAN_CANDLE = &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:cyan_candle"),
        BlockProperties(Material::DECORATION)
            .noCollision()
            .notSolid()
            .soundType(BlockSoundTypes::CANDLE)
            .hardness(0.1f)
            .resistance(0.1f));

    CandleBlocks::PURPLE_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:purple_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::BLUE_CANDLE = &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:blue_candle"),
        BlockProperties(Material::DECORATION)
            .noCollision()
            .notSolid()
            .soundType(BlockSoundTypes::CANDLE)
            .hardness(0.1f)
            .resistance(0.1f));

    CandleBlocks::BROWN_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:brown_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::GREEN_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:green_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    CandleBlocks::RED_CANDLE = &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:red_candle"),
        BlockProperties(Material::DECORATION)
            .noCollision()
            .notSolid()
            .soundType(BlockSoundTypes::CANDLE)
            .hardness(0.1f)
            .resistance(0.1f));

    CandleBlocks::BLACK_CANDLE =
        &registry.registerBlock<blocks::CandleBlock>(ResourceLocation("minecraft:black_candle"),
            BlockProperties(Material::DECORATION)
                .noCollision()
                .notSolid()
                .soundType(BlockSoundTypes::CANDLE)
                .hardness(0.1f)
                .resistance(0.1f));

    // ============================================================================
    // 蜡烛蛋糕方块（Material::CAKE, notSolid, CLOTH声音, hardness=0.5, resistance=0.5）
    // CandleCakeBlock构造函数第二个参数为关联的蜡烛方块
    // ============================================================================

    CandleBlocks::CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::CANDLE);

    CandleBlocks::WHITE_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:white_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::WHITE_CANDLE);

    CandleBlocks::ORANGE_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:orange_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::ORANGE_CANDLE);

    CandleBlocks::MAGENTA_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:magenta_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::MAGENTA_CANDLE);

    CandleBlocks::LIGHT_BLUE_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:light_blue_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::LIGHT_BLUE_CANDLE);

    CandleBlocks::YELLOW_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:yellow_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::YELLOW_CANDLE);

    CandleBlocks::LIME_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:lime_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::LIME_CANDLE);

    CandleBlocks::PINK_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:pink_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::PINK_CANDLE);

    CandleBlocks::GRAY_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:gray_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::GRAY_CANDLE);

    CandleBlocks::LIGHT_GRAY_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:light_gray_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::LIGHT_GRAY_CANDLE);

    CandleBlocks::CYAN_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:cyan_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::CYAN_CANDLE);

    CandleBlocks::PURPLE_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:purple_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::PURPLE_CANDLE);

    CandleBlocks::BLUE_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:blue_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::BLUE_CANDLE);

    CandleBlocks::BROWN_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:brown_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::BROWN_CANDLE);

    CandleBlocks::GREEN_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:green_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::GREEN_CANDLE);

    CandleBlocks::RED_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:red_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::RED_CANDLE);

    CandleBlocks::BLACK_CANDLE_CAKE = &registry.registerBlock<blocks::CandleCakeBlock>(
        ResourceLocation("minecraft:black_candle_cake"),
        BlockProperties(Material::CAKE).notSolid().soundType(BlockSoundTypes::CLOTH).hardness(0.5f).resistance(0.5f),
        CandleBlocks::BLACK_CANDLE);
}

} // namespace block_registry
} // namespace mc
