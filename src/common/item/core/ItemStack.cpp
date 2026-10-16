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

#include "ItemStack.hpp"
#include "Item.hpp"
#include "ItemRegistry.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentPatchNbt.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc {

namespace {
// NBT key constants
namespace nbt_keys {
// 1.21.11 数据组件格式键
constexpr const char* ID = "id";
constexpr const char* COUNT = "count";           // 1.21.11：i32
constexpr const char* COMPONENTS = "components"; // 1.21.11 数据组件补丁 compound
} // namespace nbt_keys
} // namespace

// ============================================================================
// 静态常量
// ============================================================================

const ItemStack ItemStack::EMPTY;

// ============================================================================
// ItemExtras 的共享空实例
// ============================================================================

const std::vector<std::unique_ptr<text::ITextComponent>>& ItemStack::_emptyLore()
{
    static const std::vector<std::unique_ptr<text::ITextComponent>> EMPTY;
    return EMPTY;
}

const AdventureModePredicate& ItemStack::_emptyPredicate()
{
    static const AdventureModePredicate EMPTY;
    return EMPTY;
}

const std::string& ItemStack::_emptyString()
{
    static const std::string EMPTY;
    return EMPTY;
}

const nlohmann::json& ItemStack::_emptyJson()
{
    static const nlohmann::json EMPTY;
    return EMPTY;
}

// ============================================================================
// 构造函数
// ============================================================================

ItemStack::ItemStack(const Item& item, i32 count)
    : m_item(&item)
    , m_count(count)
{
    // 数量验证
    if (m_count <= 0) {
        m_item = nullptr;
        m_count = 0;
    }
}

ItemStack::ItemStack(const Item* item, i32 count)
    : m_item(item)
    , m_count(count)
{
    // 数量验证
    if (m_count <= 0 || m_item == nullptr) {
        m_item = nullptr;
        m_count = 0;
    }
}

ItemStack::ItemStack(const ItemStack& other)
    : m_item(other.m_item)
    , m_count(other.m_count)
    , m_damage(other.m_damage)
    , m_repairCost(other.m_repairCost)
    , m_customName(other.m_customName ? other.m_customName->deepCopy() : nullptr)
    , m_enchantments(other.m_enchantments)
    // 空 extras 原样传 nullptr（不得无条件 make_unique——容器每 tick 逐槽拷贝，
    // 给空 extras 分配会把这笔优化反向吃掉）；非空才深拷贝
    , m_extras(other.m_extras ? std::make_unique<ItemExtras>(*other.m_extras) : nullptr)
{}

ItemStack& ItemStack::operator=(const ItemStack& other)
{
    if (this != &other) {
        m_item = other.m_item;
        m_count = other.m_count;
        m_damage = other.m_damage;
        m_repairCost = other.m_repairCost;
        m_customName = other.m_customName ? other.m_customName->deepCopy() : nullptr;
        m_enchantments = other.m_enchantments;
        m_extras = other.m_extras ? std::make_unique<ItemExtras>(*other.m_extras) : nullptr;
    }
    return *this;
}

// ============================================================================
// 数量操作
// ============================================================================

void ItemStack::setCount(i32 count)
{
    m_count = count;
    if (m_count <= 0) {
        m_count = 0;
        m_item = nullptr;
        m_damage = 0;
    }
}

i32 ItemStack::getMaxStackSize() const
{
    if (isEmpty()) {
        return 0;
    }
    // 如果有耐久度，堆叠数为1
    if (isDamageable()) {
        return 1;
    }
    // max_stack_size 组件覆盖优先，缺省回退物品基础值
    if (m_extras && m_extras->maxStackSizeOverride.has_value()) {
        return *m_extras->maxStackSizeOverride;
    }
    return m_item->maxStackSize();
}

// ============================================================================
// 数据组件覆盖
// ============================================================================

void ItemStack::setUnbreakable(bool value)
{
    if (!value) {
        // 清除不该反向分配 extras
        if (m_extras) {
            m_extras->unbreakable = false;
        }
        return;
    }
    _ensureExtras().unbreakable = true;
}

ItemRarity ItemStack::getRarity() const
{
    if (isEmpty()) {
        return ItemRarity::Common;
    }
    // 基础稀有度：rarity 组件覆盖优先，缺省回退物品自身稀有度
    ItemRarity rarity =
        (m_extras && m_extras->rarityOverride.has_value()) ? *m_extras->rarityOverride : m_item->rarity();
    // 已附魔则提升稀有度（对齐 vanilla ItemStack.getRarity 的 switch）
    if (!hasEnchantments()) {
        return rarity;
    }
    switch (rarity) {
        case ItemRarity::Common:
        case ItemRarity::Uncommon:
            return ItemRarity::Rare;
        case ItemRarity::Rare:
            return ItemRarity::Epic;
        case ItemRarity::Epic:
        default:
            return rarity;
    }
}

void ItemStack::setItemNameComponent(std::unique_ptr<text::ITextComponent> name)
{
    if (name == nullptr) {
        if (m_extras) {
            m_extras->itemName = nullptr;
        }
        return;
    }
    _ensureExtras().itemName = std::move(name);
}

void ItemStack::setItemModel(const std::string& model)
{
    if (model.empty()) {
        if (m_extras) {
            m_extras->itemModel.reset();
        }
        return;
    }
    _ensureExtras().itemModel = model;
}

i32 ItemStack::getEnchantmentValue() const
{
    if (isEmpty()) {
        return 0;
    }
    // enchantable 组件覆盖优先，缺省回退物品自身附魔能力
    if (m_extras && m_extras->enchantableOverride.has_value()) {
        return *m_extras->enchantableOverride;
    }
    return m_item->getItemEnchantability();
}

bool ItemStack::isEnchantable() const
{
    // 对齐 vanilla ItemStack.isEnchantable：存在 enchantable 组件且当前无附魔。
    // enchantable 组件缺省时回退到物品基础附魔能力（>0 视为具备 enchantable）。
    if (getEnchantmentValue() <= 0) {
        return false;
    }
    return !hasEnchantments();
}

void ItemStack::setEnchantableOverride(i32 value)
{
    if (value <= 0) {
        if (m_extras) {
            m_extras->enchantableOverride.reset();
        }
        return;
    }
    _ensureExtras().enchantableOverride = value;
}

void ItemStack::setAttackRange(item::component::AttackRange range)
{
    _ensureExtras().attackRange = range;
}

void ItemStack::setFireworkExplosion(item::component::FireworkExplosion explosion)
{
    _ensureExtras().fireworkExplosion = std::move(explosion);
}

