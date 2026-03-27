#include "PistonBlockEntity.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include "../../../util/AxisAlignedBB.hpp"
#include "../../../entity/Entity.hpp"
#include <nlohmann/json.hpp>
#include <cmath>

namespace mc {
namespace blockentity {

// ============================================================================
// 构造函数
// ============================================================================

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
    std::unique_ptr<BlockState> pistonState,
    Direction facing,
    bool extending,
    bool shouldRenderHead)
    : BlockEntity(BlockEntityType::Piston, pos)
    , m_pistonState(std::move(pistonState))
    , m_facing(facing)
    , m_extending(extending)
    , m_shouldRenderHead(shouldRenderHead)
    , m_progress(0.0f)
    , m_lastProgress(0.0f)
    , m_lastTicked(0) {
}

// ============================================================================
// BlockEntity 接口实现
// ============================================================================

bool PistonBlockEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载活塞状态
    // TODO: 从JSON加载BlockState
    // if (data.contains("blockState")) {
    //     m_pistonState = BlockState::fromJson(data["blockState"]);
    // }

    m_facing = static_cast<Direction>(data.value("facing", 0));
    m_extending = data.value("extending", true);
    m_shouldRenderHead = data.value("source", false);
    m_progress = data.value("progress", 0.0f);
    m_lastProgress = m_progress;

    return true;
}

void PistonBlockEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    // 保存活塞状态
    // TODO: 保存BlockState到JSON
    // if (m_pistonState) {
    //     data["blockState"] = m_pistonState->toJson();
    // }

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
    // TODO: 克隆 m_pistonState
    return cloned;
}

// ============================================================================
// 活塞特有方法
// ============================================================================

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
        // 动画未完成，强制完成
        m_progress = COMPLETE_THRESHOLD;
        m_lastProgress = m_progress;
    }

    // 移除方块实体
    // world.removeBlockEntity(m_pos);

    // 检查当前位置是否是移动活塞方块
    const BlockState* currentState = world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    if (currentState && currentState->getBlock().canProvidePower(*currentState)) {
        // TODO: 检查是否是 MOVING_PISTON 方块
        // 如果是，替换为最终方块
        if (m_shouldRenderHead) {
            // 放置空气
            // world.setBlockState(m_pos.x, m_pos.y, m_pos.z, nullptr, 3);
        } else if (m_pistonState) {
            // 放置被移动的方块
            // world.setBlockState(m_pos.x, m_pos.y, m_pos.z, m_pistonState.get(), 67);
            // 触发邻居更新
            // world.neighborChanged(m_pos, m_pistonState->getBlock(), m_pos);
        }
    }
}

// ============================================================================
// Tick 更新
// ============================================================================

void PistonBlockEntity::tick(IWorld& world) {
    // 记录上次tick时间
    // m_lastTicked = world.getGameTime();
    m_lastProgress = m_progress;

    if (m_lastProgress >= COMPLETE_THRESHOLD) {
        // 动画已完成，移除方块实体
        // world.removeBlockEntity(m_pos);
        return;
    }

    // 更新进度
    float newProgress = m_progress + PROGRESS_PER_TICK;

    // 移动碰撞的实体
    moveCollidedEntities(world, newProgress);

    // TODO: 处理蜂蜜块推动实体的特殊情况
    // func_227024_g_(newProgress);

    m_progress = newProgress;

    if (m_progress >= COMPLETE_THRESHOLD) {
        m_progress = COMPLETE_THRESHOLD;
    }
}

// ============================================================================
// 私有方法
// ============================================================================

float PistonBlockEntity::getExtendedProgress(float progress) const {
    return m_extending ? progress - 1.0f : 1.0f - progress;
}

void PistonBlockEntity::moveCollidedEntities(IWorld& world, float progressDelta) {
    // TODO: 实现实体推动逻辑
    // 1. 获取活塞方块的碰撞箱
    // 2. 找到碰撞箱内的所有实体
    // 3. 计算实体需要被推动的距离
    // 4. 移动实体

    MC_UNUSED(world);
    MC_UNUSED(progressDelta);

    // 当前框架还没有完善实体系统，暂时跳过实体推动
    // 实现框架如下：
    // Direction motionDir = getMotionDirection();
    // float extendedProgress = getExtendedProgress(m_progress);
    //
    // // 计算碰撞箱
    // BlockState collisionState = getCollisionRelatedBlockState();
    // VoxelShape shape = collisionState.getCollisionShape(world, m_pos);
    //
    // if (!shape.isEmpty()) {
    //     AxisAlignedBB pistonBox = moveByPositionAndProgress(shape.getBoundingBox());
    //     float moveDistance = progressDelta - m_progress;
    //
    //     // 获取碰撞实体
    //     std::vector<Entity*> entities = world.getEntitiesInAABB(
    //         pistonBox.expand(motionDir, moveDistance), nullptr);
    //
    //     for (Entity* entity : entities) {
    //         if (entity->getPushReaction() != PushReaction::Ignore) {
    //             // 计算推动距离
    //             double pushDist = calculatePushDistance(pistonBox, motionDir, entity->getBoundingBox());
    //             pushDist = std::min(pushDist, static_cast<double>(moveDistance)) + 0.01;
    //
    //             // 移动实体
    //             entity->move(MoverType::Piston,
    //                 Vector3d(pushDist * motionDir.getXOffset(),
    //                         pushDist * motionDir.getYOffset(),
    //                         pushDist * motionDir.getZOffset()));
    //
    //             // 如果是收回且有活塞头，修复实体位置
    //             if (!m_extending && m_shouldRenderHead) {
    //                 fixEntityWithinPistonBase(entity, motionDir, moveDistance);
    //             }
    //         }
    //     }
    // }
}

AxisAlignedBB PistonBlockEntity::moveByPositionAndProgress(const AxisAlignedBB& aabb) const {
    float extendedProgress = getExtendedProgress(m_progress);
    return aabb.offsetted(
        static_cast<f32>(m_pos.x) + extendedProgress * static_cast<float>(Directions::xOffset(m_facing)),
        static_cast<f32>(m_pos.y) + extendedProgress * static_cast<float>(Directions::yOffset(m_facing)),
        static_cast<f32>(m_pos.z) + extendedProgress * static_cast<float>(Directions::zOffset(m_facing))
    );
}

} // namespace blockentity
} // namespace mc
