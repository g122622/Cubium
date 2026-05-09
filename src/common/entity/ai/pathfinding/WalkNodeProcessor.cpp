#include "WalkNodeProcessor.hpp"
#include "../../../core/Constants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../world/block/blocks/decorative/CampfireBlock.hpp"
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

    // MC 1.16.5 func_237238_b_: 检查危险方块类型
    // 获取方块状态进行更详细的检查
    const BlockState* state = m_region->getBlockState(x, y, z);
    if (state != nullptr) {
        const Block& block = state->getBlock();

        // 仙人掌 - 直接站在仙人掌上（DAMAGE_CACTUS）
        if (VanillaBlocks::CACTUS != nullptr && &block == VanillaBlocks::CACTUS) {
            return PathNodeType::DamageCactus;
        }

        // 甜浆果丛 - 直接站在甜浆果丛上（DAMAGE_OTHER）
        Block* sweetBerryBush = Block::getBlock(ResourceLocation("minecraft", "sweet_berry_bush"));
        if (sweetBerryBush != nullptr && &block == sweetBerryBush) {
            return PathNodeType::DamageOther;
        }

        // 火焰/岩浆块/点燃的营火 - 直接站在危险火源上（DAMAGE_FIRE）
        if (BlockTags::FIRE().contains(block)) {
            return PathNodeType::DamageFire;
        }
        if (VanillaBlocks::MAGMA != nullptr && &block == VanillaBlocks::MAGMA) {
            return PathNodeType::DamageFire;
        }
        if ((VanillaBlocks::CAMPFIRE != nullptr && &block == VanillaBlocks::CAMPFIRE) ||
            (VanillaBlocks::SOUL_CAMPFIRE != nullptr && &block == VanillaBlocks::SOUL_CAMPFIRE)) {
            if (blocks::CampfireBlock::isLit(*state)) {
                return PathNodeType::DamageFire;
            }
        }
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

    // MC 1.16.5 func_237232_a_: 检查相邻危险方块
    // 当当前位置是可行走的或开放的时，检查周围是否有危险方块
    if (type == PathNodeType::Walkable || type == PathNodeType::Open) {
        // 检查 3x3x3 相邻区域（包括上下）
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dy = -1; dy <= 1; ++dy) {
                for (i32 dz = -1; dz <= 1; ++dz) {
                    // 跳过自身
                    if (dx == 0 && dy == 0 && dz == 0) continue;

                    i32 nx = x + dx;
                    i32 ny = y + dy;
                    i32 nz = z + dz;

                    // 检查危险方块类型
                    const BlockState* neighborState = m_region->getBlockState(nx, ny, nz);
                    if (neighborState == nullptr) continue;

                    const Block& neighborBlock = neighborState->getBlock();

                    // 仙人掌相邻 - DANGER_CACTUS
                    if (VanillaBlocks::CACTUS != nullptr && &neighborBlock == VanillaBlocks::CACTUS) {
                        return PathNodeType::DangerCactus;
                    }

                    // 甜浆果丛相邻 - DANGER_OTHER (MC 1.15+ 使用 DANGER_BERRY，但我们的枚举有这个)
                    Block* sweetBerryBush = Block::getBlock(ResourceLocation("minecraft", "sweet_berry_bush"));
                    if (sweetBerryBush != nullptr && &neighborBlock == sweetBerryBush) {
                        return PathNodeType::DangerBerry;
                    }

                    // 火焰/岩浆块/点燃的营火相邻 - DANGER_FIRE
                    if (BlockTags::FIRE().contains(neighborBlock)) {
                        return PathNodeType::DangerFire;
                    }
                    if (VanillaBlocks::MAGMA != nullptr && &neighborBlock == VanillaBlocks::MAGMA) {
                        return PathNodeType::DangerFire;
                    }
                    if ((VanillaBlocks::CAMPFIRE != nullptr && &neighborBlock == VanillaBlocks::CAMPFIRE) ||
                        (VanillaBlocks::SOUL_CAMPFIRE != nullptr && &neighborBlock == VanillaBlocks::SOUL_CAMPFIRE)) {
                        if (blocks::CampfireBlock::isLit(*neighborState)) {
                            return PathNodeType::DangerFire;
                        }
                    }

                    // 水边检查 - WATER_BORDER
                    if (m_region->isWater(nx, ny, nz)) {
                        type = PathNodeType::WaterBorder;
                    }
                }
            }
        }
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
    // 检查火焰、岩浆、岩浆块、点燃的营火等危险方块

    // 1. 岩浆（流体）
    if (m_region->isLava(x, y, z)) {
        return true;
    }

    // 2. 获取方块状态进行更详细的检查
    const BlockState* state = m_region->getBlockState(x, y, z);
    if (state == nullptr) {
        // 空气方块，不是危险方块
        return false;
    }

    const Block& block = state->getBlock();

    // 3. 火焰方块（普通火、灵魂火）
    // 使用 BlockTags::FIRE 标签检查
    if (BlockTags::FIRE().contains(block)) {
        return true;
    }

    // 4. 岩浆块（MAGMA_BLOCK）
    // 站在上面会造成伤害（HotFloor 伤害）
    if (VanillaBlocks::MAGMA != nullptr && &block == VanillaBlocks::MAGMA) {
        return true;
    }

    // 5. 点燃的营火（CampfireBlock、SoulCampfireBlock）
    // 只有点燃状态下才危险
    if (VanillaBlocks::CAMPFIRE != nullptr && &block == VanillaBlocks::CAMPFIRE) {
        if (blocks::CampfireBlock::isLit(*state)) {
            return true;
        }
    }
    if (VanillaBlocks::SOUL_CAMPFIRE != nullptr && &block == VanillaBlocks::SOUL_CAMPFIRE) {
        if (blocks::CampfireBlock::isLit(*state)) {
            return true;
        }
    }

    // 6. 仙人掌（CACTUS）
    // 接触会造成伤害
    if (VanillaBlocks::CACTUS != nullptr && &block == VanillaBlocks::CACTUS) {
        return true;
    }

    // 7. 甜浆果丛（SWEET_BERRY_BUSH）
    // 接触会造成伤害和减速（通过 ResourceLocation 查找，因为可能未在 VanillaBlocks 中注册）
    // MC 1.16.5: 只有年龄大于0的甜浆果丛才造成伤害
    // 参考: SweetBerryBushBlock.onEntityCollision
    // 由于甜浆果丛在 VanillaBlocks 中尚未注册，暂时通过 ResourceLocation 查找
    Block* sweetBerryBush = Block::getBlock(ResourceLocation("minecraft", "sweet_berry_bush"));
    if (sweetBerryBush != nullptr && &block == sweetBerryBush) {
        return true;
    }

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
