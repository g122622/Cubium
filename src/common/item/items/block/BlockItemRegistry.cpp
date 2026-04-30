#include "BlockItemRegistry.hpp"
#include "../../core/ItemRegistry.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include <spdlog/spdlog.h>

namespace mc {

BlockItemRegistry& BlockItemRegistry::instance()
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

const BlockItem* BlockItemRegistry::getBlockItem(u32 blockId) const
{
    auto it = m_blockToItem.find(blockId);
    return it != m_blockToItem.end() ? it->second : nullptr;
}

const BlockItem* BlockItemRegistry::getBlockItemByItemId(ItemId itemId) const
{
    auto it = m_itemIdToBlockItem.find(itemId);
    return it != m_itemIdToBlockItem.end() ? it->second : nullptr;
}

const BlockItem* BlockItemRegistry::getBlockItem(const Block& block) const
{
    return getBlockItem(block.blockId());
}

const Block* BlockItemRegistry::getBlock(ItemId itemId) const
{
    auto it = m_itemToBlock.find(itemId);
    return it != m_itemToBlock.end() ? it->second : nullptr;
}

bool BlockItemRegistry::isBlockItem(const Item* item) const
{
    if (item == nullptr) {
        return false;
    }
    return isBlockItem(item->itemId());
}

bool BlockItemRegistry::isBlockItem(ItemId itemId) const
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

void BlockItemRegistry::clear()
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

