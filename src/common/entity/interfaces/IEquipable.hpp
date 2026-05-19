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

#include "common/core/Types.hpp"

namespace mc {

// Forward declarations
class ItemStack;

namespace entity {

/**
 * @brief 可装备接口 - 用于可以被装备物品的实体
 *
 * 实现此接口的实体可以装备鞍、马铠等物品。
 * 例如：猪、马、驴等。
 *
 * 参考 MC 1.16.5 IEquipable
 */
class IEquipable {
public:
    virtual ~IEquipable() = default;

    /**
     * @brief 获取装备槽数量
     * @return 装备槽数量
     */
    [[nodiscard]] virtual i32 getEquipmentSlotCount() const = 0;

    /**
     * @brief 获取指定槽位的装备
     * @param slot 槽位索引 (0-based)
     * @return 装备物品（空堆表示无装备）
     */
    [[nodiscard]] virtual ItemStack getEquipment(i32 slot) const = 0;

    /**
     * @brief 设置指定槽位的装备
     * @param slot 槽位索引
     * @param item 物品
     */
    virtual void setEquipment(i32 slot, const ItemStack& item) = 0;

    /**
     * @brief 检查是否可以装备指定物品
     * @param item 要检查的物品
     * @param slot 目标槽位
     * @return 如果可以装备返回true
     */
    [[nodiscard]] virtual bool canEquip(const ItemStack& item, i32 slot) const = 0;
};

} // namespace entity
} // namespace mc
