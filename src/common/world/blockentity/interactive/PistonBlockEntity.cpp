#include "PistonBlockEntity.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../physics/collision/CollisionShape.hpp"
#include "../../../util/AxisAlignedBB.hpp"
#include "../../../util/Direction.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>

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
[[nodiscard]] f32 computeMoveDistance(f32 previousProgress, f32 nextProgress) {
    return std::max(0.0f, nextProgress - previousProgress);
}

/**
 * @brief 计算方向向量。
 */
[[nodiscard]] Vector3 toDirectionVector(Direction direction) {
    return Vector3(
        static_cast<f32>(Directions::xOffset(direction)),
        static_cast<f32>(Directions::yOffset(direction)),
        static_cast<f32>(Directions::zOffset(direction)));
}

/**
 * @brief 获取本次可能碰撞到实体的扫描包围盒。
 */
[[nodiscard]] AxisAlignedBB buildSweepBox(const AxisAlignedBB& pistonBox,
                                          Direction direction,
                                          f32 moveDistance) {
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
 *
 * 参考: MC 1.16.5 PistonTileEntity.getMovement()
 */
[[nodiscard]] f32 getMovement(const AxisAlignedBB& pistonBox, Direction direction, const AxisAlignedBB& entityBox) {
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
    , m_lastTicked(0) {
}

PistonBlockEntity::PistonBlockEntity(
    const BlockPos& pos,
    const BlockState* pistonState,
    Direction facing,
    bool extending,
    bool shouldRenderHead)
    : BlockEntity(BlockEntityType::Piston, pos)
    , m_pistonState(pistonState)
    , m_facing(facing)
    , m_extending(extending)
    , m_shouldRenderHead(shouldRenderHead)
    , m_progress(0.0f)
    , m_lastProgress(0.0f)
    , m_lastTicked(0) {
}

bool PistonBlockEntity::load(const nlohmann::json& data) {
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

void PistonBlockEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    if (m_pistonState != nullptr) {
        data["blockStateId"] = m_pistonState->stateId();
    }

    data["facing"] = static_cast<i32>(m_facing);
    data["extending"] = m_extending;
    data["source"] = m_shouldRenderHead;
    data["progress"] = m_progress;
}

std::unique_ptr<BlockEntity> PistonBlockEntity::clone() const {
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

float PistonBlockEntity::getProgress(float partialTick) const {
    if (partialTick > 1.0f) {
        partialTick = 1.0f;
    }
    return m_lastProgress + (m_progress - m_lastProgress) * partialTick;
}

Direction PistonBlockEntity::getMotionDirection() const {
    return m_extending ? m_facing : Directions::opposite(m_facing);
}

float PistonBlockEntity::getOffsetX(float partialTick) const {
    return static_cast<float>(Directions::xOffset(m_facing)) * getExtendedProgress(getProgress(partialTick));
}

float PistonBlockEntity::getOffsetY(float partialTick) const {
    return static_cast<float>(Directions::yOffset(m_facing)) * getExtendedProgress(getProgress(partialTick));
}

float PistonBlockEntity::getOffsetZ(float partialTick) const {
    return static_cast<float>(Directions::zOffset(m_facing)) * getExtendedProgress(getProgress(partialTick));
}

void PistonBlockEntity::clearPistonBlockEntity(IWorld& world) {
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
        world.setBlockState(m_pos, m_pistonState, 67);

        Block& block = const_cast<Block&>(m_pistonState->getBlock());
        for (Direction dir : Directions::all()) {
            const BlockPos neighborPos = m_pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);
            if (neighborState != nullptr && !neighborState->isAir()) {
                Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
                neighborBlock.neighborChanged(world, neighborPos, block, m_pos, false);
            }
        }
    }
}

void PistonBlockEntity::tick(IWorld& world) {
    // MC 1.16.5: 记录游戏时间用于漏斗链优化
    // m_lastTicked = world.getGameTime();  // TODO: 需要 IWorld::getGameTime()

    m_lastProgress = m_progress;

    if (m_lastProgress >= COMPLETE_THRESHOLD) {
        clearPistonBlockEntity(world);
        return;
    }

    const float newProgress = m_progress + PROGRESS_PER_TICK;
    moveCollidedEntities(world, newProgress);

    // TODO: MC 1.16.5 蜂蜜块拖拽逻辑 (func_227024_g_)
    // 当活塞推动蜂蜜块时，站在蜂蜜块上的实体应该被拖拽
    // 参考: PistonTileEntity.func_227024_g_

    m_progress = newProgress;
    if (m_progress >= COMPLETE_THRESHOLD) {
        m_progress = COMPLETE_THRESHOLD;
        clearPistonBlockEntity(world);
    }
}

float PistonBlockEntity::getExtendedProgress(float progress) const {
    return m_extending ? progress - 1.0f : 1.0f - progress;
}

void PistonBlockEntity::moveCollidedEntities(IWorld& world, float progressDelta) {
    // MC 1.16.5 对齐 - 参考 PistonTileEntity.moveCollidedEntities()
    // 缺失功能:
    // 1. PushReaction.IGNORE 检测 - 需要 Entity::getPushReaction()
    // 2. MoverType.PISTON - 需要 Entity::move(MoverType, Vec3) 重载
    // 3. 黏液块动量设置 - 需要 Entity::setMotion() 和黏液块检测

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

    for (const AxisAlignedBB& localBox : collisionBoxes) {
        const AxisAlignedBB pistonBox = moveByPositionAndProgress(localBox);
        const AxisAlignedBB sweepBox = buildSweepBox(pistonBox, motionDirection, moveDistance);

        const std::vector<Entity*> collidedEntities = world.getEntitiesInAABB(sweepBox, nullptr);
        for (Entity* entity : collidedEntities) {
            if (entity == nullptr) {
                continue;
            }

            // TODO: 检查 entity->getPushReaction() != PushReaction::IGNORE
            // MC 1.16.5: if (entity.getPushReaction() != PushReaction.IGNORE)

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
                entity->move(pushDelta.x, pushDelta.y, pushDelta.z);

                // 收回时修复实体位置，防止卡入活塞基座
                if (!m_extending && m_shouldRenderHead) {
                    fixEntityWithinPistonBase(*entity, motionDirection, moveDistance);
                }
            }
        }
    }
}

void PistonBlockEntity::fixEntityWithinPistonBase(Entity& entity, Direction direction, f32 moveDistance) {
    // MC 1.16.5: fixEntityWithinPistonBase
    // 当活塞收回时，检查实体是否卡在活塞基座内
    // 如果是，将实体推出到基座之外

    // 活塞基座碰撞箱（方块本身）
    const AxisAlignedBB pistonBaseBox(
        static_cast<f32>(m_pos.x),
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

AxisAlignedBB PistonBlockEntity::moveByPositionAndProgress(const AxisAlignedBB& aabb) const {
    const float extendedProgress = getExtendedProgress(m_progress);
    return aabb.offsetted(
        static_cast<f32>(m_pos.x) + extendedProgress * static_cast<float>(Directions::xOffset(m_facing)),
        static_cast<f32>(m_pos.y) + extendedProgress * static_cast<float>(Directions::yOffset(m_facing)),
        static_cast<f32>(m_pos.z) + extendedProgress * static_cast<float>(Directions::zOffset(m_facing))
    );
}

} // namespace blockentity
} // namespace mc
