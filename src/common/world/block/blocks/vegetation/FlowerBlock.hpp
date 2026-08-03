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

#include "../../../../util/property/Properties.hpp"
#include "../agricultural/BushBlock.hpp"
#include "DoublePlantBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>

namespace mc {
namespace blocks {

/**
 * @brief 花朵方块基类
 *
 * 小型花朵（蒲公英、玫瑰等）。
 * 可放置在草方块、泥土、耕地等上。
 * 可以放在花盆中。
 *
 * 参考: net.minecraft.block.FlowerBlock
 */
class FlowerBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param suspiciousStewEffect 可疑炖汤效果（可选）
     * @param effectDuration 效果持续时间（秒）
     */
    FlowerBlock(const BlockProperties& properties, u32 suspiciousStewEffect = 0, i32 effectDuration = 0);

    /**
     * @brief 析构函数
     */
    ~FlowerBlock() override = default;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 效果 ==========

    /**
     * @brief 获取可疑炖汤效果ID
     */
    [[nodiscard]] u32 getSuspiciousStewEffect() const { return m_suspiciousStewEffect; }

    /**
     * @brief 获取效果持续时间（秒）
     *
     * 对于瞬间效果（饱和），持续时间为秒值乘以20前的原始秒数；
     * 对于非瞬间效果，同样为秒值，使用时需乘以20转换为tick。
     * MC 原版约定：瞬间效果的秒数不会被乘以20。
     */
    [[nodiscard]] i32 getEffectDuration() const { return m_effectDuration; }

    /**
     * @brief 是否具有可疑炖汤效果
     *
     * 效果ID不为0时表示该花朵可以用于棕色哞菇的迷之炖菜。
     */
    [[nodiscard]] bool hasStewEffect() const { return m_suspiciousStewEffect != 0; }

protected:
    /**
     * @brief 检查下方是否可支撑
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

    /// 可疑炖汤效果ID
    u32 m_suspiciousStewEffect;
    /// 效果持续时间（秒）
    i32 m_effectDuration;
};

/**
 * @brief 丁香方块
 *
 * 两格高的花朵，可制作染料。
 *
 * 参考: net.minecraft.block.LilacBlock
 */
class LilacBlock : public DoublePlantBlock {
public:
    explicit LilacBlock(const BlockProperties& properties);
};

/**
 * @brief 玫瑰丛方块
 *
 * 两格高的花朵，可制作红色染料。
 *
 * 参考: net.minecraft.block.RoseBushBlock
 */
class RoseBushBlock : public DoublePlantBlock {
public:
    explicit RoseBushBlock(const BlockProperties& properties);
};

/**
 * @brief 牡丹方块
 *
 * 两格高的花朵，可制作粉色染料。
 *
 * 参考: net.minecraft.block.PeonyBlock
 */
class PeonyBlock : public DoublePlantBlock {
public:
    explicit PeonyBlock(const BlockProperties& properties);
};

/**
 * @brief 大型蕨方块
 *
 * 两格高的蕨类植物，与蕨使用相同材质但为双格版本。
 * 可放置在草方块、泥土等上。
 *
 * MC ID: minecraft:large_fern
 *
 * 参考: net.minecraft.block.DoublePlantBlock (LARGE_FERN 注册为 DoublePlantBlock)
 */
class LargeFernBlock : public DoublePlantBlock {
public:
    explicit LargeFernBlock(const BlockProperties& properties);
};

/**
 * @brief 向日葵方块
 *
 * 两格高的花朵，朝向太阳。
 *
 * 参考: net.minecraft.block.SunflowerBlock
 */
class SunflowerBlock : public DoublePlantBlock {
public:
    explicit SunflowerBlock(const BlockProperties& properties);
};

} // namespace blocks
} // namespace mc
