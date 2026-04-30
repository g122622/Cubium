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

    // MC 1.16.5: 锹对雪层(SNOW)和雪块(SNOW_BLOCK)总是可以采集
    // 注意：需要检查具体方块而非仅材质，因为还有其他SNOW材质的方块可能需要特殊处理
    const Block& block = state.owner();
    if (VanillaBlocks::SNOW && &block == VanillaBlocks::SNOW) {
        return true;
    }
    if (VanillaBlocks::SNOW_BLOCK && &block == VanillaBlocks::SNOW_BLOCK) {
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
    if (VanillaBlocks::FARMLAND) blocks.insert(VanillaBlocks::FARMLAND);

    // 沙子类
    if (VanillaBlocks::SAND) blocks.insert(VanillaBlocks::SAND);
    if (VanillaBlocks::RED_SAND) blocks.insert(VanillaBlocks::RED_SAND);
    if (VanillaBlocks::GRAVEL) blocks.insert(VanillaBlocks::GRAVEL);

    // 雪类
    if (VanillaBlocks::SNOW) blocks.insert(VanillaBlocks::SNOW);
    if (VanillaBlocks::SNOW_BLOCK) blocks.insert(VanillaBlocks::SNOW_BLOCK);

    // 粘土
    if (VanillaBlocks::CLAY) blocks.insert(VanillaBlocks::CLAY);

    // 灵魂沙/土
    if (VanillaBlocks::SOUL_SAND) blocks.insert(VanillaBlocks::SOUL_SAND);
    if (VanillaBlocks::SOUL_SOIL) blocks.insert(VanillaBlocks::SOUL_SOIL);

    // 混凝土粉末（16色）
    if (VanillaBlocks::WHITE_CONCRETE_POWDER) blocks.insert(VanillaBlocks::WHITE_CONCRETE_POWDER);
    if (VanillaBlocks::ORANGE_CONCRETE_POWDER) blocks.insert(VanillaBlocks::ORANGE_CONCRETE_POWDER);
    if (VanillaBlocks::MAGENTA_CONCRETE_POWDER) blocks.insert(VanillaBlocks::MAGENTA_CONCRETE_POWDER);
    if (VanillaBlocks::LIGHT_BLUE_CONCRETE_POWDER) blocks.insert(VanillaBlocks::LIGHT_BLUE_CONCRETE_POWDER);
    if (VanillaBlocks::YELLOW_CONCRETE_POWDER) blocks.insert(VanillaBlocks::YELLOW_CONCRETE_POWDER);
    if (VanillaBlocks::LIME_CONCRETE_POWDER) blocks.insert(VanillaBlocks::LIME_CONCRETE_POWDER);
    if (VanillaBlocks::PINK_CONCRETE_POWDER) blocks.insert(VanillaBlocks::PINK_CONCRETE_POWDER);
    if (VanillaBlocks::GRAY_CONCRETE_POWDER) blocks.insert(VanillaBlocks::GRAY_CONCRETE_POWDER);
    if (VanillaBlocks::LIGHT_GRAY_CONCRETE_POWDER) blocks.insert(VanillaBlocks::LIGHT_GRAY_CONCRETE_POWDER);
    if (VanillaBlocks::CYAN_CONCRETE_POWDER) blocks.insert(VanillaBlocks::CYAN_CONCRETE_POWDER);
    if (VanillaBlocks::PURPLE_CONCRETE_POWDER) blocks.insert(VanillaBlocks::PURPLE_CONCRETE_POWDER);
    if (VanillaBlocks::BLUE_CONCRETE_POWDER) blocks.insert(VanillaBlocks::BLUE_CONCRETE_POWDER);
    if (VanillaBlocks::BROWN_CONCRETE_POWDER) blocks.insert(VanillaBlocks::BROWN_CONCRETE_POWDER);
    if (VanillaBlocks::GREEN_CONCRETE_POWDER) blocks.insert(VanillaBlocks::GREEN_CONCRETE_POWDER);
    if (VanillaBlocks::RED_CONCRETE_POWDER) blocks.insert(VanillaBlocks::RED_CONCRETE_POWDER);
    if (VanillaBlocks::BLACK_CONCRETE_POWDER) blocks.insert(VanillaBlocks::BLACK_CONCRETE_POWDER);

    return blocks;
}

} // namespace tool
} // namespace item
} // namespace mc
