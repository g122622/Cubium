#pragma once

#include "client/sound/instance/SoundInstance.hpp"

namespace mc {

// 前向声明
class Entity;

namespace client::sound {

/**
 * @brief 矿车行驶音效
 *
 * 当矿车移动时播放行驶音效，音量根据矿车速度动态变化。
 *
 * 参考: net.minecraft.client.audio.MinecartTickableSound
 *
 * 特点:
 * - 循环播放
 * - 音量根据水平速度变化（0 ~ 0.7）
 * - 有 distance 字段用于平滑音量变化
 * - canBeSilent() 返回 true
 */
class MinecartTickableSound : public TickableSound {
public:
    /**
     * @brief 构造函数
     *
     * @param minecart 矿车实体引用
     */
    explicit MinecartTickableSound(Entity& minecart);

    /**
     * @brief 每帧更新
     *
     * 更新位置和音量。
     */
    void tick() override;

    /**
     * @brief 是否可以静音播放
     */
    [[nodiscard]] bool canBeSilent() const override { return true; }

private:
    /// 矿车实体引用
    Entity& m_minecart;

    /// 音量平滑距离值
    f32 m_distance = 0.0f;
};

/**
 * @brief 玩家骑乘矿车时的内部音效
 *
 * 当玩家骑乘矿车时播放内部音效，使用无衰减模式。
 *
 * 参考: net.minecraft.client.audio.RidingMinecartTickableSound
 *
 * 特点:
 * - 循环播放
 * - 无衰减（玩家内部声音）
 * - 音量根据水平速度变化（0 ~ 0.75）
 * - canBeSilent() 返回 true
 */
class RidingMinecartTickableSound : public TickableSound {
public:
    /**
     * @brief 构造函数
     *
     * @param player 玩家实体引用
     * @param minecart 矿车实体引用
     */
    RidingMinecartTickableSound(Entity& player, Entity& minecart);

    /**
     * @brief 每帧更新
     *
     * 更新位置和音量，检查玩家是否仍在骑乘。
     */
    void tick() override;

    /**
     * @brief 是否可以静音播放
     */
    [[nodiscard]] bool canBeSilent() const override { return true; }

private:
    /// 玩家实体引用
    Entity& m_player;

    /// 矿车实体引用
    Entity& m_minecart;
};

} // namespace mc::client::sound
} // namespace mc
