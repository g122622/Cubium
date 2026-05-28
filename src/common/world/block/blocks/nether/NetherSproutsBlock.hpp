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
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../agricultural/BushBlock.hpp"

namespace mc {

class IWorld;
class IBlockReader;

namespace blocks {

/**
 * @brief 下界苗方块
 *
 * 下界中的小型装饰植物，可以放置在菌岩或灵魂土上。
 * 高度仅3像素（0.1875格），不会生长。
 *
 * MC ID: minecraft:nether_sprouts
 *
 * 参考: net.minecraft.block.NetherSproutsBlock
 */
class NetherSproutsBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit NetherSproutsBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~NetherSproutsBlock() override = default;

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /**
     * @brief 检查下方方块是否可以支撑此植物
     *
     * 下界苗可放置在：菌岩（绯红/诡异）、灵魂土，以及默认的 BushBlock 支撑面。
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;
};

} // namespace blocks
} // namespace mc
