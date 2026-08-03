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
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
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
 * 默认使用 AGE_0_7 属性表示生长阶段（0-7，共8个阶段）。
 * 甜菜根（BeetrootBlock）覆盖为 AGE_0_3（0-3，共4个阶段）。
 *
 * 注意：年龄属性通过构造函数参数传入，避免在基类构造函数中调用虚方法
 * （C++ 中基类构造期间虚分派不会解析到派生类）。
 */
class CropBlock : public BushBlock, public IGrowable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param ageProperty 年龄属性，默认为 AGE_0_7（8个阶段）。
     *                    BeetrootBlock 传入 AGE_0_3（4个阶段）。
     */
    explicit CropBlock(
        const BlockProperties& properties, const IntegerProperty& ageProperty = BlockStateProperties::AGE_0_7());

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
    [[nodiscard]] virtual i32 getMaxAge() const { return 7; }

    /**
     * @brief 获取当前年龄
     */
    [[nodiscard]] i32 getAge(const BlockState& state) const;

    /**
     * @brief 创建指定年龄的状态
     *
     * 子类可重写此方法以自定义年龄转换逻辑（如 TorchflowerCropBlock
     * 在 age >= getMaxAge() 时返回火把花方块状态）。
     */
    [[nodiscard]] virtual const BlockState& withAge(i32 age) const;

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
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

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
    [[nodiscard]] virtual i32 getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const;

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

public:
    /**
     * @brief 计算生长速度（静态方法，供 StemBlock 等使用）
     *
     * 考虑周围 3x3 耕地湿润度和同类作物拥挤程度。
     * 基础值 1.0，每格湿润耕地 +3.0（中心）或 +0.75（周围），
     * 干燥耕地 +1.0（中心）或 +0.25（周围）。
     * 同类作物拥挤时乘以 0.5。
     * 返回值不低于 1.0。
     */
    [[nodiscard]] static f32 getGrowthChance(const Block& block, IBlockReader& world, const BlockPos& pos);

protected:
    /**
     * @brief 检查下方是否可支撑
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

    /**
     * @brief 获取植物类型 - 农作物返回 PlantType::Crop
     */
    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;

    /// 各年龄阶段的形状缓存
    std::array<CollisionShape, 8> m_shapesByAge;

private:
    /// 存储的年龄属性引用（构造时由派生类传入，避免基类构造期间虚分派问题）
    const IntegerProperty& m_ageProperty;
};

} // namespace blocks
} // namespace mc
