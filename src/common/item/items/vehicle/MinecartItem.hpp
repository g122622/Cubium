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

#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"

namespace mc {
namespace item {

/**
 * @brief 矿车物品
 *
 * 用于放置各种类型的矿车。
 */
class MinecartItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param minecartType 矿车类型
     * @param properties 物品属性
     */
    MinecartItem(entity::AbstractMinecartEntity::Type minecartType, const ItemProperties& properties);

    ~MinecartItem() override = default;

    /**
     * @brief 在方块上使用物品
     *
     * 在铁轨上放置矿车
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 获取矿车类型
     */
    [[nodiscard]] entity::AbstractMinecartEntity::Type getMinecartType() const { return m_minecartType; }

private:
    entity::AbstractMinecartEntity::Type m_minecartType;
};

} // namespace item
} // namespace mc
