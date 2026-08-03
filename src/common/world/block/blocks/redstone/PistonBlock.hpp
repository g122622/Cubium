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

#include "../../../../util/Direction.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../Block.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Material.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 活塞方块
 *
 * 活塞可以推动方块，粘性活塞还可以拉回方块。
 *
 * ## 特性
 * - 推动方块（最多12格）
 * - 粘性活塞拉回方块
 * - 推动链计算
 * - 红石激活
 *
 * ## 容易踩的坑
 * - 推动链长度限制（12格）
 * - 某些方块不能被推动（基岩等）
 * - 推动过程中不能被再次激活
 * - 方向性处理
 */
class PistonBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param sticky 是否为粘性活塞
     */
    PistonBlock(const BlockProperties& properties, bool sticky);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Block;
    }

    // ========== 活塞特有方法 ==========

    /**
     * @brief 检查活塞是否伸出
     *
     * @param state 方块状态
     * @return true 如果伸出
     */
    [[nodiscard]] static bool isExtended(const BlockState& state);

    /**
     * @brief 设置活塞伸出状态
     *
     * @param state 方块状态
     * @param extended 是否伸出
     * @return const BlockState& 更新后的状态（持久化引用）
     */
    [[nodiscard]] static const BlockState& withExtended(const BlockState& state, bool extended);

    /**
     * @brief 获取活塞朝向
     *
     * @param state 方块状态
     * @return Direction 朝向方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 检查是否为粘性活塞
     *
     * @return true 如果是粘性活塞
     */
    [[nodiscard]] bool isSticky() const { return m_sticky; }

    /**
     * @brief 检查活塞是否应该伸出
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果应该伸出
     */
    [[nodiscard]] bool shouldBeExtended(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查方块是否可以被推动
     *
     * 静态方法，用于 PistonStructureHelper。
     *
     * @param blockState 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param facing 推动方向
     * @param destroyBlocks 是否允许破坏方块
     * @param direction 活塞朝向
     * @return true 如果可以推动
     */
    [[nodiscard]] static bool canPush(const BlockState& blockState,
        IWorld& world,
        const BlockPos& pos,
        Direction facing,
        bool destroyBlocks,
        Direction direction);

    /**
     * @brief 尝试伸出活塞
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果伸出成功
     */
    bool extend(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 尝试收回活塞
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果收回成功
     */
    bool retract(IWorld& world, const BlockPos& pos, const BlockState& state);

private:
    /// 是否为粘性活塞
    bool m_sticky;

    /// 活塞动画时长（tick）
    static constexpr i32 EXTEND_DELAY = 2;
    static constexpr i32 RETRACT_DELAY = 2;

    /// 最大推动方块数
    static constexpr i32 MAX_PUSH_BLOCKS = 12;

    /**
     * @brief 检查是否需要移动并触发方块事件
     *
     * @param world 世界引用
     * @param pos 活塞位置
     * @param state 当前方块状态
     */
    void _checkForMove(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 执行移动
     *
     * @param world 世界引用
     * @param pos 活塞位置
     * @param facing 推动方向
     * @param extending 是否伸出
     * @return 如果移动成功返回 true
     */
    bool _doMove(IWorld& world, const BlockPos& pos, Direction facing, bool extending);

    /**
     * @brief 检查方块是否可以被推动
     *
     * @param state 方块状态
     * @return PushReaction 方块的推动反应
     */
    [[nodiscard]] Material::PushReaction _getBlockPushReaction(const BlockState& state) const;
};

} // namespace blocks
} // namespace mc
