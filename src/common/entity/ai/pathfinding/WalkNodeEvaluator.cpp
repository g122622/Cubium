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

#include "WalkNodeEvaluator.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/DoorBlock.hpp"
#include "common/world/block/blocks/FenceGateBlock.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
// 三种铁轨（普通 / 动力 / 探测）都继承自 AbstractRailBlock，必须按基类判定：
// 原版同样用 `block instanceof BaseRailBlock` 覆盖全部铁轨。
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "common/world/block/blocks/vegetation/LeavesBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace mc::entity::ai::pathfinding {
namespace {

/**
 * @brief 水平方向的步进向量，顺序与原版 `Direction.Plane.HORIZONTAL` 一致
 *
 * 原版顺序是 NORTH、EAST、SOUTH、WEST，即 [-Z] [+X] [+Z] [-X]。邻居写入缓冲区的
 * **顺序**会改变 A* 的展开顺序，进而影响 `f` 相等时的取舍，因此这里的顺序必须与原版
 * 相同，不能随手挑选。
 */
constexpr i32 HORIZONTAL_STEP_X[4] = {0, 1, 0, -1};
constexpr i32 HORIZONTAL_STEP_Z[4] = {-1, 0, 1, 0};

/// 方向 2D 序号：N=0、E=1、S=2、W=3；顺时针即 +1 mod 4（原版 `Direction.getClockWise`）。
constexpr i32 clockwise(i32 direction) noexcept
{
    return (direction + 1) & 3;
}

/**
 * @brief 把包围盒的六个 f32 分量折叠成一个 64 位缓存键
 *
 * 只用于「同一批查询里相同的盒子复用结果」。键必须能区分不同的盒子，因此用位模式而不是
 * 量化坐标，避免把两个不同的盒子当成同一个而返回错的碰撞结果。
 */
[[nodiscard]] u64 collisionKey(const AxisAlignedBB& box) noexcept
{
    const f32 parts[6] = {box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ};
    u64 hash = 0xCBF29CE484222325ULL;
    for (const f32 part : parts) {
        const u32 bits = static_cast<u32>(std::bit_cast<u32>(part));
        hash ^= static_cast<u64>(bits);
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

/// 流体状态是否属于某个流体标签（流体标签的成员是流体对象，不是方块）。
[[nodiscard]] bool fluidInTag(const fluid::FluidState* state, fluid::FluidTag& tag)
{
    return state != nullptr && !state->isEmpty() && tag.contains(state->getFluid());
}

/// 碰撞形状在方块本地 Y 方向上的最高点（0~1）。
[[nodiscard]] f32 shapeMaxY(const CollisionShape& shape)
{
    f32 maxY = 0.0f;
    for (const AxisAlignedBB& box : shape.boxes()) {
        maxY = std::max(maxY, box.maxY);
    }
    return maxY;
}

/**
 * @brief 实体能否从该格穿行（即「可寻路」，与「可碰撞」不是一回事）
 *
 * 与寻路相关的是「这格能不能让身体过去」而不是「这格有没有碰撞」：空气、流体、
 * 无碰撞形状的方块（花、草、火把）都能穿行。门 / 铁轨 / 树叶 / 栅栏这些细分类型
 * 在调用本函数之前已经判过，所以这里只需问方块自己「陆地计算类型下让不让过」——
 * 雪层这类「碰撞非空但层数低就能踩过去」的方块由它们各自覆写回答。
 */
[[nodiscard]] bool isPathfindable(const BlockState& state)
{
    if (state.isAir()) {
        return true;
    }
    return state.getBlock().isPathfindable(state, PathComputationType::Land);
}

} // namespace

// ===========================================================================
// 会话
// ===========================================================================

void WalkNodeEvaluator::prepare(const Region& region, const PathfindingEntity& entity, const Vector3d& entityPosition)
{
    NodeEvaluator::prepare(region, entity, entityPosition);
    m_pathTypeCache.clear();
    m_collisionCache.clear();
    m_reusableNeighbors.fill(nullptr);
}

void WalkNodeEvaluator::done()
{
    m_pathTypeCache.clear();
    m_collisionCache.clear();
    m_reusableNeighbors.fill(nullptr);
    NodeEvaluator::done();
}

// ===========================================================================
// 单格类型：getPathTypeFromState / getPathTypeStatic / checkNeighbourBlocks
// ===========================================================================

bool WalkNodeEvaluator::isBurningBlock(const BlockState& state)
{
    const Block& block = state.getBlock();
    if (BlockTags::FIRE().contains(&block) || &block == VanillaBlocks::LAVA || &block == VanillaBlocks::MAGMA) {
        return true;
    }
    if ((&block == VanillaBlocks::CAMPFIRE || &block == VanillaBlocks::SOUL_CAMPFIRE) &&
        blocks::CampfireBlock::isLit(state)) {
        return true;
    }
    return &block == VanillaBlocks::LAVA_CAULDRON;
}

PathType WalkNodeEvaluator::getPathTypeFromState(const Region& region, i32 x, i32 y, i32 z)
{
    const BlockState* state = region.getBlockState(x, y, z);
    if (state->isAir()) {
        return PathType::Open;
    }
    const Block& block = state->getBlock();
    if (BlockTags::TRAPDOORS().contains(&block) || &block == VanillaBlocks::LILY_PAD ||
        &block == VanillaBlocks::BIG_DRIPLEAF) {
        return PathType::Trapdoor;
    }
    if (&block == VanillaBlocks::POWDER_SNOW) {
        return PathType::PowderSnow;
    }
    if (&block == VanillaBlocks::CACTUS || &block == VanillaBlocks::SWEET_BERRY_BUSH) {
        return PathType::DamageOther;
    }
    if (&block == VanillaBlocks::HONEY_BLOCK) {
        return PathType::StickyHoney;
    }
    if (&block == VanillaBlocks::COCOA) {
        return PathType::Cocoa;
    }
    if (&block == VanillaBlocks::WITHER_ROSE || &block == VanillaBlocks::POINTED_DRIPSTONE) {
        return PathType::DamageCautious;
    }

    const fluid::FluidState* fluidState = region.getFluidState(x, y, z);
    if (fluidInTag(fluidState, fluid::FluidTags::LAVA())) {
        return PathType::Lava;
    }
    if (isBurningBlock(*state)) {
        return PathType::DamageFire;
    }
    if (const auto* door = dynamic_cast<const blocks::DoorBlock*>(&block); door != nullptr) {
        if (blocks::DoorBlock::isOpen(*state)) {
            return PathType::DoorOpen;
        }
        return door->isIronDoor() ? PathType::DoorIronClosed : PathType::DoorWoodClosed;
    }
    if (dynamic_cast<const blocks::AbstractRailBlock*>(&block) != nullptr) {
        return PathType::Rail;
    }
    if (dynamic_cast<const blocks::LeavesBlock*>(&block) != nullptr) {
        return PathType::Leaves;
    }
    const auto* gate = dynamic_cast<const blocks::FenceGateBlock*>(&block);
    const bool openGate = gate != nullptr && blocks::FenceGateBlock::isOpen(*state);
    if (!BlockTags::FENCES().contains(&block) && !BlockTags::WALLS().contains(&block) &&
        (gate == nullptr || openGate)) {
        if (!isPathfindable(*state)) {
            return PathType::Blocked;
        }
        return fluidInTag(fluidState, fluid::FluidTags::WATER()) ? PathType::Water : PathType::Open;
    }
    return PathType::Fence;
}

PathType WalkNodeEvaluator::checkNeighbourBlocks(const Region& region, i32 x, i32 y, i32 z, PathType fallback)
{
    // 循环顺序与「同一邻域里有多种危险时先返回哪个」都是原版语义的一部分。
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dz == 0) {
                    continue;
                }
                const PathType type = getPathTypeFromState(region, x + dx, y + dy, z + dz);
                if (type == PathType::DamageOther) {
                    return PathType::DangerOther;
                }
                if (type == PathType::DamageFire || type == PathType::Lava) {
                    return PathType::DangerFire;
                }
                if (type == PathType::Water) {
                    return PathType::WaterBorder;
                }
                if (type == PathType::DamageCautious) {
                    return PathType::DamageCautious;
                }
            }
        }
    }
    return fallback;
}

