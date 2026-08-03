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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING ANY PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "ArrowItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc {
namespace item {

/**
 * @brief 光灵箭物品
 *
 * 带发光效果的箭矢，命中生物时施加发光状态效果。
 * 光灵箭不受益于无限附魔。
 *
 * 参考 MC 1.16.5 SpectralArrowItem
 */
class SpectralArrowItem : public ArrowItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit SpectralArrowItem(const ItemProperties& properties);

    ~SpectralArrowItem() override = default;

    /**
     * @brief 创建箭矢实体
     *
     * 创建光灵箭实体而非普通箭矢实体。
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
     * MC 1.16.5: 光灵箭不受益于无限附魔。
     *
     * @param arrowStack 箭矢物品堆
     * @param bowStack 弓物品堆
     * @param player 玩家
     * @return 是否无限
     */
    [[nodiscard]] bool isInfinite(
        const ItemStack& arrowStack, const ItemStack& bowStack, Player& player) const override;

    /**
     * @brief 创建弹射物实体（发射器使用）
     *
     * 创建 SpectralArrowEntity 而非 ArrowEntity。
     *
     * @param world 世界引用
     * @param position 生成位置
     * @param stack 物品堆
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
};

} // namespace item
} // namespace mc
