#include "ItemUseContext.hpp"
#include "../../entity/entities/player/Player.hpp"
#include "../core/ItemStack.hpp"

namespace mc {

ItemUseContext::ItemUseContext(IWorld& world,
    Player* player,
    const ItemStack& stack,
    const Vector3& hitPos,
    const BlockPos& blockPos,
    Direction face,
    Hand hand,
    f32 playerYaw)
    : m_world(world)
    , m_player(player)
    , m_stack(const_cast<ItemStack*>(&stack))
    , m_hitPos(hitPos)
    , m_blockPos(blockPos)
    , m_face(face)
    , m_hand(hand)
    , m_playerYaw(playerYaw)
{
    // 计算击中点在方块内的相对坐标（0-1范围）
    m_hitX = hitPos.x - static_cast<f32>(blockPos.x);
    m_hitY = hitPos.y - static_cast<f32>(blockPos.y);
    m_hitZ = hitPos.z - static_cast<f32>(blockPos.z);

    // 确保范围在 [0, 1)
    m_hitX = m_hitX - std::floor(m_hitX);
    m_hitY = m_hitY - std::floor(m_hitY);
    m_hitZ = m_hitZ - std::floor(m_hitZ);
}

Vector3 ItemUseContext::hitPositionInBlock() const
{
    return Vector3(m_hitX, m_hitY, m_hitZ);
}

f32 ItemUseContext::getHitU(Axis axis) const
{
    switch (axis) {
        case Axis::X:
            return m_hitX;
        case Axis::Y:
            return m_hitY;
        case Axis::Z:
            return m_hitZ;
        default:
            return 0.0f;
    }
}

} // namespace mc