PathType WalkNodeEvaluator::getPathTypeStatic(const Region& region, i32 x, i32 y, i32 z)
{
    const PathType type = getPathTypeFromState(region, x, y, z);
    if (type != PathType::Open || y < region.minBuildHeight() + 1) {
        return type;
    }
    switch (getPathTypeFromState(region, x, y - 1, z)) {
        case PathType::Open:
        case PathType::Water:
        case PathType::Lava:
        case PathType::Walkable:
            return PathType::Open;
        case PathType::DamageFire:
            return PathType::DamageFire;
        case PathType::DamageOther:
            return PathType::DamageOther;
        case PathType::StickyHoney:
            return PathType::StickyHoney;
        case PathType::PowderSnow:
            return PathType::DangerPowderSnow;
        case PathType::DamageCautious:
            return PathType::DamageCautious;
        case PathType::Trapdoor:
            return PathType::DangerTrapdoor;
        default:
            return checkNeighbourBlocks(region, x, y, z, PathType::Walkable);
    }
}

// ===========================================================================
// 地面高度
// ===========================================================================

f64 WalkNodeEvaluator::getFloorLevel(const Region& region, i32 x, i32 y, i32 z)
{
    const i32 belowY = y - 1;
    const BlockState* state = region.getBlockState(x, belowY, z);
    const CollisionShape& shape = state->getCollisionShape();
    const f64 top = shape.isEmpty() ? 0.0 : static_cast<f64>(shapeMaxY(shape));
    return static_cast<f64>(belowY) + top;
}

