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

#include "client/sound/resource/SoundRegistry.hpp"
#include "client/sound/resource/SoundDefinition.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace mc::client::sound {

void SoundRegistry::registerSoundEvent(SoundEventDefinition definition)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!definition.location.path().empty()) {
        auto it = m_soundEvents.find(definition.location);

        if (it != m_soundEvents.end()) {
            // 已存在同名声音事件
            if (definition.replace) {
                // 替换模式：完全覆盖
                it->second = std::move(definition);
            } else {
                // 追加模式：合并声音列表
                // 注意：不覆盖 subtitle，保留原来的
                auto& existing = it->second;
                for (auto& sound : definition.sounds) {
                    existing.sounds.push_back(std::move(sound));
                }
            }
        } else {
            // 新声音事件
            ResourceLocation id = definition.location;
            m_soundEvents.emplace(std::move(id), std::move(definition));
        }
    }
}

const SoundEventDefinition* SoundRegistry::getSoundEvent(const ResourceLocation& id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_soundEvents.find(id);
    if (it != m_soundEvents.end()) {
        return &it->second;
    }
    return nullptr;
}

bool SoundRegistry::hasSoundEvent(const ResourceLocation& id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_soundEvents.find(id) != m_soundEvents.end();
}

std::vector<ResourceLocation> SoundRegistry::getAllSoundEventIds() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<ResourceLocation> ids;
    ids.reserve(m_soundEvents.size());

    for (const auto& [id, def] : m_soundEvents) {
        ids.push_back(id);
    }

    // 按 ID 排序，保证顺序一致
    std::sort(ids.begin(), ids.end());

    return ids;
}

size_t SoundRegistry::getSoundEventCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_soundEvents.size();
}

void SoundRegistry::merge(const SoundRegistry& other)
{
    if (this == &other) {
        return;
    }

    std::scoped_lock lock(m_mutex, other.m_mutex);

    for (const auto& [id, definition] : other.m_soundEvents) {
        auto it = m_soundEvents.find(id);

        if (it != m_soundEvents.end()) {
            if (definition.replace) {
                // 替换模式
                it->second = definition;
            } else {
                // 追加模式
                auto& existing = it->second;
                for (const auto& sound : definition.sounds) {
                    existing.sounds.push_back(sound);
                }
            }
        } else {
            m_soundEvents.emplace(id, definition);
        }
    }
}

void SoundRegistry::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_soundEvents.clear();
}

std::vector<ResourceLocation> SoundRegistry::getPreloadSounds() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<ResourceLocation> preloadList;

    for (const auto& [id, eventDef] : m_soundEvents) {
        for (const auto& soundDef : eventDef.sounds) {
            if (soundDef.preload && soundDef.type == SoundType::File) {
                preloadList.push_back(soundDef.toOggLocation());
            }
        }
    }

    return preloadList;
}

} // namespace mc::client::sound
