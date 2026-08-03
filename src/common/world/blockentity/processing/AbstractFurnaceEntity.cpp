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

#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/crafting/SmeltingRecipe.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/core/LockableBlockEntity.hpp"
#include "item/Items.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include <algorithm>
#include <initializer_list>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

namespace {

/// 火苗噼啪声概率 (每 tick 有 1/N 的概率播放音效)
constexpr i32 FIRE_CRACKLE_CHANCE = 20;

/**
 * @brief 检查物品是否为指定方块的方块物品
 * @param item 物品指针
 * @param block 方块指针
 * @return 如果物品是该方块的方块物品返回 true
 */
[[nodiscard]] bool isBlockItem(const Item* item, const Block* block)
{
    if (item == nullptr || block == nullptr) {
        return false;
    }
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*block);
    return blockItem != nullptr && item == static_cast<const Item*>(blockItem);
}

/**
 * @brief 检查物品对应的方块是否在指定方块列表中
 * @param item 物品指针
 * @param blocks 方块指针数组
 * @return 如果物品对应的方块在列表中返回 true
 */
[[nodiscard]] bool isBlockInList(const Item* item, std::initializer_list<const Block*> blocks)
{
    if (item == nullptr) {
        return false;
    }
    const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
    if (block == nullptr) {
        return false;
    }
    for (const Block* b : blocks) {
        if (b != nullptr && block == b) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查物品是否在指定物品列表中
 * @param item 物品指针
 * @param items 物品指针数组
 * @return 如果物品在列表中返回 true
 */
[[nodiscard]] bool isItemInList(const Item* item, std::initializer_list<const Item*> items)
{
    if (item == nullptr) {
        return false;
    }
    for (const Item* i : items) {
        if (i != nullptr && item == i) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 获取指定物品的燃烧时间。
 *
 * 燃烧时间单位：tick，参考 MC Java FuelValues。
 *
 * 注意：绯红(Crimson)和诡异(Warped)木质物品属于 NON_FLAMMABLE_WOOD，
 * 不可作为燃料，燃烧时间为 0。这与 MC Java 的标签排除逻辑一致。
 *
 * 苍白橡木(Pale Oak)是 MC 1.21.2+ 新增的木材类型，燃烧时间与其他主世界木材相同。
 */
[[nodiscard]] i32 getBurnTimeByItem(const Item* item)
{
    if (item == nullptr) {
        return 0;
    }

    // ========== 特殊燃料（高燃烧值）==========
    // 岩浆桶: 20000 tick (1000 秒) - 燃烧后返回空桶
    if (item == Items::LAVA_BUCKET) {
        return 20000;
    }

    // 烈焰棒: 2400 tick (120 秒)
    if (item == Items::BLAZE_ROD) {
        return 2400;
    }

    // ========== 煤炭类 ==========
    // 煤炭/木炭: 1600 tick (80 秒)
    if (item == Items::COAL || item == Items::CHARCOAL) {
        return 1600;
    }
    // 煤炭块: 16000 tick (800 秒)
    if (isBlockItem(item, VanillaBlocks::COAL_BLOCK)) {
        return 16000;
    }

    // ========== 木头类 (300 tick = 15 秒) ==========
    // 所有主世界木材类型的原木、木板、木头、去皮原木、去皮木头
    // 红树、樱花、苍白橡木、竹木额外包含原木和木头的变种
    // 注意：绯红茎和诡异茎属于 NON_FLAMMABLE_WOOD，不可燃
    // 原木
    if (isBlockInList(item,
            {VanillaBlocks::OAK_LOG,
                VanillaBlocks::SPRUCE_LOG,
                VanillaBlocks::BIRCH_LOG,
                VanillaBlocks::JUNGLE_LOG,
                VanillaBlocks::ACACIA_LOG,
                VanillaBlocks::DARK_OAK_LOG,
                VanillaBlocks::MANGROVE_LOG,
                VanillaBlocks::CHERRY_LOG,
                VanillaBlocks::PALE_OAK_LOG})) {
        return 300;
    }
    // 木板
    if (isBlockInList(item,
            {VanillaBlocks::OAK_PLANKS,
                VanillaBlocks::SPRUCE_PLANKS,
                VanillaBlocks::BIRCH_PLANKS,
                VanillaBlocks::JUNGLE_PLANKS,
                VanillaBlocks::ACACIA_PLANKS,
                VanillaBlocks::DARK_OAK_PLANKS,
                VanillaBlocks::MANGROVE_PLANKS,
                VanillaBlocks::CHERRY_PLANKS,
                VanillaBlocks::PALE_OAK_PLANKS,
                VanillaBlocks::BAMBOO_PLANKS,
                VanillaBlocks::BAMBOO_MOSAIC})) {
        return 300;
    }
    // 木头（六面树皮）
    if (isBlockInList(item,
            {VanillaBlocks::OAK_WOOD,
                VanillaBlocks::SPRUCE_WOOD,
                VanillaBlocks::BIRCH_WOOD,
                VanillaBlocks::JUNGLE_WOOD,
                VanillaBlocks::ACACIA_WOOD,
                VanillaBlocks::DARK_OAK_WOOD,
                VanillaBlocks::MANGROVE_WOOD,
                VanillaBlocks::CHERRY_WOOD,
                VanillaBlocks::PALE_OAK_WOOD})) {
        return 300;
    }
    // 去皮原木
    if (isBlockInList(item,
            {VanillaBlocks::STRIPPED_OAK_LOG,
                VanillaBlocks::STRIPPED_SPRUCE_LOG,
                VanillaBlocks::STRIPPED_BIRCH_LOG,
                VanillaBlocks::STRIPPED_JUNGLE_LOG,
                VanillaBlocks::STRIPPED_ACACIA_LOG,
                VanillaBlocks::STRIPPED_DARK_OAK_LOG,
                VanillaBlocks::STRIPPED_MANGROVE_LOG,
                VanillaBlocks::STRIPPED_CHERRY_LOG,
                VanillaBlocks::STRIPPED_PALE_OAK_LOG})) {
        return 300;
    }
    // 去皮木头（六面树皮）
    if (isBlockInList(item,
            {VanillaBlocks::STRIPPED_OAK_WOOD,
                VanillaBlocks::STRIPPED_SPRUCE_WOOD,
                VanillaBlocks::STRIPPED_BIRCH_WOOD,
                VanillaBlocks::STRIPPED_JUNGLE_WOOD,
                VanillaBlocks::STRIPPED_ACACIA_WOOD,
                VanillaBlocks::STRIPPED_DARK_OAK_WOOD,
                VanillaBlocks::STRIPPED_MANGROVE_WOOD,
                VanillaBlocks::STRIPPED_CHERRY_WOOD,
                VanillaBlocks::STRIPPED_PALE_OAK_WOOD})) {
        return 300;
    }
    // 竹木方块和去皮竹木方块
    if (isBlockInList(item,
            {VanillaBlocks::BAMBOO_BLOCK,
                VanillaBlocks::STRIPPED_BAMBOO_BLOCK,
                VanillaBlocks::BAMBOO_PLANKS,
                VanillaBlocks::BAMBOO_MOSAIC})) {
        return 300;
    }

    // ========== 木质建筑方块 (300 tick = 15 秒) ==========
    // 木质楼梯 - 所有可燃木材类型（绯红/诡异楼梯属于 NON_FLAMMABLE_WOOD，不可燃）
    if (isBlockInList(item,
            {VanillaBlocks::OAK_STAIRS,
                VanillaBlocks::SPRUCE_STAIRS,
                VanillaBlocks::BIRCH_STAIRS,
                VanillaBlocks::JUNGLE_STAIRS,
                VanillaBlocks::ACACIA_STAIRS,
                VanillaBlocks::DARK_OAK_STAIRS,
                VanillaBlocks::MANGROVE_STAIRS,
                VanillaBlocks::CHERRY_STAIRS,
                VanillaBlocks::PALE_OAK_STAIRS,
                VanillaBlocks::BAMBOO_STAIRS,
                VanillaBlocks::BAMBOO_MOSAIC_STAIRS})) {
        return 300;
    }
    // 栅栏 - 所有可燃木材类型
    if (isBlockInList(item,
            {VanillaBlocks::OAK_FENCE,
                VanillaBlocks::SPRUCE_FENCE,
                VanillaBlocks::BIRCH_FENCE,
                VanillaBlocks::JUNGLE_FENCE,
                VanillaBlocks::ACACIA_FENCE,
                VanillaBlocks::DARK_OAK_FENCE,
                VanillaBlocks::MANGROVE_FENCE,
                VanillaBlocks::CHERRY_FENCE,
                VanillaBlocks::PALE_OAK_FENCE,
                VanillaBlocks::BAMBOO_FENCE})) {
        return 300;
    }
    // 栅栏门 - 所有可燃木材类型（绯红/诡异栅栏门属于 NON_FLAMMABLE_WOOD，不可燃）
    if (isBlockInList(item,
            {VanillaBlocks::OAK_FENCE_GATE,
                VanillaBlocks::SPRUCE_FENCE_GATE,
                VanillaBlocks::BIRCH_FENCE_GATE,
                VanillaBlocks::JUNGLE_FENCE_GATE,
                VanillaBlocks::ACACIA_FENCE_GATE,
                VanillaBlocks::DARK_OAK_FENCE_GATE,
                VanillaBlocks::MANGROVE_FENCE_GATE,
                VanillaBlocks::CHERRY_FENCE_GATE,
                VanillaBlocks::PALE_OAK_FENCE_GATE,
                VanillaBlocks::BAMBOO_FENCE_GATE})) {
        return 300;
    }
    // 书架
    if (isBlockItem(item, VanillaBlocks::BOOKSHELF)) {
        return 300;
    }
    // 雕纹书架
    if (isBlockItem(item, VanillaBlocks::CHISELED_BOOKSHELF)) {
        return 300;
    }
    // 木质书架（SHELF）— 燃烧时间 = 基础木质(300) × 1.5 = 450
    if (isBlockInList(item,
            {VanillaBlocks::OAK_SHELF,
                VanillaBlocks::SPRUCE_SHELF,
                VanillaBlocks::BIRCH_SHELF,
                VanillaBlocks::JUNGLE_SHELF,
                VanillaBlocks::ACACIA_SHELF,
                VanillaBlocks::DARK_OAK_SHELF,
                VanillaBlocks::MANGROVE_SHELF,
                VanillaBlocks::CHERRY_SHELF,
                VanillaBlocks::PALE_OAK_SHELF,
                VanillaBlocks::BAMBOO_SHELF,
                VanillaBlocks::CRIMSON_SHELF,
                VanillaBlocks::WARPED_SHELF})) {
        return 450;
    }
    // 音符盒
    if (isBlockItem(item, VanillaBlocks::NOTE_BLOCK)) {
        return 300;
    }
    // 合成台
    if (isBlockItem(item, VanillaBlocks::CRAFTING_TABLE)) {
        return 300;
    }
    // 光照探测器
    if (isBlockItem(item, VanillaBlocks::DAYLIGHT_DETECTOR)) {
        return 300;
    }
    // 木质活板门 - 所有可燃木材类型（绯红/诡异活板门属于 NON_FLAMMABLE_WOOD，不可燃）
    if (isBlockInList(item,
            {VanillaBlocks::OAK_TRAPDOOR,
                VanillaBlocks::SPRUCE_TRAPDOOR,
                VanillaBlocks::BIRCH_TRAPDOOR,
                VanillaBlocks::JUNGLE_TRAPDOOR,
                VanillaBlocks::ACACIA_TRAPDOOR,
                VanillaBlocks::DARK_OAK_TRAPDOOR,
                VanillaBlocks::MANGROVE_TRAPDOOR,
                VanillaBlocks::CHERRY_TRAPDOOR,
                VanillaBlocks::PALE_OAK_TRAPDOOR,
                VanillaBlocks::BAMBOO_TRAPDOOR})) {
        return 300;
    }
    // 木质压力板 - 所有可燃木材类型
    if (isBlockInList(item,
            {VanillaBlocks::OAK_PRESSURE_PLATE,
                VanillaBlocks::SPRUCE_PRESSURE_PLATE,
                VanillaBlocks::BIRCH_PRESSURE_PLATE,
                VanillaBlocks::JUNGLE_PRESSURE_PLATE,
                VanillaBlocks::ACACIA_PRESSURE_PLATE,
                VanillaBlocks::DARK_OAK_PRESSURE_PLATE,
                VanillaBlocks::MANGROVE_PRESSURE_PLATE,
                VanillaBlocks::CHERRY_PRESSURE_PLATE,
                VanillaBlocks::PALE_OAK_PRESSURE_PLATE,
                VanillaBlocks::BAMBOO_PRESSURE_PLATE})) {
        return 300;
    }
    // 红树木根
    if (isBlockItem(item, VanillaBlocks::MANGROVE_ROOTS)) {
        return 300;
    }

    // ========== 木质台阶 (150 tick = 7.5 秒) ==========
    // 所有可燃木材类型的台阶（绯红/诡异台阶属于 NON_FLAMMABLE_WOOD，不可燃）
    if (isBlockInList(item,
            {VanillaBlocks::OAK_SLAB,
                VanillaBlocks::SPRUCE_SLAB,
                VanillaBlocks::BIRCH_SLAB,
                VanillaBlocks::JUNGLE_SLAB,
                VanillaBlocks::ACACIA_SLAB,
                VanillaBlocks::DARK_OAK_SLAB,
                VanillaBlocks::MANGROVE_SLAB,
                VanillaBlocks::CHERRY_SLAB,
                VanillaBlocks::PALE_OAK_SLAB,
                VanillaBlocks::BAMBOO_SLAB,
                VanillaBlocks::BAMBOO_MOSAIC_SLAB})) {
        return 150;
    }

    // ========== 木质门 (200 tick = 10 秒) ==========
    // MC Java: 木质门燃烧时间 200 tick（比其他木质建筑方块的 300 tick 短）
    // 绯红/诡异门属于 NON_FLAMMABLE_WOOD，不可燃
    if (isBlockInList(item,
            {VanillaBlocks::OAK_DOOR,
                VanillaBlocks::SPRUCE_DOOR,
                VanillaBlocks::BIRCH_DOOR,
                VanillaBlocks::JUNGLE_DOOR,
                VanillaBlocks::ACACIA_DOOR,
                VanillaBlocks::DARK_OAK_DOOR,
                VanillaBlocks::MANGROVE_DOOR,
                VanillaBlocks::CHERRY_DOOR,
                VanillaBlocks::PALE_OAK_DOOR,
                VanillaBlocks::BAMBOO_DOOR})) {
        return 200;
    }

    // ========== 木制工具 (200 tick = 10 秒) ==========
    if (item == Items::WOODEN_PICKAXE || item == Items::WOODEN_AXE || item == Items::WOODEN_SHOVEL ||
        item == Items::WOODEN_HOE || item == Items::WOODEN_SWORD) {
        return 200;
    }

    // ========== 弓、钓鱼竿、弩 (300 tick = 15 秒) ==========
    if (item == Items::BOW || item == Items::FISHING_ROD || item == Items::CROSSBOW) {
        return 300;
    }

    // ========== 木棍、碗、树苗、木质按钮 (100 tick = 5 秒) ==========
    if (item == Items::STICK || item == Items::BOWL) {
        return 100;
    }
    // 树苗 - 所有可燃木材类型（含红树繁殖体、樱花树苗、苍白橡树苗）
    if (isBlockInList(item,
            {VanillaBlocks::OAK_SAPLING,
                VanillaBlocks::SPRUCE_SAPLING,
                VanillaBlocks::BIRCH_SAPLING,
                VanillaBlocks::JUNGLE_SAPLING,
                VanillaBlocks::ACACIA_SAPLING,
                VanillaBlocks::DARK_OAK_SAPLING,
                VanillaBlocks::MANGROVE_PROPAGULE,
                VanillaBlocks::CHERRY_SAPLING,
                VanillaBlocks::PALE_OAK_SAPLING})) {
        return 100;
    }
    // 木质按钮（可燃木材，绯红/诡异按钮属于 NON_FLAMMABLE_WOOD，不可燃）
    if (isBlockInList(item,
            {VanillaBlocks::OAK_BUTTON,
                VanillaBlocks::SPRUCE_BUTTON,
                VanillaBlocks::BIRCH_BUTTON,
                VanillaBlocks::JUNGLE_BUTTON,
                VanillaBlocks::ACACIA_BUTTON,
                VanillaBlocks::DARK_OAK_BUTTON,
                VanillaBlocks::MANGROVE_BUTTON,
                VanillaBlocks::CHERRY_BUTTON,
                VanillaBlocks::PALE_OAK_BUTTON,
                VanillaBlocks::BAMBOO_BUTTON})) {
        return 100;
    }
    // 杜鹃花
    if (isBlockInList(item, {VanillaBlocks::AZALEA, VanillaBlocks::FLOWERING_AZALEA})) {
        return 100;
    }
    // 枯草
    if (isBlockInList(item, {VanillaBlocks::SHORT_DRY_GRASS, VanillaBlocks::TALL_DRY_GRASS})) {
        return 100;
    }
    // 落叶
    if (isBlockItem(item, VanillaBlocks::LEAF_LITTER)) {
        return 100;
    }

    // ========== 羊毛 (100 tick = 5 秒) ==========
    if (isBlockInList(item,
            {VanillaBlocks::WHITE_WOOL,
                VanillaBlocks::ORANGE_WOOL,
                VanillaBlocks::MAGENTA_WOOL,
                VanillaBlocks::LIGHT_BLUE_WOOL,
                VanillaBlocks::YELLOW_WOOL,
                VanillaBlocks::LIME_WOOL,
                VanillaBlocks::PINK_WOOL,
                VanillaBlocks::GRAY_WOOL,
                VanillaBlocks::LIGHT_GRAY_WOOL,
                VanillaBlocks::CYAN_WOOL,
                VanillaBlocks::PURPLE_WOOL,
                VanillaBlocks::BLUE_WOOL,
                VanillaBlocks::BROWN_WOOL,
                VanillaBlocks::GREEN_WOOL,
                VanillaBlocks::RED_WOOL,
                VanillaBlocks::BLACK_WOOL})) {
        return 100;
    }

    // ========== 竹子 (50 tick = 2.5 秒) ==========
    if (isBlockItem(item, VanillaBlocks::BAMBOO)) {
        return 50;
    }

    // ========== 脚手架 (50 tick = 2.5 秒) ==========
    if (item == Items::SCAFFOLDING) {
        return 50;
    }

    // ========== 干海带块 (4001 tick) ==========
    if (isBlockItem(item, VanillaBlocks::DRIED_KELP_BLOCK)) {
        return 4001;
    }

    // ========== 梯子 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::LADDER)) {
        return 300;
    }

    // ========== 死灌木 (100 tick = 5 秒) ==========
    if (isBlockItem(item, VanillaBlocks::DEAD_BUSH)) {
        return 100;
    }

    // ========== 箱子 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::CHEST)) {
        return 300;
    }

    // ========== 陷阱箱 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::TRAPPED_CHEST)) {
        return 300;
    }

    // ========== 织布机 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::LOOM)) {
        return 300;
    }

    // ========== 木桶 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::BARREL)) {
        return 300;
    }

    // ========== 制图台 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::CARTOGRAPHY_TABLE)) {
        return 300;
    }

    // ========== 制箭台 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::FLETCHING_TABLE)) {
        return 300;
    }

    // ========== 锻造台 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::SMITHING_TABLE)) {
        return 300;
    }

    // ========== 堆肥桶 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::COMPOSTER)) {
        return 300;
    }

    // ========== 讲台 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::LECTERN)) {
        return 300;
    }

    // ========== 唱片机 (300 tick = 15 秒) ==========
    if (isBlockItem(item, VanillaBlocks::JUKEBOX)) {
        return 300;
    }

    // ========== 地毯 (67 tick) ==========
    if (isBlockInList(item,
            {VanillaBlocks::WHITE_CARPET,
                VanillaBlocks::ORANGE_CARPET,
                VanillaBlocks::MAGENTA_CARPET,
                VanillaBlocks::LIGHT_BLUE_CARPET,
                VanillaBlocks::YELLOW_CARPET,
                VanillaBlocks::LIME_CARPET,
                VanillaBlocks::PINK_CARPET,
                VanillaBlocks::GRAY_CARPET,
                VanillaBlocks::LIGHT_GRAY_CARPET,
                VanillaBlocks::CYAN_CARPET,
                VanillaBlocks::PURPLE_CARPET,
                VanillaBlocks::BLUE_CARPET,
                VanillaBlocks::BROWN_CARPET,
                VanillaBlocks::GREEN_CARPET,
                VanillaBlocks::RED_CARPET,
                VanillaBlocks::BLACK_CARPET})) {
        return 67;
    }

    // ========== 告示牌 (200 tick = 10 秒) ==========
    // 所有可燃木材类型的告示牌（绯红/诡异告示牌属于 NON_FLAMMABLE_WOOD，不可燃）
    if (isItemInList(item,
            {Items::OAK_SIGN,
                Items::SPRUCE_SIGN,
                Items::BIRCH_SIGN,
                Items::JUNGLE_SIGN,
                Items::ACACIA_SIGN,
                Items::DARK_OAK_SIGN,
                Items::MANGROVE_SIGN,
                Items::CHERRY_SIGN,
                Items::BAMBOO_SIGN,
                Items::PALE_OAK_SIGN})) {
        return 200;
    }

    // ========== 悬挂告示牌 (800 tick = 40 秒) ==========
    // MC Java: 悬挂告示牌燃烧时间是普通告示牌的 4 倍
    // 绯红/诡异悬挂告示牌属于 NON_FLAMMABLE_WOOD，不可燃
    if (isBlockInList(item,
            {VanillaBlocks::OAK_HANGING_SIGN,
                VanillaBlocks::SPRUCE_HANGING_SIGN,
                VanillaBlocks::BIRCH_HANGING_SIGN,
                VanillaBlocks::JUNGLE_HANGING_SIGN,
                VanillaBlocks::ACACIA_HANGING_SIGN,
                VanillaBlocks::DARK_OAK_HANGING_SIGN,
                VanillaBlocks::MANGROVE_HANGING_SIGN,
                VanillaBlocks::CHERRY_HANGING_SIGN,
                VanillaBlocks::PALE_OAK_HANGING_SIGN,
                VanillaBlocks::BAMBOO_HANGING_SIGN})) {
        return 800;
    }

    // ========== 木船和带箱子的船 (1200 tick = 60 秒) ==========
    // MC Java: 所有类型的船和带箱子的船燃烧时间均为 1200 tick
    if (isItemInList(item,
            {Items::OAK_BOAT,
                Items::SPRUCE_BOAT,
                Items::BIRCH_BOAT,
                Items::JUNGLE_BOAT,
                Items::ACACIA_BOAT,
                Items::DARK_OAK_BOAT,
                Items::MANGROVE_BOAT,
                Items::CHERRY_BOAT,
                Items::PALE_OAK_BOAT,
                Items::BAMBOO_RAFT,
                Items::OAK_CHEST_BOAT,
                Items::SPRUCE_CHEST_BOAT,
                Items::BIRCH_CHEST_BOAT,
                Items::JUNGLE_CHEST_BOAT,
                Items::ACACIA_CHEST_BOAT,
                Items::DARK_OAK_CHEST_BOAT,
                Items::MANGROVE_CHEST_BOAT,
                Items::CHERRY_CHEST_BOAT,
                Items::PALE_OAK_CHEST_BOAT,
                Items::BAMBOO_CHEST_RAFT})) {
        return 1200;
    }

    // ========== 旗帜 (300 tick = 15 秒) ==========
    if (isItemInList(item,
            {Items::WHITE_BANNER,
                Items::ORANGE_BANNER,
                Items::MAGENTA_BANNER,
                Items::LIGHT_BLUE_BANNER,
                Items::YELLOW_BANNER,
                Items::LIME_BANNER,
                Items::PINK_BANNER,
                Items::GRAY_BANNER,
                Items::LIGHT_GRAY_BANNER,
                Items::CYAN_BANNER,
                Items::PURPLE_BANNER,
                Items::BLUE_BANNER,
                Items::BROWN_BANNER,
                Items::GREEN_BANNER,
                Items::RED_BANNER,
                Items::BLACK_BANNER})) {
        return 300;
    }

    return 0;
}
} // namespace

