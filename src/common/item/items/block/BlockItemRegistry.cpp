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

    m_initialized = true;
    spdlog::info("Registered {} block items", m_itemToBlock.size());
}

} // namespace mc