f64 WalkNodeEvaluator::getFloorLevel(const BlockPos& pos) const
{
    const Region& region = m_context->region();
    if ((canFloat() || isAmphibious()) &&
        fluidInTag(region.getFluidState(pos.x, pos.y, pos.z), fluid::FluidTags::WATER())) {
        return static_cast<f64>(pos.y) + 0.5;
    }
    return getFloorLevel(region, pos.x, pos.y, pos.z);
}

f64 WalkNodeEvaluator::mobJumpHeight() const
{
    return std::max(DEFAULT_MOB_JUMP_HEIGHT, static_cast<f64>(m_context->entity().pathfindingStepHeight()));
}

// ===========================================================================
// 含实体尺寸的类型：getPathTypeOfMob / getPathTypeWithinMobBB
// ===========================================================================

PathType WalkNodeEvaluator::getPathType(const PathfindingContext& context, i32 x, i32 y, i32 z) const
{
    return getPathTypeStatic(context.region(), x, y, z);
}

std::set<PathType> WalkNodeEvaluator::pathTypeWithinMobBb(const PathfindingContext& context, i32 x, i32 y, i32 z) const
{
    // 必须按枚举次序升序遍历：同代价时「后写入者胜出」，而原版用的是 EnumSet（按序号升序），
    // 遍历次序直接决定 OPEN 与 WALKABLE 这类同代价类型之间谁赢。
    std::set<PathType> types;
    const BlockPos entityPos = context.entityBlockPos();
    const PathType entityType = getPathType(context, entityPos.x, entityPos.y, entityPos.z);
    const PathType entityTypeBelow = getPathType(context, entityPos.x, entityPos.y - 1, entityPos.z);

    for (i32 offsetX = 0; offsetX < entityWidthCells(); ++offsetX) {
        for (i32 offsetY = 0; offsetY < entityHeightCells(); ++offsetY) {
            for (i32 offsetZ = 0; offsetZ < entityDepthCells(); ++offsetZ) {
                PathType type = getPathType(context, x + offsetX, y + offsetY, z + offsetZ);
                const bool passDoors = canPassDoors();
                if (type == PathType::DoorWoodClosed && canOpenDoors() && passDoors) {
                    type = PathType::WalkableDoor;
                }
                if (type == PathType::DoorOpen && !passDoors) {
                    type = PathType::Blocked;
                }
                // 铁轨只有与实体当前脚下（或其下一格）对齐时才算可上下车，否则视为不可通行。
                if (type == PathType::Rail && entityType != PathType::Rail && entityTypeBelow != PathType::Rail) {
                    type = PathType::UnpassableRail;
                }
                types.insert(type);
            }
        }
    }
    return types;
}