AbstractFurnaceEntity::AbstractFurnaceEntity(BlockEntityType type, const BlockPos& pos)
    : LockableBlockEntity(type, pos)
    , m_inventory([this]() { this->BlockEntity::setChanged(); })
{}

void AbstractFurnaceEntity::tick(IWorld& world)
{
    bool wasBurning = isBurning();

    if (isBurning()) {
        --m_burnTime;

        // 火苗噼啪声 - 燃烧时随机播放
        if (!world.isClientSide() && world.getRandom().nextInt(FIRE_CRACKLE_CHANCE) == 0) {
            ResourceLocation soundEvent = getFireCrackleSound();
            world.playSound(soundEvent, sound::SoundCategory::Blocks, m_pos.center(), 1.0f, 1.0f);
        }
    }

    ItemStack fuelItem = m_inventory.getFuelItem();

    const crafting::SmeltingRecipe* cachedRecipe = getRecipe(world);
    bool canSmeltNow = canSmeltWithRecipe(cachedRecipe);

    if (isBurning() || (!fuelItem.isEmpty() && canSmeltNow)) {
        if (!isBurning() && canSmeltNow) {
            m_burnTimeTotal = getBurnTimeForFuel(fuelItem);
            m_burnTime = m_burnTimeTotal;

            if (isBurning()) {
                burnFuel();
            }
        }

        if (isBurning() && canSmeltNow) {
            ++m_cookTime;

            if (m_cookTime >= m_cookTimeTotal) {
                m_cookTime = 0;
                m_cookTimeTotal = getCookTimeFromRecipe(cachedRecipe);
                smeltWithRecipe(cachedRecipe);
                BlockEntity::setChanged();
            }
        } else if (!canSmeltNow) {
            m_cookTime = std::max(0, m_cookTime - 2);
        }
    } else if (!isBurning() && m_cookTime > 0) {
        m_cookTime = std::max(0, m_cookTime - 2);
    }

    if (wasBurning != isBurning()) {
        updateBurnState(world);
        setChanged();
    }
}

