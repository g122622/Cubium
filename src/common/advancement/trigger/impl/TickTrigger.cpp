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

#include "TickTrigger.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/core/Result.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// ========== TickTriggerInstance ==========

Result<void> TickTriggerInstance::fromJson(const nlohmann::json& json)
{
    // Tick触发器没有条件
    MC_UNUSED(json);
    return {};
}

nlohmann::json TickTriggerInstance::conditionsToJson() const
{
    return nullptr;
}

// ========== TickTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> TickTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<TickTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void TickTrigger::trigger(ServerPlayer& player)
{
    // Tick触发器通过 AdvancementEventHandler::_onServerTick() 调用基类模板方法
    // AbstractCriterionTrigger<TickTriggerInstance>::trigger() 实现，不由此方法触发。
    // 此方法仅为接口占位，实际触发逻辑在服务端的事件处理器中。
    MC_UNUSED(player);
}

} // namespace mc::advancement
