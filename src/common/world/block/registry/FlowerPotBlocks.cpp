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

#include "world/block/registry/FlowerPotBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/decorative/FlowerPotBlock.hpp"
#include "world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace block_registry {

// ============================================================================
// 静态成员初始化
// ============================================================================

Block* FlowerPotBlocks::FLOWER_POT = nullptr;

// 树苗系列
Block* FlowerPotBlocks::POTTED_OAK_SAPLING = nullptr;
Block* FlowerPotBlocks::POTTED_SPRUCE_SAPLING = nullptr;
Block* FlowerPotBlocks::POTTED_BIRCH_SAPLING = nullptr;
Block* FlowerPotBlocks::POTTED_JUNGLE_SAPLING = nullptr;
Block* FlowerPotBlocks::POTTED_ACACIA_SAPLING = nullptr;
Block* FlowerPotBlocks::POTTED_DARK_OAK_SAPLING = nullptr;
Block* FlowerPotBlocks::POTTED_CHERRY_SAPLING = nullptr;
Block* FlowerPotBlocks::POTTED_PALE_OAK_SAPLING = nullptr;
Block* FlowerPotBlocks::POTTED_MANGROVE_PROPAGULE = nullptr;

// 花卉系列
Block* FlowerPotBlocks::POTTED_DANDELION = nullptr;
Block* FlowerPotBlocks::POTTED_POPPY = nullptr;
Block* FlowerPotBlocks::POTTED_BLUE_ORCHID = nullptr;
Block* FlowerPotBlocks::POTTED_ALLIUM = nullptr;
Block* FlowerPotBlocks::POTTED_AZURE_BLUET = nullptr;
Block* FlowerPotBlocks::POTTED_RED_TULIP = nullptr;
Block* FlowerPotBlocks::POTTED_ORANGE_TULIP = nullptr;
Block* FlowerPotBlocks::POTTED_WHITE_TULIP = nullptr;
Block* FlowerPotBlocks::POTTED_PINK_TULIP = nullptr;
Block* FlowerPotBlocks::POTTED_OXEYE_DAISY = nullptr;
Block* FlowerPotBlocks::POTTED_CORNFLOWER = nullptr;
Block* FlowerPotBlocks::POTTED_LILY_OF_THE_VALLEY = nullptr;
Block* FlowerPotBlocks::POTTED_WITHER_ROSE = nullptr;
Block* FlowerPotBlocks::POTTED_TORCHFLOWER = nullptr;
Block* FlowerPotBlocks::POTTED_OPEN_EYEBLOSSOM = nullptr;
Block* FlowerPotBlocks::POTTED_CLOSED_EYEBLOSSOM = nullptr;

// 蕨/枯草
Block* FlowerPotBlocks::POTTED_FERN = nullptr;
Block* FlowerPotBlocks::POTTED_DEAD_BUSH = nullptr;

// 蘑菇
Block* FlowerPotBlocks::POTTED_RED_MUSHROOM = nullptr;
Block* FlowerPotBlocks::POTTED_BROWN_MUSHROOM = nullptr;

// 仙人掌/竹子
Block* FlowerPotBlocks::POTTED_CACTUS = nullptr;
Block* FlowerPotBlocks::POTTED_BAMBOO = nullptr;

// 下界菌/菌索
Block* FlowerPotBlocks::POTTED_CRIMSON_FUNGUS = nullptr;
Block* FlowerPotBlocks::POTTED_WARPED_FUNGUS = nullptr;
Block* FlowerPotBlocks::POTTED_CRIMSON_ROOTS = nullptr;
Block* FlowerPotBlocks::POTTED_WARPED_ROOTS = nullptr;

// 杜鹃花
Block* FlowerPotBlocks::POTTED_AZALEA_BUSH = nullptr;
Block* FlowerPotBlocks::POTTED_FLOWERING_AZALEA_BUSH = nullptr;

