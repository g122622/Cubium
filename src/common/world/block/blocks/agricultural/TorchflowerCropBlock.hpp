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

#include "CropBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>

namespace mc {
namespace blocks {

/**
 * @brief 火把花作物
 *
 * 2个生长阶段（AGE_0_1）：幼苗（age=0）和成熟作物（age=1）。
 * 当骨粉或自然生长使年龄超过最大值时，作物方块变为火把花方块（FlowerBlock）。
 *
 * 与普通作物不同：
 * - 随机刻有 1/3 概率跳过（生长较慢）
 * - 骨粉每次只增加 1 个生长阶段
 * - 成熟后继续生长会变为火把花方块（而非停留在最大年龄）
 *
 * 参考: net.minecraft.world.level.block.TorchflowerCropBlock
 */
class TorchflowerCropBlock : public CropBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit TorchflowerCropBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~TorchflowerCropBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取年龄属性（火把花使用 AGE_0_1）
     */
    [[nodiscard]] const IntegerProperty& getAgeProperty() const override;

    /**
     * @brief 获取最大年龄
     *
     * 返回 2 而非 1，因为 age=2 时作物会变为火把花方块。
     * AGE_0_1 属性只存储 0 和 1，但 getMaxAge()=2 使得
     * isMaxAge() 在 age=1 时返回 false（因为 1 >= 2 为 false），
     * 骨粉仍可使用；当 grow() 计算 newAge=2 时，替换为火把花方块。
     *
     * 注意：MC Java 中 TorchflowerCropBlock.getMaxAge() 返回 2，
     * 而 AGE 属性为 AGE_1（值 0-1）。isMaxAge() 判断 getAge() >= getMaxAge()，
     * 所以 age=1 时 isMaxAge() 为 false，骨粉仍可使用；
     * 当 grow() 计算 newAge=2 时，getStateForAge(2) 返回火把花方块。
     */
    [[nodiscard]] i32 getMaxAge() const override { return 2; }

    /**
     * @brief 创建指定年龄的状态
     *
     * 重写以处理 age>=2 时返回火把花方块状态。
     * age=0 和 age=1 返回自身的作物状态，age>=2 返回火把花方块状态。
     */
    [[nodiscard]] const BlockState& withAge(i32 age) const override;

    // ========== 生长逻辑 ==========

    /**
     * @brief 随机刻（火把花有 1/3 概率跳过生长检查）
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 骨粉增加的年龄（火把花每次只增加 1）
     */
    [[nodiscard]] i32 getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const override;

    // ========== 骨粉生长 ==========

    /**
     * @brief 使用骨粉生长
     *
     * 重写以实现 age=2 时将作物替换为火把花方块的逻辑。
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 掉落物 ==========

    [[nodiscard]] u32 getCropItem() const override;
    [[nodiscard]] u32 getSeedItem() const override;

private:
    /// 两个生长阶段的形状缓存（age=0 幼苗，age=1 成熟）
    std::array<CollisionShape, 2> m_torchflowerShapesByAge;
};

} // namespace blocks
} // namespace mc
