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

#include "../../potion/Potion.hpp"
#include "ArrowItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace item {

/**
 * @brief 药水箭物品
 *
 * 带有药水效果的箭矢。命中生物时应用药水效果。
 * MC 1.16.5 中药水箭不受益于无限附魔。
 *
 * 参考 MC 1.16.5 TippedArrowItem
 */
class TippedArrowItem : public ArrowItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit TippedArrowItem(const ItemProperties& properties);

    ~TippedArrowItem() override = default;

    /**
     * @brief 创建箭矢实体
     *
     * 创建带有药水效果的箭矢实体。
     *
     * @param world 世界
     * @param stack 箭矢物品堆
     * @param shooter 射击者
     * @return 箭矢实体指针（调用者负责管理）
     */
    [[nodiscard]] entity::AbstractArrowEntity* createArrow(
        IWorld& world, const ItemStack& stack, LivingEntity& shooter) const override;

    /**
     * @brief 检查箭矢是否无限
     *
     * MC 1.16.5: 药水箭不受益于无限附魔。
     *
     * @param arrowStack 箭矢物品堆
     * @param bowStack 弓物品堆
     * @param player 玩家
     * @return 是否无限（总是返回 false）
     */
    [[nodiscard]] bool isInfinite(
        const ItemStack& arrowStack, const ItemStack& bowStack, Player& player) const override;

    /**
     * @brief 创建弹射物实体（发射器使用）
     *
     * 重写以在箭矢上应用药水效果。
     *
     * @param world 世界引用
     * @param position 生成位置
     * @param stack 物品堆（读取药水效果）
     * @param directionX 发射方向 X 分量
     * @param directionY 发射方向 Y 分量
     * @param directionZ 发射方向 Z 分量
     * @return 创建的弹射物实体
     */
    [[nodiscard]] std::unique_ptr<entity::ProjectileEntity> asProjectile(IWorld& world,
        const Vector3& position,
        const ItemStack& stack,
        f32 directionX,
        f32 directionY,
        f32 directionZ) const override;

    /**
     * @brief 获取药水类型
     * @param stack 物品堆
     * @return 药水指针，无效返回 nullptr
     */
    [[nodiscard]] static const potion::Potion* getPotion(const ItemStack& stack);

    /**
     * @brief 获取药水效果列表
     * @param stack 物品堆
     * @return 效果列表
     */
    [[nodiscard]] static std::vector<entity::effect::EffectInstance> getEffects(const ItemStack& stack);

    /**
     * @brief 设置药水类型
     * @param stack 物品堆
     * @param potion 药水类型
     */
    static void setPotion(ItemStack& stack, const potion::Potion* potion);
};

} // namespace item
} // namespace mc
