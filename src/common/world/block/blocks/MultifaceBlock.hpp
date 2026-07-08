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

#pragma once

#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IWaterLoggable.hpp"

#include <vector>

namespace mc {
namespace blocks {

/**
 * @brief 多面方块基类（MC MultifaceBlock）
 *
 * 可附着在六个面上的方块（如发光地衣、幽匿脉络）。每个方向对应一个布尔面属性
 * （NORTH/SOUTH/EAST/WEST/UP/DOWN），外加 WATERLOGGED。状态由面掩码唯一确定。
 *
 * 提供 worldgen 所需的放置/查询能力：
 * - getStateForPlacement(currentState, world, pos, direction)：在指定方向加一面。
 * - isValidStateForPlacement / hasFace / hasAnyFace：供 MultifaceSpreader 判定。
 * - canAttachTo：相邻方块该面是否实心可附着（对齐 MC Block.isFaceFull 判定）。
 */
class MultifaceBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 取某方向对应的面布尔属性。
     * @return 对应 BooleanProperty；不支持的朝向返回 nullptr。
     */
    [[nodiscard]] static const BooleanProperty* getFaceProperty(Direction direction);

    /// MC MultifaceBlock.hasFace：当前 state 是否在 direction 方向已有面。
    [[nodiscard]] static bool hasFace(const BlockState& state, Direction direction);

    /// MC MultifaceBlock.hasAnyFace：六个方向是否至少有一个面。
    [[nodiscard]] static bool hasAnyFace(const BlockState& state);

    /// MC MultifaceBlock.availableFaces：state 在哪些方向有面（仅 MultifaceBlock 子类有效）。
    [[nodiscard]] static std::vector<Direction> availableFaces(const BlockState& state);

    /// MC MultifaceBlock.pack：把方向集合按 ordinal 压成字节掩码（用于 levelEvent 3006 data 编码）。
    [[nodiscard]] static u8 pack(const std::vector<Direction>& directions);

    /**
     * @brief MC MultifaceBlock.isValidStateForPlacement：
     *        该朝向受支持 且（非本方块 或 该朝向无面）且 相邻格实心可附着。
     */
    [[nodiscard]] bool isValidStateForPlacement(
        IWorld& world, const BlockState& state, const BlockPos& pos, Direction direction) const;

    /**
     * @brief MC MultifaceBlock.getStateForPlacement(state, reader, pos, direction)
     *
     * 在 direction 方向给 state 增加一面。若当前格已是同种多面方块则保留现有面；
     * 若当前格是水源则设 WATERLOGGED；否则用默认状态。返回 nullptr 表示不可放置。
     *
     * @param currentState 当前格（origin）的方块状态（空气/水/本方块）
     */
    [[nodiscard]] const BlockState* getStateForPlacement(
        const BlockState* currentState, IWorld& world, const BlockPos& pos, Direction direction) const;

    // 引入基类玩家放置重载 getStateForPlacement(BlockItemUseContext&)，避免被上方 worldgen
    // 版本（4 参数）隐藏（-Woverloaded-virtual）。子类按需覆写玩家放置版本。
    using Block::getStateForPlacement;

    /// MC MultifaceBlock.canAttachTo：相邻方块在 direction 反方向的面是否实心可附着。
    [[nodiscard]] static bool canAttachTo(IWorld& world, Direction direction, const BlockPos& neighborPos);

    /// 默认全部朝向都受支持（对齐 MC MultifaceBlock.isFaceSupported）。
    [[nodiscard]] static bool isFaceSupported(Direction direction)
    {
        MC_UNUSED(direction);
        return true;
    }

protected:
    explicit MultifaceBlock(const BlockProperties& properties)
        : Block(properties)
    {}

    /// 注册六方向面属性 + WATERLOGGED（子类构造中调用）。
    void buildMultifaceStateContainer();
};

} // namespace blocks
} // namespace mc
