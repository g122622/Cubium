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
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/PlantType.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 灌木/植物方块基类
 *
 * 所有植物类方块的基类，提供基本的放置逻辑和形状。
 * 植物只能在特定地面（草地、泥土、耕地等）上放置。
 * 默认返回 PlantType::Plains，子类可重写 getPlantType() 返回其他类型。
 *
 * 参考: net.minecraft.block.BushBlock
 */
class BushBlock : public Block, public IPlantable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BushBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~BushBlock() override = default;

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查位置是否有效
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（植物无碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取遮挡形状（植物不遮挡光线）
     */
    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    /**
     * @brief 是否不透明（植物透明）
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

protected:
    /**
     * @brief 检查下方方块是否可以支撑此植物
     * @param groundState 下方方块状态
     * @param world 世界
     * @param groundPos 下方位置
     * @return 如果可以支撑返回true
     */
    [[nodiscard]] virtual bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const;

    // ========== IPlantable 接口实现 ==========

    /**
     * @brief 获取植物类型
     *
     * 默认返回 PlantType::Plains，子类可重写返回其他类型。
     * 例如：CropBlock 返回 PlantType::Crop，LilyPadBlock 返回 PlantType::Water。
     */
    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 获取植物方块状态
     */
    [[nodiscard]] const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const override;

    /// 植物形状（默认为完整方块大小）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
