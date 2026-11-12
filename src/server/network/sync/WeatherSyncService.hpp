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
#include <unordered_map>

namespace mc::server {

class MinecraftServer;

} // namespace mc::server

namespace mc::server::sync {

/**
 * @brief 天气同步影子状态服务
 *
 * 承接原 MinecraftServer::sendWeatherUpdate / sendInitialWeatherStateToPlayer
 * 的网络同步职责。按维度维护"上次已发送"的降雨/雷暴强度影子状态，每 tick 比对
 * 该维度 WeatherManager 的当前强度，超阈值时向该维度玩家广播
 * GameEvent(RainStrengthChange=7 / ThunderStrengthChange=8)；天气状态切换时
 * 广播该维度的 EndRaining=1 / BeginRaining=2。
 *
 * 每个有天气能力的维度独立演算、独立同步（对应原版 ServerLevel 逐维度
 * advanceWeatherCycle + PlayerList.broadcastAll(packet, dimension)）。
 *
 * 不持有天气演算状态（那属于世界级 WeatherManager），仅持网络同步用的影子状态
 * 与发送原语引用。
 */
class WeatherSyncService {
public:
    /**
     * @brief 构造天气同步服务
     *
     * @param server MinecraftServer 引用（经 dimensionManager() 取各维度天气源，
     *               经 sendPacketToPlayer 发包）
     */
    explicit WeatherSyncService(MinecraftServer& server);

    /**
     * @brief 每 tick 同步各维度天气变化给该维度在线玩家
     *
     * 对每个维度比对当前 rainStrength/rawThunderStrength 与影子状态，超阈值(0.001)
     * 时广播强度变更；天气状态切换时广播该维度的开始/结束降雨。
     */
    void tick();

    /**
     * @brief 向指定玩家发送其所在维度的初始天气状态
     *
     * 玩家登录时调用。与原版 PlayerList.sendLevelInfo 一致：仅当该维度正在下雨时
     * 发送 BeginRaining + 降雨强度 + 雷暴强度（雷暴强度已乘降雨强度）。
     *
     * @param playerId 目标玩家ID
     */
    void sendInitialWeatherStateToPlayer(PlayerId playerId);

private:
    /// 单个维度的网络同步影子状态
    struct DimensionShadow {
        f32 lastRainStrength = 0.0f;
        f32 lastThunderStrength = 0.0f;
    };

    /**
     * @brief 向指定维度的所有玩家广播一个 GameEvent
     *
     * @param dimensionId 目标维度
     * @param event 事件编号
     * @param value 事件数值
     */
    void _broadcastGameEvent(DimensionId dimensionId, i32 event, f32 value);

    /**
     * @brief 向单个玩家发送一个 GameEvent
     *
     * @param playerId 目标玩家
     * @param event 事件编号
     * @param value 事件数值
     */
    void _sendGameEvent(PlayerId playerId, i32 event, f32 value);

    MinecraftServer& m_server;
    std::unordered_map<DimensionId, DimensionShadow> m_shadows;
};

} // namespace mc::server::sync
