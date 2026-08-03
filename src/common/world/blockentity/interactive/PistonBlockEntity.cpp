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

#include "PistonBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/MoverType.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"

#include <algorithm>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 活塞推动实体时用于避免粘连的微小额外位移。
 */
constexpr f32 PISTON_PUSH_EPSILON = 0.01f;

/**
 * @brief 计算活塞在本次 tick 的推进距离。
 */
[[nodiscard]] f32 computeMoveDistance(f32 previousProgress, f32 nextProgress)
{
    return std::max(0.0f, nextProgress - previousProgress);
}

/**
 * @brief 计算方向向量。
 */
[[nodiscard]] Vector3 toDirectionVector(Direction direction)
{
    return Vector3(static_cast<f32>(Directions::xOffset(direction)),
        static_cast<f32>(Directions::yOffset(direction)),
        static_cast<f32>(Directions::zOffset(direction)));
}

/**
 * @brief 获取本次可能碰撞到实体的扫描包围盒。
 */
[[nodiscard]] AxisAlignedBB buildSweepBox(const AxisAlignedBB& pistonBox, Direction direction, f32 moveDistance)
{
    if (moveDistance <= 0.0f) {
        return pistonBox;
    }

    AxisAlignedBB sweep = pistonBox;
    const f32 dx = static_cast<f32>(Directions::xOffset(direction)) * moveDistance;
    const f32 dy = static_cast<f32>(Directions::yOffset(direction)) * moveDistance;
    const f32 dz = static_cast<f32>(Directions::zOffset(direction)) * moveDistance;

    if (dx > 0.0f) {
        sweep.maxX += dx;
    } else {
        sweep.minX += dx;
    }

    if (dy > 0.0f) {
        sweep.maxY += dy;
    } else {
        sweep.minY += dy;
    }

    if (dz > 0.0f) {
        sweep.maxZ += dz;
    } else {
        sweep.minZ += dz;
    }

    return sweep;
}

/**
 * @brief 计算实体在指定方向上需要的最小位移以脱离碰撞。
 */
[[nodiscard]] f32 getMovement(const AxisAlignedBB& pistonBox, Direction direction, const AxisAlignedBB& entityBox)
{
    switch (direction) {
        case Direction::Down:
            return pistonBox.minY - entityBox.maxY;
        case Direction::Up:
            return entityBox.minY - pistonBox.maxY;
        case Direction::North:
            return pistonBox.minZ - entityBox.maxZ;
        case Direction::South:
            return entityBox.minZ - pistonBox.maxZ;
        case Direction::West:
            return pistonBox.minX - entityBox.maxX;
        case Direction::East:
            return entityBox.minX - pistonBox.maxX;
        default:
            return 0.0f;
    }
}

} // namespace

PistonBlockEntity::PistonBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Piston, pos)
    , m_pistonState(nullptr)
    , m_facing(Direction::North)
    , m_extending(true)
    , m_shouldRenderHead(false)
    , m_progress(0.0f)
    , m_lastProgress(0.0f)
    , m_lastTicked(0)
{}

PistonBlockEntity::PistonBlockEntity(
    const BlockPos& pos, const BlockState* pistonState, Direction facing, bool extending, bool shouldRenderHead)
    : BlockEntity(BlockEntityType::Piston, pos)
    , m_pistonState(pistonState)
    , m_facing(facing)
    , m_extending(extending)
    , m_shouldRenderHead(shouldRenderHead)
    , m_progress(0.0f)
    , m_lastProgress(0.0f)
    , m_lastTicked(0)
{}

bool PistonBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    if (data.contains("blockStateId") && data["blockStateId"].is_number_unsigned()) {
        const u32 stateId = data["blockStateId"].get<u32>();
        m_pistonState = Block::getBlockState(stateId);
    } else {
        m_pistonState = nullptr;
    }

    m_facing = static_cast<Direction>(data.value("facing", 0));
    m_extending = data.value("extending", true);
    m_shouldRenderHead = data.value("source", false);
    m_progress = data.value("progress", 0.0f);
    m_lastProgress = m_progress;

    return true;
}

void PistonBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    if (m_pistonState != nullptr) {
        data["blockStateId"] = m_pistonState->stateId();
    }

    data["facing"] = static_cast<i32>(m_facing);
    data["extending"] = m_extending;
    data["source"] = m_shouldRenderHead;
    data["progress"] = m_progress;
}

std::unique_ptr<BlockEntity> PistonBlockEntity::clone() const
{
    auto cloned = std::make_unique<PistonBlockEntity>(m_pos);
    cloned->m_facing = m_facing;
    cloned->m_extending = m_extending;
    cloned->m_shouldRenderHead = m_shouldRenderHead;
    cloned->m_progress = m_progress;
    cloned->m_lastProgress = m_lastProgress;
    cloned->m_lastTicked = m_lastTicked;
    cloned->m_pistonState = m_pistonState;
    return cloned;
}

f32 PistonBlockEntity::getProgress(f32 partialTick) const
{
    if (partialTick > 1.0f) {
        partialTick = 1.0f;
    }
    return m_lastProgress + (m_progress - m_lastProgress) * partialTick;
}

Direction PistonBlockEntity::getMotionDirection() const noexcept
{
    return m_extending ? m_facing : Directions::opposite(m_facing);
}

f32 PistonBlockEntity::getOffsetX(f32 partialTick) const
{
    return static_cast<float>(Directions::xOffset(m_facing)) * getExtendedProgress(getProgress(partialTick));
}

f32 PistonBlockEntity::getOffsetY(f32 partialTick) const
{
    return static_cast<float>(Directions::yOffset(m_facing)) * getExtendedProgress(getProgress(partialTick));
}

f32 PistonBlockEntity::getOffsetZ(f32 partialTick) const
{
    return static_cast<float>(Directions::zOffset(m_facing)) * getExtendedProgress(getProgress(partialTick));
}

void PistonBlockEntity::clearPistonBlockEntity(IWorld& world)
{
    if (m_progress < COMPLETE_THRESHOLD) {
        m_progress = COMPLETE_THRESHOLD;
        m_lastProgress = m_progress;
    }

    world.removeBlockEntity(m_pos);

    if (m_shouldRenderHead) {
        world.setBlockState(m_pos, nullptr, 3);
        return;
    }

    if (m_pistonState != nullptr) {
        // 先根据邻居状态更新被移动方块的形状（如栅栏连接、楼梯朝向等）
        BlockState updatedState = Block::updateFromNeighbourShapes(*m_pistonState, world, m_pos);
        world.setBlockState(m_pos, &updatedState, 67);

        Block& block = updatedState.getBlockMutable();
        for (Direction dir : Directions::all()) {
            const BlockPos neighborPos = m_pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);
            if (neighborState != nullptr && !neighborState->isAir()) {
                Block& neighborBlock = neighborState->getBlockMutable();
                neighborBlock.neighborChanged(world, neighborPos, block, m_pos, false);
            }
        }
    }
}

void PistonBlockEntity::tick(IWorld& world)
{
    // 记录游戏时间用于漏斗链优化
    m_lastTicked = static_cast<i64>(world.getGameTime());

    m_lastProgress = m_progress;

    if (m_lastProgress >= COMPLETE_THRESHOLD) {
        clearPistonBlockEntity(world);
        return;
    }

    const f32 newProgress = m_progress + PROGRESS_PER_TICK;
    _moveCollidedEntities(world, newProgress);

    // 蜂蜜块拖拽逻辑：当活塞推动蜂蜜块时，站在蜂蜜块上的实体应该被拖拽
    _dragEntitiesOnHoneyBlock(world, newProgress);

    m_progress = newProgress;
    if (m_progress >= COMPLETE_THRESHOLD) {
        m_progress = COMPLETE_THRESHOLD;
        clearPistonBlockEntity(world);
    }
}

f32 PistonBlockEntity::getExtendedProgress(f32 progress) const
{
    return m_extending ? progress - 1.0f : 1.0f - progress;
}

