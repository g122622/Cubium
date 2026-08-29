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

#include "ShovelItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/tool/ToolItem.hpp"
#include "common/item/items/tool/ToolType.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mc {
namespace item {
namespace tool {

ShovelItem::ShovelItem(const tier::IItemTier& tier, f32 attackDamage, f32 attackSpeed, ItemProperties properties)
    : ToolItem(attackDamage, attackSpeed, tier, _initializeEffectiveBlocks(), ToolType::Shovel, properties)
{
    // 映射表使用"construct on first use"模式，无需在此初始化
}

ActionResultType ShovelItem::onItemUse(ItemUseContext& context)
{
    // 锹功能 - 熄灭营火 + 创建土径
    IWorld& world = context.world();
    const BlockPos& pos = context.blockPos();

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return ActionResultType::Pass;
    }

    const Block& block = state->owner();

    // 熄灭营火功能（优先于土径创建）
    // 锹可以熄灭点燃的营火和灵魂营火
    if (VanillaBlocks::CAMPFIRE && &block == VanillaBlocks::CAMPFIRE) {
        if (blocks::CampfireBlock::isLit(*state)) {
            // 熄灭营火
            BlockState newState = state->with(BlockStateProperties::LIT(), false);
            world.setBlockState(pos, &newState, 11);

            // 播放熄灭音效
            if (context.getPlayer() != nullptr) {
                context.getPlayer()->playSound(SoundEvents::BLOCK_CAMPFIRE_EXTINGUISH, 1.0f, 1.0f);
            }

            // 消耗耐久：直接对玩家权威手持物做 hurtAndBreak，而非 context.getItemStackMut() 拷贝
            // （耐久损耗不回写权威物品栏，同桶类对齐缺陷）。外层 damage 对比跳过通用 shrink。
            // 无玩家场景（如发射器）不损耗玩家槽位耐久，方块替换照常（对齐 vanilla）。
            if (context.getPlayer() != nullptr) {
                ItemStack& heldItem = context.getPlayer()->getHeldItem(context.getHand());
                LivingEntity::hurtAndBreak(heldItem, 1, context.getPlayer(), EquipmentSlot::MainHand);
            }

            return ActionResultType::Success;
        }
        return ActionResultType::Pass;
    }

    if (VanillaBlocks::SOUL_CAMPFIRE && &block == VanillaBlocks::SOUL_CAMPFIRE) {
        if (blocks::CampfireBlock::isLit(*state)) {
            // 熄灭灵魂营火
            BlockState newState = state->with(BlockStateProperties::LIT(), false);
            world.setBlockState(pos, &newState, 11);

            // 播放熄灭音效
            if (context.getPlayer() != nullptr) {
                context.getPlayer()->playSound(SoundEvents::BLOCK_CAMPFIRE_EXTINGUISH, 1.0f, 1.0f);
            }

            // 消耗耐久：直接对玩家权威手持物做 hurtAndBreak，而非 context.getItemStackMut() 拷贝
            // （耐久损耗不回写权威物品栏，同桶类对齐缺陷）。外层 damage 对比跳过通用 shrink。
            // 无玩家场景（如发射器）不损耗玩家槽位耐久，方块替换照常（对齐 vanilla）。
            if (context.getPlayer() != nullptr) {
                ItemStack& heldItem = context.getPlayer()->getHeldItem(context.getHand());
                LivingEntity::hurtAndBreak(heldItem, 1, context.getPlayer(), EquipmentSlot::MainHand);
            }

            return ActionResultType::Success;
        }
        return ActionResultType::Pass;
    }

    // 锹创建土径逻辑
    // 检查点击的面是否为底面（不能从下方创建土径）
    if (context.getClickedFace() == Direction::Down) {
        return ActionResultType::Pass;
    }

    // 检查是否可以转换为土径
    const Block* pathBlock = getPathBlock(&state->owner());
    if (pathBlock == nullptr) {
        return ActionResultType::Pass;
    }

    // 上方必须是空气
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr && !aboveState->isAir()) {
        return ActionResultType::Pass;
    }

    // 获取新方块状态
    const BlockState& newState = pathBlock->getDefaultState();

    // 播放创建土径音效
    if (context.getPlayer() != nullptr) {
        context.getPlayer()->playSound(SoundEvents::ITEM_SHOVEL_FLATTEN, 1.0f, 1.0f);
    }

    // 设置新方块状态
    world.setBlockState(pos, &newState, 11);

    // 消耗耐久：直接对玩家权威手持物做 hurtAndBreak，而非 context.getItemStackMut() 拷贝
    // （耐久损耗不回写权威物品栏，同桶类对齐缺陷）。外层 damage 对比跳过通用 shrink。
    // 无玩家场景（如发射器）不损耗玩家槽位耐久，方块替换照常（对齐 vanilla）。
    if (context.getPlayer() != nullptr) {
        ItemStack& heldItem = context.getPlayer()->getHeldItem(context.getHand());
        LivingEntity::hurtAndBreak(heldItem, 1, context.getPlayer(), EquipmentSlot::MainHand);
    }

    return ActionResultType::Success;
}

const Block* ShovelItem::getPathBlock(const Block* original)
{
    if (original == nullptr) {
        return nullptr;
    }
    auto& map = _getPathMap();
    auto it = map.find(original);
    if (it != map.end()) {
        return it->second;
    }
    return nullptr;
}

bool ShovelItem::canHarvestBlock(const BlockState& state) const
{
    // 如果方块需要锹，检查挖掘等级
    if (state.getHarvestTool() == TOOL_TYPE_SHOVEL) {
        return m_tier.getHarvestLevel() >= state.getHarvestLevel();
    }

    // 锹对雪层(SNOW)和雪块(SNOW_BLOCK)总是可以采集
    const Block& block = state.owner();
    if (VanillaBlocks::SNOW && &block == VanillaBlocks::SNOW) {
        return true;
    }
    if (VanillaBlocks::SNOW_BLOCK && &block == VanillaBlocks::SNOW_BLOCK) {
        return true;
    }

    return false;
}

f32 ShovelItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
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

bool ShovelItem::isEffectiveMaterial(const Material& material) const
{
    return material == Material::EARTH || material == Material::SAND || material == Material::SNOW;
}

std::unordered_set<const Block*> ShovelItem::_initializeEffectiveBlocks()
{
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

std::unordered_map<const Block*, const Block*>& ShovelItem::_getPathMap()
{
    // "construct on first use" 模式：函数局部静态变量在第一次调用时初始化
    static std::unordered_map<const Block*, const Block*> map = []() {
        std::unordered_map<const Block*, const Block*> m;

        // 草方块 -> 土径
        if (VanillaBlocks::GRASS_BLOCK && VanillaBlocks::GRASS_PATH) {
            m[VanillaBlocks::GRASS_BLOCK] = VanillaBlocks::GRASS_PATH;
        }
        // TODO: 压土径映射不完整。wiki tech_锹.txt#用途 行281 指出上方为空气的草方块、泥土、砂土、
        //   菌丝体、灰化土、缠根泥土均可被锹压为土径，此处仅映射了 grass_block。补全 dirt/coarse_dirt/
        //   mycelium/podzol/rooted_dirt -> GRASS_PATH(DIRT_PATH) 映射后，需同步补集成测试（锹压泥土→土径等）。

        return m;
    }();
    return map;
}

} // namespace tool
} // namespace item
} // namespace mc
