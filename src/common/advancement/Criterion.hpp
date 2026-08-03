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
#include "common/resource/ResourceLocation.hpp"
#include "trigger/CriterionTrigger.hpp"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

/**
 * @brief 成就条件
 *
 * 每个成就可以有多个条件，条件的完成情况决定成就是否完成。
 * 条件由触发器实例组成，当触发器触发时检查条件是否满足。
 * 例如："diamond" 条件使用 inventory_changed 触发器检查玩家是否获得钻石。
 */
class Criterion {
public:
    Criterion() = default;

    /**
     * @brief 构造条件
     * @param name 条件名称
     * @param triggerInstance 触发器实例
     */
    Criterion(std::string name, std::shared_ptr<ICriterionInstance> triggerInstance);

    /**
     * @brief 获取条件名称
     */
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }

    /**
     * @brief 获取触发器实例
     */
    [[nodiscard]] const std::shared_ptr<ICriterionInstance>& getTriggerInstance() const noexcept
    {
        return m_triggerInstance;
    }

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getTrigger() const noexcept
    {
        return m_triggerInstance ? m_triggerInstance->getId() : ResourceLocation();
    }

    /**
     * @brief 从JSON解析
     * @param name 条件名称
     * @param json JSON对象
     * @return 条件或错误
     */
    static Result<Criterion> fromJson(const std::string& name, const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    std::string m_name;
    std::shared_ptr<ICriterionInstance> m_triggerInstance;
};

} // namespace mc::advancement
