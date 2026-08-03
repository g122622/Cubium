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

#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <memory>
#include <optional>

namespace mc {

class World;
class BlockItemUseContext;
class Player;
class BlockRaycastResult;

namespace blocks {

/**
 * @brief 潜影盒方块
 *
 * 方块状态属性：
 * - FACING: 朝向（UP/DOWN/NORTH/SOUTH/EAST/WEST）
 *
 * 特点：
 * - 有 27 格存储空间
 * - 物品放入后会保留在潜影盒内（破坏不掉落）
 * - 支持红石比较器信号输出
 * - 不能将潜影盒放入另一个潜影盒（防止递归嵌套）
 * - 被活塞推动时会销毁（因为包含实体数据）
 */
class ShulkerBoxBlock : public Block {
public:
    /**
     * @brief 构造函数（无色潜影盒）
     * @param properties 方块属性
     */
    explicit ShulkerBoxBlock(const BlockProperties& properties);

    /**
     * @brief 构造函数（染色潜影盒）
     * @param color 染料颜色
     * @param properties 方块属性
     */
    ShulkerBoxBlock(DyeColor color, const BlockProperties& properties);

    /**
     * @brief 构造函数（可选颜色潜影盒）
     * @param color 染料颜色（std::nullopt 表示无色）
     * @param properties 方块属性
     */
    ShulkerBoxBlock(std::optional<DyeColor> color, const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~ShulkerBoxBlock() override = default;

    // ========== 颜色 ==========

    /**
     * @brief 获取潜影盒的颜色
     * @return 染料颜色，无色潜影盒返回 std::nullopt
     */
    [[nodiscard]] const std::optional<DyeColor>& getColor() const { return m_color; }

    /**
     * @brief 检查方块是否为潜影盒（包括所有染色变体）
     * @param block 方块引用
     * @return 如果是潜影盒返回 true
     */
    [[nodiscard]] static bool isShulkerBox(const Block& block);

    // ========== 方块状态 ==========

    /**
     * @brief 获取放置时的方块状态
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

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

    /**
     * @brief 获取方块实体类型
     * @return 方块实体类型
     */
    [[nodiscard]] BlockEntityType getBlockEntityType() const { return BlockEntityType::ShulkerBox; }

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 红石 ==========

    /**
     * @brief 检查是否可以提供红石信号
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 检查是否有比较器输入覆盖
     *
     * 潜影盒可以通过比较器输出信号，因此需要返回 true。
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取红石比较器信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 移除处理 ==========

    /**
     * @brief 方块被移除时的处理
     *
     * 潜影盒被移除时需要保留其内容物（不同于普通容器）。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查位置是否可以打开潜影盒
     *
     * 潜影盒打开时需要在上方向有足够的空间。
     *
     * @param world 世界
     * @param pos 位置
     * @param facing 朝向
     * @return 如果可以打开返回 true
     */
    [[nodiscard]] static bool canOpen(IWorld& world, const BlockPos& pos, Direction facing);

private:
    /// 潜影盒颜色，std::nullopt 表示无色潜影盒
    std::optional<DyeColor> m_color;

    /**
     * @brief 获取打开方向的碰撞检测区域
     * @param pos 方块位置
     * @param facing 朝向
     * @return 碰撞盒
     */
    [[nodiscard]] static AxisAlignedBB getOpenBoundingBox(const BlockPos& pos, Direction facing);
};

} // namespace blocks
} // namespace mc
