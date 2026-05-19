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

#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "Item.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace mc {

/**
 * @brief 物品注册表
 *
 * 单例模式，管理所有物品的注册和查找。
 * 参考: net.minecraft.item.Item.Registry / Registry.ITEM
 *
 * 用法示例:
 * @code
 * // 注册物品
 * auto& stick = ItemRegistry::instance().registerItem(
 *     ResourceLocation("minecraft:stick"),
 *     ItemProperties().maxStackSize(64)
 * );
 *
 * // 获取物品
 * Item* item = ItemRegistry::instance().getItem(
 *     ResourceLocation("minecraft:stick")
 * );
 *
 * // 通过ID获取
 * Item* item = ItemRegistry::instance().getItem(1);
 * @endcode
 */
class ItemRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static ItemRegistry& instance();

    // 禁止拷贝和移动
    ItemRegistry(const ItemRegistry&) = delete;
    ItemRegistry& operator=(const ItemRegistry&) = delete;

    /**
     * @brief 注册物品
     * @tparam ItemType 物品类型（必须继承自Item）
     * @param id 物品资源位置
     * @param args 物品构造函数参数（ItemProperties之后的参数）
     * @return 物品引用
     */
    template <typename ItemType, typename... Args>
    ItemType& registerItem(const ResourceLocation& id, Args&&... args)
    {
        static_assert(std::is_base_of_v<Item, ItemType>, "ItemType must inherit from Item");

        // 避免重复注册导致旧指针失效
        auto existingIt = m_items.find(id);
        if (existingIt != m_items.end()) {
            auto* existing = dynamic_cast<ItemType*>(existingIt->second.get());
            if (existing == nullptr) {
                throw std::logic_error("Item id already registered with different type: " + id.toString());
            }
            return *existing;
        }

        // 使用辅助函数创建实例（绕过protected构造函数限制）
        auto item = createItem<ItemType>(std::forward<Args>(args)...);
        item->m_itemLocation = id;

        // 分配物品ID
        ItemId itemId = allocateItemId();
        item->m_itemId = itemId;

        ItemType* ptr = item.get();

        // 确保 vector 大小足够
        if (m_itemsById.size() <= itemId) {
            m_itemsById.resize(itemId + 1, nullptr);
        }
        m_itemsById[itemId] = ptr;
        m_items[id] = std::move(item);

        return *ptr;
    }

    /**
     * @brief 注册简单物品
     * @param id 物品资源位置
     * @param properties 物品属性
     * @return 物品引用
     */
    Item& registerItem(const ResourceLocation& id, ItemProperties properties);

    /**
     * @brief 根据ID获取物品
     * @param itemId 物品ID
     * @return 物品指针，不存在返回nullptr
     */
    [[nodiscard]] Item* getItem(ItemId itemId) const
    {
        if (itemId >= m_itemsById.size()) {
            return nullptr;
        }
        return m_itemsById[itemId];
    }

    /**
     * @brief 根据资源位置获取物品
     * @param id 物品资源位置
     * @return 物品指针，不存在返回nullptr
     */
    [[nodiscard]] Item* getItem(const ResourceLocation& id) const
    {
        auto it = m_items.find(id);
        return it != m_items.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 遍历所有物品
     */
    void forEachItem(std::function<void(Item&)> callback)
    {
        for (auto& [id, item] : m_items) {
            callback(*item);
        }
    }

    /**
     * @brief 获取物品数量
     */
    [[nodiscard]] size_t itemCount() const { return m_items.size(); }

    /**
     * @brief 获取空气物品（表示空物品）
     */
    [[nodiscard]] Item* airItem() const { return m_airItem; }

    /**
     * @brief 检查物品ID是否存在
     */
    [[nodiscard]] bool hasItem(ItemId itemId) const
    {
        return itemId < m_itemsById.size() && m_itemsById[itemId] != nullptr;
    }

    /**
     * @brief 检查资源位置是否存在
     */
    [[nodiscard]] bool hasItem(const ResourceLocation& id) const { return m_items.find(id) != m_items.end(); }

private:
    ItemRegistry();
    ~ItemRegistry() = default;

    /**
     * @brief 分配物品ID
     */
    ItemId allocateItemId();

    /**
     * @brief 创建物品实例的辅助函数
     *
     * 用于绕过 std::make_unique 无法访问 protected 构造函数的问题
     */
    template <typename ItemType, typename... Args>
    std::unique_ptr<ItemType> createItem(Args&&... args)
    {
        return std::unique_ptr<ItemType>(new ItemType(std::forward<Args>(args)...));
    }

    std::unordered_map<ResourceLocation, std::unique_ptr<Item>> m_items;
    std::vector<Item*> m_itemsById;
    ItemId m_nextItemId = 1; // 0保留给空气
    Item* m_airItem = nullptr;
};

} // namespace mc
