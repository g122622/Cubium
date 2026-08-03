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

#include "AxeItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/HoneycombItem.hpp"
#include "common/item/items/tool/ToolItem.hpp"
#include "common/item/items/tool/ToolType.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/copper/IOxidizableBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mc {
namespace item {
namespace tool {

AxeItem::AxeItem(const tier::IItemTier& tier, f32 attackDamage, f32 attackSpeed, ItemProperties properties)
    : ToolItem(attackDamage, attackSpeed, tier, _initializeEffectiveBlocks(), ToolType::Axe, properties)
{
    // 映射表使用"construct on first use"模式，无需在此初始化
}

ActionResultType AxeItem::onItemUse(ItemUseContext& context)
{
    IWorld& world = context.world();
    const BlockPos& pos = context.blockPos();
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return ActionResultType::Pass;
    }

    // MC Java AxeItem 交互顺序：1.去皮 → 2.去氧化(刮削) → 3.除蜡
    // 每个步骤独立检查，只有第一个匹配的步骤被执行（fallthrough if-empty 模式）

    // 1. 检查是否可去皮（原木 → 去皮原木）
    const Block* strippedBlock = getStrippedBlock(&state->owner());
    if (strippedBlock != nullptr) {
        const BlockState& newState = strippedBlock->getDefaultState().withPropertiesOf(*state);

        if (context.getPlayer() != nullptr) {
            context.getPlayer()->playSound(SoundEvents::ITEM_AXE_STRIP, 1.0f, 1.0f);
        }

        world.setBlockState(pos, &newState, 11);
        ItemStack& stack = context.getItemStackMut();
        LivingEntity::hurtAndBreak(stack, 1, context.getPlayer(), EquipmentSlot::MainHand);
        return ActionResultType::Success;
    }

    // 2. 检查是否可去氧化（刮削铜方块，如 Exposed → Unaffected）
    const Block& block = state->getBlock();
    const auto* oxidizable = dynamic_cast<const blocks::IOxidizableBlock*>(&block);
    if (oxidizable != nullptr) {
        Block* previousBlock = oxidizable->getPreviousOxidationBlock();
        if (previousBlock != nullptr) {
            // 使用 withPropertiesOf 保留共有属性（楼梯朝向、台阶类型、含水状态等）
            const BlockState& newState = previousBlock->defaultState().withPropertiesOf(*state);

            world.setBlockState(pos, &newState, 11);
            // 去氧化播放 SCRAPE 粒子效果（worldEvent 3005 已包含音效和粒子）
            world.playEvent(world::WorldEvents::SCRAPE, pos, 0);
            ItemStack& stack = context.getItemStackMut();
            LivingEntity::hurtAndBreak(stack, 1, context.getPlayer(), EquipmentSlot::MainHand);
            return ActionResultType::Success;
        }
    }

