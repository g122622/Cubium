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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "RailState.hpp"
#include "common/core/Types.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include <cstddef>
#include <memory>

namespace mc {
namespace blocks {

// ============================================================================
// RailState 构造与辅助
// ============================================================================

RailState::RailState(IWorld& world, const BlockPos& pos, const AbstractRailBlock& block, const BlockState& state)
    : m_world(world)
    , m_pos(pos)
    , m_block(block)
    , m_isStraight(block.isStraight())
    , m_state(state)
{
    // 根据当前形状初始化连接列表
    RailShape shape = block.getRailShape(state);
    updateConnections(shape);
}

bool RailState::isRailAt(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    const Block* block = &state->owner();
    return dynamic_cast<const AbstractRailBlock*>(block) != nullptr;
}

bool RailState::hasNeighborRail(const BlockPos& pos) const
{
    // 检查三个Y层级：同层、上方、下方
    return isRailAt(m_world, pos) || isRailAt(m_world, pos.up()) || isRailAt(m_world, pos.down());
}

std::unique_ptr<RailState> RailState::getRail(const BlockPos& pos)
{
    // 检查同层
    const BlockState* state = m_world.getBlockState(pos);
    if (state != nullptr) {
        const Block* block = &state->owner();
        const AbstractRailBlock* rail = dynamic_cast<const AbstractRailBlock*>(block);
        if (rail != nullptr) {
            return std::make_unique<RailState>(m_world, pos, *rail, *state);
        }
    }

    // 检查上方
    BlockPos above = pos.up();
    state = m_world.getBlockState(above);
    if (state != nullptr) {
        const Block* block = &state->owner();
        const AbstractRailBlock* rail = dynamic_cast<const AbstractRailBlock*>(block);
        if (rail != nullptr) {
            return std::make_unique<RailState>(m_world, above, *rail, *state);
        }
    }

    // 检查下方
    BlockPos below = pos.down();
    state = m_world.getBlockState(below);
    if (state != nullptr) {
        const Block* block = &state->owner();
        const AbstractRailBlock* rail = dynamic_cast<const AbstractRailBlock*>(block);
        if (rail != nullptr) {
            return std::make_unique<RailState>(m_world, below, *rail, *state);
        }
    }

    return nullptr;
}

// ============================================================================
// 连接管理
// ============================================================================

void RailState::updateConnections(RailShape shape)
{
    m_connections.clear();

    // 根据铁轨形状确定两个连接方向
    // 参考 MC Java: RailState.updateConnections
    switch (shape) {
        case RailShape::NorthSouth:
            m_connections.push_back(m_pos.north());
            m_connections.push_back(m_pos.south());
            break;
        case RailShape::EastWest:
            m_connections.push_back(m_pos.west());
            m_connections.push_back(m_pos.east());
            break;
        case RailShape::AscendingEast:
            m_connections.push_back(m_pos.west());
            m_connections.push_back(m_pos.east().up());
            break;
        case RailShape::AscendingWest:
            m_connections.push_back(m_pos.west().up());
            m_connections.push_back(m_pos.east());
            break;
        case RailShape::AscendingNorth:
            m_connections.push_back(m_pos.north().up());
            m_connections.push_back(m_pos.south());
            break;
        case RailShape::AscendingSouth:
            m_connections.push_back(m_pos.north());
            m_connections.push_back(m_pos.south().up());
            break;
        case RailShape::SouthEast:
            m_connections.push_back(m_pos.east());
            m_connections.push_back(m_pos.south());
            break;
        case RailShape::SouthWest:
            m_connections.push_back(m_pos.west());
            m_connections.push_back(m_pos.south());
            break;
        case RailShape::NorthWest:
            m_connections.push_back(m_pos.west());
            m_connections.push_back(m_pos.north());
            break;
        case RailShape::NorthEast:
            m_connections.push_back(m_pos.east());
            m_connections.push_back(m_pos.north());
            break;
    }
}

void RailState::removeSoftConnections()
{
    for (size_t i = 0; i < m_connections.size();) {
        auto rail = getRail(m_connections[i]);
        if (rail != nullptr && rail->connectsTo(*this)) {
            // 更新连接位置为对方铁轨的实际位置（可能因斜坡而Y不同）
            m_connections[i] = rail->m_pos;
            ++i;
        } else {
            // 对方不再连接回来，移除此连接
            m_connections.erase(m_connections.begin() + static_cast<i32>(i));
        }
    }
}

bool RailState::connectsTo(const RailState& other) const
{
    return hasConnection(other.m_pos);
}

bool RailState::hasConnection(const BlockPos& pos) const
{
    // 仅匹配XZ坐标（忽略Y差异，因为斜坡连接的Y可能不同）
    for (const auto& conn : m_connections) {
        if (conn.x == pos.x && conn.z == pos.z) {
            return true;
        }
    }
    return false;
}

bool RailState::canConnectTo(const RailState& other) const
{
    return connectsTo(other) || m_connections.size() != 2;
}

void RailState::connectTo(RailState& other)
{
    m_connections.push_back(other.m_pos);

    // 根据连接方向确定形状
    bool north = hasConnection(m_pos.north());
    bool south = hasConnection(m_pos.south());
    bool west = hasConnection(m_pos.west());
    bool east = hasConnection(m_pos.east());

    RailShape shape = RailShape::NorthSouth; // 默认值

    // 直轨：南北或东西
    if (north || south) {
        shape = RailShape::NorthSouth;
    }
    if (west || east) {
        shape = RailShape::EastWest;
    }

    // 弯轨（仅非直线铁轨）
    if (!m_isStraight) {
        if (south && east && !north && !west) {
            shape = RailShape::SouthEast;
        }
        if (south && west && !north && !east) {
            shape = RailShape::SouthWest;
        }
        if (north && west && !south && !east) {
            shape = RailShape::NorthWest;
        }
        if (north && east && !south && !west) {
            shape = RailShape::NorthEast;
        }
    }

    // 斜坡覆盖：仅对南北或东西直轨生效
    if (shape == RailShape::NorthSouth) {
        if (isRailAt(m_world, m_pos.north().up())) {
            shape = RailShape::AscendingNorth;
        }
        if (isRailAt(m_world, m_pos.south().up())) {
            shape = RailShape::AscendingSouth;
        }
    }
    if (shape == RailShape::EastWest) {
        if (isRailAt(m_world, m_pos.east().up())) {
            shape = RailShape::AscendingEast;
        }
        if (isRailAt(m_world, m_pos.west().up())) {
            shape = RailShape::AscendingWest;
        }
    }

    // 更新世界中的方块状态
    // 使用 m_state（构造时传入的状态）来构建新状态，确保安全性
    BlockState newState = m_block.withRailShape(m_state, shape);
    m_world.setBlockState(m_pos.x, m_pos.y, m_pos.z, &newState, 3);
    // 同步更新 m_state，因为后续操作可能需要基于新状态
    m_state = newState;
    updateConnections(shape);
}

// ============================================================================
// 主计算逻辑
// ============================================================================

BlockState RailState::place(bool hasPower, bool updateBlock, RailShape currentShape)
{
    // 参考 MC Java: RailState.place
    // updateBlock 参数对应 MC Java 中的 movedByPiston 参数：
    //   true = 活塞移动铁轨时使用，强制更新世界
    //   false = 放置/邻居更新时使用，仅在形状变化时更新
    // 注意：MC Java 中还有一个额外的逻辑——当 railshape 计算结果为 null 时
    // （所有分支均未命中），会回退到 currentShape。当前 C++ 实现已通过
    // 初始化 shape = currentShape 覆盖了此情况。

    // 第一步：检查四个水平方向是否有铁轨邻居
    bool north = hasNeighborRail(m_pos.north());
    bool south = hasNeighborRail(m_pos.south());
    bool west = hasNeighborRail(m_pos.west());
    bool east = hasNeighborRail(m_pos.east());

    // 判断是否有南北或东西方向的连接
    bool hasNS = north || south;
    bool hasEW = west || east;

    // 第二步：确定基本形状
    RailShape shape = currentShape; // 默认使用当前形状作为回退

    if (hasNS && !hasEW) {
        shape = RailShape::NorthSouth;
    }
    if (hasEW && !hasNS) {
        shape = RailShape::EastWest;
    }

    // 弯轨组合标记
    bool se = south && east; // flag6
    bool sw = south && west; // flag7
    bool ne = north && east; // flag8
    bool nw = north && west; // flag9

    // 第三步：弯轨选择（仅非直线铁轨，即普通铁轨）
    if (!m_isStraight) {
        if (se && !north && !west) {
            shape = RailShape::SouthEast;
        }
        if (sw && !north && !east) {
            shape = RailShape::SouthWest;
        }
        if (nw && !south && !east) {
            shape = RailShape::NorthWest;
        }
        if (ne && !south && !west) {
            shape = RailShape::NorthEast;
        }
    }

    // 第四步：三连接和四连接处理
    // 当两个轴方向都有连接时，弯轨条件不满足（需要恰好两个相邻方向无连接）
    // 此时需要根据红石信号状态选择弯轨方向
    if ((hasNS && hasEW) || (!hasNS && !hasEW)) {
        // 先使用当前形状作为基础
        if (hasNS && hasEW) {
            shape = currentShape;
        } else if (hasNS) {
            shape = RailShape::NorthSouth;
        } else if (hasEW) {
            shape = RailShape::EastWest;
        } else {
            // 无连接：保持当前形状
            shape = currentShape;
        }

        // 非直线铁轨（普通铁轨）根据红石信号选择弯轨方向
        // 这是MC中T型道岔的核心逻辑：红石信号可以切换弯轨方向
        if (!m_isStraight) {
            if (hasPower) {
                // 有红石信号时的优先级（后面的if覆盖前面的，因此最后的匹配胜出）
                if (se) {
                    shape = RailShape::SouthEast;
                }
                if (sw) {
                    shape = RailShape::SouthWest;
                }
                if (ne) {
                    shape = RailShape::NorthEast;
                }
                if (nw) {
                    shape = RailShape::NorthWest;
                }
            } else {
                // 无红石信号时的优先级（与有信号时相反）
                if (nw) {
                    shape = RailShape::NorthWest;
                }
                if (ne) {
                    shape = RailShape::NorthEast;
                }
                if (sw) {
                    shape = RailShape::SouthWest;
                }
                if (se) {
                    shape = RailShape::SouthEast;
                }
            }
        }
    }

    // 第五步：斜坡覆盖（仅对南北和东西直轨生效）
    if (shape == RailShape::NorthSouth) {
        if (isRailAt(m_world, m_pos.north().up())) {
            shape = RailShape::AscendingNorth;
        }
        if (isRailAt(m_world, m_pos.south().up())) {
            shape = RailShape::AscendingSouth;
        }
    }
    if (shape == RailShape::EastWest) {
        if (isRailAt(m_world, m_pos.east().up())) {
            shape = RailShape::AscendingEast;
        }
        if (isRailAt(m_world, m_pos.west().up())) {
            shape = RailShape::AscendingWest;
        }
    }

    // 更新连接列表
    updateConnections(shape);

    // 构建新状态
    // 参考 MC Java: RailState.place() 使用 this.state（构造时传入的状态）来构建新状态，
    // 而非从世界中重新读取，避免在放置流程中该位置可能还不是铁轨的问题。
    BlockState newState = m_block.withRailShape(m_state, shape);
    const BlockState* oldState = m_world.getBlockState(m_pos);
    bool shapeChanged = oldState == nullptr || *oldState != newState;

    // 第六步：根据模式决定是否直接写入世界和传播连接
    // updateBlock=true 时（放置或neighborChanged），直接设置方块状态并传播连接
    // updateBlock=false 时（updatePostPlacement），仅返回计算后的状态，由调用方设置
    if (shapeChanged && updateBlock) {
        m_world.setBlockState(m_pos.x, m_pos.y, m_pos.z, &newState, 3);

        // 传播连接到相邻铁轨
        for (const auto& connPos : m_connections) {
            auto rail = getRail(connPos);
            if (rail != nullptr) {
                rail->removeSoftConnections();
                if (rail->canConnectTo(*this)) {
                    rail->connectTo(*this);
                }
            }
        }
    }

    // 返回结果：updateBlock模式下从世界读取最终状态，否则返回计算值
    if (updateBlock) {
        const BlockState* resultState = m_world.getBlockState(m_pos);
        return resultState != nullptr ? *resultState : newState;
    }
    return newState;
}

int RailState::countPotentialConnections() const
{
    int count = 0;
    // 检查四个水平方向
    if (hasNeighborRail(m_pos.north())) {
        ++count;
    }
    if (hasNeighborRail(m_pos.south())) {
        ++count;
    }
    if (hasNeighborRail(m_pos.west())) {
        ++count;
    }
    if (hasNeighborRail(m_pos.east())) {
        ++count;
    }
    return count;
}

} // namespace blocks
} // namespace mc
