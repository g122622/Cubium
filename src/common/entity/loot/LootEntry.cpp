#include "LootEntry.hpp"
#include "LootConditions.hpp"
#include "LootTable.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include <algorithm>

namespace mc {
namespace loot {

// ============================================================================
// LootEntry
// ============================================================================

LootEntry::~LootEntry() = default;

void LootEntry::addCondition(std::unique_ptr<LootCondition> condition) {
    m_conditions.push_back(std::move(condition));
}

bool LootEntry::testConditions(LootContext& context) const {
    return std::all_of(m_conditions.begin(), m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) {
            return cond && cond->test(context);
        });
}

// ============================================================================
// EmptyLootEntry
// ============================================================================

std::unique_ptr<LootEntry> EmptyLootEntry::clone() const {
    auto entry = std::make_unique<EmptyLootEntry>(m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    return entry;
}

void EmptyLootEntry::expand(LootContext& /*context*/,
                           std::function<void(LootEntry&)> consumer) const {
    // 空条目仍然可以被选择，但不生成任何物品
    consumer(*const_cast<EmptyLootEntry*>(this));
}

bool EmptyLootEntry::generate(std::function<void(const ItemStack&)> /*consumer*/,
                              LootContext& /*context*/) const {
    // 空条目不生成物品，但返回true表示"成功"（可用于条件判断）
    return true;
}

// ============================================================================
// ItemLootEntry
// ============================================================================

ItemLootEntry::ItemLootEntry(const String& itemId,
                             const RandomValueRange& count,
                             i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_itemId(itemId)
    , m_count(count)
{
}

std::unique_ptr<LootEntry> ItemLootEntry::clone() const {
    auto entry = std::make_unique<ItemLootEntry>(m_itemId, m_count, m_weight, m_quality);
    // 复制条件
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    return entry;
}

void ItemLootEntry::expand(LootContext& /*context*/,
                          std::function<void(LootEntry&)> consumer) const {
    consumer(*const_cast<ItemLootEntry*>(this));
}

bool ItemLootEntry::generate(std::function<void(const ItemStack&)> consumer,
                             LootContext& context) const {
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 获取物品
    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(m_itemId));
    if (!item) {
        return false;
    }

    // 计算数量
    i32 count = m_count.generateInt(context.getRandom());
    if (count <= 0) {
        return true;  // 数量为0不算失败
    }

    // 创建物品堆
    ItemStack stack(*item, count);
    consumer(stack);

    return true;
}

// ============================================================================
// TableLootEntry
// ============================================================================

TableLootEntry::TableLootEntry(const String& tableId, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_tableId(tableId)
{
}

std::unique_ptr<LootEntry> TableLootEntry::clone() const {
    auto entry = std::make_unique<TableLootEntry>(m_tableId, m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    return entry;
}

void TableLootEntry::expand(LootContext& /*context*/,
                           std::function<void(LootEntry&)> consumer) const {
    consumer(*const_cast<TableLootEntry*>(this));
}

bool TableLootEntry::generate(std::function<void(const ItemStack&)> consumer,
                              LootContext& context) const {
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 获取引用的掉落表
    const LootTable* table = context.getLootTable(m_tableId);
    if (!table) {
        return false;
    }

    // 生成掉落物
    auto items = table->generate(context);
    for (const auto& item : items) {
        consumer(item);
    }

    return !items.empty();
}

// ============================================================================
// AlternativesLootEntry
// ============================================================================

AlternativesLootEntry::AlternativesLootEntry(std::vector<std::unique_ptr<LootEntry>> children)
    : m_children(std::move(children))
{
}

std::unique_ptr<LootEntry> AlternativesLootEntry::clone() const {
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<AlternativesLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    return entry;
}

void AlternativesLootEntry::addChild(std::unique_ptr<LootEntry> child) {
    m_children.push_back(std::move(child));
}

void AlternativesLootEntry::expand(LootContext& /*context*/,
                                   std::function<void(LootEntry&)> consumer) const {
    consumer(*const_cast<AlternativesLootEntry*>(this));
}

bool AlternativesLootEntry::generate(std::function<void(const ItemStack&)> consumer,
                                     LootContext& context) const {
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 尝试每个子条目，直到一个成功
    for (auto& child : m_children) {
        if (child->generate(consumer, context)) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// SequenceLootEntry
// ============================================================================

SequenceLootEntry::SequenceLootEntry(std::vector<std::unique_ptr<LootEntry>> children)
    : m_children(std::move(children))
{
}

std::unique_ptr<LootEntry> SequenceLootEntry::clone() const {
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<SequenceLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    return entry;
}

void SequenceLootEntry::addChild(std::unique_ptr<LootEntry> child) {
    m_children.push_back(std::move(child));
}

void SequenceLootEntry::expand(LootContext& /*context*/,
                               std::function<void(LootEntry&)> consumer) const {
    consumer(*const_cast<SequenceLootEntry*>(this));
}

bool SequenceLootEntry::generate(std::function<void(const ItemStack&)> consumer,
                                 LootContext& context) const {
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 按顺序执行所有子条目，直到一个失败
    for (auto& child : m_children) {
        if (!child->generate(consumer, context)) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// GroupLootEntry
// ============================================================================

GroupLootEntry::GroupLootEntry(std::vector<std::unique_ptr<LootEntry>> children)
    : m_children(std::move(children))
{
}

std::unique_ptr<LootEntry> GroupLootEntry::clone() const {
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<GroupLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    return entry;
}

void GroupLootEntry::addChild(std::unique_ptr<LootEntry> child) {
    m_children.push_back(std::move(child));
}

void GroupLootEntry::expand(LootContext& /*context*/,
                            std::function<void(LootEntry&)> consumer) const {
    consumer(*const_cast<GroupLootEntry*>(this));
}

bool GroupLootEntry::generate(std::function<void(const ItemStack&)> consumer,
                              LootContext& context) const {
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 执行所有子条目
    bool anySuccess = false;
    for (auto& child : m_children) {
        if (child->generate(consumer, context)) {
            anySuccess = true;
        }
    }
    return anySuccess;
}

// ============================================================================
// LootEntryBuilder
// ============================================================================

LootEntryBuilder LootEntryBuilder::item(const String& itemId) {
    LootEntryBuilder builder;
    builder.m_itemId = itemId;
    builder.m_type = LootEntryType::Item;
    return builder;
}

LootEntryBuilder LootEntryBuilder::empty() {
    LootEntryBuilder builder;
    builder.m_type = LootEntryType::Empty;
    return builder;
}

LootEntryBuilder LootEntryBuilder::table(const String& tableId) {
    LootEntryBuilder builder;
    builder.m_tableId = tableId;
    builder.m_type = LootEntryType::Table;
    return builder;
}

LootEntryBuilder& LootEntryBuilder::count(f32 min, f32 max) {
    m_count = RandomValueRange(min, max);
    return *this;
}

LootEntryBuilder& LootEntryBuilder::count(i32 value) {
    m_count = RandomValueRange(static_cast<f32>(value), static_cast<f32>(value));
    return *this;
}

std::unique_ptr<LootEntry> LootEntryBuilder::build() const {
    switch (m_type) {
        case LootEntryType::Item:
            return std::make_unique<ItemLootEntry>(m_itemId, m_count, m_weight, m_quality);
        case LootEntryType::Table:
            return std::make_unique<TableLootEntry>(m_tableId, m_weight, m_quality);
        case LootEntryType::Empty:
        default:
            return std::make_unique<EmptyLootEntry>(m_weight, m_quality);
    }
}

} // namespace loot
} // namespace mc
