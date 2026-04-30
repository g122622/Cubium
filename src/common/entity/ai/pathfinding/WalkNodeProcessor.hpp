#pragma once

#include "NodeProcessor.hpp"
#include "PathNodeType.hpp"
#include "../../../core/Types.hpp"

namespace mc {

// 前向声明
class LivingEntity;

namespace entity::ai::pathfinding {

/**
 * @brief 行走节点处理器
 *
 * 处理地面行走实体的路径节点生成。
 * 支持：水平移动、跳跃、跌落、攀爬。
 *
 * 参考 MC 1.16.5 WalkNodeProcessor
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

    // ========== MC 1.16.5 实体引用 ==========

    /**
     * @brief 设置关联实体
     * MC 1.16.5: 用于获取 stepHeight 和 pathPriority
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
     * @brief 设置是否可以通过门
     */
    void setCanEnterDoors(bool canEnterDoors) { m_canEnterDoors = canEnterDoors; }

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
     */
    void setAvoidSun(bool avoidSun) { m_avoidSun = avoidSun; }

protected:
    [[nodiscard]] std::unique_ptr<PathPoint> createNode(i32 x, i32 y, i32 z) override;

private:
    LivingEntity* m_entity = nullptr;  // MC 1.16.5: 实体引用
    bool m_canSwim = false;
    bool m_canOpenDoors = false;
    bool m_canEnterDoors = false;
    bool m_canClimb = false;
    bool m_avoidWater = false;
    bool m_avoidSun = false;
    i32 m_maxFallDistance = 3;

    // ========== 内部方法 ==========

    /**
     * @brief 检查位置是否可行走
     */
    [[nodiscard]] bool isWalkableAt(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否可以站立
     */
    [[nodiscard]] bool canStandOn(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查位置是否是安全的（没有危险方块）
     * MC 1.16.5: 检查火焰、岩浆、仙人掌、甜浆果丛等
     */
    [[nodiscard]] bool isSafe(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查方块是否是危险方块（火焰、岩浆、仙人掌等）
     * MC 1.16.5 func_237233_a_
     */
    [[nodiscard]] bool isDangerous(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取地面高度（从指定位置向下搜索）
     */
    [[nodiscard]] i32 getGroundHeight(i32 x, i32 y, i32 z) const;

    /**
     * @brief 检查方块是否可以穿过
     */
    [[nodiscard]] bool isPassable(i32 x, i32 y, i32 z) const;

    /**
     * @brief 添加相邻节点
     */
    void addNeighbor(std::vector<PathPoint*>& neighbors, i32 x, i32 y, i32 z, PathNodeType type);

    /**
     * @brief 添加跳跃节点
     */
    void addJumpNeighbor(std::vector<PathPoint*>& neighbors, PathPoint* current, i32 dx, i32 dz);

    /**
     * @brief 添加跌落节点
     */
    void addFallNeighbor(std::vector<PathPoint*>& neighbors, i32 x, i32 startY, i32 z);
};

} // namespace entity::ai::pathfinding
} // namespace mc