bool AbstractFurnaceEntity::load(const nlohmann::json& data)
{
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    if (data.contains("BurnTime") && data["BurnTime"].is_number()) {
        m_burnTime = data["BurnTime"].get<i32>();
    }

    if (data.contains("CookTime") && data["CookTime"].is_number()) {
        m_cookTime = data["CookTime"].get<i32>();
    }

    if (data.contains("CookTimeTotal") && data["CookTimeTotal"].is_number()) {
        m_cookTimeTotal = data["CookTimeTotal"].get<i32>();
    }

    if (data.contains("StoredExperience") && data["StoredExperience"].is_number()) {
        m_storedExperience = data["StoredExperience"].get<f32>();
    }

    m_burnTimeTotal = getBurnTimeForFuel(m_inventory.getFuelItem());

    if (data.contains("Items") && data["Items"].is_array()) {
        m_inventory.clear();
        for (const auto& itemJson : data["Items"]) {
            if (!itemJson.is_object()) {
                continue;
            }

            const i32 slot = itemJson.value("Slot", -1);
            if (slot < SLOT_INPUT || slot > SLOT_OUTPUT) {
                continue;
            }

            auto stackResult = ItemStack::fromJson(itemJson);
            if (!stackResult.success()) {
                continue;
            }

            m_inventory.setItem(slot, stackResult.value());
        }

        m_burnTimeTotal = getBurnTimeForFuel(m_inventory.getFuelItem());
    }

    return true;
}

