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
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 海绵方块
 *
 * 可以吸收水的方块。
 *
 * 吸水机制：
 * - 从海绵位置开始 BFS 搜索周围的水方块
 * - 最大搜索深度：6 格
 * - 最大吸收数量：65 个水方块
 * - 处理三种类型的水：水源、流动水、海洋植物
 * - 成功吸水后变成湿润海绵
 *
 * 状态属性：无（湿润状态是不同的方块）
 */
class SpongeBlock : public Block {
public:
    explicit SpongeBlock(const BlockProperties& properties);
    ~SpongeBlock() override = default;

    // ========== 吸水 ==========

    /**
     * @brief 尝试吸水
     *
     * 从海绵位置开始 BFS 搜索周围的水方块并吸收。
     * 成功吸水后将海绵变为湿润海绵。
     *
     * @param world 世界
     * @param pos 海绵位置
     * @return 如果吸收了水返回 true
     */
    bool tryAbsorbWater(IWorld& world, const BlockPos& pos);

    // ========== 方块回调 ==========

    /**
     * @brief 方块被放置时的处理
     *
     * 放置时尝试吸水。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居方块更新
     *
     * 当邻居方块改变时尝试吸水。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param neighborBlock 邻居方块
     * @param neighborPos 邻居位置
     * @param isMoving 是否正在移动
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

private:
    /// 海绵吸水最大搜索深度
    static constexpr i32 MAX_ABSORB_DEPTH = 6;

    /// 海绵吸水最大吸收数量
    static constexpr i32 MAX_ABSORB_COUNT = 65;

    /**
     * @brief 执行吸水 BFS 搜索
     *
     * @param world 世界
     * @param pos 海绵位置
     * @return 吸收的水方块数量
     */
    i32 absorb(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
