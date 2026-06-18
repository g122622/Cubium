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

/**
 * @file DynamicGameEventListener.cpp
 * @brief 动态游戏事件监听器实现
 */

#include "common/world/gameevent/DynamicGameEventListener.hpp"

#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc::gameevent {

void DynamicGameEventListener::add(server::ServerWorld& world)
{
    move(world);
}

void DynamicGameEventListener::remove(server::ServerWorld& world)
{
    _ifChunkExists(
        world,
        m_lastSection,
        [this](GameEventListenerRegistry& registry) { registry.unregisterListener(m_listener); },
        false);
}

void DynamicGameEventListener::move(server::ServerWorld& world)
{
    // 获取监听器的当前位置
    auto position = m_listener.getListenerSource().getPosition(world);
    if (!position.has_value()) {
        return;
    }

    // 计算当前区块段位置
    mc::world::chunk::SectionPos newSection(
        mc::world::chunk::SectionPos::fromBlockPos(BlockPos(static_cast<i32>(std::floor(position->x)),
            static_cast<i32>(std::floor(position->y)),
            static_cast<i32>(std::floor(position->z)))));

    // 如果段位置未变化，无需移动
    if (m_lastSection.has_value() && m_lastSection.value() == newSection) {
        return;
    }

    // 从旧段注销
    _ifChunkExists(
        world,
        m_lastSection,
        [this](GameEventListenerRegistry& registry) { registry.unregisterListener(m_listener); },
        false);

    // 更新段位置
    m_lastSection = newSection;

    // 在新段注册（需要创建注册表如果不存在）
    _ifChunkExists(
        world,
        m_lastSection,
        [this](GameEventListenerRegistry& registry) { registry.registerListener(m_listener); },
        true);
}

void DynamicGameEventListener::_ifChunkExists(server::ServerWorld& world,
    const std::optional<mc::world::chunk::SectionPos>& sectionPos,
    const std::function<void(GameEventListenerRegistry&)>& action,
    bool createIfMissing)
{
    if (!sectionPos.has_value()) {
        return;
    }

    const auto& sp = sectionPos.value();
    ChunkData* chunk = world.chunkManager()->tryToGetChunkInMem(sp.x, sp.z);
    if (chunk == nullptr) {
        return;
    }

    if (createIfMissing) {
        // 注册监听器时，如果注册表不存在则创建
        auto factory = [&world](i32 sectionY) -> std::unique_ptr<EuclideanGameEventListenerRegistry> {
            return std::make_unique<EuclideanGameEventListenerRegistry>(
                world, sectionY, [&world, sectionY](i32 /*emptySectionY*/) {
                    // 当注册表为空时，从区块中移除（延迟处理，避免在遍历中修改）
                    // 在 MC 中，这是通过 SectionNotifier 回调实现的
                    // 此处简化处理：空的注册表会在下次访问时被跳过
                });
        };
        GameEventListenerRegistry& registry = chunk->getOrCreateGameEventListenerRegistry(sp.y, factory);
        action(registry);
    } else {
        // 注销监听器时，只需访问已存在的注册表
        GameEventListenerRegistry* registry = chunk->getGameEventListenerRegistry(sp.y);
        if (registry != nullptr) {
            action(*registry);
        }
    }
}

} // namespace mc::gameevent