void AbstractFurnaceEntity::save(nlohmann::json& data) const
{
    LockableBlockEntity::save(data);

    data["BurnTime"] = m_burnTime;
    data["CookTime"] = m_cookTime;
    data["CookTimeTotal"] = m_cookTimeTotal;
    data["StoredExperience"] = m_storedExperience;

    nlohmann::json itemsJson = nlohmann::json::array();
    for (i32 slot = SLOT_INPUT; slot <= SLOT_OUTPUT; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }

        nlohmann::json itemJson = stack.toJson();
        itemJson["Slot"] = slot;
        itemsJson.push_back(std::move(itemJson));
    }
    data["Items"] = std::move(itemsJson);
}

i32 AbstractFurnaceEntity::getComparatorSignal() const
{
    i32 totalItems = 0;
    i32 totalSlots = 3;
    i32 maxPerSlot = 64;

    for (i32 i = 0; i < totalSlots; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            totalItems += stack.getCount();
        }
    }

    i32 maxItems = totalSlots * maxPerSlot;
    if (maxItems == 0) {
        return 0;
    }

    f32 fillRatio = static_cast<f32>(totalItems) / static_cast<f32>(maxItems);
    i32 signal = static_cast<i32>(fillRatio * 14.0f);

    if (totalItems > 0) {
        signal += 1;
    }

    return std::min(signal, 15);
}

