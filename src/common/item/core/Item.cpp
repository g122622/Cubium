#include "Item.hpp"
#include "ItemStack.hpp"
#include "ItemRegistry.hpp"
#include "ActionResult.hpp"
#include "../context/ItemUseContext.hpp"
#include "../food/Food.hpp"
#include "../tag/ItemTag.hpp"
#include "../../world/block/Block.hpp"
#include "../../world/World.hpp"
#include "../../entity/core/LivingEntity.hpp"
#include "../../entity/entities/player/Player.hpp"
#include <sstream>

namespace mc {

// ============================================================================
// ItemProperties
// ============================================================================

ItemProperties& ItemProperties::maxStackSize(i32 maxStackSize) {
    if (m_maxDamage > 0) {
        // 有耐久度的物品不能堆叠
        m_maxStackSize = 1;
    } else {
        m_maxStackSize = std::clamp(maxStackSize, 1, 64);
    }
    return *this;
}

ItemProperties& ItemProperties::maxDamage(i32 maxDamage) {
    m_maxDamage = std::max(0, maxDamage);
    if (m_maxDamage > 0) {
        // 有耐久度的物品不能堆叠
        m_maxStackSize = 1;
    }
    return *this;
}

ItemProperties& ItemProperties::containerItem(const Item* containerItem) {
    m_containerItem = containerItem;
    return *this;
}

ItemProperties& ItemProperties::rarity(ItemRarity rarity) {
    m_rarity = rarity;
    return *this;
}

ItemProperties& ItemProperties::burnable(bool value) {
    m_burnable = value;
    return *this;
}

ItemProperties& ItemProperties::repairable(bool value) {
    m_repairable = value;
    return *this;
}

ItemProperties& ItemProperties::food(const item::food::Food* food) {
    m_food = food;
    return *this;
}

ItemProperties& ItemProperties::group(const ItemGroup* group) {
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
    , m_creativeTab(properties.group()) {
}

Item* Item::getItem(ItemId itemId) {
    return ItemRegistry::instance().getItem(itemId);
}

Item* Item::getItem(const ResourceLocation& id) {
    return ItemRegistry::instance().getItem(id);
}

void Item::forEachItem(std::function<void(Item&)> callback) {
    ItemRegistry::instance().forEachItem(callback);
}

ItemStack Item::getDefaultInstance() const {
    return ItemStack(*this, 1);
}

f32 Item::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    // 默认挖掘速度为1.0
    // 工具类物品会重写此方法
    (void)stack;
    (void)state;
    return 1.0f;
}

bool Item::canHarvestBlock(const BlockState& state) const {
    // 默认不能采集需要工具的方块
    // 工具类物品会重写此方法
    (void)state;
    return false;
}

String Item::getTranslationKey() const {
    return "item." + m_itemLocation.toString();
}

String Item::getTranslationKey(const ItemStack& stack) const {
    (void)stack;
    return getTranslationKey();
}

String Item::getName() const {
    // 默认返回翻译键，未来可支持语言文件
    return getTranslationKey();
}

// ============================================================================
// 物品使用 - 新增虚方法实现
// ============================================================================

ActionResultType Item::onItemUse(ItemUseContext& context) {
    // 默认实现：不做任何操作，传递给下一个处理器
    (void)context;
    return ActionResultType::Pass;
}

ItemActionResult Item::onItemRightClick(World& world, Player& player, Hand hand) {
    // 默认实现：返回传递结果
    (void)world;
    (void)player;
    (void)hand;
    return ItemActionResult::pass(ItemStack());
}

bool Item::itemInteractionForEntity(ItemStack& stack, Player& player,
                                     LivingEntity& target, Hand hand) {
    // 默认实现：不做任何操作
    (void)stack;
    (void)player;
    (void)target;
    (void)hand;
    return false;
}

ItemStack Item::onItemUseFinish(ItemStack& stack, World& world, LivingEntity& entity) {
    // 默认实现：返回原物品堆
    // 食物类物品会重写此方法来应用食物效果
    (void)world;
    (void)entity;
    return stack;
}

void Item::onPlayerStoppedUsing(ItemStack& stack, World& world,
                                 LivingEntity& entity, i32 timeLeft) {
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

void Item::inventoryTick(ItemStack& stack, World& world, Entity& entity,
                          i32 itemSlot, bool isSelected) {
    // 默认实现：不做任何操作
    // 地图、时钟等会重写此方法
    (void)stack;
    (void)world;
    (void)entity;
    (void)itemSlot;
    (void)isSelected;
}

void Item::addInformation(const ItemStack& stack, World& world,
                           std::vector<String>& tooltip, bool advanced) const {
    // 默认实现：不做任何操作
    // 子类可重写以添加自定义提示
    (void)stack;
    (void)world;
    (void)tooltip;
    (void)advanced;
}

bool Item::hasEffect(const ItemStack& stack) const {
    // 默认实现：检查是否有附魔
    return stack.hasEnchantments();
}

// ============================================================================
// 标签与分类
// ============================================================================

bool Item::isIn(const item::tag::ItemTag& tag) const {
    // 默认实现：检查物品是否在标签中
    // 需要ItemTag系统支持
    (void)tag;
    return false;
}

// ============================================================================
// 食物相关
// ============================================================================

bool Item::canEat(const ItemStack& stack, const Player& player) const {
    // 默认实现：如果不是食物，返回false
    // FoodItem会重写此方法
    (void)stack;
    (void)player;
    return false;
}

// ============================================================================
// 耐久度与修复
// ============================================================================

void Item::onDestroyed(ItemStack& stack, World& world, Entity& entity) {
    // 默认实现：不做任何操作
    // 子类可重写以播放破坏音效等
    (void)stack;
    (void)world;
    (void)entity;
}

} // namespace mc
