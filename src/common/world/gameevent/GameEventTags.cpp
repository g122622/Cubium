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

#include "common/world/gameevent/GameEventTag.hpp"
#include "common/world/gameevent/GameEventTags.hpp"

#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::gameevent {

// ============================================================================
// GameEventTag 实现
// ============================================================================

GameEventTag::GameEventTag(ResourceLocation id) noexcept
    : m_id(std::move(id))
{}

void GameEventTag::add(const GameEvent* event)
{
    if (event == nullptr) {
        spdlog::warn("GameEventTag::add(): Attempted to add null game event to tag '{}', skipping.", m_id.toString());
        return;
    }
    m_events.insert(event);
}

void GameEventTag::addAll(const std::vector<const GameEvent*>& events)
{
    for (const auto* event : events) {
        add(event);
    }
}

bool GameEventTag::contains(const GameEvent* event) const noexcept
{
    return event != nullptr && m_events.find(event) != m_events.end();
}

bool GameEventTag::contains(const GameEvent& event) const noexcept
{
    return m_events.find(&event) != m_events.end();
}

void GameEventTag::clear()
{
    m_events.clear();
}

// ============================================================================
// GameEventTags 实现
// ============================================================================

bool GameEventTags::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<GameEventTag>>& GameEventTags::_getTags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<GameEventTag>> tags;
    return tags;
}

GameEventTag& GameEventTags::ALL_VIBRATIONS()
{
    static GameEventTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "vibrations"));
    }
    return *tag;
}

GameEventTag& GameEventTags::IGNORE_VIBRATIONS_SNEAKING()
{
    static GameEventTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "ignore_vibrations_sneaking"));
    }
    return *tag;
}

GameEventTag& GameEventTags::ALLAY_CAN_LISTEN()
{
    static GameEventTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "allay_can_listen"));
    }
    return *tag;
}

GameEventTag& GameEventTags::SHRIEKER_CAN_LISTEN()
{
    static GameEventTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "shrieker_can_listen"));
    }
    return *tag;
}

GameEventTag& GameEventTags::WARDEN_CAN_LISTEN()
{
    static GameEventTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "warden_can_listen"));
    }
    return *tag;
}

