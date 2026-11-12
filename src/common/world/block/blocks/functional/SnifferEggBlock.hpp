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

#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/core/Block.hpp"

#include <memory>

namespace mc {

namespace blocks {

/**
 * @brief 嗅探兽蛋方块
 *
 * 可孵化出嗅探兽生物的蛋方块。
 * 状态属性：HATCH (0-2)
 *
 * 孵化机制（对齐 MC 1.21.11 SnifferEggBlock）：
 * - 放置时（onBlockAdded）通过 scheduleTick 调度孵化 tick。
 * - 调度延迟：常规 24000/3 + [0, 300) tick；加速 12000/3 + [0, 300) tick。
 * - 加速条件：下方方块在 BlockTags::SNIFFER_EGG_HATCH_BOOST 标签中（如苔藓块）。
 * - tick 回调推进 HATCH 等级：0→1→2，到 2 时孵化完成生成嗅探兽幼体。
 * - 加速放置时广播 ::mc::world::LevelEventId::PARTICLES_EGG_CRACK (3009) 粒子事件。
 * - 不再依赖 randomTick，因此 ticksRandomly() 返回 false。
 */
class SnifferEggBlock : public Block {
public:
    explicit SnifferEggBlock(const BlockProperties& properties);

    ~SnifferEggBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 恒不可通行
     *
     * 原版 `SnifferEggBlock#isPathfindable` 显式返回 false：蛋的碰撞形状随 HATCH 阶段变化且
     * 始终不满格，默认判据会让生物把蛋当台阶踩上去。
     */
    [[nodiscard]] bool isPathfindable(const BlockState& state, PathComputationType type) const override;

    /**
     * @brief 方块放置时调度孵化 tick
     *
     * 检测下方方块是否在 SNIFFER_EGG_HATCH_BOOST 标签中，决定孵化总时长
     * （12000 或 24000 tick），分三阶段调度（i / 3 + [0, 300) tick）。
     * 加速时广播 EGG_CRACK 粒子事件，并发出 BLOCK_PLACE 游戏事件。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    /**
     * @brief 计划刻回调 - 推进孵化进度
     *
     * HATCH < 2：播放 SNIFFER_EGG_CRACK 音效并 +1 等级。
     * HATCH = 2：播放 SNIFFER_EGG_HATCH 音效，销毁蛋方块，生成嗅探兽幼体。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 检查下方方块是否为孵化加速方块
     *
     * @param world 方块查询接口
     * @param pos 蛋方块位置
     * @return 下方方块是否在 SNIFFER_EGG_HATCH_BOOST 标签中
     */
    [[nodiscard]] static bool hatchBoost(IWorld& world, const BlockPos& pos);

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_noCrackShape;
    CollisionShape m_crackedShape;
    CollisionShape m_hatchingShape;
};

} // namespace blocks
} // namespace mc
