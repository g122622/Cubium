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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/GameMasterBlock.hpp"

#include <memory>

namespace mc {

class IWorld;
class Player;
class BlockItemUseContext;

namespace blockentity {
class CommandBlockEntity;
}

namespace blocks {

/**
 * @brief 命令方块基类
 *
 * 方块形式的命令执行器。
 *
 * 状态属性：
 * - FACING: 朝向
 * - CONDITIONAL: 是否有条件
 * - POWERED: 是否被激活
 */
class CommandBlock : public Block, public GameMasterBlock {
public:
    explicit CommandBlock(const BlockProperties& properties);
    ~CommandBlock() override = default;

    // ========== 标记接口 ==========

    [[nodiscard]] bool isGameMaster() const noexcept override { return true; }

    // ========== 状态属性 ==========

    [[nodiscard]] Direction getFacing(const BlockState& state) const;
    [[nodiscard]] bool isConditional(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 红石 ==========

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // ========== 比较器输出 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 交互 ==========

    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

protected:
    /**
     * @brief 执行命令
     *
     * 执行命令方块的命令，并触发连锁执行。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param commandEntity 命令方块实体（可为nullptr）
     */
    void execute(
        IWorld& world, const BlockPos& pos, const BlockState& state, blockentity::CommandBlockEntity* commandEntity);

    /**
     * @brief 触发连锁命令方块
     *
     * 沿着 FACING 方向触发连锁命令方块。
     *
     * @param world 世界引用
     * @param pos 当前命令方块位置
     * @param facing 连锁方向
     */
    void executeChain(IWorld& world, const BlockPos& pos, Direction facing);
};

} // namespace blocks
} // namespace mc
