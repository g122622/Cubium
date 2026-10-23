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

#include "server/mod/bedrock/addon/bridge/EventBridge.hpp"

#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/mod/bedrock/addon/content/base/damage/BedrockDamageCause.hpp"
#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"
#include "common/mod/bedrock/addon/event/ScriptEventData.hpp"
#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"
#include "common/world/IWorld.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"

#include <any>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::server {

namespace {
using namespace mc::mod::bedrock::addon;

mc::Player* _findPlayer(PlayerId id)
{
    for (auto* world : ScriptWorldAccessor::instance().getDimensions()) {
        for (auto* entity : world->getPlayers()) {
            auto* player = dynamic_cast<mc::Player*>(entity);
            if (player != nullptr && player->playerId() == id) {
                return player;
            }
        }
    }

    return nullptr;
}

mc::IWorld* _findWorld(DimensionId id)
{
    for (auto* world : ScriptWorldAccessor::instance().getDimensions()) {
        if (world->dimension() == id) {
            return world;
        }
    }

    return nullptr;
}

ScriptEventData _position(const Vector3d& pos)
{
    ScriptEventData result;
    result.setNumber("x", pos.x);
    result.setNumber("y", pos.y);
    result.setNumber("z", pos.z);

    return result;
}

// TODO: 补齐爆炸方块集合、破坏前 permutation 及可修改事件字段的回写。
template <typename Event>
ScriptEventData _snapshot(const Event& nativeEvent)
{
    ScriptEventData data;
    data.cancelled = nativeEvent.isCancelled();

    // 先确定事件主体及维度，之后的方块、物品和伤害快照共用同一事件上下文。
    mc::IWorld* world = nullptr;
    if constexpr (requires { nativeEvent.playerId; }) {
        auto* player = _findPlayer(nativeEvent.playerId);
        data.setEntity("player", player);
        data.setEntity("source", player);
        world = player != nullptr ? player->world() : nullptr;
    }

    if constexpr (requires { nativeEvent.entity; }) {
        data.setEntity("entity", nativeEvent.entity);
        world = nativeEvent.entity != nullptr ? nativeEvent.entity->world() : world;
        if constexpr (std::is_same_v<Event, event::EntityHurtEvent>) {
            data.setEntity("hurtEntity", nativeEvent.entity);
        }

        if constexpr (std::is_same_v<Event, event::EntityDeathEvent>) {
            data.setEntity("deadEntity", nativeEvent.entity);
        }
    }

    if constexpr (requires { nativeEvent.dimension; }) {
        world = _findWorld(nativeEvent.dimension);
    }

    data.setDimension("dimension", world);

    // 伤害来源可能位于发布者的调用栈上，必须在当前调用内提取字段和实体引用。
    if constexpr (requires { nativeEvent.cause; }) {
        ScriptEventData source;
        if (nativeEvent.cause != nullptr) {
            source.setString("cause", std::string(damageCauseName(nativeEvent.cause->type())));
            source.setEntity("damagingEntity", nativeEvent.cause->source());
            if (nativeEvent.cause->isProjectile()) {
                source.setEntity("damagingProjectile", nativeEvent.cause->directSource());
            }
        } else {
            source.setString("cause", "none");
        }

        data.setObject("damageSource", std::move(source));
    }

    if constexpr (requires { nativeEvent.damage; }) {
        data.setNumber("damage", nativeEvent.damage);
    }

    if constexpr (requires {
                      nativeEvent.state;
                      nativeEvent.pos;
                  }) {
        data.setBlock("block", world, nativeEvent.pos, nativeEvent.state);
    }

    // 物品和聊天文本保存值副本，延迟分发不再依赖原生事件对象的存活。
    if constexpr (requires { nativeEvent.item; }) {
        if constexpr (std::is_pointer_v<decltype(nativeEvent.item)>) {
            if (nativeEvent.item != nullptr) {
                data.setItem("itemStack", *nativeEvent.item);
            }
        } else {
            data.setItem("itemStack", nativeEvent.item);
        }
    }

    if constexpr (requires { nativeEvent.tool; }) {
        if (nativeEvent.tool != nullptr) {
            data.setItem("itemStackBeforeBreak", *nativeEvent.tool);
        }
    }

    if constexpr (requires { nativeEvent.message; }) {
        data.setString("message", nativeEvent.message);
        data.setEntity("sender", _findPlayer(nativeEvent.playerId));
    }

    if constexpr (requires { nativeEvent.username; }) {
        data.setString("playerName", nativeEvent.username);
    }

    if constexpr (requires { nativeEvent.reason; }) {
        if constexpr (std::is_same_v<decltype(nativeEvent.reason), std::string>) {
            data.setString("reason", nativeEvent.reason);
        }
    }

    if constexpr (requires { nativeEvent.position; }) {
        data.setObject("location", _position(nativeEvent.position));
    }

    if constexpr (requires { nativeEvent.currentTick; }) {
        data.setNumber("currentTick", static_cast<f64>(nativeEvent.currentTick));
    }

    if constexpr (requires { nativeEvent.raining; }) {
        data.setBoolean("raining", nativeEvent.raining);
        data.setString("newWeather", nativeEvent.raining ? "Rain" : "Clear");
    }

    if constexpr (requires {
                      nativeEvent.power;
                      nativeEvent.source;
                  }) {
        data.setEntity("source", nativeEvent.source);
        data.setNumber("power", nativeEvent.power);
    }

    // 首次加入和重生共用 playerSpawn 信号，通过载荷区分首次出生。
    if constexpr (std::is_same_v<Event, event::PlayerLoginEvent>) {
        data.setBoolean("initialSpawn", true);
    }

    if constexpr (std::is_same_v<Event, event::PlayerRespawnEvent>) {
        data.setBoolean("initialSpawn", false);
    }

    return data;
}
} // namespace

