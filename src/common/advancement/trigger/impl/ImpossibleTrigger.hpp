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

#include "../CriterionTrigger.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// Forward declare the Instance
class ImpossibleTriggerInstance;

/**
 * @brief 不可能完成的触发器
 *
 * 用于创建无法自动完成的成就条件。
 * 通常用于配方解锁成就（需要手动授予）或调试目的。
 */
class ImpossibleTrigger : public AbstractCriterionTrigger<ImpossibleTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:impossible";

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    /**
     * @brief 触发检测
     *
     * 不可能触发器永远不会触发。
     */
    void trigger(::mc::server::PlayerAdvancements& advancements)
    {
        // 不可能触发器永远不会触发
        MC_UNUSED(advancements);
    }
};

/**
 * @brief 不可能触发器实例
 */
class ImpossibleTriggerInstance : public CriterionInstance<ImpossibleTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = ImpossibleTrigger::TRIGGER_ID;

    /**
     * @brief 从JSON解析
     * 不可能触发器没有条件，直接返回空实例
     */
    Result<void> fromJson(const nlohmann::json& json)
    {
        // 不可能触发器没有任何条件
        MC_UNUSED(json);
        return {};
    }

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const
    {
        return nullptr; // 无条件
    }
};

} // namespace mc::advancement
