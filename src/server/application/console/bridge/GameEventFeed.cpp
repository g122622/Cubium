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

#include "GameEventFeed.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTag.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/event/events/ServerEvents.hpp"

#include <fmt/format.h>

#include <utility>

namespace mc::server::console {

namespace {

/// 读取实体在终端中显示的名字：优先自定义名，其次实体类型 id。
std::string _entityLabel(const Entity* entity)
{
    if (entity == nullptr) {
        return "unknown";
    }
    if (entity->hasCustomName()) {
        return entity->customNameText();
    }
    return entity->getTypeId();
}

/// 去掉 ResourceLocation 的 "minecraft:" 前缀，缩短终端显示宽度。
std::string _shortId(std::string_view id)
{
    constexpr std::string_view NAMESPACE_PREFIX = "minecraft:";
    if (id.starts_with(NAMESPACE_PREFIX)) {
        return std::string(id.substr(NAMESPACE_PREFIX.size()));
    }
    return std::string(id);
}

} // namespace

GameEventFeed::GameEventFeed(EventRingBuffer& events, std::function<void()> onEventAvailable)
    : m_events(events)
    , m_onEventAvailable(std::move(onEventAvailable))
{
    MC_ASSERT_RELEASE_MSG(m_onEventAvailable != nullptr, "GameEventFeed requires a wakeup callback");

    auto& bus = mc::server::event::ServerEventBus::instance();

    m_subscriptions.push_back(
        bus.subscribe<mc::server::event::ChatEvent>([this](const mc::server::event::ChatEvent& event) {
            _emit(TerminalEventKind::Chat, fmt::format("<{}> {}", event.username, event.message));
        }));

    m_subscriptions.push_back(
        bus.subscribe<mc::server::event::PlayerLoginEvent>([this](const mc::server::event::PlayerLoginEvent& event) {
            _emit(TerminalEventKind::Player, fmt::format("{} joined the game", event.username));
        }));

    m_subscriptions.push_back(
        bus.subscribe<mc::server::event::PlayerLogoutEvent>([this](const mc::server::event::PlayerLogoutEvent& event) {
            const std::string who = event.username.empty() ? "a player" : event.username;
            if (event.reason.empty()) {
                _emit(TerminalEventKind::Player, fmt::format("{} left the game", who));
                return;
            }
            _emit(TerminalEventKind::Player, fmt::format("{} left the game ({})", who, event.reason));
        }));

    m_subscriptions.push_back(
        bus.subscribe<mc::server::event::EntityDeathEvent>([this](const mc::server::event::EntityDeathEvent& event) {
            if (event.entity == nullptr) {
                return;
            }

            // 过滤生物噪声：世界中每只动物/怪物死亡都上报会淹没其它信息，只保留玩家死亡
            // 与玩家击杀目标（原版也只广播玩家死亡）。
            const bool isPlayer = dynamic_cast<Player*>(event.entity) != nullptr;
            const bool killedByPlayer = event.killer != nullptr && dynamic_cast<Player*>(event.killer) != nullptr;
            if (!isPlayer && !killedByPlayer) {
                return;
            }

            std::string cause = "unknown";
            if (event.cause != nullptr) {
                cause = _shortId(DamageTypeNames::getResourceLocation(event.cause->type()).toString());
            }

            std::string text = fmt::format("{} died ({})", _entityLabel(event.entity), cause);
            if (event.killer != nullptr) {
                text += fmt::format(", killed by {}", _entityLabel(event.killer));
            }
            _emit(TerminalEventKind::Game, std::move(text));
        }));

    m_subscriptions.push_back(bus.subscribe<mc::server::event::RecipeUnlockedEvent>(
        [this](const mc::server::event::RecipeUnlockedEvent& event) {
            _emit(TerminalEventKind::Game,
                fmt::format("player {} unlocked recipe {}", event.playerId, event.recipeId.toString()));
        }));

    m_subscriptions.push_back(bus.subscribe<mc::server::event::WeatherChangeEvent>(
        [this](const mc::server::event::WeatherChangeEvent& event) {
            _emit(TerminalEventKind::Game,
                fmt::format("dimension {} weather -> {}", event.dimension, event.raining ? "rain" : "clear"));
        }));
}

GameEventFeed::~GameEventFeed()
{
    auto& bus = mc::server::event::ServerEventBus::instance();
    for (const auto id : m_subscriptions) {
        bus.unsubscribe(id);
    }
}

void GameEventFeed::_emit(TerminalEventKind kind, std::string text)
{
    m_events.push(kind, std::move(text));
    m_onEventAvailable();
}

} // namespace mc::server::console
