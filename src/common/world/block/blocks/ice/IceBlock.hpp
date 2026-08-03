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
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 冰方块
 *
 * 透明冰方块，在明亮环境中会融化成水。
 * 挖掘后会变成水源方块。
 *
 * MC ID: minecraft:ice
 */
class IceBlock : public Block {
public:
    /**
     * @brief 构造冰方块
     */
    explicit IceBlock(BlockProperties properties);

    /**
     * @brief 方块被移除后
     * 冰在非寒冷生物群系会融化成水，在温暖光源附近也会融化
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 随机刻
     * 在明亮环境中融化
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }
};

/**
 * @brief 浮冰方块
 *
 * 不透明的冰方块，不会融化。
 * 挖掘后会掉落自身（使用精准采集）或什么都不掉落。
 *
 * MC ID: minecraft:packed_ice
 */
class PackedIceBlock : public Block {
public:
    /**
     * @brief 构造浮冰方块
     */
    explicit PackedIceBlock(BlockProperties properties);
};

/**
 * @brief 蓝冰方块
 *
 * 最光滑的冰方块，摩擦力极低。
 * 可以用浮冰合成，不会融化。
 *
 * MC ID: minecraft:blue_ice
 */
class BlueIceBlock : public Block {
public:
    /**
     * @brief 构造蓝冰方块
     */
    explicit BlueIceBlock(BlockProperties properties);
};

/**
 * @brief 霜冰方块
 *
 * 由冰霜行者附魔生成的临时冰方块。
 * 有 AGE 属性（0-3），随着时间推移会逐渐融化成水。
 *
 * MC ID: minecraft:frosted_ice
 */
class FrostedIceBlock : public Block {
public:
    /**
     * @brief 构造霜冰方块
     */
    explicit FrostedIceBlock(BlockProperties properties);

    /**
     * @brief 方块被添加时
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居方块变化
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief Tick 更新
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 随机刻
     * 在光源附近融化
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    /**
     * @brief 获取 AGE 属性
     */
    [[nodiscard]] static const IntegerProperty& AGE_PROP() { return BlockStateProperties::AGE_0_3(); }

    /**
     * @brief 获取霜冰年龄
     */
    [[nodiscard]] static i32 getAge(const BlockState& state) { return state.get(AGE_PROP()); }

private:
    /**
     * @brief 检查是否应该融化
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param neighborsRequired 需要的霜冰邻居数量
     * @return true 如果应该融化
     */
    [[nodiscard]] bool _shouldMelt(IBlockReader& world, const BlockPos& pos, i32 neighborsRequired) const;

    /**
     * @brief 稍微融化（增加 AGE 或变成水）
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前状态
     * @return true 如果完全融化成水
     */
    bool _slightlyMelt(IWorld& world, const BlockPos& pos, BlockState& state);
};

} // namespace blocks
} // namespace mc
