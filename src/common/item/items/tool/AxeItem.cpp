#include "AxeItem.hpp"
#include "../../../world/block/VanillaBlocks.hpp"

namespace mc {
namespace item {
namespace tool {

AxeItem::AxeItem(const tier::IItemTier& tier,
                 f32 attackDamage,
                 f32 attackSpeed,
                 ItemProperties properties)
    : ToolItem(attackDamage,
               attackSpeed,
               tier,
               initializeEffectiveBlocks(),
               ToolType::Axe,
               properties) {
}

f32 AxeItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    // 斧对木材、植物、葫芦、竹子材质有高效率
    if (isEffectiveMaterial(state.getMaterial())) {
        return m_efficiency;
    }

    // 检查特定方块
    if (isEffectiveBlock(state.owner())) {
        return m_efficiency;
    }

    return 1.0f;
}

bool AxeItem::isEffectiveMaterial(const Material& material) const {
    return material == Material::WOOD ||
           material == Material::NETHER_WOOD ||
           material == Material::PLANT ||
           material == Material::GOURD ||
           material == Material::BAMBOO;
}

std::unordered_set<const Block*> AxeItem::initializeEffectiveBlocks() {
    std::unordered_set<const Block*> blocks;

    // 木板
    if (VanillaBlocks::OAK_PLANKS) blocks.insert(VanillaBlocks::OAK_PLANKS);
    if (VanillaBlocks::SPRUCE_PLANKS) blocks.insert(VanillaBlocks::SPRUCE_PLANKS);
    if (VanillaBlocks::BIRCH_PLANKS) blocks.insert(VanillaBlocks::BIRCH_PLANKS);
    if (VanillaBlocks::JUNGLE_PLANKS) blocks.insert(VanillaBlocks::JUNGLE_PLANKS);
    if (VanillaBlocks::ACACIA_PLANKS) blocks.insert(VanillaBlocks::ACACIA_PLANKS);
    if (VanillaBlocks::DARK_OAK_PLANKS) blocks.insert(VanillaBlocks::DARK_OAK_PLANKS);

    // 原木
    if (VanillaBlocks::OAK_LOG) blocks.insert(VanillaBlocks::OAK_LOG);
    if (VanillaBlocks::SPRUCE_LOG) blocks.insert(VanillaBlocks::SPRUCE_LOG);
    if (VanillaBlocks::BIRCH_LOG) blocks.insert(VanillaBlocks::BIRCH_LOG);
    if (VanillaBlocks::JUNGLE_LOG) blocks.insert(VanillaBlocks::JUNGLE_LOG);
    if (VanillaBlocks::ACACIA_LOG) blocks.insert(VanillaBlocks::ACACIA_LOG);
    if (VanillaBlocks::DARK_OAK_LOG) blocks.insert(VanillaBlocks::DARK_OAK_LOG);

    // 树叶
    if (VanillaBlocks::OAK_LEAVES) blocks.insert(VanillaBlocks::OAK_LEAVES);
    if (VanillaBlocks::SPRUCE_LEAVES) blocks.insert(VanillaBlocks::SPRUCE_LEAVES);
    if (VanillaBlocks::BIRCH_LEAVES) blocks.insert(VanillaBlocks::BIRCH_LEAVES);
    if (VanillaBlocks::JUNGLE_LEAVES) blocks.insert(VanillaBlocks::JUNGLE_LEAVES);
    if (VanillaBlocks::ACACIA_LEAVES) blocks.insert(VanillaBlocks::ACACIA_LEAVES);
    if (VanillaBlocks::DARK_OAK_LEAVES) blocks.insert(VanillaBlocks::DARK_OAK_LEAVES);

    // 其他木制方块
    if (VanillaBlocks::BOOKSHELF) blocks.insert(VanillaBlocks::BOOKSHELF);
    if (VanillaBlocks::CRAFTING_TABLE) blocks.insert(VanillaBlocks::CRAFTING_TABLE);

    return blocks;
}

} // namespace tool
} // namespace item
} // namespace mc
