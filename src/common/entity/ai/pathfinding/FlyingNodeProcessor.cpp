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

#include "FlyingNodeProcessor.hpp"

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

#include <algorithm>
#include <cmath>

namespace mc::entity::ai::pathfinding {

namespace {

/// 飞行实体可穿越的节点类型。判定语义对应原版 FlyNodeEvaluator 中
/// `mob.getPathfindingMalus(type) >= 0` 放行的那一批。
[[nodiscard]] bool flyableNodeType(PathNodeType type)
{
    switch (type) {
        case PathNodeType::Open:
        case PathNodeType::Walkable:
        case PathNodeType::WalkableDoor:
        case PathNodeType::DoorOpen:
        case PathNodeType::Water:
        case PathNodeType::WaterBorder:
        case PathNodeType::Climbable:
        case PathNodeType::FenceGate:
        case PathNodeType::Breach:
        case PathNodeType::Leaves:
        case PathNodeType::Cocoa:
        case PathNodeType::Rail:
        case PathNodeType::Trapdoor:
        case PathNodeType::TrapdoorDown:
            return true;
        default:
            return false;
    }
}

/// 是否属于"接触即受伤"的危险类型（原版 DAMAGE_* 一族）。
[[nodiscard]] bool damagingNodeType(PathNodeType type)
{
    switch (type) {
        case PathNodeType::DamageFire:
        case PathNodeType::DamageCactus:
        case PathNodeType::DamageOther:
            return true;
        default:
            return false;
    }
}

} // namespace

// ============================================================================
// 起点
// ============================================================================

PathPoint* FlyingNodeProcessor::getStartNode(i32 x, i32 y, i32 z)
{
    // 对齐 vanilla FlyNodeEvaluator.getStart：取 floor(y + 0.5)。
    // 地面版会向下找地面，但飞行实体悬空时脚下可能是几十格之外的地面，
    // 拿它当起点会让整条路径从错误高度出发（蜜蜂在 y=6 飞行，地面在 y=0）。
    const i32 startY = y;

    PathPoint* node = getNode(x, startY, z);
    if (node != nullptr) {
        return node;
    }

    // 起点被判定为不可通行（实体半个身子埋在方块里、或被危险方块包住）：
    // 尝试包围盒四角候选点，取第一个可通行者。对应原版
    // iteratePathfindingStartNodeCandidatePositions + canStartAt 的循环。
    for (const auto& offset : {std::pair<i32, i32>{-1, -1}, {-1, 1}, {1, -1}, {1, 1}}) {
        const i32 cx = x + offset.first;
        const i32 cz = z + offset.second;
        if (!_isPassableForMob(getNodeType(cx, startY, cz))) {
            continue;
        }
        if (PathPoint* candidate = getNode(cx, startY, cz)) {
            return candidate;
        }
    }

    // 四角都不可通行：返回原位置节点（createNode 不可通行时返回 nullptr，
    // 上层据此判定"无起点"并放弃寻路，而不是拿到一个非法起点）。
    return getNode(x, startY, z);
}

// ============================================================================
// 邻居展开（26 方向）
// ============================================================================

std::vector<PathPoint*> FlyingNodeProcessor::getNeighbors(PathPoint* current)
{
    std::vector<PathPoint*> neighbors;
    neighbors.reserve(26);

    if (current == nullptr || !m_region) {
        return neighbors;
    }

    const i32 x = current->x();
    const i32 y = current->y();
    const i32 z = current->z();

    // 6 个正交方向。先取正交，后续斜向的合法性要用它们作为"直角边"。
    PathPoint* nodePosZ = _findAcceptedNode(x, y, z + 1);
    PathPoint* nodeNegX = _findAcceptedNode(x - 1, y, z);
    PathPoint* nodePosX = _findAcceptedNode(x + 1, y, z);
    PathPoint* nodeNegZ = _findAcceptedNode(x, y, z - 1);
    PathPoint* nodePosY = _findAcceptedNode(x, y + 1, z);
    PathPoint* nodeNegY = _findAcceptedNode(x, y - 1, z);

    for (PathPoint* node : {nodePosZ, nodeNegX, nodePosX, nodeNegZ, nodePosY, nodeNegY}) {
        if (node != nullptr) {
            neighbors.push_back(node);
        }
    }

    // 12 个面内斜向：要求组成它的两条直角边都可通行。
    PathPoint* nodePosYPosZ = _findAcceptedNode(x, y + 1, z + 1);
    if (nodePosYPosZ != nullptr && _hasMalus(nodePosZ) && _hasMalus(nodePosY)) {
        neighbors.push_back(nodePosYPosZ);
    }
    PathPoint* nodePosYNegX = _findAcceptedNode(x - 1, y + 1, z);
    if (nodePosYNegX != nullptr && _hasMalus(nodeNegX) && _hasMalus(nodePosY)) {
        neighbors.push_back(nodePosYNegX);
    }
    PathPoint* nodePosYPosX = _findAcceptedNode(x + 1, y + 1, z);
    if (nodePosYPosX != nullptr && _hasMalus(nodePosX) && _hasMalus(nodePosY)) {
        neighbors.push_back(nodePosYPosX);
    }
    PathPoint* nodePosYNegZ = _findAcceptedNode(x, y + 1, z - 1);
    if (nodePosYNegZ != nullptr && _hasMalus(nodeNegZ) && _hasMalus(nodePosY)) {
        neighbors.push_back(nodePosYNegZ);
    }
    PathPoint* nodeNegYPosZ = _findAcceptedNode(x, y - 1, z + 1);
    if (nodeNegYPosZ != nullptr && _hasMalus(nodePosZ) && _hasMalus(nodeNegY)) {
        neighbors.push_back(nodeNegYPosZ);
    }
    PathPoint* nodeNegYNegX = _findAcceptedNode(x - 1, y - 1, z);
    if (nodeNegYNegX != nullptr && _hasMalus(nodeNegX) && _hasMalus(nodeNegY)) {
        neighbors.push_back(nodeNegYNegX);
    }
    PathPoint* nodeNegYPosX = _findAcceptedNode(x + 1, y - 1, z);
    if (nodeNegYPosX != nullptr && _hasMalus(nodePosX) && _hasMalus(nodeNegY)) {
        neighbors.push_back(nodeNegYPosX);
    }
    PathPoint* nodeNegYNegZ = _findAcceptedNode(x, y - 1, z - 1);
    if (nodeNegYNegZ != nullptr && _hasMalus(nodeNegZ) && _hasMalus(nodeNegY)) {
        neighbors.push_back(nodeNegYNegZ);
    }
    PathPoint* nodePosXNegZ = _findAcceptedNode(x + 1, y, z - 1);
    if (nodePosXNegZ != nullptr && _hasMalus(nodeNegZ) && _hasMalus(nodePosX)) {
        neighbors.push_back(nodePosXNegZ);
    }
    PathPoint* nodePosXPosZ = _findAcceptedNode(x + 1, y, z + 1);
    if (nodePosXPosZ != nullptr && _hasMalus(nodePosZ) && _hasMalus(nodePosX)) {
        neighbors.push_back(nodePosXPosZ);
    }
    PathPoint* nodeNegXNegZ = _findAcceptedNode(x - 1, y, z - 1);
    if (nodeNegXNegZ != nullptr && _hasMalus(nodeNegZ) && _hasMalus(nodeNegX)) {
        neighbors.push_back(nodeNegXNegZ);
    }
    PathPoint* nodeNegXPosZ = _findAcceptedNode(x - 1, y, z + 1);
    if (nodeNegXPosZ != nullptr && _hasMalus(nodePosZ) && _hasMalus(nodeNegX)) {
        neighbors.push_back(nodeNegXPosZ);
    }

    // 8 个体对角：要求相关的三条直角边都可通行。
    PathPoint* nodePosXPosYNegZ = _findAcceptedNode(x + 1, y + 1, z - 1);
    if (nodePosXPosYNegZ != nullptr && _hasMalus(nodePosXNegZ) && _hasMalus(nodeNegZ) && _hasMalus(nodePosX) &&
        _hasMalus(nodePosY) && _hasMalus(nodePosYNegZ) && _hasMalus(nodePosYPosX)) {
        neighbors.push_back(nodePosXPosYNegZ);
    }
    PathPoint* nodePosXPosYPosZ = _findAcceptedNode(x + 1, y + 1, z + 1);
    if (nodePosXPosYPosZ != nullptr && _hasMalus(nodePosXPosZ) && _hasMalus(nodePosZ) && _hasMalus(nodePosX) &&
        _hasMalus(nodePosY) && _hasMalus(nodePosYPosZ) && _hasMalus(nodePosYPosX)) {
        neighbors.push_back(nodePosXPosYPosZ);
    }
    PathPoint* nodeNegXPosYNegZ = _findAcceptedNode(x - 1, y + 1, z - 1);
    if (nodeNegXPosYNegZ != nullptr && _hasMalus(nodeNegXNegZ) && _hasMalus(nodeNegZ) && _hasMalus(nodeNegX) &&
        _hasMalus(nodePosY) && _hasMalus(nodePosYNegZ) && _hasMalus(nodePosYNegX)) {
        neighbors.push_back(nodeNegXPosYNegZ);
    }
    PathPoint* nodeNegXPosYPosZ = _findAcceptedNode(x - 1, y + 1, z + 1);
    if (nodeNegXPosYPosZ != nullptr && _hasMalus(nodeNegXPosZ) && _hasMalus(nodePosZ) && _hasMalus(nodeNegX) &&
        _hasMalus(nodePosY) && _hasMalus(nodePosYPosZ) && _hasMalus(nodePosYNegX)) {
        neighbors.push_back(nodeNegXPosYPosZ);
    }
    PathPoint* nodePosXNegYNegZ = _findAcceptedNode(x + 1, y - 1, z - 1);
    if (nodePosXNegYNegZ != nullptr && _hasMalus(nodePosXNegZ) && _hasMalus(nodeNegZ) && _hasMalus(nodePosX) &&
        _hasMalus(nodeNegY) && _hasMalus(nodeNegYNegZ) && _hasMalus(nodeNegYPosX)) {
        neighbors.push_back(nodePosXNegYNegZ);
    }
    PathPoint* nodePosXNegYPosZ = _findAcceptedNode(x + 1, y - 1, z + 1);
    if (nodePosXNegYPosZ != nullptr && _hasMalus(nodePosXPosZ) && _hasMalus(nodePosZ) && _hasMalus(nodePosX) &&
        _hasMalus(nodeNegY) && _hasMalus(nodeNegYPosZ) && _hasMalus(nodeNegYPosX)) {
        neighbors.push_back(nodePosXNegYPosZ);
    }
    PathPoint* nodeNegXNegYNegZ = _findAcceptedNode(x - 1, y - 1, z - 1);
    if (nodeNegXNegYNegZ != nullptr && _hasMalus(nodeNegXNegZ) && _hasMalus(nodeNegZ) && _hasMalus(nodeNegX) &&
        _hasMalus(nodeNegY) && _hasMalus(nodeNegYNegZ) && _hasMalus(nodeNegYNegX)) {
        neighbors.push_back(nodeNegXNegYNegZ);
    }
    PathPoint* nodeNegXNegYPosZ = _findAcceptedNode(x - 1, y - 1, z + 1);
    if (nodeNegXNegYPosZ != nullptr && _hasMalus(nodeNegXPosZ) && _hasMalus(nodePosZ) && _hasMalus(nodeNegX) &&
        _hasMalus(nodeNegY) && _hasMalus(nodeNegYPosZ) && _hasMalus(nodeNegYNegX)) {
        neighbors.push_back(nodeNegXNegYPosZ);
    }

    return neighbors;
}

// ============================================================================
// 节点类型
// ============================================================================

PathNodeType FlyingNodeProcessor::getNodeType(i32 x, i32 y, i32 z)
{
    if (!m_region) {
        return PathNodeType::Blocked;
    }
    if (!m_region->isLoaded(x, z)) {
        return PathNodeType::Blocked;
    }

    // 先取方块本身的类型（含水/岩浆/火焰/仙人掌/门等细节判定）。
    PathNodeType type = PathNodeType::Open;
    if (m_region->isWater(x, y, z)) {
        type = PathNodeType::Water;
    } else if (m_region->isLava(x, y, z)) {
        type = PathNodeType::Lava;
    } else if (m_region->isWalkable(x, y, z)) {
        // 实心方块：飞行实体无法穿过。
        return PathNodeType::Blocked;
    } else {
        type = PathNodeType::Open;
    }

    // 对齐 vanilla FlyNodeEvaluator.getPathType：本格为 OPEN 且不是世界最低层时，
    // 依据**下方一格**折算类型。飞行实体不依赖地面支撑，所以"下方是实心"意味着
    // 它贴着地面飞，等价于地面版的 WALKABLE；下方是危险方块则继承危险语义。
    if (type == PathNodeType::Open && y > world::MIN_BUILD_HEIGHT) {
        const PathNodeType below = _cachedNodeType(x, y - 1, z);
        if (below == PathNodeType::DangerFire || below == PathNodeType::DamageFire || below == PathNodeType::Lava) {
            type = PathNodeType::DamageFire;
        } else if (below == PathNodeType::DangerOther || below == PathNodeType::DamageOther) {
            type = PathNodeType::DamageOther;
        } else if (below == PathNodeType::DangerCactus || below == PathNodeType::DamageCactus) {
            type = PathNodeType::DamageCactus;
        } else if (below == PathNodeType::Cocoa) {
            type = PathNodeType::Cocoa;
        } else if (below == PathNodeType::Fence) {
            // 栅栏是障碍：飞行实体不能悬停在栅栏正上方当作通道。但实体自己脚下那格除外，
            // 否则站在栅栏旁的飞行实体会把自己判成不可通行、永远无法起飞。
            const bool isOwnFootCell = m_entity != nullptr && static_cast<i32>(std::floor(m_entity->y())) == y;
            if (!isOwnFootCell) {
                type = PathNodeType::Fence;
            }
        } else if (below == PathNodeType::Blocked) {
            // 下方实心 → 沿地面飞，按 WALKABLE 计（会额外加 1 点代价，见 _findAcceptedNode）。
            type = PathNodeType::Walkable;
        } else {
            type = PathNodeType::Open;
        }
    }

    // 可通行类型再检查四邻是否贴危险方块（对齐 vanilla checkNeighbourBlocks）。
    if (type == PathNodeType::Walkable || type == PathNodeType::Open) {
        type = _checkNeighbourBlocks(x, y, z, type);
    }

    return type;
}

PathNodeType FlyingNodeProcessor::getNodeTypeWithEntity(i32 x, i32 y, i32 z)
{
    // 中心格本身不可通行，直接返回。
    const PathNodeType center = getNodeType(x, y, z);
    if (!flyableNodeType(center)) {
        return center;
    }

    // 实体体积较大时，包围盒可能横跨多格；只要其覆盖范围内有一格不可通行，
    // 就把中心格判为不可通行，避免飞行实体蹭着墙角穿过去。
    // 单格实体（width/height <= 1）走快路径，不做额外查询。
    const i32 spanX = static_cast<i32>(std::ceil(m_entityWidth - 0.01f));
    const i32 spanY = static_cast<i32>(std::ceil(m_entityHeight - 0.01f));
    if (spanX <= 1 && spanY <= 1) {
        return center;
    }

    for (i32 dy = 0; dy < std::max(1, spanY); ++dy) {
        for (i32 dx = 0; dx < std::max(1, spanX); ++dx) {
            for (i32 dz = 0; dz < std::max(1, spanX); ++dz) {
                if (dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }
                const PathNodeType neighbour = getNodeType(x + dx, y + dy, z + dz);
                if (!flyableNodeType(neighbour)) {
                    return neighbour;
                }
            }
        }
    }

    return center;
}

// ============================================================================
// 内部
// ============================================================================

void FlyingNodeProcessor::clear()
{
    m_pathTypeByPosCache.clear();
    WalkNodeProcessor::clear();
}

std::unique_ptr<PathPoint> FlyingNodeProcessor::createNode(i32 x, i32 y, i32 z)
{
    if (!m_region || !m_region->isLoaded(x, z)) {
        return nullptr;
    }

    const PathNodeType type = getNodeType(x, y, z);
    if (!flyableNodeType(type)) {
        return nullptr;
    }

    auto node = std::make_unique<PathPoint>(x, y, z);
    node->setNodeType(type);

    f32 costMalus = getPathCostPenalty(type);
    if (m_entity != nullptr) {
        if (const auto* mob = dynamic_cast<const MobEntity*>(m_entity)) {
            costMalus = mob->getPathfindingMalus(type);
        }
    }
    node->setCostMalus(costMalus);

    return node;
}

PathNodeType FlyingNodeProcessor::_cachedNodeType(i32 x, i32 y, i32 z)
{
    const u64 hash = makeHash(x, y, z);
    const auto it = m_pathTypeByPosCache.find(hash);
    if (it != m_pathTypeByPosCache.end()) {
        return it->second;
    }

    // 注意：这里必须绕过缓存重算一层，否则 getNodeType 内部对"下方一格"的
    // 递归查询会与缓存写入互相覆盖。用 WalkNodeProcessor 的方块级判定作为
    // 缓存源（它只查方块、不做邻居折算），返回值仅供上方"下方一格"折算使用。
    const PathNodeType type = WalkNodeProcessor::getNodeType(x, y, z);
    m_pathTypeByPosCache[hash] = type;
    return type;
}

bool FlyingNodeProcessor::_isPassableForMob(PathNodeType type) const
{
    if (!flyableNodeType(type)) {
        return false;
    }
    if (m_entity != nullptr) {
        if (const auto* mob = dynamic_cast<const MobEntity*>(m_entity)) {
            // 惩罚为负表示该类型对此实体完全不可通行（如蜜蜂对水设了 -1）。
            return mob->getPathfindingMalus(type) >= 0.0f;
        }
    }
    return true;
}

PathPoint* FlyingNodeProcessor::_findAcceptedNode(i32 x, i32 y, i32 z)
{
    const PathNodeType type = getNodeType(x, y, z);
    if (!_isPassableForMob(type)) {
        return nullptr;
    }

    PathPoint* node = getNode(x, y, z);
    if (node == nullptr) {
        return nullptr;
    }

    const f32 malus = [&] {
        if (const auto* mob = m_entity != nullptr ? dynamic_cast<const MobEntity*>(m_entity) : nullptr) {
            return mob->getPathfindingMalus(type);
        }
        return getPathCostPenalty(type);
    }();

    node->setNodeType(type);
    node->setCostMalus(std::max(node->costMalus(), malus));

    // 沿地面飞（WALKABLE）比空中飞行略贵，让寻路在有空隙时优先走空中捷径。
    // 对齐 vanilla FlyNodeEvaluator.findAcceptedNode：`if (pathtype == WALKABLE) node.costMalus++`。
    if (type == PathNodeType::Walkable) {
        node->setCostMalus(node->costMalus() + 1.0f);
    }

    return node;
}

bool FlyingNodeProcessor::_hasMalus(const PathPoint* node)
{
    return node != nullptr && node->costMalus() >= 0.0f;
}

PathNodeType FlyingNodeProcessor::_checkNeighbourBlocks(i32 x, i32 y, i32 z, PathNodeType type)
{
    // 对齐 vanilla NodeEvaluator.checkNeighbourBlocks：水平四邻 + 上方一格，
    // 任一为伤害型方块就把本格升级为对应 DAMAGE_* 类型，使寻路主动绕开。
    // 竖直方向（正上与正下）不参与：正下方已在上面折算过，正上方不会造成接触伤害。
    static constexpr i32 kOffsets[][3] = {
        {1, 0, 0},
        {-1, 0, 0},
        {0, 0, 1},
        {0, 0, -1},
        {0, 1, 0},
    };

    for (const auto& offset : kOffsets) {
        const i32 nx = x + offset[0];
        const i32 ny = y + offset[1];
        const i32 nz = z + offset[2];
        if (!m_region->isLoaded(nx, nz)) {
            continue;
        }

        PathNodeType neighbour = PathNodeType::Open;
        if (m_region->isWater(nx, ny, nz)) {
            neighbour = PathNodeType::Water;
        } else if (m_region->isLava(nx, ny, nz)) {
            neighbour = PathNodeType::Lava;
        } else if (m_region->isWalkable(nx, ny, nz)) {
            // 实心方块本身不造成伤害；具体危险方块（火焰/仙人掌）由下面的方块判定覆盖。
            neighbour = PathNodeType::Blocked;
        }

        if (neighbour == PathNodeType::Lava) {
            return PathNodeType::DamageFire;
        }
    }

    // 火焰/仙人掌/甜浆果这类"非实心但会造成伤害"的方块，isWalkable 判不出来，
    // 需要查方块本体。这里复用 WalkNodeProcessor 的方块级判定。
    for (const auto& offset : kOffsets) {
        const i32 nx = x + offset[0];
        const i32 ny = y + offset[1];
        const i32 nz = z + offset[2];
        if (!m_region->isLoaded(nx, nz)) {
            continue;
        }
        const PathNodeType neighbour = WalkNodeProcessor::getNodeType(nx, ny, nz);
        if (damagingNodeType(neighbour) || neighbour == PathNodeType::DangerFire ||
            neighbour == PathNodeType::DangerCactus || neighbour == PathNodeType::DangerOther) {
            return neighbour;
        }
    }

    return type;
}

} // namespace mc::entity::ai::pathfinding
