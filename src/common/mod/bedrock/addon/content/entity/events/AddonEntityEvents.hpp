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
#include <functional>
#include <memory>
#include <string>

namespace mc {
class Entity;
}

namespace mc::mod::bedrock::addon {
class EntityEventProgram;
/** @brief 脚本门面拥有的事件出口，实体只借弱引用，避免延长世界寿命。 */
using EntityEventSink = std::function<void(mc::Entity&, const std::string&)>;

/** @brief 绑定到实体的不可变事件程序和首次生成事件状态。 */
class AddonEntityEvents {
public:
    /** @brief 关联程序及事件出口，spawnPending 区分新生成和存档恢复。 */
    AddonEntityEvents(
        std::shared_ptr<const EntityEventProgram> program, std::weak_ptr<EntityEventSink> sink, bool spawnPending);
    /** @brief 执行事件并将实际触发的事件 ID 发送至脚本事件出口。 */
    Result<void> trigger(mc::Entity& entity, const std::string& eventId);
    /** @brief 首次实体 tick 触发生成事件；存档恢复不会重放生成事件。 */
    void onTick(mc::Entity& entity);

private:
    std::shared_ptr<const EntityEventProgram> m_program;
    std::weak_ptr<EntityEventSink> m_sink;
    bool m_spawnPending;
};

} // namespace mc::mod::bedrock::addon