void PistonBlockEntity::_moveCollidedEntities(IWorld& world, f32 progressDelta)
{
    if (m_pistonState == nullptr) {
        return;
    }

    const CollisionShape& collisionShape = m_pistonState->getCollisionShape();
    if (collisionShape.isEmpty()) {
        return;
    }

    const f32 moveDistance = computeMoveDistance(m_progress, progressDelta);
    if (moveDistance <= 0.0f) {
        return;
    }

    const auto collisionBoxes = collisionShape.getWorldBoxes(0, 0, 0);
    if (collisionBoxes.empty()) {
        return;
    }

    const Direction motionDirection = getMotionDirection();
    const Vector3 motionVector = toDirectionVector(motionDirection);

    // 检查是否为黏液块
    const bool isSlimeBlock = m_pistonState->is(VanillaBlocks::SLIME_BLOCK);

    for (const AxisAlignedBB& localBox : collisionBoxes) {
        const AxisAlignedBB pistonBox = _moveByPositionAndProgress(localBox);
        const AxisAlignedBB sweepBox = buildSweepBox(pistonBox, motionDirection, moveDistance);

        const std::vector<Entity*> collidedEntities = world.getEntitiesInAABB(sweepBox, nullptr);
        for (Entity* entity : collidedEntities) {
            if (entity == nullptr) {
                continue;
            }

            // 检查 PushReaction
            if (entity->getPushReaction() == PushReaction::Ignore) {
                continue;
            }

            // 黏液块特殊处理：设置实体速度
            if (isSlimeBlock && m_extending) {
                // 非玩家实体在黏液块推动时会被设置速度
                // 玩家实体不在此处处理（由客户端处理）
                Vector3 velocity = entity->velocity();
                switch (Directions::getAxis(motionDirection)) {
                    case Axis::X:
                        velocity.x = static_cast<f32>(Directions::xOffset(motionDirection));
                        break;
                    case Axis::Y:
                        velocity.y = static_cast<f32>(Directions::yOffset(motionDirection));
                        break;
                    case Axis::Z:
                        velocity.z = static_cast<f32>(Directions::zOffset(motionDirection));
                        break;
                }
                entity->setVelocity(velocity.x, velocity.y, velocity.z);
            }

            // 计算实体与活塞碰撞箱的推动距离
            const AxisAlignedBB entityBox = entity->boundingBox();
            if (!entityBox.intersects(sweepBox)) {
                continue;
            }

            // 计算精确推动距离
            f32 pushDistance = 0.0f;
            if (entityBox.intersects(pistonBox)) {
                pushDistance = getMovement(pistonBox, motionDirection, entityBox);
                pushDistance = std::min(pushDistance, moveDistance) + PISTON_PUSH_EPSILON;
            } else {
                pushDistance = moveDistance + PISTON_PUSH_EPSILON;
            }

            if (pushDistance > 0.0f) {
                const Vector3 pushDelta = motionVector * pushDistance;
                // 使用 MoverType::Piston
                entity->move(entity::MoverType::Piston, pushDelta);

                // 收回时修复实体位置，防止卡入活塞基座
                if (!m_extending && m_shouldRenderHead) {
                    _fixEntityWithinPistonBase(*entity, motionDirection, moveDistance);
                }
            }
        }
    }
}

void PistonBlockEntity::_fixEntityWithinPistonBase(Entity& entity, Direction direction, f32 moveDistance)
{
    // 当活塞收回时，检查实体是否卡在活塞基座内
    // 如果是，将实体推出到基座之外

    // 活塞基座碰撞箱（方块本身）
    const AxisAlignedBB pistonBaseBox(static_cast<f32>(m_pos.x),
        static_cast<f32>(m_pos.y),
        static_cast<f32>(m_pos.z),
        static_cast<f32>(m_pos.x + 1),
        static_cast<f32>(m_pos.y + 1),
        static_cast<f32>(m_pos.z + 1));

    const AxisAlignedBB entityBox = entity.boundingBox();

    // 检查实体是否与活塞基座相交
    if (!entityBox.intersects(pistonBaseBox)) {
        return;
    }

    // 计算推动方向（与活塞收回方向相反）
    const Direction pushDirection = Directions::opposite(direction);

    // 计算推动距离
    const f32 pushDistance = getMovement(pistonBaseBox, pushDirection, entityBox) + PISTON_PUSH_EPSILON;

    // 如果推动距离很小，说明实体已经在正确位置
    if (pushDistance <= PISTON_PUSH_EPSILON) {
        return;
    }

    // 限制推动距离不超过总移动距离
    const f32 actualPush = std::min(pushDistance, moveDistance + PISTON_PUSH_EPSILON);

    if (actualPush > 0.0f) {
        const Vector3 pushVector = toDirectionVector(pushDirection) * actualPush;
        entity.move(pushVector.x, pushVector.y, pushVector.z);
    }
}

