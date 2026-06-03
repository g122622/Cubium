#include "server/mod/bedrock/addon/bridge/EventBridge.hpp"

#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"

#include <typeindex>
#include <spdlog/spdlog.h>

namespace mc::server {

EventBridge::EventBridge() = default;

EventBridge::~EventBridge()
{
    shutdown();
}

void EventBridge::initialize(
    event::ServerEventBus& serverEventBus, mc::mod::bedrock::addon::ScriptEventBus& scriptEventBus)
{
    if (m_initialized) {
        return;
    }

    spdlog::info("[EventBridge] Initializing event bridge...");

    subscribeBeforeEvents(serverEventBus, scriptEventBus);
    subscribeAfterEvents(serverEventBus, scriptEventBus);

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

void EventBridge::subscribeBeforeEvents(event::ServerEventBus& bus, mc::mod::bedrock::addon::ScriptEventBus& scriptBus)
{
    // BlockBreakEvent — beforeEvent: scripts can cancel block breaking
    m_subscriptionIds.push_back(bus.subscribe<event::BlockBreakEvent>([&scriptBus](const event::BlockBreakEvent& e) {
        auto data = std::make_any<event::BlockBreakEvent>(e);
        if (scriptBus.dispatchBeforeEvent(typeid(event::BlockBreakEvent), data)) {
            e.cancel();
        }
    }));

    // BlockPlaceEvent — beforeEvent: scripts can cancel block placement
    m_subscriptionIds.push_back(bus.subscribe<event::BlockPlaceEvent>([&scriptBus](const event::BlockPlaceEvent& e) {
        auto data = std::make_any<event::BlockPlaceEvent>(e);
        if (scriptBus.dispatchBeforeEvent(typeid(event::BlockPlaceEvent), data)) {
            e.cancel();
        }
    }));

    // ChatEvent — beforeEvent: scripts can cancel chat messages
    m_subscriptionIds.push_back(bus.subscribe<event::ChatEvent>([&scriptBus](const event::ChatEvent& e) {
        auto data = std::make_any<event::ChatEvent>(e);
        if (scriptBus.dispatchBeforeEvent(typeid(event::ChatEvent), data)) {
            e.cancel();
        }
    }));

    // EntityHurtEvent — beforeEvent: scripts can cancel entity damage
    m_subscriptionIds.push_back(bus.subscribe<event::EntityHurtEvent>([&scriptBus](const event::EntityHurtEvent& e) {
        auto data = std::make_any<event::EntityHurtEvent>(e);
        if (scriptBus.dispatchBeforeEvent(typeid(event::EntityHurtEvent), data)) {
            e.cancel();
        }
    }));

    // PlayerHurtEvent — beforeEvent: scripts can cancel player damage
    m_subscriptionIds.push_back(bus.subscribe<event::PlayerHurtEvent>([&scriptBus](const event::PlayerHurtEvent& e) {
        auto data = std::make_any<event::PlayerHurtEvent>(e);
        if (scriptBus.dispatchBeforeEvent(typeid(event::PlayerHurtEvent), data)) {
            e.cancel();
        }
    }));

    // WeatherChangeEvent — beforeEvent: scripts can cancel weather changes
    m_subscriptionIds.push_back(
        bus.subscribe<event::WeatherChangeEvent>([&scriptBus](const event::WeatherChangeEvent& e) {
            auto data = std::make_any<event::WeatherChangeEvent>(e);
            if (scriptBus.dispatchBeforeEvent(typeid(event::WeatherChangeEvent), data)) {
                e.cancel();
            }
        }));

    // ExplosionEvent — beforeEvent: scripts can cancel explosions
    m_subscriptionIds.push_back(bus.subscribe<event::ExplosionEvent>([&scriptBus](const event::ExplosionEvent& e) {
        auto data = std::make_any<event::ExplosionEvent>(e);
        if (scriptBus.dispatchBeforeEvent(typeid(event::ExplosionEvent), data)) {
            e.cancel();
        }
    }));

    // ItemUseEvent — beforeEvent: scripts can cancel item usage
    m_subscriptionIds.push_back(bus.subscribe<event::ItemUseEvent>([&scriptBus](const event::ItemUseEvent& e) {
        auto data = std::make_any<event::ItemUseEvent>(e);
        if (scriptBus.dispatchBeforeEvent(typeid(event::ItemUseEvent), data)) {
            e.cancel();
        }
    }));

    // PlayerLoginEvent — beforeEvent: scripts can cancel player login
    m_subscriptionIds.push_back(bus.subscribe<event::PlayerLoginEvent>([&scriptBus](const event::PlayerLoginEvent& e) {
        auto data = std::make_any<event::PlayerLoginEvent>(e);
        if (scriptBus.dispatchBeforeEvent(typeid(event::PlayerLoginEvent), data)) {
            e.cancel();
        }
    }));
}

void EventBridge::subscribeAfterEvents(event::ServerEventBus& bus, mc::mod::bedrock::addon::ScriptEventBus& scriptBus)
{
    // === Player afterEvents ===
    m_subscriptionIds.push_back(bus.subscribe<event::PlayerLoginEvent>([&scriptBus](const event::PlayerLoginEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::PlayerLoginEvent), std::make_any<event::PlayerLoginEvent>(e));
        }
    }));
    m_subscriptionIds.push_back(
        bus.subscribe<event::PlayerLogoutEvent>([&scriptBus](const event::PlayerLogoutEvent& e) {
            scriptBus.enqueueAfterEvent(typeid(event::PlayerLogoutEvent), std::make_any<event::PlayerLogoutEvent>(e));
        }));
    m_subscriptionIds.push_back(
        bus.subscribe<event::PlayerRespawnEvent>([&scriptBus](const event::PlayerRespawnEvent& e) {
            scriptBus.enqueueAfterEvent(typeid(event::PlayerRespawnEvent), std::make_any<event::PlayerRespawnEvent>(e));
        }));
    m_subscriptionIds.push_back(bus.subscribe<event::DimensionChangeEvent>([&scriptBus](
                                                                               const event::DimensionChangeEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::DimensionChangeEvent), std::make_any<event::DimensionChangeEvent>(e));
    }));

    // === Block afterEvents (only if not cancelled) ===
    m_subscriptionIds.push_back(bus.subscribe<event::BlockBreakEvent>([&scriptBus](const event::BlockBreakEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::BlockBreakEvent), std::make_any<event::BlockBreakEvent>(e));
        }
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::BlockPlaceEvent>([&scriptBus](const event::BlockPlaceEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::BlockPlaceEvent), std::make_any<event::BlockPlaceEvent>(e));
        }
    }));

    // === Chat afterEvent (only if not cancelled) ===
    m_subscriptionIds.push_back(bus.subscribe<event::ChatEvent>([&scriptBus](const event::ChatEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::ChatEvent), std::make_any<event::ChatEvent>(e));
        }
    }));

    // === Entity afterEvents ===
    m_subscriptionIds.push_back(bus.subscribe<event::EntityDeathEvent>([&scriptBus](const event::EntityDeathEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::EntityDeathEvent), std::make_any<event::EntityDeathEvent>(e));
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::EntityHurtEvent>([&scriptBus](const event::EntityHurtEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::EntityHurtEvent), std::make_any<event::EntityHurtEvent>(e));
        }
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::EntitySpawnEvent>([&scriptBus](const event::EntitySpawnEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::EntitySpawnEvent), std::make_any<event::EntitySpawnEvent>(e));
    }));

    // === Item afterEvents ===
    m_subscriptionIds.push_back(bus.subscribe<event::ItemPickupEvent>([&scriptBus](const event::ItemPickupEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::ItemPickupEvent), std::make_any<event::ItemPickupEvent>(e));
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::ItemDropEvent>([&scriptBus](const event::ItemDropEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::ItemDropEvent), std::make_any<event::ItemDropEvent>(e));
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::ItemUseEvent>([&scriptBus](const event::ItemUseEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::ItemUseEvent), std::make_any<event::ItemUseEvent>(e));
        }
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::ConsumeItemEvent>([&scriptBus](const event::ConsumeItemEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::ConsumeItemEvent), std::make_any<event::ConsumeItemEvent>(e));
    }));

    // === World afterEvents ===
    m_subscriptionIds.push_back(bus.subscribe<event::WorldInitializeEvent>([&scriptBus](
                                                                               const event::WorldInitializeEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::WorldInitializeEvent), std::make_any<event::WorldInitializeEvent>(e));
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::ServerTickEvent>([&scriptBus](const event::ServerTickEvent& e) {
        scriptBus.enqueueAfterEvent(typeid(event::ServerTickEvent), std::make_any<event::ServerTickEvent>(e));
    }));
    m_subscriptionIds.push_back(bus.subscribe<event::WeatherChangeEvent>([&scriptBus](
                                                                             const event::WeatherChangeEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::WeatherChangeEvent), std::make_any<event::WeatherChangeEvent>(e));
        }
    }));

    // === Explosion afterEvent (only if not cancelled) ===
    m_subscriptionIds.push_back(bus.subscribe<event::ExplosionEvent>([&scriptBus](const event::ExplosionEvent& e) {
        if (!e.isCancelled()) {
            scriptBus.enqueueAfterEvent(typeid(event::ExplosionEvent), std::make_any<event::ExplosionEvent>(e));
        }
    }));
}

} // namespace mc::server
