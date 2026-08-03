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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/GameEvent.hpp"

#include <string>
#include <unordered_map>

namespace mc::gameevent {
namespace GameEvents {

namespace {
/**
 * @brief 延迟初始化的事件查找表
 *
 * 使用函数局部静态变量避免静态初始化顺序问题
 * （GameEvents 命名空间中的全局变量是 inline const，初始化顺序不确定）
 */
const std::unordered_map<std::string, const GameEvent*>& getEventMap()
{
    static const std::unordered_map<std::string, const GameEvent*> eventMap = {
        // 方块事件
        {"block_activate", &BLOCK_ACTIVATE},
        {"block_attach", &BLOCK_ATTACH},
        {"block_change", &BLOCK_CHANGE},
        {"block_close", &BLOCK_CLOSE},
        {"block_deactivate", &BLOCK_DEACTIVATE},
        {"block_destroy", &BLOCK_DESTROY},
        {"block_detach", &BLOCK_DETACH},
        {"block_open", &BLOCK_OPEN},
        {"block_place", &BLOCK_PLACE},

        // 容器事件
        {"container_close", &CONTAINER_CLOSE},
        {"container_open", &CONTAINER_OPEN},

        // 实体事件
        {"drink", &DRINK},
        {"eat", &EAT},
        {"elytra_glide", &ELYTRA_GLIDE},
        {"entity_damage", &ENTITY_DAMAGE},
        {"entity_die", &ENTITY_DIE},
        {"entity_dismount", &ENTITY_DISMOUNT},
        {"entity_interact", &ENTITY_INTERACT},
        {"entity_mount", &ENTITY_MOUNT},
        {"entity_place", &ENTITY_PLACE},
        {"entity_action", &ENTITY_ACTION},
        {"equip", &EQUIP},
        {"unequip", &UNEQUIP},

        // 环境事件
        {"explode", &EXPLODE},
        {"flap", &FLAP},
        {"fluid_pickup", &FLUID_PICKUP},
        {"fluid_place", &FLUID_PLACE},
        {"hit_ground", &HIT_GROUND},
        {"lightning_strike", &LIGHTNING_STRIKE},
        {"splash", &SPLASH},
        {"step", &STEP},
        {"swim", &SWIM},
        {"teleport", &TELEPORT},

        // 物品与交互事件
        {"instrument_play", &INSTRUMENT_PLAY},
        {"item_interact_finish", &ITEM_INTERACT_FINISH},
        {"item_interact_start", &ITEM_INTERACT_START},
        {"note_block_play", &NOTE_BLOCK_PLAY},
        {"prime_fuse", &PRIME_FUSE},
        {"projectile_land", &PROJECTILE_LAND},
        {"projectile_shoot", &PROJECTILE_SHOOT},
        {"shear", &SHEAR},

        // 唱片机事件
        {"jukebox_play", &JUKEBOX_PLAY},
        {"jukebox_stop_play", &JUKEBOX_STOP_PLAY},

        // 幽匿事件
        {"sculk_sensor_tendrils_clicking", &SCULK_SENSOR_TENDRILS_CLICKING},
        {"shriek", &SHRIEK},

        // 共鸣事件
        {"resonate_1", &RESONATE_1},
        {"resonate_2", &RESONATE_2},
        {"resonate_3", &RESONATE_3},
        {"resonate_4", &RESONATE_4},
        {"resonate_5", &RESONATE_5},
        {"resonate_6", &RESONATE_6},
        {"resonate_7", &RESONATE_7},
        {"resonate_8", &RESONATE_8},
        {"resonate_9", &RESONATE_9},
        {"resonate_10", &RESONATE_10},
        {"resonate_11", &RESONATE_11},
        {"resonate_12", &RESONATE_12},
        {"resonate_13", &RESONATE_13},
        {"resonate_14", &RESONATE_14},
        {"resonate_15", &RESONATE_15},
    };
    return eventMap;
}
} // namespace

const GameEvent* getGameEventById(const std::string& id)
{
    const auto& map = getEventMap();
    auto it = map.find(id);
    return (it != map.end()) ? it->second : nullptr;
}

} // namespace GameEvents
} // namespace mc::gameevent
