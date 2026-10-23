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

#include "common/mod/bedrock/addon/content/entity/events/AddonEntityEvents.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventProgram.hpp"
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

AddonEntityEvents::AddonEntityEvents(
    std::shared_ptr<const EntityEventProgram> program, std::weak_ptr<EntityEventSink> sink, bool spawnPending)
    : m_program(std::move(program))
    , m_sink(std::move(sink))
    , m_spawnPending(spawnPending)
{}

Result<void> AddonEntityEvents::trigger(mc::Entity& entity, const std::string& eventId)
{
    auto triggered = m_program->execute(entity, eventId);
    if (triggered.failed()) {
        return triggered.error();
    }
    if (auto sink = m_sink.lock()) {
        for (const auto& name : triggered.value()) {
            (*sink)(entity, name);
        }
    }
    return Result<void>::ok();
}

void AddonEntityEvents::onTick(mc::Entity& entity)
{
    if (!m_spawnPending) {
        return;
    }
    m_spawnPending = false;
    if (m_program->contains("minecraft:entity_spawned")) {
        auto result = trigger(entity, "minecraft:entity_spawned");
        if (result.failed()) {
            spdlog::warn("Failed to execute spawn event for '{}': {}", entity.getTypeId(), result.error().message());
        }
    }
}

} // namespace mc::mod::bedrock::addon
