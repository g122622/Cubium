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

#include "WalkNodeProcessor.hpp"
#include "../../../core/Constants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../world/block/blocks/DoorBlock.hpp"
#include "../../../world/block/blocks/FenceGateBlock.hpp"
#include "../../../world/block/blocks/decorative/CampfireBlock.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../core/MobEntity.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cmath>

namespace mc::entity::ai::pathfinding {

using namespace mc::math;

PathNodeType WalkNodeProcessor::getNodeType(i32 x, i32 y, i32 z)
{
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

    // 检查危险方块类型
    // 获取方块状态进行更详细的检查
    const BlockState* state = m_region->getBlockState(x, y, z);
    if (state != nullptr) {
        const Block& block = state->getBlock();

        // 仙人掌 - 直接站在仙人掌上（DAMAGE_CACTUS）
        if (VanillaBlocks::CACTUS != nullptr && &block == VanillaBlocks::CACTUS) {
            return PathNodeType::DamageCactus;
        }

        // 甜浆果丛 - 直接站在甜浆果丛上（DAMAGE_OTHER）
        if (VanillaBlocks::SWEET_BERRY_BUSH != nullptr && &block == VanillaBlocks::SWEET_BERRY_BUSH) {
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

        // 门方块检测 — 对应 MC WalkNodeEvaluator.getPathTypeFromState()
        auto* doorBlock = dynamic_cast<const blocks::DoorBlock*>(&block);
        if (doorBlock != nullptr) {
            if (blocks::DoorBlock::isOpen(*state)) {
                return PathNodeType::DoorOpen;
            }
            // MC 用 doorblock.type().canOpenByHand() 区分木门和铁门
            // 本项目用 isIronDoor() 判断：铁门无法手动开关，木门可以
            return doorBlock->isIronDoor() ? PathNodeType::DoorIronClosed : PathNodeType::DoorWoodClosed;
        }

        // 栅栏门检测 — 对应 MC WalkNodeEvaluator.getPathTypeFromState() 中的 FenceGateBlock 分支
        // MC 原版中关闭的栅栏门返回 FENCE，打开的栅栏门走 isPathfindable 检查
        // 本项目使用独立的 FenceGate 类型：打开的栅栏门可通行(cost=0)，关闭的栅栏门不可通行(cost=-1)
        auto* fenceGateBlock = dynamic_cast<const blocks::FenceGateBlock*>(&block);
        if (fenceGateBlock != nullptr) {
            if (blocks::FenceGateBlock::isOpen(*state)) {
                return PathNodeType::FenceGate; // 打开的栅栏门，可通行
            }
            // 关闭的栅栏门，返回 Fence 表示不可通行
            return PathNodeType::Fence;
        }
    }

    // 节点位置语义：查询的是实体脚部所在的空间（空气或可穿过方块），支撑来自下方方块。
    // 实心方块本身不可作为节点（实体无法站进实心方块内）→ Blocked。
    if (m_region->isWalkable(x, y, z)) {
        return PathNodeType::Blocked;
    }

    // 此处位置为可穿过（空气等）：脚下(x,y-1,z)实心 → 可站立 → Walkable。
    if (y > world::MIN_BUILD_HEIGHT && m_region->isWalkable(x, y - 1, z)) {
        return PathNodeType::Walkable;
    }

    // 脚下无可站立支撑：向下搜索地面，按最大跌落距离判定。
    i32 groundY = _getGroundHeight(x, y, z);
    if (groundY < y - m_maxFallDistance) {
        return PathNodeType::DangerFall;
    }
    return PathNodeType::Open;
}

PathNodeType WalkNodeProcessor::getNodeTypeWithEntity(i32 x, i32 y, i32 z)
{
    // 获取基础类型
    PathNodeType type = getNodeType(x, y, z);

    if (type == PathNodeType::Blocked) {
        return PathNodeType::Blocked;
    }

    // 门类型转换逻辑 — 对应 MC WalkNodeEvaluator.getPathTypeWithinMobBB()
    // 关闭的木门 + 能开门 + 能穿门 => 可行走的门（WalkableDoor）
    if (type == PathNodeType::DoorWoodClosed && m_canOpenDoors && m_canEnterDoors) {
        type = PathNodeType::WalkableDoor;
    }
    // 打开的门 + 不能穿门 => 阻塞
    if (type == PathNodeType::DoorOpen && !m_canEnterDoors) {
        return PathNodeType::Blocked;
    }
    // 关闭的铁门始终不可通过（铁门无法手动打开，costMalus=-1.0 已阻止通行）

    // 检查相邻危险方块
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

                    // 甜浆果丛相邻 - DANGER_BERRY
                    if (VanillaBlocks::SWEET_BERRY_BUSH != nullptr &&
                        &neighborBlock == VanillaBlocks::SWEET_BERRY_BUSH) {
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

    // 检查实体高度范围内的所有方块
    i32 heightCount = static_cast<i32>(std::ceil(m_entityHeight));
    for (i32 dy = 1; dy <= heightCount; ++dy) {
        PathNodeType upperType = getNodeType(x, y + dy, z);
        if (upperType == PathNodeType::Blocked) {
            return PathNodeType::Blocked;
        }
    }

    // 对于宽度大于0.6的实体，检查额外位置
    // 宽实体需要检查角落碰撞
    if (m_entityWidth > 0.6f && m_entity != nullptr) {
        // 检查实体边界框覆盖的所有方块位置
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

PathPoint* WalkNodeProcessor::getStartNode(i32 x, i32 y, i32 z)
{
    // 找到实体脚下的地面
    i32 groundY = _getGroundHeight(x, y, z);

    // 如果实体在地面之上，使用实体当前Y
    if (groundY < y) {
        groundY = y;
    }

    return getNode(x, groundY, z);
}

std::vector<PathPoint*> WalkNodeProcessor::getNeighbors(PathPoint* current)
{
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

        if (type == PathNodeType::Walkable || type == PathNodeType::Water || type == PathNodeType::Climbable ||
            type == PathNodeType::WalkableDoor || type == PathNodeType::DoorOpen || type == PathNodeType::FenceGate) {
            // 检查对角线移动时是否被阻挡
            if (isDiagonal) {
                PathNodeType type1 = getNodeType(x + dx[i], y, z);
                PathNodeType type2 = getNodeType(x, y, z + dz[i]);

                if (type1 == PathNodeType::Blocked || type2 == PathNodeType::Blocked) {
                    continue;
                }

                // 不能对角线穿过门节点 — 对应 MC WalkNodeEvaluator.isDiagonalValid()
                if (type == PathNodeType::WalkableDoor || type1 == PathNodeType::WalkableDoor ||
                    type2 == PathNodeType::WalkableDoor) {
                    continue;
                }
            }

            _addNeighbor(neighbors, nx, y, nz, type);
        } else if (type == PathNodeType::Open || type == PathNodeType::DangerFall) {
            // 检查是否需要跳跃
            PathNodeType upperType = getNodeType(x, y + 1, z);
            if (upperType == PathNodeType::Walkable && _canStandOn(nx, y, nz)) {
                // 可以跳跃上去
                _addNeighbor(neighbors, nx, y + 1, nz, PathNodeType::Walkable);
            } else if (type == PathNodeType::DangerFall && currentType == PathNodeType::Walkable) {
                // 可以跌落
                i32 groundY = _getGroundHeight(nx, y, nz);
                if (groundY >= y - m_maxFallDistance) {
                    _addNeighbor(neighbors, nx, groundY, nz, PathNodeType::Walkable);
                }
            }
        } else if (type == PathNodeType::Blocked) {
            // 尝试跳上障碍物
            if (currentType == PathNodeType::Walkable && _canStandOn(nx, y, nz)) {
                PathNodeType upperType = getNodeType(nx, y + 1, nz);
                if (upperType == PathNodeType::Walkable || upperType == PathNodeType::Open) {
                    _addNeighbor(neighbors, nx, y + 1, nz, PathNodeType::Walkable);
                }
            }
        }
    }

    // 攀爬（梯子、藤蔓等）
    if (m_canClimb && currentType == PathNodeType::Climbable) {
        // 向上攀爬
        PathNodeType upperType = getNodeType(x, y + 1, z);
        if (upperType == PathNodeType::Climbable) {
            _addNeighbor(neighbors, x, y + 1, z, PathNodeType::Climbable);
        }
        // 向下攀爬
        PathNodeType lowerType = getNodeType(x, y - 1, z);
        if (lowerType == PathNodeType::Climbable) {
            _addNeighbor(neighbors, x, y - 1, z, PathNodeType::Climbable);
        }
    }

    // 水中移动
    if (currentType == PathNodeType::Water && m_canSwim) {
        // 向上游泳
        PathNodeType upperType = getNodeType(x, y + 1, z);
        if (upperType == PathNodeType::Water) {
            _addNeighbor(neighbors, x, y + 1, z, PathNodeType::Water);
        }
        // 向下游泳
        PathNodeType lowerType = getNodeType(x, y - 1, z);
        if (lowerType == PathNodeType::Water) {
            _addNeighbor(neighbors, x, y - 1, z, PathNodeType::Water);
        }
    }

    return neighbors;
}

std::unique_ptr<PathPoint> WalkNodeProcessor::createNode(i32 x, i32 y, i32 z)
{
    if (!m_region || !m_region->isLoaded(x, z)) {
        return nullptr;
    }

    PathNodeType type = getNodeType(x, y, z);

    if (type == PathNodeType::Blocked) {
        return nullptr;
    }

    auto node = std::make_unique<PathPoint>(x, y, z);
    node->setNodeType(type);

    // 优先使用 MobEntity 的 pathfinding malus（对应 MC Java WalkNodeEvaluator 中
    // node.costMalus = mob.getPathfindingMalus(type)），未设置时回退到默认代价
    // （getPathCostPenalty，等价于 MC Java 的 PathType.getMalus()）
    f32 costMalus = getPathCostPenalty(type);
    if (m_entity != nullptr) {
        const auto* mob = dynamic_cast<const MobEntity*>(m_entity);
        if (mob != nullptr) {
            costMalus = mob->getPathfindingMalus(type);
        }
    }
    node->setCostMalus(costMalus);

    return node;
}

bool WalkNodeProcessor::_isWalkableAt(i32 x, i32 y, i32 z) const
{
    if (!m_region) return false;

    // 检查位置本身是否可以站立
    return m_region->isWalkable(x, y, z);
}

bool WalkNodeProcessor::_canStandOn(i32 x, i32 y, i32 z) const
{
    if (!m_region) return false;

    // 检查脚下是否有支撑
    return m_region->isWalkable(x, y, z) && _isPassable(x, y + 1, z);
}

bool WalkNodeProcessor::_isSafe(i32 x, i32 y, i32 z) const
{
    if (!m_region) return false;

    // 检查位置本身和周围是否有危险
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

bool WalkNodeProcessor::isDangerous(i32 x, i32 y, i32 z) const
{
    if (!m_region) return false;

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
    // 接触会造成伤害和减速
    if (VanillaBlocks::SWEET_BERRY_BUSH != nullptr && &block == VanillaBlocks::SWEET_BERRY_BUSH) {
        return true;
    }

    return false;
}

i32 WalkNodeProcessor::_getGroundHeight(i32 x, i32 y, i32 z) const
{
    if (!m_region) return y;

    // 从指定位置向下搜索地面
    for (i32 checkY = y; checkY >= world::MIN_BUILD_HEIGHT; --checkY) {
        if (m_region->isWalkable(x, checkY, z)) {
            return checkY;
        }
    }

    return world::MIN_BUILD_HEIGHT;
}

bool WalkNodeProcessor::_isPassable(i32 x, i32 y, i32 z) const
{
    if (!m_region) return true;

    // 检查位置是否可以穿过（空气、水等）
    return !m_region->isWalkable(x, y, z) || m_region->isWater(x, y, z);
}

void WalkNodeProcessor::_addNeighbor(std::vector<PathPoint*>& neighbors, i32 x, i32 y, i32 z, PathNodeType type)
{
    PathPoint* node = getNode(x, y, z);
    if (node) {
        node->setNodeType(type);

        // 类型变更时同步 costMalus，对应 MC Java WalkNodeEvaluator.getNodeAndUpdateCostToMax
        // 的 node.costMalus = Math.max(node.costMalus, mob.getPathfindingMalus(type))
        // 取 max 语义保证：若节点已被更"昂贵"的类型标记过，不会因后续更"便宜"的类型
        // 覆盖而降低代价。这是 MC 原版的保守策略——一旦某节点被判定为危险（高代价），
        // 后续相邻检测中的其他类型判定不应削弱该危险标记。
        f32 newCostMalus = getPathCostPenalty(type);
        if (m_entity != nullptr) {
            const auto* mob = dynamic_cast<const MobEntity*>(m_entity);
            if (mob != nullptr) {
                newCostMalus = mob->getPathfindingMalus(type);
            }
        }
        node->setCostMalus(std::max(node->costMalus(), newCostMalus));

        neighbors.push_back(node);
        m_openNodes.push_back(node);
    }
}

void WalkNodeProcessor::_addJumpNeighbor(std::vector<PathPoint*>& neighbors, PathPoint* current, i32 dx, i32 dz)
{
    // 检查是否可以跳跃到相邻位置
    i32 x = current->x() + dx;
    i32 z = current->z() + dz;

    // 检查跳跃路径
    PathNodeType type1 = getNodeType(x, current->y(), z);
    PathNodeType type2 = getNodeType(x, current->y() + 1, z);

    if (type1 == PathNodeType::Walkable && type2 == PathNodeType::Open) {
        _addNeighbor(neighbors, x, current->y() + 1, z, PathNodeType::Walkable);
    }
}

void WalkNodeProcessor::_addFallNeighbor(std::vector<PathPoint*>& neighbors, i32 x, i32 startY, i32 z)
{
    // 检查是否可以跌落到相邻位置
    i32 groundY = _getGroundHeight(x, startY, z);
    if (groundY >= startY - m_maxFallDistance) {
        _addNeighbor(neighbors, x, groundY, z, PathNodeType::Walkable);
    }
}

} // namespace mc::entity::ai::pathfinding
