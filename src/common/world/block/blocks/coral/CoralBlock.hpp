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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 珊瑚颜色枚举
 */
enum class CoralColor : u8 {
    Tube = 0,   // 管状珊瑚（蓝色）
    Brain = 1,  // 脑珊瑚（粉色）
    Bubble = 2, // 气泡珊瑚（紫色）
    Fire = 3,   // 火焰珊瑚（红色）
    Horn = 4    // 角珊瑚（黄色）
};

/**
 * @brief 珊瑚方块基类
 *
 * 水下的珊瑚方块，离开水会变成死珊瑚。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 状态属性：
 * - WATERLOGGED: 是否含水
 */
class CoralBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param color 珊瑚颜色
     * @param deadBlock 死珊瑚方块ID
     * @param properties 方块属性
     */
    CoralBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~CoralBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 水检测 ==========

    /**
     * @brief 检查是否在水中
     */
    [[nodiscard]] bool isInWater(const BlockState& state) const;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

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

protected:
    /**
     * @brief 检查周围是否有水
     */
    [[nodiscard]] bool isWaterNearby(IWorld& world, const BlockPos& pos) const;

    /// 珊瑚颜色
    CoralColor m_color;
    /// 死珊瑚方块ID
    u32 m_deadBlock;
};

/**
 * @brief 珊瑚扇方块
 *
 * 墙上的珊瑚扇，可以放置在墙面上。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 状态属性：
 * - WATERLOGGED: 是否含水
 * - HORIZONTAL_FACING: 朝向
 */
class CoralFanBlock : public Block, public IWaterLoggable {
public:
    CoralFanBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties);
    ~CoralFanBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

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

protected:
    /**
     * @brief 检查是否可以附着到指定方向
     */
    [[nodiscard]] bool canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const;

    /// 珊瑚颜色
    CoralColor m_color;
    /// 死珊瑚方块ID
    u32 m_deadBlock;
};

/**
 * @brief 墙珊瑚扇方块
 *
 * 类似珊瑚扇，但专门用于墙面放置。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 状态属性：
 * - WATERLOGGED: 是否含水
 * - FACING: 朝向
 */
class CoralWallFanBlock : public Block, public IWaterLoggable {
public:
    CoralWallFanBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties);
    ~CoralWallFanBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

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

protected:
    /**
     * @brief 检查是否可以附着到指定方向
     */
    [[nodiscard]] bool canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const;

    /// 珊瑚颜色
    CoralColor m_color;
    /// 死珊瑚方块ID
    u32 m_deadBlock;
};

/**
 * @brief 珊瑚块方块
 *
 * 固体的珊瑚块，不会因缺水而死亡。
 */
class CoralBlockBlock : public Block {
public:
    explicit CoralBlockBlock(CoralColor color, const BlockProperties& properties);
    ~CoralBlockBlock() override = default;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /// 珊瑚颜色
    CoralColor m_color;
};

} // namespace blocks
} // namespace mc
