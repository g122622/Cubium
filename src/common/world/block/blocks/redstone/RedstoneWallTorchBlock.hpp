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

#include "../../../../util/Direction.hpp"
#include "../../../../util/property/Properties.hpp"
#include "RedstoneTorchBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 墙上的红石火把方块
 *
 * 附着在墙上的红石火把，功能与普通红石火把相同，
 * 但附着方向不同（水平方向而非垂直向上）。
 *
 * ## 特性
 * - 信号反转：附着面有信号时熄灭
 * - 输出强度：点亮时15，熄灭时0
 * - 弱信号输出：向除附着面外的所有方向输出
 * - 可旋转
 *
 * ## 容易踩的坑
 * - 不向附着面输出信号
 * - shouldBeOff 检查附着面方向的信号
 * - 需要处理支撑方块检测
 *
 * 参考: net.minecraft.block.RedstoneWallTorchBlock
 */
class RedstoneWallTorchBlock : public RedstoneTorchBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneWallTorchBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 墙红石火把特有方法 ==========

    /**
     * @brief 获取火把朝向
     *
     * @param state 方块状态
     * @return Direction 朝向方向（火把指向的方向）
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 设置火把朝向
     *
     * @param state 方块状态
     * @param facing 朝向
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withFacing(BlockState state, Direction facing);

    /**
     * @brief 检查火把是否应该熄灭
     *
     * 当附着方块被充能时，火把应该熄灭。
     *
     * @param world 世界引用
     * @param pos 火把位置
     * @param state 当前方块状态
     * @return true 如果应该熄灭
     */
    [[nodiscard]] bool shouldBeOff(IWorld& world, const BlockPos& pos, const BlockState& state) const;

private:
    /**
     * @brief 检查是否可以放置在指定位置
     *
     * @param world 世界引用
     * @param pos 火把位置
     * @param facing 火把朝向
     * @return true 如果可以放置
     */
    [[nodiscard]] bool _canPlaceAt(IWorld& world, const BlockPos& pos, Direction facing) const;

    /**
     * @brief 更新火把状态
     *
     * @param world 世界引用
     * @param pos 火把位置
     * @param state 当前方块状态
     */
    void _updateState(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