AxisAlignedBB PistonBlockEntity::_moveByPositionAndProgress(const AxisAlignedBB& aabb) const
{
    const f32 extendedProgress = getExtendedProgress(m_progress);
    return aabb.offsetted(
        static_cast<f32>(m_pos.x) + extendedProgress * static_cast<f32>(Directions::xOffset(m_facing)),
        static_cast<f32>(m_pos.y) + extendedProgress * static_cast<f32>(Directions::yOffset(m_facing)),
        static_cast<f32>(m_pos.z) + extendedProgress * static_cast<f32>(Directions::zOffset(m_facing)));
}

bool PistonBlockEntity::_isHoneyBlock() const
{
    // 检查活塞移动的方块是否是蜂蜜块
    if (m_pistonState == nullptr) {
        return false;
    }
    return m_pistonState->is(VanillaBlocks::HONEY_BLOCK);
}

void PistonBlockEntity::_dragEntitiesOnHoneyBlock(IWorld& world, f32 progressDelta)
{
    // 当活塞推动蜂蜜块时，站在蜂蜜块上方的实体应该被拖拽
    if (!_isHoneyBlock()) {
        return;
    }

    const Direction motionDirection = getMotionDirection();

    // 蜂蜜块拖拽只对水平移动有效
    if (Directions::getAxis(motionDirection) != Axis::Y) {
        // 计算移动距离
        const f32 moveDistance = computeMoveDistance(m_progress, progressDelta);
        if (moveDistance <= 0.0f) {
            return;
        }

        // 获取蜂蜜块碰撞箱上方的区域
        // 碰撞箱 Y 轴最高点 + 1.5 格高度作为扫描区域
        const CollisionShape& collisionShape = m_pistonState->getCollisionShape();
        if (collisionShape.isEmpty()) {
            return;
        }

        // 获取碰撞箱的最高 Y 值
        const auto boxes = collisionShape.getWorldBoxes(0, 0, 0);
        if (boxes.empty()) {
            return;
        }

        f32 maxY = 0.0f;
        for (const auto& box : boxes) {
            maxY = std::max(maxY, box.maxY);
        }

        // 构建蜂蜜块上方实体的扫描区域
        // 从碰撞箱顶部向上 1.5 格
        AxisAlignedBB honeyBox =
            _moveByPositionAndProgress(AxisAlignedBB(0.0f, maxY, 0.0f, 1.0f, maxY + 1.5000001f, 1.0f));

        // 获取该区域内的实体
        const std::vector<Entity*> entitiesAbove = world.getEntitiesInAABB(honeyBox, nullptr);

        for (Entity* entity : entitiesAbove) {
            if (entity == nullptr) {
                continue;
            }

            // 检查实体是否满足拖拽条件
            // 1. 实体的 PushReaction 为 NORMAL
            // 2. 实体站在地面上
            // 3. 实体的 X/Z 坐标在蜂蜜块范围内
            if (entity->getPushReaction() != PushReaction::Normal) {
                continue;
            }

            if (!entity->onGround()) {
                continue;
            }

            // 检查实体 X/Z 坐标是否在蜂蜜块范围内
            const Vector3 entityPos = entity->position();
            if (entityPos.x < honeyBox.minX || entityPos.x > honeyBox.maxX || entityPos.z < honeyBox.minZ ||
                entityPos.z > honeyBox.maxZ) {
                continue;
            }

            // 拖拽实体
            const Vector3 dragVector = toDirectionVector(motionDirection) * moveDistance;
            entity->move(entity::MoverType::Piston, dragVector);
        }
    }
}

} // namespace blockentity
} // namespace mc
