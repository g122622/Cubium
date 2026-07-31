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

#include "server/sync/WeatherSyncService.hpp"

#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <cmath>

using namespace mc::trace;

namespace mc::server::sync {

WeatherSyncService::WeatherSyncService(MinecraftServer& server)
    : m_server(server)
{}

void WeatherSyncService::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "sendWeatherUpdate", "phase", "weather_sync");

    // 天气仅存在于主世界
    auto* overworld = m_server.dimensionManager().getOverworld();
    if (!overworld || !overworld->world() || !overworld->world()->weatherManager()) return;

    auto& weatherMgr = *overworld->world()->weatherManager();
    f32 rainStrength = weatherMgr.rainStrength();
    f32 thunderStrength = weatherMgr.thunderStrength();

    constexpr f32 STRENGTH_THRESHOLD = 0.001f;
    bool rainChanged = std::abs(rainStrength - m_lastSentRainStrength) > STRENGTH_THRESHOLD;
    bool thunderChanged = std::abs(thunderStrength - m_lastSentThunderStrength) > STRENGTH_THRESHOLD;

    if (rainChanged) {
        mc::network::ir::play::GameEvent evt;
        evt.event = 7; // RainStrengthChange
        evt.value = rainStrength;
        m_server.broadcastPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(evt)},
        });
        m_lastSentRainStrength = rainStrength;
    }

    if (thunderChanged) {
        mc::network::ir::play::GameEvent evt;
        evt.event = 8; // ThunderStrengthChange
        evt.value = thunderStrength;
        m_server.broadcastPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(evt)},
        });
        m_lastSentThunderStrength = thunderStrength;
    }

    if (weatherMgr.hasWeatherChanged()) {
        auto weatherType = weatherMgr.weatherType();
        if (weatherType == weather::WeatherType::Clear) {
            mc::network::ir::play::GameEvent evt;
            evt.event = 1; // EndRaining
            evt.value = 0.0f;
            m_server.broadcastPacket(mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(evt)},
            });
        } else if (weatherType == weather::WeatherType::Rain || weatherType == weather::WeatherType::Thunder) {
            mc::network::ir::play::GameEvent evt;
            evt.event = 2; // BeginRaining
            evt.value = 0.0f;
            m_server.broadcastPacket(mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(evt)},
            });
        }
    }
}

void WeatherSyncService::sendInitialWeatherStateToPlayer(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "SendInitialWeatherState", "phase", "weather_sync");

    // 天气仅存在于主世界
    auto* overworld = m_server.dimensionManager().getOverworld();
    if (!overworld || !overworld->world() || !overworld->world()->weatherManager()) return;

    auto& weatherMgr = *overworld->world()->weatherManager();
    f32 rainStrength = weatherMgr.rainStrength();
    f32 thunderStrength = weatherMgr.thunderStrength();

    {
        mc::network::ir::play::GameEvent evt;
        evt.event = 7; // RainStrengthChange
        evt.value = rainStrength;
        m_server.sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(evt)},
            });
    }

    {
        mc::network::ir::play::GameEvent evt;
        evt.event = 8; // ThunderStrengthChange
        evt.value = thunderStrength;
        m_server.sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(evt)},
            });
    }
}

} // namespace mc::server::sync
