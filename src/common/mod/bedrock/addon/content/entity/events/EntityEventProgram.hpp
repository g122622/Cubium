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
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {
class Entity;
}

namespace mc::mod::bedrock::addon {

/** @brief 已校验、可重复执行的实体事件程序。 */
class EntityEventProgram {
public:
    /** @brief 校验事件动作并规范化常量属性值。 */
    [[nodiscard]] static Result<EntityEventProgram> compile(
        const nlohmann::json& events, const nlohmann::json& properties);
    /** @brief 计算完整动作后暂存属性；递归或步骤超限不提交任何写入。 */
    [[nodiscard]] Result<std::vector<std::string>> execute(mc::Entity& entity, const std::string& eventId) const;
    /** @brief 检查指定事件是否有定义。 */
    [[nodiscard]] bool contains(const std::string& eventId) const;

private:
    nlohmann::json m_events;
};

} // namespace mc::mod::bedrock::addon
