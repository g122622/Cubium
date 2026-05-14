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
#include "../../util/AxisAlignedBB.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {

class BlockPos;
class ChunkPos;
class AxisAlignedBB;

namespace world {
namespace border {

/**
 * @brief 边界状态枚举（用于客户端渲染）
 *
 * 参考: net.minecraft.world.border.BorderStatus
 */
enum class BorderStatus : u8 {
    Stationary = 0, // 静止 - 灰色
    Shrinking = 1,  // 收缩中 - 红色
    Growing = 2     // 扩大中 - 绿色
};

/**
 * @brief 边界变化监听器接口
 *
 * 参考: net.minecraft.world.border.IBorderListener
 */
class IBorderListener {
public:
    virtual ~IBorderListener() = default;

    /**
     * @brief 边界大小变化时调用
     * @param newSize 新的边界大小
     */
    virtual void onSizeChanged(double newSize) = 0;

    /**
     * @brief 边界过渡开始时调用
     * @param oldSize 起始大小
     * @param newSize 目标大小
     * @param timeMs 过渡时间（毫秒）
     */
    virtual void onTransitionStarted(double oldSize, double newSize, u64 timeMs) = 0;

    /**
     * @brief 边界中心变化时调用
     * @param x 新中心 X 坐标
     * @param z 新中心 Z 坐标
     */
    virtual void onCenterChanged(double x, double z) = 0;

    /**
     * @brief 警告时间变化时调用
     * @param warningTime 新警告时间（秒）
     */
    virtual void onWarningTimeChanged(i32 warningTime) = 0;

    /**
     * @brief 警告距离变化时调用
     * @param warningDistance 新警告距离（格）
     */
    virtual void onWarningDistanceChanged(i32 warningDistance) = 0;

    /**
     * @brief 伤害缓冲变化时调用
     * @param damageBuffer 新伤害缓冲距离
     */
    virtual void onDamageBufferChanged(double damageBuffer) = 0;

    /**
     * @brief 每格伤害变化时调用
     * @param damagePerBlock 新每格伤害量
     */
    virtual void onDamagePerBlockChanged(double damagePerBlock) = 0;
};

/**
 * @brief 边界状态接口（状态模式）
 *
 * 参考: net.minecraft.world.border.WorldBorder.IBorderInfo
 *
 * 使用状态模式支持边界大小的渐变动画。
 * - StationaryBorderState: 静止边界，固定大小
 * - MovingBorderState: 移动边界，线性插值过渡
 */
class IBorderState {
public:
    virtual ~IBorderState() = default;

    /**
     * @brief 获取当前边界最小 X 坐标
     */
    [[nodiscard]] virtual double getMinX() const = 0;

    /**
     * @brief 获取当前边界最大 X 坐标
     */
    [[nodiscard]] virtual double getMaxX() const = 0;

    /**
     * @brief 获取当前边界最小 Z 坐标
     */
    [[nodiscard]] virtual double getMinZ() const = 0;

    /**
     * @brief 获取当前边界最大 Z 坐标
     */
    [[nodiscard]] virtual double getMaxZ() const = 0;

    /**
     * @brief 获取当前边界大小（直径）
     */
    [[nodiscard]] virtual double getSize() const = 0;

    /**
     * @brief 获取边界变化速度（格/毫秒）
     *
     * 仅在 MovingBorderState 中有效，静止边界返回 0。
     */
    [[nodiscard]] virtual double getResizeSpeed() const = 0;

    /**
     * @brief 获取到达目标大小的剩余时间（毫秒）
     *
     * 静止边界返回 0，移动边界返回剩余毫秒数。
     */
    [[nodiscard]] virtual u64 getTimeUntilTarget() const = 0;

    /**
     * @brief 获取目标边界大小
     *
     * 静止边界返回当前大小，移动边界返回目标大小。
     */
    [[nodiscard]] virtual double getTargetSize() const = 0;

    /**
     * @brief 获取边界状态
     */
    [[nodiscard]] virtual BorderStatus getStatus() const = 0;

    /**
     * @brief 更新边界状态（每 tick 调用）
     *
     * @return 如果过渡完成，返回新的静止状态；否则返回 this
     */
    [[nodiscard]] virtual std::unique_ptr<IBorderState> tick() = 0;

