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

#include "BlockItemRegistry.hpp"

#include "GameMasterBlockItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <functional>
#include <string>
#include <spdlog/spdlog.h>

namespace mc {

BlockItemRegistry& BlockItemRegistry::instance() noexcept
{
    static BlockItemRegistry instance;
    return instance;
}

void BlockItemRegistry::registerBlockItem(const Block& block, BlockItem& item)
{
    const BlockItem* itemPtr = &item;
    const ItemId itemId = item.itemId();

    // 存储映射关系
    m_blockToItem[block.blockId()] = itemPtr;
    m_itemToBlock[itemId] = &block;
    m_itemIdToBlockItem[itemId] = itemPtr;
}

const BlockItem* BlockItemRegistry::getBlockItem(u32 blockId) const noexcept
{
    auto it = m_blockToItem.find(blockId);
    return it != m_blockToItem.end() ? it->second : nullptr;
}

const BlockItem* BlockItemRegistry::getBlockItemByItemId(ItemId itemId) const noexcept
{
    auto it = m_itemIdToBlockItem.find(itemId);
    return it != m_itemIdToBlockItem.end() ? it->second : nullptr;
}

const BlockItem* BlockItemRegistry::getBlockItem(const Block& block) const noexcept
{
    return getBlockItem(block.blockId());
}

const Block* BlockItemRegistry::getBlock(ItemId itemId) const noexcept
{
    auto it = m_itemToBlock.find(itemId);
    return it != m_itemToBlock.end() ? it->second : nullptr;
}

bool BlockItemRegistry::isBlockItem(const Item* item) const noexcept
{
    if (item == nullptr) {
        return false;
    }
    return isBlockItem(item->itemId());
}

bool BlockItemRegistry::isBlockItem(ItemId itemId) const noexcept
{
    return m_itemToBlock.find(itemId) != m_itemToBlock.end();
}

void BlockItemRegistry::forEachBlockItem(std::function<void(const BlockItem&)> callback) const
{
    for (const auto& [blockId, item] : m_blockToItem) {
        if (item != nullptr) {
            callback(*item);
        }
    }
}

void BlockItemRegistry::clear() noexcept
{
    m_blockToItem.clear();
    m_itemToBlock.clear();
    m_itemIdToBlockItem.clear();
    m_initialized = false;
}

void BlockItemRegistry::initializeVanillaBlockItems()
{
    if (m_initialized) {
        return;
    }

    spdlog::info("Initializing vanilla block items...");

    auto registerSimpleBlock = [this](Block* block, const std::string& name) {
        if (block == nullptr) {
            spdlog::warn("Block '{}' is null, skipping", name);
            return;
        }

        // 用传入的 name 构造物品资源位置，而非方块的 blockLocation。
        // vanilla 中部分方块（potted_*、wall_torch、candle_cake 等）没有与方块同名的物品，
        // 其物品映射到另一个名字（potted_* → flower_pot、wall_torch → torch 等），
        // 这些方块的注册调用已传入正确的物品名；若用 blockLocation 则会得到不存在的物品名，
        // 导致 JavaItemIdMap 映射失败兜底成 air。
        ResourceLocation itemLoc("minecraft", name);

        BlockItem* registeredItem = nullptr;
        Item* existingItem = ItemRegistry::instance().getItem(itemLoc);
        if (existingItem != nullptr) {
            registeredItem = dynamic_cast<BlockItem*>(existingItem);
            if (registeredItem == nullptr) {
                spdlog::warn("Item '{}' already exists but is not a BlockItem, skipping", itemLoc.toString());
                return;
            }
        } else {
            registeredItem =
                &ItemRegistry::instance().registerItem<BlockItem>(itemLoc, *block, ItemProperties().maxStackSize(64));
        }

        // 获取注册后的信息
        u32 blockId = block->blockId();
        ItemId itemId = registeredItem->itemId();

        // 存储映射关系（ItemRegistry 拥有物品的所有权）
        m_blockToItem[blockId] = registeredItem;
        m_itemToBlock[itemId] = block;
        m_itemIdToBlockItem[itemId] = registeredItem;
    };

    // 基础方块
    registerSimpleBlock(VanillaBlocks::STONE, "stone");
    registerSimpleBlock(VanillaBlocks::GRASS_BLOCK, "grass_block");
    registerSimpleBlock(VanillaBlocks::DIRT, "dirt");
    registerSimpleBlock(VanillaBlocks::COBBLESTONE, "cobblestone");
    registerSimpleBlock(VanillaBlocks::OAK_PLANKS, "oak_planks");
    registerSimpleBlock(VanillaBlocks::BEDROCK, "bedrock");
    registerSimpleBlock(VanillaBlocks::SAND, "sand");
    registerSimpleBlock(VanillaBlocks::SUSPICIOUS_SAND, "suspicious_sand");
    registerSimpleBlock(VanillaBlocks::GRAVEL, "gravel");
    registerSimpleBlock(VanillaBlocks::SUSPICIOUS_GRAVEL, "suspicious_gravel");

    // 石头变种
    registerSimpleBlock(VanillaBlocks::GRANITE, "granite");
    registerSimpleBlock(VanillaBlocks::POLISHED_GRANITE, "polished_granite");
    registerSimpleBlock(VanillaBlocks::DIORITE, "diorite");
    registerSimpleBlock(VanillaBlocks::POLISHED_DIORITE, "polished_diorite");
    registerSimpleBlock(VanillaBlocks::ANDESITE, "andesite");
    registerSimpleBlock(VanillaBlocks::POLISHED_ANDESITE, "polished_andesite");

    // 泥土变种
    registerSimpleBlock(VanillaBlocks::COARSE_DIRT, "coarse_dirt");
    registerSimpleBlock(VanillaBlocks::PODZOL, "podzol");

    // 砂岩
    registerSimpleBlock(VanillaBlocks::SANDSTONE, "sandstone");
    registerSimpleBlock(VanillaBlocks::CHISELED_SANDSTONE, "chiseled_sandstone");
    registerSimpleBlock(VanillaBlocks::CUT_SANDSTONE, "cut_sandstone");
    registerSimpleBlock(VanillaBlocks::SMOOTH_SANDSTONE, "smooth_sandstone");
    registerSimpleBlock(VanillaBlocks::RED_SANDSTONE, "red_sandstone");
    registerSimpleBlock(VanillaBlocks::CHISELED_RED_SANDSTONE, "chiseled_red_sandstone");
    registerSimpleBlock(VanillaBlocks::CUT_RED_SANDSTONE, "cut_red_sandstone");
    registerSimpleBlock(VanillaBlocks::SMOOTH_RED_SANDSTONE, "smooth_red_sandstone");

    // 矿石
    registerSimpleBlock(VanillaBlocks::GOLD_ORE, "gold_ore");
    registerSimpleBlock(VanillaBlocks::IRON_ORE, "iron_ore");
    registerSimpleBlock(VanillaBlocks::COAL_ORE, "coal_ore");
    registerSimpleBlock(VanillaBlocks::DIAMOND_ORE, "diamond_ore");
    registerSimpleBlock(VanillaBlocks::EMERALD_ORE, "emerald_ore");
    registerSimpleBlock(VanillaBlocks::LAPIS_ORE, "lapis_ore");
    registerSimpleBlock(VanillaBlocks::REDSTONE_ORE, "redstone_ore");

    // 矿物方块
    registerSimpleBlock(VanillaBlocks::COAL_BLOCK, "coal_block");
    registerSimpleBlock(VanillaBlocks::GOLD_BLOCK, "gold_block");
    registerSimpleBlock(VanillaBlocks::IRON_BLOCK, "iron_block");
    registerSimpleBlock(VanillaBlocks::DIAMOND_BLOCK, "diamond_block");
    registerSimpleBlock(VanillaBlocks::EMERALD_BLOCK, "emerald_block");
    registerSimpleBlock(VanillaBlocks::LAPIS_BLOCK, "lapis_block");
    registerSimpleBlock(VanillaBlocks::REDSTONE_BLOCK, "redstone_block");
    registerSimpleBlock(VanillaBlocks::NETHERITE_BLOCK, "netherite_block");

    // 粗矿块
    registerSimpleBlock(VanillaBlocks::RAW_IRON_BLOCK, "raw_iron_block");
    registerSimpleBlock(VanillaBlocks::RAW_COPPER_BLOCK, "raw_copper_block");
    registerSimpleBlock(VanillaBlocks::RAW_GOLD_BLOCK, "raw_gold_block");

    // 铜块（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_BLOCK, "copper_block");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER, "exposed_copper");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER, "weathered_copper");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER, "oxidized_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_BLOCK, "waxed_copper_block");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER, "waxed_exposed_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER, "waxed_weathered_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER, "waxed_oxidized_copper");

    // 切制铜（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::CUT_COPPER, "cut_copper");
    registerSimpleBlock(VanillaBlocks::EXPOSED_CUT_COPPER, "exposed_cut_copper");
    registerSimpleBlock(VanillaBlocks::WEATHERED_CUT_COPPER, "weathered_cut_copper");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_CUT_COPPER, "oxidized_cut_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_CUT_COPPER, "waxed_cut_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_CUT_COPPER, "waxed_exposed_cut_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_CUT_COPPER, "waxed_weathered_cut_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER, "waxed_oxidized_cut_copper");

    // 凿制铜（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::CHISELED_COPPER, "chiseled_copper");
    registerSimpleBlock(VanillaBlocks::EXPOSED_CHISELED_COPPER, "exposed_chiseled_copper");
    registerSimpleBlock(VanillaBlocks::WEATHERED_CHISELED_COPPER, "weathered_chiseled_copper");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_CHISELED_COPPER, "oxidized_chiseled_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_CHISELED_COPPER, "waxed_chiseled_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_CHISELED_COPPER, "waxed_exposed_chiseled_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_CHISELED_COPPER, "waxed_weathered_chiseled_copper");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_CHISELED_COPPER, "waxed_oxidized_chiseled_copper");

    // 建筑方块
    registerSimpleBlock(VanillaBlocks::BRICKS, "bricks");
    registerSimpleBlock(VanillaBlocks::MOSSY_COBBLESTONE, "mossy_cobblestone");
    registerSimpleBlock(VanillaBlocks::BOOKSHELF, "bookshelf");

