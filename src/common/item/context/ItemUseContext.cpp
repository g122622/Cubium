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

#include "ItemUseContext.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>

namespace mc {

ItemUseContext::ItemUseContext(IWorld& world,
    Player* player,
    const ItemStack& stack,
    const Vector3& hitPos,
    const BlockPos& blockPos,
    Direction face,
    Hand hand,
    f32 playerYaw,
    f32 playerPitch)
    : m_world(world)
    , m_player(player)
    , m_stack(const_cast<ItemStack*>(&stack))
    , m_hitPos(hitPos)
    , m_blockPos(blockPos)
    , m_face(face)
    , m_hand(hand)
    , m_playerYaw(playerYaw)
    , m_playerPitch(playerPitch)
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