void ItemStack::setFireworks(item::component::Fireworks fireworks)
{
    // 本方法是所有写入路径（JSON/wire/NBT 读取与直接调用）的唯一收口，在此统一规范化，
    // 避免下游按未校验的值分配/编码。两处约束来自 vanilla Fireworks：
    // 1. flight_duration 是 UNSIGNED_BYTE（NBT/JSON 侧 0..255，超出即无法回写）；
    // 2. explosions 上限 256（构造函数对超限抛 IllegalArgumentException）。
    using item::component::Fireworks;
    if (fireworks.flightDuration < 0 || fireworks.flightDuration > 255) {
        spdlog::warn("Clamping firework flight duration from {} to the range [0, 255]", fireworks.flightDuration);
        fireworks.flightDuration = std::clamp(fireworks.flightDuration, 0, 255);
    }
    if (fireworks.explosions.size() > static_cast<usize>(Fireworks::MAX_EXPLOSIONS)) {
        spdlog::warn("Truncating firework explosions from {} to the maximum of {}",
            fireworks.explosions.size(),
            Fireworks::MAX_EXPLOSIONS);
        fireworks.explosions.resize(static_cast<usize>(Fireworks::MAX_EXPLOSIONS));
    }
    _ensureExtras().fireworks = std::move(fireworks);
}

// ============================================================================
// 附魔
// ============================================================================

i32 ItemStack::getEnchantmentLevel(const std::string& enchantmentId) const
{
    return m_enchantments.getLevel(enchantmentId);
}

bool ItemStack::hasEnchantment(const std::string& enchantmentId) const
{
    return m_enchantments.has(enchantmentId);
}

void ItemStack::addEnchantment(const std::string& enchantmentId, i32 level)
{
    m_enchantments.set(enchantmentId, level);
}

// ============================================================================
// 耐久度
// ============================================================================

bool ItemStack::isDamageable() const
{
    if (isEmpty()) {
        return false;
    }
    // unbreakable 组件置位后物品不可损坏（对齐 vanilla isDamageableItem 的 !has(UNBREAKABLE)）
    if (m_extras && m_extras->unbreakable) {
        return false;
    }
    // max_damage 组件覆盖优先
    if (m_extras && m_extras->maxDamageOverride.has_value()) {
        return *m_extras->maxDamageOverride > 0;
    }
    return m_item->isDamageable();
}

bool ItemStack::canBeHurtBy(const DamageSource& source) const
{
    // 防火物品不会被火焰伤害源摧毁
    if (isEmpty()) {
        return false;
    }
    if (m_item->isIn(item::tag::ItemTags::FIRE_RESISTANT()) && source.isFire()) {
        return false;
    }
    return true;
}

bool ItemStack::isDamaged() const
{
    return isDamageable() && m_damage > 0;
}

void ItemStack::setDamage(i32 damage)
{
    if (!isDamageable()) {
        return;
    }
    m_damage = std::max(0, damage);
    i32 maxDamage = getMaxDamage();
    if (m_damage >= maxDamage) {
        // 物品损坏，清空堆
        m_count = 0;
        m_item = nullptr;
        m_damage = 0;
    }
}

i32 ItemStack::getMaxDamage() const
{
    if (isEmpty()) {
        return 0;
    }
    // max_damage 组件覆盖优先，缺省回退物品基础值
    if (m_extras && m_extras->maxDamageOverride.has_value()) {
        return *m_extras->maxDamageOverride;
    }
    return m_item->maxDamage();
}

bool ItemStack::attemptDamageItem(i32 amount)
{
    return attemptDamageItem(amount, nullptr);
}

