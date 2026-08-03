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

#include "FireInfoRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/registry/BambooBlocks.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"
#include "common/world/block/registry/BuildingVariantBlocks.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
#include "common/world/block/registry/CherryBlocks.hpp"
#include "common/world/block/registry/ColoredBlocks.hpp"
#include "common/world/block/registry/GardenBlocks.hpp"
#include "common/world/block/registry/MangroveBlocks.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/PaleGardenBlocks.hpp"
#include "common/world/block/registry/RedstoneBlocks.hpp"
#include "common/world/block/registry/ShelfBlocks.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/block/registry/VegetationBlocks.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// FireInfoRegistry
// ============================================================================

FireInfoRegistry& FireInfoRegistry::instance() noexcept
{
    static FireInfoRegistry s_instance;
    return s_instance;
}

void FireInfoRegistry::registerFireInfo(u32 blockId, i32 encouragement, i32 flammability)
{
    m_fireInfos[blockId] = FireInfo(encouragement, flammability);
}

FireInfo FireInfoRegistry::getFireInfo(u32 blockId) const noexcept
{
    auto it = m_fireInfos.find(blockId);
    if (it != m_fireInfos.end()) {
        return it->second;
    }
    return FireInfo(0, 0);
}

i32 FireInfoRegistry::getFlammability(u32 blockId) const noexcept
{
    return getFireInfo(blockId).flammability;
}

i32 FireInfoRegistry::getEncouragement(u32 blockId) const noexcept
{
    return getFireInfo(blockId).encouragement;
}

void FireInfoRegistry::clear()
{
    m_fireInfos.clear();
}

// ============================================================================
// 原版火焰参数常量
// 参考: net.minecraft.world.level.block.FireBlock.bootStrap()
//
// 【重要】火焰蔓延系统架构说明：
//
// 本项目中火焰蔓延（FireBlock 火焰扩散/燃烧）与岩浆点燃（LavaFluid 点燃）是两个独立系统：
//
// 1. 火焰蔓延：仅依赖本注册表（FireInfoRegistry）。FireBlock::canCatchFire()、
//    FireBlock::tryCatchFire()、FireBlock::areNeighborsFlammable() 等方法全部通过
//    BlockState::getFlammability()/getFireSpreadSpeed() 查询本注册表，不检查 Material。
//    如果方块未在本注册表中注册，火焰不会蔓延到该方块，该方块也不会被火焰烧毁。
//    这与 MC 原版行为一致：原版 FireBlock.bootStrap() 也仅注册特定方块，无材质回退机制。
//
// 2. 岩浆点燃：通过 Material::isFlammable() 判断。LavaFluid 使用 Material 的可燃性
//    标记来决定岩浆是否可以点燃相邻方块。这与火焰蔓延完全独立。
//    Material::WOOD、Material::PLANT 等标记为 isFlammable=true 的方块可以被岩浆点燃，
//    即使它们未在本注册表中注册。
//
// 因此，告示牌、树苗、蘑菇方块等虽然未在本注册表中注册（与 MC 原版 bootStrap() 一致），
// 但由于它们使用 Material::WOOD 或 Material::PLANT 材质，仍可被岩浆点燃。
// ============================================================================

// 点燃概率常量
static constexpr i32 IGNITE_INSTANT = 60; // 瞬间点燃（地毯、植物等）
static constexpr i32 IGNITE_EASY = 30;    // 易点燃（树叶、羊毛等）
static constexpr i32 IGNITE_MEDIUM = 15;  // 中等点燃（藤蔓等）
static constexpr i32 IGNITE_HARD = 5;     // 难点燃（木板、原木等）

// 燃烧概率常量
static constexpr i32 BURN_INSTANT = 100; // 瞬间燃烧（植物、藤蔓等）
static constexpr i32 BURN_EASY = 60;     // 易燃烧（树叶、羊毛等）
static constexpr i32 BURN_MEDIUM = 20;   // 中等燃烧（木板等）
static constexpr i32 BURN_HARD = 5;      // 难燃烧（原木等）