PathType WalkNodeEvaluator::getPathTypeOfMob(const PathfindingContext& context, i32 x, i32 y, i32 z) const
{
    const std::set<PathType> types = pathTypeWithinMobBb(context, x, y, z);
    if (types.contains(PathType::Fence)) {
        return PathType::Fence;
    }
    if (types.contains(PathType::UnpassableRail)) {
        return PathType::UnpassableRail;
    }

    const PathfindingEntity& entity = context.entity();
    PathType best = PathType::Blocked;
    for (const PathType type : types) {
        if (entity.pathfindingMalus(type) < 0.0f) {
            return type;
        }
        if (entity.pathfindingMalus(type) >= entity.pathfindingMalus(best)) {
            best = type;
        }
    }
    // 窄实体（宽 <= 1 格）站在一块可走的方块上、而该格本身是空气时按空气处理：
    // 否则「站在台阶边缘」会被误判成需要抬脚的实心格。
    // 注意这个改写只适用于「空气 / 空着的地形」这类无碰撞类型；WALKABLE 虽然代价也是 0，
    // 但它代表脚下踩实，改写成 OPEN 会让实体永远找不到可以起步的落脚点。
    if (entityWidthCells() <= 1 && (best == PathType::Trapdoor || best == PathType::PowderSnow) &&
        entity.pathfindingMalus(best) == 0.0f && getPathType(context, x, y, z) == PathType::Open) {
        return PathType::Open;
    }
    return best;
}

PathType WalkNodeEvaluator::cachedPathType(i32 x, i32 y, i32 z)
{
    const u64 key = nodeKey(x, y, z);
    const auto found = m_pathTypeCache.find(key);
    if (found != m_pathTypeCache.end()) {
        return found->second;
    }
    const PathType type = getPathTypeOfMob(*m_context, x, y, z);
    m_pathTypeCache.emplace(key, type);
    return type;
}

bool WalkNodeEvaluator::hasCollisions(const AxisAlignedBB& box)
{
    const u64 key = collisionKey(box);
    const auto found = m_collisionCache.find(key);
    if (found != m_collisionCache.end()) {
        return found->second;
    }
    const bool collides = m_context->region().hasCollision(&m_context->entity(), box);
    m_collisionCache.emplace(key, collides);
    return collides;
}

// ===========================================================================
// 起点
// ===========================================================================

bool WalkNodeEvaluator::canStartAt(i32 x, i32 y, i32 z) const
{
    const PathType type = const_cast<WalkNodeEvaluator*>(this)->cachedPathType(x, y, z);
    return type != PathType::Open && m_context->entity().pathfindingMalus(type) >= 0.0f;
}

PathPoint* WalkNodeEvaluator::getStartNode(i32 x, i32 y, i32 z)
{
    PathPoint* node = getNode(x, y, z);
    node->setType(cachedPathType(x, y, z));
    node->setCostMalus(m_context->entity().pathfindingMalus(node->type()));
    return node;
}

PathPoint* WalkNodeEvaluator::getStart()
{
    const PathfindingContext& context = *m_context;
    const Region& region = context.region();
    const Vector3d& position = context.entityPosition();

    i32 y = static_cast<i32>(std::floor(position.y));
    const i32 blockX = static_cast<i32>(std::floor(position.x));
    const i32 blockZ = static_cast<i32>(std::floor(position.z));
    const BlockState* state = region.getBlockState(blockX, y, blockZ);

    // 起点 Y 的推导分三种情况：
    //   1. 站在水里且能漂浮：向上爬出水柱；
    //   2. 站在地面上：取脚下 0.5 格的位置（避免半砖 / 台阶把起点抬高半格）；
    //   3. 悬空：先向下找到第一个「不可穿过」的方块，再在其上方一格作为起点。
    if (!context.entity().pathfindingCanStandOnFluid(region.getFluidState(blockX, y, blockZ))) {
        if (canFloat() && context.entity().pathfindingInWater()) {
            while (fluidInTag(region.getFluidState(blockX, y, blockZ), fluid::FluidTags::WATER())) {
                ++y;
            }
            --y;
        } else if (context.entity().pathfindingOnGround()) {
            y = static_cast<i32>(std::floor(position.y + 0.5));
        } else {
            ++y;
            while (y > region.minBuildHeight()) {
                --y;
                state = region.getBlockState(blockX, y, blockZ);
                if (!state->isAir() && !isPathfindable(*state)) {
                    break;
                }
                ++y;
            }
        }
    } else {
        while (context.entity().pathfindingCanStandOnFluid(region.getFluidState(blockX, y, blockZ))) {
            ++y;
        }
        --y;
    }

    const BlockPos entityPos = context.entityBlockPos();
    if (!canStartAt(entityPos.x, y, entityPos.z)) {
        // 起点那一列不可用：尝试实体包围盒的四个角，任一可用就取该列。
        const AxisAlignedBB box = context.entity().pathfindingBoundingBox();
        const f32 corners[4][2] = {
            {box.minX, box.minZ}, {box.minX, box.maxZ}, {box.maxX, box.minZ}, {box.maxX, box.maxZ}};
        for (const auto& corner : corners) {
            const i32 cornerX = static_cast<i32>(std::floor(corner[0]));
            const i32 cornerZ = static_cast<i32>(std::floor(corner[1]));
            if (canStartAt(cornerX, y, cornerZ)) {
                return getStartNode(cornerX, y, cornerZ);
            }
        }
    }
    return getStartNode(entityPos.x, y, entityPos.z);
}

