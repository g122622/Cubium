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
#include "../../world/border/WorldBorder.hpp"
#include "Packet.hpp"

namespace mc::network {

/**
 * @brief 世界边界包动作类型
 */
enum class WorldBorderAction : u8 {
    SetSize = 0,            // 设置大小（立即）
    LerpSize = 1,           // 渐变大小
    SetCenter = 2,          // 设置中心
    Initialize = 3,         // 完整初始化
    SetWarningTime = 4,     // 设置警告时间
    SetWarningDistance = 5, // 设置警告距离
    SetDamageBuffer = 6,    // 设置伤害缓冲
    SetDamagePerBlock = 7   // 设置每格伤害
};

/**
 * @brief 世界边界同步包
 *
 * 用于服务端向客户端同步世界边界状态，支持：
 * - 设置边界大小（立即或渐变）
 * - 设置边界中心
 * - 设置伤害参数
 * - 设置警告参数
 * - 完整初始化
 */
class WorldBorderPacket : public Packet {
public:
    WorldBorderPacket();
    ~WorldBorderPacket() override = default;

    // 移动语义
    WorldBorderPacket(WorldBorderPacket&& other) noexcept = default;
    WorldBorderPacket& operator=(WorldBorderPacket&& other) noexcept = default;

    // 禁止拷贝（Packet基类通常不可拷贝）
    WorldBorderPacket(const WorldBorderPacket&) = delete;
    WorldBorderPacket& operator=(const WorldBorderPacket&) = delete;

    // ========================================================================
    // 静态工厂方法
    // ========================================================================

    /**
     * @brief 创建设置大小包
     * @param size 新的边界大小
     */
    static WorldBorderPacket setSize(f64 size);

    /**
     * @brief 创建渐变大小包
     * @param oldSize 起始大小
     * @param newSize 目标大小
     * @param timeMs 过渡时间（毫秒）
     */
    static WorldBorderPacket lerpSize(f64 oldSize, f64 newSize, u64 timeMs);

    /**
     * @brief 创建设置中心包
     * @param x 中心 X 坐标
     * @param z 中心 Z 坐标
     */
    static WorldBorderPacket setCenter(f64 x, f64 z);

    /**
     * @brief 创建完整初始化包
     * @param border 世界边界对象
     */
    static WorldBorderPacket initialize(const world::border::WorldBorder& border);

    /**
     * @brief 创建设置警告时间包
     * @param warningTime 警告时间（秒）
     */
    static WorldBorderPacket setWarningTime(i32 warningTime);

    /**
     * @brief 创建设置警告距离包
     * @param warningDistance 警告距离（格）
     */
    static WorldBorderPacket setWarningDistance(i32 warningDistance);

    /**
     * @brief 创建设置伤害缓冲包
     * @param damageBuffer 伤害缓冲距离
     */
    static WorldBorderPacket setDamageBuffer(f64 damageBuffer);

    /**
     * @brief 创建设置每格伤害包
     * @param damagePerBlock 每格伤害量
     */
    static WorldBorderPacket setDamagePerBlock(f64 damagePerBlock);

    // ========================================================================
    // 序列化
    // ========================================================================

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========================================================================
    // 访问器
    // ========================================================================

    WorldBorderAction action() const { return m_action; }

    // 大小相关
    f64 size() const { return m_size; }
    f64 oldSize() const { return m_oldSize; }
    f64 newSize() const { return m_newSize; }
    u64 timeMs() const { return m_timeMs; }

    // 中心相关
    f64 centerX() const { return m_centerX; }
    f64 centerZ() const { return m_centerZ; }

    // 伤害相关
    f64 damagePerBlock() const { return m_damagePerBlock; }
    f64 damageBuffer() const { return m_damageBuffer; }

    // 警告相关
    i32 warningTime() const { return m_warningTime; }
    i32 warningDistance() const { return m_warningDistance; }

private:
    explicit WorldBorderPacket(WorldBorderAction action);

    WorldBorderAction m_action = WorldBorderAction::SetSize;

    // 大小
    f64 m_size = 0.0;
    f64 m_oldSize = 0.0;
    f64 m_newSize = 0.0;
    u64 m_timeMs = 0;

    // 中心
    f64 m_centerX = 0.0;
    f64 m_centerZ = 0.0;

    // 伤害
    f64 m_damagePerBlock = 0.2;
    f64 m_damageBuffer = 5.0;

    // 警告
    i32 m_warningTime = 15;
    i32 m_warningDistance = 5;
};

} // namespace mc::network
