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
#include "common/network/packet/Packet.hpp"

namespace mc {

// Forward declarations
class Player;
struct PlayerAbilities;

namespace network {

/**
 * @brief 玩家能力标志位
 */
enum class PlayerAbilityFlags : u8 {
    None = 0,
    Invulnerable = 0x01, // bit 0: 无敌
    Flying = 0x02,       // bit 1: 飞行中
    CanFly = 0x04,       // bit 2: 可飞行
    CreativeMode = 0x08  // bit 3: 创造模式
};

/**
 * @brief 玩家能力数据包
 *
 * 同步玩家能力到客户端，包括：
 * - 无敌状态
 * - 飞行状态
 * - 是否可以飞行
 * - 是否为创造模式
 * - 飞行速度
 * - 行走速度
 *
 * 协议格式:
 * | 字段     | 类型 | 说明                                    |
 * |----------|------|-----------------------------------------|
 * | flags    | u8   | bit0:无敌, bit1:飞行中, bit2:可飞行, bit3:创造 |
 * | flySpeed | f32  | 飞行速度                                |
 * | walkSpeed| f32  | 行走速度                                |
 */
class PlayerAbilitiesPacket : public Packet {
public:
    PlayerAbilitiesPacket();

    /**
     * @brief 从玩家能力构造
     *
     * @param abilities 玩家能力结构
     */
    explicit PlayerAbilitiesPacket(const PlayerAbilities& abilities);

    /**
     * @brief 从玩家对象构造
     *
     * @param player 玩家对象
     * @return 玩家能力数据包
     */
    static PlayerAbilitiesPacket fromPlayer(const Player& player);

    /**
     * @brief 从游戏模式构造默认能力
     *
     * @param mode 游戏模式
     * @return 玩家能力数据包
     */
    static PlayerAbilitiesPacket fromGameMode(GameMode mode);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getters ==========

    /**
     * @brief 是否无敌
     */
    [[nodiscard]] bool invulnerable() const
    {
        return (m_flags & static_cast<u8>(PlayerAbilityFlags::Invulnerable)) != 0;
    }

    /**
     * @brief 是否正在飞行
     */
    [[nodiscard]] bool flying() const { return (m_flags & static_cast<u8>(PlayerAbilityFlags::Flying)) != 0; }

    /**
     * @brief 是否可以飞行
     */
    [[nodiscard]] bool canFly() const { return (m_flags & static_cast<u8>(PlayerAbilityFlags::CanFly)) != 0; }

    /**
     * @brief 是否为创造模式
     */
    [[nodiscard]] bool creativeMode() const
    {
        return (m_flags & static_cast<u8>(PlayerAbilityFlags::CreativeMode)) != 0;
    }

    /**
     * @brief 获取飞行速度
     */
    [[nodiscard]] f32 flySpeed() const { return m_flySpeed; }

    /**
     * @brief 获取行走速度
     */
    [[nodiscard]] f32 walkSpeed() const { return m_walkSpeed; }

    /**
     * @brief 获取原始标志位
     */
    [[nodiscard]] u8 flags() const { return m_flags; }

    // ========== Setters ==========

    /**
     * @brief 设置无敌状态
     */
    void setInvulnerable(bool value)
    {
        if (value) {
            m_flags |= static_cast<u8>(PlayerAbilityFlags::Invulnerable);
        } else {
            m_flags &= ~static_cast<u8>(PlayerAbilityFlags::Invulnerable);
        }
    }

    /**
     * @brief 设置飞行状态
     */
    void setFlying(bool value)
    {
        if (value) {
            m_flags |= static_cast<u8>(PlayerAbilityFlags::Flying);
        } else {
            m_flags &= ~static_cast<u8>(PlayerAbilityFlags::Flying);
        }
    }

    /**
     * @brief 设置是否可飞行
     */
    void setCanFly(bool value)
    {
        if (value) {
            m_flags |= static_cast<u8>(PlayerAbilityFlags::CanFly);
        } else {
            m_flags &= ~static_cast<u8>(PlayerAbilityFlags::CanFly);
        }
    }

    /**
     * @brief 设置创造模式标志
     */
    void setCreativeMode(bool value)
    {
        if (value) {
            m_flags |= static_cast<u8>(PlayerAbilityFlags::CreativeMode);
        } else {
            m_flags &= ~static_cast<u8>(PlayerAbilityFlags::CreativeMode);
        }
    }

    /**
     * @brief 设置飞行速度
     */
    void setFlySpeed(f32 speed) { m_flySpeed = speed; }

    /**
     * @brief 设置行走速度
     */
    void setWalkSpeed(f32 speed) { m_walkSpeed = speed; }

private:
    u8 m_flags = 0;
    f32 m_flySpeed = 0.05f; // MC 默认飞行速度
    f32 m_walkSpeed = 0.1f; // MC 默认行走速度
};

} // namespace network
} // namespace mc
