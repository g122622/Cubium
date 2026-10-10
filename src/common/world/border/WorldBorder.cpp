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
    explicit StationaryBorderState(f64 size, f64 centerX, f64 centerZ);

    [[nodiscard]] f64 getMinX() const override { return m_minX; }
    [[nodiscard]] f64 getMaxX() const override { return m_maxX; }
    [[nodiscard]] f64 getMinZ() const override { return m_minZ; }
    [[nodiscard]] f64 getMaxZ() const override { return m_maxZ; }
    [[nodiscard]] f64 getSize() const override { return m_size; }
    [[nodiscard]] f64 getResizeSpeed() const override { return 0.0; }
    [[nodiscard]] u64 getTimeUntilTarget() const override { return 0; }
    [[nodiscard]] f64 getTargetSize() const override { return m_size; }
    [[nodiscard]] BorderStatus getStatus() const override { return BorderStatus::Stationary; }
    [[nodiscard]] std::unique_ptr<IBorderState> tick() override { return nullptr; }
    void onCenterChanged(f64 centerX, f64 centerZ) override;

private:
    void _updateBounds();

    f64 m_size;
    f64 m_centerX;
    f64 m_centerZ;
    f64 m_minX, m_maxX, m_minZ, m_maxZ;
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
    MovingBorderState(f64 oldSize, f64 newSize, u64 timeMs, f64 centerX, f64 centerZ);

    [[nodiscard]] f64 getMinX() const override;
    [[nodiscard]] f64 getMaxX() const override;
    [[nodiscard]] f64 getMinZ() const override;
    [[nodiscard]] f64 getMaxZ() const override;
    [[nodiscard]] f64 getSize() const override;
    [[nodiscard]] f64 getResizeSpeed() const override;
    [[nodiscard]] u64 getTimeUntilTarget() const override;
    [[nodiscard]] f64 getTargetSize() const override { return m_newSize; }
    [[nodiscard]] BorderStatus getStatus() const override;
    [[nodiscard]] std::unique_ptr<IBorderState> tick() override;
    void onCenterChanged(f64 centerX, f64 centerZ) override;

private:
    void _updateBounds() const;

    f64 m_oldSize;     // 起始大小
    f64 m_newSize;     // 目标大小
    u64 m_startTime;      // 开始时间（毫秒）
    u64 m_endTime;        // 结束时间（毫秒）
    u64 m_transitionTime; // 过渡总时长（毫秒）
    f64 m_centerX;
    f64 m_centerZ;
    mutable f64 m_cachedMinX, m_cachedMaxX, m_cachedMinZ, m_cachedMaxZ;
    mutable f64 m_cachedSize;
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

StationaryBorderState::StationaryBorderState(f64 size, f64 centerX, f64 centerZ)
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

void StationaryBorderState::onCenterChanged(f64 centerX, f64 centerZ)
{
    m_centerX = centerX;
    m_centerZ = centerZ;
    _updateBounds();
}

// ============================================================================
// MovingBorderState 实现
// ============================================================================

MovingBorderState::MovingBorderState(f64 oldSize, f64 newSize, u64 timeMs, f64 centerX, f64 centerZ)
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

f64 MovingBorderState::getMinX() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedMinX;
}

f64 MovingBorderState::getMaxX() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedMaxX;
}

f64 MovingBorderState::getMinZ() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedMinZ;
}

f64 MovingBorderState::getMaxZ() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedMaxZ;
}

f64 MovingBorderState::getSize() const
{
    if (m_dirty) {
        _updateBounds();
    }
    return m_cachedSize;
}

