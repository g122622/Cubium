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

namespace mc {

class LivingEntity;

namespace entity {

/**
 * @brief 撞飞型近战生物接口
 */
class IFlinging {
public:
    virtual ~IFlinging() = default;

    /**
     * @brief 获取撞飞攻击动画剩余 tick
     */
    [[nodiscard]] virtual i32 getFlingAnimationTicks() const = 0;

    /**
     * @brief 执行一次带撞飞语义的近战攻击
     *
     * TODO: 当前无生产调用者。HoglinEntity/ZoglinEntity 已改为 override attackEntityAsMob 自管完整攻击链
     * （动画+随机化伤害+flingTarget+onAttackEntity+音效），不再走此封装。新 fling 型生物应遵循
     * attackEntityAsMob override 模式（参照 IronGolemEntity::attackEntityAsMob / HoglinEntity::attackEntityAsMob），
     * 复用 flingTarget 而非本方法——本方法自带 hurt 调用，若在 attackEntityAsMob override 内调用会造成双重伤害。
     * 现仅被单元测试 FlingingSupportTypesTest 引用验证 fling 核心语义，待确认无生产保留价值后可删除。
     */
    [[nodiscard]] static bool attackWithFling(LivingEntity& attacker, LivingEntity& target, bool attackerIsBaby);

    /**
     * @brief 对目标施加撞飞
     */
    static void flingTarget(LivingEntity& attacker, LivingEntity& target);
};

} // namespace entity
} // namespace mc