Target WalkNodeEvaluator::getTarget(f64 x, f64 y, f64 z)
{
    return _getTargetNodeAt(x, y, z);
}

// ===========================================================================
// 邻居
// ===========================================================================

i32 WalkNodeEvaluator::getNeighbors(PathPoint** neighbors, PathPoint* point)
{
    const PathType aboveType = cachedPathType(point->x(), point->y() + 1, point->z());
    const PathType selfType = cachedPathType(point->x(), point->y(), point->z());

    // 头顶可站、且脚下不是蜂蜜块时，才允许往上跳。
    i32 jumpHeight = 0;
    if (m_context->entity().pathfindingMalus(aboveType) >= 0.0f && selfType != PathType::StickyHoney) {
        jumpHeight = static_cast<i32>(std::floor(std::max(1.0f, m_context->entity().pathfindingStepHeight())));
    }
    const f64 floorLevel = getFloorLevel(BlockPos(point->x(), point->y(), point->z()));
    return collectNeighbors(point, jumpHeight, floorLevel, selfType, neighbors);
}

i32 WalkNodeEvaluator::collectNeighbors(
    PathPoint* point, i32 jumpHeight, f64 floorLevel, PathType currentType, PathPoint** neighbors)
{
    i32 count = 0;
    for (i32 direction = 0; direction < 4; ++direction) {
        PathPoint* neighbor = findAcceptedNode(point->x() + HORIZONTAL_STEP_X[direction],
            point->y(),
            point->z() + HORIZONTAL_STEP_Z[direction],
            jumpHeight,
            floorLevel,
            HORIZONTAL_STEP_X[direction],
            HORIZONTAL_STEP_Z[direction],
            currentType);
        m_reusableNeighbors[static_cast<size_t>(direction)] = neighbor;
        if (isNeighborValid(neighbor, *point)) {
            neighbors[count++] = neighbor;
        }
    }

    // 对角邻居：两个正交邻居必须都有效，且对角本身也有效。
    for (i32 direction = 0; direction < 4; ++direction) {
        const i32 sideA = direction;
        const i32 sideB = clockwise(direction);
        if (!isDiagonalValid(*point,
                m_reusableNeighbors[static_cast<size_t>(sideA)],
                m_reusableNeighbors[static_cast<size_t>(sideB)])) {
            continue;
        }
        PathPoint* neighbor = findAcceptedNode(point->x() + HORIZONTAL_STEP_X[sideA] + HORIZONTAL_STEP_X[sideB],
            point->y(),
            point->z() + HORIZONTAL_STEP_Z[sideA] + HORIZONTAL_STEP_Z[sideB],
            jumpHeight,
            floorLevel,
            HORIZONTAL_STEP_X[sideA],
            HORIZONTAL_STEP_Z[sideA],
            currentType);
        if (isDiagonalValid(neighbor)) {
            neighbors[count++] = neighbor;
        }
    }
    return count;
}

