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

#include "WorldBorder.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <utility>

namespace mc {
namespace world {
namespace border {

// ============================================================================
// 静止边界状态
// ============================================================================

/**
 * @brief 静止边界状态
 *
 * 边界大小固定，不进行过渡动画。
 */
class StationaryBorderState : public IBorderState {
public:
    explicit StationaryBorderState(double size, double centerX, double centerZ);

    [[nodiscard]] double getMinX() const override { return m_minX; }
    [[nodiscard]] double getMaxX() const override { return m_maxX; }
    [[nodiscard]] double getMinZ() const override { return m_minZ; }
    [[nodiscard]] double getMaxZ() const override { return m_maxZ; }
    [[nodiscard]] double getSize() const override { return m_size; }
    [[nodiscard]] double getResizeSpeed() const override { return 0.0; }
    [[nodiscard]] u64 getTimeUntilTarget() const override { return 0; }
    [[nodiscard]] double getTargetSize() const override { return m_size; }
    [[nodiscard]] BorderStatus getStatus() const override { return BorderStatus::Stationary; }
    [[nodiscard]] std::unique_ptr<IBorderState> tick() override { return nullptr; }
    void onCenterChanged(double centerX, double centerZ) override;

private:
    void _updateBounds();

    double m_size;
    double m_centerX;
    double m_centerZ;
    double m_minX, m_maxX, m_minZ, m_maxZ;
};

// ============================================================================
// 移动边界状态
// ============================================================================

/**
 * @brief 移动边界状态
 *
 * 边界大小从起始值线性插值到目标值。
 */
class MovingBorderState : public IBorderState {
public:
    MovingBorderState(double oldSize, double newSize, u64 timeMs, double centerX, double centerZ);

    [[nodiscard]] double getMinX() const override;
    [[nodiscard]] double getMaxX() const override;
    [[nodiscard]] double getMinZ() const override;
    [[nodiscard]] double getMaxZ() const override;
    [[nodiscard]] double getSize() const override;
    [[nodiscard]] double getResizeSpeed() const override;
    [[nodiscard]] u64 getTimeUntilTarget() const override;
    [[nodiscard]] double getTargetSize() const override { return m_newSize; }
    [[nodiscard]] BorderStatus getStatus() const override;
    [[nodiscard]] std::unique_ptr<IBorderState> tick() override;
    void onCenterChanged(double centerX, double centerZ) override;

private:
    void _updateBounds() const;

    double m_oldSize;     // 起始大小
    double m_newSize;     // 目标大小
    u64 m_startTime;      // 开始时间（毫秒）
    u64 m_endTime;        // 结束时间（毫秒）
    u64 m_transitionTime; // 过渡总时长（毫秒）
    double m_centerX;
    double m_centerZ;
    mutable double m_cachedMinX, m_cachedMaxX, m_cachedMinZ, m_cachedMaxZ;
    mutable double m_cachedSize;
    mutable bool m_dirty = true;
};

// ============================================================================
// 工具函数
// ============================================================================

namespace {

/**
 * @brief 获取当前时间戳（毫秒）
 */
u64 getCurrentTimeMs()
{
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return static_cast<u64>(ms.count());
}

} // anonymous namespace

// ============================================================================
// StationaryBorderState 实现
// ============================================================================

StationaryBorderState::StationaryBorderState(double size, double centerX, double centerZ)
    : m_size(size)
    , m_centerX(centerX)
    , m_centerZ(centerZ)
{
    _updateBounds();
}

void StationaryBorderState::_updateBounds()
{
    m_minX = m_centerX - m_size / 2.0;
    m_maxX = m_centerX + m_size / 2.0;
    m_minZ = m_centerZ - m_size / 2.0;
    m_maxZ = m_centerZ + m_size / 2.0;
}

void StationaryBorderState::onCenterChanged(double centerX, double centerZ)
{
    m_centerX = centerX;
    m_centerZ = centerZ;
    _updateBounds();
}

// ============================================================================
// MovingBorderState 实现
// ============================================================================

MovingBorderState::MovingBorderState(double oldSize, double newSize, u64 timeMs, double centerX, double centerZ)
    : m_oldSize(oldSize)
    , m_newSize(newSize)
    , m_centerX(centerX)
    , m_centerZ(centerZ)
{
    u64 now = getCurrentTimeMs();
    m_startTime = now;
    m_endTime = now + timeMs;
    m_transitionTime = timeMs;
    _updateBounds();
}

double MovingBorderState::getMinX() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedMinX;
}

double MovingBorderState::getMaxX() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedMaxX;
}

double MovingBorderState::getMinZ() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedMinZ;
}

double MovingBorderState::getMaxZ() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedMaxZ;
}

double MovingBorderState::getSize() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedSize;
}

double MovingBorderState::getResizeSpeed() const
{
    if (m_transitionTime == 0) {
        return 0.0;
    }
    return std::abs(m_newSize - m_oldSize) / static_cast<double>(m_transitionTime);
}

u64 MovingBorderState::getTimeUntilTarget() const
{
    u64 now = getCurrentTimeMs();
    if (now >= m_endTime) {
        return 0;
    }
    return m_endTime - now;
}

