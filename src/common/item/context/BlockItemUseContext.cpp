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

#include "BlockItemUseContext.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace mc {

namespace {

// 与 MC 1.21.11 Direction.orderedByNearest(Entity) 一致：
// 依据玩家视线俯仰角(pitch)与偏航角(yaw)对 6 个方向按"与视线夹角"由小到大排序。
// 返回的数组长度恒为 6，且第 i 个与第 5-i 个互为相反方向。
//
// 参数：
//   yawDeg   玩家偏航角（度）。MC 约定：0=南, 90=西, 180=北, 270=东。
//   pitchDeg 玩家俯仰角（度）。MC 约定：0=水平，正=俯视，负=仰视。
std::array<Direction, 6> orderedByNearest(f32 yawDeg, f32 pitchDeg)
{
    // MC 1.21.11: f = viewXRot*(π/180)；f1 = -viewYRot*(π/180)
    // 注：MC 的 viewXRot 即 pitch，正=俯视，负=仰视。
    const f32 f = pitchDeg * math::DEG_TO_RAD;
    const f32 f1 = -yawDeg * math::DEG_TO_RAD;
    const f32 f2 = std::sin(f);   // sin(pitch)
    const f32 f3 = std::cos(f);   // cos(pitch)
    const f32 f4 = std::sin(f1);  // sin(-yaw)
    const f32 f5 = std::cos(f1);  // cos(-yaw)
    const bool flag = f4 > 0.0f;  // 东向分量>0
    const bool flag1 = f2 < 0.0f; // true=仰视（pitch<0 时 sin(pitch)<0），false=俯视
    const bool flag2 = f5 > 0.0f; // 南向分量>0
    const f32 f6 = flag ? f4 : -f4;
    const f32 f7 = flag1 ? -f2 : f2;
    const f32 f8 = flag2 ? f5 : -f5;
    const f32 f9 = f6 * f3;
    const f32 f10 = f8 * f3;
    const Direction direction = flag ? Direction::East : Direction::West;
    const Direction direction1 = flag1 ? Direction::Up : Direction::Down;
    const Direction direction2 = flag2 ? Direction::South : Direction::North;

    // makeDirectionArray(a, b, c) = {a, b, c, opposite(c), opposite(b), opposite(a)}
    const auto makeArray = [](Direction a, Direction b, Direction c) -> std::array<Direction, 6> {
        return {a, b, c, Directions::opposite(c), Directions::opposite(b), Directions::opposite(a)};
    };

    if (f6 > f8) {
        if (f7 > f9) {
            return makeArray(direction1, direction, direction2);
        }
        return f10 > f7 ? makeArray(direction, direction2, direction1) : makeArray(direction, direction1, direction2);
    }
    if (f7 > f10) {
        return makeArray(direction1, direction2, direction);
    }
    return f9 > f7 ? makeArray(direction2, direction, direction1) : makeArray(direction2, direction1, direction);
}

} // namespace

BlockItemUseContext::BlockItemUseContext(IWorld& world,
    Player* player,
    const ItemStack& stack,
    const Vector3& hitPos,
    const BlockPos& blockPos,
    Direction face,
    f32 playerYaw,
    f32 playerPitch)
    : ItemUseContext(world, player, stack, hitPos, blockPos, face, Hand::MainHand, playerYaw, playerPitch)
    , m_replacingClickedBlock(false)
{
    // 计算相邻位置（击中面的另一侧）
    m_adjacentPos = BlockPos(blockPos.x + Directions::xOffset(face),
        blockPos.y + Directions::yOffset(face),
        blockPos.z + Directions::zOffset(face));

    // 计算玩家水平朝向
    // MC 的 yaw: 0=南, 90=西, 180=北, 270=东
    // 我们的 Direction: North=2, South=3, West=4, East=5
    f32 yaw = playerYaw;
    while (yaw < 0.0f)
        yaw += 360.0f;
    while (yaw >= 360.0f)
        yaw -= 360.0f;

    if (yaw < 45.0f || yaw >= 315.0f) {
        m_horizontalDirection = Direction::South; // 面向南
    } else if (yaw < 135.0f) {
        m_horizontalDirection = Direction::West; // 面向西
    } else if (yaw < 225.0f) {
        m_horizontalDirection = Direction::North; // 面向北
    } else {
        m_horizontalDirection = Direction::East; // 面向东
    }

    _initialize();
}

