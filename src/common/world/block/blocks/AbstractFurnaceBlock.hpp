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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <memory>

namespace mc {

class World;
class BlockItemUseContext;
class Player;

namespace blocks {

/**
 * @brief 熔炉方块基类
 *
 * 提供熔炉、高炉、烟熏炉的通用功能：
 * - FACING 属性：水平朝向
 * - LIT 属性：是否点燃
 * - 红石比较器信号
 * - 方块实体交互
 *
 * 参考: net.minecraft.block.AbstractFurnaceBlock
 *
 * 子类:
 * - FurnaceBlock（普通熔炉）
 * - BlastFurnaceBlock（高炉）
 * - SmokerBlock（烟熏炉）
 */
class AbstractFurnaceBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit AbstractFurnaceBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~AbstractFurnaceBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 获取放置时的方块状态
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 光照 ==========

    /**
     * @brief 获取方块状态的动态光照等级
     *
     * 对应 MC 1.21.11 的 litBlockEmission(13)：LIT=true 时返回 13，否则返回 0。
     * 参考: net.minecraft.world.level.block.Blocks.litBlockEmission
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    // ========== 方块实体 ==========

    /**
     * @brief 检查是否有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

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
     * @brief 检查是否有红石比较器输入覆盖
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override { return true; }

    /**
     * @brief 获取红石比较器信号
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 旋转和镜像 ==========

    /**
     * @brief 旋转方块状态
     * @param state 原状态
     * @param rotation 旋转
     * @return 旋转后的状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     * @param state 原状态
     * @param mirror 镜像
     * @return 镜像后的状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查熔炉是否点燃
     * @param state 方块状态
     * @return 如果点燃返回true
     */
    [[nodiscard]] static bool isLit(const BlockState& state);

protected:
    /**
     * @brief 与熔炉交互（打开GUI）
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     */
    [[nodiscard]] virtual bool interactWith(IWorld& world, const BlockPos& pos, Player& player) = 0;

    /**
     * @brief 获取方块实体类型
     */
    [[nodiscard]] virtual BlockEntityType getBlockEntityType() const = 0;
};

} // namespace blocks
} // namespace mc