bool ItemStack::attemptDamageItem(i32 amount, LivingEntity* entity)
{
    if (!isDamageable()) {
        return false;
    }

    // 耐久保护附魔处理
    i32 unbreakingLevel = item::enchant::EnchantmentHelper::getUnbreakingLevel(*this);

    if (unbreakingLevel > 0) {
        // 护甲的耐久保护有 60% 概率不生效
        bool isArmor = m_item != nullptr && m_item->isArmor();

        // 优先使用实体所在世界的随机数生成器
        // 物品耐久保护的概率计算应使用世界关联的随机源
        math::IRandom* random = nullptr;
        if (entity != nullptr) {
            IWorld* world = entity->world();
            if (world != nullptr) {
                random = &world->getRandom();
            }
        }

        if (random != nullptr) {
            // 使用世界关联的随机源
            // 对齐 vanilla 1.21.11 RemoveBinomial.process（RemoveBinomial.java:21-23 小 amount 路径）：
            //   for (i32 j = 0; j < amount; j++) { if (random.nextFloat() < chance) removed++; }
            //   return amount - removed;
            // 即对【原始 amount】做固定次数独立伯努利试验，累加被忽略（移除）的点数，循环上界不自减。
            // 此前实现 `for (i < amount) { if (ignore) --amount; }` 每次忽略后减小循环上界，致 amount>1
            // 时试验次数减少、保护效果被削弱（level=1 工具 amount=2 期望剩余 1.25 vs vanilla 1.0），
            // 影响荆棘反弹（ThornsEnchantment hurtAndBreak amount=2）等多点损耗场景。改用 removed 累加器
            // 对齐 vanilla。shouldIgnoreDurabilityLoss 的 chance（工具 level/(level+1)、盔甲 0.4*level/(level+1)）
            // 已与 vanilla remove_binomial fraction chance 数值等价，仅循环结构需对齐。
            i32 removed = 0;
            for (i32 i = 0; i < amount; ++i) {
                if (item::enchant::EnchantmentHelper::shouldIgnoreDurabilityLoss(unbreakingLevel, isArmor, *random)) {
                    ++removed;
                }
            }
            amount -= removed;
        } else {
            // 降级：无实体或实体不在世界中时，使用线程局部静态随机源
            static thread_local math::Random s_random(
                static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

            i32 removed = 0;
            for (i32 i = 0; i < amount; ++i) {
                if (item::enchant::EnchantmentHelper::shouldIgnoreDurabilityLoss(unbreakingLevel, isArmor, s_random)) {
                    ++removed;
                }
            }
            amount -= removed;
        }

        // 如果所有伤害都被抵消，不造成伤害
        if (amount <= 0) {
            return false;
        }
    }

    // 保存旧耐久度（用于事件触发）
    // 耐久度 = maxDamage - damage
    i32 maxDamage = getMaxDamage();
    i32 oldDurability = maxDamage - m_damage;

    m_damage += amount;

    // 触发耐久变化事件（进度系统）
    if (entity != nullptr) {
        IWorld* world = entity->world();
        if (world != nullptr) {
            i32 newDurability = maxDamage - m_damage;
            // 尝试获取玩家ID（如果是玩家）
            Player* player = dynamic_cast<Player*>(entity);
            if (player != nullptr) {
                world->onItemDurabilityChange(player->id(), *this, oldDurability, newDurability);
            }
        }
    }

    if (m_damage >= maxDamage) {
        // 物品损坏
        m_count = 0;
        m_item = nullptr;
        m_damage = 0;
        return true;
    }
    return false;
}

// ============================================================================
// 堆叠操作
// ============================================================================

bool ItemStack::isStackable() const
{
    if (isEmpty()) {
        return false;
    }
    i32 maxStack = getMaxStackSize();
    if (maxStack <= 1) {
        return false;
    }
    // 已损坏的可堆叠物品不能堆叠
    if (isDamageable() && isDamaged()) {
        return false;
    }
    return true;
}

bool ItemStack::canMergeWith(const ItemStack& other) const
{
    if (isEmpty() || other.isEmpty()) {
        return false;
    }

    // 物品类型必须相同
    if (m_item != other.m_item) {
        return false;
    }

    // 如果有耐久度，需要耐久度相同才能合并
    if (isDamageable() && m_damage != other.m_damage) {
        return false;
    }

    // 比较修复成本（铁砧操作次数）
    if (m_repairCost != other.m_repairCost) {
        return false;
    }

    // 比较自定义名称
    bool customNameEqual = false;
    if (m_customName && other.m_customName) {
        customNameEqual = *m_customName == *other.m_customName;
    } else if (!m_customName && !other.m_customName) {
        customNameEqual = true;
    }
    if (!customNameEqual) {
        return false;
    }

    // 比较 Lore
    const auto& lore = getLore();
    const auto& otherLore = other.getLore();
    if (lore.size() != otherLore.size()) {
        return false;
    }
    for (size_t i = 0; i < lore.size(); ++i) {
        if (lore[i] && otherLore[i]) {
            if (*lore[i] != *otherLore[i]) {
                return false;
            }
        } else if (lore[i] || otherLore[i]) {
            return false;
        }
    }

    if (getPotionId() != other.getPotionId()) {
        return false;
    }

    // 比较附魔 - 相同附魔的物品可以堆叠
    if (m_enchantments != other.m_enchantments) {
        return false;
    }

    if (_dataRef() != other._dataRef()) {
        return false;
    }

    // 组件覆盖须一致（含 unbreakable / rarity / attack_range 等）
    if (!_componentOverridesEqual(other)) {
        return false;
    }

    // 检查是否还有堆叠空间
    return m_count < getMaxStackSize();
}

bool ItemStack::isSameItem(const ItemStack& other) const
{
    if (isEmpty() && other.isEmpty()) {
        return true;
    }
    if (isEmpty() || other.isEmpty()) {
        return false;
    }
    return m_item == other.m_item;
}

ItemStack ItemStack::split(i32 amount)
{
    if (isEmpty()) {
        return EMPTY;
    }

    i32 splitCount = std::min(amount, m_count);
    ItemStack result(*m_item, splitCount);
    result.m_damage = m_damage;
    result.m_repairCost = m_repairCost;
    result.m_customName = m_customName ? m_customName->deepCopy() : nullptr;
    result.m_enchantments = m_enchantments;
    result.m_extras = m_extras ? std::make_unique<ItemExtras>(*m_extras) : nullptr;

    // 减少当前堆
    setCount(m_count - splitCount);

    return result;
}

ItemStack ItemStack::copy() const
{
    if (isEmpty()) {
        return EMPTY;
    }
    // 统一使用深拷贝构造，避免新增组件字段时遗漏独立复制路径。
    return ItemStack(*this);
}

ItemStack ItemStack::transmuteCopy(const Item& newItem, i32 newCount) const
{
    // 对应 MC 1.21.11 ItemStack#transmuteCopy(Item, i32)
    // 创建新物品堆，保留原物品堆的所有额外数据
    if (newCount <= 0) {
        return EMPTY;
    }
    ItemStack result(newItem, newCount);
    // 保留原物品堆的额外数据（不保留耐久度，因为新物品可能是满耐久）
    result.m_customName = m_customName ? m_customName->deepCopy() : nullptr;
    result.m_enchantments = m_enchantments;
    result.m_extras = m_extras ? std::make_unique<ItemExtras>(*m_extras) : nullptr;
    result.m_repairCost = m_repairCost;
    return result;
}

// ============================================================================
// 物品功能
// ============================================================================

f32 ItemStack::getDestroySpeed(const BlockState& state) const
{
    if (isEmpty()) {
        return 1.0f;
    }
    return m_item->getDestroySpeed(*this, state);
}

bool ItemStack::canHarvestBlock(const BlockState& state) const
{
    if (isEmpty()) {
        return false;
    }
    return m_item->canHarvestBlock(state);
}

// ============================================================================
// 冒险模式谓词
// ============================================================================

bool ItemStack::canPlaceOnBlockInAdventureMode(const BlockState& state) const
{
    if (isEmpty() || !hasCanPlaceOn()) {
        return false;
    }
    return getCanPlaceOn().test(state);
}

bool ItemStack::canPlaceOnBlockInAdventureMode(IWorld& world, const BlockState& state) const
{
    if (isEmpty() || !hasCanPlaceOn()) {
        return false;
    }
    return getCanPlaceOn().test(world, state);
}

bool ItemStack::canPlaceOnBlockInAdventureMode(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    if (isEmpty() || !hasCanPlaceOn()) {
        return false;
    }
    return getCanPlaceOn().test(world, pos, state);
}

bool ItemStack::canBreakBlockInAdventureMode(const BlockState& state) const
{
    if (isEmpty() || !hasCanDestroy()) {
        return false;
    }
    return getCanDestroy().test(state);
}

bool ItemStack::canBreakBlockInAdventureMode(IWorld& world, const BlockState& state) const
{
    if (isEmpty() || !hasCanDestroy()) {
        return false;
    }
    return getCanDestroy().test(world, state);
}

bool ItemStack::canBreakBlockInAdventureMode(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    if (isEmpty() || !hasCanDestroy()) {
        return false;
    }
    return getCanDestroy().test(world, pos, state);
}

void ItemStack::inventoryTick(IWorld& world, Entity& entity, i32 itemSlot, bool isSelected)
{
    if (isEmpty()) {
        return;
    }
    m_item->inventoryTick(*this, world, entity, itemSlot, isSelected);
}

void ItemStack::onArmorTick(IWorld& world, LivingEntity& player)
{
    if (isEmpty()) {
        return;
    }
    m_item->onArmorTick(*this, world, player);
}

void ItemStack::onCraftedBy(Player& player, i32 amount)
{
    if (isEmpty()) {
        return;
    }
    MC_UNUSED(amount);
    // 统计更新由 ServerPlayer::onItemCrafted 处理
    // 这里只委托给 Item 的合成回调
    IWorld* world = player.world();
    if (world != nullptr) {
        // m_item 是 const Item*，但 onCraftedBy 需要修改 ItemStack（如 NBT 标签），
        // onCraftedBy 本身不修改 Item 对象，所以 const_cast 安全
        const_cast<Item*>(m_item)->onCraftedBy(*this, *world, player);
    }
}

// ============================================================================
// 显示名称
// ============================================================================

std::unique_ptr<text::ITextComponent> ItemStack::getDisplayName() const
{
    if (isEmpty()) {
        return std::make_unique<text::StringTextComponent>("");
    }

    // 如果有自定义名称，返回自定义名称
    if (m_customName) {
        return m_customName->deepCopy();
    }

    // item_name 组件覆盖优先于物品默认名（对齐 vanilla ItemStack.getHoverName：
    // custom_name 优先，其次 item_name，最后物品默认名）
    if (m_extras && m_extras->itemName) {
        return m_extras->itemName->deepCopy();
    }

    // 否则返回物品名称
    return std::make_unique<text::StringTextComponent>(m_item->getName());
}

// ============================================================================
// JSON 序列化
// ============================================================================

nlohmann::json ItemStack::toJson() const
{
    nlohmann::json json;
    if (isEmpty()) {
        return json;
    }

    json["id"] = m_item->itemLocation().toString();
    json["Count"] = m_count;

    if (m_damage > 0) {
        json["Damage"] = m_damage;
    }

    if (hasEnchantments()) {
        json["Enchantments"] = m_enchantments.toJson();
    }

    if (m_customName) {
        json["CustomName"] = m_customName->toJson();
    }

    const auto& lore = getLore();
    if (!lore.empty()) {
        nlohmann::json loreJson = nlohmann::json::array();
        for (const auto& line : lore) {
            if (line) {
                loreJson.push_back(line->toJson());
            }
        }
        json["Lore"] = std::move(loreJson);
    }

    if (!getPotionId().empty()) {
        json["Potion"] = getPotionId();
    }

    if (hasTag()) {
        json["Tag"] = _dataRef();
    }

    // 数据组件覆盖（JSON 用组件名做键，缺省不写出）
    if (m_extras) {
        if (m_extras->maxStackSizeOverride.has_value()) {
            json["minecraft:max_stack_size"] = *m_extras->maxStackSizeOverride;
        }
        if (m_extras->maxDamageOverride.has_value()) {
            json["minecraft:max_damage"] = *m_extras->maxDamageOverride;
        }
        if (m_extras->unbreakable) {
            json["minecraft:unbreakable"] = true;
        }
        if (m_extras->itemName) {
            json["minecraft:item_name"] = m_extras->itemName->toJson();
        }
        if (m_extras->itemModel.has_value()) {
            json["minecraft:item_model"] = *m_extras->itemModel;
        }
        if (m_extras->rarityOverride.has_value()) {
            const auto name = rarityName(*m_extras->rarityOverride);
            json["minecraft:rarity"] = std::string(name.value_or("common"));
        }
        if (m_extras->enchantableOverride.has_value()) {
            json["minecraft:enchantable"] = *m_extras->enchantableOverride;
        }
        if (m_extras->attackRange.has_value()) {
            const auto& ar = *m_extras->attackRange;
            json["minecraft:attack_range"] = {{"min_reach", ar.minRange},
                {"max_reach", ar.maxRange},
                {"min_creative_reach", ar.minCreativeRange},
                {"max_creative_reach", ar.maxCreativeRange},
                {"hitbox_margin", ar.hitboxMargin},
                {"mob_factor", ar.mobFactor}};
        }
        if (m_extras->fireworkExplosion.has_value()) {
            const auto& explosion = *m_extras->fireworkExplosion;
            json["minecraft:firework_explosion"] = {{"shape", std::string(fireworkShapeName(explosion.shape))},
                {"colors", explosion.colors},
                {"fade_colors", explosion.fadeColors},
                {"has_trail", explosion.hasTrail},
                {"has_twinkle", explosion.hasTwinkle}};
        }
        if (m_extras->fireworks.has_value()) {
            auto explosions = nlohmann::json::array();
            for (const auto& explosion : m_extras->fireworks->explosions) {
                explosions.push_back({{"shape", std::string(fireworkShapeName(explosion.shape))},
                    {"colors", explosion.colors},
                    {"fade_colors", explosion.fadeColors},
                    {"has_trail", explosion.hasTrail},
                    {"has_twinkle", explosion.hasTwinkle}});
            }
            json["minecraft:fireworks"] = {
                {"flight_duration", m_extras->fireworks->flightDuration}, {"explosions", std::move(explosions)}};
        }
    }

    return json;
}

Result<ItemStack> ItemStack::fromJson(const nlohmann::json& json)
{
    if (json.is_null() || !json.is_object()) {
        return EMPTY;
    }

    if (!json.contains("id") || !json["id"].is_string()) {
        return Error(ErrorCode::InvalidData, "ItemStack JSON missing 'id' field");
    }

    std::string itemId = json["id"].get<std::string>();
    ResourceLocation itemLocation(itemId);
    const Item* item = ItemRegistry::instance().getItem(itemLocation);
    if (item == nullptr) {
        return Error(ErrorCode::InvalidItem, "Unknown item ID: " + itemId);
    }

    // 数量：与 toJson 的写出键对应
    i32 count = 1;
    if (json.contains("Count") && json["Count"].is_number()) {
        count = json["Count"].get<i32>();
    }

    ItemStack stack(item, count);

    if (json.contains("Damage") && json["Damage"].is_number()) {
        stack.setDamage(json["Damage"].get<i32>());
    }

    if (json.contains("Enchantments")) {
        auto enchantResult = item::enchant::EnchantmentContainer::fromJson(json["Enchantments"]);
        if (enchantResult.success()) {
            stack.m_enchantments = std::move(enchantResult.value());
        }
    }

    if (json.contains("CustomName") && json["CustomName"].is_object()) {
        stack.m_customName = text::ITextComponent::fromJson(json["CustomName"]);
    }

    if (json.contains("Lore") && json["Lore"].is_array()) {
        for (const auto& lineJson : json["Lore"]) {
            if (lineJson.is_object()) {
                stack._ensureExtras().lore.push_back(text::ITextComponent::fromJson(lineJson));
            }
        }
    }

    if (json.contains("Potion") && json["Potion"].is_string()) {
        stack.setPotionId(json["Potion"].get<std::string>());
    }

    if (json.contains("Tag") && json["Tag"].is_object()) {
        stack._ensureExtras().customData = json["Tag"];
    }

    // 数据组件覆盖（与 toJson 的写出键对应）
    if (json.contains("minecraft:max_stack_size") && json["minecraft:max_stack_size"].is_number()) {
        stack._ensureExtras().maxStackSizeOverride = json["minecraft:max_stack_size"].get<i32>();
    }
    if (json.contains("minecraft:max_damage") && json["minecraft:max_damage"].is_number()) {
        stack._ensureExtras().maxDamageOverride = json["minecraft:max_damage"].get<i32>();
    }
    if (json.contains("minecraft:unbreakable") && json["minecraft:unbreakable"].is_boolean()) {
        stack.setUnbreakable(json["minecraft:unbreakable"].get<bool>());
    }
    if (json.contains("minecraft:item_name")) {
        const auto& nameJson = json["minecraft:item_name"];
        if (nameJson.is_object()) {
            stack.setItemNameComponent(text::ITextComponent::fromJson(nameJson));
        }
    }
    if (json.contains("minecraft:item_model") && json["minecraft:item_model"].is_string()) {
        stack.setItemModel(json["minecraft:item_model"].get<std::string>());
    }
    if (json.contains("minecraft:rarity") && json["minecraft:rarity"].is_string()) {
        if (auto rarity = rarityFromName(json["minecraft:rarity"].get<std::string>()); rarity.has_value()) {
            stack._ensureExtras().rarityOverride = *rarity;
        }
    }
    if (json.contains("minecraft:enchantable") && json["minecraft:enchantable"].is_number()) {
        stack.setEnchantableOverride(json["minecraft:enchantable"].get<i32>());
    }
    if (json.contains("minecraft:attack_range") && json["minecraft:attack_range"].is_object()) {
        const auto& ar = json["minecraft:attack_range"];
        item::component::AttackRange range{};
        if (ar.contains("min_reach")) {
            range.minRange = ar["min_reach"].get<f32>();
        }
        if (ar.contains("max_reach")) {
            range.maxRange = ar["max_reach"].get<f32>();
        }
        if (ar.contains("min_creative_reach")) {
            range.minCreativeRange = ar["min_creative_reach"].get<f32>();
        }
        if (ar.contains("max_creative_reach")) {
            range.maxCreativeRange = ar["max_creative_reach"].get<f32>();
        }
        if (ar.contains("hitbox_margin")) {
            range.hitboxMargin = ar["hitbox_margin"].get<f32>();
        }
        if (ar.contains("mob_factor")) {
            range.mobFactor = ar["mob_factor"].get<f32>();
        }
        stack.setAttackRange(range);
    }
    if (json.contains("minecraft:firework_explosion") && json["minecraft:firework_explosion"].is_object()) {
        stack.setFireworkExplosion(_fireworkExplosionFromJson(json["minecraft:firework_explosion"]));
    }
    if (json.contains("minecraft:fireworks") && json["minecraft:fireworks"].is_object()) {
        const auto& fireworksJson = json["minecraft:fireworks"];
        item::component::Fireworks fireworks{};
        if (fireworksJson.contains("flight_duration") && fireworksJson["flight_duration"].is_number()) {
            fireworks.flightDuration = fireworksJson["flight_duration"].get<i32>();
        }
        if (fireworksJson.contains("explosions") && fireworksJson["explosions"].is_array()) {
            for (const auto& explosionJson : fireworksJson["explosions"]) {
                if (explosionJson.is_object()) {
                    fireworks.explosions.push_back(_fireworkExplosionFromJson(explosionJson));
                }
            }
        }
        stack.setFireworks(std::move(fireworks));
    }

    return stack;
}

item::component::FireworkExplosion ItemStack::_fireworkExplosionFromJson(const nlohmann::json& json)
{
    item::component::FireworkExplosion explosion{};
    if (json.contains("shape") && json["shape"].is_string()) {
        explosion.shape = item::component::fireworkShapeFromName(json["shape"].get<std::string>())
                              .value_or(item::component::FireworkShape::SmallBall);
    }
    // 逐元素校验为整数后再收集：`get<std::vector<i32>>()` 遇到非整元素会抛异常，
    // 而 JSON 可能来自数据包/命令等不可信输入。
    auto readColors = [&json](const char* key) {
        std::vector<i32> colors;
        auto it = json.find(key);
        if (it == json.end() || !it->is_array()) {
            return colors;
        }
        colors.reserve(it->size());
        for (const auto& element : *it) {
            if (element.is_number_integer()) {
                colors.push_back(element.get<i32>());
            } else {
                spdlog::warn("Ignoring non-integer firework color entry in '{}'", key);
            }
        }
        return colors;
    };
    explosion.colors = readColors("colors");
    explosion.fadeColors = readColors("fade_colors");
    if (json.contains("has_trail") && json["has_trail"].is_boolean()) {
        explosion.hasTrail = json["has_trail"].get<bool>();
    }
    if (json.contains("has_twinkle") && json["has_twinkle"].is_boolean()) {
        explosion.hasTwinkle = json["has_twinkle"].get<bool>();
    }
    return explosion;
}

// ============================================================================
// 数据组件补丁转换（NBT/wire 序列化用）
// ============================================================================

item::component::DataComponentPatch ItemStack::toComponentPatch() const
{
    using namespace item::component;
    DataComponentPatch patch;

    if (m_damage > 0) {
        patch.add(DataComponentType::Damage, DataComponentPayload{std::in_place_index<1>, m_damage});
    }
    if (m_repairCost > 0) {
        patch.add(DataComponentType::RepairCost, DataComponentPayload{std::in_place_index<1>, m_repairCost});
    }
    if (m_customName) {
        patch.add(DataComponentType::CustomName,
            DataComponentPayload{std::in_place_index<2>, m_customName ? m_customName->deepCopy() : nullptr});
    }
    const auto& lore = getLore();
    if (!lore.empty()) {
        std::vector<std::unique_ptr<text::ITextComponent>> loreCopy;
        loreCopy.reserve(lore.size());
        for (const auto& line : lore) {
            loreCopy.push_back(line ? line->deepCopy() : nullptr);
        }
        patch.add(DataComponentType::Lore, DataComponentPayload{std::in_place_index<3>, std::move(loreCopy)});
    }
    if (hasEnchantments()) {
        patch.add(DataComponentType::Enchantments, DataComponentPayload{std::in_place_index<4>, m_enchantments});
    }
    // PotionContents：potionId + customColor + customEffects + customName。
    // customColor/customEffects 在业务侧由 PotionUtils 走 customData JSON
    // （CustomPotionColor/CustomPotionEffects 键）承载，这里桥接到组件载荷以便 wire 传输。
    // customName（药水自定义名）项目暂无承载，保持 nullopt。
    const bool hasPotionFields = !getPotionId().empty() || potion::PotionUtils::hasCustomEffects(*this) ||
        potion::PotionUtils::getCustomPotionColor(*this).has_value();
    if (hasPotionFields) {
        PotionContentsPayload pc{};
        pc.potionId = getPotionId();
        if (const auto color = potion::PotionUtils::getCustomPotionColor(*this); color.has_value()) {
            pc.customColor = static_cast<i32>(static_cast<u32>(*color));
        }
        pc.customEffects = potion::PotionUtils::getCustomEffects(*this);
        patch.add(DataComponentType::PotionContents, DataComponentPayload{std::in_place_index<5>, std::move(pc)});
    }
    if (hasCanPlaceOn()) {
        patch.add(DataComponentType::CanPlaceOn, DataComponentPayload{std::in_place_index<6>, getCanPlaceOn()});
    }
    if (hasCanDestroy()) {
        patch.add(DataComponentType::CanBreak, DataComponentPayload{std::in_place_index<6>, getCanDestroy()});
    }
    if (hasTag()) {
        auto custom = _dataRef();
        if (custom.contains("BundleContents")) {
            auto contents = custom["BundleContents"];
            contents.erase("selected");
            patch.add(DataComponentType::BundleContents,
                DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, std::move(contents)});
            custom.erase("BundleContents");
        }
        for (const auto& [key, type] : {std::pair{"writable_book_content", DataComponentType::WritableBookContent},
                 std::pair{"written_book_content", DataComponentType::WrittenBookContent}}) {
            if (custom.contains(key)) {
                patch.add(type, DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, custom[key]});
                custom.erase(key);
            }
        }
        if (custom.contains("BlockEntityTag") && custom["BlockEntityTag"].contains("Patterns")) {
            patch.add(DataComponentType::BannerPatterns,
                DataComponentPayload{
                    std::in_place_index<ComponentPayloadIndex::Json>, custom["BlockEntityTag"]["Patterns"]});
            custom["BlockEntityTag"].erase("Patterns");
            if (custom["BlockEntityTag"].empty()) custom.erase("BlockEntityTag");
        }
        if (custom.contains("StoredEnchantments")) {
            item::enchant::EnchantmentContainer stored;
            for (const auto& entry : custom["StoredEnchantments"])
                stored.set(entry.at("id").get<std::string>(), entry.at("lvl").get<i32>());
            patch.add(DataComponentType::StoredEnchantments,
                DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Enchantments>, std::move(stored)});
            custom.erase("StoredEnchantments");
        }
        if (!custom.empty())
            patch.add(DataComponentType::CustomData,
                DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, std::move(custom)});
    }
    // 数据组件覆盖（仅非空时写出）
    if (m_extras) {
        if (m_extras->maxStackSizeOverride.has_value()) {
            patch.add(DataComponentType::MaxStackSize,
                DataComponentPayload{std::in_place_index<1>, *m_extras->maxStackSizeOverride});
        }
        if (m_extras->maxDamageOverride.has_value()) {
            patch.add(DataComponentType::MaxDamage,
                DataComponentPayload{std::in_place_index<1>, *m_extras->maxDamageOverride});
        }
        if (m_extras->unbreakable) {
            patch.add(DataComponentType::Unbreakable, DataComponentPayload{std::in_place_index<8>, true});
        }
        if (m_extras->itemName) {
            patch.add(DataComponentType::ItemName,
                DataComponentPayload{std::in_place_index<2>, m_extras->itemName->deepCopy()});
        }
        if (m_extras->itemModel.has_value()) {
            patch.add(
                DataComponentType::ItemModel, DataComponentPayload{std::in_place_index<10>, *m_extras->itemModel});
        }
        if (m_extras->rarityOverride.has_value()) {
            patch.add(
                DataComponentType::Rarity, DataComponentPayload{std::in_place_index<9>, *m_extras->rarityOverride});
        }
        if (m_extras->enchantableOverride.has_value()) {
            patch.add(DataComponentType::Enchantable,
                DataComponentPayload{std::in_place_index<1>, *m_extras->enchantableOverride});
        }
        if (m_extras->attackRange.has_value()) {
            patch.add(
                DataComponentType::AttackRange, DataComponentPayload{std::in_place_index<11>, *m_extras->attackRange});
        }
        if (m_extras->fireworkExplosion.has_value()) {
            patch.add(DataComponentType::FireworkExplosion,
                DataComponentPayload{
                    std::in_place_index<ComponentPayloadIndex::FireworkExplosion>, *m_extras->fireworkExplosion});
        }
        if (m_extras->fireworks.has_value()) {
            patch.add(DataComponentType::Fireworks,
                DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Fireworks>, *m_extras->fireworks});
        }
    }
    return patch;
}

