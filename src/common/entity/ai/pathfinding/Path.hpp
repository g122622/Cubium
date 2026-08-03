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

#include "../../../core/Types.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "PathPoint.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace mc {

// 前向声明
class Entity;

namespace entity::ai::pathfinding {

/**
 * @brief 路径对象
 *
 * 表示从起点到终点的完整路径，包含所有路径点。
 */
class Path {
public:
    Path() = default;

    /**
     * @brief 拷贝构造函数
     */
    Path(const Path& other) = default;

    /**
     * @brief 移动构造函数
     */
    Path(Path&& other) noexcept
        : m_points(std::move(other.m_points))
        , m_target(other.m_target)
        , m_distToTarget(other.m_distToTarget)
        , m_reachesTarget(other.m_reachesTarget)
        , m_currentIndex(other.m_currentIndex)
    {
        other.m_distToTarget = 0.0f;
        other.m_reachesTarget = false;
        other.m_currentIndex = 0;
    }

    /**
     * @brief 拷贝赋值运算符
     */
    Path& operator=(const Path& other) = default;

    /**
     * @brief 移动赋值运算符
     */
    Path& operator=(Path&& other) noexcept
    {
        if (this != &other) {
            m_points = std::move(other.m_points);
            m_target = other.m_target;
            m_distToTarget = other.m_distToTarget;
            m_reachesTarget = other.m_reachesTarget;
            m_currentIndex = other.m_currentIndex;
            other.m_distToTarget = 0.0f;
            other.m_reachesTarget = false;
            other.m_currentIndex = 0;
        }
        return *this;
    }

    /**
     * @brief 析构函数
     */
    ~Path() = default;

    /**
     * @brief 构造函数
     * @param points 路径点数组
     */
    explicit Path(std::vector<PathPoint>&& points)
        : m_points(std::move(points))
    {}

    /**
     * @brief 完整构造函数
     * @param points 路径点数组
     * @param target 目标位置
     * @param reachesTarget 是否到达目标
     */
    Path(std::vector<PathPoint>&& points, const BlockPos& target, bool reachesTarget)
        : m_points(std::move(points))
        , m_target(target)
        , m_reachesTarget(reachesTarget)
    {}

    // ========== 路径信息 ==========

    /**
     * @brief 获取路径长度
     */
    [[nodiscard]] size_t length() const { return m_points.size(); }

    /**
     * @brief 检查路径是否为空
     */
    [[nodiscard]] bool empty() const { return m_points.empty(); }

    /**
     * @brief 获取路径终点
     * @return 终点路径点，如果路径为空返回nullptr
     */
    [[nodiscard]] const PathPoint* getEnd() const { return m_points.empty() ? nullptr : &m_points.back(); }

    /**
     * @brief 获取路径起点
     * @return 起点路径点，如果路径为空返回nullptr
     */
    [[nodiscard]] const PathPoint* getStart() const { return m_points.empty() ? nullptr : &m_points.front(); }

    /**
     * @brief 获取目标位置
     */
    [[nodiscard]] const BlockPos& getTarget() const { return m_target; }

    /**
     * @brief 设置目标位置
     */
    void setTarget(const BlockPos& target) { m_target = target; }

    /**
     * @brief 检查是否到达目标
     */
    [[nodiscard]] bool reachesTarget() const { return m_reachesTarget; }

    /**
     * @brief 设置是否到达目标
     */
    void setReachesTarget(bool reaches) { m_reachesTarget = reaches; }

    /**
     * @brief 获取到目标的距离
     */
    [[nodiscard]] f32 getDistToTarget() const { return m_distToTarget; }

    /**
     * @brief 设置到目标的距离
     */
    void setDistToTarget(f32 dist) { m_distToTarget = dist; }

    // ========== 路径点访问 ==========

    /**
     * @brief 获取指定索引的路径点
     * @param index 索引
     * @return 路径点指针，如果索引无效返回nullptr
     */
    [[nodiscard]] const PathPoint* getPoint(size_t index) const
    {
        if (index >= m_points.size()) {
            return nullptr;
        }
        return &m_points[index];
    }

    /**
     * @brief 获取所有路径点
     */
    [[nodiscard]] const std::vector<PathPoint>& getPoints() const { return m_points; }

    // ========== 路径导航 ==========

    /**
     * @brief 获取当前目标路径点
     * @return 当前目标路径点，如果已到达终点返回nullptr
     */
    [[nodiscard]] const PathPoint* getCurrentTarget() const
    {
        if (m_points.empty() || m_currentIndex >= static_cast<i32>(m_points.size())) {
            return nullptr;
        }
        return &m_points[static_cast<size_t>(m_currentIndex)];
    }

    /**
     * @brief 获取当前路径点索引
     */
    [[nodiscard]] i32 getCurrentIndex() const { return m_currentIndex; }

    /**
     * @brief 前进到下一个路径点
     * @return 是否还有下一个路径点
     */
    bool advance()
    {
        ++m_currentIndex;
        return m_currentIndex < static_cast<i32>(m_points.size());
    }

    /**
     * @brief 检查是否已到达终点
     */
    [[nodiscard]] bool isFinished() const
    {
        return m_points.empty() || m_currentIndex >= static_cast<i32>(m_points.size());
    }

