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

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

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
     * @brief 检查条件是否满足
     * @return 恒返回 true（无条件触发器）
     */
    [[nodiscard]] bool test() const { return true; }

    Result<void> fromJson(const nlohmann::json& json);

    [[nodiscard]] nlohmann::json conditionsToJson() const;
};

} // namespace mc::advancement
