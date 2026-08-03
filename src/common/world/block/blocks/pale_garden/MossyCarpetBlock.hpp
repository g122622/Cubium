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
#include "../../Block.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <cstddef>
#include <vector>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 苔藓地毯方块（pale_moss_carpet）
 *
 * 可附着在方块侧面/底面的苔藓地毯。BASE（bottom）为 true 时是平铺在地面上的底座，
 * 否则是悬挂在方块面（北/东/南/西）上的薄片，通过 WALL_HEIGHT 属性区分 LOW/TALL。
 *
 * 状态属性：
 * - BOTTOM：是否为底座（平铺地面）
 * - NORTH/EAST/SOUTH/WEST：各水平方向的附着高度 (NONE/LOW/TALL)
 *
 * 放置与更新逻辑对齐 MC 1.21.11 MossyCarpetBlock：
 * - canSurvive：底座需下方非空气；非底座需下方为同类型底座方块。
 * - getUpdatedState：根据各水平方向相邻方块是否可附着，决定该方向 LOW/NONE；
 *   若上方同类型方块该方向非 NONE 且自身非底座，则升级为 TALL。
 * - 无任何面且非底座时销毁为空气。
 *
 * MC ID: minecraft:pale_moss_carpet
 *
 * 参考: net.minecraft.world.level.block.MossyCarpetBlock
 */
class MossyCarpetBlock : public Block {
public:
    using WallHeight = BlockStateProperties::WallHeight;

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit MossyCarpetBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~MossyCarpetBlock() override = default;

    // ========== 放置与更新 ==========

    /**
     * @brief 获取放置状态（按相邻方块更新各方向附着高度）
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 邻居更新：重新计算各方向附着，无面则销毁
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 是否可存活（canSurvive）
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 旋转/镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    /// 各水平方向到 WALL_HEIGHT 属性的映射（北/东/南/西）。
    static const EnumProperty<WallHeight>& _propertyForDirection(Direction direction);

    /// 是否存在任意面或底座（无面且非底座时方块应销毁）。
    [[nodiscard]] static bool _hasFaces(const BlockState& state);

    /// 相邻方块在 direction 方向是否可附着（对齐 MC canSupportAtFace）。
    [[nodiscard]] static bool _canSupportAtFace(IWorld& world, const BlockPos& pos, Direction direction);

    /// 重新计算各水平方向附着高度（对齐 MC getUpdatedState）。
    [[nodiscard]] BlockState _getUpdatedState(
        BlockState state, IWorld& world, const BlockPos& pos, bool includeBase) const;

    /// 按状态索引缓存全部形状（BOTTOM×4方向×3高度 = 2×3^4 = 162 种）。
    void _initShapes();

    /// 由 BOTTOM 与四方向高度计算扁平索引（对齐 WallBlock._getShapeIndex 思路）。
    [[nodiscard]] static size_t _shapeIndex(
        bool bottom, WallHeight north, WallHeight east, WallHeight south, WallHeight west);

    std::vector<CollisionShape> m_shapes;
};

} // namespace blocks
} // namespace mc