    /**
     * @brief 重置路径进度
     */
    void reset() { m_currentIndex = 0; }

    /**
     * @brief 设置当前索引
     */
    void setCurrentIndex(i32 index)
    {
        m_currentIndex = std::max(0, std::min(index, static_cast<i32>(m_points.size()) - 1));
    }

    /**
     * @brief 设置路径长度（裁剪）
     * @param length 新的路径长度
     */
    void setCurrentPathLength(i32 length)
    {
        if (length >= 0 && length < static_cast<i32>(m_points.size())) {
            m_points.resize(static_cast<size_t>(length));
        }
    }

    /**
     * @brief 设置指定索引的路径点
     * @param index 索引
     * @param point 新的路径点
     */
    void setPoint(size_t index, const PathPoint& point)
    {
        if (index < m_points.size()) {
            m_points[index] = point;
        }
    }

    /**
     * @brief 设置指定索引的路径点（移动语义）
     * @param index 索引
     * @param point 新的路径点
     */
    void setPoint(size_t index, PathPoint&& point)
    {
        if (index < m_points.size()) {
            m_points[index] = std::move(point);
        }
    }

    // ========== 路径操作 ==========

    /**
     * @brief 添加路径点（用于路径构建）
     */
    void addPoint(const PathPoint& point) { m_points.push_back(point); }

    /**
     * @brief 添加路径点（移动语义）
     */
    void addPoint(PathPoint&& point) { m_points.push_back(std::move(point)); }

    /**
     * @brief 从终点反向构建路径
     * @param end 终点路径点（通过parent链回溯）
     * @return 构建的路径
     */
    static Path buildFromEnd(const PathPoint* end)
    {
        // 先计算路径长度以预留空间
        size_t count = 0;
        const PathPoint* current = end;
        while (current != nullptr) {
            ++count;
            current = current->parent();
        }

        std::vector<PathPoint> points;
        points.reserve(count);

        // 回溯父节点
        current = end;
        while (current != nullptr) {
            points.push_back(current->clone());
            current = current->parent();
        }

        // 反转路径（起点在前）
        std::reverse(points.begin(), points.end());

        return Path(std::move(points));
    }

    /**
     * @brief 裁剪路径起点
     * @param count 要移除的起点数量
     */
    void trimStart(size_t count)
    {
        if (count >= m_points.size()) {
            m_points.clear();
            m_currentIndex = 0;
        } else {
            m_points.erase(m_points.begin(), m_points.begin() + static_cast<i32>(count));
            m_currentIndex = std::max(0, m_currentIndex - static_cast<i32>(count));
        }
    }

    /**
     * @brief 从指定索引处截断路径
     *
     * 移除从 index 开始到末尾的所有路径点。
     * 用于阳光避让逻辑：当寻路路径中某个节点暴露在阳光下时，
     * 在该节点处截断路径，使实体只在阴影区域移动。
     *
     * @param index 截断起始索引（该索引及之后的节点将被移除）
     */
    void truncateNodes(i32 index)
    {
        if (index >= 0 && index < static_cast<i32>(m_points.size())) {
            m_points.resize(static_cast<size_t>(index));
            // 如果当前索引超出新的路径长度，调整到末尾
            if (m_currentIndex >= static_cast<i32>(m_points.size())) {
                m_currentIndex = std::max(0, static_cast<i32>(m_points.size()) - 1);
            }
        }
    }

    // ========== 状态检查 ==========

    /**
     * @brief 检查路径是否到达了目标位置
     * @param x 目标X坐标
     * @param y 目标Y坐标
     * @param z 目标Z坐标
     * @param tolerance 位置容差
     */
    [[nodiscard]] bool reachesTarget(i32 x, i32 y, i32 z, f32 tolerance = 1.0f) const
    {
        if (m_points.empty()) {
            return false;
        }

        const PathPoint& end = m_points.back();
        f32 dx = static_cast<f32>(end.x() - x);
        f32 dy = static_cast<f32>(end.y() - y);
        f32 dz = static_cast<f32>(end.z() - z);

        return (dx * dx + dy * dy + dz * dz) <= tolerance * tolerance;
    }

    /**
     * @brief 获取路径点位置的向量（考虑实体宽度）
     * @param entity 实体
     * @param index 路径点索引
     * @return 位置向量
     */
    [[nodiscard]] Vector3d getVectorFromIndex(const Entity* entity, i32 index) const;

    /**
     * @brief 获取当前位置向量
     * @param entity 实体
     * @return 当前目标位置向量
     */
    [[nodiscard]] Vector3d getPosition(const Entity* entity) const;

    /**
     * @brief 检查两个路径是否相同
     * @param other 另一个路径
     */
    [[nodiscard]] bool isSamePath(const Path& other) const
    {
        if (m_points.size() != other.m_points.size()) {
            return false;
        }
        for (size_t i = 0; i < m_points.size(); ++i) {
            if (!m_points[i].equals(other.m_points[i])) {
                return false;
            }
        }
        return true;
    }

private:
    std::vector<PathPoint> m_points;
    BlockPos m_target;            // 目标位置
    f32 m_distToTarget = 0.0f;    // 到目标的距离
    bool m_reachesTarget = false; // 是否到达目标
    i32 m_currentIndex = 0;       // 当前路径点索引
};

} // namespace entity::ai::pathfinding
} // namespace mc
