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
#include "common/util/assert/AssertAll.hpp"

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
    // Tick触发器对所有监听器都触发
    // 注意：实际触发逻辑需要在服务端模块中实现，
    // 这里需要访问 PlayerAdvancements 来触发监听器
    // 服务端会在每tick调用此方法，触发器将检查所有已注册的监听器
    MC_UNUSED(player);
}

} // namespace mc::advancement
