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
#include <cstddef>
#include <vector>

namespace mc {

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

    // 计算玩家水平朝向。等价原版 UseOnContext.getHorizontalDirection() → Entity.getDirection()
    //   → Direction.fromYRot(yaw) = from2DDataValue(floor(yaw / 90 + 0.5) & 3)；
    // MC 的 yaw: 0=南, 90=东, 180=北, 270=西（视线单位向量 (cos yaw, 0, -sin yaw)）。
    // 有玩家时取其身体朝向实时值，否则用构造时传入的 yaw 快照（测试/程序化放置）。
    // 旧实现按 yaw < 45 / < 135 / < 225 / else 四档划分，North/East/West 三档各偏一个象限
    // （yaw=90 得 West、yaw=180 得 South、yaw=270 得 East），使炉子/门/床/活塞/铁砧等
    // 所有依赖玩家朝向的方块朝向错误。
    const f32 yaw = (m_player != nullptr) ? m_player->yaw() : m_playerYaw;
    m_horizontalDirection = Directions::fromYRot(yaw);

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

Direction BlockItemUseContext::getNearestLookingDirection() const
{
    //   return Direction.orderedByNearest(player)[0];
    // 无条件返回玩家视线最近方向（不经过 getNearestLookingDirections 的「点击面反向提首」处理）。
    // 用于木桶/活塞/发射器等「朝向玩家视线反方向」放置的方块（facing = opposite(this)）。
    //
    // 取玩家视线 yaw/pitch：优先真实玩家实体，回退构造时传入的 m_playerYaw/m_playerPitch
    // （与 getNearestLookingDirections 一致）。
    f32 yawDeg = m_playerYaw;
    f32 pitchDeg = m_playerPitch;
    if (m_player != nullptr) {
        yawDeg = m_player->yaw();
        pitchDeg = m_player->pitch();
    }

    return Directions::orderedByNearest(yawDeg, pitchDeg)[0];
}

std::vector<Direction> BlockItemUseContext::getNearestLookingDirections() const
{
    // 取点击面之前，要先把「点击面的反向」提到排序首位。
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

    std::array<Direction, 6> ordered = Directions::orderedByNearest(yawDeg, pitchDeg);

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