void ItemStack::applyComponentPatch(const item::component::DataComponentPatch& patch)
{
    using namespace item::component;
    // custom_data 可位于任意注册序位置，先应用以免覆盖随后还原的结构化组件。
    for (const auto& entry : patch.added()) {
        if (entry.typeId == componentTypeId(DataComponentType::CustomData)) {
            _ensureExtras().customData = std::get<nlohmann::json>(entry.value);
        }
    }
    for (const auto& entry : patch.added()) {
        auto type = componentTypeById(entry.typeId);
        if (!type.has_value()) {
            continue;
        }
        switch (*type) {
            case DataComponentType::Damage: {
                if (const auto* p = std::get_if<i32>(&entry.value)) {
                    m_damage = *p;
                }
                break;
            }
            case DataComponentType::RepairCost: {
                if (const auto* p = std::get_if<i32>(&entry.value)) {
                    m_repairCost = *p;
                }
                break;
            }
            case DataComponentType::CustomName: {
                const auto& p = std::get<std::unique_ptr<text::ITextComponent>>(entry.value);
                m_customName = p ? p->deepCopy() : nullptr;
                break;
            }
            case DataComponentType::Lore: {
                const auto& lines = std::get<std::vector<std::unique_ptr<text::ITextComponent>>>(entry.value);
                if (lines.empty()) {
                    // 空 lore 不该反向分配一份 extras
                    clearLore();
                } else {
                    auto& lore = _ensureExtras().lore;
                    lore.clear();
                    lore.reserve(lines.size());
                    for (const auto& line : lines) {
                        lore.push_back(line ? line->deepCopy() : nullptr);
                    }
                }
                break;
            }
            case DataComponentType::Enchantments: {
                m_enchantments = std::get<item::enchant::EnchantmentContainer>(entry.value);
                break;
            }
            case DataComponentType::PotionContents: {
                const auto& pc = std::get<PotionContentsPayload>(entry.value);
                setPotionId(pc.potionId);
                // customColor/customEffects 写回 customData JSON（与 PotionUtils 路径一致）。
                std::optional<u32> color{};
                if (pc.customColor.has_value()) {
                    color = static_cast<u32>(*pc.customColor);
                }
                potion::PotionUtils::setCustomPotionColor(*this, color);
                potion::PotionUtils::setCustomEffects(*this, pc.customEffects);
                break;
            }
            case DataComponentType::CanPlaceOn: {
                setCanPlaceOn(std::get<AdventureModePredicate>(entry.value));
                break;
            }
            case DataComponentType::CanBreak: {
                setCanDestroy(std::get<AdventureModePredicate>(entry.value));
                break;
            }
            case DataComponentType::BundleContents:
                getOrCreateTag()["BundleContents"] = std::get<nlohmann::json>(entry.value);
                break;
            case DataComponentType::WritableBookContent:
                getOrCreateTag()["writable_book_content"] = std::get<nlohmann::json>(entry.value);
                break;
            case DataComponentType::WrittenBookContent:
                getOrCreateTag()["written_book_content"] = std::get<nlohmann::json>(entry.value);
                break;
            case DataComponentType::BannerPatterns:
                getOrCreateTag()["BlockEntityTag"]["Patterns"] = std::get<nlohmann::json>(entry.value);
                break;
            case DataComponentType::StoredEnchantments: {
                auto stored = nlohmann::json::array();
                for (const auto& value : std::get<item::enchant::EnchantmentContainer>(entry.value).getAll())
                    stored.push_back({{"id", value.enchantmentId}, {"lvl", value.level}});
                getOrCreateTag()["StoredEnchantments"] = std::move(stored);
                break;
            }
            case DataComponentType::CustomData:
                break;
            case DataComponentType::MaxStackSize: {
                if (const auto* p = std::get_if<i32>(&entry.value)) {
                    _ensureExtras().maxStackSizeOverride = *p;
                }
                break;
            }
            case DataComponentType::MaxDamage: {
                if (const auto* p = std::get_if<i32>(&entry.value)) {
                    _ensureExtras().maxDamageOverride = *p;
                }
                break;
            }
            case DataComponentType::Unbreakable: {
                setUnbreakable(true);
                break;
            }
            case DataComponentType::ItemName: {
                const auto& p = std::get<std::unique_ptr<text::ITextComponent>>(entry.value);
                setItemNameComponent(p ? p->deepCopy() : nullptr);
                break;
            }
            case DataComponentType::ItemModel: {
                setItemModel(std::get<std::string>(entry.value));
                break;
            }
            case DataComponentType::Rarity: {
                _ensureExtras().rarityOverride = std::get<ItemRarity>(entry.value);
                break;
            }
            case DataComponentType::Enchantable: {
                if (const auto* p = std::get_if<i32>(&entry.value)) {
                    setEnchantableOverride(*p);
                }
                break;
            }
            case DataComponentType::AttackRange: {
                setAttackRange(std::get<item::component::AttackRange>(entry.value));
                break;
            }
            case DataComponentType::FireworkExplosion: {
                setFireworkExplosion(std::get<item::component::FireworkExplosion>(entry.value));
                break;
            }
            case DataComponentType::Fireworks: {
                setFireworks(std::get<item::component::Fireworks>(entry.value));
                break;
            }
        }
    }
    for (i32 typeId : patch.removed()) {
        auto type = componentTypeById(typeId);
        if (!type.has_value()) {
            continue;
        }
        switch (*type) {
            case DataComponentType::Damage:
                m_damage = 0;
                break;
            case DataComponentType::RepairCost:
                m_repairCost = 0;
                break;
            case DataComponentType::CustomName:
                m_customName = nullptr;
                break;
            case DataComponentType::Lore:
                clearLore();
                break;
            case DataComponentType::Enchantments:
                m_enchantments.clear();
                break;
            case DataComponentType::PotionContents:
                setPotionId(std::string{});
                break;
            case DataComponentType::CanPlaceOn:
                setCanPlaceOn(AdventureModePredicate{});
                break;
            case DataComponentType::CanBreak:
                setCanDestroy(AdventureModePredicate{});
                break;
            case DataComponentType::BundleContents:
                if (m_extras) m_extras->customData.erase("BundleContents");
                break;
            case DataComponentType::WritableBookContent:
                if (m_extras) m_extras->customData.erase("writable_book_content");
                break;
            case DataComponentType::WrittenBookContent:
                if (m_extras) m_extras->customData.erase("written_book_content");
                break;
            case DataComponentType::BannerPatterns:
                if (m_extras && m_extras->customData.contains("BlockEntityTag"))
                    m_extras->customData["BlockEntityTag"].erase("Patterns");
                break;
            case DataComponentType::StoredEnchantments:
                if (m_extras) m_extras->customData.erase("StoredEnchantments");
                break;
            case DataComponentType::CustomData:
                // 清除不该反向分配 extras
                if (m_extras) {
                    m_extras->customData = nlohmann::json{};
                }
                break;
            case DataComponentType::MaxStackSize:
                if (m_extras) {
                    m_extras->maxStackSizeOverride.reset();
                }
                break;
            case DataComponentType::MaxDamage:
                if (m_extras) {
                    m_extras->maxDamageOverride.reset();
                }
                break;
            case DataComponentType::Unbreakable:
                setUnbreakable(false);
                break;
            case DataComponentType::ItemName:
                setItemNameComponent(nullptr);
                break;
            case DataComponentType::ItemModel:
                setItemModel(std::string{});
                break;
            case DataComponentType::Rarity:
                if (m_extras) {
                    m_extras->rarityOverride.reset();
                }
                break;
            case DataComponentType::Enchantable:
                setEnchantableOverride(0);
                break;
            case DataComponentType::AttackRange:
                if (m_extras) {
                    m_extras->attackRange.reset();
                }
                break;
            case DataComponentType::FireworkExplosion:
                if (m_extras) {
                    m_extras->fireworkExplosion.reset();
                }
                break;
            case DataComponentType::Fireworks:
                if (m_extras) {
                    m_extras->fireworks.reset();
                }
                break;
        }
    }
}

