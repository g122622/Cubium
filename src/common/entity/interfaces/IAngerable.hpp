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
#include <optional>

namespace mc {

// Forward declarations
class LivingEntity;
class Player;

namespace entity {

/**
 * @brief 愤怒接口 - 用于可愤怒的实体
 *
 * 实现此接口的实体可以在被攻击后记住攻击者并进行反击。
 * 例如：狼、铁傀儡、末影人、僵尸猪灵等。
 *
 * 参考 MC 1.16.5 IAngerable
 */
class IAngerable {
public:
    virtual ~IAngerable() = default;

    /**
     * @brief 设置攻击目标
     * @param target 目标实体，nullptr表示清除目标
     */
    virtual void setAttackTarget(LivingEntity* target) = 0;

    /**
     * @brief 获取当前攻击目标
     * @return 当前攻击目标，可能为nullptr
     */
    virtual LivingEntity* getAttackTarget() const = 0;

    /**
     * @brief 设置复仇目标（最近攻击自己的实体）
     * @param target 攻击者实体
     */
    virtual void setRevengeTarget(LivingEntity* target) = 0;

    /**
     * @brief 获取复仇目标（最近攻击自己的实体）
     * @return 复仇目标，可能为nullptr
     */
    virtual LivingEntity* getRevengeTarget() const = 0;

    /**
     * @brief 获取复仇计时器
     * @return 复仇计时器值（ticks）
     */
    virtual i32 getRevengeTimer() const = 0;

    /**
     * @brief 检查是否处于愤怒状态
     * @return 如果实体正在愤怒则返回true
     */
    virtual bool isAngry() const = 0;

    /**
     * @brief 设置愤怒状态
     * @param angry 是否愤怒
     */
    virtual void setAngry(bool angry) = 0;

    /**
     * @brief 获取愤怒剩余时间（ticks）
     * @return 愤怒剩余时间
     */
    virtual i32 getAngerTime() const = 0;

    /**
     * @brief 设置愤怒时间
     * @param time 愤怒时间（ticks）
     */
    virtual void setAngerTime(i32 time) = 0;

    /**
     * @brief 每tick更新愤怒计时器
     *
     * 子类应在tick()中调用此方法
     */
    virtual void updateAnger()
    {
        if (getAngerTime() > 0) {
            setAngerTime(getAngerTime() - 1);
            if (getAngerTime() == 0) {
                setAngry(false);
                setAttackTarget(nullptr);
            }
        }
    }
};

} // namespace entity
} // namespace mc