    /**
     * @brief 边界中心变化时调用
     */
    virtual void onCenterChanged(double centerX, double centerZ) = 0;
};

/**
 * @brief 世界边界类
 *
 * 管理 Minecraft 世界边界，支持：
 * - 边界大小设置（立即或渐变）
 * - 边界中心设置
 * - 伤害参数配置
 * - 警告参数配置
 * - 玩家越界检测
 *
 * 参考: net.minecraft.world.border.WorldBorder
 */
class WorldBorder {
public:
    /**
     * @brief 世界最大边界大小（3000万格）
     */
    static constexpr double MAX_SIZE = 2.9999872E7;

    /**
     * @brief 默认构造函数
     *
     * 初始化边界为中心 (0, 0)，大小 6000 万格。
     */
    WorldBorder();

    /**
     * @brief 析构函数
     */
    ~WorldBorder();

    // 禁止拷贝
    WorldBorder(const WorldBorder&) = delete;
    WorldBorder& operator=(const WorldBorder&) = delete;

    // 允许移动
    WorldBorder(WorldBorder&&) noexcept;
    WorldBorder& operator=(WorldBorder&&) noexcept;

    // ========================================================================
    // 边界状态查询
    // ========================================================================

    /**
     * @brief 获取当前边界大小（直径）
     */
    [[nodiscard]] double getSize() const;

    /**
     * @brief 获取目标边界大小
     */
    [[nodiscard]] double getTargetSize() const;

    /**
     * @brief 获取边界中心 X 坐标
     */
    [[nodiscard]] double getCenterX() const { return m_centerX; }

    /**
     * @brief 获取边界中心 Z 坐标
     */
    [[nodiscard]] double getCenterZ() const { return m_centerZ; }

    /**
     * @brief 获取边界最小 X 坐标
     */
    [[nodiscard]] double getMinX() const;

    /**
     * @brief 获取边界最大 X 坐标
     */
    [[nodiscard]] double getMaxX() const;

    /**
     * @brief 获取边界最小 Z 坐标
     */
    [[nodiscard]] double getMinZ() const;

    /**
     * @brief 获取边界最大 Z 坐标
     */
    [[nodiscard]] double getMaxZ() const;

    /**
     * @brief 获取边界状态
     */
    [[nodiscard]] BorderStatus getStatus() const;

    /**
     * @brief 获取边界变化速度（格/毫秒）
     */
    [[nodiscard]] double getResizeSpeed() const;

    /**
     * @brief 获取到达目标大小的剩余时间（毫秒）
     */
    [[nodiscard]] u64 getTimeUntilTarget() const;

    // ========================================================================
    // 伤害参数
    // ========================================================================

    /**
     * @brief 获取每格伤害量
     *
     * 默认值: 0.2
     */
    [[nodiscard]] double getDamagePerBlock() const { return m_damagePerBlock; }

    /**
     * @brief 获取伤害缓冲距离
     *
     * 默认值: 5.0
     */
    [[nodiscard]] double getDamageBuffer() const { return m_damageBuffer; }

    /**
     * @brief 设置每格伤害量
     */
    void setDamagePerBlock(double damagePerBlock);

    /**
     * @brief 设置伤害缓冲距离
     */
    void setDamageBuffer(double damageBuffer);

    // ========================================================================
    // 警告参数
    // ========================================================================

    /**
     * @brief 获取警告时间（秒）
     *
     * 默认值: 15 秒
     */
    [[nodiscard]] i32 getWarningTime() const { return m_warningTime; }

    /**
     * @brief 获取警告距离（格）
     *
     * 默认值: 5 格
     */
    [[nodiscard]] i32 getWarningDistance() const { return m_warningDistance; }

    /**
     * @brief 设置警告时间
     */
    void setWarningTime(i32 warningTime);

    /**
     * @brief 设置警告距离
     */
    void setWarningDistance(i32 warningDistance);

    // ========================================================================
    // 边界设置
    // ========================================================================

    /**
     * @brief 立即设置边界大小
     * @param size 新的边界大小
     */
    void setSize(double size);

