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

#include "server/mod/bedrock/addon/bridge/ServerEventSignals.hpp"

#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"
#include "server/event/events/ServerEvents.hpp"
#include <vector>

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
