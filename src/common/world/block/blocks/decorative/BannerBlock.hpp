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
#include "../../../../util/color/DyeColor.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../blockentity/interactive/BannerEntity.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <unordered_map>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockPos;
class BlockEntity;

namespace blocks {

/**
 * @brief 旗帜抽象基类
 *
 * 提供旗帜方块的通用功能，包括颜色管理、方块实体创建。
 * 子类：StandingBannerBlock（站立旗帜）和 WallBannerBlock（墙壁旗帜）
 *
 * vanilla 1.21.11 旗帜不持有 waterlogged 属性（不实现 SimpleWaterloggedBlock），
 * 因此本项目旗帜同样不可含水，状态容器仅含 rotation/facing。
 *
 * 参考: net.minecraft.block.AbstractBannerBlock
 */
class AbstractBannerBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param color 旗帜底色
     */
    AbstractBannerBlock(const BlockProperties& properties, DyeColor color);

    ~AbstractBannerBlock() override = default;

    // ========== 颜色 ==========

    /**
     * @brief 获取旗帜底色
     * @return 染料颜色
     */
    [[nodiscard]] DyeColor getColor() const noexcept { return m_color; }

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建旗帜方块实体
     *
     * 创建 BannerEntity 并设置底色为方块的颜色。
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 放置回调 ==========

    /**
     * @brief 方块放置后回调
     *
     * 检查方块实体是否需要从物品数据加载。
     *
     * @param stack 放置该方块的物品堆（可能携带自定义名称等组件）
     */
    void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack) override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

protected:
    /// 旗帜底色
    DyeColor m_color;
};

/**
 * @brief 站立旗帜
 *
 * 放置在地面上的旗帜，有16个旋转方向（每22.5度）。
 *
 * 状态属性：
 * - ROTATION_0_15: 0-15，表示16个旋转方向
 *
 * 参考: net.minecraft.block.BannerBlock
 */
class StandingBannerBlock : public AbstractBannerBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param color 旗帜底色
     */
    StandingBannerBlock(const BlockProperties& properties, DyeColor color);

    ~StandingBannerBlock() override = default;

    // ========== 放置逻辑 ==========

    /**
     * @brief 根据玩家朝向计算旋转值
     * @return 带ROTATION属性的方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查下方是否有实心方块支撑
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 邻居更新 ==========

    /**
     * @brief 下方方块被移除时旗帜变为空气
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    /// 站立旗帜碰撞形状（旗杆：8x16x8像素）
    CollisionShape m_shape;
};

/**
 * @brief 墙壁旗帜
 *
 * 附着在墙面上的旗帜，有4个水平朝向。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 北、南、东、西四个方向
 *
 * 参考: net.minecraft.block.WallBannerBlock
 */
class WallBannerBlock : public AbstractBannerBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param color 旗帜底色
     */
    WallBannerBlock(const BlockProperties& properties, DyeColor color);

    ~WallBannerBlock() override = default;

    // ========== 放置逻辑 ==========

    /**
     * @brief 根据玩家视线方向选择墙面朝向
     * @return 带HORIZONTAL_FACING属性的方块状态，无法放置时返回默认状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查背面是否有实心方块支撑
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 邻居更新 ==========

    /**
     * @brief 支撑方块被移除时旗帜变为空气
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    /// 各方向的碰撞形状
    std::unordered_map<Direction, CollisionShape> m_shapesByDirection;
};

} // namespace blocks
} // namespace mc
