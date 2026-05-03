#include "AxeItem.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../item/context/ItemUseContext.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../sound/SoundCategory.hpp"

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
    // 映射表使用"construct on first use"模式，无需在此初始化
}

ActionResultType AxeItem::onItemUse(ItemUseContext& context) {
    // MC 1.16.5: 斧头去皮逻辑
    IWorld& world = context.world();
    const BlockPos& pos = context.blockPos();
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查是否可去皮
    const Block* strippedBlock = getStrippedBlock(&state->owner());
    if (strippedBlock == nullptr) {
        return ActionResultType::Pass;
    }

    // 获取新方块状态
    const BlockState& newState = strippedBlock->getDefaultState();

    // 播放去皮音效
    if (context.getPlayer() != nullptr) {
        context.getPlayer()->playSound(SoundEvents::ITEM_AXE_STRIP, 1.0f, 1.0f);
    }

    // 设置新方块状态（MC 1.16.5 flags: 11 = 同步到客户端+更新邻居）
    world.setBlockState(pos, &newState, 11);

    // 消耗耐久度
    ItemStack& stack = context.getItemStackMut();
    stack.attemptDamageItem(1);

    return ActionResultType::Success;
}

const Block* AxeItem::getStrippedBlock(const Block* original) {
    if (original == nullptr) {
        return nullptr;
    }
    auto& map = getStrippingMap();
    auto it = map.find(original);
    if (it != map.end()) {
        return it->second;
    }
    return nullptr;
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

    // 木栅栏门
    if (VanillaBlocks::OAK_FENCE_GATE) blocks.insert(VanillaBlocks::OAK_FENCE_GATE);

    // 木栅栏
    if (VanillaBlocks::OAK_FENCE) blocks.insert(VanillaBlocks::OAK_FENCE);

    // 木楼梯
    if (VanillaBlocks::OAK_STAIRS) blocks.insert(VanillaBlocks::OAK_STAIRS);

    // 木台阶
    if (VanillaBlocks::OAK_SLAB) blocks.insert(VanillaBlocks::OAK_SLAB);

    // 木按钮
    if (VanillaBlocks::OAK_BUTTON) blocks.insert(VanillaBlocks::OAK_BUTTON);

    // 木压力板
    if (VanillaBlocks::OAK_PRESSURE_PLATE) blocks.insert(VanillaBlocks::OAK_PRESSURE_PLATE);

    // 木活板门
    if (VanillaBlocks::OAK_TRAPDOOR) blocks.insert(VanillaBlocks::OAK_TRAPDOOR);

    // 下界木质方块
    if (VanillaBlocks::CRIMSON_STEM) blocks.insert(VanillaBlocks::CRIMSON_STEM);
    if (VanillaBlocks::WARPED_STEM) blocks.insert(VanillaBlocks::WARPED_STEM);

    return blocks;
}

std::unordered_map<const Block*, const Block*>& AxeItem::getStrippingMap() {
    // "construct on first use" 模式：函数局部静态变量在第一次调用时初始化
    static std::unordered_map<const Block*, const Block*> map = []() {
        std::unordered_map<const Block*, const Block*> m;

        // MC 1.16.5: 原木去皮映射
        // 橡木
        if (VanillaBlocks::OAK_LOG && VanillaBlocks::STRIPPED_OAK_LOG)
            m[VanillaBlocks::OAK_LOG] = VanillaBlocks::STRIPPED_OAK_LOG;
        if (VanillaBlocks::OAK_WOOD && VanillaBlocks::STRIPPED_OAK_WOOD)
            m[VanillaBlocks::OAK_WOOD] = VanillaBlocks::STRIPPED_OAK_WOOD;

        // 云杉木
        if (VanillaBlocks::SPRUCE_LOG && VanillaBlocks::STRIPPED_SPRUCE_LOG)
            m[VanillaBlocks::SPRUCE_LOG] = VanillaBlocks::STRIPPED_SPRUCE_LOG;
        if (VanillaBlocks::SPRUCE_WOOD && VanillaBlocks::STRIPPED_SPRUCE_WOOD)
            m[VanillaBlocks::SPRUCE_WOOD] = VanillaBlocks::STRIPPED_SPRUCE_WOOD;

        // 白桦木
        if (VanillaBlocks::BIRCH_LOG && VanillaBlocks::STRIPPED_BIRCH_LOG)
            m[VanillaBlocks::BIRCH_LOG] = VanillaBlocks::STRIPPED_BIRCH_LOG;
        if (VanillaBlocks::BIRCH_WOOD && VanillaBlocks::STRIPPED_BIRCH_WOOD)
            m[VanillaBlocks::BIRCH_WOOD] = VanillaBlocks::STRIPPED_BIRCH_WOOD;

        // 丛林木
        if (VanillaBlocks::JUNGLE_LOG && VanillaBlocks::STRIPPED_JUNGLE_LOG)
            m[VanillaBlocks::JUNGLE_LOG] = VanillaBlocks::STRIPPED_JUNGLE_LOG;
        if (VanillaBlocks::JUNGLE_WOOD && VanillaBlocks::STRIPPED_JUNGLE_WOOD)
            m[VanillaBlocks::JUNGLE_WOOD] = VanillaBlocks::STRIPPED_JUNGLE_WOOD;

        // 金合欢木
        if (VanillaBlocks::ACACIA_LOG && VanillaBlocks::STRIPPED_ACACIA_LOG)
            m[VanillaBlocks::ACACIA_LOG] = VanillaBlocks::STRIPPED_ACACIA_LOG;
        if (VanillaBlocks::ACACIA_WOOD && VanillaBlocks::STRIPPED_ACACIA_WOOD)
            m[VanillaBlocks::ACACIA_WOOD] = VanillaBlocks::STRIPPED_ACACIA_WOOD;

        // 深色橡木
        if (VanillaBlocks::DARK_OAK_LOG && VanillaBlocks::STRIPPED_DARK_OAK_LOG)
            m[VanillaBlocks::DARK_OAK_LOG] = VanillaBlocks::STRIPPED_DARK_OAK_LOG;
        if (VanillaBlocks::DARK_OAK_WOOD && VanillaBlocks::STRIPPED_DARK_OAK_WOOD)
            m[VanillaBlocks::DARK_OAK_WOOD] = VanillaBlocks::STRIPPED_DARK_OAK_WOOD;

        // 绯红菌柄 - 注意：去皮绯红/诡异菌柄尚未注册
        // if (VanillaBlocks::CRIMSON_STEM && VanillaBlocks::STRIPPED_CRIMSON_STEM)
        //     m[VanillaBlocks::CRIMSON_STEM] = VanillaBlocks::STRIPPED_CRIMSON_STEM;

        // 诡异菌柄 - 注意：去皮绯红/诡异菌柄尚未注册
        // if (VanillaBlocks::WARPED_STEM && VanillaBlocks::STRIPPED_WARPED_STEM)
        //     m[VanillaBlocks::WARPED_STEM] = VanillaBlocks::STRIPPED_WARPED_STEM;

        return m;
    }();
    return map;
}

} // namespace tool
} // namespace item
} // namespace mc