void FireInfoRegistry::initializeVanillaFireInfos()
{
    using namespace block_registry;

    // ========================================================================
    // 木板类 — ignite=5, burn=20
    // ========================================================================
    registerFireInfo(BaseBlocks::OAK_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BaseBlocks::SPRUCE_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BaseBlocks::BIRCH_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BaseBlocks::JUNGLE_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BaseBlocks::ACACIA_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BaseBlocks::DARK_OAK_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(CherryBlocks::CHERRY_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(MangroveBlocks::MANGROVE_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(PaleGardenBlocks::PALE_OAK_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BambooBlocks::BAMBOO_PLANKS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BambooBlocks::BAMBOO_MOSAIC->blockId(), IGNITE_HARD, BURN_MEDIUM);

    // ========================================================================
    // 台阶类 — ignite=5, burn=20
    // ========================================================================
    registerFireInfo(BuildingVariantBlocks::OAK_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::SPRUCE_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::BIRCH_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::JUNGLE_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::ACACIA_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::DARK_OAK_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(CherryBlocks::CHERRY_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(MangroveBlocks::MANGROVE_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(PaleGardenBlocks::PALE_OAK_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BambooBlocks::BAMBOO_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BambooBlocks::BAMBOO_MOSAIC_SLAB->blockId(), IGNITE_HARD, BURN_MEDIUM);

    // ========================================================================
    // 栅栏门类 — ignite=5, burn=20
    // ========================================================================
    registerFireInfo(BuildingVariantBlocks::OAK_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::SPRUCE_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::BIRCH_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::JUNGLE_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::ACACIA_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::DARK_OAK_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(CherryBlocks::CHERRY_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(MangroveBlocks::MANGROVE_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(PaleGardenBlocks::PALE_OAK_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BambooBlocks::BAMBOO_FENCE_GATE->blockId(), IGNITE_HARD, BURN_MEDIUM);

    // ========================================================================
    // 栅栏类 — ignite=5, burn=20
    // ========================================================================
    registerFireInfo(BuildingVariantBlocks::OAK_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::SPRUCE_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::BIRCH_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::JUNGLE_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::ACACIA_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::DARK_OAK_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(CherryBlocks::CHERRY_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(MangroveBlocks::MANGROVE_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(PaleGardenBlocks::PALE_OAK_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BambooBlocks::BAMBOO_FENCE->blockId(), IGNITE_HARD, BURN_MEDIUM);

    // 注意: 绯红木栅栏(CRIMSON_FENCE)和诡异木栅栏(WARPED_FENCE)不可燃，不注册

    // ========================================================================
    // 楼梯类 — ignite=5, burn=20
    // ========================================================================
    registerFireInfo(BuildingVariantBlocks::OAK_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::SPRUCE_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::BIRCH_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::JUNGLE_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::ACACIA_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BuildingVariantBlocks::DARK_OAK_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(CherryBlocks::CHERRY_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(MangroveBlocks::MANGROVE_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(PaleGardenBlocks::PALE_OAK_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BambooBlocks::BAMBOO_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(BambooBlocks::BAMBOO_MOSAIC_STAIRS->blockId(), IGNITE_HARD, BURN_MEDIUM);

    // ========================================================================
    // 原木类 — ignite=5, burn=5
    // ========================================================================
    registerFireInfo(BaseBlocks::OAK_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::SPRUCE_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::BIRCH_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::JUNGLE_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::ACACIA_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::DARK_OAK_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(CherryBlocks::CHERRY_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(MangroveBlocks::MANGROVE_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(PaleGardenBlocks::PALE_OAK_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BambooBlocks::BAMBOO_BLOCK->blockId(), IGNITE_HARD, BURN_HARD);

    // ========================================================================
    // 去皮原木类 — ignite=5, burn=5
    // ========================================================================
    registerFireInfo(BaseBlocks::STRIPPED_OAK_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_SPRUCE_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_BIRCH_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_JUNGLE_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_ACACIA_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_DARK_OAK_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(CherryBlocks::STRIPPED_CHERRY_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(MangroveBlocks::STRIPPED_MANGROVE_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(PaleGardenBlocks::STRIPPED_PALE_OAK_LOG->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BambooBlocks::STRIPPED_BAMBOO_BLOCK->blockId(), IGNITE_HARD, BURN_HARD);

    // ========================================================================
    // 去皮木头类 — ignite=5, burn=5
    // ========================================================================
    registerFireInfo(BaseBlocks::STRIPPED_OAK_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_SPRUCE_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_BIRCH_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_JUNGLE_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_ACACIA_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::STRIPPED_DARK_OAK_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(CherryBlocks::STRIPPED_CHERRY_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(MangroveBlocks::STRIPPED_MANGROVE_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(PaleGardenBlocks::STRIPPED_PALE_OAK_WOOD->blockId(), IGNITE_HARD, BURN_HARD);

    // ========================================================================
    // 木头类 — ignite=5, burn=5
    // ========================================================================
    registerFireInfo(BaseBlocks::OAK_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::SPRUCE_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::BIRCH_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::JUNGLE_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::ACACIA_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(BaseBlocks::DARK_OAK_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(CherryBlocks::CHERRY_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(MangroveBlocks::MANGROVE_WOOD->blockId(), IGNITE_HARD, BURN_HARD);
    registerFireInfo(PaleGardenBlocks::PALE_OAK_WOOD->blockId(), IGNITE_HARD, BURN_HARD);

    // ========================================================================
    // 红树木根 — ignite=5, burn=20
    // ========================================================================
    registerFireInfo(MangroveBlocks::MANGROVE_ROOTS->blockId(), IGNITE_HARD, BURN_MEDIUM);

    // ========================================================================
    // 树叶类 — ignite=30, burn=60
    // ========================================================================
    registerFireInfo(BaseBlocks::OAK_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(BaseBlocks::SPRUCE_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(BaseBlocks::BIRCH_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(BaseBlocks::JUNGLE_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(BaseBlocks::ACACIA_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(BaseBlocks::DARK_OAK_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(CherryBlocks::CHERRY_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(MangroveBlocks::MANGROVE_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(PaleGardenBlocks::PALE_OAK_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(CaveBlocks::AZALEA_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(CaveBlocks::FLOWERING_AZALEA_LEAVES->blockId(), IGNITE_EASY, BURN_EASY);

    // ========================================================================
    // 羊毛类 — ignite=30, burn=60
    // ========================================================================
    registerFireInfo(ColoredBlocks::WHITE_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::ORANGE_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::MAGENTA_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::LIGHT_BLUE_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::YELLOW_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::LIME_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::PINK_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::GRAY_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::LIGHT_GRAY_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::CYAN_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::PURPLE_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::BLUE_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::BROWN_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::GREEN_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::RED_WOOL->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(ColoredBlocks::BLACK_WOOL->blockId(), IGNITE_EASY, BURN_EASY);

    // ========================================================================
    // 地毯类 — ignite=60, burn=20
    // ========================================================================
    registerFireInfo(ColoredBlocks::WHITE_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::ORANGE_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::MAGENTA_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::LIGHT_BLUE_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::YELLOW_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::LIME_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::PINK_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::GRAY_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::LIGHT_GRAY_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::CYAN_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::PURPLE_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::BLUE_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::BROWN_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::GREEN_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::RED_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);
    registerFireInfo(ColoredBlocks::BLACK_CARPET->blockId(), IGNITE_INSTANT, BURN_MEDIUM);

    // ========================================================================
    // 植物/花草类 — ignite=60, burn=100
    // ========================================================================
    registerFireInfo(VegetationBlocks::SHORT_GRASS->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::FERN->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(NaturalBlocks::DEAD_BUSH->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(GardenBlocks::SHORT_DRY_GRASS->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(GardenBlocks::TALL_DRY_GRASS->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::SUNFLOWER->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::LILAC->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::ROSE_BUSH->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::PEONY->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::TALL_GRASS->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::LARGE_FERN->blockId(), IGNITE_INSTANT, BURN_INSTANT);

    // 花
    registerFireInfo(VegetationBlocks::DANDELION->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::POPPY->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(PaleGardenBlocks::OPEN_EYEBLOSSOM->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(PaleGardenBlocks::CLOSED_EYEBLOSSOM->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::BLUE_ORCHID->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::ALLIUM->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::AZURE_BLUET->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::RED_TULIP->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::ORANGE_TULIP->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::WHITE_TULIP->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::PINK_TULIP->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::OXEYE_DAISY->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::CORNFLOWER->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::LILY_OF_THE_VALLEY->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(TrailsBlocks::TORCHFLOWER->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(TrailsBlocks::PITCHER_PLANT->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(VegetationBlocks::WITHER_ROSE->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(TrailsBlocks::PINK_PETALS->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(GardenBlocks::WILDFLOWERS->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(GardenBlocks::LEAF_LITTER->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(GardenBlocks::CACTUS_FLOWER->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(CaveBlocks::SPORE_BLOSSOM->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(GardenBlocks::BUSH->blockId(), IGNITE_INSTANT, BURN_INSTANT);

    // ========================================================================
    // 杂项可燃方块
    // ========================================================================

    // 书架 — ignite=30, burn=20
    registerFireInfo(BuildingBlocks::BOOKSHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);

    // TNT — ignite=15, burn=100
    registerFireInfo(BuildingBlocks::TNT->blockId(), IGNITE_MEDIUM, BURN_INSTANT);

    // 藤蔓 — ignite=15, burn=100
    registerFireInfo(NaturalBlocks::VINE->blockId(), IGNITE_MEDIUM, BURN_INSTANT);

    // 煤炭块 — ignite=5, burn=5
    registerFireInfo(BaseBlocks::COAL_BLOCK->blockId(), IGNITE_HARD, BURN_HARD);

    // 干草块 — ignite=60, burn=20
    registerFireInfo(BuildingBlocks::HAY_BLOCK->blockId(), IGNITE_INSTANT, BURN_MEDIUM);

    // 标靶 — ignite=15, burn=20
    registerFireInfo(RedstoneBlocks::TARGET->blockId(), IGNITE_MEDIUM, BURN_MEDIUM);

    // 干海带块 — ignite=30, burn=60
    registerFireInfo(NaturalBlocks::DRIED_KELP_BLOCK->blockId(), IGNITE_EASY, BURN_EASY);

    // 竹子 — ignite=60, burn=60
    registerFireInfo(VegetationBlocks::BAMBOO->blockId(), IGNITE_INSTANT, BURN_EASY);

    // 脚手架 — ignite=60, burn=60
    registerFireInfo(BuildingBlocks::SCAFFOLDING->blockId(), IGNITE_INSTANT, BURN_EASY);

    // 讲台 — ignite=30, burn=20
    registerFireInfo(BuildingBlocks::LECTERN->blockId(), IGNITE_EASY, BURN_MEDIUM);

    // 堆肥桶 — ignite=5, burn=20
    registerFireInfo(BuildingBlocks::COMPOSTER->blockId(), IGNITE_HARD, BURN_MEDIUM);

    // 洞穴藤蔓 — ignite=15, burn=60
    registerFireInfo(CaveBlocks::CAVE_VINES->blockId(), IGNITE_MEDIUM, BURN_EASY);
    registerFireInfo(CaveBlocks::CAVE_VINES_PLANT->blockId(), IGNITE_MEDIUM, BURN_EASY);

    // 杜鹃花 — ignite=30, burn=60
    registerFireInfo(CaveBlocks::AZALEA->blockId(), IGNITE_EASY, BURN_EASY);
    registerFireInfo(CaveBlocks::FLOWERING_AZALEA->blockId(), IGNITE_EASY, BURN_EASY);

    // 大垂滴叶 — ignite=60, burn=100
    registerFireInfo(CaveBlocks::BIG_DRIPLEAF->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(CaveBlocks::BIG_DRIPLEAF_STEM->blockId(), IGNITE_INSTANT, BURN_INSTANT);
    registerFireInfo(CaveBlocks::SMALL_DRIPLEAF->blockId(), IGNITE_INSTANT, BURN_INSTANT);

    // 垂根 — ignite=30, burn=60
    registerFireInfo(CaveBlocks::HANGING_ROOTS->blockId(), IGNITE_EASY, BURN_EASY);

    // 发光地衣 — ignite=15, burn=100
    registerFireInfo(CaveBlocks::GLOW_LICHEN->blockId(), IGNITE_MEDIUM, BURN_INSTANT);

    // ========================================================================
    // 苍白花园方块 — ignite=5, burn=100
    // ========================================================================
    registerFireInfo(PaleGardenBlocks::PALE_MOSS_BLOCK->blockId(), IGNITE_HARD, BURN_INSTANT);
    registerFireInfo(PaleGardenBlocks::PALE_MOSS_CARPET->blockId(), IGNITE_HARD, BURN_INSTANT);
    registerFireInfo(PaleGardenBlocks::PALE_HANGING_MOSS->blockId(), IGNITE_HARD, BURN_INSTANT);

    // 注意: 告示牌（SIGN/WALL_SIGN）和悬挂告示牌（HANGING_SIGN/WALL_HANGING_SIGN）
    // 在 MC 原版 FireBlock.bootStrap() 中未注册为可燃方块，此处不注册以保持一致。
    // 绯红木和诡异木告示牌同样不可燃。

    // 注意: 音符盒（NOTE_BLOCK）和唱片机（JUKEBOX）在 MC 原版 FireBlock.bootStrap()
    //       中未注册为可燃方块，此处不注册以保持一致。

    // 注意: 树苗（SAPLING）在 MC 原版 FireBlock.bootStrap() 中未注册为可燃方块，
    //       此处不注册以保持一致。

    // 注意: 蘑菇方块（BROWN_MUSHROOM_BLOCK/RED_MUSHROOM_BLOCK/MUSHROOM_STEM）
    //       在 MC 原版 FireBlock.bootStrap() 中未注册为可燃方块，此处不注册以保持一致。

    // 注意: 苍白橡木心材（CREAKING_HEART）在 MC 原版 FireBlock.bootStrap() 中
    //       未注册为可燃方块，此处不注册以保持一致。

    // ========================================================================
    // 甜浆果丛 — ignite=60, burn=100 (MC原版 FireBlock.bootStrap())
    // ========================================================================
    registerFireInfo(VegetationBlocks::SWEET_BERRY_BUSH->blockId(), IGNITE_INSTANT, BURN_INSTANT);

    // 注意: 可可豆（COCOA）、农作物（WHEAT/CARROTS/POTATOES/BEETROOTS）、
    //       南瓜/西瓜茎（PUMPKIN_STEM/MELON_STEM/ATTACHED_*_STEM）和甘蔗（SUGAR_CANE）
    //       在 MC 原版 FireBlock.bootStrap() 中未注册为可燃方块，此处不注册以保持一致。

    // ========================================================================
    // 蜂箱 — ignite=5, burn=20 (木质方块)
    // 蜂巢 — ignite=30, burn=20 (自然方块，更易燃)
    // 参考: MC原版 FireBlock.bootStrap()
    // ========================================================================
    registerFireInfo(NaturalBlocks::BEEHIVE->blockId(), IGNITE_HARD, BURN_MEDIUM);
    registerFireInfo(NaturalBlocks::BEE_NEST->blockId(), IGNITE_EASY, BURN_MEDIUM);

    // 萤火虫灌木 — ignite=60, burn=100
    registerFireInfo(GardenBlocks::FIREFLY_BUSH->blockId(), IGNITE_INSTANT, BURN_INSTANT);

    // ========================================================================
    // 木质书架（SHELF）— ignite=30, burn=20
    // 与普通书架（BOOKSHELF）相同，属于"容易点燃、中等燃烧"级别。
    // 注意：CRIMSON_SHELF 和 WARPED_SHELF 为下界木质，不可燃，不在此注册。
    // 参考: MC原版 FireBlock.bootStrap()
    // ========================================================================
    registerFireInfo(block_registry::ShelfBlocks::OAK_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::SPRUCE_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::BIRCH_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::JUNGLE_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::ACACIA_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::DARK_OAK_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::MANGROVE_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::CHERRY_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::PALE_OAK_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
    registerFireInfo(block_registry::ShelfBlocks::BAMBOO_SHELF->blockId(), IGNITE_EASY, BURN_MEDIUM);
}

} // namespace blocks
} // namespace mc
