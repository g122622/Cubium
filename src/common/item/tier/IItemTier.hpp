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

#include "../../core/Types.hpp"

namespace mc {
namespace crafting {
class Ingredient; // Forward declaration
}

namespace item {
namespace tier {

/**
 * @brief 工具层级接口
 *
 * 定义工具材质层级的属性。每种材质（木、石、铁、金、钻石、下界合金）
 * 实现此接口，提供不同的属性值。
 *
 * 属性说明：
 * - getMaxUses(): 耐久度，工具能承受的伤害值
 * - getEfficiency(): 挖掘效率倍率，影响挖掘速度
 * - getAttackDamage(): 基础攻击伤害加成
 * - getHarvestLevel(): 挖掘等级，决定能采集哪些方块
 *   - 0: 木/金 - 可采集煤矿、石头
 *   - 1: 石 - 可采集铁矿
 *   - 2: 铁 - 可采集钻石矿、金矿
 *   - 3: 钻石 - 可采集黑曜石
 *   - 4: 下界合金 - 可采集远古残骸
 * - getEnchantability(): 附魔能力，影响附魔品质
 * - getRepairMaterial(): 修复材料，用于铁砧修复
 *
 * 参考: net.minecraft.item.IItemTier
 */
class IItemTier {
public:
    virtual ~IItemTier() = default;

    /**
     * @brief 获取最大耐久度
     * @return 工具能承受的最大伤害值
     */
    [[nodiscard]] virtual i32 getMaxUses() const = 0;

    /**
     * @brief 获取挖掘效率
     * @return 挖掘有效方块时的速度倍率
     */
    [[nodiscard]] virtual f32 getEfficiency() const = 0;

    /**
     * @brief 获取基础攻击伤害加成
     * @return 加到工具基础伤害上的额外伤害
     */
    [[nodiscard]] virtual f32 getAttackDamage() const = 0;

    /**
     * @brief 获取挖掘等级
     * @return 0-4 的挖掘等级，决定能采集的方块类型
     */
    [[nodiscard]] virtual i32 getHarvestLevel() const = 0;

    /**
     * @brief 获取附魔能力
     * @return 附魔能力值，越高附魔效果越好
     */
    [[nodiscard]] virtual i32 getEnchantability() const = 0;

    /**
     * @brief 获取修复材料
     * @return 用于铁砧修复的原料Ingredient
     */
    [[nodiscard]] virtual const crafting::Ingredient& getRepairMaterial() const = 0;
};

} // namespace tier
} // namespace item
} // namespace mc
