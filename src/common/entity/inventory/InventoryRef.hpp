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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/entity/inventory/IInventory.hpp"
#include "entity/inventory/ISidedInventory.hpp"
#include <cstddef>
#include <memory>
#include <utility>

namespace mc {

/**
 * @brief 背包引用，管理可能拥有的背包指针
 *
 * 封装了两种背包指针的所有权语义：
 * - 非拥有引用：指向由其他系统（如方块实体、实体）管理的背包，不需要释放
 * - 拥有引用：通过 ISidedInventoryProvider 动态创建的背包，需要在作用域结束时释放
 *
 * 这个类型主要用于 HopperEntity::getInventoryAtPosition() 的返回值，
 * 解决了之前 ISidedInventoryProvider::createInventory() 返回的 unique_ptr
 * 被 release() 后导致内存泄漏的问题。
 *
 * 用法：
 * @code
 * InventoryRef ref = HopperEntity::getInventoryAtPosition(world, pos);
 * if (ref != nullptr) {
 *     // 通过 get() 访问 IInventory 接口
 *     ref->getItem(0);
 *     // 或者
 *     IInventory* inv = ref.get();
 * }
 * // ref 析构时，如果是拥有引用则自动释放
 * @endcode
 *
 * 参考: MC Java 中 HopperBlockEntity.getBlockContainer() 返回的 Container 引用
 *       在 Java 中由 GC 管理，C++ 中需要显式的所有权管理
 */
class InventoryRef {
public:
    /**
     * @brief 构造空引用（无背包）
     */
    InventoryRef() = default;

    /**
     * @brief 构造非拥有引用
     * @param inventory 由其他系统管理的背包指针，不需要释放
     */
    explicit InventoryRef(IInventory* inventory) noexcept
        : m_ptr(inventory)
    {}

    /**
     * @brief 构造拥有引用
     * @param owned 通过 ISidedInventoryProvider 动态创建的背包，需要释放
     */
    explicit InventoryRef(std::unique_ptr<ISidedInventory> owned) noexcept
        : m_owned(std::move(owned))
        , m_ptr(m_owned.get())
    {}

    // 支持移动语义
    InventoryRef(InventoryRef&& other) noexcept
        : m_owned(std::move(other.m_owned))
        , m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    InventoryRef& operator=(InventoryRef&& other) noexcept
    {
        if (this != &other) {
            m_owned = std::move(other.m_owned);
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    // 禁止拷贝（拥有语义不可拷贝）
    InventoryRef(const InventoryRef&) = delete;
    InventoryRef& operator=(const InventoryRef&) = delete;

    ~InventoryRef() = default;

    /**
     * @brief 获取背包指针
     * @return 背包指针，如果无背包返回 nullptr
     */
    [[nodiscard]] IInventory* get() const noexcept { return m_ptr; }

    /**
     * @brief 获取拥有的 ISidedInventory 指针（如果拥有）
     * @return 拥有的 ISidedInventory 指针，如果不拥有返回 nullptr
     */
    [[nodiscard]] ISidedInventory* ownedSidedInventory() const noexcept;

    /**
     * @brief 检查是否为空引用
     * @return 如果无背包返回 true
     */
    [[nodiscard]] bool isEmpty() const noexcept { return m_ptr == nullptr; }

    /**
     * @brief 检查是否拥有背包（需要释放）
     * @return 如果拥有背包返回 true
     */
    [[nodiscard]] bool isOwning() const noexcept { return m_owned != nullptr; }

    /**
     * @brief 通过指针访问背包
     */
    IInventory* operator->() const noexcept { return m_ptr; }

    /**
     * @brief 解引用访问背包
     */
    IInventory& operator*() const noexcept { return *m_ptr; }

    /**
     * @brief 隐式转换为 bool，检查是否有背包
     */
    explicit operator bool() const noexcept { return m_ptr != nullptr; }

    /**
     * @brief 与 nullptr 比较
     */
    bool operator==(std::nullptr_t) const noexcept { return m_ptr == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return m_ptr != nullptr; }

    /**
     * @brief 释放所有权，返回拥有的 unique_ptr
     *
     * 调用后 InventoryRef 变为空引用。
     * 如果不是拥有引用，返回 nullptr。
     *
     * @return 拥有的 unique_ptr，如果不拥有返回 nullptr
     */
    std::unique_ptr<ISidedInventory> releaseOwnership() noexcept
    {
        m_ptr = nullptr;
        return std::move(m_owned);
    }

private:
    std::unique_ptr<ISidedInventory> m_owned; ///< 拥有的背包（ISidedInventoryProvider 创建）
    IInventory* m_ptr = nullptr;              ///< 背包指针（可能指向 m_owned 或外部对象）
};

} // namespace mc
