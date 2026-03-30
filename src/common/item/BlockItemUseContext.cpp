#include "BlockItemUseContext.hpp"
#include "../entity/entities/player/Player.hpp"
#include "../world/block/Material.hpp"

namespace mc {

BlockItemUseContext::BlockItemUseContext(const IWorld& world,
                                         Player* player,
                                         const ItemStack& stack,
                                         const Vector3& hitPos,
                                         const BlockPos& blockPos,
                                         Direction face,
                                         f32 playerYaw)
    : ItemUseContext(world, player, stack, hitPos, blockPos, face)
    , m_replacingClickedBlock(false)
{
    // 计算相邻位置（击中面的另一侧）
    m_adjacentPos = BlockPos(
        blockPos.x + Directions::xOffset(face),
        blockPos.y + Directions::yOffset(face),
        blockPos.z + Directions::zOffset(face)
    );

    // 计算玩家水平朝向
    // MC 的 yaw: 0=南, 90=西, 180=北, 270=东
    // 我们的 Direction: North=2, South=3, West=4, East=5
    f32 yaw = playerYaw;
    while (yaw < 0.0f) yaw += 360.0f;
    while (yaw >= 360.0f) yaw -= 360.0f;

    if (yaw < 45.0f || yaw >= 315.0f) {
        m_horizontalDirection = Direction::South;  // 面向南
    } else if (yaw < 135.0f) {
        m_horizontalDirection = Direction::West;   // 面向西
    } else if (yaw < 225.0f) {
        m_horizontalDirection = Direction::North;  // 面向北
    } else {
        m_horizontalDirection = Direction::East;   // 面向东
    }

    initialize();
}

void BlockItemUseContext::initialize()
{
    // 检查点击的方块是否可替换
    m_replacingClickedBlock = canReplace(m_blockPos);

    // 确定实际放置位置
    if (m_replacingClickedBlock) {
        m_placementPos = m_blockPos;
    } else {
        m_placementPos = m_adjacentPos;
    }
}

bool BlockItemUseContext::canReplace(const BlockPos& pos) const
{
    // 检查位置是否在世界边界内
    if (!m_world.isWithinWorldBounds(pos.x, pos.y, pos.z)) {
        return false;
    }

    // 获取当前方块状态
    const BlockState* state = m_world.getBlockState(pos.x, pos.y, pos.z);

    // 空气可以替换
    if (state == nullptr || state->isAir()) {
        return true;
    }

    // 检查材质是否可替换
    const Material& material = state->owner().material();
    if (material.isReplaceable()) {
        return true;
    }

    // 液体可以替换
    if (material.isLiquid()) {
        return true;
    }

    return false;
}

bool BlockItemUseContext::canPlace() const
{
    // 检查放置位置是否在世界边界内
    if (!m_world.isWithinWorldBounds(m_placementPos.x, m_placementPos.y, m_placementPos.z)) {
        return false;
    }

    // 检查放置位置是否可替换
    return canReplace(m_placementPos);
}

Direction BlockItemUseContext::placementDirection() const
{
    // 对于需要朝向的方块，返回玩家的朝向
    // 这是方块应该面向的方向
    return m_horizontalDirection;
}

const BlockState* BlockItemUseContext::getBlockStateAtPlacementPos() const
{
    if (!m_world.isWithinWorldBounds(m_placementPos.x, m_placementPos.y, m_placementPos.z)) {
        return nullptr;
    }
    return m_world.getBlockState(m_placementPos.x, m_placementPos.y, m_placementPos.z);
}

} // namespace mc
