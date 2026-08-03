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

#include <array>
#include <cstddef>

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
#include "../../Material.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 玻璃板/铁栏杆基类
 *
 * 玻璃板和铁栏杆共享相同的连接逻辑：
 * - 根据相邻方块自动连接
 * - 四个方向的布尔属性
 * - 支持水logged状态
 * - 同类方块或BARS标签方块之间互相连接时跳过连接面渲染
 */
class PaneBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit PaneBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     */
    BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 更新 ==========

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    /**
     * @brief 获取碰撞形状
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取形状（用于渲染）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 玻璃板/铁栏杆有复杂的形状，需要精确的形状遮挡检测。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 判断与邻居方块之间的面是否应该跳过渲染
     *
     * 实现逻辑：
     * - 同类方块（同一 Block 实例）之间：垂直方向始终跳过，水平方向仅当双方都连接时跳过
     * - BARS 标签方块之间（如铁栏杆↔铜栏杆）：同上，垂直方向始终跳过，
     *   水平方向仅当双方都有对应方向的连接属性时跳过
     *
     * 参考: net.minecraft.world.level.block.IronBarsBlock#skipRendering
     */
    [[nodiscard]] bool skipRendering(
        const BlockState& selfState, const BlockState& neighborState, Direction direction) const override;

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const noexcept override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 属性访问 ==========

    /**
     * @brief 检查是否连接到指定方向
     * @param state 方块状态
     * @param facing 方向
     * @return 如果连接返回true
     */
    [[nodiscard]] static bool connectsTo(const BlockState& state, Direction facing) noexcept;

protected:
    /**
     * @brief 检查是否应该连接到相邻方块
     * @param world 世界引用
     * @param pos 本方块位置
     * @param neighborState 邻居方块状态
     * @param direction 方向
     * @return 如果应该连接返回true
     */
    [[nodiscard]] bool shouldConnectTo(
        IWorld& world, const BlockPos& pos, const BlockState& neighborState, Direction direction) const;

    /**
     * @brief 计算形状索引
     */
    [[nodiscard]] static size_t getShapeIndex(bool north, bool east, bool south, bool west) noexcept;

    /**
     * @brief 将方向映射到对应的布尔属性（NORTH/SOUTH/EAST/WEST）
     */
    [[nodiscard]] static const BooleanProperty& _directionToProperty(Direction direction) noexcept;

    /// 中心柱形状
    CollisionShape m_centerShape;
    /// 边缘形状
    CollisionShape m_sideShapes[6];
    /// 组合形状缓存
    std::array<CollisionShape, 16> m_shapes;
};

} // namespace blocks
} // namespace mc
