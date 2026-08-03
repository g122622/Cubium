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

#include "BundleContents.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item::items {

// ============================================================================
// 静态成员
// ============================================================================

const BundleContents BundleContents::EMPTY;

// ============================================================================
// 构造
// ============================================================================

BundleContents::BundleContents(std::vector<ItemStack> items)
    : m_items(std::move(items))
    , m_weight(computeContentWeight(m_items))
    , m_selectedItem(NO_SELECTED_ITEM)
{}

BundleContents::BundleContents(std::vector<ItemStack> items, i64 weight, i32 selectedItem)
    : m_items(std::move(items))
    , m_weight(weight)
    , m_selectedItem(selectedItem)
{}

// ============================================================================
// 查询
// ============================================================================

std::vector<ItemStack> BundleContents::itemsCopy() const
{
    std::vector<ItemStack> copies;
    copies.reserve(m_items.size());
    for (const auto& item : m_items) {
        copies.push_back(item.copy());
    }
    return copies;
}

i32 BundleContents::numberOfItemsToShow() const
{
    // 对应 MC 1.21.11 BundleContents#getNumberOfItemsToShow
    i32 size = static_cast<i32>(m_items.size());
    i32 j = size > 12 ? 11 : 12;
    i32 k = size % 4;
    i32 l = (k == 0) ? 0 : (4 - k);
    return std::min(size, j - l);
}

// ============================================================================
// 静态工具
// ============================================================================

i64 BundleContents::computeContentWeight(const std::vector<ItemStack>& items)
{
    i64 total = 0;
    for (const auto& stack : items) {
        // 单个物品权重 × 数量
        i64 perItem = getWeight(stack);
        i64 count = stack.getCount();
        // 防止溢出（理论上 64×64=4096 不会溢出 i64）
        total += perItem * count;
    }
    return total;
}

i64 BundleContents::getWeight(const ItemStack& stack)
{
    // 对应 MC 1.21.11 BundleContents#getWeight
    if (stack.isEmpty()) {
        return 0;
    }

    // 通过物品 ID 识别收纳袋（"bundle" 或 "*_bundle"）
    // 这与 BUNDLES 物品标签的成员一致，避免循环依赖 BundleItem 头文件。
    const ResourceLocation& id = stack.getItem()->itemLocation();
    const std::string& path = id.path();
    bool isBundle = (path == "bundle") || (path.size() > 7 && path.compare(path.size() - 7, 7, "_bundle") == 0);
    if (isBundle) {
        // 收纳袋嵌套权重 = BUNDLE_IN_BUNDLE_WEIGHT + 内袋权重
        BundleContents inner = fromItemStack(stack);
        return BUNDLE_IN_BUNDLE_WEIGHT + inner.weight();
    }

    // 普通物品：64 / maxStackSize（向上取整避免 0）
    i32 maxStack = stack.getMaxStackSize();
    if (maxStack <= 0) {
        return MAX_WEIGHT; // 异常物品视为满权重
    }
    // 64 / maxStackSize，向上取整
    return (MAX_WEIGHT + maxStack - 1) / maxStack;
}

bool BundleContents::canItemBeInBundle(const ItemStack& stack)
{
    // 对应 MC 1.21.11 BundleContents#canItemBeInBundle
    return !stack.isEmpty() && stack.getItem()->canFitInsideContainerItems();
}

BundleContents BundleContents::fromItemStack(const ItemStack& stack)
{
    // 从物品堆的 NBT 读取 BundleContents
    // NBT 路径：tag.custom_data.BundleContents
    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr) {
        return EMPTY;
    }
    auto it = tag->find("BundleContents");
    if (it == tag->end() || !it->is_object()) {
        return EMPTY;
    }
    return fromJson(*it);
}

// ============================================================================
// 序列化
// ============================================================================

nlohmann::json BundleContents::toJson() const
{
    nlohmann::json json = nlohmann::json::object();
    nlohmann::json itemsArray = nlohmann::json::array();
    for (const auto& item : m_items) {
        itemsArray.push_back(item.toJson());
    }
    json["items"] = std::move(itemsArray);
    json["weight"] = m_weight;
    json["selected"] = m_selectedItem;
    return json;
}

BundleContents BundleContents::fromJson(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return EMPTY;
    }

    std::vector<ItemStack> items;
    auto itemsIt = json.find("items");
    if (itemsIt != json.end() && itemsIt->is_array()) {
        for (const auto& elem : *itemsIt) {
            auto result = ItemStack::fromJson(elem);
            if (result.success()) {
                items.push_back(std::move(result).value());
            }
        }
    }

    i64 weight = 0;
    auto weightIt = json.find("weight");
    if (weightIt != json.end() && weightIt->is_number_integer()) {
        weight = weightIt->get<i64>();
    } else {
        // 权重缺失时重新计算
        weight = computeContentWeight(items);
    }

    i32 selected = NO_SELECTED_ITEM;
    auto selectedIt = json.find("selected");
    if (selectedIt != json.end() && selectedIt->is_number_integer()) {
        selected = selectedIt->get<i32>();
    }

    return BundleContents(std::move(items), weight, selected);
}

