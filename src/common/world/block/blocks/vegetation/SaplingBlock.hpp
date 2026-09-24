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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include <functional>

namespace mc {

namespace blocks {

// 前向声明
class ITreeConfig;

/**
 * @brief 树苗方块
 *
 * 可以生长成树木的树苗。
 * 使用 STAGE_0_1 属性表示生长阶段。
 * 当阶段达到最大值时，在合适的条件下会生长成树。
 *
 * 树木生成通过 TreeGenerator 回调实现。SaplingBlock::grow() 内部
 * 通过 IWorld::createFeatureRegion() 从 ServerWorld 的已加载区块
 * 构建临时区域（以 IWorld 接口返回），然后将该区域传递给 TreeGenerator。
 */
class SaplingBlock : public BushBlock, public IGrowable {
public:
    /**
     * @brief 树木生成器函数类型
     *
     * 接收 IWorld& —— 实际运行时由 ServerWorld::createFeatureRegion()
     * 返回的 WorldGenRegion（继承自 IWorld）。lambda 内部在 server 侧
     * 将 IWorld& static_cast 为 WorldGenRegion& 后调用 TreeFeature::place()。
     *
     * @param world 临时构建的区域（以 IWorld 接口暴露）
     * @param pos 树苗位置
     * @param random 随机数生成器
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
    ~SaplingBlock() noexcept override = default;

    /**
     * @brief 后置注入树木生成器
     *
     * common 层方块注册时无法引用 server gen 的 TreeFeature，
     * 因此构造时传入空 generator，由 server 侧初始化阶段调用此方法
     * 注入真实的 TreeGenerator（捕获 TreeFeature 并调用其 place()）。
     */
    void setTreeGenerator(TreeGenerator treeGenerator) { m_treeGenerator = std::move(treeGenerator); }

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
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    /**
     * @brief 尝试生长
     *
     * 通过 IWorld::createFeatureRegion() 从已加载区块构建临时区域，
     * 然后调用 TreeGenerator 进行树木生成。
     *
     * @param world 世界
     * @param pos 位置
     * @param state 状态
     * @return 如果成功生长返回true
     */
    bool grow(IWorld& world, const BlockPos& pos, BlockState& state);

    // ========== IGrowable 接口 ==========

    /**
     * @brief 检查是否可以生长（骨粉可用）
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 检查是否可以使用骨粉
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

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