void GameEventTags::initialize()
{
    if (s_initialized) {
        return;
    }

    auto& tags = _getTags();

    // 内置默认值须与数据包 data/minecraft/tags/game_event/*.json 一致。
    // 数据包加载器（GameEventTagLoader）随后在其上追加/替换。

    // vibrations：所有可被监听的振动事件（幽匿感测体默认监听集合）。
    auto vibrations = std::make_unique<GameEventTag>(ResourceLocation("minecraft", "vibrations"));
    vibrations->addAll({&GameEvents::BLOCK_ATTACH,
        &GameEvents::BLOCK_CHANGE,
        &GameEvents::BLOCK_CLOSE,
        &GameEvents::BLOCK_DESTROY,
        &GameEvents::BLOCK_DETACH,
        &GameEvents::BLOCK_OPEN,
        &GameEvents::BLOCK_PLACE,
        &GameEvents::BLOCK_ACTIVATE,
        &GameEvents::BLOCK_DEACTIVATE,
        &GameEvents::CONTAINER_CLOSE,
        &GameEvents::CONTAINER_OPEN,
        &GameEvents::DRINK,
        &GameEvents::EAT,
        &GameEvents::ELYTRA_GLIDE,
        &GameEvents::ENTITY_DAMAGE,
        &GameEvents::ENTITY_DIE,
        &GameEvents::ENTITY_DISMOUNT,
        &GameEvents::ENTITY_INTERACT,
        &GameEvents::ENTITY_MOUNT,
        &GameEvents::ENTITY_PLACE,
        &GameEvents::ENTITY_ACTION,
        &GameEvents::EQUIP,
        &GameEvents::EXPLODE,
        &GameEvents::FLUID_PICKUP,
        &GameEvents::FLUID_PLACE,
        &GameEvents::HIT_GROUND,
        &GameEvents::INSTRUMENT_PLAY,
        &GameEvents::ITEM_INTERACT_FINISH,
        &GameEvents::LIGHTNING_STRIKE,
        &GameEvents::NOTE_BLOCK_PLAY,
        &GameEvents::PRIME_FUSE,
        &GameEvents::PROJECTILE_LAND,
        &GameEvents::PROJECTILE_SHOOT,
        &GameEvents::SHEAR,
        &GameEvents::SPLASH,
        &GameEvents::STEP,
        &GameEvents::SWIM,
        &GameEvents::TELEPORT,
        &GameEvents::UNEQUIP,
        &GameEvents::RESONATE_1,
        &GameEvents::RESONATE_2,
        &GameEvents::RESONATE_3,
        &GameEvents::RESONATE_4,
        &GameEvents::RESONATE_5,
        &GameEvents::RESONATE_6,
        &GameEvents::RESONATE_7,
        &GameEvents::RESONATE_8,
        &GameEvents::RESONATE_9,
        &GameEvents::RESONATE_10,
        &GameEvents::RESONATE_11,
        &GameEvents::RESONATE_12,
        &GameEvents::RESONATE_13,
        &GameEvents::RESONATE_14,
        &GameEvents::RESONATE_15,
        &GameEvents::FLAP});
    tags[vibrations->getId()] = std::move(vibrations);

    // ignore_vibrations_sneaking：源实体潜行时被忽略的振动事件。
    auto ignoreSneaking = std::make_unique<GameEventTag>(ResourceLocation("minecraft", "ignore_vibrations_sneaking"));
    ignoreSneaking->addAll({&GameEvents::HIT_GROUND,
        &GameEvents::PROJECTILE_SHOOT,
        &GameEvents::STEP,
        &GameEvents::SWIM,
        &GameEvents::ITEM_INTERACT_START,
        &GameEvents::ITEM_INTERACT_FINISH});
    tags[ignoreSneaking->getId()] = std::move(ignoreSneaking);

    // allay_can_listen：悦灵可监听的事件。
    auto allayCanListen = std::make_unique<GameEventTag>(ResourceLocation("minecraft", "allay_can_listen"));
    allayCanListen->addAll({&GameEvents::NOTE_BLOCK_PLAY});
    tags[allayCanListen->getId()] = std::move(allayCanListen);

    // shrieker_can_listen：幽匿尖啸体可监听的事件。
    auto shriekerCanListen = std::make_unique<GameEventTag>(ResourceLocation("minecraft", "shrieker_can_listen"));
    shriekerCanListen->addAll({&GameEvents::SCULK_SENSOR_TENDRILS_CLICKING});
    tags[shriekerCanListen->getId()] = std::move(shriekerCanListen);

    // warden_can_listen：监守者可监听的振动事件（= vibrations + shriek + #shrieker_can_listen）。
    // GameEventTag 是扁平集合（不支持 #tag 嵌套引用），故把 vibrations 与 shrieker_can_listen
    // 的成员内联合并（同 BlockTags 中 lava_pool_stone_cannot_replace 的合并模式）。
    auto wardenCanListen = std::make_unique<GameEventTag>(ResourceLocation("minecraft", "warden_can_listen"));
    for (const auto* event : tags.at(ResourceLocation("minecraft", "vibrations"))->getEvents()) {
        wardenCanListen->add(event);
    }
    for (const auto* event : tags.at(ResourceLocation("minecraft", "shrieker_can_listen"))->getEvents()) {
        wardenCanListen->add(event);
    }
    wardenCanListen->add(&GameEvents::SHRIEK);
    tags[wardenCanListen->getId()] = std::move(wardenCanListen);

    s_initialized = true;
}

GameEventTag* GameEventTags::getTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    return it != tags.end() ? it->second.get() : nullptr;
}

GameEventTag& GameEventTags::registerTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return *it->second;
    }
    auto inserted = tags.emplace(id, std::make_unique<GameEventTag>(id));
    return *inserted.first->second;
}

void GameEventTags::forEachTag(std::function<void(GameEventTag&)> callback)
{
    for (auto& [id, tag] : _getTags()) {
        callback(*tag);
    }
}

} // namespace mc::gameevent
