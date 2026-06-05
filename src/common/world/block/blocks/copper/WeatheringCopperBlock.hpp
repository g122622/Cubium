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
#include "../../Block.hpp"

namespace mc {

class IWorld;
class BlockPos;

namespace blocks {

/**
 * @brief 可风化铜方块
 *
 * 支持氧化等级属性的铜方块基类。非涂蜡的铜方块会在随机刻中尝试氧化。
 * 氧化算法：扫描4格曼哈顿距离内的同类型铜方块，
 * 如果周围有更氧化的同类型方块，则加速氧化；
 * 如果周围有更少氧化的同类型方块，则减缓氧化。
 *
 * 参考: net.minecraft.block.OxidizableBlock (1.17+)
 */
class WeatheringCopperBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 初始氧化等级
     */
    WeatheringCopperBlock(const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel);

    ~WeatheringCopperBlock() override = default;

    /**
     * @brief 获取当前氧化等级
     */
    [[nodiscard]] BlockStateProperties::OxidationLevel getOxidationLevel() const { return m_oxidationLevel; }

    /**
     * @brief 设置下一氧化等级对应的方块
     *
     * 注册铜方块后调用，建立氧化链。
     */
    void setNextOxidationBlock(Block* nextBlock) { m_nextOxidationBlock = nextBlock; }

    /**
     * @brief 随机Tick - 尝试氧化
     *
     * 非涂蜡铜方块有概率在随机刻中氧化到下一等级。
     * 氧化概率受周围方块氧化等级影响。
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     *
     * 只有未达到最高氧化等级的方块才响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override
    {
        return m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized;
    }

protected:
    /**
     * @brief 获取下一氧化等级对应的方块
     *
     * 由子类或注册时设置氧化方块链。
     * 返回 nullptr 表示已是最高等级，无法继续氧化。
     */
    [[nodiscard]] virtual Block* getNextOxidationBlock() const;

    /// 当前氧化等级
    BlockStateProperties::OxidationLevel m_oxidationLevel;

    /// 下一氧化等级对应的方块（由注册时设置）
    Block* m_nextOxidationBlock = nullptr;
};

/**
 * @brief 涂蜡铜方块
 *
 * 涂蜡后的铜方块不会氧化，OXIDATION属性固定。
 * 参考: net.minecraft.block.WaxableBlock (逻辑上，MC中涂蜡方块只是不响应氧化)
 */
class WaxedCopperBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit WaxedCopperBlock(const BlockProperties& properties)
        : Block(properties)
    {}

    ~WaxedCopperBlock() override = default;
};

} // namespace blocks
} // namespace mc