// ============================================================================
// 比较
// ============================================================================

bool BundleContents::operator==(const BundleContents& other) const
{
    if (m_weight != other.m_weight) {
        return false;
    }
    if (m_selectedItem != other.m_selectedItem) {
        return false;
    }
    if (m_items.size() != other.m_items.size()) {
        return false;
    }
    for (Size i = 0; i < m_items.size(); ++i) {
        if (!(m_items[i] == other.m_items[i])) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Mutable
// ============================================================================

BundleContents::Mutable::Mutable(BundleContents contents)
    : m_items(std::move(contents.m_items))
    , m_weight(contents.m_weight)
    , m_selectedItem(contents.m_selectedItem)
{}

BundleContents::Mutable& BundleContents::Mutable::clearItems()
{
    m_items.clear();
    m_weight = 0;
    m_selectedItem = NO_SELECTED_ITEM;
    return *this;
}

i32 BundleContents::Mutable::findStackIndex(const ItemStack& stack) const
{
    // 对应 MC 1.21.11 BundleContents.Mutable#findStackIndex
    if (!stack.isStackable()) {
        return -1;
    }
    for (Size i = 0; i < m_items.size(); ++i) {
        if (m_items[i].canMergeWith(stack)) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

i32 BundleContents::Mutable::getMaxAmountToAdd(const ItemStack& stack) const
{
    // 对应 MC 1.21.11 BundleContents.Mutable#getMaxAmountToAdd
    // 剩余权重 / 单个物品权重
    i64 remaining = MAX_WEIGHT - m_weight;
    if (remaining <= 0) {
        return 0;
    }
    i64 perItem = getWeight(stack);
    if (perItem <= 0) {
        // 零权重物品理论上无限可加，但限制为堆叠上限
        return stack.getMaxStackSize();
    }
    return static_cast<i32>(std::min<i64>(remaining / perItem, stack.getMaxStackSize()));
}

i32 BundleContents::Mutable::tryInsert(ItemStack& stack)
{
    // 对应 MC 1.21.11 BundleContents.Mutable#tryInsert
    if (!canItemBeInBundle(stack)) {
        return 0;
    }
    i32 toAdd = std::min(stack.getCount(), getMaxAmountToAdd(stack));
    if (toAdd == 0) {
        return 0;
    }

    // 更新权重
    i64 perItem = getWeight(stack);
    m_weight += perItem * toAdd;

    i32 existingIndex = findStackIndex(stack);
    if (existingIndex != -1) {
        // 已存在可合并堆：移除旧堆，增加数量后放到最前
        ItemStack existing = std::move(m_items[existingIndex]);
        m_items.erase(m_items.begin() + existingIndex);
        existing.grow(toAdd);
        stack.shrink(toAdd);
        m_items.insert(m_items.begin(), std::move(existing));
    } else {
        // 新堆：分出 toAdd 个放到最前
        ItemStack split = stack.split(toAdd);
        m_items.insert(m_items.begin(), std::move(split));
    }

    return toAdd;
}

i32 BundleContents::Mutable::tryTransfer(Slot& slot, Player& player)
{
    // 对应 MC 1.21.11 BundleContents.Mutable#tryTransfer
    ItemStack slotStack = slot.getItem();
    i32 maxAdd = getMaxAmountToAdd(slotStack);
    if (!canItemBeInBundle(slotStack)) {
        return 0;
    }
    // safeTake 会从槽位取出物品并触发 onTake
    ItemStack taken = slot.safeTake(slotStack.getCount(), maxAdd, player);
    if (taken.isEmpty()) {
        return 0;
    }
    return tryInsert(taken);
}

void BundleContents::Mutable::toggleSelectedItem(i32 index)
{
    // 对应 MC 1.21.11 BundleContents.Mutable#toggleSelectedItem
    bool isOutside = (index < 0 || index >= static_cast<i32>(m_items.size()));
    if (m_selectedItem != index && !isOutside) {
        m_selectedItem = index;
    } else {
        m_selectedItem = NO_SELECTED_ITEM;
    }
}

std::optional<ItemStack> BundleContents::Mutable::removeOne()
{
    // 对应 MC 1.21.11 BundleContents.Mutable#removeOne
    if (m_items.empty()) {
        return std::nullopt;
    }
    i32 index = (m_selectedItem < 0 || m_selectedItem >= static_cast<i32>(m_items.size())) ? 0 : m_selectedItem;
    ItemStack removed = std::move(m_items[index]);
    m_items.erase(m_items.begin() + index);

    // 重新计算权重
    i64 perItem = getWeight(removed);
    m_weight -= perItem * removed.getCount();
    if (m_weight < 0) {
        m_weight = 0; // 防御性：避免负权重
    }

    toggleSelectedItem(NO_SELECTED_ITEM);
    return removed;
}

BundleContents BundleContents::Mutable::toImmutable() const
{
    return BundleContents(m_items, m_weight, m_selectedItem);
}

} // namespace item::items
} // namespace mc
