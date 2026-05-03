#include "WalkNodeProcessor.hpp"
#include "../../../core/Constants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../core/LivingEntity.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

using namespace mc::math;

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

    // MC 1.16.5: 检查实体高度范围内的所有方块
    i32 heightCount = static_cast<i32>(std::ceil(m_entityHeight));
    for (i32 dy = 1; dy <= heightCount; ++dy) {
        PathNodeType upperType = getNodeType(x, y + dy, z);
        if (upperType == PathNodeType::Blocked) {
            return PathNodeType::Blocked;
        }
    }

    // MC 1.16.5: 对于宽度大于0.6的实体，检查额外位置
    // 宽实体需要检查角落碰撞
    if (m_entityWidth > 0.6f && m_entity != nullptr) {
        // MC 1.16.5: 检查实体边界框覆盖的所有方块位置
        // 计算实体边界框的最小/最大坐标
        f64 entityX = m_entity->x();
        f64 entityZ = m_entity->z();
        f32 halfWidth = m_entityWidth / 2.0f;

        // 获取边界框覆盖的方块范围
        i32 minX = floorTo<i32>(entityX - halfWidth);
        i32 maxX = floorTo<i32>(entityX + halfWidth);
        i32 minZ = floorTo<i32>(entityZ - halfWidth);
        i32 maxZ = floorTo<i32>(entityZ + halfWidth);

        // 检查边界框内的所有方块
        for (i32 bx = minX; bx <= maxX; ++bx) {
            for (i32 bz = minZ; bz <= maxZ; ++bz) {
                // 跳过中心位置（已经检查过）
                if (bx == x && bz == z) continue;

                PathNodeType boxType = getNodeType(bx, y, bz);
                if (boxType == PathNodeType::Blocked) {
                    return PathNodeType::Blocked;
                }

                // 检查高度
                for (i32 dy = 1; dy <= heightCount; ++dy) {
                    PathNodeType upperBoxType = getNodeType(bx, y + dy, bz);
                    if (upperBoxType == PathNodeType::Blocked) {
                        return PathNodeType::Blocked;
                    }
                }
            }
        }
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

    // MC 1.16.5: 检查位置本身和周围是否有危险
    // 首先检查岩浆
    if (m_region->isLava(x, y, z)) {
        return false;
    }

    // 检查周围9格是否有危险方块
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                if (isDangerous(x + dx, y + dy, z + dz)) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool WalkNodeProcessor::isDangerous(i32 x, i32 y, i32 z) const {
    if (!m_region) return false;

    // MC 1.16.5 func_237233_a_:
    // 检查火焰、岩浆、岩浆块、营火等危险方块
    // 目前简化实现，只检查岩浆
    // TODO: 需要Region接口扩展以支持更详细的方块类型检查

    // 岩浆
    if (m_region->isLava(x, y, z)) {
        return true;
    }

    // TODO: 添加更多危险方块检查
    // - 火焰 (Blocks.FIRE)
    // - 岩浆块 (Blocks.MAGMA_BLOCK)
    // - 点燃的营火 (CampfireBlock.isLit)
    // - 仙人掌 (Blocks.CACTUS)
    // - 甜浆果丛 (Blocks.SWEET_BERRY_BUSH)

    return false;
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