// ============================================================================
// NBT 序列化（1.21.11 数据组件格式：{id, count:i32, components}）
// ============================================================================

void ItemStack::toNbt(nbt::tags::compound_tag& tag) const
{
    if (isEmpty()) {
        tag.put(nbt_keys::ID, std::string("minecraft:air"));
        tag.put(nbt_keys::COUNT, static_cast<i32>(0));
        return;
    }

    // 物品ID
    tag.put(nbt_keys::ID, m_item->itemLocation().toString());

    // 数量（1.21.11：i32）
    tag.put(nbt_keys::COUNT, static_cast<i32>(m_count));

    // 组件补丁：仅当有非默认组件时写出 components 段
    auto patch = toComponentPatch();
    if (patch.isEmpty()) {
        return;
    }
    auto componentsTag = std::make_unique<nbt::tags::compound_tag>();
    item::component::writePatchToNbt(*componentsTag, patch);
    tag.value.emplace(nbt_keys::COMPONENTS, std::move(componentsTag));
}

Result<ItemStack> ItemStack::fromNbt(const nbt::tags::compound_tag& tag)
{
    // 获取物品ID
    auto it = tag.value.find(nbt_keys::ID);
    if (it == tag.value.end() || it->second->id() != nbt::TagId::String) {
        return Error(ErrorCode::InvalidData, "ItemStack NBT missing 'id' field");
    }

    std::string itemId = dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;

    // 查找物品
    ResourceLocation itemLocation(itemId);
    const Item* item = ItemRegistry::instance().getItem(itemLocation);

    // 数量：1.21.11 键名 "count"。vanilla 的 Codec.INT 在 NbtOps 下接受任意数值型标签
    // （Byte/Short/Int/Long），故此处同样接受 Byte 与 Int 两种承载。
    i32 count = 1;
    auto countIt = tag.value.find(nbt_keys::COUNT);
    if (countIt != tag.value.end()) {
        if (countIt->second->id() == nbt::TagId::Byte) {
            count = dynamic_cast<const nbt::tags::byte_tag&>(*countIt->second).value;
        } else if (countIt->second->id() == nbt::TagId::Int) {
            count = dynamic_cast<const nbt::tags::int_tag&>(*countIt->second).value;
        }
    }

    // 如果物品不存在或数量为0，返回空物品
    if (item == nullptr || count <= 0) {
        return EMPTY;
    }

    ItemStack stack(item, count);

    // 1.21.11 组件格式：读取 components 补丁段
    auto componentsIt = tag.value.find(nbt_keys::COMPONENTS);
    if (componentsIt != tag.value.end() && componentsIt->second->id() == nbt::TagId::Compound) {
        const auto& componentsCompound = dynamic_cast<const nbt::tags::compound_tag&>(*componentsIt->second);
        stack.applyComponentPatch(item::component::readPatchFromNbt(componentsCompound));
    }

    return stack;
}

