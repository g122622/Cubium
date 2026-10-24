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
#include "common/mod/bedrock/addon/content/molang/MolangExpression.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {
class Entity;
}

namespace mc::mod::bedrock::addon {

/** @brief 一个已验证的组件组添加或移除操作。 */
struct EntityComponentGroupChange {
    bool add;
    std::string group;
};

/** @brief 一次实际触发事件的 ID 及其组件组修改快照。 */
struct EntityEventNotification {
    std::string id;
    std::vector<EntityComponentGroupChange> componentChanges;
};

/** @brief 完成计算的事件通知与按执行顺序排列的组件组操作。 */
struct EntityEventExecution {
    std::vector<EntityEventNotification> notifications;
    std::vector<EntityComponentGroupChange> componentChanges;
};

/** @brief 已校验、可重复执行的实体事件程序。 */
class EntityEventProgram {
public:
    /** @brief 校验事件动作并规范化常量属性值。 */
    [[nodiscard]] static Result<EntityEventProgram> compile(
        const nlohmann::json& events, const nlohmann::json& properties, const nlohmann::json& groups);
    /** @brief 计算完整动作后暂存属性；递归或步骤超限不提交任何写入。 */
    [[nodiscard]] Result<EntityEventExecution> execute(mc::Entity& entity, const std::string& eventId) const;
    /** @brief 使用交互或战斗提供的完整主体上下文执行事件。 */
    [[nodiscard]] Result<EntityEventExecution> execute(
        const EntityFilterContext& context, const std::string& eventId) const;
    /** @brief 检查指定事件是否有定义。 */
    [[nodiscard]] bool contains(const std::string& eventId) const;

private:
    nlohmann::json m_events;
    nlohmann::json m_properties;
    std::unordered_map<std::string, MolangExpression> m_expressions;
    std::vector<EntityFilter> m_filters;
};

} // namespace mc::mod::bedrock::addon