bool AbstractFurnaceEntity::isFuel(const ItemStack& stack)
{
    return getBurnTime(stack) > 0;
}

i32 AbstractFurnaceEntity::getBurnTime(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return 0;
    }

    return getBurnTimeByItem(stack.getItem());
}

i32 AbstractFurnaceEntity::getCookTime(IWorld& world) const
{
    const crafting::SmeltingRecipe* recipe = getRecipe(world);
    return getCookTimeFromRecipe(recipe);
}

i32 AbstractFurnaceEntity::getCookTimeFromRecipe(const crafting::SmeltingRecipe* recipe) const
{
    if (recipe != nullptr) {
        return recipe->getCookTime();
    }
    return getDefaultCookTime();
}

bool AbstractFurnaceEntity::canSmelt(IWorld& world) const
{
    return canSmeltWithRecipe(getRecipe(world));
}

bool AbstractFurnaceEntity::canSmeltWithRecipe(const crafting::SmeltingRecipe* recipe) const
{
    const ItemStack& input = m_inventory.getInputItem();
    if (input.isEmpty()) {
        return false;
    }

    if (recipe == nullptr) {
        return false;
    }

    const ItemStack& output = m_inventory.getOutputItem();
    const ItemStack& result = recipe->getResultItem();

    if (output.isEmpty()) {
        return true;
    }

    if (!output.canStackWith(result)) {
        return false;
    }

    i32 resultCount = output.getCount() + result.getCount();
    return resultCount <= output.getMaxStackSize();
}

