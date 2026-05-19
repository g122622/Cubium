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
#include "../agricultural/BushBlock.hpp"
#include <functional>

namespace mc {
namespace blocks {

// Forward declaration
class ITreeConfig;

/**
 * @brief 树苗方块
 *
 * 可以生长成树木的树苗。
 * 使用 STAGE_0_1 属性表示生长阶段。
 * 当阶段达到最大值时，在合适的条件下会生长成树。
 *
 * 参考: net.minecraft.block.SaplingBlock
 */
class SaplingBlock : public BushBlock {
public:
    /**
     * @brief 树木生成器函数类型
     */
    using TreeGenerator = std::function<void(IWorld&, const BlockPos&, math::IRandom&)>;

    /**
     * @brief 构造函数
     * @param treeGenerator 树木生成器
     * @param properties 方块属性
     */
    SaplingBlock(TreeGenerator treeGenerator, const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~SaplingBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取阶段
     */
    [[nodiscard]] i32 getStage(const BlockState& state) const;

    /**
     * @brief 创建指定阶段的状态
     */
    [[nodiscard]] const BlockState& withStage(i32 stage) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 生长逻辑 ==========

    /**
     * @brief 随机刻
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否需要随机 tick
     */
    [[nodiscard]] bool ticksRandomly() const override { return true; }

    /**
     * @brief 尝试生长
     * @param world 世界
     * @param pos 位置
     * @param state 状态
     * @return 如果成功生长返回true
     */
    bool grow(IWorld& world, const BlockPos& pos, BlockState& state);

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /**
     * @brief 检查下方是否可支撑
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

    /// 树木生成器
    TreeGenerator m_treeGenerator;
};

} // namespace blocks
} // namespace mc
