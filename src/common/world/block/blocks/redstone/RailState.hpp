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

#pragma once

#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

// Forward declaration
class AbstractRailBlock;

/**
 * @brief 铁轨连接状态计算器
 *
 * 管理铁轨的连接关系和形状计算。每当铁轨放置或邻居发生变化时，
 * 使用此类重新计算铁轨的连接形状。
 *
 * 关键功能：
 * - 在三个Y层级（同层、上一层、下一层）检测相邻铁轨
 * - 维护连接列表，追踪铁轨与其他铁轨的连接关系
 * - 处理弯轨、直轨、斜轨的形状选择
 * - 处理三连接和四连接情况下的红石道岔切换
 * - 在放置和更新时传播连接变化到相邻铁轨
 *
 * 参考: net.minecraft.world.level.block.RailState
 */
class RailState {
public:
    /**
     * @brief 构造函数
     * @param world 世界引用
     * @param pos 铁轨位置
     * @param block 铁轨方块
     * @param state 铁轨当前方块状态
     */
    RailState(IWorld& world, const BlockPos& pos, const AbstractRailBlock& block, const BlockState& state);

    /**
     * @brief 计算铁轨形状并更新世界
     *
     * 这是铁轨形状计算的主入口方法，在铁轨放置或邻居变化时调用。
     *
     * @param hasPower 是否有红石信号（用于三连接道岔切换）
     * @param updateBlock 是否实际更新世界中的方块状态
     * @param currentShape 当前铁轨形状（作为回退值）
     * @return 更新后的方块状态
     */
    BlockState place(bool hasPower, bool updateBlock, RailShape currentShape);

    /**
     * @brief 计算水平方向上的潜在连接数
     * @return 四个水平方向上有铁轨的方向数量（0-4）
     */
    [[nodiscard]] int countPotentialConnections() const;

private:
    /**
     * @brief 根据当前形状更新连接列表
     * @param shape 当前铁轨形状
     */
    void updateConnections(RailShape shape);

    /**
     * @brief 移除无效的软连接
     *
     * 遍历当前连接列表，移除那些对方不再连接回来的连接。
     * 同时将连接位置更新为对方铁轨的实际位置（可能因斜坡而Y不同）。
     */
    void removeSoftConnections();

    /**
     * @brief 检查是否与指定RailState有连接
     * @param other 另一个RailState
     * @return 如果连接列表中有与other位置XZ匹配的连接则返回true
     */
    [[nodiscard]] bool connectsTo(const RailState& other) const;

    /**
     * @brief 检查是否与指定位置有连接（仅匹配XZ坐标）
     * @param pos 目标位置
     * @return 如果连接列表中有与pos XZ匹配的连接则返回true
     */
    [[nodiscard]] bool hasConnection(const BlockPos& pos) const;

    /**
     * @brief 检查是否可以与指定RailState建立连接
     *
     * 如果已经连接，或者当前连接数少于2，则可以建立连接。
     * 每个铁轨最多只能有2个连接。
     *
     * @param other 另一个RailState
     * @return 是否可以建立连接
     */
    [[nodiscard]] bool canConnectTo(const RailState& other) const;

    /**
     * @brief 与指定RailState建立连接并更新形状
     * @param other 另一个RailState
     */
    void connectTo(RailState& other);

    /**
     * @brief 在指定位置查找铁轨（检查同层、上方、下方三个Y层级）
     * @param pos 目标位置
     * @return 找到的RailState，如果未找到返回nullptr
     */
    std::unique_ptr<RailState> getRail(const BlockPos& pos);

    /**
     * @brief 检查指定位置是否有铁轨（检查同层、上方、下方）
     * @param pos 目标位置
     * @return 是否有铁轨
     */
    [[nodiscard]] bool hasNeighborRail(const BlockPos& pos) const;

    /**
     * @brief 检查指定位置是否有铁轨方块（只检查确切位置）
     * @param pos 目标位置
     * @return 是否有铁轨方块
     */
    [[nodiscard]] static bool isRailAt(IWorld& world, const BlockPos& pos);

    /// 世界引用
    IWorld& m_world;

    /// 铁轨位置
    BlockPos m_pos;

    /// 铁轨方块
    const AbstractRailBlock& m_block;

    /// 是否为直轨（不支持弯轨，动力铁轨/探测铁轨/激活铁轨为true）
    bool m_isStraight;

    /// 连接位置列表（最多2个连接）
    std::vector<BlockPos> m_connections;
};

} // namespace blocks
} // namespace mc
