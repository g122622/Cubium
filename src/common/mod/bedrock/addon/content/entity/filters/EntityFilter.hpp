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
#include <nlohmann/json.hpp>

namespace mc {
class Entity;
}

namespace mc::mod::bedrock::addon {

/** @brief 同步过滤求值的实体上下文，缺失的交互参与者显式传 nullptr。 */
struct EntityFilterContext {
    mc::Entity& self;
    const mc::Entity* other;
    const mc::Entity* player;
    const mc::Entity* parent;
    const mc::Entity* damager;
    const mc::Entity* target;
};

/** @brief 已校验的实体过滤条件，可由事件、行为和实体查询复用。 */
class EntityFilter {
public:
    /** @brief 校验条件、操作符、主体和参数，未知测试明确返回 Unsupported。 */
    [[nodiscard]] static Result<EntityFilter> compile(const nlohmann::json& filter);
    /** @brief 读取当前有效实体状态，短路求出过滤结果。 */
    [[nodiscard]] Result<bool> matches(const EntityFilterContext& context) const;

private:
    nlohmann::json m_filter;
};
} // namespace mc::mod::bedrock::addon