void AbstractFurnaceEntity::smelt(IWorld& world)
{
    smeltWithRecipe(getRecipe(world));
}

void AbstractFurnaceEntity::smeltWithRecipe(const crafting::SmeltingRecipe* recipe)
{
    if (!canSmeltWithRecipe(recipe)) {
        return;
    }

    MC_ASSERT(recipe != nullptr);

    ItemStack input = m_inventory.getInputItem();
    ItemStack result = recipe->getResultItem().copy();

    input.shrink(1);
    m_inventory.setInputItem(input);

    ItemStack output = m_inventory.getOutputItem();
    if (output.isEmpty()) {
        m_inventory.setOutputItem(result.copy());
    } else {
        output.grow(result.getCount());
        m_inventory.setOutputItem(output);
    }

    // 湿海绵干燥逻辑：当输入是湿海绵且燃料槽有空桶时，将空桶变为水桶
    if (input.getItem() == Items::WET_SPONGE) {
        ItemStack fuelItem = m_inventory.getFuelItem();
        if (!fuelItem.isEmpty() && fuelItem.getItem() == Items::BUCKET) {
            m_inventory.setFuelItem(ItemStack(Items::WATER_BUCKET, 1));
        }
    }

    m_storedExperience += recipe->getExperience();

    setChanged();
}

