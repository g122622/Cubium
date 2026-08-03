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
#include "NodeProcessor.hpp"
#include "PathNodeType.hpp"
#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class LivingEntity;

namespace entity::ai::pathfinding {

/**
 * @brief 行走节点处理器
 *
 * 处理地面行走实体的路径节点生成。
 * 支持：水平移动、跳跃、跌落、攀爬。
 */
class WalkNodeProcessor : public NodeProcessor {
public:
    WalkNodeProcessor() = default;
    ~WalkNodeProcessor() override = default;

    // ========== NodeProcessor 接口实现 ==========

    [[nodiscard]] PathNodeType getNodeType(i32 x, i32 y, i32 z) override;
    [[nodiscard]] PathNodeType getNodeTypeWithEntity(i32 x, i32 y, i32 z) override;
    [[nodiscard]] PathPoint* getStartNode(i32 x, i32 y, i32 z) override;
    [[nodiscard]] std::vector<PathPoint*> getNeighbors(PathPoint* current) override;

    // ========== 实体引用 ==========

    /**
     * @brief 设置关联实体
     */
    void setEntity(LivingEntity* entity) { m_entity = entity; }

    /**
     * @brief 获取关联实体
     */
    [[nodiscard]] LivingEntity* getEntity() const { return m_entity; }

    // ========== 配置 ==========

    /**
     * @brief 设置是否可以游泳
     */
    void setCanSwim(bool canSwim) { m_canSwim = canSwim; }

    /**
     * * @brief 设置是否可以开门
     */
    void setCanOpenDoors(bool canOpenDoors) { m_canOpenDoors = canOpenDoors; }

    /**
     * @brief 获取是否可以开门
     */
    [[nodiscard]] bool canOpenDoors() const noexcept { return m_canOpenDoors; }

    /**
     * @brief 设置是否可以通过门
     *
     * 对应 MC 的 canPassDoors，默认为 true。
     * 当为 true 时，打开的门（DoorOpen）和打开/关闭的栅栏门（FenceGate）被视为可通行；
     * 当为 false 时，打开的门被转换为 Blocked，栅栏门不可通行。
     */
    void setCanEnterDoors(bool canEnterDoors) { m_canEnterDoors = canEnterDoors; }

    /**
     * @brief 获取是否可以通过门
     */
    [[nodiscard]] bool canEnterDoors() const noexcept { return m_canEnterDoors; }

    /**
     * @brief 设置是否可以爬墙
     */
    void setCanClimb(bool canClimb) { m_canClimb = canClimb; }

    /**
     * @brief 设置最大跌落距离
     */
    void setMaxFallDistance(i32 distance) { m_maxFallDistance = distance; }

    /**
     * @brief 设置是否避免水
     */
    void setAvoidWater(bool avoidWater) { m_avoidWater = avoidWater; }

    /**
     * @brief 设置是否避免太阳
     *
     * 当设置为 true 时，WalkNodeProcessor 会标记阳光避让标志。
     * 实际的阳光避让逻辑由 PathNavigator::_trimPath() 实现：
     * 路径计算完成后，遍历路径节点并在第一个暴露在阳光下的节点处截断路径，
     * 使实体只在阴影区域移动。如果实体当前已在阳光下，则保留完整路径。
     * 这对应 MC Java 版的 GroundPathNavigation.trimPath() 实现。
     */
    void setAvoidSun(bool avoidSun) { m_avoidSun = avoidSun; }

    // ========== 测试接口 ==========

    /**
     * @brief 检查方块是否是危险方块（火焰、岩浆、仙人掌等）
     * @note 公开用于单元测试
     */
    [[nodiscard]] bool isDangerous(i32 x, i32 y, i32 z) const;

protected:
    [[nodiscard]] std::unique_ptr<PathPoint> createNode(i32 x, i32 y, i32 z) override;

private:
    LivingEntity* m_entity = nullptr;
    bool m_canSwim = false;
    bool m_canOpenDoors = false;
    bool m_canEnterDoors = true;
    bool m_canClimb = false;
    bool m_avoidWater = false;
    bool m_avoidSun = false;
    i32 m_maxFallDistance = 3;

    // ========== 内部方法 ==========

    /**
     * @brief 检查位置是否可行走
     */
    [[nodiscard]] bool _isWalkableAt(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否可以站立
     */
    [[nodiscard]] bool _canStandOn(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否是安全的（没有危险方块）
     */
    [[nodiscard]] bool _isSafe(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取地面高度（从指定位置向下搜索）
     */
    [[nodiscard]] i32 _getGroundHeight(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查方块是否可以穿过
     */
    [[nodiscard]] bool _isPassable(i32 x, i32 y, i32 z) const;

    /**
     * @brief 添加相邻节点
     */
    void _addNeighbor(std::vector<PathPoint*>& neighbors, i32 x, i32 y, i32 z, PathNodeType type);

    /**
     * @brief 添加跳跃节点
     */
    void _addJumpNeighbor(std::vector<PathPoint*>& neighbors, PathPoint* current, i32 dx, i32 dz);

    /**
     * @brief 添加跌落节点
     */
    void _addFallNeighbor(std::vector<PathPoint*>& neighbors, i32 x, i32 startY, i32 z);
};

} // namespace entity::ai::pathfinding
} // namespace mc
