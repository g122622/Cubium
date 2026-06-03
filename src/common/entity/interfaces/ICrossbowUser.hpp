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
#include "common/entity/interfaces/IRangedAttackMob.hpp"

namespace mc {

// Forward declarations
class ItemStack;
class LivingEntity;

namespace entity {

/**
 * @brief 弩使用者接口 - 用于可以使用弩的实体
 *
 * 实现此接口的实体可以使用弩进行攻击。
 * 例如：掠夺者、猪灵等。
 */
class ICrossbowUser : public IRangedAttackMob {
public:
    ~ICrossbowUser() override = default;

    /**
     * @brief 设置弩的装填状态
     * @param charging 是否正在装填
     */
    virtual void setChargingCrossbow(bool charging) = 0;

    /**
     * @brief 检查是否正在装填弩
     * @return 如果正在装填返回true
     */
    virtual bool isChargingCrossbow() const = 0;

    /**
     * @brief 当弩装填完成时调用
     * @param crossbow 弩物品
     */
    virtual void onCrossbowLoadComplete(::mc::ItemStack& crossbow) = 0;

    /**
     * @brief 发射弩箭
     * @param target 目标实体
     * @param crossbow 弩物品
     * @param charge 蓄力程度
     */
    virtual void shootCrossbow(::mc::LivingEntity* target, ::mc::ItemStack& crossbow, f32 charge) = 0;

    /**
     * @brief 获取弩装填时间（ticks）
     * @return 装填时间
     */
    virtual i32 getCrossbowChargeTime() const { return 25; }
};

} // namespace entity
} // namespace mc
