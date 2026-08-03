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

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

/**
 * @brief 避免振动触发器实例
 *
 * 当玩家成功避免触发幽匿感测体/幽匿尖啸体的振动信号时触发。
 * 此成就条件无额外谓词，任何成功的避免振动都会触发。
 */
class AvoidVibrationTriggerInstance : public CriterionInstance<AvoidVibrationTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:avoid_vibration";

    /**
     * @brief 从JSON解析
     * 避免振动触发器没有条件
     */
    Result<void> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const;
};

/**
 * @brief 避免振动触发器
 *
 * 当玩家通过潜行等方式成功避免振动信号传播时触发。
 * 对应 MC 1.21.11 的 CriteriaTriggers.AVOID_VIBRATION。
 *
 * 触发场景：玩家潜行时产生的事件（如踩方块、投掷物等）
 * 被振动系统忽略（isIgnoredBySneaking 返回 true），
 * 且振动接收者支持触发规避成就（canTriggerAvoidVibration 返回 true）。
 */
class AvoidVibrationTrigger : public AbstractCriterionTrigger<AvoidVibrationTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:avoid_vibration";

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    /**
     * @brief 从JSON反序列化实例
     */
    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    // trigger() 方法不在 common 模块中实现，因为需要 server::PlayerAdvancements 的完整定义。
    // 调用方应使用基类的 trigger 模板方法：
    //   trigger->AbstractCriterionTrigger<AvoidVibrationTriggerInstance>::trigger(*advancements, predicate);
    // 参考 AdvancementEventHandler 中的调用模式。
};

} // namespace mc::advancement
