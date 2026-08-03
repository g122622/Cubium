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

#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 红石矿石方块
 *
 * 玩家攻击或踩踏时点亮，随机刻后熄灭。
 * 点亮时发出光照等级9。
 * lit 状态通过 LIT 属性区分，redstone_ore 与 deepslate_redstone_ore 共用本类。
 *
 * 参考: net.minecraft.block.RedStoneOreBlock
 */
class RedstoneOreBlock : public Block {
public:
    explicit RedstoneOreBlock(const BlockProperties& properties);

    ~RedstoneOreBlock() override = default;

    /**
     * @brief 获取发光等级 - 点亮时为9，熄灭时为0
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return state.get(BlockStateProperties::LIT()) ? 9 : 0;
    }

    /**
     * @brief 玩家攻击时点亮矿石
     */
    void attack(const BlockState& state, IWorld& world, const BlockPos& pos, Player& player) override;

    /**
     * @brief 实体踩踏时点亮矿石
     */
    void onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    /**
     * @brief 随机刻 - 熄灭点亮的矿石
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    /**
     * @brief 点亮矿石并调度熄灭tick
     */
    void interact(IWorld& world, const BlockPos& pos, BlockState& state) const;
};

} // namespace blocks
} // namespace mc
