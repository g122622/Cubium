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

#include "Item.hpp"
#include "ActionResult.hpp"
#include "ItemRegistry.hpp"
#include "ItemStack.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/food/Food.hpp"
#include "common/item/tag/ItemTag.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace mc {

// ============================================================================
// ItemProperties
// ============================================================================

ItemProperties& ItemProperties::maxStackSize(i32 maxStackSize)
{
    if (m_maxDamage > 0) {
        // 有耐久度的物品不能堆叠
        m_maxStackSize = 1;
    } else {
        m_maxStackSize = std::clamp(maxStackSize, 1, mc::item::DEFAULT_MAX_STACK_SIZE);
    }
    return *this;
}

ItemProperties& ItemProperties::maxDamage(i32 maxDamage)
{
    m_maxDamage = std::max(0, maxDamage);
    if (m_maxDamage > 0) {
        // 有耐久度的物品不能堆叠
        m_maxStackSize = 1;
    }
    return *this;
}

ItemProperties& ItemProperties::containerItem(const Item* containerItem)
{
    m_containerItem = containerItem;
    return *this;
}

ItemProperties& ItemProperties::rarity(ItemRarity rarity)
{
    m_rarity = rarity;
    return *this;
}

ItemProperties& ItemProperties::burnable(bool value)
{
    m_burnable = value;
    return *this;
}

ItemProperties& ItemProperties::repairable(bool value)
{
    m_repairable = value;
    return *this;
}

ItemProperties& ItemProperties::food(const item::food::Food* food)
{
    m_food = food;
    return *this;
}

ItemProperties& ItemProperties::group(const ItemGroup* group)
{
    m_creativeTab = group;
    return *this;
}

// ============================================================================
// Item
// ============================================================================

Item::Item(ItemProperties properties)
    : m_maxStackSize(properties.maxStackSize())
    , m_maxDamage(properties.maxDamage())
    , m_containerItem(properties.containerItem())
    , m_rarity(properties.rarity())
    , m_burnable(properties.isBurnable())
    , m_repairable(properties.isRepairable())
    , m_food(properties.food())
    , m_creativeTab(properties.group())
{}

Item* Item::getItem(ItemId itemId)
{
    return ItemRegistry::instance().getItem(itemId);
}

Item* Item::getItem(const ResourceLocation& id)
{
    return ItemRegistry::instance().getItem(id);
}

void Item::forEachItem(std::function<void(Item&)> callback)
{
    ItemRegistry::instance().forEachItem(callback);
}

ItemStack Item::getDefaultInstance() const
{
    return ItemStack(*this, 1);
}

f32 Item::getDestroySpeed(const ItemStack& stack, const BlockState& state) const
{
    // 默认挖掘速度为1.0
    // 工具类物品会重写此方法
    (void)stack;
    (void)state;
    return 1.0f;
}

bool Item::canHarvestBlock(const BlockState& state) const
{
    // 默认不能采集需要工具的方块
    // 工具类物品会重写此方法
    (void)state;
    return false;
}

std::string Item::getTranslationKey() const
{
    return "item." + m_itemLocation.toString();
}

std::string Item::getTranslationKey(const ItemStack& stack) const
{
    (void)stack;
    return getTranslationKey();
}

std::string Item::getName() const
{
    // 默认返回翻译键，未来可支持语言文件
    return getTranslationKey();
}

i32 Item::getUseDuration(const ItemStack& stack) const
{
    (void)stack;
    // 如果物品是食物，返回正常食用时间（32 ticks）或快速食用时间（16 ticks）
    if (isFood() && m_food != nullptr) {
        return m_food->isFastEat() ? 16 : 32;
    }
    return 0;
}

// ============================================================================
// 物品使用 - 新增虚方法实现
// ============================================================================

ActionResultType Item::onItemUse(ItemUseContext& context)
{
    // 默认实现：不做任何操作，传递给下一个处理器
    (void)context;
    return ActionResultType::Pass;
}

ItemActionResult Item::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 派发自定义物品组件回调 - onUse（右键空中使用物品）
    {
        auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
        std::string itemTypeId = itemLocation().toString();
        if (itemCompReg.hasUseCallback(itemTypeId)) {
            mc::mod::bedrock::addon::ItemComponentUseEvent useEvent;
            useEvent.itemTypeId = itemTypeId;
            useEvent.sourceId = player.id();
            ItemStack heldStack = player.getHeldItem(hand);
            useEvent.itemStackAmount = heldStack.getCount();
            itemCompReg.dispatchUse(itemTypeId, useEvent);
        }
    }

    // 食物自动处理逻辑
    if (isFood()) {
        ItemStack heldStack = player.getHeldItem(hand);
        // canEatWhenFull 参数从 Food.canAlwaysEat() 获取
        bool canEatWhenFull = m_food->canAlwaysEat();
        if (player.canEat(canEatWhenFull)) {
            player.setActiveHand(hand);
            return ItemActionResult::success(heldStack);
        } else {
            return ItemActionResult::fail(heldStack);
        }
    }
    // 默认实现：返回传递结果
    (void)world;
    return ItemActionResult::pass(ItemStack());
}

bool Item::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    // 默认实现：不做任何操作
    (void)stack;
    (void)player;
    (void)target;
    (void)hand;
    return false;
}

ItemStack Item::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    // 默认实现：返回原物品堆
    // 食物类物品会重写此方法来应用食物效果
    (void)world;
    (void)entity;
    return stack;
}

