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

#include "GameEventTagLoader.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/resource/tag/GenericTagLoader.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEventTags.hpp"
#include "common/world/gameevent/GameEvents.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

namespace {

/// 游戏事件标签目录（相对 data/<namespace>/）
constexpr std::string_view GAME_EVENT_TAG_DIRECTORY = "tags/game_event";

/**
 * @brief 游戏事件指针池
 *
 * 通用加载器以 size_t 索引传递成员。本池把 const GameEvent* 映射为稠密索引，
 * 使内置标签成员（由 GameEventTags::initialize 直接以指针注册）与数据包成员
 * 共用同一索引空间。
 */
class _GameEventMemberPool {
public:
    std::size_t getOrCreate(const gameevent::GameEvent* event)
    {
        auto it = m_indexByEvent.find(event);
        if (it != m_indexByEvent.end()) {
            return it->second;
        }
        std::size_t index = m_events.size();
        m_events.push_back(event);
        m_indexByEvent.emplace(event, index);
        return index;
    }

    [[nodiscard]] const gameevent::GameEvent* at(std::size_t index) const { return m_events.at(index); }

private:
    std::vector<const gameevent::GameEvent*> m_events;
    std::unordered_map<const gameevent::GameEvent*, std::size_t> m_indexByEvent;
};

void _buildCallbacks(_GameEventMemberPool& pool,
    resource::tag::TagMemberResolver& resolveMember,
    resource::tag::TagMemberReader& readTag,
    resource::tag::TagFiller& fillTag)
{
    resolveMember = [&pool](std::string_view memberId) -> std::size_t {
        // 数据包成员为 "minecraft:step" 形式，getGameEventById 期望不含命名空间的路径。
        std::string id(memberId);
        const auto colonPos = id.find(':');
        if (colonPos != std::string::npos) {
            id = id.substr(colonPos + 1);
        }
        const gameevent::GameEvent* event = gameevent::GameEvents::getGameEventById(id);
        if (event == nullptr) {
            return resource::tag::TAG_MEMBER_NOT_FOUND;
        }
        return pool.getOrCreate(event);
    };

    readTag = [&pool](const ResourceLocation& tagId) -> std::vector<std::size_t> {
        std::vector<std::size_t> members;
        auto* tag = gameevent::GameEventTags::getTag(tagId);
        if (tag == nullptr) {
            return members;
        }
        for (const auto* event : tag->getEvents()) {
            members.push_back(pool.getOrCreate(event));
        }
        return members;
    };

    fillTag = [&pool](
                  const ResourceLocation& tagId, const std::vector<std::size_t>& members, bool replace) {
        gameevent::GameEventTag& tag = gameevent::GameEventTags::registerTag(tagId);
        if (replace) {
            tag.clear();
        }
        for (std::size_t index : members) {
            tag.add(pool.at(index));
        }
    };
}

} // namespace

Result<size_t> GameEventTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    _GameEventMemberPool pool;
    resource::tag::TagMemberResolver resolveMember;
    resource::tag::TagMemberReader readTag;
    resource::tag::TagFiller fillTag;
    _buildCallbacks(pool, resolveMember, readTag, fillTag);

    return resource::tag::GenericTagLoader::loadFromDataPackRepository(
        dataPackList, GAME_EVENT_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

Result<size_t> GameEventTagLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    _GameEventMemberPool pool;
    resource::tag::TagMemberResolver resolveMember;
    resource::tag::TagMemberReader readTag;
    resource::tag::TagFiller fillTag;
    _buildCallbacks(pool, resolveMember, readTag, fillTag);

    return resource::tag::GenericTagLoader::loadFromResourcePack(
        pack, GAME_EVENT_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

} // namespace mc
