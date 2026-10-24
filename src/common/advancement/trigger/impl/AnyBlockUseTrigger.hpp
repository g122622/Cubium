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
#include "common/advancement/trigger/conditions/CriterionContext.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// 前向声明 Instance 类
class AnyBlockUseTriggerInstance;

/**
 * @brief 任意方块使用触发器
 *
 * 当玩家使用方块（放置/使用/交互方块）时无条件触发。
 * 参考 MC 1.21.11: CriteriaTriggers.DEFAULT_BLOCK_USE（任意方块使用）。
 *
 * 与 PlacedBlockTrigger 的区别：PlacedBlockTrigger 仅在方块实际放置时触发，
 * 且携带方块/位置/物品谓词；AnyBlockUseTrigger 在玩家与方块产生 consumesAction
 * 交互时无条件触发，不携带任何条件参数。
 */
class AnyBlockUseTrigger : public AbstractCriterionTrigger<AnyBlockUseTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:default_block_use";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;
};

/**
 * @brief 物品作用于方块触发器（对齐 vanilla ItemUsedOnLocationTrigger）
 *
 * 玩家手持物品对某方块完成交互时触发（点燃 TNT、用药水喂食幼崽、给铜块打蜡等）。
 * 条件为 location（ContextAwarePredicate：location_check / block_state_property），
 * 数据包 adventure/lighten_up 等 10 处依赖它。
 */
class ItemUsedOnBlockTrigger : public AbstractCriterionTrigger<AnyBlockUseTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:item_used_on_block";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;
};

/**
 * @brief 任意方块使用触发器实例
 *
 * 无条件实例，test 恒返回 true。
 */
class AnyBlockUseTriggerInstance : public CriterionInstance<AnyBlockUseTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = AnyBlockUseTrigger::TRIGGER_ID;

    AnyBlockUseTriggerInstance() = default;

    /**
     * @brief 检查条件是否满足（无 location 条件时恒真）
     */
    [[nodiscard]] bool test() const
    {
        return true;
    }

    /**
     * @brief 携带上下文判定（location 条件支持 ContextAwarePredicate 条件列表写法）
     * @note 不带 location 条件的实例（default_block_use）恒真
     */
    [[nodiscard]] bool test(const CriterionContext& context) const;

    Result<void> fromJson(const nlohmann::json& json);

    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    /// location 条件（default_block_use 无条件时为空）
    ContextAwarePredicate m_locationContext;
};

} // namespace mc::advancement
