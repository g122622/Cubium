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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../Block.hpp"
#include "../../IGrowable.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/vegetation/SaplingBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 杜鹃花丛方块
 *
 * MC 1.21.11: 继承自 VegetationBlock，只能种植在黏土或可种植面上。
 * 碰撞箱为双层：上半部分16x8像素 + 底部茎干4x8像素。
 * 骨粉效果：45%概率生长为杜鹃树。
 * isValidBonemealTarget 检查上方无流体。
 *
 * 参考: net.minecraft.world.level.block.AzaleaBlock (MC 1.21.11)
 */
class AzaleaBlock : public Block, public IGrowable, public IPlantable {
public:
    /**
     * @brief 构造杜鹃花丛方块
     * @param treeGenerator 杜鹃树生成器回调（骨粉成功时调用）
     * @param properties 方块属性
     */
    explicit AzaleaBlock(SaplingBlock::TreeGenerator treeGenerator, const BlockProperties& properties);

    ~AzaleaBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 检查是否可以放置
     *
     * MC 1.21.11: mayPlaceOn 检查下方是黏土或可种植面
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== IGrowable 接口 ==========

    /**
     * @brief 检查骨粉是否可用
     *
     * MC 1.21.11: isValidBonemealTarget 检查上方无流体
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 骨粉是否成功（45%概率）
     *
     * MC 1.21.11: isBonemealSuccess 返回 random.nextFloat() < 0.45
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 生长为杜鹃树
     *
     * MC 1.21.11: performBonemeal 调用 TreeGrower.AZALEA.growTree(...)
     * 实现流程：构建 WorldGenRegion → 派生种子 → 清空方块 → 调用树生成器
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== IPlantable 接口 ==========

    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;
    [[nodiscard]] const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const override;

protected:
    /**
     * @brief 检查下方方块是否适合种植杜鹃花
     *
     * MC 1.21.11: mayPlaceOn 检查 CLAY 或 super.mayPlaceOn
     */
    [[nodiscard]] virtual bool mayPlaceOn(const BlockState& state, IBlockReader& world, const BlockPos& pos) const;

private:
    /// 杜鹃树生成器回调
    SaplingBlock::TreeGenerator m_treeGenerator;
    CollisionShape m_shape;
};

/**
 * @brief 开花的杜鹃花丛方块
 *
 * 与普通杜鹃花丛功能相同，但具有不同的外观。
 * 碰撞箱与 AzaleaBlock 相同。
 *
 * 参考: net.minecraft.world.level.block.FloweringAzaleaBlock (MC 1.21.11)
 */
class FloweringAzaleaBlock : public AzaleaBlock {
public:
    /**
     * @brief 构造开花杜鹃花丛方块
     * @param treeGenerator 杜鹃树生成器回调（骨粉成功时调用）
     * @param properties 方块属性
     */
    explicit FloweringAzaleaBlock(SaplingBlock::TreeGenerator treeGenerator, const BlockProperties& properties);

    ~FloweringAzaleaBlock() override = default;
};

} // namespace blocks
} // namespace mc