    // 3. 检查是否可除蜡（涂蜡铜方块 → 未涂蜡铜方块）
    auto waxedOffState = item::items::HoneycombItem::getWaxedOff(*state);
    if (waxedOffState.has_value()) {
        // 除蜡：播放 WAX_OFF 世界事件（包含音效+粒子），无需单独调用 playSound
        world.setBlockState(pos, &waxedOffState.value(), 11);
        world.playEvent(world::WorldEvents::WAX_OFF, pos, 0);
        ItemStack& stack = context.getItemStackMut();
        LivingEntity::hurtAndBreak(stack, 1, context.getPlayer(), EquipmentSlot::MainHand);
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

const Block* AxeItem::getStrippedBlock(const Block* original)
{
    if (original == nullptr) {
        return nullptr;
    }
    auto& map = _getStrippingMap();
    auto it = map.find(original);
    if (it != map.end()) {
        return it->second;
    }
    return nullptr;
}

f32 AxeItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
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

bool AxeItem::isEffectiveMaterial(const Material& material) const
{
    // 斧对木材、下界木材、植物、高植物、葫芦、竹子材质有高效率
    return material == Material::WOOD || material == Material::NETHER_WOOD || material == Material::PLANT ||
        material == Material::REPLACEABLE_PLANT || material == Material::TALL_PLANTS || material == Material::GOURD ||
        material == Material::BAMBOO;
}

std::unordered_set<const Block*> AxeItem::_initializeEffectiveBlocks()
{
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
    if (VanillaBlocks::STRIPPED_CRIMSON_STEM) blocks.insert(VanillaBlocks::STRIPPED_CRIMSON_STEM);
    if (VanillaBlocks::STRIPPED_WARPED_STEM) blocks.insert(VanillaBlocks::STRIPPED_WARPED_STEM);
    if (VanillaBlocks::CRIMSON_HYPHAE) blocks.insert(VanillaBlocks::CRIMSON_HYPHAE);
    if (VanillaBlocks::WARPED_HYPHAE) blocks.insert(VanillaBlocks::WARPED_HYPHAE);
    if (VanillaBlocks::STRIPPED_CRIMSON_HYPHAE) blocks.insert(VanillaBlocks::STRIPPED_CRIMSON_HYPHAE);
    if (VanillaBlocks::STRIPPED_WARPED_HYPHAE) blocks.insert(VanillaBlocks::STRIPPED_WARPED_HYPHAE);

    // 樱花木质方块 (1.20+)
    if (VanillaBlocks::CHERRY_LOG) blocks.insert(VanillaBlocks::CHERRY_LOG);
    if (VanillaBlocks::CHERRY_WOOD) blocks.insert(VanillaBlocks::CHERRY_WOOD);
    if (VanillaBlocks::STRIPPED_CHERRY_LOG) blocks.insert(VanillaBlocks::STRIPPED_CHERRY_LOG);
    if (VanillaBlocks::STRIPPED_CHERRY_WOOD) blocks.insert(VanillaBlocks::STRIPPED_CHERRY_WOOD);
    if (VanillaBlocks::CHERRY_PLANKS) blocks.insert(VanillaBlocks::CHERRY_PLANKS);
    if (VanillaBlocks::CHERRY_LEAVES) blocks.insert(VanillaBlocks::CHERRY_LEAVES);

    // 红树木质方块 (1.19+)
    if (VanillaBlocks::MANGROVE_LOG) blocks.insert(VanillaBlocks::MANGROVE_LOG);
    if (VanillaBlocks::MANGROVE_WOOD) blocks.insert(VanillaBlocks::MANGROVE_WOOD);
    if (VanillaBlocks::STRIPPED_MANGROVE_LOG) blocks.insert(VanillaBlocks::STRIPPED_MANGROVE_LOG);
    if (VanillaBlocks::STRIPPED_MANGROVE_WOOD) blocks.insert(VanillaBlocks::STRIPPED_MANGROVE_WOOD);
    if (VanillaBlocks::MANGROVE_PLANKS) blocks.insert(VanillaBlocks::MANGROVE_PLANKS);
    if (VanillaBlocks::MANGROVE_LEAVES) blocks.insert(VanillaBlocks::MANGROVE_LEAVES);

    // 苍白橡木质方块 (1.21+)
    if (VanillaBlocks::PALE_OAK_LOG) blocks.insert(VanillaBlocks::PALE_OAK_LOG);
    if (VanillaBlocks::PALE_OAK_WOOD) blocks.insert(VanillaBlocks::PALE_OAK_WOOD);
    if (VanillaBlocks::STRIPPED_PALE_OAK_LOG) blocks.insert(VanillaBlocks::STRIPPED_PALE_OAK_LOG);
    if (VanillaBlocks::STRIPPED_PALE_OAK_WOOD) blocks.insert(VanillaBlocks::STRIPPED_PALE_OAK_WOOD);
    if (VanillaBlocks::PALE_OAK_PLANKS) blocks.insert(VanillaBlocks::PALE_OAK_PLANKS);
    if (VanillaBlocks::PALE_OAK_LEAVES) blocks.insert(VanillaBlocks::PALE_OAK_LEAVES);

    // 梯子和脚手架（斧可以有效挖掘）
    if (VanillaBlocks::LADDER) blocks.insert(VanillaBlocks::LADDER);
    if (VanillaBlocks::SCAFFOLDING) blocks.insert(VanillaBlocks::SCAFFOLDING);

    // 木质按钮（斧可以有效挖掘）
    if (VanillaBlocks::OAK_BUTTON) blocks.insert(VanillaBlocks::OAK_BUTTON);
    if (VanillaBlocks::SPRUCE_BUTTON) blocks.insert(VanillaBlocks::SPRUCE_BUTTON);
    if (VanillaBlocks::BIRCH_BUTTON) blocks.insert(VanillaBlocks::BIRCH_BUTTON);
    if (VanillaBlocks::JUNGLE_BUTTON) blocks.insert(VanillaBlocks::JUNGLE_BUTTON);
    if (VanillaBlocks::ACACIA_BUTTON) blocks.insert(VanillaBlocks::ACACIA_BUTTON);
    if (VanillaBlocks::DARK_OAK_BUTTON) blocks.insert(VanillaBlocks::DARK_OAK_BUTTON);
    if (VanillaBlocks::CRIMSON_BUTTON) blocks.insert(VanillaBlocks::CRIMSON_BUTTON);
    if (VanillaBlocks::WARPED_BUTTON) blocks.insert(VanillaBlocks::WARPED_BUTTON);

    return blocks;
}

std::unordered_map<const Block*, const Block*>& AxeItem::_getStrippingMap()
{
    // "construct on first use" 模式：函数局部静态变量在第一次调用时初始化
    static std::unordered_map<const Block*, const Block*> map = []() {
        std::unordered_map<const Block*, const Block*> m;

        // 原木去皮映射
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

        // 绯红菌柄
        if (VanillaBlocks::CRIMSON_STEM && VanillaBlocks::STRIPPED_CRIMSON_STEM)
            m[VanillaBlocks::CRIMSON_STEM] = VanillaBlocks::STRIPPED_CRIMSON_STEM;
        if (VanillaBlocks::CRIMSON_HYPHAE && VanillaBlocks::STRIPPED_CRIMSON_HYPHAE)
            m[VanillaBlocks::CRIMSON_HYPHAE] = VanillaBlocks::STRIPPED_CRIMSON_HYPHAE;

        // 诡异菌柄
        if (VanillaBlocks::WARPED_STEM && VanillaBlocks::STRIPPED_WARPED_STEM)
            m[VanillaBlocks::WARPED_STEM] = VanillaBlocks::STRIPPED_WARPED_STEM;
        if (VanillaBlocks::WARPED_HYPHAE && VanillaBlocks::STRIPPED_WARPED_HYPHAE)
            m[VanillaBlocks::WARPED_HYPHAE] = VanillaBlocks::STRIPPED_WARPED_HYPHAE;

        // 樱花木 (1.20+)
        if (VanillaBlocks::CHERRY_LOG && VanillaBlocks::STRIPPED_CHERRY_LOG)
            m[VanillaBlocks::CHERRY_LOG] = VanillaBlocks::STRIPPED_CHERRY_LOG;
        if (VanillaBlocks::CHERRY_WOOD && VanillaBlocks::STRIPPED_CHERRY_WOOD)
            m[VanillaBlocks::CHERRY_WOOD] = VanillaBlocks::STRIPPED_CHERRY_WOOD;

        // 红树木 (1.19+)
        if (VanillaBlocks::MANGROVE_LOG && VanillaBlocks::STRIPPED_MANGROVE_LOG)
            m[VanillaBlocks::MANGROVE_LOG] = VanillaBlocks::STRIPPED_MANGROVE_LOG;
        if (VanillaBlocks::MANGROVE_WOOD && VanillaBlocks::STRIPPED_MANGROVE_WOOD)
            m[VanillaBlocks::MANGROVE_WOOD] = VanillaBlocks::STRIPPED_MANGROVE_WOOD;

        // 苍白橡木 (1.21+)
        if (VanillaBlocks::PALE_OAK_LOG && VanillaBlocks::STRIPPED_PALE_OAK_LOG)
            m[VanillaBlocks::PALE_OAK_LOG] = VanillaBlocks::STRIPPED_PALE_OAK_LOG;
        if (VanillaBlocks::PALE_OAK_WOOD && VanillaBlocks::STRIPPED_PALE_OAK_WOOD)
            m[VanillaBlocks::PALE_OAK_WOOD] = VanillaBlocks::STRIPPED_PALE_OAK_WOOD;

        return m;
    }();
    return map;
}

} // namespace tool
} // namespace item
} // namespace mc
