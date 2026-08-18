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

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../agricultural/BushBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/DirectionProperty.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <vector>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 枯叶方块
 *
 * 地面装饰枯叶，可分段堆叠放置（1-4 段）。
 * 具有 FACING（水平朝向）和 SEGMENT_AMOUNT（1-4）两个状态属性。
 *
 * ## 放置行为
 * - 首次放置：根据玩家朝向设置 FACING，SEGMENT_AMOUNT=1
 * - 右键已有枯叶：SEGMENT_AMOUNT+1（最多4），保持 FACING
 * - 潜行右键：正常放置新方块而非堆叠
 *
 * ## 支撑判定
 * - 可放置在任意顶面支撑形状完整（isFaceSturdy(Up, Full)）的方块上表面，
 *   包括草方块、泥土、石头、沙子、陶瓦、玻璃等顶面完整的方块。
 * - 重写 canSustain 用 isFaceSturdy(Up, Full) 判定，不沿用 BushBlock 的 DIRT 标签判定。
 *
 * ## 形状
 * - 每个枯叶段为 8x1x8 像素的盒子（高度 1/16）
 * - 1 段：朝 FACING 方向
 * - 2 段：两段，FACING 和逆时针90度
 * - 3 段：三段，L形排列
 * - 4 段：四段，填满整个方块
 *
 * MC ID: minecraft:leaf_litter
 */
class LeafLitterBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LeafLitterBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~LeafLitterBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取水平朝向属性
     */
    static const DirectionProperty& FACING() { return BlockStateProperties::HORIZONTAL_FACING(); }

    /**
     * @brief 获取枯叶段数属性
     */
    static const IntegerProperty& SEGMENT_AMOUNT() { return BlockStateProperties::SEGMENT_AMOUNT(); }

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 如果目标位置已有同类型枯叶，增加 SEGMENT_AMOUNT；
     * 否则创建新方块，FACING 为玩家朝向的反方向。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查方块是否可被替换（用于堆叠放置）
     *
     * 当玩家未潜行、手持同类型物品且 SEGMENT_AMOUNT < 4 时返回 true。
     */
    [[nodiscard]] bool isReplaceable(const BlockState& state, const BlockItemUseContext& context) const override;

    /**
     * @brief 检查下方方块能否支撑枯叶
     *
     * 重写 BushBlock::canSustain：用下方方块顶面是否支撑形状完整（isFaceSturdy(Up, Full)）
     * 判定，而非 BushBlock 默认的 DIRT 标签判定。这样枯叶可放置在任意顶面完整的方块上
     * （草方块/泥土/石头/沙子/陶瓦/玻璃等），与原版行为一致。
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

    // ========== 旋转/镜像 ==========

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状（根据 FACING 和 SEGMENT_AMOUNT 变化）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    /**
     * @brief 初始化所有方块状态的形状
     *
     * 根据 FACING 和 SEGMENT_AMOUNT 组合计算形状：
     * - 单个枯叶段为 8x1x8 像素盒子（0-0.5, 0-1/16, 0-0.5）
     * - 每增加一个段，逆时针旋转90度叠加
     * - 共 4x4=16 种形状
     */
    void _initShapes();

    /**
     * @brief 计算指定朝向和段数的形状
     * @param facing 朝向
     * @param amount 枯叶段数（1-4）
     * @return 碰撞形状
     */
    [[nodiscard]] static CollisionShape _calculateShape(Direction facing, i32 amount);

    /**
     * @brief 获取枯叶段数的状态值
     */
    [[nodiscard]] i32 _getAmount(const BlockState& state) const;

    /**
     * @brief 获取朝向的状态值
     */
    [[nodiscard]] Direction _getFacing(const BlockState& state) const;

    /// 按状态索引存储的形状缓存（16种组合：4方向 x 4段数）
    std::vector<CollisionShape> m_shapes;
};

} // namespace blocks
} // namespace mc
