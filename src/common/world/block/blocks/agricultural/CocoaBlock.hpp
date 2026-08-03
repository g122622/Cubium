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

#include "../../IGrowable.hpp"
#include "../HorizontalBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>

namespace mc {

class IBlockReader;
class IWorld;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 可可豆方块
 *
 * 生长在丛林原木侧面的植物方块。
 * 有三个生长阶段 (AGE 0-2)，可使用骨粉催熟。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向（指向丛林原木的方向）
 * - AGE_0_2: 生长阶段 (0-2)
 */
class CocoaBlock : public HorizontalBlock, public IGrowable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CocoaBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~CocoaBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取 AGE 属性
     */
    static const IntegerProperty& AGE() { return BlockStateProperties::AGE_0_2(); }

    /**
     * @brief 获取最大年龄
     */
    [[nodiscard]] static constexpr i32 getMaxAge() { return 2; }

    /**
     * @brief 获取当前年龄
     */
    [[nodiscard]] i32 getAge(const BlockState& state) const;

    /**
     * @brief 创建指定年龄的状态
     */
    [[nodiscard]] const BlockState& withAge(const BlockState& state, i32 age) const;

    /**
     * @brief 是否为最大年龄
     */
    [[nodiscard]] bool isMaxAge(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 邻居更新 ==========

    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== IGrowable 接口 ==========

    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 其他 ==========

    /**
     * @brief 不阻挡实体移动
     */
    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

protected:
    /**
     * @brief 创建状态容器
     */
    void createBlockStateInternal();

private:
    /// 各方向各年龄的形状缓存
    /// [DirectionIndex][Age] - DirectionIndex: North=0, South=1, West=2, East=3
    std::array<std::array<CollisionShape, 3>, 4> m_shapesByDirectionAndAge;

    /**
     * @brief 初始化形状缓存
     */
    void _initShapes();

    /**
     * @brief 检查是否可以附着在指定方向
     */
    [[nodiscard]] bool _canAttachTo(IBlockReader& world, const BlockPos& pos, Direction facing) const;
};

} // namespace blocks
} // namespace mc
