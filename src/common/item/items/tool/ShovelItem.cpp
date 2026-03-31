#include "ShovelItem.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/Block.hpp"

namespace mc {
namespace item {
namespace tool {

ShovelItem::ShovelItem(const tier::IItemTier& tier,
                       f32 attackDamage,
                       f32 attackSpeed,
                       ItemProperties properties)
    : ToolItem(attackDamage,
               attackSpeed,
               tier,
               initializeEffectiveBlocks(),
               ToolType::Shovel,
               properties) {
}

bool ShovelItem::canHarvestBlock(const BlockState& state) const {
    // 1. 如果方块需要锹，检查挖掘等级
    if (state.getHarvestTool() == TOOL_TYPE_SHOVEL) {
        return m_tier.getHarvestLevel() >= state.getHarvestLevel();
    }

    // 锹对雪类方块总是可以采集
    // 注意：雪层可能需要特殊处理（多层雪）
    const Material& mat = state.getMaterial();
    if (mat == Material::SNOW) {
        return true;
    }

    return false;
}

f32 ShovelItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    // 锹对泥土、沙子、雪材质有高效率
    if (isEffectiveMaterial(state.getMaterial())) {
        return m_efficiency;
    }

    // 检查特定方块
    if (isEffectiveBlock(state.owner())) {
        return m_efficiency;
    }

    return 1.0f;
}

bool ShovelItem::isEffectiveMaterial(const Material& material) const {
    return material == Material::EARTH ||
           material == Material::SAND ||
           material == Material::SNOW;
}

std::unordered_set<const Block*> ShovelItem::initializeEffectiveBlocks() {
    std::unordered_set<const Block*> blocks;

    // 泥土类
    if (VanillaBlocks::DIRT) blocks.insert(VanillaBlocks::DIRT);
    if (VanillaBlocks::GRASS_BLOCK) blocks.insert(VanillaBlocks::GRASS_BLOCK);
    if (VanillaBlocks::COARSE_DIRT) blocks.insert(VanillaBlocks::COARSE_DIRT);
    if (VanillaBlocks::PODZOL) blocks.insert(VanillaBlocks::PODZOL);
    if (VanillaBlocks::GRASS_PATH) blocks.insert(VanillaBlocks::GRASS_PATH);
    if (VanillaBlocks::MYCELIUM) blocks.insert(VanillaBlocks::MYCELIUM);

    // 沙子类
    if (VanillaBlocks::SAND) blocks.insert(VanillaBlocks::SAND);
    if (VanillaBlocks::GRAVEL) blocks.insert(VanillaBlocks::GRAVEL);

    // 雪类
    if (VanillaBlocks::SNOW) blocks.insert(VanillaBlocks::SNOW);

    // 粘土
    if (VanillaBlocks::CLAY) blocks.insert(VanillaBlocks::CLAY);

    // 灵魂沙/土
    if (VanillaBlocks::SOUL_SAND) blocks.insert(VanillaBlocks::SOUL_SAND);
    if (VanillaBlocks::SOUL_SOIL) blocks.insert(VanillaBlocks::SOUL_SOIL);

    return blocks;
}

} // namespace tool
} // namespace item
} // namespace mc
