/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"

namespace mc {

class IWorld;
class IBlockReader;

namespace blocks {

/**
 * @brief 下界菌索方块
 *
 * 下界中的装饰植物根须，可放置在菌岩或灵魂土上。
 * 绯红菌索和诡异菌索共享此类，仅材质颜色不同。
 * 高度13像素（0.8125格），不会生长。
 *
 * MC ID: minecraft:crimson_roots, minecraft:warped_roots
 *
 * 参考: net.minecraft.block.NetherRootsBlock
 */
class NetherRootsBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit NetherRootsBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~NetherRootsBlock() override = default;

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /**
     * @brief 检查下方方块是否可以支撑此植物
     *
     * 下界菌索可放置在：菌岩（绯红/诡异）、灵魂土，以及默认的 BushBlock 支撑面。
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

    /**
     * @brief 获取植物类型 - 下界菌索返回 PlantType::Nether
     */
    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;
};

} // namespace blocks
} // namespace mc
