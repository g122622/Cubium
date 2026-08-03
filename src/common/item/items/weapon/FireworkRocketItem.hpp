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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc {

class IWorld;
class Player;

namespace entity {
class FireworkRocketEntity;
}

namespace item {

/**
 * @brief 烟花火箭物品
 *
 * 可由玩家右键发射或由发射器发射的烟花火箭。
 * 发射时创建 FireworkRocketEntity 实体。
 *
 * 实现 ProjectileItem 接口以支持发射器自动注册弹射物发射行为。
 * 烟花火箭从 ItemStack 读取飞行时间和爆炸效果数据。
 *
 * 参考 MC 1.16.5 FireworkRocketItem
 */
class FireworkRocketItem : public Item, public ProjectileItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit FireworkRocketItem(const ItemProperties& properties);

    ~FireworkRocketItem() override = default;

    // 禁止拷贝和移动
    FireworkRocketItem(const FireworkRocketItem&) = delete;
    FireworkRocketItem& operator=(const FireworkRocketItem&) = delete;
    FireworkRocketItem(FireworkRocketItem&&) = delete;
    FireworkRocketItem& operator=(FireworkRocketItem&&) = delete;

    // ============================================================================
    // ProjectileItem 接口实现
    // ============================================================================

    /**
     * @brief 创建弹射物实体
     *
     * 创建 FireworkRocketEntity 并从物品堆读取烟花数据。
     */
    [[nodiscard]] std::unique_ptr<entity::ProjectileEntity> asProjectile(IWorld& world,
        const Vector3& position,
        const ItemStack& stack,
        f32 directionX,
        f32 directionY,
        f32 directionZ) const override;

    /**
     * @brief 获取发射器配置
     * @return 烟花火箭配置（power=0.5, uncertainty=1.0）
     */
    [[nodiscard]] ProjectileDispenseConfig getDispenseConfig() const override;
};

} // namespace item
} // namespace mc
