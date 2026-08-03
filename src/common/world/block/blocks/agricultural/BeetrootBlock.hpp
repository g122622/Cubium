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
#include "CropBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>

namespace mc {
namespace blocks {

/**
 * @brief 甜菜根作物
 *
 * 4个生长阶段（AGE_0_3），成熟时掉落甜菜根和甜菜根种子。
 * 形状高度：2, 4, 6, 8 像素。
 * 生长速度比其他作物慢（有 1/3 概率跳过生长）。
 */
class BeetrootBlock : public CropBlock {
public:
    explicit BeetrootBlock(const BlockProperties& properties);
    ~BeetrootBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取年龄属性（甜菜根使用 AGE_0_3）
     */
    [[nodiscard]] const IntegerProperty& getAgeProperty() const override;

    /**
     * @brief 获取最大年龄（甜菜根为 3）
     */
    [[nodiscard]] i32 getMaxAge() const override { return 3; }

    // ========== 生长逻辑 ==========

    /**
     * @brief 随机刻（甜菜根生长较慢）
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 骨粉增加的年龄（甜菜根较少）
     */
    [[nodiscard]] i32 getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 掉落物 ==========

    [[nodiscard]] u32 getCropItem() const override;
    [[nodiscard]] u32 getSeedItem() const override;

private:
    std::array<CollisionShape, 4> m_beetrootShapesByAge;
};

} // namespace blocks
} // namespace mc