f64 MovingBorderState::getResizeSpeed() const
{
    if (m_transitionTime == 0) {
        return 0.0;
    }
    return std::abs(m_newSize - m_oldSize) / static_cast<f64>(m_transitionTime);
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

void MovingBorderState::onCenterChanged(f64 centerX, f64 centerZ)
{
    m_centerX = centerX;
    m_centerZ = centerZ;
    m_dirty = true;
}

void MovingBorderState::_updateBounds() const
{
    // 计算当前大小（线性插值）
    u64 now = getCurrentTimeMs();
    f64 progress = 0.0;
    if (m_transitionTime > 0) {
        progress = static_cast<f64>(now - m_startTime) / static_cast<f64>(m_transitionTime);
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

f64 WorldBorder::getSize() const
{
    return m_state->getSize();
}

f64 WorldBorder::getTargetSize() const
{
    return m_state->getTargetSize();
}

f64 WorldBorder::getMinX() const
{
    return m_state->getMinX();
}

f64 WorldBorder::getMaxX() const
{
    return m_state->getMaxX();
}

f64 WorldBorder::getMinZ() const
{
    return m_state->getMinZ();
}

f64 WorldBorder::getMaxZ() const
{
    return m_state->getMaxZ();
}

BorderStatus WorldBorder::getStatus() const
{
    return m_state->getStatus();
}

f64 WorldBorder::getResizeSpeed() const
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

void WorldBorder::setDamagePerBlock(f64 damagePerBlock)
{
    m_damagePerBlock = damagePerBlock;
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onDamagePerBlockChanged(damagePerBlock);
        }
    }
}

void WorldBorder::setDamageBuffer(f64 damageBuffer)
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

void WorldBorder::setSize(f64 size)
{
    size = std::clamp(size, 1.0, MAX_SIZE);
    m_state = std::make_unique<StationaryBorderState>(size, m_centerX, m_centerZ);
    _notifySizeChanged(size);
}

void WorldBorder::setSizeLerp(f64 oldSize, f64 newSize, u64 timeMs)
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

void WorldBorder::setCenter(f64 x, f64 z)
{
    m_centerX = x;
    m_centerZ = z;
    m_state->onCenterChanged(x, z);
    _notifyCenterChanged(x, z);
}

// ============================================================================
// 边界检测
// ============================================================================

bool WorldBorder::contains(f64 x, f64 z) const
{
    return x > getMinX() && x < getMaxX() && z > getMinZ() && z < getMaxZ();
}

bool WorldBorder::contains(const BlockPos& pos) const
{
    // 方块位置检测：方块必须在边界内（方块边界需要完全在内）
    return (static_cast<f64>(pos.x) + 1.0) > getMinX() && static_cast<f64>(pos.x) < getMaxX() &&
        (static_cast<f64>(pos.z) + 1.0) > getMinZ() && static_cast<f64>(pos.z) < getMaxZ();
}

bool WorldBorder::intersects(const AxisAlignedBB& box) const
{
    return box.maxX > getMinX() && box.minX < getMaxX() && box.maxZ > getMinZ() && box.minZ < getMaxZ();
}

bool WorldBorder::intersectsChunk(i32 chunkX, i32 chunkZ) const
{
    constexpr f64 CHUNK_SIZE = static_cast<f64>(world::CHUNK_WIDTH);
    f64 chunkMinX = static_cast<f64>(chunkX) * CHUNK_SIZE;
    f64 chunkMinZ = static_cast<f64>(chunkZ) * CHUNK_SIZE;
    f64 chunkMaxX = chunkMinX + CHUNK_SIZE;
    f64 chunkMaxZ = chunkMinZ + CHUNK_SIZE;

    return chunkMaxX > getMinX() && chunkMinX < getMaxX() && chunkMaxZ > getMinZ() && chunkMinZ < getMaxZ();
}

f64 WorldBorder::getClosestDistance(f64 x, f64 z) const
{
    f64 distToMinX = x - getMinX(); // 到西边界的距离
    f64 distToMaxX = getMaxX() - x; // 到东边界的距离
    f64 distToMinZ = z - getMinZ(); // 到北边界的距离
    f64 distToMaxZ = getMaxZ() - z; // 到南边界的距离

    // 返回最小距离（如果点在边界内则为正，否则为负）
    return std::min({distToMinX, distToMaxX, distToMinZ, distToMaxZ});
}

f64 WorldBorder::getClosestDistance(const AxisAlignedBB& box) const
{
    // 计算 AABB 中心到边界的距离
    f64 centerX = (box.minX + box.maxX) / 2.0;
    f64 centerZ = (box.minZ + box.maxZ) / 2.0;

    // 使用 AABB 的最近边计算距离
    f64 distToMinX = box.minX - getMinX(); // 负值表示超出边界
    f64 distToMaxX = getMaxX() - box.maxX;
    f64 distToMinZ = box.minZ - getMinZ();
    f64 distToMaxZ = getMaxZ() - box.maxZ;

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

void WorldBorder::_notifySizeChanged(f64 newSize)
{
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onSizeChanged(newSize);
        }
    }
}

void WorldBorder::_notifyTransitionStarted(f64 oldSize, f64 newSize, u64 timeMs)
{
    for (auto& weakListener : m_listeners) {
        if (auto listener = weakListener.lock()) {
            listener->onTransitionStarted(oldSize, newSize, timeMs);
        }
    }
}

void WorldBorder::_notifyCenterChanged(f64 x, f64 z)
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
