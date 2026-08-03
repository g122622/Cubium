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
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"

namespace mc {

// 前向声明
enum class EquipmentSlot : u8;

namespace item::items {

/**
 * @brief 鞘翅
 *
 * 允许玩家滑翔的特殊胸甲。
 * 参考: net.minecraft.item.ElytraItem
 *
 * 特性：
 * - 占用胸甲槽位
 * - 有432点耐久度
 * - 每滑翔1秒消耗1点耐久度
 * - 可用幻翼膜修复
 */
class ElytraItem : public Item {
public:
    /**
     * @brief 构造鞘翅
     * @param properties 物品属性
     */
    explicit ElytraItem(ItemProperties properties);

    // ========== 物品重写方法 ==========

    /**
     * @brief 是否为护甲物品
     *
     * 鞘翅虽然不继承 ArmorItem，但占用胸甲槽位，
     * 受伤时同样会损耗耐久度，且耐久保护附魔使用护甲概率。
     *
     * @return true
     */
    [[nodiscard]] bool isArmor() const override { return true; }

    /**
     * @brief 是否可修复
     */
    [[nodiscard]] bool isRepairable() const { return true; }

    /**
     * @brief 右键使用物品
     *
     * 如果玩家当前胸甲槽位为空，则装备鞘翅。
     *
     * @param world 世界
     * @param player 玩家
     * @param hand 使用的手
     * @return 动作结果
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 物品Tick
     *
     * 每tick检查滑翔状态并消耗耐久度。
     *
     * @param stack 物品堆
     * @param world 世界
     * @param entity 持有实体
     * @param itemSlot 物品栏槽位
     * @param isSelected 是否被选中
     */
    void inventoryTick(ItemStack& stack, IWorld& world, Entity& entity, i32 itemSlot, bool isSelected) const override;

    // ========== 鞘翅特有方法 ==========

    /**
     * @brief 检查鞘翅是否受损
     * @param stack 物品堆
     * @return 是否受损（耐久度低于阈值）
     */
    [[nodiscard]] static bool isUsable(const ItemStack& stack);

    /**
     * @brief 检查实体是否正在滑翔
     * @param entity 实体
     * @return 是否正在滑翔
     */
    [[nodiscard]] static bool isGliding(const LivingEntity& entity);

    /**
     * @brief 滑翔时消耗耐久度
     * @param stack 物品堆
     * @param entity 使用者
     * @param slot 物品所在的装备槽位
     */
    static void damageElytra(ItemStack& stack, LivingEntity& entity, EquipmentSlot slot);

private:
    /// 鞘翅的最大耐久度
    static constexpr i32 MAX_DURABILITY = 432;
};

} // namespace item::items
} // namespace mc
