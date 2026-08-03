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
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"
#include <array>
#include <cstddef>
#include <map>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 墙方块
 *
 * 支持与相邻墙/栅栏连接，并自动调整高度。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 状态属性：
 * - UP: 是否有顶部突出
 * - NORTH/WEST/EAST/SOUTH: 各方向连接高度 (NONE, LOW, TALL)
 * - WATERLOGGED: 是否含水
 */
class WallBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit WallBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~WallBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 检查一个形状是否完全覆盖另一个形状
     *
     * 使用 VoxelShape 布尔运算: !Shapes.joinIsNotEmpty(testShape, coverShape, OnlyFirst)
     * 含义: 如果 testShape 中没有任何部分不被 coverShape 覆盖，则返回 true。
     *
     * @param testShape 被测试的形状（需要被覆盖的区域）
     * @param coverShape 覆盖形状
     * @return 如果 coverShape 完全覆盖 testShape 则返回 true
     */
    [[nodiscard]] static bool isCovered(const VoxelShape& testShape, const VoxelShape& coverShape);

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 墙有复杂的形状，需要精确的形状遮挡检测。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 判断方块是否为墙
     * @param state 方块状态
     * @return 如果是墙返回 true
     */
    [[nodiscard]] static bool isWall(const BlockState& state);

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 红石连接 ==========

    [[nodiscard]] bool canConnectRedstone(const BlockState& state, Direction side) const noexcept override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

private:
    /**
     * @brief 计算连接状态
     * @param world 世界
     * @param pos 位置
     * @param state 当前状态
     * @return 更新后的状态
     */
    [[nodiscard]] BlockState _calculateState(const IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查邻居是否连接到墙，以及连接高度
     *
     * 连接逻辑:
     * 1. 其他墙 -> 总是连接
     * 2. 栅栏门平行时 -> 连接
     * 3. 铁栏杆 -> 连接
     * 4. 固体方块（非连接例外）-> 连接
     *
     * 连接高度的确定:
     * - 如果上方方块的碰撞形状下方面覆盖了对应方向的测试形状，则连接为 Tall
     * - 否则连接为 Low
     *
     * @param state 邻居状态
     * @param neighborSide 邻居相对于墙的方向
     * @param aboveFaceShape 上方方块碰撞形状的下方面投影
     * @return 连接高度
     */
    [[nodiscard]] BlockStateProperties::WallHeight _getWallHeight(
        const BlockState& state, Direction neighborSide, const VoxelShape& aboveFaceShape) const;

    /**
     * @brief 判断墙柱(UP)是否应该升起
     *
     * 综合考虑四方向连接高度和上方方块状态决定是否显示墙柱。
     *
     * @param state 当前墙状态
     * @param northHeight 北面连接高度
     * @param eastHeight 东面连接高度
     * @param southHeight 南面连接高度
     * @param westHeight 西面连接高度
     * @param world 世界
     * @param pos 墙位置
     * @return 如果应该升起墙柱返回 true
     */
    [[nodiscard]] bool _shouldRaisePost(const BlockState& state,
        BlockStateProperties::WallHeight northHeight,
        BlockStateProperties::WallHeight eastHeight,
        BlockStateProperties::WallHeight southHeight,
        BlockStateProperties::WallHeight westHeight,
        const IWorld& world,
        const BlockPos& pos) const;

    /**
     * @brief 获取形状索引
     * @param up 是否有顶部
     * @param north 北面高度
     * @param east 东面高度
     * @param south 南面高度
     * @param west 西面高度
     * @return 形状索引
     */
    [[nodiscard]] static size_t _getShapeIndex(bool up,
        BlockStateProperties::WallHeight north,
        BlockStateProperties::WallHeight east,
        BlockStateProperties::WallHeight south,
        BlockStateProperties::WallHeight west);

    /// 基础墙形状（无顶部）
    CollisionShape m_baseShape;

    /// 墙柱形状（中间）
    CollisionShape m_pillarShape;

    /// 预计算的形状缓存
    std::array<CollisionShape, 162> m_shapes; // 2(up) * 3(north) * 3(east) * 3(south) * 3(west)

    /// 墙柱测试形状: 中心2x2像素的柱形区域 (7/16, 0, 7/16) -> (9/16, 1, 9/16)
    /// 墙柱测试形状: 中心2x2像素的柱形区域 (7/16, 0, 7/16) -> (9/16, 1, 9/16)
    static VoxelShape s_testShapePost;

    /// 墙臂测试形状: 每个方向对应一个2x16x9像素的墙臂区域
    /// 北面 (旋转前): (7/16, 0, 0) -> (9/16, 1, 9/16)
    static std::map<Direction, VoxelShape> s_testShapesWall;
};

} // namespace blocks
} // namespace mc
