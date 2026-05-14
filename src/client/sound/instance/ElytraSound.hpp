#pragma once

#include "client/sound/instance/SoundInstance.hpp"

namespace mc::client {

// 前向声明
class ClientEntity;

} // namespace mc::client

namespace mc::client::sound {

/**
 * @brief 鞘翅飞行声音
 *
 * 当玩家使用鞘翅滑翔时播放的循环声音。
 * 音量根据玩家速度变化。
 *
 * 参考: net.minecraft.client.audio.ElytraSound
 */
class ElytraSound : public TickableSound {
public:
    /**
     * @brief 构造鞘翅声音
     *
     * @param player 玩家客户端实体引用
     */
    explicit ElytraSound(const ClientEntity& player);

    ~ElytraSound() override = default;

    // 禁止拷贝
    ElytraSound(const ElytraSound&) = delete;
    ElytraSound& operator=(const ElytraSound&) = delete;

    // 允许移动
    ElytraSound(ElytraSound&&) noexcept = default;
    ElytraSound& operator=(ElytraSound&&) noexcept = default;

    // ========================================================================
    // TickableSound 接口
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 根据玩家的鞘翅飞行状态和速度更新音量。
     */
    void tick() override;

private:
    const ClientEntity& m_player;
    i32 m_time = 0; // 时间计数器
};

} // namespace mc::client::sound