BorderStatus MovingBorderState::getStatus() const
{
    return (m_newSize > m_oldSize) ? BorderStatus::Growing : BorderStatus::Shrinking;
}

std::unique_ptr<IBorderState> MovingBorderState::tick()
{
    if (getTimeUntilTarget() == 0) {
        // 过渡完成，返回静止状态
        return std::make_unique<StationaryBorderState>(m_newSize, m_centerX, m_centerZ);
    }
    // 标记需要重新计算
    m_dirty = true;
    return nullptr;
}

void MovingBorderState::onCenterChanged(double centerX, double centerZ)
{
    m_centerX = centerX;
    m_centerZ = centerZ;
    m_dirty = true;
}

void MovingBorderState::_updateBounds() const
{
    // 计算当前大小（线性插值）
    u64 now = getCurrentTimeMs();
    double progress = 0.0;
    if (m_transitionTime > 0) {
        progress = static_cast<double>(now - m_startTime) / static_cast<double>(m_transitionTime);
        progress = std::clamp(progress, 0.0, 1.0);
    }
    m_cachedSize = m_oldSize + (m_newSize - m_oldSize) * progress;

    // 计算边界
    m_cachedMinX = m_centerX - m_cachedSize / 2.0;
    m_cachedMaxX = m_centerX + m_cachedSize / 2.0;
    m_cachedMinZ = m_centerZ - m_cachedSize / 2.0;
    m_cachedMaxZ = m_centerZ + m_cachedSize / 2.0;

    m_dirty = false;
}

// ============================================================================
// WorldBorder 实现
// ============================================================================

WorldBorder::WorldBorder()
    : m_state(std::make_unique<StationaryBorderState>(6.0E7, 0.0, 0.0))
{}

WorldBorder::~WorldBorder() = default;

WorldBorder::WorldBorder(WorldBorder&&) noexcept = default;

WorldBorder& WorldBorder::operator=(WorldBorder&&) noexcept = default;

// ============================================================================
// 边界状态查询
// ============================================================================

double WorldBorder::getSize() const
{
    return m_state->getSize();
}

double WorldBorder::getTargetSize() const
{
    return m_state->getTargetSize();
}

double WorldBorder::getMinX() const
{
    return m_state->getMinX();
}

double WorldBorder::getMaxX() const
{
    return m_state->getMaxX();
}

double WorldBorder::getMinZ() const
{
    return m_state->getMinZ();
}

double WorldBorder::getMaxZ() const
{
    return m_state->getMaxZ();
}

BorderStatus WorldBorder::getStatus() const
{
    return m_state->getStatus();
}

double WorldBorder::getResizeSpeed() const
{
    return m_state->getResizeSpeed();
}

u64 WorldBorder::getTimeUntilTarget() const
{
    return m_state->getTimeUntilTarget();
}

// ============================================================================
// 伤害参数
// ============================================================================

void WorldBorder::setDamagePerBlock(double damagePerBlock)
{
    m_damagePerBlock = damagePerBlock;
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onDamagePerBlockChanged(damagePerBlock);
        }
    }
}

void WorldBorder::setDamageBuffer(double damageBuffer)
{
    m_damageBuffer = damageBuffer;
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onDamageBufferChanged(damageBuffer);
        }
    }
}

// ============================================================================
// 警告参数
// ============================================================================

void WorldBorder::setWarningTime(i32 warningTime)
{
    m_warningTime = warningTime;
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onWarningTimeChanged(warningTime);
        }
    }
}

void WorldBorder::setWarningDistance(i32 warningDistance)
{
    m_warningDistance = warningDistance;
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onWarningDistanceChanged(warningDistance);
        }
    }
}

// ============================================================================
// 边界设置
// ============================================================================

void WorldBorder::setSize(double size)
{
    size = std::clamp(size, 1.0, MAX_SIZE);
    m_state = std::make_unique<StationaryBorderState>(size, m_centerX, m_centerZ);
    _notifySizeChanged(size);
}

void WorldBorder::setSizeLerp(double oldSize, double newSize, u64 timeMs)
{
    oldSize = std::clamp(oldSize, 1.0, MAX_SIZE);
    newSize = std::clamp(newSize, 1.0, MAX_SIZE);

    if (oldSize == newSize || timeMs == 0) {
        setSize(newSize);
        return;
    }

    m_state = std::make_unique<MovingBorderState>(oldSize, newSize, timeMs, m_centerX, m_centerZ);
    _notifyTransitionStarted(oldSize, newSize, timeMs);
}

void WorldBorder::setCenter(double x, double z)
{
    m_centerX = x;
    m_centerZ = z;
    m_state->onCenterChanged(x, z);
    _notifyCenterChanged(x, z);
}

// ============================================================================
// 边界检测
// ============================================================================

bool WorldBorder::contains(double x, double z) const
{
    return x > getMinX() && x < getMaxX() && z > getMinZ() && z < getMaxZ();
}

