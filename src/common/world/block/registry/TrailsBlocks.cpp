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
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/agricultural/PitcherCropBlock.hpp"
#include "world/block/blocks/agricultural/TorchflowerCropBlock.hpp"
#include "world/block/blocks/functional/TrailsBlocks.hpp"
#include "world/block/blocks/garden/FlowerBedBlock.hpp"
#include "world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "world/block/blocks/vegetation/FlowerBlock.hpp"
#include "world/block/registry/BaseBlocks.hpp"

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
    // 刷扫音效：BRUSH_SAND，完成音效：BRUSH_SAND_COMPLETED
    // 刷扫完成后转换为沙方块（对齐 MC 1.21.11 BrushableBlock.turns_into = sand）
    TrailsBlocks::SUSPICIOUS_SAND = &registry.registerBlock<blocks::BrushableBlock>(
        ResourceLocation("minecraft:suspicious_sand"),
        BlockProperties(Material::SAND).hardness(0.25f).resistance(0.25f).soundType(BlockSoundTypes::SUSPICIOUS_SAND),
        BaseBlocks::SAND,
        SoundEvents::BRUSH_SAND,
        SoundEvents::BRUSH_SAND_COMPLETED);

    // 可疑的沙砾 - 受重力影响，可以被刷子刷出物品，DUSTED属性(0-3)
    // 刷扫音效：BRUSH_GRAVEL，完成音效：BRUSH_GRAVEL_COMPLETED
    // 刷扫完成后转换为沙砾方块（对齐 MC 1.21.11 BrushableBlock.turns_into = gravel）
    TrailsBlocks::SUSPICIOUS_GRAVEL = &registry.registerBlock<blocks::BrushableBlock>(
        ResourceLocation("minecraft:suspicious_gravel"),
        BlockProperties(Material::SAND).hardness(0.25f).resistance(0.25f).soundType(BlockSoundTypes::SUSPICIOUS_GRAVEL),
        BaseBlocks::GRAVEL,
        SoundEvents::BRUSH_GRAVEL,
        SoundEvents::BRUSH_GRAVEL_COMPLETED);

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

    // 粉红色花瓣 - 樱花林生物群系的装饰花，可堆叠放置（1-4朵），骨粉催熟
    // 使用 FlowerBedBlock 实现，与野花（WILDFLOWERS）共享同一方块类型
    // replaceable 标志允许同类型物品堆叠放置（isReplaceable 重写会检查物品类型）
    TrailsBlocks::PINK_PETALS =
        &registry.registerBlock<blocks::FlowerBedBlock>(ResourceLocation("minecraft:pink_petals"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .replaceable()
                .soundType(BlockSoundTypes::PINK_PETALS));

    // ============================================================================
    // 火把花
    // ============================================================================

    // 火把花 - 小型花朵，可放置在泥土/草方块等上，可用于制作可疑炖菜（夜视效果）
    // 骨粉可催熟火把花作物为火把花；无支撑方块时自动掉落
    // potted_torchflower 方块在 FlowerPotBlocks.cpp 中注册
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

    // 瓶草作物 - 5个生长阶段（AGE_0_4），AGE>=3时变为双格植物
    // 随机刻生长；骨粉每次增加1个阶段；只能放置在耕地上；掉落瓶草荚果
    // 形状随AGE变化：AGE 0为鳞茎（窄柱），AGE 1-2为单层作物，AGE 3-4为双层作物
    TrailsBlocks::PITCHER_CROP =
        &registry.registerBlock<blocks::PitcherCropBlock>(ResourceLocation("minecraft:pitcher_crop"),
            BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).soundType(BlockSoundTypes::CROP));
}

} // namespace block_registry
} // namespace mc
