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

#include "HorizontalBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/IWaterLoggable.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 书架方块
 *
 * 1.21.4+ 新增的木质书架方块，可放置3个物品（任意物品，不限于书籍）。
 * 继承自 HorizontalBlock，实现 IWaterLoggable 接口。
 *
 * 方块状态属性：
 * - FACING: 朝向（北/南/东/西），继承自 HorizontalBlock
 * - POWERED: 红石充能状态
 * - SIDE_CHAIN_PART: 侧链连接部分（UNCONNECTED/LEFT/CENTER/RIGHT）
 * - WATERLOGGED: 是否含水
 *
 * 红石比较器输出：3位二进制编码，每个槽位占用对应1位（0-7）。
 * 需要配合 ShelfBlockEntity 使用。
 *
 * 注意：Shelf 方块不能为附魔台提供附魔能量（与 Bookshelf 不同）。
 *
 * 参考: net.minecraft.block.ShelfBlock (MC 1.21.11)
 */
class ShelfBlock : public HorizontalBlock, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ShelfBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~ShelfBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家朝向设置 FACING，根据水中位置设置 WATERLOGGED。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 邻居方块更新
     *
     * 处理 WATERLOGGED 的流体 tick 调度。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 光照 ==========

    /**
     * @brief 是否使用形状进行光照遮挡
     *
     * 书架使用非完整方块的碰撞箱，需要基于形状计算光照遮挡。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 红石 ==========

    /**
     * @brief 检查是否有比较器输入覆盖
     *
     * 书架支持比较器读取3个槽位的占用状态。
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取红石比较器信号
     *
     * 使用3位二进制编码，从 ShelfBlockEntity 读取每个槽位的占用状态：
     * - 槽位0占用: bit 0 = 1
     * - 槽位1占用: bit 1 = 2
     * - 槽位2占用: bit 2 = 4
     * - 最大输出: 7
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 信号强度 (0-7)
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 方块实体 ==========

    /**
     * @brief 检查是否有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

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

    // ========== 旋转和镜像 ==========

    /**
     * @brief 旋转书架方块
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像书架方块
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

} // namespace blocks
} // namespace mc
