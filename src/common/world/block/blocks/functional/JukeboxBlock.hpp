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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"
#include <memory>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockEntity;
class BlockRaycastResult;

namespace blocks {

/**
 * @brief 唱片机方块
 *
 * 可以播放音乐唱片的功能方块。玩家手持唱片右键放入唱片开始播放，
 * 空手右键取出唱片停止播放。
 *
 * 状态属性：
 * - HAS_RECORD: 是否有唱片
 *
 * 红石：
 * - 比较器信号强度由唱片类型决定（1-15）
 * - 正在播放时直接输出信号强度15（isSignalSource）
 *
 */
class JukeboxBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit JukeboxBlock(const BlockProperties& properties);
    ~JukeboxBlock() noexcept override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 红石 ==========

    /**
     * @brief 是红石信号源（对齐 vanilla `JukeboxBlock#isSignalSource`，恒 true）
     *
     * 唱片机在播放唱片时向上方与四周输出强度 15 的弱信号（见 getWeakPower）。
     * 早期漏了这个重写，唱片机因此在红石线计算里不被当作信号源。
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 弱信号输出（对齐 vanilla `JukeboxBlock#getSignal`）
     *
     * 正在播放唱片时输出 15，否则 0。
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // ========== 交互 ==========

    /**
     * @brief 处理玩家右键交互
     *
     * 有唱片时取出唱片，无唱片且玩家手持唱片时放入唱片。
     */
    [[nodiscard]] BlockActionResult useItemOn(ItemStack& stack,
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    [[nodiscard]] BlockActionResult useWithoutItem(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 方块移除时回调
     *
     * 掉落唱片机内的唱片。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    // ========== 工具方法 ==========

    /**
     * @brief 检查是否有唱片
     */
    [[nodiscard]] static bool hasRecord(const BlockState& state)
    {
        return state.get(BlockStateProperties::HAS_RECORD());
    }

protected:
    /// 唱片机形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
