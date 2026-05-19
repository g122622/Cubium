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

#include "../../core/Types.hpp"
#include "Packet.hpp"

namespace mc::network {

/**
 * @brief 游戏状态变化原因
 *
 * 参考 MC 1.16.5 SChangeGameStatePacket
 */
enum class GameStateChangeReason : u8 {
    InvalidBed = 0,              // 床无效（在白天或非主维度使用）
    EndRaining = 1,              // 雨停
    BeginRaining = 2,            // 开始下雨
    ChangeGameMode = 3,          // 游戏模式改变
    WinGame = 4,                 // 胜利（进入终末之诗）
    DemoEvent = 5,               // 演示事件
    ArrowHitPlayer = 6,          // 箭击中玩家
    RainStrengthChange = 7,      // 降雨强度变化
    ThunderStrengthChange = 8,   // 雷暴强度变化
    PlayPufferFishSting = 9,     // 河豚刺痛
    PlayElderGuardianCurse = 10, // 远古守卫者诅咒
    EnableRespawnScreen = 11,    // 启用重生屏幕
    LimitedCrafting = 12,        // 限制合成
    StartWaitingChunks = 13      // 开始等待区块
};

/**
 * @brief 游戏状态变化包
 *
 * 用于同步游戏状态变化，包括：
 * - 天气变化（开始/停止下雨，强度变化）
 * - 游戏模式改变
 * - 床无效通知
 * - 其他游戏事件
 *
 * 参考 MC 1.16.5 SChangeGameStatePacket
 */
class GameStateChangePacket : public Packet {
public:
    GameStateChangePacket();
    GameStateChangePacket(GameStateChangeReason reason, f32 value);

    /**
     * @brief 创建雨停包
     */
    static GameStateChangePacket endRain() { return GameStateChangePacket(GameStateChangeReason::EndRaining, 0.0f); }

    /**
     * @brief 创建开始下雨包
     */
    static GameStateChangePacket beginRain()
    {
        return GameStateChangePacket(GameStateChangeReason::BeginRaining, 0.0f);
    }

    /**
     * @brief 创建降雨强度变化包
     *
     * @param strength 降雨强度 (0.0 - 1.0)
     */
    static GameStateChangePacket rainStrength(f32 strength)
    {
        return GameStateChangePacket(GameStateChangeReason::RainStrengthChange, strength);
    }

    /**
     * @brief 创建雷暴强度变化包
     *
     * @param strength 雷暴强度 (0.0 - 1.0)
     */
    static GameStateChangePacket thunderStrength(f32 strength)
    {
        return GameStateChangePacket(GameStateChangeReason::ThunderStrengthChange, strength);
    }

    /**
     * @brief 创建游戏模式变化包
     *
     * @param mode 新的游戏模式
     */
    static GameStateChangePacket gameModeChange(GameMode mode)
    {
        return GameStateChangePacket(GameStateChangeReason::ChangeGameMode, static_cast<f32>(mode));
    }

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    GameStateChangeReason reason() const { return m_reason; }
    f32 value() const { return m_value; }

private:
    GameStateChangeReason m_reason = GameStateChangeReason::InvalidBed;
    f32 m_value = 0.0f;
};

} // namespace mc::network