EventBridge::EventBridge() = default;

EventBridge::~EventBridge()
{
    shutdown();
}

EventBridge::EventBridge(EventBridge&& other) noexcept
    : m_subscriptionIds(std::move(other.m_subscriptionIds))
    , m_initialized(other.m_initialized)
{
    other.m_initialized = false;
}

EventBridge& EventBridge::operator=(EventBridge&& other) noexcept
{
    if (this != &other) {
        shutdown();
        m_subscriptionIds = std::move(other.m_subscriptionIds);
        m_initialized = other.m_initialized;
        other.m_initialized = false;
    }

    return *this;
}

void EventBridge::initialize(
    event::ServerEventBus& serverEventBus, mc::mod::bedrock::addon::ScriptEventBus& scriptEventBus)
{
    if (m_initialized) {
        return;
    }

    spdlog::info("[EventBridge] Initializing event bridge...");

    _subscribeBeforeEvents(serverEventBus, scriptEventBus);
    _subscribeAfterEvents(serverEventBus, scriptEventBus);

    m_initialized = true;
    spdlog::info("[EventBridge] Event bridge initialized ({} subscriptions)", m_subscriptionIds.size());
}

void EventBridge::shutdown()
{
    if (!m_initialized) {
        return;
    }

    auto& bus = event::ServerEventBus::instance();
    for (auto id : m_subscriptionIds) {
        bus.unsubscribe(id);
    }

    m_subscriptionIds.clear();
    m_initialized = false;

    spdlog::info("[EventBridge] Event bridge shut down");
}

bool EventBridge::isInitialized() const
{
    return m_initialized;
}

