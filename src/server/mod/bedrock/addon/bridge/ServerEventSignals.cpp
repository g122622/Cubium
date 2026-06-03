#include "server/mod/bedrock/addon/bridge/ServerEventSignals.hpp"

#include "server/event/events/ServerEvents.hpp"

#include <typeindex>

namespace mc::server {

using namespace mc::mod::bedrock::addon;
using namespace mc::server::event;

std::vector<EventSignalInfo> getBeforeEventSignals()
{
    return {
        // 方块事件
        {"blockBreak", typeid(BlockBreakEvent), true},
        {"blockPlace", typeid(BlockPlaceEvent), true},

        // 聊天事件
        {"chatSend", typeid(ChatEvent), true},

        // 实体事件
        {"entityHurt", typeid(EntityHurtEvent), true},
        {"playerHurt", typeid(PlayerHurtEvent), true},

        // 物品事件
        {"itemUse", typeid(ItemUseEvent), true},

        // 世界事件
        {"weatherChange", typeid(WeatherChangeEvent), true},
        {"explosion", typeid(ExplosionEvent), true},

        // 玩家事件
        {"playerJoin", typeid(PlayerLoginEvent), true},
    };
}

std::vector<EventSignalInfo> getAfterEventSignals()
{
    return {
        // 玩家事件
        {"playerJoin", typeid(PlayerLoginEvent), false},
        {"playerLeave", typeid(PlayerLogoutEvent), false},
        {"playerRespawn", typeid(PlayerRespawnEvent), false},
        {"playerDimensionChange", typeid(DimensionChangeEvent), false},

        // 方块事件
        {"blockBreak", typeid(BlockBreakEvent), false},
        {"blockPlace", typeid(BlockPlaceEvent), false},

        // 聊天事件
        {"chatSend", typeid(ChatEvent), false},

        // 实体事件
        {"entityDeath", typeid(EntityDeathEvent), false},
        {"entityHurt", typeid(EntityHurtEvent), false},
        {"entitySpawn", typeid(EntitySpawnEvent), false},

        // 物品事件
        {"itemPickup", typeid(ItemPickupEvent), false},
        {"itemDrop", typeid(ItemDropEvent), false},
        {"itemUse", typeid(ItemUseEvent), false},
        {"itemConsume", typeid(ConsumeItemEvent), false},

        // 世界事件
        {"worldInitialize", typeid(WorldInitializeEvent), false},
        {"serverTick", typeid(ServerTickEvent), false},
        {"weatherChange", typeid(WeatherChangeEvent), false},
        {"explosion", typeid(ExplosionEvent), false},
    };
}

} // namespace mc::server