void Item::onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft)
{
    // 默认实现：不做任何操作
    // 弓、三叉戟等会重写此方法
    (void)stack;
    (void)world;
    (void)entity;
    (void)timeLeft;
}

// ============================================================================
// 物品Tick与提示
// ============================================================================

void Item::inventoryTick(ItemStack& stack, IWorld& world, Entity& entity, i32 itemSlot, bool isSelected) const
{
    // 默认实现：不做任何操作
    // 地图、时钟等会重写此方法
    (void)stack;
    (void)world;
    (void)entity;
    (void)itemSlot;
    (void)isSelected;
}

void Item::onArmorTick(ItemStack& stack, IWorld& world, LivingEntity& player) const
{
    // 默认实现：不做任何操作
    // 鞘翅等特殊护甲会重写此方法
    (void)stack;
    (void)world;
    (void)player;
}

void Item::addInformation(const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const
{
    // 默认实现：不做任何操作
    // 子类可重写以添加自定义提示
    // world 可为 null（对应 MC 的 EMPTY TooltipContext，如客户端 Player 无 IWorld 时）
    (void)stack;
    (void)world;
    (void)tooltip;
    (void)advanced;
}

bool Item::hasEffect(const ItemStack& stack) const
{
    // 默认实现：检查是否有附魔
    return stack.hasEnchantments();
}

// ============================================================================
// 标签与分类
// ============================================================================

bool Item::isIn(const item::tag::ItemTag& tag) const
{
    return tag.contains(this);
}

// ============================================================================
// 食物相关
// ============================================================================

bool Item::canEat(const ItemStack& stack, const Player& player) const
{
    // 默认实现：如果不是食物，返回false
    // FoodItem会重写此方法
    (void)stack;
    (void)player;
    return false;
}

// ============================================================================
// 合成回调
// ============================================================================

void Item::onCraftedBy(ItemStack& stack, IWorld& world, Player& player)
{
    // 默认实现：转发给 onCraftedPostProcess
    (void)player;
    onCraftedPostProcess(stack, world);
}

void Item::onCraftedPostProcess(ItemStack& stack, IWorld& world)
{
    // 默认实现：不做任何操作
    // 子类可重写以执行合成后的特殊初始化
    // 例如 FilledMapItem 重写此方法处理地图缩放/锁定
    (void)stack;
    (void)world;
}

// ============================================================================
// 耐久度与修复
// ============================================================================

void Item::onDestroyed(ItemStack& stack, IWorld& world, Entity& entity)
{
    // 默认实现：不做任何操作
    // 子类可重写以播放破坏音效等
    (void)stack;
    (void)world;
    (void)entity;
}

// ============================================================================
// 工具相关
// ============================================================================

bool Item::hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker)
{
    // 默认实现：不造成耐久消耗
    // 工具类物品（剑、斧等）会重写此方法
    (void)stack;
    (void)target;
    (void)attacker;
    return false;
}

void Item::postHitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker)
{
    // 默认实现：无效果
    // 重锤(MaceItem)重写此方法以重置下落距离
    (void)stack;
    (void)target;
    (void)attacker;
}

bool Item::onBlockDestroyed(
    ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& breaker)
{
    // 默认实现：不造成耐久消耗
    // 工具类物品（镐、斧、铲、锄等）会重写此方法
    (void)stack;
    (void)world;
    (void)state;
    (void)pos;
    (void)breaker;
    return false;
}

bool Item::isSuitableFor(const BlockState& state) const
{
    // 默认实现：不适用于任何方块
    // 工具类物品会重写此方法以匹配工具类型
    (void)state;
    return false;
}

void Item::fillItemGroup(const ItemGroup& group, std::vector<ItemStack>& items) const
{
    // 默认实现：如果物品属于该组，添加一个默认物品堆
    if (isInGroup(group)) {
        items.push_back(getDefaultInstance());
    }
}

bool Item::isInGroup(const ItemGroup& group) const
{
    // 默认实现：检查物品的创造模式组是否匹配
    return m_creativeTab == &group;
}

// ============================================================================
// 新增方法实现
// ============================================================================

ItemRarity Item::getRarity(const ItemStack& stack) const
{
    // 附魔物品稀有度提升
    if (stack.hasEnchantments()) {
        switch (m_rarity) {
            case ItemRarity::Common:
            case ItemRarity::Uncommon:
                return ItemRarity::Rare;
            case ItemRarity::Rare:
                return ItemRarity::Epic;
            case ItemRarity::Epic:
            default:
                return m_rarity;
        }
    }
    return m_rarity;
}

bool Item::isEnchantable(const ItemStack& stack) const
{
    // 物品可附魔当且仅当堆叠数为1且可损坏
    (void)stack;
    return m_maxStackSize == 1 && isDamageable();
}

bool Item::getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const
{
    // 默认实现：检查修复材料是否是容器物品
    // 子类（如ArmorItem、ToolItem）会重写此方法检查特定材料
    (void)toRepair;
    if (m_repairable && m_containerItem != nullptr) {
        return repair.getItem() == m_containerItem;
    }
    return false;
}

item::ItemAttributeModifiers Item::getAttributeModifiers(i32 equipmentSlot) const
{
    // 默认实现：返回空的属性修饰符
    // 子类（如SwordItem、ArmorItem）会重写此方法添加特定属性
    (void)equipmentSlot;
    return item::ItemAttributeModifiers();
}

} // namespace mc
