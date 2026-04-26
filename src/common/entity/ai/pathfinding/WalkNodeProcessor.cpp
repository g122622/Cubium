#include "WalkNodeProcessor.hpp"
#include "../../../core/Constants.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

PathNodeType WalkNodeProcessor::getNodeType(i32 x, i32 y, i32 z) {
    if (!m_region) {
        return PathNodeType::Blocked;
    }

    // 检查是否加载
    if (!m_region->isLoaded(x, z)) {
        return PathNodeType::Blocked;
    }

    // 检查水
    if (m_region->isWater(x, y, z)) {
        return m_canSwim ? PathNodeType::Water : PathNodeType::Blocked;
    }

    // 检查岩浆
    if (m_region->isLava(x, y, z)) {
        return PathNodeType::Lava;
    }

    // 检查是否可行走
    if (m_region->isWalkable(x, y, z)) {
        // 检查下方是否有支撑
        if (canStandOn(x, y - 1, z)) {
            return PathNodeType::Walkable;
        }
        return PathNodeType::Open;
    }

    // 检查是否可以穿过（空气）
    if (isPassable(x, y, z)) {
        // 向下寻找地面
        i32 groundY = getGroundHeight(x, y, z);
        if (groundY < y - m_maxFallDistance) {
            return PathNodeType::DangerFall;
        }
        return PathNodeType::Open;
    }

    return PathNodeType::Blocked;
}

PathNodeType WalkNodeProcessor::getNodeTypeWithEntity(i32 x, i32 y, i32 z) {
    // 获取基础类型
    PathNodeType type = getNodeType(x, y, z);

    if (type == PathNodeType::Blocked) {
        return PathNodeType::Blocked;
    }

    // 检查实体高度范围内的所有方块
    i32 heightCount = static_cast<i32>(std::ceil(m_entityHeight));
    for (i32 dy = 1; dy <= heightCount; ++dy) {
        PathNodeType upperType = getNodeType(x, y + dy, z);
        if (upperType == PathNodeType::Blocked) {
            return PathNodeType::Blocked;
        }
    }

    // 检查实体宽度范围内是否有障碍
    if (m_entityWidth > 0.6f) {
        // 对于宽度大于0.6的实体，检查额外位置
        // 简化实现：只检查角落
        // TODO: 更精确的碰撞检测
    }

    return type;
}

PathPoint* WalkNodeProcessor::getStartNode(i32 x, i32 y, i32 z) {
    // 找到实体脚下的地面
    i32 groundY = getGroundHeight(x, y, z);

    // 如果实体在地面之上，使用实体当前Y
    if (groundY < y) {
        groundY = y;
    }

    return getNode(x, groundY, z);
}

std::vector<PathPoint*> WalkNodeProcessor::getNeighbors(PathPoint* current) {
    std::vector<PathPoint*> neighbors;
    neighbors.reserve(26); // 最多26个相邻节点

    if (!current || !m_region) {
        return neighbors;
    }

    i32 x = current->x();
    i32 y = current->y();
    i32 z = current->z();
    PathNodeType currentType = current->nodeType();

    // 水平方向：4个方向 + 4个对角线
    static const i32 dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const i32 dz[] = {0, 0, 1, -1, 1, -1, 1, -1};

    for (i32 i = 0; i < 8; ++i) {
        i32 nx = x + dx[i];
        i32 nz = z + dz[i];

        // 检查是否是对角线移动
        bool isDiagonal = (i >= 4);

        // 检查水平移动
        PathNodeType type = getNodeType(nx, y, nz);

        if (type == PathNodeType::Walkable || type == PathNodeType::Water ||
            type == PathNodeType::Climbable) {
            // 检查对角线移动时是否被阻挡
            if (isDiagonal) {
                PathNodeType type1 = getNodeType(x + dx[i], y, z);
                PathNodeType type2 = getNodeType(x, y, z + dz[i]);

                if (type1 == PathNodeType::Blocked || type2 == PathNodeType::Blocked) {
                    continue;
                }
            }

            addNeighbor(neighbors, nx, y, nz, type);
        }
        else if (type == PathNodeType::Open || type == PathNodeType::DangerFall) {
            // 检查是否需要跳跃
            PathNodeType upperType = getNodeType(x, y + 1, z);
            if (upperType == PathNodeType::Walkable && canStandOn(nx, y, nz)) {
                // 可以跳跃上去
                addNeighbor(neighbors, nx, y + 1, nz, PathNodeType::Walkable);
            }
            else if (type == PathNodeType::DangerFall && currentType == PathNodeType::Walkable) {
                // 可以跌落
                i32 groundY = getGroundHeight(nx, y, nz);
                if (groundY >= y - m_maxFallDistance) {
                    addNeighbor(neighbors, nx, groundY, nz, PathNodeType::Walkable);
                }
            }
        }
        else if (type == PathNodeType::Blocked) {
            // 尝试跳上障碍物
            if (currentType == PathNodeType::Walkable && canStandOn(nx, y, nz)) {
                PathNodeType upperType = getNodeType(nx, y + 1, nz);
                if (upperType == PathNodeType::Walkable || upperType == PathNodeType::Open) {
                    addNeighbor(neighbors, nx, y + 1, nz, PathNodeType::Walkable);
                }
            }
        }
    }

    // 攀爬（梯子、藤蔓等）
    if (m_canClimb && currentType == PathNodeType::Climbable) {
        // 向上攀爬
        PathNodeType upperType = getNodeType(x, y + 1, z);
        if (upperType == PathNodeType::Climbable) {
            addNeighbor(neighbors, x, y + 1, z, PathNodeType::Climbable);
        }
        // 向下攀爬
        PathNodeType lowerType = getNodeType(x, y - 1, z);
        if (lowerType == PathNodeType::Climbable) {
            addNeighbor(neighbors, x, y - 1, z, PathNodeType::Climbable);
        }
    }

    // 水中移动
    if (currentType == PathNodeType::Water && m_canSwim) {
        // 向上游泳
        PathNodeType upperType = getNodeType(x, y + 1, z);
        if (upperType == PathNodeType::Water) {
            addNeighbor(neighbors, x, y + 1, z, PathNodeType::Water);
        }
        // 向下游泳
        PathNodeType lowerType = getNodeType(x, y - 1, z);
        if (lowerType == PathNodeType::Water) {
            addNeighbor(neighbors, x, y - 1, z, PathNodeType::Water);
        }
    }

    return neighbors;
}

