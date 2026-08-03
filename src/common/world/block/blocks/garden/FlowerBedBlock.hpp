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
 */

#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../IGrowable.hpp"
#include "../agricultural/BushBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/DirectionProperty.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <vector>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 花瓣床方块
 *
 * 可堆叠放置的地面装饰花（粉红色花瓣、野花）。
 * 具有 FACING（水平朝向）和 FLOWER_AMOUNT（1-4）两个状态属性。
 *
 * ## 放置行为
 * - 首次放置：根据玩家朝向设置 FACING，AMOUNT=1
 * - 右键已有花瓣床：AMOUNT+1（最多4），不消耗 FACING
 * - 潜行右键：正常放置新方块而非堆叠
 *
 * ## 骨粉行为
 * - AMOUNT < 4：增加1
 * - AMOUNT = 4：弹出一个自身物品
 *
 * ## 掉落
 * - 破坏时掉落 AMOUNT 数量的自身物品
 *
 * ## 形状
 * - 每个花瓣段为 8x3x8 像素的盒子
 * - AMOUNT=1：一段，朝 FACING 方向
 * - AMOUNT=2：两段，FACING 和逆时针90度
 * - AMOUNT=3：三段，L形排列
 * - AMOUNT=4：四段，填满整个方块
 *
 * MC ID: minecraft:flower_bed（粉红色花瓣和野花共享此方块类型）
 *
 * 野花（wildflowers）通过 WildflowerFeature 在世界生成时放置，
 * 初始 AMOUNT 为 1-4 随机（与 FACING 共 16 种状态等权重）。
 * 野花的 configured_feature 由数据包 JSON 驱动（如 minecraft:wildflowers_birch_forest、
 * minecraft:wildflowers_meadow），由 ConfiguredFeatureLoader 加载。
 *
 * 参考: net.minecraft.block.FlowerBedBlock
 */
class FlowerBedBlock : public BushBlock, public IGrowable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FlowerBedBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~FlowerBedBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取水平朝向属性
     */
    static const DirectionProperty& FACING() { return BlockStateProperties::HORIZONTAL_FACING(); }

    /**
     * @brief 获取花瓣数量属性
     */
    static const IntegerProperty& AMOUNT() { return BlockStateProperties::FLOWER_AMOUNT(); }

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 如果目标位置已有同类型花瓣床，增加 AMOUNT；
     * 否则创建新方块，FACING 为玩家朝向的反方向。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查方块是否可被替换（用于堆叠放置）
     *
     * 当玩家未潜行、手持同类型物品且 AMOUNT < 4 时返回 true。
     */
    [[nodiscard]] bool isReplaceable(const BlockState& state, const BlockItemUseContext& context) const override;

    // ========== 旋转/镜像 ==========

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 骨粉 ==========

    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状（根据 FACING 和 AMOUNT 变化）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 掉落 ==========

    // 花瓣床的掉落数量由战利品表根据 flower_amount 属性决定
    // 数据包中的战利品表已正确配置：掉落数量 = AMOUNT 值

private:
    /**
     * @brief 初始化所有方块状态的形状
     *
     * 根据 FACING 和 AMOUNT 组合计算形状：
     * - 单个花瓣段为 8x3x8 像素盒子（0-8/16, 0-3/16, 0-8/16）
     * - 每增加一个 AMOUNT，逆时针旋转90度叠加一个段
     * - 共 4x4=16 种形状
     */
    void _initShapes();

    /**
     * @brief 计算指定朝向和数量的形状
     * @param facing 朝向
     * @param amount 花瓣数量（1-4）
     * @return 碰撞形状
     */
    [[nodiscard]] static CollisionShape _calculateShape(Direction facing, i32 amount);

    /**
     * @brief 获取花瓣数量的状态值
     */
    [[nodiscard]] i32 _getAmount(const BlockState& state) const;

    /**
     * @brief 获取朝向的状态值
     */
    [[nodiscard]] Direction _getFacing(const BlockState& state) const;

    /// 按状态索引存储的形状缓存（16种组合：4方向 x 4数量）
    std::vector<CollisionShape> m_shapes;
};

} // namespace blocks
} // namespace mc
