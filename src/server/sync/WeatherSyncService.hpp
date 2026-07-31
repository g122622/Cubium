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

#pragma once

#include "common/core/Types.hpp"

namespace mc::server {

class MinecraftServer;

} // namespace mc::server

namespace mc::server::sync {

/**
 * @brief 天气同步影子状态服务
 *
 * 承接原 MinecraftServer::sendWeatherUpdate / sendInitialWeatherStateToPlayer
 * 的网络同步职责。维护两份"上次已发送"的降雨/雷暴强度影子状态，每 tick 比对
 * 主世界 WeatherManager 的当前强度，超阈值时广播 GameEvent(RainStrengthChange=7
 * /ThunderStrengthChange=8)；天气状态切换时广播 EndRaining=1/BeginRaining=2。
 *
 * 全局单份，仅主世界驱动：Minecraft 1.21.11 天气仅存在于主世界（overworld），
 * 下界/末地无天气演算。本服务固定从主世界 ServerWorld 的 WeatherManager 取值，
 * 不接受维度参数。
 *
 * 不持有天气演算状态（那属于世界级 WeatherManager），仅持网络同步用的影子状态
 * 与发送原语引用。构造须在 MinecraftServer 的 m_dimensionManager 及主世界
 * WeatherManager 就绪之后。
 */
class WeatherSyncService {
public:
    /**
     * @brief 构造天气同步服务
     *
     * @param server MinecraftServer 引用（经 dimensionManager() 取主世界天气源，
     *               经 broadcastPacket/sendPacketToPlayer 发包）
     */
    explicit WeatherSyncService(MinecraftServer& server);

    /**
     * @brief 每 tick 同步天气变化给所有在线玩家
     *
     * 比对主世界当前 rainStrength/thunderStrength 与影子状态，超阈值(0.001)时
     * 广播强度变更；天气状态切换时广播开始/结束降雨。原 MinecraftServer::sendWeatherUpdate。
     */
    void tick();

    /**
     * @brief 向指定玩家发送当前天气的初始强度
     *
     * 玩家登录/进入世界时调用，无条件发送当前 rainStrength/thunderStrength
     * （无变化检测）。原 MinecraftServer::sendInitialWeatherStateToPlayer。
     *
     * @param playerId 目标玩家ID
     */
    void sendInitialWeatherStateToPlayer(PlayerId playerId);

private:
    MinecraftServer& m_server;
    f32 m_lastSentRainStrength = 0.0f;
    f32 m_lastSentThunderStrength = 0.0f;
};

} // namespace mc::server::sync
