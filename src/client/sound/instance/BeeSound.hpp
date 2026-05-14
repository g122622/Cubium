#pragma once

#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"

namespace mc::client {

// 前向声明
class ClientEntity;

} // namespace mc::client

namespace mc::client::sound {

/**
 * @brief 蜜蜂声音基类
 *
 * 蜜蜂飞行和愤怒声音的基类，根据蜜蜂的速度动态调整音量和音调。
 * 支持在飞行声音和愤怒声音之间切换。
 *
 * 参考: net.minecraft.client.audio.BeeSound
 */
class BeeSound : public TickableSound {
public:
    /**
     * @brief 构造蜜蜂声音
     *
     * @param bee 蜜蜂客户端实体引用
     * @param soundEventId 声音事件ID
     */
    BeeSound(const ClientEntity& bee, const ResourceLocation& soundEventId);

    ~BeeSound() override = default;

    // 禁止拷贝
    BeeSound(const BeeSound&) = delete;
    BeeSound& operator=(const BeeSound&) = delete;

    // 允许移动
    BeeSound(BeeSound&&) noexcept = default;
    BeeSound& operator=(BeeSound&&) noexcept = default;

    // ========================================================================
    // TickableSound 接口
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 根据蜜蜂的速度调整音量和音调，
     * 如果需要切换声音则播放下一个声音。
     */
    void tick() override;

    /**
     * @brief 是否可以静音播放
     *
     * 蜜蜂声音可以在音量为0时播放，用于声音切换。
     */
    [[nodiscard]] bool canBeSilent() const override { return true; }

protected:
    /**
     * @brief 获取下一个声音
     *
     * 在飞行声音和愤怒声音之间切换。
     */
    [[nodiscard]] virtual std::unique_ptr<TickableSound> getNextSound() = 0;

    /**
     * @brief 检查是否应该切换声音
     */
    [[nodiscard]] virtual bool shouldSwitchSound() = 0;

    /**
     * @brief 获取蜜蜂实体
     */
    [[nodiscard]] const ClientEntity& bee() const { return m_bee; }

private:
    /**
     * @brief 获取最小音调
     *
     * 幼年蜜蜂音调更高。
     */
    [[nodiscard]] f32 getMinPitch() const;

    /**
     * @brief 获取最大音调
     *
     * 幼年蜜蜂音调更高。
     */
    [[nodiscard]] f32 getMaxPitch() const;

    const ClientEntity& m_bee;
    bool m_hasSwitchedSound = false;
};

/**
 * @brief 蜜蜂飞行声音
 *
 * 蜜蜂正常飞行时播放的循环声音。
 * 当蜜蜂变得愤怒时切换到愤怒声音。
 *
 * 参考: net.minecraft.client.audio.BeeFlightSound
 */
class BeeFlightSound : public BeeSound {
public:
    /**
     * @brief 构造飞行声音
     *
     * @param bee 蜜蜂客户端实体引用
     */
    explicit BeeFlightSound(const ClientEntity& bee);

    ~BeeFlightSound() override = default;

protected:
    [[nodiscard]] std::unique_ptr<TickableSound> getNextSound() override;
    [[nodiscard]] bool shouldSwitchSound() override;
};

/**
 * @brief 蜜蜂愤怒声音
 *
 * 蜜蜂愤怒（攻击目标）时播放的循环声音。
 * 当蜜蜂不再愤怒时切换回飞行声音。
 *
 * 参考: net.minecraft.client.audio.BeeAngrySound
 */
class BeeAngrySound : public BeeSound {
public:
    /**
     * @brief 构造愤怒声音
     *
     * @param bee 蜜蜂客户端实体引用
     */
    explicit BeeAngrySound(const ClientEntity& bee);

    ~BeeAngrySound() override = default;

protected:
    [[nodiscard]] std::unique_ptr<TickableSound> getNextSound() override;
    [[nodiscard]] bool shouldSwitchSound() override;
};

} // namespace mc::client::sound