bool AbstractFurnaceEntity::burnFuel()
{
    ItemStack fuel = m_inventory.getFuelItem();
    if (fuel.isEmpty()) {
        return false;
    }

    // 容器物品消耗逻辑：有容器物品的情况（如岩浆桶），直接用容器物品替换燃料槽
    if (fuel.hasContainerItem()) {
        m_inventory.setFuelItem(fuel.getContainerItem());
    } else {
        // 没有容器物品的情况，减少燃料数量
        fuel.shrink(1);
        m_inventory.setFuelItem(fuel);
    }

    return true;
}

void AbstractFurnaceEntity::updateBurnState(IWorld& world)
{
    (void)world;
}

const crafting::SmeltingRecipe* AbstractFurnaceEntity::getRecipe(IWorld& world) const
{
    (void)world;

    const ItemStack& input = m_inventory.getInputItem();
    if (input.isEmpty()) {
        return nullptr;
    }

    return crafting::RecipeManager::instance().getSmeltingRecipe(input, getRecipeType());
}

// ========== ISidedInventory 接口 ==========

std::vector<i32> AbstractFurnaceEntity::getSlotsForFace(Direction side) const
{
    // 熔炉槽位访问规则：
    // - 上方 (Direction::Up)：输入槽（槽位 0）
    // - 下方 (Direction::Down)：输出槽（槽位 2）、燃料槽（槽位 1）
    // - 侧面：燃料槽（槽位 1）
    switch (side) {
        case Direction::Up:
            // 上方：输入槽
            return {SLOT_INPUT};
        case Direction::Down:
            // 下方：输出槽、燃料槽
            return {SLOT_OUTPUT, SLOT_FUEL};
        default:
            // 侧面（北、南、东、西）：燃料槽
            return {SLOT_FUEL};
    }
}

