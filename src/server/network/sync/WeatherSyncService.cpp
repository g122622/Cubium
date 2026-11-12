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

#include "server/network/sync/WeatherSyncService.hpp"

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/weather/WeatherState.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <cmath>
#include <utility>

using namespace mc::trace;

namespace mc::server::sync {

WeatherSyncService::WeatherSyncService(MinecraftServer& server)
    : m_server(server)
{}

void WeatherSyncService::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "sendWeatherUpdate", "phase", "weather_sync");

    constexpr f32 STRENGTH_THRESHOLD = 0.001f;

    // 每个有天气能力的维度独立演算、独立广播（对应原版逐维度
    // ServerLevel.advanceWeatherCycle + PlayerList.broadcastAll(packet, dimension)）。
    for (DimensionId dimensionId : m_server.dimensionManager().getDimensionIds()) {
        auto* dimension = m_server.dimensionManager().getDimension(dimensionId);
        if (dimension == nullptr) {
            continue;
        }

        auto* world = dimension->world();
        auto* weatherMgr = world != nullptr ? world->weatherManager() : nullptr;
        if (weatherMgr == nullptr) {
            continue;
        }

        DimensionShadow& shadow = m_shadows[dimensionId];
        const f32 rainStrength = weatherMgr->rainStrength(1.0f);
        // 原版广播 thunderLevel 原始值，客户端 getThunderLevel 再乘降雨强度。
        const f32 thunderStrength = weatherMgr->rawThunderStrength(1.0f);

        if (std::abs(rainStrength - shadow.lastRainStrength) > STRENGTH_THRESHOLD) {
            _broadcastGameEvent(dimensionId, 7, rainStrength); // RainStrengthChange
            shadow.lastRainStrength = rainStrength;
        }

        if (std::abs(thunderStrength - shadow.lastThunderStrength) > STRENGTH_THRESHOLD) {
            _broadcastGameEvent(dimensionId, 8, thunderStrength); // ThunderStrengthChange
            shadow.lastThunderStrength = thunderStrength;
        }

        if (weatherMgr->hasWeatherChanged()) {
            if (weatherMgr->weatherType() == weather::WeatherType::Clear) {
                _broadcastGameEvent(dimensionId, 1, 0.0f); // EndRaining
            } else {
                _broadcastGameEvent(dimensionId, 2, 0.0f); // BeginRaining
            }

            // 原版天气切换时补发当前强度，避免强度变化未超过阈值时客户端状态滞后。
            _broadcastGameEvent(dimensionId, 7, rainStrength);
            _broadcastGameEvent(dimensionId, 8, thunderStrength);
            shadow.lastRainStrength = rainStrength;
            shadow.lastThunderStrength = thunderStrength;
        }
    }
}

void WeatherSyncService::sendInitialWeatherStateToPlayer(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "SendInitialWeatherState", "phase", "weather_sync");

    // 使用玩家当前维度，而不是固定主世界。
    const DimensionId dimensionId = m_server.dimensionManager().getPlayerDimension(playerId);
    auto* dimension = m_server.dimensionManager().getDimension(dimensionId);
    auto* world = dimension != nullptr ? dimension->world() : nullptr;
    auto* weatherMgr = world != nullptr ? world->weatherManager() : nullptr;
    if (weatherMgr == nullptr) {
        return;
    }

    // 原版 PlayerList.sendLevelInfo：仅在该维度正在下雨时下发天气包整簇。
    if (!weatherMgr->isRaining()) {
        return;
    }

    _sendGameEvent(playerId, 2, 0.0f); // BeginRaining
    _sendGameEvent(playerId, 7, weatherMgr->rainStrength(1.0f));
    // 原版初始同步发送 getThunderLevel(1.0F)，即已乘降雨强度的值。
    _sendGameEvent(playerId, 8, weatherMgr->thunderStrength(1.0f));
}

void WeatherSyncService::_broadcastGameEvent(DimensionId dimensionId, i32 event, f32 value)
{
    auto* dimension = m_server.dimensionManager().getDimension(dimensionId);
    if (dimension == nullptr) {
        return;
    }

    mc::network::ir::play::GameEvent evt;
    evt.event = event;
    evt.value = value;
    const mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(evt)},
    };

    for (PlayerId playerId : dimension->players()) {
        m_server.sendPacketToPlayer(playerId, packet);
    }
}

void WeatherSyncService::_sendGameEvent(PlayerId playerId, i32 event, f32 value)
{
    mc::network::ir::play::GameEvent evt;
    evt.event = event;
    evt.value = value;
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(evt)},
        });
}

} // namespace mc::server::sync
