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

#include "../../Enchantment.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {

class IWorld;
class BlockPos;

namespace item {
namespace enchant {

/**
 * @brief 冰霜行者附魔
 *
 * 在水面上行走时创建霜冰。
 *
 * 效果:
 * - 每级增加冰霜范围
 * - I: 半径 2 格, II: 半径 3 格
 * - 最大 II 级
 * - 与深海探索者互斥
 *
 * 位置依赖效果:
 * - 当实体在地面上且非骑乘状态时，在脚下水面放置霜冰
 * - 冰霜范围 = level + 1 格
 * - 霜冰会在一段时间后自动融化（由 FrostedIceBlock 处理）
 */
class FrostWalkerEnchantment : public Enchantment {
public:
    FrostWalkerEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:frost_walker"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.frost_walker";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::ArmorFeet; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 2; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] bool isTreasure() const noexcept override
    {
        return true; // 只能从箱子或交易获得
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 10 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 15; }

    /**
     * @brief 获取冰霜半径
     * @param level 附魔等级
     * @return 半径（格）
     */
    [[nodiscard]] static i32 getFrostRadius(i32 level)
    {
        // 每级半径 +1
        return level + 1;
    }

    /**
     * @brief 检查是否与深海探索者互斥
     */
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;

    /**
     * @brief 位置依赖效果：在地面上行走时冻结水面
     *
     * 当实体在地面上且非骑乘状态时，在脚下的水面上放置霜冰。
     * 冰霜行者始终处于"活跃"状态（只要在地面），不需要跟踪激活/停用。
     *
     * @param entity 持有附魔物品的实体
     * @param stack 附魔物品堆
     * @param slot 装备槽位
     * @param level 附魔等级
     * @param isActive 当前该附魔是否已经处于活跃状态
     * @return 是否应该保持活跃状态
     */
    [[nodiscard]] bool onLocationChanged(
        LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level, bool isActive) const override;

    /**
     * @brief 停用位置依赖效果（冰霜行者无需清理，提供空实现）
     *
     * 冰霜行者每次位置变化时重新放置霜冰，不需要在停用时移除效果。
     * 霜冰会由 FrostedIceBlock 自行融化。
     *
     * @param entity 持有附魔物品的实体
     * @param stack 附魔物品堆
     * @param slot 装备槽位
     * @param level 附魔等级
     */
    void onLocationEffectDeactivated(LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level) const override;

private:
    /**
     * @brief 在实体脚下放置霜冰
     *
     * @param world 世界
     * @param center 实体所在的方块位置
     * @param radius 冰霜半径
     */
    void placeFrostedIce(IWorld& world, const BlockPos& center, i32 radius) const;
};

} // namespace enchant
} // namespace item
} // namespace mc