    auto registerSimpleBlock = [this](Block* block, const String& name) {
        if (block == nullptr) {
            spdlog::warn("Block '{}' is null, skipping", name);
            return;
        }

        // 从方块位置推断物品名称
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
                itemLoc,
                *block,
                ItemProperties().maxStackSize(64)
            );
        }

        // 获取注册后的信息
        u32 blockId = block->blockId();
        ItemId itemId = registeredItem->itemId();

        // 存储映射关系（ItemRegistry 拥有物品的所有权）
        m_blockToItem[blockId] = registeredItem;
        m_itemToBlock[itemId] = block;
        m_itemIdToBlockItem[itemId] = registeredItem;

        spdlog::debug("Registered block item: {} -> blockId={}, itemId={}",
                      name, blockId, itemId);
    };

    // 基础方块
    registerSimpleBlock(VanillaBlocks::STONE, "stone");
    registerSimpleBlock(VanillaBlocks::GRASS_BLOCK, "grass_block");
    registerSimpleBlock(VanillaBlocks::DIRT, "dirt");
    registerSimpleBlock(VanillaBlocks::COBBLESTONE, "cobblestone");
    registerSimpleBlock(VanillaBlocks::OAK_PLANKS, "oak_planks");
    registerSimpleBlock(VanillaBlocks::BEDROCK, "bedrock");
    registerSimpleBlock(VanillaBlocks::SAND, "sand");
    registerSimpleBlock(VanillaBlocks::GRAVEL, "gravel");

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
    registerSimpleBlock(VanillaBlocks::RED_SANDSTONE, "red_sandstone");

    // 矿石
    registerSimpleBlock(VanillaBlocks::GOLD_ORE, "gold_ore");
    registerSimpleBlock(VanillaBlocks::IRON_ORE, "iron_ore");
    registerSimpleBlock(VanillaBlocks::COAL_ORE, "coal_ore");
    registerSimpleBlock(VanillaBlocks::DIAMOND_ORE, "diamond_ore");
    registerSimpleBlock(VanillaBlocks::EMERALD_ORE, "emerald_ore");
    registerSimpleBlock(VanillaBlocks::LAPIS_ORE, "lapis_ore");
    registerSimpleBlock(VanillaBlocks::REDSTONE_ORE, "redstone_ore");

    // 矿物方块
    registerSimpleBlock(VanillaBlocks::GOLD_BLOCK, "gold_block");
    registerSimpleBlock(VanillaBlocks::IRON_BLOCK, "iron_block");
    registerSimpleBlock(VanillaBlocks::DIAMOND_BLOCK, "diamond_block");
    registerSimpleBlock(VanillaBlocks::EMERALD_BLOCK, "emerald_block");
    registerSimpleBlock(VanillaBlocks::LAPIS_BLOCK, "lapis_block");
    registerSimpleBlock(VanillaBlocks::REDSTONE_BLOCK, "redstone_block");

    // 建筑方块
    registerSimpleBlock(VanillaBlocks::BRICKS, "bricks");
    registerSimpleBlock(VanillaBlocks::MOSSY_COBBLESTONE, "mossy_cobblestone");
    registerSimpleBlock(VanillaBlocks::BOOKSHELF, "bookshelf");
    registerSimpleBlock(VanillaBlocks::OBSIDIAN, "obsidian");

    // 木板变种
    registerSimpleBlock(VanillaBlocks::SPRUCE_PLANKS, "spruce_planks");
    registerSimpleBlock(VanillaBlocks::BIRCH_PLANKS, "birch_planks");
    registerSimpleBlock(VanillaBlocks::JUNGLE_PLANKS, "jungle_planks");
    registerSimpleBlock(VanillaBlocks::ACACIA_PLANKS, "acacia_planks");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_PLANKS, "dark_oak_planks");

    // 原木
    registerSimpleBlock(VanillaBlocks::OAK_LOG, "oak_log");
    registerSimpleBlock(VanillaBlocks::SPRUCE_LOG, "spruce_log");
    registerSimpleBlock(VanillaBlocks::BIRCH_LOG, "birch_log");
    registerSimpleBlock(VanillaBlocks::JUNGLE_LOG, "jungle_log");
    registerSimpleBlock(VanillaBlocks::ACACIA_LOG, "acacia_log");
    registerSimpleBlock(VanillaBlocks::DARK_OAK_LOG, "dark_oak_log");

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

    // 其他
    registerSimpleBlock(VanillaBlocks::SNOW, "snow");
    registerSimpleBlock(VanillaBlocks::ICE, "ice");
    registerSimpleBlock(VanillaBlocks::GLOWSTONE, "glowstone");
    registerSimpleBlock(VanillaBlocks::NETHERRACK, "netherrack");
    registerSimpleBlock(VanillaBlocks::END_STONE, "end_stone");

    // 功能方块
    registerSimpleBlock(VanillaBlocks::CRAFTING_TABLE, "crafting_table");

    // 石砖系列
    registerSimpleBlock(VanillaBlocks::STONE_BRICKS, "stone_bricks");
    registerSimpleBlock(VanillaBlocks::MOSSY_STONE_BRICKS, "mossy_stone_bricks");
    registerSimpleBlock(VanillaBlocks::CRACKED_STONE_BRICKS, "cracked_stone_bricks");
    registerSimpleBlock(VanillaBlocks::CHISELED_STONE_BRICKS, "chiseled_stone_bricks");

    // 石英系列
    registerSimpleBlock(VanillaBlocks::QUARTZ_BLOCK, "quartz_block");
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
    registerSimpleBlock(VanillaBlocks::SOUL_SAND, "soul_sand");
    registerSimpleBlock(VanillaBlocks::SOUL_SOIL, "soul_soil");
    registerSimpleBlock(VanillaBlocks::BASALT, "basalt");
    registerSimpleBlock(VanillaBlocks::POLISHED_BASALT, "polished_basalt");
    registerSimpleBlock(VanillaBlocks::BLACKSTONE, "blackstone");
    registerSimpleBlock(VanillaBlocks::POLISHED_BLACKSTONE, "polished_blackstone");
    registerSimpleBlock(VanillaBlocks::CRYING_OBSIDIAN, "crying_obsidian");
    registerSimpleBlock(VanillaBlocks::MAGMA, "magma");
    registerSimpleBlock(VanillaBlocks::NETHER_WART_BLOCK, "nether_wart_block");
    registerSimpleBlock(VanillaBlocks::CRIMSON_STEM, "crimson_stem");
    registerSimpleBlock(VanillaBlocks::WARPED_STEM, "warped_stem");
    registerSimpleBlock(VanillaBlocks::CRIMSON_NYLIUM, "crimson_nylium");
    registerSimpleBlock(VanillaBlocks::WARPED_NYLIUM, "warped_nylium");
    registerSimpleBlock(VanillaBlocks::SHROOMLIGHT, "shroomlight");

    // 自然方块扩展
    registerSimpleBlock(VanillaBlocks::CLAY, "clay");
    registerSimpleBlock(VanillaBlocks::MYCELIUM, "mycelium");
    registerSimpleBlock(VanillaBlocks::GRASS_PATH, "grass_path");
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
    registerSimpleBlock(VanillaBlocks::BAMBOO, "bamboo");

    // 下界矿石
    registerSimpleBlock(VanillaBlocks::NETHER_QUARTZ_ORE, "nether_quartz_ore");
    registerSimpleBlock(VanillaBlocks::NETHER_GOLD_ORE, "nether_gold_ore");
    registerSimpleBlock(VanillaBlocks::ANCIENT_DEBRIS, "ancient_debris");

    // 铜矿 (1.17+)
    registerSimpleBlock(VanillaBlocks::COPPER_ORE, "copper_ore");

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
    registerSimpleBlock(VanillaBlocks::LANTERN, "lantern");
    registerSimpleBlock(VanillaBlocks::SOUL_LANTERN, "soul_lantern");
    registerSimpleBlock(VanillaBlocks::CAMPFIRE, "campfire");
    registerSimpleBlock(VanillaBlocks::SOUL_CAMPFIRE, "soul_campfire");
    registerSimpleBlock(VanillaBlocks::JACK_O_LANTERN, "jack_o_lantern");
    registerSimpleBlock(VanillaBlocks::TARGET, "target");
    registerSimpleBlock(VanillaBlocks::NOTE_BLOCK, "note_block");
    registerSimpleBlock(VanillaBlocks::DRAGON_EGG, "dragon_egg");
    registerSimpleBlock(VanillaBlocks::CONDUIT, "conduit");

    // 楼梯
    registerSimpleBlock(VanillaBlocks::OAK_STAIRS, "oak_stairs");
    registerSimpleBlock(VanillaBlocks::STONE_STAIRS, "stone_stairs");
    registerSimpleBlock(VanillaBlocks::COBBLESTONE_STAIRS, "cobblestone_stairs");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_STAIRS, "prismarine_stairs");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_BRICK_STAIRS, "prismarine_brick_stairs");
    registerSimpleBlock(VanillaBlocks::DARK_PRISMARINE_STAIRS, "dark_prismarine_stairs");

    // 台阶
    registerSimpleBlock(VanillaBlocks::OAK_SLAB, "oak_slab");
    registerSimpleBlock(VanillaBlocks::STONE_SLAB, "stone_slab");
    registerSimpleBlock(VanillaBlocks::COBBLESTONE_SLAB, "cobblestone_slab");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_SLAB, "prismarine_slab");
    registerSimpleBlock(VanillaBlocks::PRISMARINE_BRICK_SLAB, "prismarine_brick_slab");
    registerSimpleBlock(VanillaBlocks::DARK_PRISMARINE_SLAB, "dark_prismarine_slab");

    // 墙
    registerSimpleBlock(VanillaBlocks::COBBLESTONE_WALL, "cobblestone_wall");
    registerSimpleBlock(VanillaBlocks::STONE_BRICK_WALL, "stone_brick_wall");

    // 栅栏
    registerSimpleBlock(VanillaBlocks::OAK_FENCE, "oak_fence");
    registerSimpleBlock(VanillaBlocks::OAK_FENCE_GATE, "oak_fence_gate");

    // 门和活板门
    registerSimpleBlock(VanillaBlocks::OAK_DOOR, "oak_door");
    registerSimpleBlock(VanillaBlocks::IRON_DOOR, "iron_door");
    registerSimpleBlock(VanillaBlocks::OAK_TRAPDOOR, "oak_trapdoor");
    registerSimpleBlock(VanillaBlocks::IRON_TRAPDOOR, "iron_trapdoor");

    // 红石方块
    registerSimpleBlock(VanillaBlocks::REDSTONE_WIRE, "redstone_wire");
    registerSimpleBlock(VanillaBlocks::REDSTONE_TORCH, "redstone_torch");
    registerSimpleBlock(VanillaBlocks::REDSTONE_LAMP, "redstone_lamp");
    registerSimpleBlock(VanillaBlocks::REDSTONE_BLOCK, "redstone_block");
    registerSimpleBlock(VanillaBlocks::REDSTONE_REPEATER, "redstone_repeater");
    registerSimpleBlock(VanillaBlocks::REDSTONE_COMPARATOR, "redstone_comparator");
    registerSimpleBlock(VanillaBlocks::OBSERVER, "observer");
    registerSimpleBlock(VanillaBlocks::LEVER, "lever");
    registerSimpleBlock(VanillaBlocks::STONE_BUTTON, "stone_button");
    registerSimpleBlock(VanillaBlocks::OAK_BUTTON, "oak_button");
    registerSimpleBlock(VanillaBlocks::STONE_PRESSURE_PLATE, "stone_pressure_plate");
    registerSimpleBlock(VanillaBlocks::OAK_PRESSURE_PLATE, "oak_pressure_plate");
    registerSimpleBlock(VanillaBlocks::LIGHT_WEIGHTED_PRESSURE_PLATE, "light_weighted_pressure_plate");
    registerSimpleBlock(VanillaBlocks::HEAVY_WEIGHTED_PRESSURE_PLATE, "heavy_weighted_pressure_plate");
    registerSimpleBlock(VanillaBlocks::DAYLIGHT_DETECTOR, "daylight_detector");
    registerSimpleBlock(VanillaBlocks::PISTON, "piston");
    registerSimpleBlock(VanillaBlocks::STICKY_PISTON, "sticky_piston");
    registerSimpleBlock(VanillaBlocks::DISPENSER, "dispenser");
    registerSimpleBlock(VanillaBlocks::DROPPER, "dropper");
    registerSimpleBlock(VanillaBlocks::TRIPWIRE, "tripwire");
    registerSimpleBlock(VanillaBlocks::TRIPWIRE_HOOK, "tripwire_hook");

    // 铁轨
    registerSimpleBlock(VanillaBlocks::RAIL, "rail");
    registerSimpleBlock(VanillaBlocks::POWERED_RAIL, "powered_rail");
    registerSimpleBlock(VanillaBlocks::DETECTOR_RAIL, "detector_rail");
    registerSimpleBlock(VanillaBlocks::ACTIVATOR_RAIL, "activator_rail");

    // 耕地
    registerSimpleBlock(VanillaBlocks::FARMLAND, "farmland");

    m_initialized = true;
    spdlog::info("Registered {} block items", m_itemToBlock.size());
}

} // namespace mc