void BlockItemUseContext::_initialize()
{
    // 检查点击的方块是否可替换
    m_replacingClickedBlock = _canReplace(m_blockPos);

    // 确定实际放置位置
    if (m_replacingClickedBlock) {
        m_placementPos = m_blockPos;
    } else {
        m_placementPos = m_adjacentPos;
    }
}

bool BlockItemUseContext::_canReplace(const BlockPos& pos) const
{
    // 检查位置是否在世界边界内
    if (!m_world.isWithinWorldBounds(pos)) {
        return false;
    }

    // 获取当前方块状态
    const BlockState* state = m_world.getBlockState(pos);

    // 空位置视为可替换
    if (state == nullptr) {
        return true;
    }

    // 调用方块的 isReplaceable 虚方法进行上下文感知的替换判断
    // 这允许方块实现自定义替换逻辑（如花瓣床堆叠、台阶合并为双层）
    // 默认实现返回 BlockProperties::replaceable() 标志值
    return state->getBlock().isReplaceable(*state, *this);
}

bool BlockItemUseContext::canPlace() const
{
    // 检查放置位置是否在世界边界内
    if (!m_world.isWithinWorldBounds(m_placementPos)) {
        return false;
    }

    // 检查放置位置是否可替换
    return _canReplace(m_placementPos);
}

Direction BlockItemUseContext::placementDirection() const
{
    // 对于需要朝向的方块，返回玩家的朝向
    // 这是方块应该面向的方向
    return m_horizontalDirection;
}

const BlockState* BlockItemUseContext::getBlockStateAtPlacementPos() const
{
    if (!m_world.isWithinWorldBounds(m_placementPos)) {
        return nullptr;
    }
    return m_world.getBlockState(m_placementPos);
}

std::vector<Direction> BlockItemUseContext::getNearestLookingDirections() const
{
    // 与 MC 1.21.11 BlockPlaceContext.getNearestLookingDirections 对齐：
    //   Direction[] adirection = Direction.orderedByNearest(player);
    //   if (replaceClicked) return adirection;
    //   Direction direction = getClickedFace();
    //   找到 direction.getOpposite() 在 adirection 中的下标 i；
    //   若 i > 0，将 adirection[0..i-1] 整体后移一位到 [1..i]，并把 adirection[0] 置为
    //   direction.getOpposite()（即把"点击面的反向"提到首位）。
    //   return adirection;

    // 获取玩家视线 yaw/pitch。优先使用真实玩家实体；测试上下文可能 player==nullptr，
    // 此时回退到构造时传入的 m_playerYaw / m_playerPitch（由调用方显式提供，
    // 例如 BlockInteractionManager 通过 ServerPlayerData->pitch 传入）。
    f32 yawDeg = m_playerYaw;
    f32 pitchDeg = m_playerPitch;
    if (m_player != nullptr) {
        yawDeg = m_player->yaw();
        pitchDeg = m_player->pitch();
    }

    std::array<Direction, 6> ordered = orderedByNearest(yawDeg, pitchDeg);

    if (m_replacingClickedBlock) {
        return std::vector<Direction>(ordered.begin(), ordered.end());
    }

    const Direction clickedFace = m_face;
    const Direction priority = Directions::opposite(clickedFace);

    // 在 ordered 中定位 priority 的下标
    std::size_t i = 0;
    while (i < ordered.size() && ordered[i] != priority) {
        ++i;
    }

    if (i > 0) {
        // 将 ordered[0..i-1] 后移到 [1..i]，ordered[0] = priority
        std::vector<Direction> result(ordered.size());
        result[0] = priority;
        for (std::size_t k = 0; k < i; ++k) {
            result[k + 1] = ordered[k];
        }
        for (std::size_t k = i + 1; k < ordered.size(); ++k) {
            result[k] = ordered[k];
        }
        return result;
    }

    return std::vector<Direction>(ordered.begin(), ordered.end());
}

} // namespace mc
