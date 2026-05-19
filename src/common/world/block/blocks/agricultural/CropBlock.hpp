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

#include "../../IGrowable.hpp"
#include "BushBlock.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class ServerWorld;

namespace blocks {

/**
 * @brief 农作物方块基类
 *
 * 可生长的农作物，如小麦、胡萝卜、马铃薯等。
 * 使用 AGE_0_7 属性表示生长阶段（0-7，共8个阶段）。
 *
 * 参考: net.minecraft.block.CropsBlock
 */
class CropBlock : public BushBlock, public IGrowable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CropBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~CropBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取年龄属性
     */
    [[nodiscard]] virtual const IntegerProperty& getAgeProperty() const;

    /**
     * @brief 获取最大年龄
     */
    [[nodiscard]] virtual int getMaxAge() const { return 7; }

    /**
     * @brief 获取当前年龄
     */
    [[nodiscard]] int getAge(const BlockState& state) const;

    /**
     * @brief 创建指定年龄的状态
     */
    [[nodiscard]] const BlockState& withAge(int age) const;

    /**
     * @brief 是否为最大年龄
     */
    [[nodiscard]] bool isMaxAge(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 生长逻辑 ==========

    /**
     * @brief 随机刻（用于生长）
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否需要随机 tick
     *
     * 农作物总是需要随机 tick 以便生长检查。
     * 在 randomTick() 中会检查是否成熟并提前返回。
     *
     * 注意：Block::ticksRandomly() 是无状态方法，无法检查作物是否成熟。
     * 成熟检查在 randomTick() 中进行。
     */
    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== IGrowable 接口实现 ==========

    /**
     * @brief 检查是否可以生长（未成熟时可以）
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 骨粉是否有效（总是有效）
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 使用骨粉生长（便捷方法，自动创建随机数）
     */
    void grow(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 获取骨粉增加的年龄
     *
     * 增长值由世界种子和方块位置派生的确定性随机数生成，
     * 不要使用全局 rand()，否则同一世界内的结果会不可复现。
     */
    [[nodiscard]] virtual int getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 掉落物 ==========

    /**
     * @brief 获取作物物品（成熟时掉落）
     */
    [[nodiscard]] virtual u32 getCropItem() const = 0;

    /**
     * @brief 获取种子物品
     */
    [[nodiscard]] virtual u32 getSeedItem() const = 0;

protected:
    /**
     * @brief 检查下方是否可支撑
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

    /**
     * @brief 计算生长速度（静态方法，供 StemBlock 使用）
     *
     * 参考 MC 1.16.5: CropsBlock.getGrowthChance
     * 考虑周围耕地湿润度和同类作物拥挤程度
     */
    [[nodiscard]] static float getGrowthChance(const Block& block, IBlockReader& world, const BlockPos& pos);

    /// 各年龄阶段的形状缓存
    std::array<CollisionShape, 8> m_shapesByAge;
};

} // namespace blocks
} // namespace mc