void EventBridge::_subscribeBeforeEvents(event::ServerEventBus& bus, mc::mod::bedrock::addon::ScriptEventBus& scriptBus)
{
    // BlockBreakEvent — beforeEvent: scripts can cancel block breaking
    m_subscriptionIds.push_back(bus.subscribe<event::BlockBreakEvent>([&scriptBus](const event::BlockBreakEvent& e) {
        auto data = std::any(_snapshot(e));
        if (scriptBus.dispatchBeforeEvent(typeid(event::BlockBreakEvent), data)) {
            e.cancel();
        }
    }));

    // BlockPlaceEvent — beforeEvent: scripts can cancel block placement
    m_subscriptionIds.push_back(bus.subscribe<event::BlockPlaceEvent>([&scriptBus](const event::BlockPlaceEvent& e) {
        auto data = std::any(_snapshot(e));
        if (scriptBus.dispatchBeforeEvent(typeid(event::BlockPlaceEvent), data)) {
            e.cancel();
        }
    }));

    // ChatEvent — beforeEvent: scripts can cancel chat messages
    m_subscriptionIds.push_back(bus.subscribe<event::ChatEvent>([&scriptBus](const event::ChatEvent& e) {
        auto data = std::any(_snapshot(e));
        if (scriptBus.dispatchBeforeEvent(typeid(event::ChatEvent), data)) {
            e.cancel();
        }
    }));

    // EntityHurtEvent — beforeEvent: scripts can cancel entity damage
    m_subscriptionIds.push_back(bus.subscribe<event::EntityHurtEvent>([&scriptBus](const event::EntityHurtEvent& e) {
        auto data = std::any(_snapshot(e));
        if (scriptBus.dispatchBeforeEvent(typeid(event::EntityHurtEvent), data)) {
            e.cancel();
        }
    }));

    // PlayerHurtEvent — beforeEvent: scripts can cancel player damage
    m_subscriptionIds.push_back(bus.subscribe<event::PlayerHurtEvent>([&scriptBus](const event::PlayerHurtEvent& e) {
        auto data = std::any(_snapshot(e));
        if (scriptBus.dispatchBeforeEvent(typeid(event::PlayerHurtEvent), data)) {
            e.cancel();
        }
    }));

    // WeatherChangeEvent — beforeEvent: scripts can cancel weather changes
    m_subscriptionIds.push_back(
        bus.subscribe<event::WeatherChangeEvent>([&scriptBus](const event::WeatherChangeEvent& e) {
            auto data = std::any(_snapshot(e));
            if (scriptBus.dispatchBeforeEvent(typeid(event::WeatherChangeEvent), data)) {
                e.cancel();
            }
        }));

    // ExplosionEvent — beforeEvent: scripts can cancel explosions
    m_subscriptionIds.push_back(bus.subscribe<event::ExplosionEvent>([&scriptBus](const event::ExplosionEvent& e) {
        auto data = std::any(_snapshot(e));
        if (scriptBus.dispatchBeforeEvent(typeid(event::ExplosionEvent), data)) {
            e.cancel();
        }
    }));

    // ItemUseEvent — beforeEvent: scripts can cancel item usage
    m_subscriptionIds.push_back(bus.subscribe<event::ItemUseEvent>([&scriptBus](const event::ItemUseEvent& e) {
        auto data = std::any(_snapshot(e));
        if (scriptBus.dispatchBeforeEvent(typeid(event::ItemUseEvent), data)) {
            e.cancel();
        }
    }));

    // PlayerLoginEvent — beforeEvent: scripts can cancel player login
    m_subscriptionIds.push_back(bus.subscribe<event::PlayerLoginEvent>([&scriptBus](const event::PlayerLoginEvent& e) {
        auto data = std::any(_snapshot(e));
        if (scriptBus.dispatchBeforeEvent(typeid(event::PlayerLoginEvent), data)) {
            e.cancel();
        }
    }));
}