bool WalkNodeEvaluator::isNeighborValid(const PathPoint* neighbor, const PathPoint& point) const
{
    // 自节点本身不可走时，允许邻居把路径「引回」可走区域，否则会卡死在负代价格里。
    return neighbor != nullptr && !neighbor->closed() && (neighbor->costMalus() >= 0.0f || point.costMalus() < 0.0f);
}

bool WalkNodeEvaluator::isDiagonalValid(const PathPoint& point, const PathPoint* sideA, const PathPoint* sideB) const
{
    if (sideB == nullptr || sideA == nullptr || sideB->y() > point.y() || sideA->y() > point.y()) {
        return false;
    }
    if (sideA->type() == PathType::WalkableDoor || sideB->type() == PathType::WalkableDoor) {
        return false;
    }
    // 超窄实体可以穿过两根栅栏柱之间的缝（栅栏碰撞盒只有 0.125 格宽）。
    const bool squeezeBetweenFences = sideB->type() == PathType::Fence && sideA->type() == PathType::Fence &&
        m_context->entity().pathfindingWidth() < 0.5f;
    return (sideB->y() < point.y() || sideB->costMalus() >= 0.0f || squeezeBetweenFences) &&
        (sideA->y() < point.y() || sideA->costMalus() >= 0.0f || squeezeBetweenFences);
}

bool WalkNodeEvaluator::isDiagonalValid(const PathPoint* diagonal) const
{
    if (diagonal == nullptr || diagonal->closed()) {
        return false;
    }
    return diagonal->type() != PathType::WalkableDoor && diagonal->costMalus() >= 0.0f;
}

PathPoint* WalkNodeEvaluator::nodeAndUpdateCostToMax(i32 x, i32 y, i32 z, PathType type, f32 malus)
{
    PathPoint* node = getNode(x, y, z);
    node->setType(type);
    node->setCostMalus(std::max(node->costMalus(), malus));
    return node;
}

PathPoint* WalkNodeEvaluator::blockedNode(i32 x, i32 y, i32 z)
{
    PathPoint* node = getNode(x, y, z);
    node->setType(PathType::Blocked);
    node->setCostMalus(-1.0f);
    return node;
}

PathPoint* WalkNodeEvaluator::closedNode(i32 x, i32 y, i32 z, PathType type)
{
    PathPoint* node = getNode(x, y, z);
    node->setClosed(true);
    node->setType(type);
    node->setCostMalus(getPathMalus(type));
    return node;
}

bool WalkNodeEvaluator::canReachWithoutCollision(const PathPoint& point)
{
    const AxisAlignedBB& entityBox = m_context->entity().pathfindingBoundingBox();
    Vector3d delta(point.x() - m_context->entityPosition().x + entityBox.width() / 2.0,
        point.y() - m_context->entityPosition().y + entityBox.height() / 2.0,
        point.z() - m_context->entityPosition().z + entityBox.depth() / 2.0);
    const f64 boxSize = std::max({entityBox.width(), entityBox.height(), entityBox.depth()});
    const i32 steps = static_cast<i32>(std::ceil(delta.length() / boxSize));
    if (steps <= 0) {
        return true;
    }
    delta = delta * (1.0 / static_cast<f64>(steps));

    AxisAlignedBB box = entityBox;
    for (i32 step = 1; step <= steps; ++step) {
        box = box.offsetted(static_cast<f32>(delta.x), static_cast<f32>(delta.y), static_cast<f32>(delta.z));
        if (hasCollisions(box)) {
            return false;
        }
    }
    return true;
}