std::unique_ptr<PathPoint> WalkNodeProcessor::createNode(i32 x, i32 y, i32 z) {
    if (!m_region || !m_region->isLoaded(x, z)) {
        return nullptr;
    }

    PathNodeType type = getNodeType(x, y, z);

    if (type == PathNodeType::Blocked) {
        return nullptr;
    }

    auto node = std::make_unique<PathPoint>(x, y, z);
    node->setNodeType(type);
    node->setCostMalus(getPathCostPenalty(type));

    return node;
}

bool WalkNodeProcessor::isWalkableAt(i32 x, i32 y, i32 z) const {
    if (!m_region) return false;

    // 检查位置本身是否可以站立
    return m_region->isWalkable(x, y, z);
}

bool WalkNodeProcessor::canStandOn(i32 x, i32 y, i32 z) const {
    if (!m_region) return false;

    // 检查脚下是否有支撑
    return m_region->isWalkable(x, y, z) && isPassable(x, y + 1, z);
}

bool WalkNodeProcessor::isSafe(i32 x, i32 y, i32 z) const {
    if (!m_region) return false;

    // 检查是否在火焰、仙人掌等危险方块旁边
    // 简化实现
    return !m_region->isLava(x, y, z);
}

i32 WalkNodeProcessor::getGroundHeight(i32 x, i32 y, i32 z) const {
    if (!m_region) return y;

    // 从指定位置向下搜索地面
    for (i32 checkY = y; checkY >= world::MIN_BUILD_HEIGHT; --checkY) {
        if (m_region->isWalkable(x, checkY, z)) {
            return checkY;
        }
    }

    return world::MIN_BUILD_HEIGHT;
}

bool WalkNodeProcessor::isPassable(i32 x, i32 y, i32 z) const {
    if (!m_region) return true;

    // 检查位置是否可以穿过（空气、水等）
    return !m_region->isWalkable(x, y, z) || m_region->isWater(x, y, z);
}

void WalkNodeProcessor::addNeighbor(std::vector<PathPoint*>& neighbors, i32 x, i32 y, i32 z, PathNodeType type) {
    PathPoint* node = getNode(x, y, z);
    if (node) {
        node->setNodeType(type);
        neighbors.push_back(node);
        m_openNodes.push_back(node);
    }
}

void WalkNodeProcessor::addJumpNeighbor(std::vector<PathPoint*>& neighbors, PathPoint* current, i32 dx, i32 dz) {
    // 检查是否可以跳跃到相邻位置
    i32 x = current->x() + dx;
    i32 z = current->z() + dz;

    // 检查跳跃路径
    PathNodeType type1 = getNodeType(x, current->y(), z);
    PathNodeType type2 = getNodeType(x, current->y() + 1, z);

    if (type1 == PathNodeType::Walkable && type2 == PathNodeType::Open) {
        addNeighbor(neighbors, x, current->y() + 1, z, PathNodeType::Walkable);
    }
}

void WalkNodeProcessor::addFallNeighbor(std::vector<PathPoint*>& neighbors, i32 x, i32 startY, i32 z) {
    // 检查是否可以跌落到相邻位置
    i32 groundY = getGroundHeight(x, startY, z);
    if (groundY >= startY - m_maxFallDistance) {
        addNeighbor(neighbors, x, groundY, z, PathNodeType::Walkable);
    }
}

} // namespace mc::entity::ai::pathfinding