bool WorldBorder::contains(const BlockPos& pos) const
{
    // 方块位置检测：方块必须在边界内（方块边界需要完全在内）
    return (static_cast<double>(pos.x) + 1.0) > getMinX() && static_cast<double>(pos.x) < getMaxX() &&
        (static_cast<double>(pos.z) + 1.0) > getMinZ() && static_cast<double>(pos.z) < getMaxZ();
}

bool WorldBorder::intersects(const AxisAlignedBB& box) const
{
    return box.maxX > getMinX() && box.minX < getMaxX() && box.maxZ > getMinZ() && box.minZ < getMaxZ();
}

bool WorldBorder::intersectsChunk(i32 chunkX, i32 chunkZ) const
{
    constexpr double CHUNK_SIZE = static_cast<double>(world::CHUNK_WIDTH);
    double chunkMinX = static_cast<double>(chunkX) * CHUNK_SIZE;
    double chunkMinZ = static_cast<double>(chunkZ) * CHUNK_SIZE;
    double chunkMaxX = chunkMinX + CHUNK_SIZE;
    double chunkMaxZ = chunkMinZ + CHUNK_SIZE;

    return chunkMaxX > getMinX() && chunkMinX < getMaxX() && chunkMaxZ > getMinZ() && chunkMinZ < getMaxZ();
}

double WorldBorder::getClosestDistance(double x, double z) const
{
    double distToMinX = x - getMinX(); // 到西边界的距离
    double distToMaxX = getMaxX() - x; // 到东边界的距离
    double distToMinZ = z - getMinZ(); // 到北边界的距离
    double distToMaxZ = getMaxZ() - z; // 到南边界的距离

    // 返回最小距离（如果点在边界内则为正，否则为负）
    return std::min({distToMinX, distToMaxX, distToMinZ, distToMaxZ});
}

double WorldBorder::getClosestDistance(const AxisAlignedBB& box) const
{
    // 计算 AABB 中心到边界的距离
    double centerX = (box.minX + box.maxX) / 2.0;
    double centerZ = (box.minZ + box.maxZ) / 2.0;

    // 使用 AABB 的最近边计算距离
    double distToMinX = box.minX - getMinX(); // 负值表示超出边界
    double distToMaxX = getMaxX() - box.maxX;
    double distToMinZ = box.minZ - getMinZ();
    double distToMaxZ = getMaxZ() - box.maxZ;

    // 返回最小距离
    return std::min({distToMinX, distToMaxX, distToMinZ, distToMaxZ});
}

// ============================================================================
// 更新与监听
// ============================================================================

void WorldBorder::tick()
{
    if (auto newState = m_state->tick()) {
        m_state = std::move(newState);
    }
}

void WorldBorder::addListener(std::shared_ptr<IBorderListener> listener)
{
    m_listeners.push_back(listener);
    // 清理过期的监听器
    m_listeners.erase(std::remove_if(m_listeners.begin(),
                          m_listeners.end(),
                          [](const std::weak_ptr<IBorderListener>& weak) { return weak.expired(); }),
        m_listeners.end());
}

void WorldBorder::removeListener(std::shared_ptr<IBorderListener> listener)
{
    m_listeners.erase(std::remove_if(m_listeners.begin(),
                          m_listeners.end(),
                          [&listener](const std::weak_ptr<IBorderListener>& weak) {
                              auto locked = weak.lock();
                              return !locked || locked == listener;
                          }),
        m_listeners.end());
}

// ============================================================================
// 序列化
// ============================================================================

WorldBorder::SerializedData WorldBorder::serialize() const
{
    SerializedData data;
    data.centerX = m_centerX;
    data.centerZ = m_centerZ;
    data.size = m_state->getSize();
    data.targetSize = m_state->getTargetSize();
    data.timeUntilTarget = m_state->getTimeUntilTarget();
    data.damagePerBlock = m_damagePerBlock;
    data.damageBuffer = m_damageBuffer;
    data.warningTime = m_warningTime;
    data.warningDistance = m_warningDistance;
    return data;
}

void WorldBorder::deserialize(const SerializedData& data)
{
    m_centerX = data.centerX;
    m_centerZ = data.centerZ;
    m_damagePerBlock = data.damagePerBlock;
    m_damageBuffer = data.damageBuffer;
    m_warningTime = data.warningTime;
    m_warningDistance = data.warningDistance;

    if (data.timeUntilTarget > 0 && data.size != data.targetSize) {
        // 恢复过渡状态
        setSizeLerp(data.size, data.targetSize, data.timeUntilTarget);
    } else {
        setSize(data.targetSize);
    }
}

// ============================================================================
// 私有方法
// ============================================================================

void WorldBorder::_notifySizeChanged(double newSize)
{
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onSizeChanged(newSize);
        }
    }
}

void WorldBorder::_notifyTransitionStarted(double oldSize, double newSize, u64 timeMs)
{
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onTransitionStarted(oldSize, newSize, timeMs);
        }
    }
}

void WorldBorder::_notifyCenterChanged(double x, double z)
{
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onCenterChanged(x, z);
        }
    }
}

} // namespace border
} // namespace world
} // namespace mc
