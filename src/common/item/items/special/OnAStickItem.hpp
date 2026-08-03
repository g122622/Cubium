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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
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
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include <string>

namespace mc {

// Forward declarations
class Player;
class IWorld;
class Entity;
class ItemStack;

namespace entity {
class IRideable;
}

namespace item {

/**
 * @brief 钓竿类物品基类
 *
 * 这是一个用于控制可骑乘实体的物品类型。
 * 当玩家骑乘特定类型的实体时，右键使用此物品可以触发加速。
 *
 * 典型用途：
 * - 胡萝卜钓竿 (Carrot on a Stick) - 控制猪
 * - 诡异菌钓竿 (Warped Fungus on a Stick) - 控制炽足兽
 *
 * 使用机制：
 * 1. 玩家必须正在骑乘目标实体类型
 * 2. 右键触发加速（通过 IRideable::boost()）
 * 3. 消耗耐久度
 * 4. 耐久度耗尽后转换为普通钓鱼竿
 *
 * 参考 MC 1.16.5: net.minecraft.item.OnAStickItem
 */
class OnAStickItem : public Item {
public:
    /**
     * @brief 构造函数
     *
     * @param properties 物品属性（应包含 maxDamage 耐久度）
     * @param entityId 目标实体类型ID（如 "minecraft:pig" 或 "minecraft:strider"）
     * @param durabilityCost 每次加速消耗的耐久度
     */
    OnAStickItem(const ItemProperties& properties, const std::string& entityId, i32 durabilityCost);

    ~OnAStickItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 右键使用物品
     *
     * 当玩家骑乘目标实体时，触发加速效果。
     * 消耗耐久度，耐久度耗尽后转换为钓鱼竿。
     *
     * @param world 世界引用
     * @param player 玩家引用
     * @param hand 使用的手
     * @return 动作结果
     */
    [[nodiscard]] ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 获取附魔能力
     * @return 附魔能力值（MC 1.16.5: 1）
     */
    [[nodiscard]] i32 getItemEnchantability() const override { return 1; }

    // ========== OnAStickItem 特有方法 ==========

    /**
     * @brief 获取目标实体类型ID
     * @return 实体类型ID字符串
     */
    [[nodiscard]] const std::string& getEntityTypeId() const { return m_entityId; }

    /**
     * @brief 获取每次加速消耗的耐久度
     * @return 耐久度消耗值
     */
    [[nodiscard]] i32 getDurabilityCost() const { return m_durabilityCost; }

private:
    std::string m_entityId; ///< 目标实体类型ID
    i32 m_durabilityCost;   ///< 每次加速消耗的耐久度
};

} // namespace item
} // namespace mc
