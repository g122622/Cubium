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

#include "../../../core/Types.hpp"
#include "../../core/Item.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {

// 前向声明
class IWorld;
class Player;
class LivingEntity;

namespace entity {
class AbstractArrowEntity;
}

namespace item {

/**
 * @brief 箭矢物品
 *
 * 用于弓和弩的弹药物品。可以堆叠（最大64个）。
 *
 * MC 1.16.5 箭矢类型:
 * - 普通箭 (Arrow): 标准箭矢
 * - 药水箭 (Tipped Arrow): 带药水效果的箭矢
 * - 光灵箭 (Spectral Arrow): 带发光效果的箭矢（仅创造模式可获得）
 *
 * 参考 MC 1.16.5 ArrowItem
 */
class ArrowItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit ArrowItem(const ItemProperties& properties);

    ~ArrowItem() override = default;

    /**
     * @brief 创建箭矢实体
     *
     * 根据箭矢物品类型创建对应的箭矢实体。
     * 子类可重写以创建特殊箭矢（如光灵箭、药水箭）。
     *
     * @param world 世界
     * @param stack 箭矢物品堆
     * @param shooter 射击者
     * @return 箭矢实体指针（调用者负责管理）
     */
    [[nodiscard]] virtual entity::AbstractArrowEntity* createArrow(
        IWorld& world, const ItemStack& stack, LivingEntity& shooter) const;

    /**
     * @brief 检查箭矢是否无限
     *
     * MC 1.16.5: 只有普通箭受益于无限附魔。
     * 光灵箭和药水箭不受益。
     *
     * @param arrowStack 箭矢物品堆
     * @param bowStack 弓物品堆
     * @param player 玩家
     * @return 是否无限（不被消耗）
     */
    [[nodiscard]] virtual bool isInfinite(const ItemStack& arrowStack, const ItemStack& bowStack, Player& player) const;
};

} // namespace item
} // namespace mc