namespace {

/// 花盆方块通用属性：瞬间破坏、非固体、无碰撞
/// 对应 MC Java 的 flowerPotProperties(): instabreak + noOcclusion + pushReaction(DESTROY)
BlockProperties flowerPotProperties()
{
    return BlockProperties(Material::DECORATION).noCollision().notSolid().hardness(0.0f).resistance(0.0f);
}

/// 注册一个 potted_* 方块
/// @param registry 方块注册表
/// @param pottedName 方块id路径（如 "potted_dandelion"）
/// @param content 内容物方块指针
/// @return 注册的花盆方块引用
Block& registerPotted(BlockRegistry& registry, const char* pottedName, const Block* content)
{
    return registry.registerBlock<blocks::FlowerPotBlock>(
        ResourceLocation("minecraft", pottedName), flowerPotProperties(), content);
}

} // namespace

void registerFlowerPotBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ========================================================================
    // 空花盆（minecraft:flower_pot）—— content 为 nullptr
    // 必须最先注册，因为 potted_* 的反查映射表在 FlowerPotBlock 构造时填充
    // ========================================================================
    FlowerPotBlocks::FLOWER_POT = &registry.registerBlock<blocks::FlowerPotBlock>(
        ResourceLocation("minecraft:flower_pot"), flowerPotProperties(), nullptr);

    // ========================================================================
    // 树苗系列
    // ========================================================================
    FlowerPotBlocks::POTTED_OAK_SAPLING = &registerPotted(registry, "potted_oak_sapling", VanillaBlocks::OAK_SAPLING);
    FlowerPotBlocks::POTTED_SPRUCE_SAPLING =
        &registerPotted(registry, "potted_spruce_sapling", VanillaBlocks::SPRUCE_SAPLING);
    FlowerPotBlocks::POTTED_BIRCH_SAPLING =
        &registerPotted(registry, "potted_birch_sapling", VanillaBlocks::BIRCH_SAPLING);
    FlowerPotBlocks::POTTED_JUNGLE_SAPLING =
        &registerPotted(registry, "potted_jungle_sapling", VanillaBlocks::JUNGLE_SAPLING);
    FlowerPotBlocks::POTTED_ACACIA_SAPLING =
        &registerPotted(registry, "potted_acacia_sapling", VanillaBlocks::ACACIA_SAPLING);
    FlowerPotBlocks::POTTED_DARK_OAK_SAPLING =
        &registerPotted(registry, "potted_dark_oak_sapling", VanillaBlocks::DARK_OAK_SAPLING);
    FlowerPotBlocks::POTTED_CHERRY_SAPLING =
        &registerPotted(registry, "potted_cherry_sapling", VanillaBlocks::CHERRY_SAPLING);
    FlowerPotBlocks::POTTED_PALE_OAK_SAPLING =
        &registerPotted(registry, "potted_pale_oak_sapling", VanillaBlocks::PALE_OAK_SAPLING);
    FlowerPotBlocks::POTTED_MANGROVE_PROPAGULE =
        &registerPotted(registry, "potted_mangrove_propagule", VanillaBlocks::MANGROVE_PROPAGULE);

    // ========================================================================
    // 花卉系列
    // ========================================================================
    FlowerPotBlocks::POTTED_DANDELION = &registerPotted(registry, "potted_dandelion", VanillaBlocks::DANDELION);
    FlowerPotBlocks::POTTED_POPPY = &registerPotted(registry, "potted_poppy", VanillaBlocks::POPPY);
    FlowerPotBlocks::POTTED_BLUE_ORCHID = &registerPotted(registry, "potted_blue_orchid", VanillaBlocks::BLUE_ORCHID);
    FlowerPotBlocks::POTTED_ALLIUM = &registerPotted(registry, "potted_allium", VanillaBlocks::ALLIUM);
    FlowerPotBlocks::POTTED_AZURE_BLUET = &registerPotted(registry, "potted_azure_bluet", VanillaBlocks::AZURE_BLUET);
    FlowerPotBlocks::POTTED_RED_TULIP = &registerPotted(registry, "potted_red_tulip", VanillaBlocks::RED_TULIP);
    FlowerPotBlocks::POTTED_ORANGE_TULIP =
        &registerPotted(registry, "potted_orange_tulip", VanillaBlocks::ORANGE_TULIP);
    FlowerPotBlocks::POTTED_WHITE_TULIP = &registerPotted(registry, "potted_white_tulip", VanillaBlocks::WHITE_TULIP);
    FlowerPotBlocks::POTTED_PINK_TULIP = &registerPotted(registry, "potted_pink_tulip", VanillaBlocks::PINK_TULIP);
    FlowerPotBlocks::POTTED_OXEYE_DAISY = &registerPotted(registry, "potted_oxeye_daisy", VanillaBlocks::OXEYE_DAISY);
    FlowerPotBlocks::POTTED_CORNFLOWER = &registerPotted(registry, "potted_cornflower", VanillaBlocks::CORNFLOWER);
    FlowerPotBlocks::POTTED_LILY_OF_THE_VALLEY =
        &registerPotted(registry, "potted_lily_of_the_valley", VanillaBlocks::LILY_OF_THE_VALLEY);
    FlowerPotBlocks::POTTED_WITHER_ROSE = &registerPotted(registry, "potted_wither_rose", VanillaBlocks::WITHER_ROSE);
    FlowerPotBlocks::POTTED_TORCHFLOWER = &registerPotted(registry, "potted_torchflower", VanillaBlocks::TORCHFLOWER);
    FlowerPotBlocks::POTTED_OPEN_EYEBLOSSOM =
        &registerPotted(registry, "potted_open_eyeblossom", VanillaBlocks::OPEN_EYEBLOSSOM);
    FlowerPotBlocks::POTTED_CLOSED_EYEBLOSSOM =
        &registerPotted(registry, "potted_closed_eyeblossom", VanillaBlocks::CLOSED_EYEBLOSSOM);

    // ========================================================================
    // 蕨/枯草
    // ========================================================================
    FlowerPotBlocks::POTTED_FERN = &registerPotted(registry, "potted_fern", VanillaBlocks::FERN);
    FlowerPotBlocks::POTTED_DEAD_BUSH = &registerPotted(registry, "potted_dead_bush", VanillaBlocks::DEAD_BUSH);

    // ========================================================================
    // 蘑菇
    // ========================================================================
    FlowerPotBlocks::POTTED_RED_MUSHROOM =
        &registerPotted(registry, "potted_red_mushroom", VanillaBlocks::RED_MUSHROOM);
    FlowerPotBlocks::POTTED_BROWN_MUSHROOM =
        &registerPotted(registry, "potted_brown_mushroom", VanillaBlocks::BROWN_MUSHROOM);

    // ========================================================================
    // 仙人掌/竹子
    // ========================================================================
    FlowerPotBlocks::POTTED_CACTUS = &registerPotted(registry, "potted_cactus", VanillaBlocks::CACTUS);
    FlowerPotBlocks::POTTED_BAMBOO = &registerPotted(registry, "potted_bamboo", VanillaBlocks::BAMBOO);

    // ========================================================================
    // 下界菌/菌索
    // ========================================================================
    FlowerPotBlocks::POTTED_CRIMSON_FUNGUS =
        &registerPotted(registry, "potted_crimson_fungus", VanillaBlocks::CRIMSON_FUNGUS);
    FlowerPotBlocks::POTTED_WARPED_FUNGUS =
        &registerPotted(registry, "potted_warped_fungus", VanillaBlocks::WARPED_FUNGUS);
    FlowerPotBlocks::POTTED_CRIMSON_ROOTS =
        &registerPotted(registry, "potted_crimson_roots", VanillaBlocks::CRIMSON_ROOTS);
    FlowerPotBlocks::POTTED_WARPED_ROOTS =
        &registerPotted(registry, "potted_warped_roots", VanillaBlocks::WARPED_ROOTS);

    // ========================================================================
    // 杜鹃花（注意：方块id为 potted_azalea_bush / potted_flowering_azalea_bush）
    // ========================================================================
    FlowerPotBlocks::POTTED_AZALEA_BUSH = &registerPotted(registry, "potted_azalea_bush", VanillaBlocks::AZALEA);
    FlowerPotBlocks::POTTED_FLOWERING_AZALEA_BUSH =
        &registerPotted(registry, "potted_flowering_azalea_bush", VanillaBlocks::FLOWERING_AZALEA);
}

} // namespace block_registry
} // namespace mc
