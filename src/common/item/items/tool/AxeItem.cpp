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
    // MC 1.16.5: 斧对木材、下界木材、植物、高植物、葫芦、竹子材质有高效率
    return material == Material::WOOD ||
           material == Material::NETHER_WOOD ||
           material == Material::PLANT ||
           material == Material::REPLACEABLE_PLANT ||
           material == Material::TALL_PLANTS ||
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

    // 木头（六面树皮）
    if (VanillaBlocks::OAK_WOOD) blocks.insert(VanillaBlocks::OAK_WOOD);
    if (VanillaBlocks::SPRUCE_WOOD) blocks.insert(VanillaBlocks::SPRUCE_WOOD);
    if (VanillaBlocks::BIRCH_WOOD) blocks.insert(VanillaBlocks::BIRCH_WOOD);
    if (VanillaBlocks::JUNGLE_WOOD) blocks.insert(VanillaBlocks::JUNGLE_WOOD);
    if (VanillaBlocks::ACACIA_WOOD) blocks.insert(VanillaBlocks::ACACIA_WOOD);
    if (VanillaBlocks::DARK_OAK_WOOD) blocks.insert(VanillaBlocks::DARK_OAK_WOOD);

    // 去皮原木
    if (VanillaBlocks::STRIPPED_OAK_LOG) blocks.insert(VanillaBlocks::STRIPPED_OAK_LOG);
    if (VanillaBlocks::STRIPPED_SPRUCE_LOG) blocks.insert(VanillaBlocks::STRIPPED_SPRUCE_LOG);
    if (VanillaBlocks::STRIPPED_BIRCH_LOG) blocks.insert(VanillaBlocks::STRIPPED_BIRCH_LOG);
    if (VanillaBlocks::STRIPPED_JUNGLE_LOG) blocks.insert(VanillaBlocks::STRIPPED_JUNGLE_LOG);
    if (VanillaBlocks::STRIPPED_ACACIA_LOG) blocks.insert(VanillaBlocks::STRIPPED_ACACIA_LOG);
    if (VanillaBlocks::STRIPPED_DARK_OAK_LOG) blocks.insert(VanillaBlocks::STRIPPED_DARK_OAK_LOG);

    // 去皮木头
    if (VanillaBlocks::STRIPPED_OAK_WOOD) blocks.insert(VanillaBlocks::STRIPPED_OAK_WOOD);
    if (VanillaBlocks::STRIPPED_SPRUCE_WOOD) blocks.insert(VanillaBlocks::STRIPPED_SPRUCE_WOOD);
    if (VanillaBlocks::STRIPPED_BIRCH_WOOD) blocks.insert(VanillaBlocks::STRIPPED_BIRCH_WOOD);
    if (VanillaBlocks::STRIPPED_JUNGLE_WOOD) blocks.insert(VanillaBlocks::STRIPPED_JUNGLE_WOOD);
    if (VanillaBlocks::STRIPPED_ACACIA_WOOD) blocks.insert(VanillaBlocks::STRIPPED_ACACIA_WOOD);
    if (VanillaBlocks::STRIPPED_DARK_OAK_WOOD) blocks.insert(VanillaBlocks::STRIPPED_DARK_OAK_WOOD);

    // 树叶
    if (VanillaBlocks::OAK_LEAVES) blocks.insert(VanillaBlocks::OAK_LEAVES);
    if (VanillaBlocks::SPRUCE_LEAVES) blocks.insert(VanillaBlocks::SPRUCE_LEAVES);
    if (VanillaBlocks::BIRCH_LEAVES) blocks.insert(VanillaBlocks::BIRCH_LEAVES);
    if (VanillaBlocks::JUNGLE_LEAVES) blocks.insert(VanillaBlocks::JUNGLE_LEAVES);
    if (VanillaBlocks::ACACIA_LEAVES) blocks.insert(VanillaBlocks::ACACIA_LEAVES);
    if (VanillaBlocks::DARK_OAK_LEAVES) blocks.insert(VanillaBlocks::DARK_OAK_LEAVES);

    // 木制工作台
    if (VanillaBlocks::CRAFTING_TABLE) blocks.insert(VanillaBlocks::CRAFTING_TABLE);
    if (VanillaBlocks::BOOKSHELF) blocks.insert(VanillaBlocks::BOOKSHELF);

    // 木门
    if (VanillaBlocks::OAK_DOOR) blocks.insert(VanillaBlocks::OAK_DOOR);
    // TODO: 其他木头门

    // 木栅栏门
    if (VanillaBlocks::OAK_FENCE_GATE) blocks.insert(VanillaBlocks::OAK_FENCE_GATE);
    // TODO: 其他木头栅栏门

    // 木栅栏
    if (VanillaBlocks::OAK_FENCE) blocks.insert(VanillaBlocks::OAK_FENCE);
    // TODO: 其他木头栅栏

    // 木楼梯
    if (VanillaBlocks::OAK_STAIRS) blocks.insert(VanillaBlocks::OAK_STAIRS);
    // TODO: 其他木头楼梯

    // 木台阶
    if (VanillaBlocks::OAK_SLAB) blocks.insert(VanillaBlocks::OAK_SLAB);
    // TODO: 其他木头台阶

    // 木按钮
    if (VanillaBlocks::OAK_BUTTON) blocks.insert(VanillaBlocks::OAK_BUTTON);
    // TODO: 其他木头按钮

    // 木压力板
    if (VanillaBlocks::OAK_PRESSURE_PLATE) blocks.insert(VanillaBlocks::OAK_PRESSURE_PLATE);
    // TODO: 其他木头压力板

    // 木活板门
    if (VanillaBlocks::OAK_TRAPDOOR) blocks.insert(VanillaBlocks::OAK_TRAPDOOR);
    // TODO: 其他木头活板门

    // 下界木质方块
    if (VanillaBlocks::CRIMSON_STEM) blocks.insert(VanillaBlocks::CRIMSON_STEM);
    if (VanillaBlocks::WARPED_STEM) blocks.insert(VanillaBlocks::WARPED_STEM);
    // TODO: 绯红/诡异菌核、去皮菌柄、绯红/诡异木板（尚未注册）

    return blocks;
}

} // namespace tool
} // namespace item
} // namespace mc
