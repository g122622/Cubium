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
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;

namespace blocks {

/**
 * @brief 耕地方块
 *
 * 用于种植农作物的土地。具有湿润等级属性（0-7）。
 * 靠近水源会提高湿润等级。
 *
 * 状态属性：
 * - MOISTURE: 湿润等级 (0-7)
 *
 */
class FarmlandBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FarmlandBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~FarmlandBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override
    {
        return true;
    }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 是否按形状做光照遮挡（对齐 vanilla `FarmBlock#useShapeForLightOcclusion`）
     *
     * vanilla 返回 true：耕地比整格矮 1 像素，光照引擎据此按实际形状做面遮挡，
     * 而不是把整格当作挡光。早期缺少这个重写，耕地退回默认值 false。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 移动和交互 ==========

    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    void onFallenUpon(
        IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance) override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 工具方法 ==========

    /**
     * @brief 转变为泥土
     * @param entity 导致转变的实体（踩踏者），可为 nullptr（如上方放置方块、干燥等）
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    static void turnToDirt(Entity* entity, IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 检查是否湿润
     */
    [[nodiscard]] static bool isMoist(const BlockState& state)
    {
        return state.get(BlockStateProperties::MOISTURE_0_7()) > 0;
    }

    /**
     * @brief 检查位置是否有水
     */
    [[nodiscard]] static bool hasWater(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查上方是否有作物
     *
     * 使用 MAINTAINS_FARMLAND 方块标签检测，与 MC 原版逻辑一致。
     */
    [[nodiscard]] static bool hasCrops(IWorld& world, const BlockPos& pos);

    // ========== 植物支撑 ==========

    /**
     * @brief 检查耕地是否可以支撑指定类型的植物
     *
     * 耕地支持 PlantType::Crop（农作物）类型的植物。
     */
    [[nodiscard]] bool canSustainPlant(const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos,
        Direction facing,
        const IPlantable& plant) const override;

private:
    /// 耕地形状（高度15像素）
    CollisionShape m_shape;
    /// 寻路通行判定。
    [[nodiscard]] bool isPathfindable(const BlockState& state, PathComputationType type) const override;
};

} // namespace blocks
} // namespace mc
