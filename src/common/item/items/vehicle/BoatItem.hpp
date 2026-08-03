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

#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"

namespace mc {
namespace item {

/**
 * @brief 船物品类
 *
 * 继承自 Item 基类，实现放置船实体的功能。
 * 同时用于普通船和带箱子的船（Chest Boat），通过 m_hasChest 区分。
 *
 * 使用方式：
 * - 玩家右键点击水面或陆地
 * - 在击中位置生成对应类型的船实体
 * - 船自动朝向玩家的朝向
 * - 非创造模式消耗物品
 */
class BoatItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param boatType 船的木材类型
     * @param hasChest 是否为带箱子的船
     * @param properties 物品属性
     */
    BoatItem(entity::BoatEntity::Type boatType, bool hasChest, const ItemProperties& properties);

    ~BoatItem() override = default;

    /**
     * @brief 获取船的类型
     * @return 船的木材类型
     */
    [[nodiscard]] entity::BoatEntity::Type getBoatType() const noexcept { return m_boatType; }

    /**
     * @brief 是否为带箱子的船
     * @return true 表示带箱子的船，false 表示普通船
     */
    [[nodiscard]] bool hasChest() const noexcept { return m_hasChest; }

    /**
     * @brief 在方块上使用物品
     *
     * 行为：
     * 1. 获取击中点位置
     * 2. 在击中位置生成船实体
     * 3. 设置船的朝向为玩家的朝向
     * 4. 检查碰撞，如果有碰撞返回 Fail
     * 5. 服务端生成船实体
     * 6. 非创造模式消耗物品
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;

private:
    entity::BoatEntity::Type m_boatType;
    bool m_hasChest;
};

} // namespace item
} // namespace mc
