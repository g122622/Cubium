/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include <initializer_list>
#include <list>
#include <unordered_map>
#include <utility>

namespace mc {

/**
 * @brief 保持插入顺序的哈希集合
 *
 * 结合了哈希表的 O(1) 查找/插入/删除性能与双向链表的插入顺序保证。
 * 参考 MC Java 的 ObjectLinkedOpenHashSet（fastutil）设计。
 *
 * 典型用途：需要按插入顺序遍历、同时需要快速成员检测的场景，
 * 如 VaultBlockEntity 的已奖励玩家列表（超上限时淘汰最早插入的玩家）。
 *
 * 用法示例：
 * @code
 * LinkedHashSet<std::string> players;
 * players.insert("uuid-1");
 * players.insert("uuid-2");
 * players.contains("uuid-1"); // true
 * players.erase(players.begin()); // 淘汰最早插入的元素
 * @endcode
 *
 * @tparam T 元素类型，需支持 std::hash 和 operator==
 */
template <typename T>
class LinkedHashSet {
public:
    // ========== 类型别名 ==========

    using key_type = T;
    using value_type = T;
    using size_type = typename std::list<T>::size_type;
    using difference_type = typename std::list<T>::difference_type;
    using reference = T&;
    using const_reference = const T&;
    using iterator = typename std::list<T>::iterator;
    using const_iterator = typename std::list<T>::const_iterator;

    // ========== 构造/析构 ==========

    LinkedHashSet() = default;
    LinkedHashSet(const LinkedHashSet&) = default;
    LinkedHashSet(LinkedHashSet&&) noexcept = default;
    LinkedHashSet& operator=(const LinkedHashSet&) = default;
    LinkedHashSet& operator=(LinkedHashSet&&) noexcept = default;
    ~LinkedHashSet() = default;

    /**
     * @brief 从迭代器范围构造
     */
    template <typename InputIt>
    LinkedHashSet(InputIt first, InputIt last)
    {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    /**
     * @brief 从初始化列表构造
     */
    LinkedHashSet(std::initializer_list<T> init)
    {
        for (const auto& value : init) {
            insert(value);
        }
    }

    // ========== 迭代器 ==========

    iterator begin() noexcept { return m_order.begin(); }
    const_iterator begin() const noexcept { return m_order.begin(); }
    const_iterator cbegin() const noexcept { return m_order.cbegin(); }

    iterator end() noexcept { return m_order.end(); }
    const_iterator end() const noexcept { return m_order.end(); }
    const_iterator cend() const noexcept { return m_order.cend(); }

    // ========== 容量 ==========

    [[nodiscard]] bool empty() const noexcept { return m_order.empty(); }
    [[nodiscard]] size_type size() const noexcept { return m_order.size(); }

    // ========== 修改器 ==========

    /**
     * @brief 插入元素（如果不存在）
     * @param value 要插入的值
     * @return pair<iterator, bool>：迭代器指向该元素，bool 表示是否是新插入的
     */
    std::pair<iterator, bool> insert(const T& value)
    {
        auto mapIt = m_lookup.find(value);
        if (mapIt != m_lookup.end()) {
            // 元素已存在，不修改顺序
            return {mapIt->second, false};
        }
        m_order.push_back(value);
        auto listIt = std::prev(m_order.end());
        m_lookup[value] = listIt;
        return {listIt, true};
    }

    /**
     * @brief 插入元素（移动语义）
     */
    std::pair<iterator, bool> insert(T&& value)
    {
        auto mapIt = m_lookup.find(value);
        if (mapIt != m_lookup.end()) {
            return {mapIt->second, false};
        }
        m_order.push_back(std::move(value));
        auto listIt = std::prev(m_order.end());
        // 注意：value 已被移动，不能再使用 value 作为 key
        // 但此时 listIt 指向的元素就是刚插入的，直接用它
        m_lookup[*listIt] = listIt;
        return {listIt, true};
    }

    /**
     * @brief 通过迭代器删除元素
     * @param pos 指向要删除元素的迭代器
     * @return 指向被删除元素之后元素的迭代器
     */
    iterator erase(iterator pos)
    {
        if (pos == m_order.end()) {
            return pos;
        }
        m_lookup.erase(*pos);
        return m_order.erase(pos);
    }

    /**
     * @brief 通过 const 迭代器删除元素
     */
    iterator erase(const_iterator pos)
    {
        if (pos == m_order.cend()) {
            return m_order.end();
        }
        m_lookup.erase(*pos);
        return m_order.erase(pos);
    }

    /**
     * @brief 通过值删除元素
     * @param value 要删除的值
     * @return 是否成功删除
     */
    bool erase(const T& value)
    {
        auto mapIt = m_lookup.find(value);
        if (mapIt == m_lookup.end()) {
            return false;
        }
        m_order.erase(mapIt->second);
        m_lookup.erase(mapIt);
        return true;
    }

    /**
     * @brief 清空集合
     */
    void clear() noexcept
    {
        m_order.clear();
        m_lookup.clear();
    }

    // ========== 查找 ==========

    /**
     * @brief 检查元素是否存在
     */
    [[nodiscard]] bool contains(const T& value) const { return m_lookup.find(value) != m_lookup.end(); }

    /**
     * @brief 返回匹配元素的迭代器，未找到则返回 end()
     */
    iterator find(const T& value)
    {
        auto mapIt = m_lookup.find(value);
        if (mapIt == m_lookup.end()) {
            return m_order.end();
        }
        return mapIt->second;
    }

    /**
     * @brief 返回匹配元素的 const 迭代器，未找到则返回 end()
     */
    [[nodiscard]] const_iterator find(const T& value) const
    {
        auto mapIt = m_lookup.find(value);
        if (mapIt == m_lookup.end()) {
            return m_order.cend();
        }
        return mapIt->second;
    }

    /**
     * @brief 返回匹配元素的数量（0 或 1）
     */
    [[nodiscard]] size_type count(const T& value) const { return m_lookup.count(value); }

private:
    std::list<T> m_order;                     ///< 保持插入顺序的双向链表
    std::unordered_map<T, iterator> m_lookup; ///< 值到链表迭代器的映射，实现 O(1) 查找
};

} // namespace mc
