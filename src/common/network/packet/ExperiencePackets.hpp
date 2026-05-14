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

#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include "Packet.hpp"
#include "PacketSerializer.hpp"

namespace mc {

// 前向声明
class Player;

namespace network {

/**
 * @brief 经验同步包 (S->C)
 *
 * 服务端向客户端同步玩家的经验状态。
 *
 * 参考 MC 1.16.5 SSetExperiencePacket
 *
 * 协议格式:
 * | 字段       | 类型 | 说明                        |
 * |------------|------|----------------------------|
 * | progress   | f32  | 当前等级进度 (0.0 - 1.0)    |
 * | totalXp    | i32  | 累计总经验值                |
 * | level      | i32  | 当前等级                    |
 */
class SetExperiencePacket : public Packet {
public:
    SetExperiencePacket()
        : Packet(PacketType::SetExperience)
    {}

    /**
     * @brief 从玩家对象构造
     * @param player 玩家对象
     */
    static SetExperiencePacket fromPlayer(const Player& player);

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override { return sizeof(PacketHeader) + 12; }

    // ========== Getters ==========

    /**
     * @brief 获取当前等级进度 (0.0 - 1.0)
     */
    [[nodiscard]] f32 progress() const { return m_progress; }

    /**
     * @brief 获取累计总经验值
     */
    [[nodiscard]] i32 totalXp() const { return m_totalXp; }

    /**
     * @brief 获取当前等级
     */
    [[nodiscard]] i32 level() const { return m_level; }

    // ========== Setters ==========

    /**
     * @brief 设置经验状态
     * @param progress 当前等级进度 (0.0 - 1.0)
     * @param totalXp 累计总经验值
     * @param level 当前等级
     */
    void setExperience(f32 progress, i32 totalXp, i32 level)
    {
        m_progress = progress;
        m_totalXp = totalXp;
        m_level = level;
    }

    /**
     * @brief 设置当前等级进度
     */
    void setProgress(f32 progress) { m_progress = progress; }

    /**
     * @brief 设置累计总经验值
     */
    void setTotalXp(i32 totalXp) { m_totalXp = totalXp; }

    /**
     * @brief 设置当前等级
     */
    void setLevel(i32 level) { m_level = level; }

private:
    f32 m_progress = 0.0f; // 当前等级进度 (0.0 - 1.0)
    i32 m_totalXp = 0;     // 累计总经验值
    i32 m_level = 0;       // 当前等级
};

/**
 * @brief 经验球生成包 (S->C)
 *
 * 服务端通知客户端在世界中生成经验球实体。
 *
 * 参考 MC 1.16.5 SSpawnExperienceOrbPacket
 *
 * 协议格式:
 * | 字段       | 类型 | 说明                        |
 * |------------|------|----------------------------|
 * | entityId   | i32  | 实体ID                      |
 * | x          | f64  | X坐标                       |
 * | y          | f64  | Y坐标                       |
 * | z          | f64  | Z坐标                       |
 * | xpValue    | i16  | 经验值                      |
 */
class SpawnExperienceOrbPacket : public Packet {
public:
    SpawnExperienceOrbPacket()
        : Packet(PacketType::SpawnExperienceOrb)
    {}

    /**
     * @brief 构造经验球生成包
     * @param entityId 实体ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param xpValue 经验值
     */
    SpawnExperienceOrbPacket(i32 entityId, f64 x, f64 y, f64 z, i16 xpValue)
        : Packet(PacketType::SpawnExperienceOrb)
        , m_entityId(entityId)
        , m_x(x)
        , m_y(y)
        , m_z(z)
        , m_xpValue(xpValue)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override { return sizeof(PacketHeader) + 30; }

    // ========== Getters ==========

    /**
     * @brief 获取实体ID
     */
    [[nodiscard]] i32 entityId() const { return m_entityId; }

    /**
     * @brief 获取X坐标
     */
    [[nodiscard]] f64 x() const { return m_x; }

    /**
     * @brief 获取Y坐标
     */
    [[nodiscard]] f64 y() const { return m_y; }

    /**
     * @brief 获取Z坐标
     */
    [[nodiscard]] f64 z() const { return m_z; }

    /**
     * @brief 获取经验值
     */
    [[nodiscard]] i16 xpValue() const { return m_xpValue; }

    // ========== Setters ==========

    /**
     * @brief 设置实体ID
     */
    void setEntityId(i32 id) { m_entityId = id; }

    /**
     * @brief 设置位置
     */
    void setPosition(f64 x, f64 y, f64 z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    /**
     * @brief 设置经验值
     */
    void setXpValue(i16 value) { m_xpValue = value; }

private:
    i32 m_entityId = 0; // 实体ID
    f64 m_x = 0.0;      // X坐标
    f64 m_y = 0.0;      // Y坐标
    f64 m_z = 0.0;      // Z坐标
    i16 m_xpValue = 1;  // 经验值 (MC限制: 1-2477)
};

} // namespace network
} // namespace mc