// ============================================================================
// 比较操作符
// ============================================================================

bool ItemStack::operator==(const ItemStack& other) const
{
    if (isEmpty() && other.isEmpty()) {
        return true;
    }
    if (isEmpty() || other.isEmpty()) {
        return false;
    }

    // 比较 m_customName
    bool customNameEqual = false;
    if (m_customName && other.m_customName) {
        customNameEqual = *m_customName == *other.m_customName;
    } else if (!m_customName && !other.m_customName) {
        customNameEqual = true;
    }

    // 比较 Lore
    // 注：本函数处在热路径上（LivingEntity 每实体 × 8 装备槽 × 每 tick 的装备变更检测），
    // 但无 extras 时各 getter 返回的是共享空实例，比较退化为"两个空容器相等"，开销与
    // 改动前的内联成员一致。
    const auto& lore = getLore();
    const auto& otherLore = other.getLore();
    bool loreEqual = lore.size() == otherLore.size();
    if (loreEqual) {
        for (size_t i = 0; i < lore.size(); ++i) {
            if (lore[i] && otherLore[i]) {
                if (*lore[i] != *otherLore[i]) {
                    loreEqual = false;
                    break;
                }
            } else if (lore[i] || otherLore[i]) {
                loreEqual = false;
                break;
            }
        }
    }

    // 数据组件覆盖：两个物品堆的覆盖集合须一致（无覆盖时退化为全部 nullopt/false）
    const bool overridesEqual = _componentOverridesEqual(other);

    return m_item == other.m_item && m_count == other.m_count && m_repairCost == other.m_repairCost &&
        m_damage == other.m_damage && customNameEqual && loreEqual && getPotionId() == other.getPotionId() &&
        _dataRef() == other._dataRef() && m_enchantments.getAll() == other.m_enchantments.getAll() &&
        getCanPlaceOn() == other.getCanPlaceOn() && getCanDestroy() == other.getCanDestroy() && overridesEqual;
}