    /**
     * @brief 渐变设置边界大小
     * @param oldSize 起始大小（通常使用当前大小）
     * @param newSize 目标大小
     * @param timeMs 过渡时间（毫秒）
     */
    void setSizeLerp(double oldSize, double newSize, u64 timeMs);

    /**
     * @brief 设置边界中心
     * @param x 中心 X 坐标
     * @param z 中心 Z 坐标
     */
    void setCenter(double x, double z);

    // ========================================================================
    // 边界检测
    // ========================================================================

    /**
     * @brief 检测坐标点是否在边界内
     * @param x X 坐标
     * @param z Z 坐标
     * @return 如果点在边界内返回 true
     */
    [[nodiscard]] bool contains(double x, double z) const;

    /**
     * @brief 检测方块位置是否在边界内
     * @param pos 方块位置
     * @return 如果方块在边界内返回 true
     */
    [[nodiscard]] bool contains(const BlockPos& pos) const;

    /**
     * @brief 检测 AABB 是否与边界相交
     * @param box 轴对齐包围盒
     * @return 如果 AABB 与边界相交返回 true
     */
    [[nodiscard]] bool intersects(const AxisAlignedBB& box) const;

    /**
     * @brief 检测区块是否与边界相交
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 如果区块与边界相交返回 true
     */
    [[nodiscard]] bool intersectsChunk(i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 获取点到边界的最近距离
     *
     * 如果点在边界内返回正数（到边界的距离），
     * 如果点在边界外返回负数（超出边界的距离）。
     *
     * @param x X 坐标
     * @param z Z 坐标
     * @return 到边界的最近距离
     */
    [[nodiscard]] double getClosestDistance(double x, double z) const;

    /**
     * @brief 获取 AABB 到边界的最近距离
     * @param box 轴对齐包围盒
     * @return 到边界的最近距离
     */
    [[nodiscard]] double getClosestDistance(const AxisAlignedBB& box) const;

    // ========================================================================
    // 更新与监听
    // ========================================================================

    /**
     * @brief 更新边界状态（每 tick 调用）
     *
     * 如果边界正在过渡，更新当前大小。
     */
    void tick();

    /**
     * @brief 添加边界变化监听器
     * @param listener 监听器
     */
    void addListener(std::shared_ptr<IBorderListener> listener);

    /**
     * @brief 移除边界变化监听器
     * @param listener 监听器
     */
    void removeListener(std::shared_ptr<IBorderListener> listener);

    // ========================================================================
    // 序列化
    // ========================================================================

    /**
     * @brief 边界设置序列化数据
     */
    struct SerializedData {
        double centerX = 0.0;
        double centerZ = 0.0;
        double size = 6.0E7;
        double targetSize = 6.0E7;
        u64 timeUntilTarget = 0; // 过渡剩余时间（毫秒）
        double damagePerBlock = 0.2;
        double damageBuffer = 5.0;
        i32 warningTime = 15;
        i32 warningDistance = 5;
    };

    /**
     * @brief 序列化边界设置
     */
    [[nodiscard]] SerializedData serialize() const;

    /**
     * @brief 从序列化数据恢复边界设置
     * @param data 序列化数据
     */
    void deserialize(const SerializedData& data);

private:
    /**
     * @brief 通知所有监听器边界大小变化
     */
    void notifySizeChanged(double newSize);

    /**
     * @brief 通知所有监听器过渡开始
     */
    void notifyTransitionStarted(double oldSize, double newSize, u64 timeMs);

    /**
     * @brief 通知所有监听器中心变化
     */
    void notifyCenterChanged(double x, double z);

    /**
     * @brief 更新边界状态缓存
     */
    void updateCachedBounds();

private:
    double m_centerX = 0.0;        // 边界中心 X
    double m_centerZ = 0.0;        // 边界中心 Z
    double m_damagePerBlock = 0.2; // 每格伤害量
    double m_damageBuffer = 5.0;   // 伤害缓冲距离
    i32 m_warningTime = 15;        // 警告时间（秒）
    i32 m_warningDistance = 5;     // 警告距离（格）

    std::unique_ptr<IBorderState> m_state;                   // 边界状态（静止/移动）
    std::vector<std::weak_ptr<IBorderListener>> m_listeners; // 监听器列表
};

} // namespace border
} // namespace world
} // namespace mc