    // 木质书架变体（1.21.4+）
    registerSimpleBlock(VanillaBlocks::OAK_SHELF, "oak_shelf");
    registerSimpleBlock(VanillaBlocks::SPRUCE_SHELF, "spruce_shelf");
    registerSimpleBlock(VanillaBlocks::BIRCH_SHELF, "birch_shelf");
    registerSimpleBlock(VanillaBlocks::JUNGLE_SHELF, "jungle_shelf");
    registerSimpleBlock(VanillaBlocks::ACACIA_SHELF, "acacia_shelf");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_SHELF, "dark_oak_shelf");
    registerSimpleBlock(VanillaBlocks::MANGROVE_SHELF, "mangrove_shelf");
    registerSimpleBlock(VanillaBlocks::CHERRY_SHELF, "cherry_shelf");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_SHELF, "pale_oak_shelf");
    registerSimpleBlock(VanillaBlocks::BAMBOO_SHELF, "bamboo_shelf");
    registerSimpleBlock(VanillaBlocks::CRIMSON_SHELF, "crimson_shelf");
    registerSimpleBlock(VanillaBlocks::WARPED_SHELF, "warped_shelf");
    registerSimpleBlock(VanillaBlocks::OBSIDIAN, "obsidian");

    // 木板变种
    registerSimpleBlock(VanillaBlocks::SPRUCE_PLANKS, "spruce_planks");
    registerSimpleBlock(VanillaBlocks::BIRCH_PLANKS, "birch_planks");
    registerSimpleBlock(VanillaBlocks::JUNGLE_PLANKS, "jungle_planks");
    registerSimpleBlock(VanillaBlocks::ACACIA_PLANKS, "acacia_planks");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_PLANKS, "dark_oak_planks");
    registerSimpleBlock(VanillaBlocks::BAMBOO_PLANKS, "bamboo_planks");
    registerSimpleBlock(VanillaBlocks::BAMBOO_MOSAIC, "bamboo_mosaic");

    // 原木
    registerSimpleBlock(VanillaBlocks::OAK_LOG, "oak_log");
    registerSimpleBlock(VanillaBlocks::SPRUCE_LOG, "spruce_log");
    registerSimpleBlock(VanillaBlocks::BIRCH_LOG, "birch_log");
    registerSimpleBlock(VanillaBlocks::JUNGLE_LOG, "jungle_log");
    registerSimpleBlock(VanillaBlocks::ACACIA_LOG, "acacia_log");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_LOG, "dark_oak_log");

    // 竹木原木
    registerSimpleBlock(VanillaBlocks::BAMBOO_BLOCK, "bamboo_block");
    registerSimpleBlock(VanillaBlocks::STRIPPED_BAMBOO_BLOCK, "stripped_bamboo_block");

    // 树叶
    registerSimpleBlock(VanillaBlocks::OAK_LEAVES, "oak_leaves");
    registerSimpleBlock(VanillaBlocks::SPRUCE_LEAVES, "spruce_leaves");
    registerSimpleBlock(VanillaBlocks::BIRCH_LEAVES, "birch_leaves");
    registerSimpleBlock(VanillaBlocks::JUNGLE_LEAVES, "jungle_leaves");
    registerSimpleBlock(VanillaBlocks::ACACIA_LEAVES, "acacia_leaves");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_LEAVES, "dark_oak_leaves");

    // 羊毛
    registerSimpleBlock(VanillaBlocks::WHITE_WOOL, "white_wool");
    registerSimpleBlock(VanillaBlocks::ORANGE_WOOL, "orange_wool");
    registerSimpleBlock(VanillaBlocks::MAGENTA_WOOL, "magenta_wool");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_WOOL, "light_blue_wool");
    registerSimpleBlock(VanillaBlocks::YELLOW_WOOL, "yellow_wool");
    registerSimpleBlock(VanillaBlocks::LIME_WOOL, "lime_wool");
    registerSimpleBlock(VanillaBlocks::PINK_WOOL, "pink_wool");
    registerSimpleBlock(VanillaBlocks::GRAY_WOOL, "gray_wool");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_WOOL, "light_gray_wool");
    registerSimpleBlock(VanillaBlocks::CYAN_WOOL, "cyan_wool");
    registerSimpleBlock(VanillaBlocks::PURPLE_WOOL, "purple_wool");
    registerSimpleBlock(VanillaBlocks::BLUE_WOOL, "blue_wool");
    registerSimpleBlock(VanillaBlocks::BROWN_WOOL, "brown_wool");
    registerSimpleBlock(VanillaBlocks::GREEN_WOOL, "green_wool");
    registerSimpleBlock(VanillaBlocks::RED_WOOL, "red_wool");
    registerSimpleBlock(VanillaBlocks::BLACK_WOOL, "black_wool");

    // 地毯 (16色)
    registerSimpleBlock(VanillaBlocks::WHITE_CARPET, "white_carpet");
    registerSimpleBlock(VanillaBlocks::ORANGE_CARPET, "orange_carpet");
    registerSimpleBlock(VanillaBlocks::MAGENTA_CARPET, "magenta_carpet");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_CARPET, "light_blue_carpet");
    registerSimpleBlock(VanillaBlocks::YELLOW_CARPET, "yellow_carpet");
    registerSimpleBlock(VanillaBlocks::LIME_CARPET, "lime_carpet");
    registerSimpleBlock(VanillaBlocks::PINK_CARPET, "pink_carpet");
    registerSimpleBlock(VanillaBlocks::GRAY_CARPET, "gray_carpet");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_CARPET, "light_gray_carpet");
    registerSimpleBlock(VanillaBlocks::CYAN_CARPET, "cyan_carpet");
    registerSimpleBlock(VanillaBlocks::PURPLE_CARPET, "purple_carpet");
    registerSimpleBlock(VanillaBlocks::BLUE_CARPET, "blue_carpet");
    registerSimpleBlock(VanillaBlocks::BROWN_CARPET, "brown_carpet");
    registerSimpleBlock(VanillaBlocks::GREEN_CARPET, "green_carpet");
    registerSimpleBlock(VanillaBlocks::RED_CARPET, "red_carpet");
    registerSimpleBlock(VanillaBlocks::BLACK_CARPET, "black_carpet");

    // 其他
    registerSimpleBlock(VanillaBlocks::SNOW, "snow");
    registerSimpleBlock(VanillaBlocks::ICE, "ice");
    registerSimpleBlock(VanillaBlocks::GLOWSTONE, "glowstone");
    registerSimpleBlock(VanillaBlocks::NETHERRACK, "netherrack");
    registerSimpleBlock(VanillaBlocks::END_STONE, "end_stone");

    // 功能方块
    registerSimpleBlock(VanillaBlocks::CRAFTING_TABLE, "crafting_table");
    registerSimpleBlock(VanillaBlocks::FURNACE, "furnace");
    registerSimpleBlock(VanillaBlocks::BLAST_FURNACE, "blast_furnace");
    registerSimpleBlock(VanillaBlocks::SMOKER, "smoker");
    registerSimpleBlock(VanillaBlocks::BARREL, "barrel");
    registerSimpleBlock(VanillaBlocks::GRINDSTONE, "grindstone");
    registerSimpleBlock(VanillaBlocks::CARTOGRAPHY_TABLE, "cartography_table");
    registerSimpleBlock(VanillaBlocks::FLETCHING_TABLE, "fletching_table");
    registerSimpleBlock(VanillaBlocks::SMITHING_TABLE, "smithing_table");
    registerSimpleBlock(VanillaBlocks::COMPOSTER, "composter");
    registerSimpleBlock(VanillaBlocks::CAKE, "cake");
    registerSimpleBlock(VanillaBlocks::LECTERN, "lectern");
    registerSimpleBlock(VanillaBlocks::LOOM, "loom");
    registerSimpleBlock(VanillaBlocks::JUKEBOX, "jukebox");
    registerSimpleBlock(VanillaBlocks::CHEST, "chest");
    registerSimpleBlock(VanillaBlocks::TRAPPED_CHEST, "trapped_chest");

    // 铁砧（最大堆叠数为1）
    {
        auto registerAnvilBlock = [this](Block* block, const std::string& name) {
            if (block == nullptr) {
                spdlog::warn("Block '{}' is null, skipping", name);
                return;
            }
            const ResourceLocation& blockLoc = block->blockLocation();
            ResourceLocation itemLoc(blockLoc.namespace_(), blockLoc.path());
            BlockItem* registeredItem = nullptr;
            Item* existingItem = ItemRegistry::instance().getItem(itemLoc);
            if (existingItem != nullptr) {
                registeredItem = dynamic_cast<BlockItem*>(existingItem);
                if (registeredItem == nullptr) {
                    spdlog::warn("Item '{}' already exists but is not a BlockItem, skipping", itemLoc.toString());
                    return;
                }
            } else {
                registeredItem = &ItemRegistry::instance().registerItem<BlockItem>(
                    itemLoc, *block, ItemProperties().maxStackSize(1));
            }
            u32 blockId = block->blockId();
            ItemId itemId = registeredItem->itemId();
            m_blockToItem[blockId] = registeredItem;
            m_itemToBlock[itemId] = block;
            m_itemIdToBlockItem[itemId] = registeredItem;
        };
        registerAnvilBlock(VanillaBlocks::ANVIL, "anvil");
        registerAnvilBlock(VanillaBlocks::CHIPPED_ANVIL, "chipped_anvil");
        registerAnvilBlock(VanillaBlocks::DAMAGED_ANVIL, "damaged_anvil");
    }

    // 石砖系列
    registerSimpleBlock(VanillaBlocks::STONE_BRICKS, "stone_bricks");
    registerSimpleBlock(VanillaBlocks::MOSSY_STONE_BRICKS, "mossy_stone_bricks");
    registerSimpleBlock(VanillaBlocks::CRACKED_STONE_BRICKS, "cracked_stone_bricks");
    registerSimpleBlock(VanillaBlocks::CHISELED_STONE_BRICKS, "chiseled_stone_bricks");

    // 虫蚀方块
    registerSimpleBlock(VanillaBlocks::INFESTED_STONE, "infested_stone");
    registerSimpleBlock(VanillaBlocks::INFESTED_COBBLESTONE, "infested_cobblestone");
    registerSimpleBlock(VanillaBlocks::INFESTED_STONE_BRICKS, "infested_stone_bricks");
    registerSimpleBlock(VanillaBlocks::INFESTED_MOSSY_STONE_BRICKS, "infested_mossy_stone_bricks");
    registerSimpleBlock(VanillaBlocks::INFESTED_CRACKED_STONE_BRICKS, "infested_cracked_stone_bricks");
    registerSimpleBlock(VanillaBlocks::INFESTED_CHISELED_STONE_BRICKS, "infested_chiseled_stone_bricks");

    // 石英系列
    registerSimpleBlock(VanillaBlocks::QUARTZ_BLOCK, "quartz_block");
    registerSimpleBlock(VanillaBlocks::SMOOTH_QUARTZ, "smooth_quartz");
    registerSimpleBlock(VanillaBlocks::CHISELED_QUARTZ_BLOCK, "chiseled_quartz_block");
    registerSimpleBlock(VanillaBlocks::QUARTZ_PILLAR, "quartz_pillar");

    // 海晶系列
    registerSimpleBlock(VanillaBlocks::PRISMARINE, "prismarine");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_BRICKS, "prismarine_bricks");
    registerSimpleBlock(VanillaBlocks::DARK_PRISMARINE, "dark_prismarine");
    registerSimpleBlock(VanillaBlocks::SEA_LANTERN, "sea_lantern");

    // 紫珀系列
    registerSimpleBlock(VanillaBlocks::PURPUR_BLOCK, "purpur_block");
    registerSimpleBlock(VanillaBlocks::PURPUR_PILLAR, "purpur_pillar");

    // 末地系列
    registerSimpleBlock(VanillaBlocks::END_STONE_BRICKS, "end_stone_bricks");

    // 骨块与干草块
    registerSimpleBlock(VanillaBlocks::BONE_BLOCK, "bone_block");
    registerSimpleBlock(VanillaBlocks::HAY_BLOCK, "hay_block");

    // 下界方块
    registerSimpleBlock(VanillaBlocks::NETHER_BRICKS, "nether_bricks");
    registerSimpleBlock(VanillaBlocks::RED_NETHER_BRICKS, "red_nether_bricks");
    registerSimpleBlock(VanillaBlocks::SOUL_SAND, "soul_sand");
    registerSimpleBlock(VanillaBlocks::SOUL_SOIL, "soul_soil");
    registerSimpleBlock(VanillaBlocks::BASALT, "basalt");
    registerSimpleBlock(VanillaBlocks::POLISHED_BASALT, "polished_basalt");
    registerSimpleBlock(VanillaBlocks::BLACKSTONE, "blackstone");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE, "polished_blackstone");
    registerSimpleBlock(VanillaBlocks::GILDED_BLACKSTONE, "gilded_blackstone");
    // 黑石衍生方块（楼梯/台阶/墙）
    registerSimpleBlock(VanillaBlocks::BLACKSTONE_STAIRS, "blackstone_stairs");
    registerSimpleBlock(VanillaBlocks::BLACKSTONE_SLAB, "blackstone_slab");
    registerSimpleBlock(VanillaBlocks::BLACKSTONE_WALL, "blackstone_wall");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE_STAIRS, "polished_blackstone_stairs");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE_SLAB, "polished_blackstone_slab");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE_WALL, "polished_blackstone_wall");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS, "polished_blackstone_brick_stairs");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_SLAB, "polished_blackstone_brick_slab");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_WALL, "polished_blackstone_brick_wall");
    registerSimpleBlock(VanillaBlocks::CRYING_OBSIDIAN, "crying_obsidian");
    registerSimpleBlock(VanillaBlocks::MAGMA, "magma_block");
    registerSimpleBlock(VanillaBlocks::NETHER_WART_BLOCK, "nether_wart_block");
    registerSimpleBlock(VanillaBlocks::CRIMSON_STEM, "crimson_stem");
    registerSimpleBlock(VanillaBlocks::WARPED_STEM, "warped_stem");
    registerSimpleBlock(VanillaBlocks::CRIMSON_HYPHAE, "crimson_hyphae");
    registerSimpleBlock(VanillaBlocks::WARPED_HYPHAE, "warped_hyphae");
    registerSimpleBlock(VanillaBlocks::STRIPPED_CRIMSON_STEM, "stripped_crimson_stem");
    registerSimpleBlock(VanillaBlocks::STRIPPED_WARPED_STEM, "stripped_warped_stem");
    registerSimpleBlock(VanillaBlocks::STRIPPED_CRIMSON_HYPHAE, "stripped_crimson_hyphae");
    registerSimpleBlock(VanillaBlocks::STRIPPED_WARPED_HYPHAE, "stripped_warped_hyphae");
    registerSimpleBlock(VanillaBlocks::CRIMSON_NYLIUM, "crimson_nylium");
    registerSimpleBlock(VanillaBlocks::WARPED_NYLIUM, "warped_nylium");
    registerSimpleBlock(VanillaBlocks::SHROOMLIGHT, "shroomlight");

    // 绯红/诡异木板及衍生方块
    registerSimpleBlock(VanillaBlocks::CRIMSON_PLANKS, "crimson_planks");
    registerSimpleBlock(VanillaBlocks::WARPED_PLANKS, "warped_planks");
    registerSimpleBlock(VanillaBlocks::CRIMSON_STAIRS, "crimson_stairs");
    registerSimpleBlock(VanillaBlocks::WARPED_STAIRS, "warped_stairs");
    registerSimpleBlock(VanillaBlocks::CRIMSON_SLAB, "crimson_slab");
    registerSimpleBlock(VanillaBlocks::WARPED_SLAB, "warped_slab");
    registerSimpleBlock(VanillaBlocks::CRIMSON_FENCE, "crimson_fence");
    registerSimpleBlock(VanillaBlocks::WARPED_FENCE, "warped_fence");

    // 深板岩方块 (1.17+)
    registerSimpleBlock(VanillaBlocks::DEEPSLATE, "deepslate");
    registerSimpleBlock(VanillaBlocks::COBBLED_DEEPSLATE, "cobbled_deepslate");
    registerSimpleBlock(VanillaBlocks::POLISHED_DEEPSLATE, "polished_deepslate");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_BRICKS, "deepslate_bricks");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_TILES, "deepslate_tiles");
    registerSimpleBlock(VanillaBlocks::CHISELED_DEEPSLATE, "chiseled_deepslate");
    registerSimpleBlock(VanillaBlocks::CRACKED_DEEPSLATE_BRICKS, "cracked_deepslate_bricks");
    registerSimpleBlock(VanillaBlocks::CRACKED_DEEPSLATE_TILES, "cracked_deepslate_tiles");
    registerSimpleBlock(VanillaBlocks::REINFORCED_DEEPSLATE, "reinforced_deepslate");
    registerSimpleBlock(VanillaBlocks::COBBLED_DEEPSLATE_STAIRS, "cobbled_deepslate_stairs");
    registerSimpleBlock(VanillaBlocks::COBBLED_DEEPSLATE_SLAB, "cobbled_deepslate_slab");
    registerSimpleBlock(VanillaBlocks::COBBLED_DEEPSLATE_WALL, "cobbled_deepslate_wall");
    registerSimpleBlock(VanillaBlocks::POLISHED_DEEPSLATE_STAIRS, "polished_deepslate_stairs");
    registerSimpleBlock(VanillaBlocks::POLISHED_DEEPSLATE_SLAB, "polished_deepslate_slab");
    registerSimpleBlock(VanillaBlocks::POLISHED_DEEPSLATE_WALL, "polished_deepslate_wall");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_BRICK_STAIRS, "deepslate_brick_stairs");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_BRICK_SLAB, "deepslate_brick_slab");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_BRICK_WALL, "deepslate_brick_wall");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_TILE_STAIRS, "deepslate_tile_stairs");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_TILE_SLAB, "deepslate_tile_slab");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_TILE_WALL, "deepslate_tile_wall");
    registerSimpleBlock(VanillaBlocks::INFESTED_DEEPSLATE, "infested_deepslate");
    registerSimpleBlock(VanillaBlocks::SMOOTH_BASALT, "smooth_basalt");

    // 凝灰岩方块 (1.17+)
    registerSimpleBlock(VanillaBlocks::TUFF, "tuff");
    registerSimpleBlock(VanillaBlocks::POLISHED_TUFF, "polished_tuff");
    registerSimpleBlock(VanillaBlocks::TUFF_BRICKS, "tuff_bricks");
    registerSimpleBlock(VanillaBlocks::CHISELED_TUFF, "chiseled_tuff");
    registerSimpleBlock(VanillaBlocks::CHISELED_TUFF_BRICKS, "chiseled_tuff_bricks");
    registerSimpleBlock(VanillaBlocks::TUFF_STAIRS, "tuff_stairs");
    registerSimpleBlock(VanillaBlocks::TUFF_SLAB, "tuff_slab");
    registerSimpleBlock(VanillaBlocks::TUFF_WALL, "tuff_wall");
    registerSimpleBlock(VanillaBlocks::POLISHED_TUFF_STAIRS, "polished_tuff_stairs");
    registerSimpleBlock(VanillaBlocks::POLISHED_TUFF_SLAB, "polished_tuff_slab");
    registerSimpleBlock(VanillaBlocks::POLISHED_TUFF_WALL, "polished_tuff_wall");
    registerSimpleBlock(VanillaBlocks::TUFF_BRICK_STAIRS, "tuff_brick_stairs");
    registerSimpleBlock(VanillaBlocks::TUFF_BRICK_SLAB, "tuff_brick_slab");
    registerSimpleBlock(VanillaBlocks::TUFF_BRICK_WALL, "tuff_brick_wall");

    // 树脂方块 (1.21+)
    registerSimpleBlock(VanillaBlocks::RESIN_CLUMP, "resin_clump");
    registerSimpleBlock(VanillaBlocks::RESIN_BLOCK, "resin_block");
    registerSimpleBlock(VanillaBlocks::RESIN_BRICKS, "resin_bricks");
    registerSimpleBlock(VanillaBlocks::CHISELED_RESIN_BRICKS, "chiseled_resin_bricks");
    registerSimpleBlock(VanillaBlocks::RESIN_BRICK_STAIRS, "resin_brick_stairs");
    registerSimpleBlock(VanillaBlocks::RESIN_BRICK_SLAB, "resin_brick_slab");
    registerSimpleBlock(VanillaBlocks::RESIN_BRICK_WALL, "resin_brick_wall");

    // 樱花原木与木材 (1.20+)
    registerSimpleBlock(VanillaBlocks::CHERRY_LOG, "cherry_log");
    registerSimpleBlock(VanillaBlocks::CHERRY_WOOD, "cherry_wood");
    registerSimpleBlock(VanillaBlocks::STRIPPED_CHERRY_LOG, "stripped_cherry_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_CHERRY_WOOD, "stripped_cherry_wood");
    registerSimpleBlock(VanillaBlocks::CHERRY_PLANKS, "cherry_planks");
    registerSimpleBlock(VanillaBlocks::CHERRY_LEAVES, "cherry_leaves");
    registerSimpleBlock(VanillaBlocks::CHERRY_SAPLING, "cherry_sapling");

    // 苍白橡木原木与木材 (1.21+)
    registerSimpleBlock(VanillaBlocks::PALE_OAK_LOG, "pale_oak_log");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_WOOD, "pale_oak_wood");
    registerSimpleBlock(VanillaBlocks::STRIPPED_PALE_OAK_LOG, "stripped_pale_oak_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_PALE_OAK_WOOD, "stripped_pale_oak_wood");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_PLANKS, "pale_oak_planks");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_LEAVES, "pale_oak_leaves");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_SAPLING, "pale_oak_sapling");

    // 苍白花园自然方块 (1.21+)
    registerSimpleBlock(VanillaBlocks::PALE_MOSS_BLOCK, "pale_moss_block");
    registerSimpleBlock(VanillaBlocks::PALE_MOSS_CARPET, "pale_moss_carpet");
    registerSimpleBlock(VanillaBlocks::PALE_HANGING_MOSS, "pale_hanging_moss");
    registerSimpleBlock(VanillaBlocks::OPEN_EYEBLOSSOM, "open_eyeblossom");
    registerSimpleBlock(VanillaBlocks::CLOSED_EYEBLOSSOM, "closed_eyeblossom");
    registerSimpleBlock(VanillaBlocks::CREAKING_HEART, "creaking_heart");

    // 洞穴方块 (1.17+)
    registerSimpleBlock(VanillaBlocks::MOSS_BLOCK, "moss_block");
    registerSimpleBlock(VanillaBlocks::MOSS_CARPET, "moss_carpet");
    registerSimpleBlock(VanillaBlocks::ROOTED_DIRT, "rooted_dirt");
    registerSimpleBlock(VanillaBlocks::HANGING_ROOTS, "hanging_roots");
    registerSimpleBlock(VanillaBlocks::AZALEA, "azalea");
    registerSimpleBlock(VanillaBlocks::FLOWERING_AZALEA, "flowering_azalea");
    registerSimpleBlock(VanillaBlocks::AZALEA_LEAVES, "azalea_leaves");
    registerSimpleBlock(VanillaBlocks::FLOWERING_AZALEA_LEAVES, "flowering_azalea_leaves");
    registerSimpleBlock(VanillaBlocks::SPORE_BLOSSOM, "spore_blossom");

    // 泥巴系列方块 (1.19+)
    registerSimpleBlock(VanillaBlocks::MUD, "mud");
    registerSimpleBlock(VanillaBlocks::PACKED_MUD, "packed_mud");
    registerSimpleBlock(VanillaBlocks::MUD_BRICKS, "mud_bricks");
    registerSimpleBlock(VanillaBlocks::MUD_BRICK_STAIRS, "mud_brick_stairs");
    registerSimpleBlock(VanillaBlocks::MUD_BRICK_SLAB, "mud_brick_slab");
    registerSimpleBlock(VanillaBlocks::MUD_BRICK_WALL, "mud_brick_wall");

    // 红树林基础方块 (1.19+)
    registerSimpleBlock(VanillaBlocks::MANGROVE_LOG, "mangrove_log");
    registerSimpleBlock(VanillaBlocks::MANGROVE_WOOD, "mangrove_wood");
    registerSimpleBlock(VanillaBlocks::STRIPPED_MANGROVE_LOG, "stripped_mangrove_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_MANGROVE_WOOD, "stripped_mangrove_wood");
    registerSimpleBlock(VanillaBlocks::MANGROVE_PLANKS, "mangrove_planks");
    registerSimpleBlock(VanillaBlocks::MANGROVE_LEAVES, "mangrove_leaves");
    registerSimpleBlock(VanillaBlocks::MANGROVE_PROPAGULE, "mangrove_propagule");
    registerSimpleBlock(VanillaBlocks::MANGROVE_ROOTS, "mangrove_roots");
    registerSimpleBlock(VanillaBlocks::MUDDY_MANGROVE_ROOTS, "muddy_mangrove_roots");

    // 自然方块扩展
    registerSimpleBlock(VanillaBlocks::CLAY, "clay");
    registerSimpleBlock(VanillaBlocks::MYCELIUM, "mycelium");
    registerSimpleBlock(VanillaBlocks::GRASS_PATH, "dirt_path");
    registerSimpleBlock(VanillaBlocks::PACKED_ICE, "packed_ice");
    registerSimpleBlock(VanillaBlocks::BLUE_ICE, "blue_ice");
    registerSimpleBlock(VanillaBlocks::SLIME_BLOCK, "slime_block");
    registerSimpleBlock(VanillaBlocks::HONEY_BLOCK, "honey_block");
    registerSimpleBlock(VanillaBlocks::CACTUS, "cactus");
    registerSimpleBlock(VanillaBlocks::LILY_PAD, "lily_pad");
    registerSimpleBlock(VanillaBlocks::VINE, "vine");
    registerSimpleBlock(VanillaBlocks::COBWEB, "cobweb");
    registerSimpleBlock(VanillaBlocks::SUGAR_CANE, "sugar_cane");
    registerSimpleBlock(VanillaBlocks::RED_SAND, "red_sand");
    registerSimpleBlock(VanillaBlocks::DRIED_KELP_BLOCK, "dried_kelp_block");
    registerSimpleBlock(VanillaBlocks::SEA_PICKLE, "sea_pickle");

    // 珊瑚方块 - 活
    registerSimpleBlock(VanillaBlocks::TUBE_CORAL_BLOCK, "tube_coral_block");
    registerSimpleBlock(VanillaBlocks::BRAIN_CORAL_BLOCK, "brain_coral_block");
    registerSimpleBlock(VanillaBlocks::BUBBLE_CORAL_BLOCK, "bubble_coral_block");
    registerSimpleBlock(VanillaBlocks::FIRE_CORAL_BLOCK, "fire_coral_block");
    registerSimpleBlock(VanillaBlocks::HORN_CORAL_BLOCK, "horn_coral_block");

    // 珊瑚方块 - 死
    registerSimpleBlock(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK, "dead_tube_coral_block");
    registerSimpleBlock(VanillaBlocks::DEAD_BRAIN_CORAL_BLOCK, "dead_brain_coral_block");
    registerSimpleBlock(VanillaBlocks::DEAD_BUBBLE_CORAL_BLOCK, "dead_bubble_coral_block");
    registerSimpleBlock(VanillaBlocks::DEAD_FIRE_CORAL_BLOCK, "dead_fire_coral_block");
    registerSimpleBlock(VanillaBlocks::DEAD_HORN_CORAL_BLOCK, "dead_horn_coral_block");

    // 珊瑚扇 - 活
    registerSimpleBlock(VanillaBlocks::TUBE_CORAL_FAN, "tube_coral_fan");
    registerSimpleBlock(VanillaBlocks::BRAIN_CORAL_FAN, "brain_coral_fan");
    registerSimpleBlock(VanillaBlocks::BUBBLE_CORAL_FAN, "bubble_coral_fan");
    registerSimpleBlock(VanillaBlocks::FIRE_CORAL_FAN, "fire_coral_fan");
    registerSimpleBlock(VanillaBlocks::HORN_CORAL_FAN, "horn_coral_fan");

    // 珊瑚扇 - 死
    registerSimpleBlock(VanillaBlocks::DEAD_TUBE_CORAL_FAN, "dead_tube_coral_fan");
    registerSimpleBlock(VanillaBlocks::DEAD_BRAIN_CORAL_FAN, "dead_brain_coral_fan");
    registerSimpleBlock(VanillaBlocks::DEAD_BUBBLE_CORAL_FAN, "dead_bubble_coral_fan");
    registerSimpleBlock(VanillaBlocks::DEAD_FIRE_CORAL_FAN, "dead_fire_coral_fan");
    registerSimpleBlock(VanillaBlocks::DEAD_HORN_CORAL_FAN, "dead_horn_coral_fan");
    registerSimpleBlock(VanillaBlocks::SEAGRASS, "seagrass");
    // tall_seagrass 在 vanilla 中没有独立物品（由骨粉作用于海草生成），不注册 BlockItem。
    registerSimpleBlock(VanillaBlocks::BAMBOO, "bamboo");

    // 下界矿石
    registerSimpleBlock(VanillaBlocks::NETHER_QUARTZ_ORE, "nether_quartz_ore");
    registerSimpleBlock(VanillaBlocks::NETHER_GOLD_ORE, "nether_gold_ore");
    registerSimpleBlock(VanillaBlocks::ANCIENT_DEBRIS, "ancient_debris");

    // 铜矿 (1.17+)
    registerSimpleBlock(VanillaBlocks::COPPER_ORE, "copper_ore");

    // 深板岩矿石 (1.17+)
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_COAL_ORE, "deepslate_coal_ore");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_IRON_ORE, "deepslate_iron_ore");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_COPPER_ORE, "deepslate_copper_ore");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_GOLD_ORE, "deepslate_gold_ore");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_DIAMOND_ORE, "deepslate_diamond_ore");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_LAPIS_ORE, "deepslate_lapis_ore");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_EMERALD_ORE, "deepslate_emerald_ore");
    registerSimpleBlock(VanillaBlocks::DEEPSLATE_REDSTONE_ORE, "deepslate_redstone_ore");

    // 玻璃
    registerSimpleBlock(VanillaBlocks::GLASS, "glass");

    // 染色玻璃 (16色)
    registerSimpleBlock(VanillaBlocks::WHITE_STAINED_GLASS, "white_stained_glass");
    registerSimpleBlock(VanillaBlocks::ORANGE_STAINED_GLASS, "orange_stained_glass");
    registerSimpleBlock(VanillaBlocks::MAGENTA_STAINED_GLASS, "magenta_stained_glass");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_STAINED_GLASS, "light_blue_stained_glass");
    registerSimpleBlock(VanillaBlocks::YELLOW_STAINED_GLASS, "yellow_stained_glass");
    registerSimpleBlock(VanillaBlocks::LIME_STAINED_GLASS, "lime_stained_glass");
    registerSimpleBlock(VanillaBlocks::PINK_STAINED_GLASS, "pink_stained_glass");
    registerSimpleBlock(VanillaBlocks::GRAY_STAINED_GLASS, "gray_stained_glass");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_STAINED_GLASS, "light_gray_stained_glass");
    registerSimpleBlock(VanillaBlocks::CYAN_STAINED_GLASS, "cyan_stained_glass");
    registerSimpleBlock(VanillaBlocks::PURPLE_STAINED_GLASS, "purple_stained_glass");
    registerSimpleBlock(VanillaBlocks::BLUE_STAINED_GLASS, "blue_stained_glass");
    registerSimpleBlock(VanillaBlocks::BROWN_STAINED_GLASS, "brown_stained_glass");
    registerSimpleBlock(VanillaBlocks::GREEN_STAINED_GLASS, "green_stained_glass");
    registerSimpleBlock(VanillaBlocks::RED_STAINED_GLASS, "red_stained_glass");
    registerSimpleBlock(VanillaBlocks::BLACK_STAINED_GLASS, "black_stained_glass");

    // 染色玻璃板
    registerSimpleBlock(VanillaBlocks::GLASS_PANE, "glass_pane");

    // 混凝土 (16色)
    registerSimpleBlock(VanillaBlocks::WHITE_CONCRETE, "white_concrete");
    registerSimpleBlock(VanillaBlocks::ORANGE_CONCRETE, "orange_concrete");
    registerSimpleBlock(VanillaBlocks::MAGENTA_CONCRETE, "magenta_concrete");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_CONCRETE, "light_blue_concrete");
    registerSimpleBlock(VanillaBlocks::YELLOW_CONCRETE, "yellow_concrete");
    registerSimpleBlock(VanillaBlocks::LIME_CONCRETE, "lime_concrete");
    registerSimpleBlock(VanillaBlocks::PINK_CONCRETE, "pink_concrete");
    registerSimpleBlock(VanillaBlocks::GRAY_CONCRETE, "gray_concrete");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_CONCRETE, "light_gray_concrete");
    registerSimpleBlock(VanillaBlocks::CYAN_CONCRETE, "cyan_concrete");
    registerSimpleBlock(VanillaBlocks::PURPLE_CONCRETE, "purple_concrete");
    registerSimpleBlock(VanillaBlocks::BLUE_CONCRETE, "blue_concrete");
    registerSimpleBlock(VanillaBlocks::BROWN_CONCRETE, "brown_concrete");
    registerSimpleBlock(VanillaBlocks::GREEN_CONCRETE, "green_concrete");
    registerSimpleBlock(VanillaBlocks::RED_CONCRETE, "red_concrete");
    registerSimpleBlock(VanillaBlocks::BLACK_CONCRETE, "black_concrete");

    // 混凝土粉末 (16色)
    registerSimpleBlock(VanillaBlocks::WHITE_CONCRETE_POWDER, "white_concrete_powder");
    registerSimpleBlock(VanillaBlocks::ORANGE_CONCRETE_POWDER, "orange_concrete_powder");
    registerSimpleBlock(VanillaBlocks::MAGENTA_CONCRETE_POWDER, "magenta_concrete_powder");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_CONCRETE_POWDER, "light_blue_concrete_powder");
    registerSimpleBlock(VanillaBlocks::YELLOW_CONCRETE_POWDER, "yellow_concrete_powder");
    registerSimpleBlock(VanillaBlocks::LIME_CONCRETE_POWDER, "lime_concrete_powder");
    registerSimpleBlock(VanillaBlocks::PINK_CONCRETE_POWDER, "pink_concrete_powder");
    registerSimpleBlock(VanillaBlocks::GRAY_CONCRETE_POWDER, "gray_concrete_powder");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_CONCRETE_POWDER, "light_gray_concrete_powder");
    registerSimpleBlock(VanillaBlocks::CYAN_CONCRETE_POWDER, "cyan_concrete_powder");
    registerSimpleBlock(VanillaBlocks::PURPLE_CONCRETE_POWDER, "purple_concrete_powder");
    registerSimpleBlock(VanillaBlocks::BLUE_CONCRETE_POWDER, "blue_concrete_powder");
    registerSimpleBlock(VanillaBlocks::BROWN_CONCRETE_POWDER, "brown_concrete_powder");
    registerSimpleBlock(VanillaBlocks::GREEN_CONCRETE_POWDER, "green_concrete_powder");
    registerSimpleBlock(VanillaBlocks::RED_CONCRETE_POWDER, "red_concrete_powder");
    registerSimpleBlock(VanillaBlocks::BLACK_CONCRETE_POWDER, "black_concrete_powder");

    // 陶瓦 (16色+普通)
    registerSimpleBlock(VanillaBlocks::TERRACOTTA, "terracotta");
    registerSimpleBlock(VanillaBlocks::WHITE_TERRACOTTA, "white_terracotta");
    registerSimpleBlock(VanillaBlocks::ORANGE_TERRACOTTA, "orange_terracotta");
    registerSimpleBlock(VanillaBlocks::MAGENTA_TERRACOTTA, "magenta_terracotta");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_TERRACOTTA, "light_blue_terracotta");
    registerSimpleBlock(VanillaBlocks::YELLOW_TERRACOTTA, "yellow_terracotta");
    registerSimpleBlock(VanillaBlocks::LIME_TERRACOTTA, "lime_terracotta");
    registerSimpleBlock(VanillaBlocks::PINK_TERRACOTTA, "pink_terracotta");
    registerSimpleBlock(VanillaBlocks::GRAY_TERRACOTTA, "gray_terracotta");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_TERRACOTTA, "light_gray_terracotta");
    registerSimpleBlock(VanillaBlocks::CYAN_TERRACOTTA, "cyan_terracotta");
    registerSimpleBlock(VanillaBlocks::PURPLE_TERRACOTTA, "purple_terracotta");
    registerSimpleBlock(VanillaBlocks::BLUE_TERRACOTTA, "blue_terracotta");
    registerSimpleBlock(VanillaBlocks::BROWN_TERRACOTTA, "brown_terracotta");
    registerSimpleBlock(VanillaBlocks::GREEN_TERRACOTTA, "green_terracotta");
    registerSimpleBlock(VanillaBlocks::RED_TERRACOTTA, "red_terracotta");
    registerSimpleBlock(VanillaBlocks::BLACK_TERRACOTTA, "black_terracotta");

    // 釉面陶瓦 (16色，可旋转、不可被活塞拉动)
    registerSimpleBlock(VanillaBlocks::WHITE_GLAZED_TERRACOTTA, "white_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::ORANGE_GLAZED_TERRACOTTA, "orange_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::MAGENTA_GLAZED_TERRACOTTA, "magenta_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_GLAZED_TERRACOTTA, "light_blue_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::YELLOW_GLAZED_TERRACOTTA, "yellow_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::LIME_GLAZED_TERRACOTTA, "lime_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::PINK_GLAZED_TERRACOTTA, "pink_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::GRAY_GLAZED_TERRACOTTA, "gray_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_GLAZED_TERRACOTTA, "light_gray_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::CYAN_GLAZED_TERRACOTTA, "cyan_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::PURPLE_GLAZED_TERRACOTTA, "purple_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::BLUE_GLAZED_TERRACOTTA, "blue_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::BROWN_GLAZED_TERRACOTTA, "brown_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::GREEN_GLAZED_TERRACOTTA, "green_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::RED_GLAZED_TERRACOTTA, "red_glazed_terracotta");
    registerSimpleBlock(VanillaBlocks::BLACK_GLAZED_TERRACOTTA, "black_glazed_terracotta");

    // 去皮原木
    registerSimpleBlock(VanillaBlocks::STRIPPED_OAK_LOG, "stripped_oak_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_SPRUCE_LOG, "stripped_spruce_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_BIRCH_LOG, "stripped_birch_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_JUNGLE_LOG, "stripped_jungle_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_ACACIA_LOG, "stripped_acacia_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_DARK_OAK_LOG, "stripped_dark_oak_log");
    registerSimpleBlock(VanillaBlocks::STRIPPED_OAK_WOOD, "stripped_oak_wood");
    registerSimpleBlock(VanillaBlocks::STRIPPED_SPRUCE_WOOD, "stripped_spruce_wood");
    registerSimpleBlock(VanillaBlocks::STRIPPED_BIRCH_WOOD, "stripped_birch_wood");
    registerSimpleBlock(VanillaBlocks::STRIPPED_JUNGLE_WOOD, "stripped_jungle_wood");
    registerSimpleBlock(VanillaBlocks::STRIPPED_ACACIA_WOOD, "stripped_acacia_wood");
    registerSimpleBlock(VanillaBlocks::STRIPPED_DARK_OAK_WOOD, "stripped_dark_oak_wood");

    // 木头（六面树皮）
    registerSimpleBlock(VanillaBlocks::OAK_WOOD, "oak_wood");
    registerSimpleBlock(VanillaBlocks::SPRUCE_WOOD, "spruce_wood");
    registerSimpleBlock(VanillaBlocks::BIRCH_WOOD, "birch_wood");
    registerSimpleBlock(VanillaBlocks::JUNGLE_WOOD, "jungle_wood");
    registerSimpleBlock(VanillaBlocks::ACACIA_WOOD, "acacia_wood");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_WOOD, "dark_oak_wood");

    // 树苗
    registerSimpleBlock(VanillaBlocks::OAK_SAPLING, "oak_sapling");
    registerSimpleBlock(VanillaBlocks::SPRUCE_SAPLING, "spruce_sapling");
    registerSimpleBlock(VanillaBlocks::BIRCH_SAPLING, "birch_sapling");
    registerSimpleBlock(VanillaBlocks::JUNGLE_SAPLING, "jungle_sapling");
    registerSimpleBlock(VanillaBlocks::ACACIA_SAPLING, "acacia_sapling");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_SAPLING, "dark_oak_sapling");

    // 植被方块
    registerSimpleBlock(VanillaBlocks::SHORT_GRASS, "short_grass");
    registerSimpleBlock(VanillaBlocks::TALL_GRASS, "tall_grass");
    registerSimpleBlock(VanillaBlocks::FERN, "fern");
    registerSimpleBlock(VanillaBlocks::LARGE_FERN, "large_fern");
    registerSimpleBlock(VanillaBlocks::DANDELION, "dandelion");
    registerSimpleBlock(VanillaBlocks::POPPY, "poppy");
    registerSimpleBlock(VanillaBlocks::BLUE_ORCHID, "blue_orchid");
    registerSimpleBlock(VanillaBlocks::ALLIUM, "allium");
    registerSimpleBlock(VanillaBlocks::AZURE_BLUET, "azure_bluet");
    registerSimpleBlock(VanillaBlocks::RED_TULIP, "red_tulip");
    registerSimpleBlock(VanillaBlocks::ORANGE_TULIP, "orange_tulip");
    registerSimpleBlock(VanillaBlocks::WHITE_TULIP, "white_tulip");
    registerSimpleBlock(VanillaBlocks::PINK_TULIP, "pink_tulip");
    registerSimpleBlock(VanillaBlocks::OXEYE_DAISY, "oxeye_daisy");
    registerSimpleBlock(VanillaBlocks::LILY_OF_THE_VALLEY, "lily_of_the_valley");
    registerSimpleBlock(VanillaBlocks::CORNFLOWER, "cornflower");
    registerSimpleBlock(VanillaBlocks::WITHER_ROSE, "wither_rose");
    registerSimpleBlock(VanillaBlocks::TORCHFLOWER, "torchflower");
    registerSimpleBlock(VanillaBlocks::PITCHER_PLANT, "pitcher_plant");
    registerSimpleBlock(VanillaBlocks::PINK_PETALS, "pink_petals");
    registerSimpleBlock(VanillaBlocks::CACTUS_FLOWER, "cactus_flower");
    registerSimpleBlock(VanillaBlocks::WILDFLOWERS, "wildflowers");
    registerSimpleBlock(VanillaBlocks::SUNFLOWER, "sunflower");
    registerSimpleBlock(VanillaBlocks::LILAC, "lilac");
    registerSimpleBlock(VanillaBlocks::ROSE_BUSH, "rose_bush");
    registerSimpleBlock(VanillaBlocks::PEONY, "peony");
    registerSimpleBlock(VanillaBlocks::BROWN_MUSHROOM, "brown_mushroom");
    registerSimpleBlock(VanillaBlocks::RED_MUSHROOM, "red_mushroom");
    registerSimpleBlock(VanillaBlocks::BROWN_MUSHROOM_BLOCK, "brown_mushroom_block");
    registerSimpleBlock(VanillaBlocks::RED_MUSHROOM_BLOCK, "red_mushroom_block");
    registerSimpleBlock(VanillaBlocks::MUSHROOM_STEM, "mushroom_stem");

    // 雪块
    registerSimpleBlock(VanillaBlocks::SNOW_BLOCK, "snow_block");

    // 其他方块
    registerSimpleBlock(VanillaBlocks::OBSIDIAN, "obsidian");
    registerSimpleBlock(VanillaBlocks::TNT, "tnt");
    registerSimpleBlock(VanillaBlocks::SPONGE, "sponge");
    registerSimpleBlock(VanillaBlocks::WET_SPONGE, "wet_sponge");
    registerSimpleBlock(VanillaBlocks::CAULDRON, "cauldron");
    registerSimpleBlock(VanillaBlocks::ENCHANTING_TABLE, "enchanting_table");
    registerSimpleBlock(VanillaBlocks::BEACON, "beacon");
    registerSimpleBlock(VanillaBlocks::BREWING_STAND, "brewing_stand");
    registerSimpleBlock(VanillaBlocks::ENDER_CHEST, "ender_chest");
    registerSimpleBlock(VanillaBlocks::SHULKER_BOX, "shulker_box");
    // 潜影盒 (16色)
    registerSimpleBlock(VanillaBlocks::WHITE_SHULKER_BOX, "white_shulker_box");
    registerSimpleBlock(VanillaBlocks::ORANGE_SHULKER_BOX, "orange_shulker_box");
    registerSimpleBlock(VanillaBlocks::MAGENTA_SHULKER_BOX, "magenta_shulker_box");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_SHULKER_BOX, "light_blue_shulker_box");
    registerSimpleBlock(VanillaBlocks::YELLOW_SHULKER_BOX, "yellow_shulker_box");
    registerSimpleBlock(VanillaBlocks::LIME_SHULKER_BOX, "lime_shulker_box");
    registerSimpleBlock(VanillaBlocks::PINK_SHULKER_BOX, "pink_shulker_box");
    registerSimpleBlock(VanillaBlocks::GRAY_SHULKER_BOX, "gray_shulker_box");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_SHULKER_BOX, "light_gray_shulker_box");
    registerSimpleBlock(VanillaBlocks::CYAN_SHULKER_BOX, "cyan_shulker_box");
    registerSimpleBlock(VanillaBlocks::PURPLE_SHULKER_BOX, "purple_shulker_box");
    registerSimpleBlock(VanillaBlocks::BLUE_SHULKER_BOX, "blue_shulker_box");
    registerSimpleBlock(VanillaBlocks::BROWN_SHULKER_BOX, "brown_shulker_box");
    registerSimpleBlock(VanillaBlocks::GREEN_SHULKER_BOX, "green_shulker_box");
    registerSimpleBlock(VanillaBlocks::RED_SHULKER_BOX, "red_shulker_box");
    registerSimpleBlock(VanillaBlocks::BLACK_SHULKER_BOX, "black_shulker_box");
    registerSimpleBlock(VanillaBlocks::LANTERN, "lantern");
    registerSimpleBlock(VanillaBlocks::SOUL_LANTERN, "soul_lantern");

    // 铜灯笼（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_LANTERN, "copper_lantern");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_LANTERN, "exposed_copper_lantern");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_LANTERN, "weathered_copper_lantern");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_LANTERN, "oxidized_copper_lantern");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_LANTERN, "waxed_copper_lantern");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_LANTERN, "waxed_exposed_copper_lantern");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_LANTERN, "waxed_weathered_copper_lantern");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_LANTERN, "waxed_oxidized_copper_lantern");
    registerSimpleBlock(VanillaBlocks::CAMPFIRE, "campfire");
    registerSimpleBlock(VanillaBlocks::SOUL_CAMPFIRE, "soul_campfire");
    registerSimpleBlock(VanillaBlocks::JACK_O_LANTERN, "jack_o_lantern");

    // 蜡烛
    registerSimpleBlock(VanillaBlocks::CANDLE, "candle");
    registerSimpleBlock(VanillaBlocks::WHITE_CANDLE, "white_candle");
    registerSimpleBlock(VanillaBlocks::ORANGE_CANDLE, "orange_candle");
    registerSimpleBlock(VanillaBlocks::MAGENTA_CANDLE, "magenta_candle");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_CANDLE, "light_blue_candle");
    registerSimpleBlock(VanillaBlocks::YELLOW_CANDLE, "yellow_candle");
    registerSimpleBlock(VanillaBlocks::LIME_CANDLE, "lime_candle");
    registerSimpleBlock(VanillaBlocks::PINK_CANDLE, "pink_candle");
    registerSimpleBlock(VanillaBlocks::GRAY_CANDLE, "gray_candle");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_CANDLE, "light_gray_candle");
    registerSimpleBlock(VanillaBlocks::CYAN_CANDLE, "cyan_candle");
    registerSimpleBlock(VanillaBlocks::PURPLE_CANDLE, "purple_candle");
    registerSimpleBlock(VanillaBlocks::BLUE_CANDLE, "blue_candle");
    registerSimpleBlock(VanillaBlocks::BROWN_CANDLE, "brown_candle");
    registerSimpleBlock(VanillaBlocks::GREEN_CANDLE, "green_candle");
    registerSimpleBlock(VanillaBlocks::RED_CANDLE, "red_candle");
    registerSimpleBlock(VanillaBlocks::BLACK_CANDLE, "black_candle");

    // 蜡烛蛋糕（candle_cake）在 vanilla 中没有独立物品：玩家通过在蛋糕上插蜡烛生成该方块，
    // 物品形式仍是 cake + candle，故不为其注册 BlockItem（否则 JavaItemIdMap 因物品名不存在而告警）。

    registerSimpleBlock(VanillaBlocks::TARGET, "target");
    registerSimpleBlock(VanillaBlocks::NOTE_BLOCK, "note_block");
    registerSimpleBlock(VanillaBlocks::DRAGON_EGG, "dragon_egg");
    registerSimpleBlock(VanillaBlocks::CONDUIT, "conduit");

    // 装饰/实用方块
    registerSimpleBlock(VanillaBlocks::CARVED_PUMPKIN, "carved_pumpkin");
    registerSimpleBlock(VanillaBlocks::END_ROD, "end_rod");
    registerSimpleBlock(VanillaBlocks::RESPAWN_ANCHOR, "respawn_anchor");
    registerSimpleBlock(VanillaBlocks::CHAIN, "iron_chain");

    // 铜锁链（含氧化和涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_CHAIN, "copper_chain");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_CHAIN, "exposed_copper_chain");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_CHAIN, "weathered_copper_chain");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_CHAIN, "oxidized_copper_chain");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_CHAIN, "waxed_copper_chain");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_CHAIN, "waxed_exposed_copper_chain");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_CHAIN, "waxed_weathered_copper_chain");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHAIN, "waxed_oxidized_copper_chain");
    registerSimpleBlock(VanillaBlocks::LADDER, "ladder");
    registerSimpleBlock(VanillaBlocks::SCAFFOLDING, "scaffolding");
    registerSimpleBlock(VanillaBlocks::IRON_BARS, "iron_bars");

    // 铜栏杆（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_BARS, "copper_bars");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_BARS, "exposed_copper_bars");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_BARS, "weathered_copper_bars");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_BARS, "oxidized_copper_bars");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_BARS, "waxed_copper_bars");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_BARS, "waxed_exposed_copper_bars");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_BARS, "waxed_weathered_copper_bars");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_BARS, "waxed_oxidized_copper_bars");

    // 铜格栅（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_GRATE, "copper_grate");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_GRATE, "exposed_copper_grate");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_GRATE, "weathered_copper_grate");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_GRATE, "oxidized_copper_grate");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_GRATE, "waxed_copper_grate");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_GRATE, "waxed_exposed_copper_grate");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_GRATE, "waxed_weathered_copper_grate");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_GRATE, "waxed_oxidized_copper_grate");

    // 铜灯（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_BULB, "copper_bulb");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_BULB, "exposed_copper_bulb");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_BULB, "weathered_copper_bulb");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_BULB, "oxidized_copper_bulb");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_BULB, "waxed_copper_bulb");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_BULB, "waxed_exposed_copper_bulb");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_BULB, "waxed_weathered_copper_bulb");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_BULB, "waxed_oxidized_copper_bulb");

    // 楼梯
    registerSimpleBlock(VanillaBlocks::OAK_STAIRS, "oak_stairs");
    registerSimpleBlock(VanillaBlocks::SPRUCE_STAIRS, "spruce_stairs");
    registerSimpleBlock(VanillaBlocks::BIRCH_STAIRS, "birch_stairs");
    registerSimpleBlock(VanillaBlocks::JUNGLE_STAIRS, "jungle_stairs");
    registerSimpleBlock(VanillaBlocks::ACACIA_STAIRS, "acacia_stairs");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_STAIRS, "dark_oak_stairs");
    registerSimpleBlock(VanillaBlocks::MANGROVE_STAIRS, "mangrove_stairs");
    registerSimpleBlock(VanillaBlocks::CHERRY_STAIRS, "cherry_stairs");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_STAIRS, "pale_oak_stairs");
    registerSimpleBlock(VanillaBlocks::BAMBOO_STAIRS, "bamboo_stairs");
    registerSimpleBlock(VanillaBlocks::BAMBOO_MOSAIC_STAIRS, "bamboo_mosaic_stairs");
    registerSimpleBlock(VanillaBlocks::STONE_STAIRS, "stone_stairs");
    registerSimpleBlock(VanillaBlocks::COBBLESTONE_STAIRS, "cobblestone_stairs");
    registerSimpleBlock(VanillaBlocks::SANDSTONE_STAIRS, "sandstone_stairs");
    registerSimpleBlock(VanillaBlocks::SMOOTH_SANDSTONE_STAIRS, "smooth_sandstone_stairs");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_STAIRS, "prismarine_stairs");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_BRICK_STAIRS, "prismarine_brick_stairs");
    registerSimpleBlock(VanillaBlocks::DARK_PRISMARINE_STAIRS, "dark_prismarine_stairs");
    registerSimpleBlock(VanillaBlocks::STONE_BRICK_STAIRS, "stone_brick_stairs");
    registerSimpleBlock(VanillaBlocks::MOSSY_STONE_BRICK_STAIRS, "mossy_stone_brick_stairs");
    registerSimpleBlock(VanillaBlocks::GRANITE_STAIRS, "granite_stairs");
    registerSimpleBlock(VanillaBlocks::POLISHED_GRANITE_STAIRS, "polished_granite_stairs");
    registerSimpleBlock(VanillaBlocks::DIORITE_STAIRS, "diorite_stairs");
    registerSimpleBlock(VanillaBlocks::POLISHED_DIORITE_STAIRS, "polished_diorite_stairs");
    registerSimpleBlock(VanillaBlocks::ANDESITE_STAIRS, "andesite_stairs");
    registerSimpleBlock(VanillaBlocks::POLISHED_ANDESITE_STAIRS, "polished_andesite_stairs");
    registerSimpleBlock(VanillaBlocks::BRICK_STAIRS, "brick_stairs");
    registerSimpleBlock(VanillaBlocks::MOSSY_COBBLESTONE_STAIRS, "mossy_cobblestone_stairs");
    registerSimpleBlock(VanillaBlocks::NETHER_BRICK_STAIRS, "nether_brick_stairs");
    registerSimpleBlock(VanillaBlocks::RED_NETHER_BRICK_STAIRS, "red_nether_brick_stairs");
    registerSimpleBlock(VanillaBlocks::END_STONE_BRICK_STAIRS, "end_stone_brick_stairs");
    registerSimpleBlock(VanillaBlocks::QUARTZ_STAIRS, "quartz_stairs");
    registerSimpleBlock(VanillaBlocks::SMOOTH_QUARTZ_STAIRS, "smooth_quartz_stairs");
    registerSimpleBlock(VanillaBlocks::PURPUR_STAIRS, "purpur_stairs");
    registerSimpleBlock(VanillaBlocks::RED_SANDSTONE_STAIRS, "red_sandstone_stairs");
    registerSimpleBlock(VanillaBlocks::SMOOTH_RED_SANDSTONE_STAIRS, "smooth_red_sandstone_stairs");

    // 切制铜楼梯（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::CUT_COPPER_STAIRS, "cut_copper_stairs");
    registerSimpleBlock(VanillaBlocks::EXPOSED_CUT_COPPER_STAIRS, "exposed_cut_copper_stairs");
    registerSimpleBlock(VanillaBlocks::WEATHERED_CUT_COPPER_STAIRS, "weathered_cut_copper_stairs");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_CUT_COPPER_STAIRS, "oxidized_cut_copper_stairs");
    registerSimpleBlock(VanillaBlocks::WAXED_CUT_COPPER_STAIRS, "waxed_cut_copper_stairs");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_CUT_COPPER_STAIRS, "waxed_exposed_cut_copper_stairs");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_CUT_COPPER_STAIRS, "waxed_weathered_cut_copper_stairs");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER_STAIRS, "waxed_oxidized_cut_copper_stairs");

    // 台阶
    registerSimpleBlock(VanillaBlocks::OAK_SLAB, "oak_slab");
    registerSimpleBlock(VanillaBlocks::SPRUCE_SLAB, "spruce_slab");
    registerSimpleBlock(VanillaBlocks::BIRCH_SLAB, "birch_slab");
    registerSimpleBlock(VanillaBlocks::JUNGLE_SLAB, "jungle_slab");
    registerSimpleBlock(VanillaBlocks::ACACIA_SLAB, "acacia_slab");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_SLAB, "dark_oak_slab");
    registerSimpleBlock(VanillaBlocks::MANGROVE_SLAB, "mangrove_slab");
    registerSimpleBlock(VanillaBlocks::CHERRY_SLAB, "cherry_slab");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_SLAB, "pale_oak_slab");
    registerSimpleBlock(VanillaBlocks::BAMBOO_SLAB, "bamboo_slab");
    registerSimpleBlock(VanillaBlocks::BAMBOO_MOSAIC_SLAB, "bamboo_mosaic_slab");
    registerSimpleBlock(VanillaBlocks::STONE_SLAB, "stone_slab");
    registerSimpleBlock(VanillaBlocks::COBBLESTONE_SLAB, "cobblestone_slab");
    registerSimpleBlock(VanillaBlocks::SANDSTONE_SLAB, "sandstone_slab");
    registerSimpleBlock(VanillaBlocks::SMOOTH_SANDSTONE_SLAB, "smooth_sandstone_slab");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_SLAB, "prismarine_slab");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_BRICK_SLAB, "prismarine_brick_slab");
    registerSimpleBlock(VanillaBlocks::DARK_PRISMARINE_SLAB, "dark_prismarine_slab");
    registerSimpleBlock(VanillaBlocks::STONE_BRICK_SLAB, "stone_brick_slab");
    registerSimpleBlock(VanillaBlocks::MOSSY_STONE_BRICK_SLAB, "mossy_stone_brick_slab");
    registerSimpleBlock(VanillaBlocks::GRANITE_SLAB, "granite_slab");
    registerSimpleBlock(VanillaBlocks::POLISHED_GRANITE_SLAB, "polished_granite_slab");
    registerSimpleBlock(VanillaBlocks::DIORITE_SLAB, "diorite_slab");
    registerSimpleBlock(VanillaBlocks::POLISHED_DIORITE_SLAB, "polished_diorite_slab");
    registerSimpleBlock(VanillaBlocks::ANDESITE_SLAB, "andesite_slab");
    registerSimpleBlock(VanillaBlocks::POLISHED_ANDESITE_SLAB, "polished_andesite_slab");
    registerSimpleBlock(VanillaBlocks::BRICK_SLAB, "brick_slab");
    registerSimpleBlock(VanillaBlocks::MOSSY_COBBLESTONE_SLAB, "mossy_cobblestone_slab");
    registerSimpleBlock(VanillaBlocks::NETHER_BRICK_SLAB, "nether_brick_slab");
    registerSimpleBlock(VanillaBlocks::RED_NETHER_BRICK_SLAB, "red_nether_brick_slab");
    registerSimpleBlock(VanillaBlocks::END_STONE_BRICK_SLAB, "end_stone_brick_slab");
    registerSimpleBlock(VanillaBlocks::QUARTZ_SLAB, "quartz_slab");
    registerSimpleBlock(VanillaBlocks::SMOOTH_QUARTZ_SLAB, "smooth_quartz_slab");
    registerSimpleBlock(VanillaBlocks::PURPUR_SLAB, "purpur_slab");
    registerSimpleBlock(VanillaBlocks::RED_SANDSTONE_SLAB, "red_sandstone_slab");
    registerSimpleBlock(VanillaBlocks::SMOOTH_RED_SANDSTONE_SLAB, "smooth_red_sandstone_slab");
    registerSimpleBlock(VanillaBlocks::CUT_SANDSTONE_SLAB, "cut_sandstone_slab");
    registerSimpleBlock(VanillaBlocks::CUT_RED_SANDSTONE_SLAB, "cut_red_sandstone_slab");
    registerSimpleBlock(VanillaBlocks::SMOOTH_STONE_SLAB, "smooth_stone_slab");
    registerSimpleBlock(VanillaBlocks::PETRIFIED_OAK_SLAB, "petrified_oak_slab");

    // 切制铜台阶（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::CUT_COPPER_SLAB, "cut_copper_slab");
    registerSimpleBlock(VanillaBlocks::EXPOSED_CUT_COPPER_SLAB, "exposed_cut_copper_slab");
    registerSimpleBlock(VanillaBlocks::WEATHERED_CUT_COPPER_SLAB, "weathered_cut_copper_slab");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_CUT_COPPER_SLAB, "oxidized_cut_copper_slab");
    registerSimpleBlock(VanillaBlocks::WAXED_CUT_COPPER_SLAB, "waxed_cut_copper_slab");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_CUT_COPPER_SLAB, "waxed_exposed_cut_copper_slab");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_CUT_COPPER_SLAB, "waxed_weathered_cut_copper_slab");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER_SLAB, "waxed_oxidized_cut_copper_slab");

    // 墙
    registerSimpleBlock(VanillaBlocks::COBBLESTONE_WALL, "cobblestone_wall");
    registerSimpleBlock(VanillaBlocks::MOSSY_COBBLESTONE_WALL, "mossy_cobblestone_wall");
    registerSimpleBlock(VanillaBlocks::STONE_BRICK_WALL, "stone_brick_wall");
    registerSimpleBlock(VanillaBlocks::MOSSY_STONE_BRICK_WALL, "mossy_stone_brick_wall");
    registerSimpleBlock(VanillaBlocks::BRICK_WALL, "brick_wall");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_WALL, "prismarine_wall");
    registerSimpleBlock(VanillaBlocks::SANDSTONE_WALL, "sandstone_wall");
    registerSimpleBlock(VanillaBlocks::RED_SANDSTONE_WALL, "red_sandstone_wall");
    registerSimpleBlock(VanillaBlocks::NETHER_BRICK_WALL, "nether_brick_wall");
    registerSimpleBlock(VanillaBlocks::RED_NETHER_BRICK_WALL, "red_nether_brick_wall");
    registerSimpleBlock(VanillaBlocks::END_STONE_BRICK_WALL, "end_stone_brick_wall");
    registerSimpleBlock(VanillaBlocks::GRANITE_WALL, "granite_wall");
    registerSimpleBlock(VanillaBlocks::DIORITE_WALL, "diorite_wall");
    registerSimpleBlock(VanillaBlocks::ANDESITE_WALL, "andesite_wall");

    // 栅栏
    registerSimpleBlock(VanillaBlocks::OAK_FENCE, "oak_fence");
    registerSimpleBlock(VanillaBlocks::SPRUCE_FENCE, "spruce_fence");
    registerSimpleBlock(VanillaBlocks::BIRCH_FENCE, "birch_fence");
    registerSimpleBlock(VanillaBlocks::JUNGLE_FENCE, "jungle_fence");
    registerSimpleBlock(VanillaBlocks::ACACIA_FENCE, "acacia_fence");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_FENCE, "dark_oak_fence");
    registerSimpleBlock(VanillaBlocks::MANGROVE_FENCE, "mangrove_fence");
    registerSimpleBlock(VanillaBlocks::CHERRY_FENCE, "cherry_fence");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_FENCE, "pale_oak_fence");
    registerSimpleBlock(VanillaBlocks::BAMBOO_FENCE, "bamboo_fence");
    registerSimpleBlock(VanillaBlocks::NETHER_BRICK_FENCE, "nether_brick_fence");
    registerSimpleBlock(VanillaBlocks::OAK_FENCE_GATE, "oak_fence_gate");
    registerSimpleBlock(VanillaBlocks::SPRUCE_FENCE_GATE, "spruce_fence_gate");
    registerSimpleBlock(VanillaBlocks::BIRCH_FENCE_GATE, "birch_fence_gate");
    registerSimpleBlock(VanillaBlocks::JUNGLE_FENCE_GATE, "jungle_fence_gate");
    registerSimpleBlock(VanillaBlocks::ACACIA_FENCE_GATE, "acacia_fence_gate");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_FENCE_GATE, "dark_oak_fence_gate");
    registerSimpleBlock(VanillaBlocks::MANGROVE_FENCE_GATE, "mangrove_fence_gate");
    registerSimpleBlock(VanillaBlocks::CHERRY_FENCE_GATE, "cherry_fence_gate");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_FENCE_GATE, "pale_oak_fence_gate");
    registerSimpleBlock(VanillaBlocks::BAMBOO_FENCE_GATE, "bamboo_fence_gate");
    registerSimpleBlock(VanillaBlocks::CRIMSON_FENCE_GATE, "crimson_fence_gate");
    registerSimpleBlock(VanillaBlocks::WARPED_FENCE_GATE, "warped_fence_gate");

    // 门和活板门
    registerSimpleBlock(VanillaBlocks::OAK_DOOR, "oak_door");
    registerSimpleBlock(VanillaBlocks::SPRUCE_DOOR, "spruce_door");
    registerSimpleBlock(VanillaBlocks::BIRCH_DOOR, "birch_door");
    registerSimpleBlock(VanillaBlocks::JUNGLE_DOOR, "jungle_door");
    registerSimpleBlock(VanillaBlocks::ACACIA_DOOR, "acacia_door");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_DOOR, "dark_oak_door");
    registerSimpleBlock(VanillaBlocks::MANGROVE_DOOR, "mangrove_door");
    registerSimpleBlock(VanillaBlocks::CHERRY_DOOR, "cherry_door");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_DOOR, "pale_oak_door");
    registerSimpleBlock(VanillaBlocks::BAMBOO_DOOR, "bamboo_door");
    registerSimpleBlock(VanillaBlocks::CRIMSON_DOOR, "crimson_door");
    registerSimpleBlock(VanillaBlocks::WARPED_DOOR, "warped_door");
    registerSimpleBlock(VanillaBlocks::IRON_DOOR, "iron_door");
    // 铜门（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_DOOR, "copper_door");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_DOOR, "exposed_copper_door");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_DOOR, "weathered_copper_door");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_DOOR, "oxidized_copper_door");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_DOOR, "waxed_copper_door");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_DOOR, "waxed_exposed_copper_door");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_DOOR, "waxed_weathered_copper_door");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_DOOR, "waxed_oxidized_copper_door");
    registerSimpleBlock(VanillaBlocks::OAK_TRAPDOOR, "oak_trapdoor");
    registerSimpleBlock(VanillaBlocks::SPRUCE_TRAPDOOR, "spruce_trapdoor");
    registerSimpleBlock(VanillaBlocks::BIRCH_TRAPDOOR, "birch_trapdoor");
    registerSimpleBlock(VanillaBlocks::JUNGLE_TRAPDOOR, "jungle_trapdoor");
    registerSimpleBlock(VanillaBlocks::ACACIA_TRAPDOOR, "acacia_trapdoor");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_TRAPDOOR, "dark_oak_trapdoor");
    registerSimpleBlock(VanillaBlocks::MANGROVE_TRAPDOOR, "mangrove_trapdoor");
    registerSimpleBlock(VanillaBlocks::CHERRY_TRAPDOOR, "cherry_trapdoor");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_TRAPDOOR, "pale_oak_trapdoor");
    registerSimpleBlock(VanillaBlocks::BAMBOO_TRAPDOOR, "bamboo_trapdoor");
    registerSimpleBlock(VanillaBlocks::CRIMSON_TRAPDOOR, "crimson_trapdoor");
    registerSimpleBlock(VanillaBlocks::WARPED_TRAPDOOR, "warped_trapdoor");
    registerSimpleBlock(VanillaBlocks::IRON_TRAPDOOR, "iron_trapdoor");
    // 铜活板门（8 种氧化/涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_TRAPDOOR, "copper_trapdoor");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_TRAPDOOR, "exposed_copper_trapdoor");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_TRAPDOOR, "weathered_copper_trapdoor");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_TRAPDOOR, "oxidized_copper_trapdoor");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_TRAPDOOR, "waxed_copper_trapdoor");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR, "waxed_exposed_copper_trapdoor");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR, "waxed_weathered_copper_trapdoor");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR, "waxed_oxidized_copper_trapdoor");

    // 红石方块
    // 注意：REDSTONE_WIRE 没有独立物品，红石粉物品（REDSTONE）放在地上时变成 REDSTONE_WIRE 方块
    // REDSTONE_BLOCK 已在矿物方块中注册
    registerSimpleBlock(VanillaBlocks::TORCH, "torch");
    // 墙上变体映射到同一物品
    registerSimpleBlock(VanillaBlocks::WALL_TORCH, "torch");
    registerSimpleBlock(VanillaBlocks::REDSTONE_TORCH, "redstone_torch");
    registerSimpleBlock(VanillaBlocks::REDSTONE_WALL_TORCH, "redstone_torch");
    registerSimpleBlock(VanillaBlocks::SOUL_TORCH, "soul_torch");
    registerSimpleBlock(VanillaBlocks::SOUL_WALL_TORCH, "soul_torch");
    registerSimpleBlock(VanillaBlocks::REDSTONE_LAMP, "redstone_lamp");
    registerSimpleBlock(VanillaBlocks::REDSTONE_REPEATER, "repeater");
    registerSimpleBlock(VanillaBlocks::REDSTONE_COMPARATOR, "comparator");
    registerSimpleBlock(VanillaBlocks::OBSERVER, "observer");

    // 避雷针（含氧化和涂蜡变种）
    registerSimpleBlock(VanillaBlocks::LIGHTNING_ROD, "lightning_rod");
    registerSimpleBlock(VanillaBlocks::EXPOSED_LIGHTNING_ROD, "exposed_lightning_rod");
    registerSimpleBlock(VanillaBlocks::WEATHERED_LIGHTNING_ROD, "weathered_lightning_rod");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_LIGHTNING_ROD, "oxidized_lightning_rod");
    registerSimpleBlock(VanillaBlocks::WAXED_LIGHTNING_ROD, "waxed_lightning_rod");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_LIGHTNING_ROD, "waxed_exposed_lightning_rod");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_LIGHTNING_ROD, "waxed_weathered_lightning_rod");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_LIGHTNING_ROD, "waxed_oxidized_lightning_rod");

    // 铜傀儡雕像（1.21.11，含氧化和涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_GOLEM_STATUE, "copper_golem_statue");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE, "exposed_copper_golem_statue");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE, "weathered_copper_golem_statue");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE, "oxidized_copper_golem_statue");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_GOLEM_STATUE, "waxed_copper_golem_statue");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE, "waxed_exposed_copper_golem_statue");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE, "waxed_weathered_copper_golem_statue");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE, "waxed_oxidized_copper_golem_statue");

    // 铜箱子（1.21.11，含氧化和涂蜡变种）
    registerSimpleBlock(VanillaBlocks::COPPER_CHEST, "copper_chest");
    registerSimpleBlock(VanillaBlocks::EXPOSED_COPPER_CHEST, "exposed_copper_chest");
    registerSimpleBlock(VanillaBlocks::WEATHERED_COPPER_CHEST, "weathered_copper_chest");
    registerSimpleBlock(VanillaBlocks::OXIDIZED_COPPER_CHEST, "oxidized_copper_chest");
    registerSimpleBlock(VanillaBlocks::WAXED_COPPER_CHEST, "waxed_copper_chest");
    registerSimpleBlock(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST, "waxed_exposed_copper_chest");
    registerSimpleBlock(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST, "waxed_weathered_copper_chest");
    registerSimpleBlock(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST, "waxed_oxidized_copper_chest");

    // 钟
    registerSimpleBlock(VanillaBlocks::BELL, "bell");

    registerSimpleBlock(VanillaBlocks::LEVER, "lever");
    registerSimpleBlock(VanillaBlocks::STONE_BUTTON, "stone_button");
    registerSimpleBlock(VanillaBlocks::OAK_BUTTON, "oak_button");
    registerSimpleBlock(VanillaBlocks::SPRUCE_BUTTON, "spruce_button");
    registerSimpleBlock(VanillaBlocks::BIRCH_BUTTON, "birch_button");
    registerSimpleBlock(VanillaBlocks::JUNGLE_BUTTON, "jungle_button");
    registerSimpleBlock(VanillaBlocks::ACACIA_BUTTON, "acacia_button");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_BUTTON, "dark_oak_button");
    registerSimpleBlock(VanillaBlocks::CRIMSON_BUTTON, "crimson_button");
    registerSimpleBlock(VanillaBlocks::WARPED_BUTTON, "warped_button");
    registerSimpleBlock(VanillaBlocks::MANGROVE_BUTTON, "mangrove_button");
    registerSimpleBlock(VanillaBlocks::CHERRY_BUTTON, "cherry_button");
    registerSimpleBlock(VanillaBlocks::BAMBOO_BUTTON, "bamboo_button");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_BUTTON, "pale_oak_button");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE_BUTTON, "polished_blackstone_button");
    registerSimpleBlock(VanillaBlocks::STONE_PRESSURE_PLATE, "stone_pressure_plate");
    registerSimpleBlock(VanillaBlocks::OAK_PRESSURE_PLATE, "oak_pressure_plate");
    registerSimpleBlock(VanillaBlocks::SPRUCE_PRESSURE_PLATE, "spruce_pressure_plate");
    registerSimpleBlock(VanillaBlocks::BIRCH_PRESSURE_PLATE, "birch_pressure_plate");
    registerSimpleBlock(VanillaBlocks::JUNGLE_PRESSURE_PLATE, "jungle_pressure_plate");
    registerSimpleBlock(VanillaBlocks::ACACIA_PRESSURE_PLATE, "acacia_pressure_plate");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_PRESSURE_PLATE, "dark_oak_pressure_plate");
    registerSimpleBlock(VanillaBlocks::CRIMSON_PRESSURE_PLATE, "crimson_pressure_plate");
    registerSimpleBlock(VanillaBlocks::WARPED_PRESSURE_PLATE, "warped_pressure_plate");
    registerSimpleBlock(VanillaBlocks::MANGROVE_PRESSURE_PLATE, "mangrove_pressure_plate");
    registerSimpleBlock(VanillaBlocks::CHERRY_PRESSURE_PLATE, "cherry_pressure_plate");
    registerSimpleBlock(VanillaBlocks::BAMBOO_PRESSURE_PLATE, "bamboo_pressure_plate");
    registerSimpleBlock(VanillaBlocks::PALE_OAK_PRESSURE_PLATE, "pale_oak_pressure_plate");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE_PRESSURE_PLATE, "polished_blackstone_pressure_plate");
    registerSimpleBlock(VanillaBlocks::LIGHT_WEIGHTED_PRESSURE_PLATE, "light_weighted_pressure_plate");
    registerSimpleBlock(VanillaBlocks::HEAVY_WEIGHTED_PRESSURE_PLATE, "heavy_weighted_pressure_plate");
    registerSimpleBlock(VanillaBlocks::DAYLIGHT_DETECTOR, "daylight_detector");
    registerSimpleBlock(VanillaBlocks::PISTON, "piston");
    registerSimpleBlock(VanillaBlocks::STICKY_PISTON, "sticky_piston");
    registerSimpleBlock(VanillaBlocks::DISPENSER, "dispenser");
    registerSimpleBlock(VanillaBlocks::DROPPER, "dropper");
    // tripwire 在 vanilla 中没有独立物品（由 tripwire_hook 与 string 触发形成），不注册 BlockItem。
    registerSimpleBlock(VanillaBlocks::TRIPWIRE_HOOK, "tripwire_hook");

    // 铁轨
    registerSimpleBlock(VanillaBlocks::RAIL, "rail");
    registerSimpleBlock(VanillaBlocks::POWERED_RAIL, "powered_rail");
    registerSimpleBlock(VanillaBlocks::DETECTOR_RAIL, "detector_rail");
    registerSimpleBlock(VanillaBlocks::ACTIVATOR_RAIL, "activator_rail");

    // 其他未分类方块
    registerSimpleBlock(VanillaBlocks::DEAD_BUSH, "dead_bush");
    registerSimpleBlock(VanillaBlocks::TURTLE_EGG, "turtle_egg");
    registerSimpleBlock(VanillaBlocks::BEE_NEST, "bee_nest");
    registerSimpleBlock(VanillaBlocks::BEEHIVE, "beehive");
    registerSimpleBlock(VanillaBlocks::WARPED_WART_BLOCK, "warped_wart_block");
    registerSimpleBlock(VanillaBlocks::WEEPING_VINES, "weeping_vines");
    registerSimpleBlock(VanillaBlocks::TWISTING_VINES, "twisting_vines");
    registerSimpleBlock(VanillaBlocks::CRIMSON_ROOTS, "crimson_roots");
    registerSimpleBlock(VanillaBlocks::WARPED_ROOTS, "warped_roots");
    registerSimpleBlock(VanillaBlocks::NETHER_SPROUTS, "nether_sprouts");
    registerSimpleBlock(VanillaBlocks::END_PORTAL_FRAME, "end_portal_frame");
    registerSimpleBlock(VanillaBlocks::CHORUS_FLOWER, "chorus_flower");

    // 耕地
    registerSimpleBlock(VanillaBlocks::FARMLAND, "farmland");

    // 农作物方块的 BlockItem 注册：
    // 种子物品现在注册为 SeedsItem（BlockItem 子类），关联到对应的作物方块。
    // 例如 WHEAT_SEEDS 关联到 minecraft:wheat 方块，PUMPKIN_SEEDS 关联到 minecraft:pumpkin_stem 方块。
    // SeedsItem 在 Items::_registerSeeds() 中通过 registerItem<SeedsItem>() 注册，
    // 此处需要建立作物方块到种子物品的映射，以便通过方块查找对应的物品。
    //
    // 注意：Items::WHEAT 是普通物品（minecraft:wheat），不是 BlockItem，与 WHEAT_SEEDS 无关。
    // Items::WHEAT 的注册在 _registerCrops() 中，其物品 ID 与小麦方块 ID 同名但类型不同。
    // registerSimpleBlock 会检测到 Items::WHEAT 已存在且非 BlockItem 而跳过注册，这是正确行为。
    // 胡萝卜/马铃薯的物品注册同理——它们是食物，不是 BlockItem。
    {
        auto registerSeedBlockItem = [this](Block* cropBlock, Item* seedItem) {
            if (cropBlock == nullptr || seedItem == nullptr) {
                return;
            }
            auto* seedBlockItem = dynamic_cast<BlockItem*>(seedItem);
            if (seedBlockItem != nullptr) {
                registerBlockItem(*cropBlock, *seedBlockItem);
            }
        };
        registerSeedBlockItem(VanillaBlocks::WHEAT, Items::WHEAT_SEEDS);
        registerSeedBlockItem(VanillaBlocks::PUMPKIN_STEM, Items::PUMPKIN_SEEDS);
        registerSeedBlockItem(VanillaBlocks::MELON_STEM, Items::MELON_SEEDS);
        registerSeedBlockItem(VanillaBlocks::BEETROOTS, Items::BEETROOT_SEEDS);
        registerSeedBlockItem(VanillaBlocks::TORCHFLOWER_CROP, Items::TORCHFLOWER_SEEDS);
        registerSeedBlockItem(VanillaBlocks::PITCHER_CROP, Items::PITCHER_POD);
    }

    // 可可豆方块：minecraft:cocoa 方块与 minecraft:cocoa_beans 物品名称不同，
    // Items::COCOA_BEANS 已在 Items.cpp 中通过 registerBlockBackedItem 注册为 BlockItem
    // （关联到 VanillaBlocks::COCOA），这里显式注册映射。
    if (VanillaBlocks::COCOA != nullptr && Items::COCOA_BEANS != nullptr) {
        auto* cocoaBlockItem = dynamic_cast<BlockItem*>(Items::COCOA_BEANS);
        if (cocoaBlockItem != nullptr) {
            registerBlockItem(*VanillaBlocks::COCOA, *cocoaBlockItem);
        }
    }

    // 甜浆果丛方块：minecraft:sweet_berry_bush 方块不能由物品直接放置，
    // SWEET_BERRIES 物品是食物而非种植物品，因此不需要注册 BlockItem。

    // 旗帜（站立式，每种颜色对应一个BannerItem；墙壁旗帜不需要单独注册物品）
    registerSimpleBlock(VanillaBlocks::WHITE_BANNER, "white_banner");
    registerSimpleBlock(VanillaBlocks::ORANGE_BANNER, "orange_banner");
    registerSimpleBlock(VanillaBlocks::MAGENTA_BANNER, "magenta_banner");
    registerSimpleBlock(VanillaBlocks::LIGHT_BLUE_BANNER, "light_blue_banner");
    registerSimpleBlock(VanillaBlocks::YELLOW_BANNER, "yellow_banner");
    registerSimpleBlock(VanillaBlocks::LIME_BANNER, "lime_banner");
    registerSimpleBlock(VanillaBlocks::PINK_BANNER, "pink_banner");
    registerSimpleBlock(VanillaBlocks::GRAY_BANNER, "gray_banner");
    registerSimpleBlock(VanillaBlocks::LIGHT_GRAY_BANNER, "light_gray_banner");
    registerSimpleBlock(VanillaBlocks::CYAN_BANNER, "cyan_banner");
    registerSimpleBlock(VanillaBlocks::PURPLE_BANNER, "purple_banner");
    registerSimpleBlock(VanillaBlocks::BLUE_BANNER, "blue_banner");
    registerSimpleBlock(VanillaBlocks::BROWN_BANNER, "brown_banner");
    registerSimpleBlock(VanillaBlocks::GREEN_BANNER, "green_banner");
    registerSimpleBlock(VanillaBlocks::RED_BANNER, "red_banner");
    registerSimpleBlock(VanillaBlocks::BLACK_BANNER, "black_banner");

    // 墙壁旗帜映射到与站立旗帜相同的物品
    {
        auto registerWallBanner = [this](Block* wallBlock, Block* standingBlock) {
            if (wallBlock == nullptr || standingBlock == nullptr) {
                return;
            }
            const BlockItem* standingItem = getBlockItem(standingBlock->blockId());
            if (standingItem != nullptr) {
                m_blockToItem[wallBlock->blockId()] = standingItem;
            }
        };
        registerWallBanner(VanillaBlocks::WHITE_WALL_BANNER, VanillaBlocks::WHITE_BANNER);
        registerWallBanner(VanillaBlocks::ORANGE_WALL_BANNER, VanillaBlocks::ORANGE_BANNER);
        registerWallBanner(VanillaBlocks::MAGENTA_WALL_BANNER, VanillaBlocks::MAGENTA_BANNER);
        registerWallBanner(VanillaBlocks::LIGHT_BLUE_WALL_BANNER, VanillaBlocks::LIGHT_BLUE_BANNER);
        registerWallBanner(VanillaBlocks::YELLOW_WALL_BANNER, VanillaBlocks::YELLOW_BANNER);
        registerWallBanner(VanillaBlocks::LIME_WALL_BANNER, VanillaBlocks::LIME_BANNER);
        registerWallBanner(VanillaBlocks::PINK_WALL_BANNER, VanillaBlocks::PINK_BANNER);
        registerWallBanner(VanillaBlocks::GRAY_WALL_BANNER, VanillaBlocks::GRAY_BANNER);
        registerWallBanner(VanillaBlocks::LIGHT_GRAY_WALL_BANNER, VanillaBlocks::LIGHT_GRAY_BANNER);
        registerWallBanner(VanillaBlocks::CYAN_WALL_BANNER, VanillaBlocks::CYAN_BANNER);
        registerWallBanner(VanillaBlocks::PURPLE_WALL_BANNER, VanillaBlocks::PURPLE_BANNER);
        registerWallBanner(VanillaBlocks::BLUE_WALL_BANNER, VanillaBlocks::BLUE_BANNER);
        registerWallBanner(VanillaBlocks::BROWN_WALL_BANNER, VanillaBlocks::BROWN_BANNER);
        registerWallBanner(VanillaBlocks::GREEN_WALL_BANNER, VanillaBlocks::GREEN_BANNER);
        registerWallBanner(VanillaBlocks::RED_WALL_BANNER, VanillaBlocks::RED_BANNER);
        registerWallBanner(VanillaBlocks::BLACK_WALL_BANNER, VanillaBlocks::BLACK_BANNER);
    }

    // 告示牌 - 站立告示牌由 Items.cpp 的 WallOrFloorItem 注册，
    // 这里需要为站立和墙壁变体建立映射
    {
        auto registerWallSign = [this](Block* standingBlock, Block* wallBlock) {
            if (standingBlock == nullptr || wallBlock == nullptr) {
                return;
            }
            // 站立告示牌由 Items.cpp 通过 WallOrFloorItem 注册，从 ItemRegistry 获取已注册的物品
            const ResourceLocation& blockLoc = standingBlock->blockLocation();
            ResourceLocation itemLoc(blockLoc.namespace_(), blockLoc.path());
            Item* existingItem = ItemRegistry::instance().getItem(itemLoc);
            if (existingItem == nullptr) {
                return;
            }
            auto* signItem = dynamic_cast<BlockItem*>(existingItem);
            if (signItem == nullptr) {
                return;
            }
            // 站立告示牌的映射
            m_blockToItem[standingBlock->blockId()] = signItem;
            m_itemToBlock[signItem->itemId()] = standingBlock;
            m_itemIdToBlockItem[signItem->itemId()] = signItem;
            // 墙壁告示牌映射到相同的物品
            m_blockToItem[wallBlock->blockId()] = signItem;
        };

        // 12种木材类型的告示牌
        registerWallSign(VanillaBlocks::OAK_SIGN, VanillaBlocks::OAK_WALL_SIGN);
        registerWallSign(VanillaBlocks::SPRUCE_SIGN, VanillaBlocks::SPRUCE_WALL_SIGN);
        registerWallSign(VanillaBlocks::BIRCH_SIGN, VanillaBlocks::BIRCH_WALL_SIGN);
        registerWallSign(VanillaBlocks::JUNGLE_SIGN, VanillaBlocks::JUNGLE_WALL_SIGN);
        registerWallSign(VanillaBlocks::ACACIA_SIGN, VanillaBlocks::ACACIA_WALL_SIGN);
        registerWallSign(VanillaBlocks::DARK_OAK_SIGN, VanillaBlocks::DARK_OAK_WALL_SIGN);
        registerWallSign(VanillaBlocks::CRIMSON_SIGN, VanillaBlocks::CRIMSON_WALL_SIGN);
        registerWallSign(VanillaBlocks::WARPED_SIGN, VanillaBlocks::WARPED_WALL_SIGN);
        registerWallSign(VanillaBlocks::MANGROVE_SIGN, VanillaBlocks::MANGROVE_WALL_SIGN);
        registerWallSign(VanillaBlocks::CHERRY_SIGN, VanillaBlocks::CHERRY_WALL_SIGN);
        registerWallSign(VanillaBlocks::BAMBOO_SIGN, VanillaBlocks::BAMBOO_WALL_SIGN);
        registerWallSign(VanillaBlocks::PALE_OAK_SIGN, VanillaBlocks::PALE_OAK_WALL_SIGN);
    }

    // 悬挂告示牌 - 同样使用 WallOrFloorItem 注册
    {
        auto registerWallHangingSign = [this](Block* hangingBlock, Block* wallHangingBlock) {
            if (hangingBlock == nullptr || wallHangingBlock == nullptr) {
                return;
            }
            const ResourceLocation& blockLoc = hangingBlock->blockLocation();
            ResourceLocation itemLoc(blockLoc.namespace_(), blockLoc.path());
            Item* existingItem = ItemRegistry::instance().getItem(itemLoc);
            if (existingItem == nullptr) {
                return;
            }
            auto* hangingSignItem = dynamic_cast<BlockItem*>(existingItem);
            if (hangingSignItem == nullptr) {
                return;
            }
            // 悬挂告示牌的映射
            m_blockToItem[hangingBlock->blockId()] = hangingSignItem;
            m_itemToBlock[hangingSignItem->itemId()] = hangingBlock;
            m_itemIdToBlockItem[hangingSignItem->itemId()] = hangingSignItem;
            // 墙壁悬挂告示牌映射到相同的物品
            m_blockToItem[wallHangingBlock->blockId()] = hangingSignItem;
        };

        // 12种木材类型的悬挂告示牌
        registerWallHangingSign(VanillaBlocks::OAK_HANGING_SIGN, VanillaBlocks::OAK_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::SPRUCE_HANGING_SIGN, VanillaBlocks::SPRUCE_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::BIRCH_HANGING_SIGN, VanillaBlocks::BIRCH_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::JUNGLE_HANGING_SIGN, VanillaBlocks::JUNGLE_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::ACACIA_HANGING_SIGN, VanillaBlocks::ACACIA_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::DARK_OAK_HANGING_SIGN, VanillaBlocks::DARK_OAK_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::CRIMSON_HANGING_SIGN, VanillaBlocks::CRIMSON_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::WARPED_HANGING_SIGN, VanillaBlocks::WARPED_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::MANGROVE_HANGING_SIGN, VanillaBlocks::MANGROVE_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::CHERRY_HANGING_SIGN, VanillaBlocks::CHERRY_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::BAMBOO_HANGING_SIGN, VanillaBlocks::BAMBOO_WALL_HANGING_SIGN);
        registerWallHangingSign(VanillaBlocks::PALE_OAK_HANGING_SIGN, VanillaBlocks::PALE_OAK_WALL_HANGING_SIGN);
    }

    // 游戏管理员方块 - 需要创造模式 + OP等级>=2 才能放置和破坏
    // 这些方块使用 GameMasterBlockItem 替代普通 BlockItem，
    // 在放置时检查玩家 canUseGameMasterBlocks() 权限
    {
        auto registerGameMasterBlock = [this](Block* block, const std::string& name) {
            if (block == nullptr) {
                spdlog::warn("GameMaster block '{}' is null, skipping", name);
                return;
            }

            const ResourceLocation& blockLoc = block->blockLocation();
            ResourceLocation itemLoc(blockLoc.namespace_(), blockLoc.path());

            BlockItem* registeredItem = nullptr;
            Item* existingItem = ItemRegistry::instance().getItem(itemLoc);
            if (existingItem != nullptr) {
                registeredItem = dynamic_cast<BlockItem*>(existingItem);
                if (registeredItem == nullptr) {
                    spdlog::warn("Item '{}' already exists but is not a BlockItem, skipping", itemLoc.toString());
                    return;
                }
            } else {
                registeredItem = &ItemRegistry::instance().registerItem<GameMasterBlockItem>(
                    itemLoc, *block, ItemProperties().maxStackSize(64));
            }

            // 存储映射关系
            u32 blockId = block->blockId();
            ItemId itemId = registeredItem->itemId();
            m_blockToItem[blockId] = registeredItem;
            m_itemToBlock[itemId] = block;
            m_itemIdToBlockItem[itemId] = registeredItem;
        };

        registerGameMasterBlock(VanillaBlocks::STRUCTURE_BLOCK, "structure_block");
        registerGameMasterBlock(VanillaBlocks::JIGSAW, "jigsaw");

        // BarrierBlock 使用普通 BlockItem 注册（MC Java 中 BarrierBlock 不是 GameMasterBlockItem，
        // 创造模式玩家均可放置，只有硬度=-1 防止破坏）
        registerSimpleBlock(VanillaBlocks::BARRIER, "barrier");

        // 命令方块 - 需要创造模式 + OP等级>=2 才能放置
        registerGameMasterBlock(VanillaBlocks::COMMAND_BLOCK, "command_block");
        registerGameMasterBlock(VanillaBlocks::REPEATING_COMMAND_BLOCK, "repeating_command_block");
        registerGameMasterBlock(VanillaBlocks::CHAIN_COMMAND_BLOCK, "chain_command_block");
    }

    // 花盆方块
    // MC Java 设计：所有 potted_* 方块共用一个 minecraft:flower_pot 物品。
    // 空花盆放置后得到 minecraft:flower_pot 方块；
    // 玩家右键空花盆并手持可盆栽植物时，方块变为对应的 potted_* 方块（无新物品产生）。
    // 因此这里将所有 potted_* 方块都映射到同一个 flower_pot 物品。
    registerSimpleBlock(VanillaBlocks::FLOWER_POT, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_OAK_SAPLING, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_SPRUCE_SAPLING, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_BIRCH_SAPLING, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_JUNGLE_SAPLING, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_ACACIA_SAPLING, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_DARK_OAK_SAPLING, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_CHERRY_SAPLING, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_PALE_OAK_SAPLING, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_MANGROVE_PROPAGULE, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_DANDELION, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_POPPY, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_BLUE_ORCHID, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_ALLIUM, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_AZURE_BLUET, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_RED_TULIP, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_ORANGE_TULIP, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_WHITE_TULIP, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_PINK_TULIP, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_OXEYE_DAISY, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_CORNFLOWER, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_LILY_OF_THE_VALLEY, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_WITHER_ROSE, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_TORCHFLOWER, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_OPEN_EYEBLOSSOM, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_CLOSED_EYEBLOSSOM, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_FERN, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_DEAD_BUSH, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_RED_MUSHROOM, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_BROWN_MUSHROOM, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_CACTUS, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_BAMBOO, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_CRIMSON_FUNGUS, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_WARPED_FUNGUS, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_CRIMSON_ROOTS, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_WARPED_ROOTS, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_AZALEA_BUSH, "flower_pot");
    registerSimpleBlock(VanillaBlocks::POTTED_FLOWERING_AZALEA_BUSH, "flower_pot");

    m_initialized = true;
    spdlog::info("Registered {} block items", m_itemToBlock.size());
}

} // namespace mc
