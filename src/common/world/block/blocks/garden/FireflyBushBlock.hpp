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
#include "../agricultural/BushBlock.hpp"
#include "common/world/block/Block.hpp"

namespace mc {

class IBlockReader;
class IWorld;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 萤火虫灌木方块
 *
 * 装饰性发光植物（亮度 2），自然生成在水边的草方块/菌丝体上，沼泽中任意位置。
 * 继承 BushBlock 走默认 canSurvive（下方须为 #dirt 标签或耕地），不重写 canSustain，
 * 使 SimpleBlockFeature 的 canSurvive 终判生效，避免在世界生成时浮空于水面。
 *
 * 骨粉行为（对齐 wiki 萤火虫灌木丛#骨粉）：对不含雪的萤火虫灌木使用骨粉，会在曼哈顿距离
 * 为 1 的随机可种植方块上生成一个新的萤火虫灌木。即遍历水平 4 方向邻居，找首个为空气
 * 且其下方可支撑（isValidPosition）的位置，在该处放置默认灌木状态。骨粉 100% 成功。
 *
 * MC ID: minecraft:firefly_bush
 */
class FireflyBushBlock : public BushBlock, public IGrowable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FireflyBushBlock(const BlockProperties& properties);

    ~FireflyBushBlock() noexcept override = default;

    // ========== IGrowable 接口实现 ==========

    /**
     * @brief 检查是否可以生长（骨粉目标判定）
     *
     * 等价于 hasSpreadableNeighbourPos：遍历水平 4 方向邻居，若存在「空气 + 其下方可支撑
     * 萤火虫灌木」的位置则返回 true（存在可种植蔓延目标）。无可种植邻居时骨粉不生效。
     *
     * @param world 世界读取器
     * @param pos 当前方块位置
     * @param state 当前方块状态
     * @param isClientSide 是否为客户端
     * @return 存在可种植邻居位置返回 true
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 检查骨粉是否必定生效
     *
     * 萤火虫灌木骨粉 100% 成功（wiki 记录为确定行为，非概率），返回 true。
     *
     * @param world 世界
     * @param random 随机数生成器（用于决定蔓延方向，本方法不消费）
     * @param pos 当前方块位置
     * @param state 当前方块状态
     * @return 始终返回 true
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 执行骨粉生长
     *
     * 等价于 findSpreadableNeighbourPos：以随机顺序遍历水平 4 方向邻居，找首个「空气 +
     * 其下方可支撑萤火虫灌木」的位置，在该处放置萤火虫灌木默认状态。曼哈顿距离恰为 1
     * （仅水平方向，不含上下）。骨粉必定找到目标（canGrow 已保证存在），但若随机序遍历
     * 未命中（理论上不会发生，防御性处理）则不放置。
     *
     * @param world 世界
     * @param random 随机数生成器（用于打乱水平方向遍历顺序）
     * @param pos 当前方块位置
     * @param state 当前方块状态
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

private:
    /**
     * @brief 在水平 4 方向邻居中查找首个可种植蔓延位置
     *
     * 遍历给定方向序列，对每个方向计算邻居位置 neighbourPos，若该位置为空气（isAir）
     * 且 neighbourPos 处放置萤火虫灌木可通过 isValidPosition（即其下方可支撑），则返回
     * 该位置。等价于 Java BonemealableBlock.getSpreadableNeighbourPos（canSurvive 判定）。
     *
     * 接收 IBlockReader&：与 canGrow 入参类型及 isValidPosition 形参类型一致。
     * grow 的 IWorld& 实参为 ServerWorld（同时实现 IWorld 与 IBlockReader），向下转换安全。
     *
     * @param world 世界读取器（IBlockReader&，提供 getBlockState / isValidPosition）
     * @param pos 当前方块位置
     * @param directions 水平方向序列（正序用于 canGrow 判定存在性，随机序用于 grow 选址）
     * @return 首个可种植蔓延位置，无则返回 nullopt
     */
    [[nodiscard]] std::optional<BlockPos> getSpreadableNeighbourPos(
        IBlockReader& world, const BlockPos& pos, const std::array<Direction, 4>& directions) const;
};

} // namespace blocks
} // namespace mc
