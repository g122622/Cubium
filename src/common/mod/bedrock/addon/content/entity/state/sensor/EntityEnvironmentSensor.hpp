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

#include "common/mod/bedrock/addon/content/entity/events/EntityEventTrigger.hpp"
#include <vector>

namespace mc::mod::bedrock::addon {
class EntityEventProgram;

/** @brief 无历史状态的环境传感器，每个实体 tick 按定义顺序检查所有触发器。 */
class EntityEnvironmentSensor {
public:
    /** @brief 校验组件并编译对象或数组形式的触发器。 */
    [[nodiscard]] static Result<EntityEnvironmentSensor> compile(const nlohmann::json& definition);
    /** @brief 校验全部目标事件能在所属实体定义内解析。 */
    [[nodiscard]] Result<void> validateEvents(const EntityEventProgram& events) const;
    /** @brief 按当前实体状态触发匹配事件，属性及组件组修改仍延迟应用。 */
    Result<void> tick(mc::Entity& entity) const;

private:
    std::vector<EntityEventTrigger> m_triggers;
};
} // namespace mc::mod::bedrock::addon
