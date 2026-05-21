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

#include "LootEntry.hpp"
#include "LootConditions.hpp"
#include "LootFunctions.hpp"
#include "LootTable.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include <algorithm>

namespace mc {
namespace loot {

// ============================================================================
// LootEntry
// ============================================================================

LootEntry::~LootEntry() = default;

void LootEntry::addCondition(std::unique_ptr<LootCondition> condition)
{
    m_conditions.push_back(std::move(condition));
}

bool LootEntry::testConditions(LootContext& context) const
{
    return std::all_of(m_conditions.begin(),
        m_conditions.end(),
        [&context](const std::unique_ptr<LootCondition>& cond) { return cond && cond->test(context); });
}

void LootEntry::addFunction(std::unique_ptr<LootFunction> function)
{
    m_functions.push_back(std::move(function));
}

ItemStack LootEntry::applyFunctions(ItemStack stack, LootContext& context) const
{
    for (const auto& func : m_functions) {
        if (func && func->testConditions(context)) {
            stack = func->apply(std::move(stack), context);
            if (stack.isEmpty()) {
                break; // 函数可以返回空堆来取消掉落
            }
        }
    }
    return stack;
}

// ============================================================================
// EmptyLootEntry
// ============================================================================

std::unique_ptr<LootEntry> EmptyLootEntry::clone() const
{
    auto entry = std::make_unique<EmptyLootEntry>(m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void EmptyLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    // 空条目仍然可以被选择，但不生成任何物品
    consumer(*const_cast<EmptyLootEntry*>(this));
}

bool EmptyLootEntry::generate(std::function<void(const ItemStack&)> /*consumer*/, LootContext& /*context*/) const
{
    // 空条目不生成物品，但返回true表示"成功"（可用于条件判断）
    return true;
}

// ============================================================================
// ItemLootEntry
// ============================================================================

ItemLootEntry::ItemLootEntry(const std::string& itemId, const RandomValueRange& count, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_itemId(itemId)
    , m_count(count)
{}

std::unique_ptr<LootEntry> ItemLootEntry::clone() const
{
    auto entry = std::make_unique<ItemLootEntry>(m_itemId, m_count, m_weight, m_quality);
    // 复制条件
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    // 复制函数
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void ItemLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<ItemLootEntry*>(this));
}

bool ItemLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
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
        return true; // 数量为0不算失败
    }

    // 创建物品堆
    ItemStack stack(*item, count);

    // 应用条目级函数
    stack = applyFunctions(std::move(stack), context);

    // 如果函数返回空堆，则不生成物品
    if (!stack.isEmpty()) {
        consumer(stack);
    }

    return true;
}

// ============================================================================
// TableLootEntry
// ============================================================================

TableLootEntry::TableLootEntry(const std::string& tableId, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_tableId(tableId)
{}

std::unique_ptr<LootEntry> TableLootEntry::clone() const
{
    auto entry = std::make_unique<TableLootEntry>(m_tableId, m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void TableLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<TableLootEntry*>(this));
}

bool TableLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
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
// TagLootEntry
// ============================================================================

TagLootEntry::TagLootEntry(const std::string& tagId, bool expand, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_tagId(tagId)
    , m_expand(expand)
{}

std::unique_ptr<LootEntry> TagLootEntry::clone() const
{
    auto entry = std::make_unique<TagLootEntry>(m_tagId, m_expand, m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void TagLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<TagLootEntry*>(this));
}

bool TagLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 查找标签
    auto* tag = item::tag::ItemTags::getTag(m_tagId);
    if (!tag || tag->getItems().empty()) {
        // 标签不存在或为空，不生成物品
        return false;
    }

    const auto& items = tag->getItems();
    if (m_expand) {
        bool anyGenerated = false;
        for (const Item* item : items) {
            ItemStack stack(*item, 1);
            stack = applyFunctions(std::move(stack), context);
            if (!stack.isEmpty()) {
                consumer(stack);
                anyGenerated = true;
            }
        }
        return anyGenerated;
    }

    auto it = items.begin();
    std::advance(it, context.getRandom().nextInt(static_cast<i32>(items.size())));
    ItemStack stack(**it, 1);
    stack = applyFunctions(std::move(stack), context);
    if (!stack.isEmpty()) {
        consumer(stack);
    }
    return true;
}

// ============================================================================
// DynamicLootEntry
// ============================================================================

DynamicLootEntry::DynamicLootEntry(const std::string& name, i32 weight, i32 quality)
    : LootEntry(weight, quality)
    , m_name(name)
{}

std::unique_ptr<LootEntry> DynamicLootEntry::clone() const
{
    auto entry = std::make_unique<DynamicLootEntry>(m_name, m_weight, m_quality);
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void DynamicLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    // 动态条目不展开，自身作为候选条目
    consumer(*const_cast<DynamicLootEntry*>(this));
}

bool DynamicLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 检查条件
    if (!testConditions(context)) {
        return false;
    }

    // 目前仅支持 minecraft:contents（从容器方块实体中读取物品）
    if (m_name == "minecraft:contents" || m_name == "contents") {
        // 从上下文获取方块实体
        auto* blockEntity = context.get<BlockEntity>(LootParams::BLOCK_ENTITY);
        if (blockEntity) {
            auto* containerEntity = dynamic_cast<ContainerBlockEntity*>(blockEntity);
            if (containerEntity) {
                IInventory* inventory = containerEntity->getInventory();
                if (inventory) {
                    bool anyItems = false;
                    for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
                        ItemStack stack = inventory->getItem(i);
                        if (!stack.isEmpty()) {
                            stack = applyFunctions(std::move(stack), context);
                            if (!stack.isEmpty()) {
                                consumer(stack);
                                anyItems = true;
                            }
                        }
                    }
                    return anyItems;
                }
            }
        }
        // 无方块实体或非容器，返回 false
        return false;
    }