PathPoint* WalkNodeEvaluator::findAcceptedNode(
    i32 x, i32 y, i32 z, i32 jumpHeight, f64 floorLevel, i32 stepX, i32 stepZ, PathType neighborType)
{
    PathPoint* node = nullptr;
    const f64 candidateFloor = getFloorLevel(BlockPos(x, y, z));
    if (candidateFloor - floorLevel > mobJumpHeight()) {
        return nullptr;
    }

    const PathType type = cachedPathType(x, y, z);
    const f32 malus = m_context->entity().pathfindingMalus(type);
    if (malus >= 0.0f) {
        node = nodeAndUpdateCostToMax(x, y, z, type, malus);
    }

    // 从栅栏 / 关闭的门所在格出发时，如果实体实际上够不过去，就不允许进入该格：
    // 否则路径会「穿栅栏而过」。
    if (doesBlockHavePartialCollision(neighborType) && node != nullptr && node->costMalus() >= 0.0f &&
        !canReachWithoutCollision(*node)) {
        node = nullptr;
    }

    if (type == PathType::Walkable || (isAmphibious() && type == PathType::Water)) {
        return node;
    }

    if ((node == nullptr || node->costMalus() < 0.0f) && jumpHeight > 0 && type != PathType::Fence &&
        (!isAmphibious() || type != PathType::Water) && type != PathType::UnpassableRail &&
        type != PathType::Trapdoor && type != PathType::PowderSnow) {
        return tryJumpOn(x, y, z, jumpHeight, floorLevel, stepX, stepZ, neighborType);
    }
    if (!isAmphibious() && type == PathType::Water && !canFloat()) {
        return tryFindFirstNonWaterBelow(x, y, z, node);
    }
    if (type == PathType::Open) {
        return tryFindFirstGroundNodeBelow(x, y, z);
    }
    if (doesBlockHavePartialCollision(type) && node == nullptr) {
        return closedNode(x, y, z, type);
    }
    return node;
}

PathPoint* WalkNodeEvaluator::tryJumpOn(
    i32 x, i32 y, i32 z, i32 jumpHeight, f64 floorLevel, i32 stepX, i32 stepZ, PathType neighborType)
{
    PathPoint* node = findAcceptedNode(x, y + 1, z, jumpHeight - 1, floorLevel, stepX, stepZ, neighborType);
    if (node == nullptr) {
        return nullptr;
    }
    if (m_context->entity().pathfindingWidth() >= 1.0f) {
        return node;
    }
    if (node->type() != PathType::Open && node->type() != PathType::Walkable) {
        return node;
    }
    // 窄实体往侧上方走：判定它从当前格跨到目标格上方时不会撞到中间的门框 / 墙角。
    const f64 centreX = static_cast<f64>(x - stepX) + 0.5;
    const f64 centreZ = static_cast<f64>(z - stepZ) + 0.5;
    const f64 half = static_cast<f64>(m_context->entity().pathfindingWidth()) / 2.0;
    const f64 bottom =
        getFloorLevel(BlockPos(static_cast<i32>(std::floor(centreX)), y + 1, static_cast<i32>(std::floor(centreZ)))) +
        0.001;
    const f64 top = static_cast<f64>(m_context->entity().pathfindingHeight()) +
        getFloorLevel(BlockPos(node->x(), node->y(), node->z())) - 0.002;
    const AxisAlignedBB box(static_cast<f32>(centreX - half),
        static_cast<f32>(bottom),
        static_cast<f32>(centreZ - half),
        static_cast<f32>(centreX + half),
        static_cast<f32>(top),
        static_cast<f32>(centreZ + half));
    return hasCollisions(box) ? nullptr : node;
}

PathPoint* WalkNodeEvaluator::tryFindFirstNonWaterBelow(i32 x, i32 y, i32 z, PathPoint* fallback)
{
    --y;
    while (y > m_context->region().minBuildHeight()) {
        const PathType type = cachedPathType(x, y, z);
        if (type != PathType::Water) {
            return fallback;
        }
        fallback = nodeAndUpdateCostToMax(x, y, z, type, m_context->entity().pathfindingMalus(type));
        --y;
    }
    return fallback;
}

PathPoint* WalkNodeEvaluator::tryFindFirstGroundNodeBelow(i32 x, i32 y, i32 z)
{
    for (i32 scanY = y - 1; scanY >= m_context->region().minBuildHeight(); --scanY) {
        // 允许下落的最大距离由实体决定（目标、血量、难度共同决定），不是常数 3。
        if (y - scanY > m_context->entity().pathfindingMaxFallDistance()) {
            return blockedNode(x, scanY, z);
        }
        const PathType type = cachedPathType(x, scanY, z);
        const f32 malus = m_context->entity().pathfindingMalus(type);
        if (type == PathType::Open) {
            continue;
        }
        if (malus >= 0.0f) {
            return nodeAndUpdateCostToMax(x, scanY, z, type, malus);
        }
        return blockedNode(x, scanY, z);
    }
    return blockedNode(x, y, z);
}

} // namespace mc::entity::ai::pathfinding
