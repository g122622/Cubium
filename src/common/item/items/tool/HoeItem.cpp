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

#include "HoeItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../item/context/ItemUseContext.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/Direction.hpp"
#include "../../../world/IWorld.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/tool/ToolItem.hpp"
#include "common/item/items/tool/ToolType.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mc {
namespace item {
namespace tool {

HoeItem::HoeItem(const tier::IItemTier& tier, i32 attackDamage, f32 attackSpeed, ItemProperties properties)
    : ToolItem(
          static_cast<f32>(attackDamage), attackSpeed, tier, _initializeEffectiveBlocks(), ToolType::Hoe, properties)
{
    // 映射表使用"construct on first use"模式，无需在此初始化
}

ActionResultType HoeItem::onItemUse(ItemUseContext& context)
{
    // 锄耕地逻辑
    IWorld& world = context.world();
    const BlockPos& pos = context.blockPos();

    // 检查点击的面是否为底面（不能从下方耕地）
    if (context.getClickedFace() == Direction::Down) {
        return ActionResultType::Pass;
    }

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查是否可以转换为耕地
    const Block* tilledBlock = getTilledBlock(&state->owner());
    if (tilledBlock == nullptr) {
        return ActionResultType::Pass;
    }

    // 上方必须是空气
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr && !aboveState->isAir()) {
        return ActionResultType::Pass;
    }

    // 获取新方块状态
    const BlockState& newState = tilledBlock->getDefaultState();

    // 播放耕地音效
    if (context.getPlayer() != nullptr) {
        context.getPlayer()->playSound(SoundEvents::ITEM_HOE_TILL, 1.0f, 1.0f);
    }

    // 设置新方块状态
    world.setBlockState(pos, &newState, 11);

    // 消耗耐久：直接对玩家权威手持物（player->getHeldItem(hand)）做 hurtAndBreak，而非
    // context.getItemStackMut()（调用方局部拷贝，耐久损耗不回写权威物品栏——同桶类对齐缺陷）。
    // 外层 useItemOnBlock/handleItemUseOn 的 damage 对比会检测到权威槽 damage 变化跳过通用 shrink，
    // 避免把耐久损耗误当数量消耗（vanilla 锄耕地损耗 1 耐久，数量不变）。
    if (context.getPlayer() != nullptr) {
        ItemStack& heldItem = context.getPlayer()->getHeldItem(context.getHand());
        LivingEntity::hurtAndBreak(heldItem, 1, context.getPlayer(), EquipmentSlot::MainHand);
    }

    return ActionResultType::Success;
}

const Block* HoeItem::getTilledBlock(const Block* original) noexcept
{
    if (original == nullptr) {
        return nullptr;
    }
    auto& map = _getTillingMap();
    auto it = map.find(original);
    if (it != map.end()) {
        return it->second;
    }
    return nullptr;
}

f32 HoeItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
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

bool HoeItem::isEffectiveMaterial(const Material& material) const
{
    // 锄对树叶材质有效
    return material == Material::LEAVES;
}

std::unordered_set<const Block*> HoeItem::_initializeEffectiveBlocks()
{
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

    // 诡异疣块
    if (VanillaBlocks::WARPED_WART_BLOCK) blocks.insert(VanillaBlocks::WARPED_WART_BLOCK);

    // 干海带块
    if (VanillaBlocks::DRIED_KELP_BLOCK) blocks.insert(VanillaBlocks::DRIED_KELP_BLOCK);

    // 标靶方块
    if (VanillaBlocks::TARGET) blocks.insert(VanillaBlocks::TARGET);

    // 菌光体
    if (VanillaBlocks::SHROOMLIGHT) blocks.insert(VanillaBlocks::SHROOMLIGHT);

    return blocks;
}

std::unordered_map<const Block*, const Block*>& HoeItem::_getTillingMap()
{
    // "construct on first use" 模式：函数局部静态变量在第一次调用时初始化
    static std::unordered_map<const Block*, const Block*> map = []() {
        std::unordered_map<const Block*, const Block*> m;

        // 泥土/草地 -> 耕地
        if (VanillaBlocks::GRASS_BLOCK && VanillaBlocks::FARMLAND) {
            m[VanillaBlocks::GRASS_BLOCK] = VanillaBlocks::FARMLAND;
        }
        if (VanillaBlocks::GRASS_PATH && VanillaBlocks::FARMLAND) {
            m[VanillaBlocks::GRASS_PATH] = VanillaBlocks::FARMLAND;
        }
        if (VanillaBlocks::DIRT && VanillaBlocks::FARMLAND) {
            m[VanillaBlocks::DIRT] = VanillaBlocks::FARMLAND;
        }
        // 注意：砂土(COARSE_DIRT)锄后变成泥土，不是耕地
        // 需要再锄一次才能变成耕地
        if (VanillaBlocks::COARSE_DIRT && VanillaBlocks::DIRT) {
            m[VanillaBlocks::COARSE_DIRT] = VanillaBlocks::DIRT;
        }

        return m;
    }();
    return map;
}

} // namespace tool
} // namespace item
} // namespace mc
