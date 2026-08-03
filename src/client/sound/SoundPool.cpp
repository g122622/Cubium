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

#include "client/sound/SoundPool.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundTypes.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc::client::sound {

SoundInstanceId SoundPool::add(std::unique_ptr<ISoundInstance> sound)
{
    if (!sound) {
        return mc::sound::INVALID_SOUND_INSTANCE_ID;
    }

    SoundInstanceId id = m_nextId++;
    sound->setId(id);

    // 添加到映射
    const ResourceLocation& soundEventId = sound->getSoundEventId();
    SoundCategory category = sound->getCategory();

    m_soundEventMap.emplace(soundEventId, id);
    m_categoryMap.emplace(category, id);
    m_sounds.emplace(id, std::move(sound));

    return id;
}

ISoundInstance* SoundPool::get(SoundInstanceId id)
{
    auto it = m_sounds.find(id);
    return it != m_sounds.end() ? it->second.get() : nullptr;
}

const ISoundInstance* SoundPool::get(SoundInstanceId id) const
{
    auto it = m_sounds.find(id);
    return it != m_sounds.end() ? it->second.get() : nullptr;
}

bool SoundPool::remove(SoundInstanceId id)
{
    auto it = m_sounds.find(id);
    if (it == m_sounds.end()) {
        return false;
    }

    // 从辅助映射中移除
    const ResourceLocation& soundEventId = it->second->getSoundEventId();
    SoundCategory category = it->second->getCategory();

    // 从声音事件映射中移除一个
    auto range = m_soundEventMap.equal_range(soundEventId);
    for (auto eventIt = range.first; eventIt != range.second; ++eventIt) {
        if (eventIt->second == id) {
            m_soundEventMap.erase(eventIt);
            break;
        }
    }

    // 从类别映射中移除一个
    auto catRange = m_categoryMap.equal_range(category);
    for (auto catIt = catRange.first; catIt != catRange.second; ++catIt) {
        if (catIt->second == id) {
            m_categoryMap.erase(catIt);
            break;
        }
    }

    // 移除声音
    m_sounds.erase(it);
    return true;
}

void SoundPool::clear()
{
    m_sounds.clear();
    m_soundEventMap.clear();
    m_categoryMap.clear();
}

std::vector<SoundInstanceId> SoundPool::getByCategory(SoundCategory category) const
{
    std::vector<SoundInstanceId> result;

    auto range = m_categoryMap.equal_range(category);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }

    return result;
}

size_t SoundPool::removeByCategory(SoundCategory category)
{
    auto ids = getByCategory(category);
    for (SoundInstanceId id : ids) {
        remove(id);
    }
    return ids.size();
}

std::vector<SoundInstanceId> SoundPool::getBySoundEvent(const ResourceLocation& soundEventId) const
{
    std::vector<SoundInstanceId> result;

    auto range = m_soundEventMap.equal_range(soundEventId);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }

    return result;
}

size_t SoundPool::removeBySoundEvent(const ResourceLocation& soundEventId)
{
    auto ids = getBySoundEvent(soundEventId);
    for (SoundInstanceId id : ids) {
        remove(id);
    }
    return ids.size();
}

size_t SoundPool::tick()
{
    size_t removed = 0;

    // 收集已完成的声音
    std::vector<SoundInstanceId> toRemove;
    for (const auto& [id, sound] : m_sounds) {
        if (sound->isDone()) {
            toRemove.push_back(id);
        }
    }

    // 移除已完成的声音
    for (SoundInstanceId id : toRemove) {
        remove(id);
        ++removed;
    }

    return removed;
}

} // namespace mc::client::sound
