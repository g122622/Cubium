#pragma once

#include "client/sound/instance/SoundInstance.hpp"

namespace mc::client {

// 前向声明
class ClientEntity;

}

namespace mc::client::sound {

/**
 * @brief 守卫者攻击声音
 *
 * 当守卫者正在攻击目标时播放的循环声音。
 * 音量根据攻击动画进度变化。
 *
 * 参考: net.minecraft.client.audio.GuardianSound
 */
class GuardianSound : public TickableSound {
public:
    /**
     * @brief 构造守卫者声音
     *
     * @param guardian 守卫者客户端实体引用
     */
    explicit GuardianSound(const ClientEntity& guardian);

    ~GuardianSound() override = default;

    // 禁止拷贝
    GuardianSound(const GuardianSound&) = delete;
    GuardianSound& operator=(const GuardianSound&) = delete;

    // 允许移动
    GuardianSound(GuardianSound&&) noexcept = default;
    GuardianSound& operator=(GuardianSound&&) noexcept = default;

    // ========================================================================
    // TickableSound 接口
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 根据守卫者的攻击目标状态更新音量和音调。
     */
    void tick() override;

    /**
     * @brief 是否可以静音播放
     */
    [[nodiscard]] bool canBeSilent() const override { return true; }

private:
    const ClientEntity& m_guardian;
};

} // namespace mc::client::sound
