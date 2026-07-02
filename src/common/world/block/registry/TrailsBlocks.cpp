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

#include "world/block/registry/TrailsBlocks.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/agricultural/TorchflowerCropBlock.hpp"
#include "world/block/blocks/functional/TrailsBlocks.hpp"
#include "world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "world/block/blocks/vegetation/FlowerBlock.hpp"

namespace mc {
namespace block_registry {

// 可疑方块（考古）
Block* TrailsBlocks::SUSPICIOUS_SAND = nullptr;
Block* TrailsBlocks::SUSPICIOUS_GRAVEL = nullptr;

// 雕书架
Block* TrailsBlocks::CHISELED_BOOKSHELF = nullptr;

// 饰纹陶罐
Block* TrailsBlocks::DECORATED_POT = nullptr;

// 监守者蛋
Block* TrailsBlocks::SNIFFER_EGG = nullptr;

// 粉红色花瓣
Block* TrailsBlocks::PINK_PETALS = nullptr;

// 火把花
Block* TrailsBlocks::TORCHFLOWER = nullptr;

// 瓶草
Block* TrailsBlocks::PITCHER_PLANT = nullptr;

// 作物
Block* TrailsBlocks::TORCHFLOWER_CROP = nullptr;
Block* TrailsBlocks::PITCHER_CROP = nullptr;

void registerTrailsBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 可疑方块（考古）
    // ============================================================================

    // 可疑的沙 - 受重力影响，可以被刷子刷出物品，DUSTED属性(0-3)
    TrailsBlocks::SUSPICIOUS_SAND = &registry.registerBlock<blocks::BrushableBlock>(
        ResourceLocation("minecraft:suspicious_sand"),
        BlockProperties(Material::SAND).hardness(0.25f).resistance(0.25f).soundType(BlockSoundTypes::SUSPICIOUS_SAND));

    // 可疑的沙砾 - 受重力影响，可以被刷子刷出物品，DUSTED属性(0-3)
    TrailsBlocks::SUSPICIOUS_GRAVEL =
        &registry.registerBlock<blocks::BrushableBlock>(ResourceLocation("minecraft:suspicious_gravel"),
            BlockProperties(Material::SAND)
                .hardness(0.25f)
                .resistance(0.25f)
                .soundType(BlockSoundTypes::SUSPICIOUS_GRAVEL));

    // ============================================================================
    // 雕书架
    // ============================================================================

    // 雕书架 - 可以放置6本书，可被红石比较器检测
    // FACING + SLOT_0~5_OCCUPIED
    TrailsBlocks::CHISELED_BOOKSHELF =
        &registry.registerBlock<blocks::ChiseledBookshelfBlock>(ResourceLocation("minecraft:chiseled_bookshelf"),
            BlockProperties(Material::WOOD)
                .hardness(1.5f)
                .resistance(1.5f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::CHISELED_BOOKSHELF)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 饰纹陶罐
    // ============================================================================

    // 饰纹陶罐 - 由陶片合成，无碰撞箱
    // FACING + CRACKED + WATERLOGGED
    TrailsBlocks::DECORATED_POT =
        &registry.registerBlock<blocks::DecoratedPotBlock>(ResourceLocation("minecraft:decorated_pot"),
            BlockProperties(Material::DECORATION)
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::DECORATED_POT)
                .noCollision());

    // ============================================================================
    // 监守者蛋
    // ============================================================================

    // 监守者蛋 - 可孵化出嗅探兽生物，HATCH属性(0-2)
    TrailsBlocks::SNIFFER_EGG =
        &registry.registerBlock<blocks::SnifferEggBlock>(ResourceLocation("minecraft:sniffer_egg"),
            BlockProperties(Material::EARTH).hardness(0.5f).resistance(0.5f).soundType(BlockSoundTypes::SNIFFER_EGG));

    // ============================================================================
    // 粉红色花瓣
    // ============================================================================

    // 粉红色花瓣 - 樱花林生物群系的装饰植物
    TrailsBlocks::PINK_PETALS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_petals"),
        BlockProperties(Material::PLANT).noCollision().notSolid().soundType(BlockSoundTypes::PINK_PETALS));

    // ============================================================================
    // 火把花
    // ============================================================================

    // 火把花 - 小型花朵，可放置在泥土/草方块等上，可用于制作可疑炖菜（夜视效果）
    // 骨粉可催熟火把花作物为火把花；无支撑方块时自动掉落
    // TODO: 花盆系统完善后，需注册 potted_torchflower 方块（FlowerPotBlock 内含火炬花）
    //       并将其添加到 BlockTags::FLOWER_POTS 和相关标签中
    TrailsBlocks::TORCHFLOWER = &registry.registerBlock<blocks::FlowerBlock>(ResourceLocation("minecraft:torchflower"),
        BlockProperties(Material::PLANT).noCollision().notSolid().soundType(BlockSoundTypes::TORCHFLOWER),
        static_cast<u32>(entity::effect::EffectType::NightVision),
        8);

    // ============================================================================
    // 瓶草
    // ============================================================================

    // 瓶草 - 双层植物
    TrailsBlocks::PITCHER_PLANT =
        &registry.registerBlock<blocks::DoublePlantBlock>(ResourceLocation("minecraft:pitcher_plant"),
            BlockProperties(Material::PLANT).noCollision().notSolid().soundType(BlockSoundTypes::CROP));

    // ============================================================================
    // 作物方块
    // ============================================================================

    // 火把花作物 - 2个生长阶段（AGE_0_1），成熟后继续生长变为火把花方块
    // 随机刻有1/3概率跳过；骨粉每次增加1个阶段；只能放置在耕地上
    TrailsBlocks::TORCHFLOWER_CROP =
        &registry.registerBlock<blocks::TorchflowerCropBlock>(ResourceLocation("minecraft:torchflower_crop"),
            BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).soundType(BlockSoundTypes::CROP));

    // TODO: 瓶草作物当前注册为SimpleBlock占位，需要升级为专用的PitcherCropBlock。
    // PitcherCropBlock需实现：1) AGE_0_4整数属性（5个生长阶段）和HALF属性（上半/下半）；
    // 2) AGE>=3时方块变为双层（上半+下半），需要DoublePlantBlock类似的半块管理；
    // 3) 随机刻生长逻辑（与普通作物不同，需要更长时间）；
    // 4) 骨粉可催熟；5) 掉落：未成熟时掉落瓶草荚果，成熟时可能额外掉落瓶草荚果；
    // 6) 只能放置在耕地上；7) 形状随AGE变化（AGE<3为单层作物，AGE>=3为双层）；
    // 8) 瓶草荚果(pitcher_pod)为种子物品，右键耕地放置此作物方块。
    // MC Java中PitcherCropBlock有完整实现可参考。
    TrailsBlocks::PITCHER_CROP = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pitcher_crop"),
        BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).soundType(BlockSoundTypes::CROP));
}

} // namespace block_registry
} // namespace mc
