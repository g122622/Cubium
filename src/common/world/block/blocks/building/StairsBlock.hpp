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

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"
#include <array>
#include <cstddef>
#include <optional>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 楼梯方块
 *
 * 支持内角/外角自动检测和连接。
 * 实现 IWaterLoggable 接口以支持含水功能。
 *
 * 状态属性：
 * - FACING: 楼梯朝向 (NORTH, SOUTH, EAST, WEST) - 楼梯上升的方向
 * - HALF: 上半/下半 (TOP, BOTTOM) - 楼梯是正放还是倒放
 * - SHAPE: 楼梯形状 (STRAIGHT, INNER_LEFT, INNER_RIGHT, OUTER_LEFT, OUTER_RIGHT)
 * - WATERLOGGED: 是否含水
 */
class StairsBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param baseState 源方块状态（用于继承属性）
     * @param properties 方块属性
     */
    StairsBlock(const BlockState& baseState, const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~StairsBlock() override = default;

    // ========== 状态容器 ==========

    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 楼梯有复杂的形状，需要精确的形状遮挡检测。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 其他 ==========

    /**
     * @brief 检查是否为楼梯
     * @param state 方块状态
     * @return 如果是楼梯返回true
     */
    [[nodiscard]] static bool isStairs(const BlockState& state);

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     *
     * 如果方块含水，返回水的流体状态。
     *
     * @param state 方块状态
     * @return 流体状态指针
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     *
     * @param state 方块状态
     * @return 是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

private:
    /**
     * @brief 邻居楼梯信息
     *
     * 当指定方向上的邻居是同层且朝向垂直的楼梯时，
     * 携带该邻居的朝向信息，用于后续角形状判定。
     */
    struct _NeighborStairsInfo {
        Direction facing; ///< 邻居楼梯的朝向
    };

    /**
     * @brief 计算楼梯形状
     * @param state 当前状态
     * @param world 世界
     * @param pos 位置
     * @return 楼梯形状
     */
    [[nodiscard]] static BlockStateProperties::StairsShape _calculateShape(
        const BlockState& state, IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查邻居是否为符合条件的楼梯
     *
     * 检查指定方向上的邻居方块是否为同层楼梯且朝向垂直。
     * 如果是，返回包含邻居朝向信息的结构；否则返回 nullopt。
     *
     * @param state 当前方块状态
     * @param world 世界
     * @param pos 当前方块位置
     * @param facing 检查方向
     * @return 如果邻居是符合条件的楼梯返回其朝向信息，否则返回nullopt
     */
    [[nodiscard]] static std::optional<_NeighborStairsInfo> _neighborIsStairs(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction facing);

    /**
     * @brief 检查指定方向是否有不同的楼梯（用于角形状判定）
     *
     * 检查 pos + face 方向上的方块是否为与本楼梯朝向或层次不同的楼梯。
     * 如果该方向不是同朝向同层次的楼梯，返回 true（可以形成角形状）。
     *
     * @param state 当前方块状态
     * @param world 世界
     * @param pos 当前位置
     * @param face 检查方向
     * @return 如果该方向不是同朝向同层次的楼梯返回true
     */
    [[nodiscard]] static bool _isDifferentStairs(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction face);

    /**
     * @brief 获取状态索引
     * @param state 方块状态
     * @return 状态索引 (0-19)
     */
    [[nodiscard]] static size_t _getStateIndex(const BlockState& state);

    /**
     * @brief 根据状态获取形状
     * @param state 方块状态
     * @return 形状的常量引用
     */
    [[nodiscard]] const CollisionShape& _getShapeForState(const BlockState& state) const;

    /// 源方块状态（用于继承属性如硬度、抗性等）
    const BlockState* m_baseState;

    /// 完整方块形状（用于双层台阶）
    CollisionShape m_fullCubeShape;
};

} // namespace blocks
} // namespace mc
