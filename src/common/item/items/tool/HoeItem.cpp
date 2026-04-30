#include "HoeItem.hpp"
#include "../../../world/block/VanillaBlocks.hpp"

namespace mc {
namespace item {
namespace tool {

HoeItem::HoeItem(const tier::IItemTier& tier,
                 i32 attackDamage,
                 f32 attackSpeed,
                 ItemProperties properties)
    : ToolItem(static_cast<f32>(attackDamage),
               attackSpeed,
               tier,
               initializeEffectiveBlocks(),
               ToolType::Hoe,
               properties) {
}

f32 HoeItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    // 锄对树叶材质有效
    if (state.getMaterial() == Material::LEAVES) {
        return m_efficiency;
    }

    // 检查特定方块
    if (isEffectiveBlock(state.owner())) {
        return m_efficiency;
    }

    return 1.0f;
}

bool HoeItem::isEffectiveMaterial(const Material& material) const {
    // 锄对树叶材质有效
    return material == Material::LEAVES;
}

std::unordered_set<const Block*> HoeItem::initializeEffectiveBlocks() {
    std::unordered_set<const Block*> blocks;

    // 干草块
    if (VanillaBlocks::HAY_BLOCK) blocks.insert(VanillaBlocks::HAY_BLOCK);

    // 海绵
    if (VanillaBlocks::SPONGE) blocks.insert(VanillaBlocks::SPONGE);
    if (VanillaBlocks::WET_SPONGE) blocks.insert(VanillaBlocks::WET_SPONGE);

    // 树叶
    if (VanillaBlocks::OAK_LEAVES) blocks.insert(VanillaBlocks::OAK_LEAVES);
    if (VanillaBlocks::SPRUCE_LEAVES) blocks.insert(VanillaBlocks::SPRUCE_LEAVES);
    if (VanillaBlocks::BIRCH_LEAVES) blocks.insert(VanillaBlocks::BIRCH_LEAVES);
    if (VanillaBlocks::JUNGLE_LEAVES) blocks.insert(VanillaBlocks::JUNGLE_LEAVES);
    if (VanillaBlocks::ACACIA_LEAVES) blocks.insert(VanillaBlocks::ACACIA_LEAVES);
    if (VanillaBlocks::DARK_OAK_LEAVES) blocks.insert(VanillaBlocks::DARK_OAK_LEAVES);

    // 地狱疣块
    if (VanillaBlocks::NETHER_WART_BLOCK) blocks.insert(VanillaBlocks::NETHER_WART_BLOCK);

    // 干海带块
    if (VanillaBlocks::DRIED_KELP_BLOCK) blocks.insert(VanillaBlocks::DRIED_KELP_BLOCK);

    // 标靶方块
    if (VanillaBlocks::TARGET) blocks.insert(VanillaBlocks::TARGET);

    // 荧光块
    if (VanillaBlocks::SHROOMLIGHT) blocks.insert(VanillaBlocks::SHROOMLIGHT);

    return blocks;
}

} // namespace tool
} // namespace item
} // namespace mc