    // 未知动态名称，不生成物品
    return false;
}

// ============================================================================
// AlternativesLootEntry
// ============================================================================

AlternativesLootEntry::AlternativesLootEntry(std::vector<std::unique_ptr<LootEntry>> children)
    : m_children(std::move(children))
{}

std::unique_ptr<LootEntry> AlternativesLootEntry::clone() const
{
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<AlternativesLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void AlternativesLootEntry::addChild(std::unique_ptr<LootEntry> child)
{
    m_children.push_back(std::move(child));
}

void AlternativesLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<AlternativesLootEntry*>(this));
}

bool AlternativesLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
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
{}

std::unique_ptr<LootEntry> SequenceLootEntry::clone() const
{
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<SequenceLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void SequenceLootEntry::addChild(std::unique_ptr<LootEntry> child)
{
    m_children.push_back(std::move(child));
}

void SequenceLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<SequenceLootEntry*>(this));
}

bool SequenceLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
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
{}

std::unique_ptr<LootEntry> GroupLootEntry::clone() const
{
    std::vector<std::unique_ptr<LootEntry>> clonedChildren;
    for (const auto& child : m_children) {
        clonedChildren.push_back(child->clone());
    }
    auto entry = std::make_unique<GroupLootEntry>(std::move(clonedChildren));
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }
    return entry;
}

void GroupLootEntry::addChild(std::unique_ptr<LootEntry> child)
{
    m_children.push_back(std::move(child));
}

void GroupLootEntry::expand(LootContext& /*context*/, std::function<void(LootEntry&)> consumer) const
{
    consumer(*const_cast<GroupLootEntry*>(this));
}

bool GroupLootEntry::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
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

LootEntryBuilder LootEntryBuilder::item(const std::string& itemId)
{
    LootEntryBuilder builder;
    builder.m_itemId = itemId;
    builder.m_type = LootEntryType::Item;
    return builder;
}

LootEntryBuilder LootEntryBuilder::empty()
{
    LootEntryBuilder builder;
    builder.m_type = LootEntryType::Empty;
    return builder;
}

LootEntryBuilder LootEntryBuilder::table(const std::string& tableId)
{
    LootEntryBuilder builder;
    builder.m_tableId = tableId;
    builder.m_type = LootEntryType::Table;
    return builder;
}

LootEntryBuilder LootEntryBuilder::tag(const std::string& tagId, bool expand)
{
    LootEntryBuilder builder;
    builder.m_tagId = tagId;
    builder.m_expand = expand;
    builder.m_type = LootEntryType::Tag;
    return builder;
}

LootEntryBuilder LootEntryBuilder::dynamic_(const std::string& name)
{
    LootEntryBuilder builder;
    builder.m_dynamicName = name;
    builder.m_type = LootEntryType::Dynamic;
    return builder;
}

LootEntryBuilder& LootEntryBuilder::count(f32 min, f32 max)
{
    m_count = RandomValueRange(min, max);
    return *this;
}

LootEntryBuilder& LootEntryBuilder::count(i32 value)
{
    m_count = RandomValueRange(static_cast<f32>(value), static_cast<f32>(value));
    return *this;
}

std::unique_ptr<LootEntry> LootEntryBuilder::build() const
{
    std::unique_ptr<LootEntry> entry;

    switch (m_type) {
        case LootEntryType::Item:
            entry = std::make_unique<ItemLootEntry>(m_itemId, m_count, m_weight, m_quality);
            break;
        case LootEntryType::Table:
            entry = std::make_unique<TableLootEntry>(m_tableId, m_weight, m_quality);
            break;
        case LootEntryType::Tag:
            entry = std::make_unique<TagLootEntry>(m_tagId, m_expand, m_weight, m_quality);
            break;
        case LootEntryType::Dynamic:
            entry = std::make_unique<DynamicLootEntry>(m_dynamicName, m_weight, m_quality);
            break;
        case LootEntryType::Empty:
        default:
            entry = std::make_unique<EmptyLootEntry>(m_weight, m_quality);
            break;
    }

    // 添加条件
    for (const auto& cond : m_conditions) {
        entry->addCondition(cond->clone());
    }

    // 添加函数
    for (const auto& func : m_functions) {
        entry->addFunction(func->clone());
    }

    return entry;
}

} // namespace loot
} // namespace mc