bool ItemStack::_componentOverridesEqual(const ItemStack& other) const
{
    const auto& a = m_extras;
    const auto& b = other.m_extras;

    const bool aHas = a && a->hasComponentOverrides();
    const bool bHas = b && b->hasComponentOverrides();
    if (!aHas && !bHas) {
        return true;
    }
    if (aHas != bHas) {
        return false;
    }

    const auto sameText = [](const std::unique_ptr<text::ITextComponent>& x,
                              const std::unique_ptr<text::ITextComponent>& y) {
        if (x && y) {
            return *x == *y;
        }
        return !x && !y;
    };

    return a->maxStackSizeOverride == b->maxStackSizeOverride && a->maxDamageOverride == b->maxDamageOverride &&
        a->unbreakable == b->unbreakable && sameText(a->itemName, b->itemName) && a->itemModel == b->itemModel &&
        a->rarityOverride == b->rarityOverride && a->enchantableOverride == b->enchantableOverride &&
        a->attackRange == b->attackRange && a->fireworkExplosion == b->fireworkExplosion &&
        a->fireworks == b->fireworks;
}

// ============================================================================
// 容器物品
// ============================================================================

ItemStack ItemStack::getContainerItem() const
{
    if (isEmpty()) {
        return EMPTY;
    }

    const Item* container = m_item->containerItem();
    if (container == nullptr) {
        return EMPTY;
    }

    // 创建容器物品堆（数量为1）
    // 注意：耐久度等属性需要复制（如使用过的桶装牛奶返回空桶）
    ItemStack result(container, 1);
    return result;
}

