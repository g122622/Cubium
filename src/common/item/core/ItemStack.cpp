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
#include "common/util/text/TextParser.hpp"
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

namespace mc {

namespace {
// NBT key constants
namespace nbt_keys {
// 1.21.11 数据组件格式键
constexpr const char* ID = "id";
constexpr const char* COUNT = "count";           // 1.21.11：int（旧 1.16.5 为 "Count" byte）
constexpr const char* COMPONENTS = "components"; // 1.21.11 数据组件补丁 compound

// 旧 1.16.5 格式键（仅 fromNbt 旧存档回退读取用）
constexpr const char* LEGACY_COUNT = "Count";
constexpr const char* TAG = "tag";
constexpr const char* DAMAGE = "Damage";
constexpr const char* ENCHANTMENTS = "Enchantments";
constexpr const char* DISPLAY = "display";
constexpr const char* NAME = "Name";
constexpr const char* LORE = "Lore";
constexpr const char* REPAIR_COST = "RepairCost";
constexpr const char* POTION = "Potion";
constexpr const char* CAN_PLACE_ON = "CanPlaceOn";
constexpr const char* CAN_DESTROY = "CanDestroy";
} // namespace nbt_keys
} // namespace

// ============================================================================
// 静态常量
// ============================================================================

const ItemStack ItemStack::EMPTY;

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
    , m_customName(other.m_customName ? other.m_customName->deepCopy() : nullptr)
    , m_enchantments(other.m_enchantments)
    , m_potionId(other.m_potionId)
    , m_customData(other.m_customData)
    , m_canPlaceOn(other.m_canPlaceOn)
    , m_canDestroy(other.m_canDestroy)
{
    // 深拷贝 Lore
    for (const auto& line : other.m_lore) {
        m_lore.push_back(line ? line->deepCopy() : nullptr);
    }
}

ItemStack& ItemStack::operator=(const ItemStack& other)
{
    if (this != &other) {
        m_item = other.m_item;
        m_count = other.m_count;
        m_damage = other.m_damage;
        m_customName = other.m_customName ? other.m_customName->deepCopy() : nullptr;
        m_potionId = other.m_potionId;
        m_customData = other.m_customData;
        m_enchantments = other.m_enchantments;
        m_canPlaceOn = other.m_canPlaceOn;
        m_canDestroy = other.m_canDestroy;

        // 深拷贝 Lore
        m_lore.clear();
        for (const auto& line : other.m_lore) {
            m_lore.push_back(line ? line->deepCopy() : nullptr);
        }
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
    if (m_item->isDamageable()) {
        return 1;
    }
    return m_item->maxStackSize();
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
        math::Random* random = nullptr;
        if (entity != nullptr) {
            IWorld* world = entity->world();
            if (world != nullptr) {
                random = &world->getRandom();
            }
        }

        if (random != nullptr) {
            // 使用世界关联的随机源
            for (i32 i = 0; i < amount; ++i) {
                if (item::enchant::EnchantmentHelper::shouldIgnoreDurabilityLoss(unbreakingLevel, isArmor, *random)) {
                    --amount;
                }
            }
        } else {
            // 降级：无实体或实体不在世界中时，使用线程局部静态随机源
            static thread_local math::Random s_random(
                static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

            for (i32 i = 0; i < amount; ++i) {
                if (item::enchant::EnchantmentHelper::shouldIgnoreDurabilityLoss(unbreakingLevel, isArmor, s_random)) {
                    --amount;
                }
            }
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
    if (m_lore.size() != other.m_lore.size()) {
        return false;
    }
    for (size_t i = 0; i < m_lore.size(); ++i) {
        if (m_lore[i] && other.m_lore[i]) {
            if (*m_lore[i] != *other.m_lore[i]) {
                return false;
            }
        } else if (m_lore[i] || other.m_lore[i]) {
            return false;
        }
    }

    if (m_potionId != other.m_potionId) {
        return false;
    }

    // 比较附魔 - 相同附魔的物品可以堆叠
    if (m_enchantments != other.m_enchantments) {
        return false;
    }

    if (m_customData != other.m_customData) {
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
    result.m_customName = m_customName ? m_customName->deepCopy() : nullptr;
    result.m_lore.clear();
    for (const auto& line : m_lore) {
        result.m_lore.push_back(line->deepCopy());
    }
    result.m_potionId = m_potionId;
    result.m_customData = m_customData;
    result.m_enchantments = m_enchantments;

    // 减少当前堆
    setCount(m_count - splitCount);

    return result;
}

ItemStack ItemStack::copy() const
{
    if (isEmpty()) {
        return EMPTY;
    }
    ItemStack result(*m_item, m_count);
    result.m_damage = m_damage;
    result.m_customName = m_customName ? m_customName->deepCopy() : nullptr;
    result.m_lore.clear();
    for (const auto& line : m_lore) {
        result.m_lore.push_back(line->deepCopy());
    }
    result.m_potionId = m_potionId;
    result.m_customData = m_customData;
    // 复制附魔
    result.m_enchantments = m_enchantments;
    return result;
}

ItemStack ItemStack::transmuteCopy(const Item& newItem, i32 newCount) const
{
    // 对应 MC 1.21.11 ItemStack#transmuteCopy(Item, int)
    // 创建新物品堆，保留原物品堆的所有额外数据
    if (newCount <= 0) {
        return EMPTY;
    }
    ItemStack result(newItem, newCount);
    // 保留原物品堆的额外数据（不保留耐久度，因为新物品可能是满耐久）
    result.m_customName = m_customName ? m_customName->deepCopy() : nullptr;
    result.m_lore.clear();
    for (const auto& line : m_lore) {
        result.m_lore.push_back(line->deepCopy());
    }
    result.m_potionId = m_potionId;
    result.m_customData = m_customData;
    result.m_enchantments = m_enchantments;
    result.m_canPlaceOn = m_canPlaceOn;
    result.m_canDestroy = m_canDestroy;
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
    return m_canPlaceOn.test(state);
}

bool ItemStack::canPlaceOnBlockInAdventureMode(IWorld& world, const BlockState& state) const
{
    if (isEmpty() || !hasCanPlaceOn()) {
        return false;
    }
    return m_canPlaceOn.test(world, state);
}

bool ItemStack::canPlaceOnBlockInAdventureMode(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    if (isEmpty() || !hasCanPlaceOn()) {
        return false;
    }
    return m_canPlaceOn.test(world, pos, state);
}

bool ItemStack::canBreakBlockInAdventureMode(const BlockState& state) const
{
    if (isEmpty() || !hasCanDestroy()) {
        return false;
    }
    return m_canDestroy.test(state);
}

bool ItemStack::canBreakBlockInAdventureMode(IWorld& world, const BlockState& state) const
{
    if (isEmpty() || !hasCanDestroy()) {
        return false;
    }
    return m_canDestroy.test(world, state);
}

bool ItemStack::canBreakBlockInAdventureMode(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    if (isEmpty() || !hasCanDestroy()) {
        return false;
    }
    return m_canDestroy.test(world, pos, state);
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

    if (!m_lore.empty()) {
        nlohmann::json loreJson = nlohmann::json::array();
        for (const auto& line : m_lore) {
            if (line) {
                loreJson.push_back(line->toJson());
            }
        }
        json["Lore"] = std::move(loreJson);
    }

    if (!m_potionId.empty()) {
        json["Potion"] = m_potionId;
    }

    if (hasTag()) {
        json["Tag"] = m_customData;
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

    if (json.contains("CustomName")) {
        const auto& customNameJson = json["CustomName"];
        if (customNameJson.is_string()) {
            // 旧格式：纯字符串
            stack.m_customName = text::TextParser::parse(customNameJson.get<std::string>());
        } else if (customNameJson.is_object()) {
            // 新格式：JSON 文本组件
            stack.m_customName = text::ITextComponent::fromJson(customNameJson);
        }
    }

    if (json.contains("Lore") && json["Lore"].is_array()) {
        for (const auto& lineJson : json["Lore"]) {
            if (lineJson.is_string()) {
                stack.m_lore.push_back(text::TextParser::parse(lineJson.get<std::string>()));
            } else if (lineJson.is_object()) {
                stack.m_lore.push_back(text::ITextComponent::fromJson(lineJson));
            }
        }
    }

    if (json.contains("Potion") && json["Potion"].is_string()) {
        stack.m_potionId = json["Potion"].get<std::string>();
    }

    if (json.contains("Tag") && json["Tag"].is_object()) {
        stack.m_customData = json["Tag"];
    }

    return stack;
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
    if (!m_lore.empty()) {
        std::vector<std::unique_ptr<text::ITextComponent>> loreCopy;
        loreCopy.reserve(m_lore.size());
        for (const auto& line : m_lore) {
            loreCopy.push_back(line ? line->deepCopy() : nullptr);
        }
        patch.add(DataComponentType::Lore, DataComponentPayload{std::in_place_index<3>, std::move(loreCopy)});
    }
    if (hasEnchantments()) {
        patch.add(DataComponentType::Enchantments, DataComponentPayload{std::in_place_index<4>, m_enchantments});
    }
    // PotionContents：potionId + customColor + customEffects + customName。
    // customColor/customEffects 在业务侧由 PotionUtils 走 m_customData JSON
    // （CustomPotionColor/CustomPotionEffects 键）承载，这里桥接到组件载荷以便 wire 传输。
    // customName（药水自定义名）项目暂无承载，保持 nullopt。
    const bool hasPotionFields = !m_potionId.empty() || potion::PotionUtils::hasCustomEffects(*this) ||
        potion::PotionUtils::getCustomPotionColor(*this).has_value();
    if (hasPotionFields) {
        PotionContentsPayload pc{};
        pc.potionId = m_potionId;
        if (const auto color = potion::PotionUtils::getCustomPotionColor(*this); color.has_value()) {
            pc.customColor = static_cast<i32>(static_cast<u32>(*color));
        }
        pc.customEffects = potion::PotionUtils::getCustomEffects(*this);
        patch.add(DataComponentType::PotionContents, DataComponentPayload{std::in_place_index<5>, std::move(pc)});
    }
    if (hasCanPlaceOn()) {
        patch.add(DataComponentType::CanPlaceOn, DataComponentPayload{std::in_place_index<6>, m_canPlaceOn});
    }
    if (hasCanDestroy()) {
        patch.add(DataComponentType::CanBreak, DataComponentPayload{std::in_place_index<6>, m_canDestroy});
    }
    if (hasTag()) {
        patch.add(DataComponentType::CustomData, DataComponentPayload{std::in_place_index<7>, m_customData});
    }
    return patch;
}

void ItemStack::applyComponentPatch(const item::component::DataComponentPatch& patch)
{
    using namespace item::component;
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
                m_lore.clear();
                for (const auto& line : lines) {
                    m_lore.push_back(line ? line->deepCopy() : nullptr);
                }
                break;
            }
            case DataComponentType::Enchantments: {
                m_enchantments = std::get<item::enchant::EnchantmentContainer>(entry.value);
                break;
            }
            case DataComponentType::PotionContents: {
                const auto& pc = std::get<PotionContentsPayload>(entry.value);
                m_potionId = pc.potionId;
                // customColor/customEffects 写回 m_customData JSON（与 PotionUtils 路径一致）。
                std::optional<u32> color{};
                if (pc.customColor.has_value()) {
                    color = static_cast<u32>(*pc.customColor);
                }
                potion::PotionUtils::setCustomPotionColor(*this, color);
                potion::PotionUtils::setCustomEffects(*this, pc.customEffects);
                break;
            }
            case DataComponentType::CanPlaceOn: {
                m_canPlaceOn = std::get<AdventureModePredicate>(entry.value);
                break;
            }
            case DataComponentType::CanBreak: {
                m_canDestroy = std::get<AdventureModePredicate>(entry.value);
                break;
            }
            case DataComponentType::CustomData: {
                m_customData = std::get<nlohmann::json>(entry.value);
                break;
            }
            case DataComponentType::MaxStackSize:
            case DataComponentType::MaxDamage:
            case DataComponentType::Enchantable:
            case DataComponentType::Unbreakable:
            case DataComponentType::ItemName:
            case DataComponentType::ItemModel:
            case DataComponentType::Rarity:
                // TODO: 未落地组件不映射到 ItemStack 字段。
                break;
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
                m_lore.clear();
                break;
            case DataComponentType::Enchantments:
                m_enchantments.clear();
                break;
            case DataComponentType::PotionContents:
                m_potionId.clear();
                break;
            case DataComponentType::CanPlaceOn:
                m_canPlaceOn = AdventureModePredicate{};
                break;
            case DataComponentType::CanBreak:
                m_canDestroy = AdventureModePredicate{};
                break;
            case DataComponentType::CustomData:
                m_customData = nlohmann::json{};
                break;
            case DataComponentType::MaxStackSize:
            case DataComponentType::MaxDamage:
            case DataComponentType::Enchantable:
            case DataComponentType::Unbreakable:
            case DataComponentType::ItemName:
            case DataComponentType::ItemModel:
            case DataComponentType::Rarity:
                // TODO: 未落地组件不映射到 ItemStack 字段。
                break;
        }
    }
}

// ============================================================================
// NBT 序列化（1.21.11 数据组件格式：{id, count:int, components}）
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

    // 数量（1.21.11：int）
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

    // 数量：1.21.11 用 "count"(int/byte)，旧格式用 "Count"(byte)
    i32 count = 1;
    auto countIt = tag.value.find(nbt_keys::COUNT);
    if (countIt == tag.value.end()) {
        countIt = tag.value.find(nbt_keys::LEGACY_COUNT);
    }
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

    // 优先按 1.21.11 组件格式解析；若无 components 但有 "tag"，回退旧 1.16.5 格式
    auto componentsIt = tag.value.find(nbt_keys::COMPONENTS);
    if (componentsIt != tag.value.end() && componentsIt->second->id() == nbt::TagId::Compound) {
        const auto& componentsCompound = dynamic_cast<const nbt::tags::compound_tag&>(*componentsIt->second);
        stack.applyComponentPatch(item::component::readPatchFromNbt(componentsCompound));
        return stack;
    }

    auto tagIt = tag.value.find(nbt_keys::TAG);
    if (tagIt == tag.value.end() || tagIt->second->id() != nbt::TagId::Compound) {
        return stack;
    }
    applyLegacyTagCompound(stack, dynamic_cast<const nbt::tags::compound_tag&>(*tagIt->second));
    return stack;
}

// ============================================================================
// 旧 1.16.5 {tag{...}} 格式回退读取（读旧存档用）
// ============================================================================

void ItemStack::applyLegacyTagCompound(ItemStack& stack, const nbt::tags::compound_tag& tagCompound)
{
    auto it = tagCompound.value.find(nbt_keys::DAMAGE);
    if (it != tagCompound.value.end()) {
        if (it->second->id() == nbt::TagId::Int) {
            stack.m_damage = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        } else if (it->second->id() == nbt::TagId::Short) {
            stack.m_damage = dynamic_cast<const nbt::tags::short_tag&>(*it->second).value;
        }
    }

    it = tagCompound.value.find(nbt_keys::ENCHANTMENTS);
    if (it != tagCompound.value.end() && it->second->id() == nbt::TagId::List) {
        auto& enchList = dynamic_cast<const nbt::tags::list_tag&>(*it->second);
        stack.m_enchantments = item::enchant::EnchantmentContainer::fromNbt(enchList);
    }

    it = tagCompound.value.find(nbt_keys::DISPLAY);
    if (it != tagCompound.value.end() && it->second->id() == nbt::TagId::Compound) {
        const auto& display = dynamic_cast<const nbt::tags::compound_tag&>(*it->second);

        auto nameIt = display.value.find(nbt_keys::NAME);
        if (nameIt != display.value.end() && nameIt->second->id() == nbt::TagId::String) {
            std::string nameJson = dynamic_cast<const nbt::tags::string_tag&>(*nameIt->second).value;
            stack.m_customName = text::TextParser::parse(nameJson);
        }

        auto loreIt = display.value.find(nbt_keys::LORE);
        if (loreIt != display.value.end() && loreIt->second->id() == nbt::TagId::List) {
            auto& loreList = dynamic_cast<const nbt::tags::list_tag&>(*loreIt->second);
            if (loreList.element_id() == nbt::TagId::String) {
                auto& stringList = dynamic_cast<const nbt::tags::string_list_tag&>(loreList);
                for (const auto& lineJson : stringList.value) {
                    stack.m_lore.push_back(text::TextParser::parse(lineJson));
                }
            }
        }
    }

    it = tagCompound.value.find(nbt_keys::REPAIR_COST);
    if (it != tagCompound.value.end() && it->second->id() == nbt::TagId::Int) {
        stack.m_repairCost = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
    }

    it = tagCompound.value.find(nbt_keys::POTION);
    if (it != tagCompound.value.end() && it->second->id() == nbt::TagId::String) {
        stack.m_potionId = dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;
    }

    it = tagCompound.value.find(nbt_keys::CAN_PLACE_ON);
    if (it != tagCompound.value.end() && it->second->id() == nbt::TagId::List) {
        auto& listTag = dynamic_cast<const nbt::tags::list_tag&>(*it->second);
        if (listTag.element_id() == nbt::TagId::String) {
            auto& stringList = dynamic_cast<const nbt::tags::string_list_tag&>(listTag);
            std::vector<std::string> predicates;
            predicates.reserve(stringList.value.size());
            for (const auto& blockId : stringList.value) {
                predicates.push_back(blockId);
            }
            stack.m_canPlaceOn = AdventureModePredicate(std::move(predicates));
        }
    }

    it = tagCompound.value.find(nbt_keys::CAN_DESTROY);
    if (it != tagCompound.value.end() && it->second->id() == nbt::TagId::List) {
        auto& listTag = dynamic_cast<const nbt::tags::list_tag&>(*it->second);
        if (listTag.element_id() == nbt::TagId::String) {
            auto& stringList = dynamic_cast<const nbt::tags::string_list_tag&>(listTag);
            std::vector<std::string> predicates;
            predicates.reserve(stringList.value.size());
            for (const auto& blockId : stringList.value) {
                predicates.push_back(blockId);
            }
            stack.m_canDestroy = AdventureModePredicate(std::move(predicates));
        }
    }

    // 旧格式 custom_data 存为 JSON 字符串
    it = tagCompound.value.find("custom_data");
    if (it != tagCompound.value.end() && it->second->id() == nbt::TagId::String) {
        std::string customDataStr = dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;
        auto parsed = nlohmann::json::parse(customDataStr, nullptr, false);
        if (!parsed.is_discarded() && parsed.is_object()) {
            stack.m_customData = parsed;
        }
    }
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

    // 比较 m_lore
    bool loreEqual = m_lore.size() == other.m_lore.size();
    if (loreEqual) {
        for (size_t i = 0; i < m_lore.size(); ++i) {
            if (m_lore[i] && other.m_lore[i]) {
                if (*m_lore[i] != *other.m_lore[i]) {
                    loreEqual = false;
                    break;
                }
            } else if (m_lore[i] || other.m_lore[i]) {
                loreEqual = false;
                break;
            }
        }
    }

    return m_item == other.m_item && m_count == other.m_count && m_damage == other.m_damage && customNameEqual &&
        loreEqual && m_potionId == other.m_potionId && m_customData == other.m_customData &&
        m_enchantments.getAll() == other.m_enchantments.getAll() && m_canPlaceOn == other.m_canPlaceOn &&
        m_canDestroy == other.m_canDestroy;
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
    return m_customData.is_object() && !m_customData.empty();
}

const nlohmann::json* ItemStack::getTag() const
{
    if (!hasTag()) {
        return nullptr;
    }

    return &m_customData;
}

nlohmann::json* ItemStack::getTag()
{
    if (!hasTag()) {
        return nullptr;
    }

    return &m_customData;
}

nlohmann::json& ItemStack::getOrCreateTag()
{
    if (!m_customData.is_object()) {
        m_customData = nlohmann::json::object();
    }

    return m_customData;
}

const nlohmann::json* ItemStack::getChildTag(const std::string& name) const
{
    if (!m_customData.is_object()) {
        return nullptr;
    }

    auto iter = m_customData.find(name);
    if (iter == m_customData.end() || !iter->is_object()) {
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
    if (!m_customData.is_object()) {
        return;
    }

    m_customData.erase(name);
    if (m_customData.empty()) {
        m_customData = nlohmann::json();
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