bool AbstractFurnaceEntity::_isSlotAccessibleForDirection(i32 slot, Direction direction) const
{
    const std::vector<i32> accessibleSlots = getSlotsForFace(direction);
    for (i32 accessibleSlot : accessibleSlots) {
        if (accessibleSlot == slot) {
            return true;
        }
    }
    return false;
}

bool AbstractFurnaceEntity::canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    // 首先检查方向是否允许访问该槽位
    if (!_isSlotAccessibleForDirection(slot, direction)) {
        return false;
    }

    // 根据槽位类型检查物品是否有效
    switch (slot) {
        case SLOT_INPUT:
            // 输入槽：接受任何物品（熔炼配方检查在 tick 中进行）
            return true;
        case SLOT_FUEL: {
            // 燃料槽：接受燃料
            if (isFuel(stack)) {
                return true;
            }
            // 或者如果燃料槽当前不是空桶则接受空桶（用于装岩浆桶燃烧后的空桶）
            if (stack.getItem() == Items::BUCKET) {
                const ItemStack fuelItem = m_inventory.getFuelItem();
                return fuelItem.isEmpty() || fuelItem.getItem() != Items::BUCKET;
            }
            return false;
        }
        case SLOT_OUTPUT:
            // 输出槽：不允许插入（只能提取）
            return false;
        default:
            return false;
    }
}

bool AbstractFurnaceEntity::canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    // 从下方提取燃料槽时，只允许提取空桶或水桶
    // 这是为了防止漏斗从燃料槽中提取部分使用的燃料（如岩浆桶）
    if (direction == Direction::Down && slot == SLOT_FUEL) {
        const Item* item = stack.getItem();
        // 只允许提取空桶和水桶（湿海绵干燥产生的水桶）
        return item == Items::BUCKET || item == Items::WATER_BUCKET;
    }

    return true;
}

} // namespace blockentity
} // namespace mc