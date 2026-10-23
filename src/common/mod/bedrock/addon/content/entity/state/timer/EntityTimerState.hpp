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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventTrigger.hpp"

namespace mc::math {
class IRandom;
}

namespace mc::mod::bedrock::addon {
/** @brief 实体计时器的当前间隔、剩余 tick 与组件事件引用。 */
class EntityTimerState {
public:
    /** @brief 校验固定、区间或加权时长及到期事件配置。 */
    static Result<void> validateDefinition(const nlohmann::json& definition);
    /** @brief 为一次组件激活选择初始间隔并开始计时。 */
    [[nodiscard]] static Result<EntityTimerState> create(const nlohmann::json& definition, mc::math::IRandom& random);
    /** @brief 校验保存的计时状态，不接触运行中的实体。 */
    static Result<void> validateSavedState(const nlohmann::json& saved);
    /** @brief 推进一个 tick，到期时先处理循环状态再触发事件，避免重入重复执行。 */
    Result<void> tick(mc::Entity& entity);
    /** @brief 保存间隔、剩余时间和运行状态。 */
    [[nodiscard]] nlohmann::json save() const;
    /** @brief 校验整个保存快照后替换计时状态。 */
    Result<void> load(const nlohmann::json& saved);

private:
    EntityTimerState() = default;
    /** @brief 按组件配置及宿主随机源选择下一次间隔。 */
    [[nodiscard]] i64 _chooseInterval(mc::math::IRandom& random) const;

    nlohmann::json m_definition;
    EntityEventTrigger m_trigger;
    i64 m_interval;
    i64 m_remaining;
    bool m_running;
};
} // namespace mc::mod::bedrock::addon