void EventBridge::_subscribeAfterEvents(event::ServerEventBus& bus, mc::mod::bedrock::addon::ScriptEventBus& scriptBus)
{
    // === Player afterEvents ===
    m_subscriptionIds.push_back(bus.subscribe<event::PlayerLoginEvent>([&scriptBus](const event::PlayerLoginEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::PlayerLoginEvent), std::any(_snapshot(e)));
            scriptBus.enqueueAfterEvent(typeid(ScriptPlayerSpawnEvent), std::any(_snapshot(e)));
        }
    }));
    m_subscriptionIds.push_back(
        bus.subscribe<event::PlayerLogoutEvent>([&scriptBus](const event::PlayerLogoutEvent& e) {
            scriptBus.enqueueAfterEvent(typeid(event::PlayerLogoutEvent), std::any(_snapshot(e)));
        }));
    m_subscriptionIds.push_back(
        bus.subscribe<event::PlayerRespawnEvent>([&scriptBus](const event::PlayerRespawnEvent& e) {
            scriptBus.enqueueAfterEvent(typeid(event::PlayerRespawnEvent), std::any(_snapshot(e)));
            scriptBus.enqueueAfterEvent(typeid(ScriptPlayerSpawnEvent), std::any(_snapshot(e)));
        }));
    m_subscriptionIds.push_back(
        bus.subscribe<event::DimensionChangeEvent>([&scriptBus](const event::DimensionChangeEvent& e) {
            scriptBus.enqueueAfterEvent(typeid(event::DimensionChangeEvent), std::any(_snapshot(e)));
        }));

    // === Block afterEvents (only if not cancelled) ===
    m_subscriptionIds.push_back(bus.subscribe<event::BlockBreakEvent>([&scriptBus](const event::BlockBreakEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::BlockBreakEvent), std::any(_snapshot(e)));
        }
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::BlockPlaceEvent>([&scriptBus](const event::BlockPlaceEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::BlockPlaceEvent), std::any(_snapshot(e)));
        }
    }));

    // === Chat afterEvent (only if not cancelled) ===
    m_subscriptionIds.push_back(bus.subscribe<event::ChatEvent>([&scriptBus](const event::ChatEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::ChatEvent), std::any(_snapshot(e)));
        }
    }));

    // === Entity afterEvents ===
    m_subscriptionIds.push_back(bus.subscribe<event::EntityDeathEvent>([&scriptBus](const event::EntityDeathEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::EntityDeathEvent), std::any(_snapshot(e)));
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::EntityHurtEvent>([&scriptBus](const event::EntityHurtEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::EntityHurtEvent), std::any(_snapshot(e)));
        }
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::EntitySpawnEvent>([&scriptBus](const event::EntitySpawnEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::EntitySpawnEvent), std::any(_snapshot(e)));
    }));

    // === Item afterEvents ===
    m_subscriptionIds.push_back(bus.subscribe<event::ItemPickupEvent>([&scriptBus](const event::ItemPickupEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::ItemPickupEvent), std::any(_snapshot(e)));
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::ItemDropEvent>([&scriptBus](const event::ItemDropEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::ItemDropEvent), std::any(_snapshot(e)));
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::ItemUseEvent>([&scriptBus](const event::ItemUseEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::ItemUseEvent), std::any(_snapshot(e)));
        }
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::ConsumeItemEvent>([&scriptBus](const event::ConsumeItemEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::ConsumeItemEvent), std::any(_snapshot(e)));
    }));

    // === World afterEvents ===
    m_subscriptionIds.push_back(
        bus.subscribe<event::WorldInitializeEvent>([&scriptBus](const event::WorldInitializeEvent& e) {
            scriptBus.enqueueAfterEvent(typeid(event::WorldInitializeEvent), std::any(_snapshot(e)));
        }));
    m_subscriptionIds.push_back(bus.subscribe<event::ServerTickEvent>([&scriptBus](const event::ServerTickEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::ServerTickEvent), std::any(_snapshot(e)));
    }));
    m_subscriptionIds.push_back(
        bus.subscribe<event::WeatherChangeEvent>([&scriptBus](const event::WeatherChangeEvent& e) {
            if (!e.isCancelled()) {
                scriptBus.enqueueAfterEvent(typeid(event::WeatherChangeEvent), std::any(_snapshot(e)));
            }
        }));

    // === Explosion afterEvent (only if not cancelled) ===
    m_subscriptionIds.push_back(bus.subscribe<event::ExplosionEvent>([&scriptBus](const event::ExplosionEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::ExplosionEvent), std::any(_snapshot(e)));
        }
    }));
}

} // namespace mc::server