bool ItemStack::hasContainerItem() const
{
    if (isEmpty()) {
        return false;
    }
    return m_item->hasContainerItem();
}

bool ItemStack::hasTag() const
{
    const nlohmann::json& data = _dataRef();
    return data.is_object() && !data.empty();
}

const nlohmann::json* ItemStack::getTag() const
{
    if (!hasTag()) {
        return nullptr;
    }

    return &_dataRef();
}

nlohmann::json* ItemStack::getTag()
{
    if (!hasTag()) {
        return nullptr;
    }

    return &m_extras->customData;
}

nlohmann::json& ItemStack::getOrCreateTag()
{
    // 写语义：必须物化 extras
    nlohmann::json& data = _ensureExtras().customData;
    if (!data.is_object()) {
        data = nlohmann::json::object();
    }

    return data;
}

const nlohmann::json* ItemStack::getChildTag(const std::string& name) const
{
    const nlohmann::json& data = _dataRef();
    if (!data.is_object()) {
        return nullptr;
    }

    auto iter = data.find(name);
    if (iter == data.end() || !iter->is_object()) {
        return nullptr;
    }

    return &(*iter);
}

nlohmann::json& ItemStack::getOrCreateChildTag(const std::string& name)
{
    nlohmann::json& child = getOrCreateTag()[name];
    if (!child.is_object()) {
        child = nlohmann::json::object();
    }

    return child;
}

void ItemStack::removeChildTag(const std::string& name)
{
    // 无 extras 时本就无标签可移除，不分配
    if (!m_extras) {
        return;
    }

    nlohmann::json& data = m_extras->customData;
    if (!data.is_object()) {
        return;
    }

    data.erase(name);
    if (data.empty()) {
        data = nlohmann::json();
    }
}

// ============================================================================
// 标签合并
// ============================================================================

void ItemStack::mergeTag(const nlohmann::json& other)
{
    if (!other.is_object()) {
        return;
    }
    nlohmann::json& tag = getOrCreateTag();
    mergeJsonObjects(tag, other);
}

void ItemStack::mergeTag(nlohmann::json&& other)
{
    if (!other.is_object()) {
        return;
    }
    nlohmann::json& tag = getOrCreateTag();
    // 对于移动语义，仍然需要递归合并对象
    mergeJsonObjects(tag, other);
}

void ItemStack::mergeJsonObjects(nlohmann::json& target, const nlohmann::json& source)
{
    if (!target.is_object() || !source.is_object()) {
        // 非对象类型，直接替换
        target = source;
        return;
    }

    // 遍历源对象的所有键值对
    for (auto& [key, value] : source.items()) {
        if (target.contains(key) && target[key].is_object() && value.is_object()) {
            // 两边都是对象，递归合并
            mergeJsonObjects(target[key], value);
        } else {
            // 直接设置/覆盖
            target[key] = value;
        }
    }
}

} // namespace mc
