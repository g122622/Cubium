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
#include "common/mod/bedrock/addon/content/entity/filters/EntityFilter.hpp"
#include <optional>
#include <string>

namespace mc {
class Entity;
}

namespace mc::mod::bedrock::addon {
/** @brief 组件中的事件引用及其可选触发条件。 */
class EntityEventTrigger {
public:
    /** @brief 编译字符串或 event/target/filters 对象形式的自身事件引用。 */
    [[nodiscard]] static Result<EntityEventTrigger> compile(const nlohmann::json& definition);
    /** @brief 返回被引用的事件名称供定义加载时验证。 */
    [[nodiscard]] const std::string& eventId() const;
    /** @brief 过滤通过后向实体自己的事件程序提交触发。 */
    Result<void> fire(mc::Entity& entity) const;
    /** @brief 在死亡或交互调用栈中保留参与者与伤害上下文。 */
    Result<void> fire(const EntityFilterContext& context) const;

private:
    std::string m_eventId;
    std::optional<EntityFilter> m_filter;
};
} // namespace mc::mod::bedrock::addon
