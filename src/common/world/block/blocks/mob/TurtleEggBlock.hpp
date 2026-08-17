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
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;

namespace blocks {

/**
 * @brief 海龟蛋方块
 *
 * 海龟产下的蛋，会缓慢孵化。
 *
 * 状态属性：
 * - EGGS_1_4: 蛋的数量 (1-4)
 * - HATCH_0_2: 孵化阶段 (0-2)
 */
class TurtleEggBlock : public Block {
public:
    explicit TurtleEggBlock(const BlockProperties& properties);
    ~TurtleEggBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getEggs(const BlockState& state) const;
    [[nodiscard]] BlockState withEggs(i32 count) const;

    [[nodiscard]] i32 getHatch(const BlockState& state) const;
    [[nodiscard]] BlockState withHatch(i32 hatch) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 堆叠替换 ==========

    // 对齐 vanilla Java 1.21 TurtleEggBlock.canBeReplaced（TurtleEggBlock.java:147-152）：手持海龟蛋物品
    // + 非潜行 + eggs<4 时，已有海龟蛋方块「可被替换」（实为堆叠，getStateForPlacement 据此把已有蛋
    // eggs+1）。与 CandleBlock::isReplaceable 同构。修复前缺失此 override：基类 isReplaceable 返回
    // m_isReplaceable（海龟蛋注册未调 .replaceable()，故 false），点击已有蛋顶面时 _canReplace=false →
    // placementPos=上方 air → getStateForPlacement 检测 air（非已有蛋）→ 新放蛋落上方而非堆叠，
    // 与 vanilla「一个方块空间最多 4 个蛋」偏差（wiki tech_海龟蛋.txt#用途）。
    [[nodiscard]] bool isReplaceable(const BlockState& state, const BlockItemUseContext& context) const override;

    // ========== 孵化逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 实体交互 (踩破蛋) ==========

    void onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    void onFallenUpon(
        IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    /**
     * @brief 检查是否可以孵化 (光照条件)
     * @param world 世界
     * @param random 随机数生成器
     * @return 是否可以孵化
     */
    [[nodiscard]] bool _canGrow(IWorld& world, math::IRandom& random) const;

    /**
     * @brief 检查下方是否为沙子
     * @param world 世界读取器
     * @param pos 海龟蛋位置
     * @return 是否在沙子上
     */
    [[nodiscard]] bool _hasProperHabitat(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 检查实体是否可以踩破蛋
     * @param world 世界
     * @param entity 实体
     * @return 是否可以踩破
     */
    [[nodiscard]] bool _canTrample(IWorld& world, Entity& entity) const;

    /**
     * @brief 尝试踩破蛋
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param entity 实体
     * @param chance 触发概率 (1/chance)
     */
    void _tryTrample(IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, i32 chance) const;

    /**
     * @brief 移除一个蛋
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void _removeOneEgg(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查实体是否为僵尸类型
     * @param entity 实体
     * @return 是否为僵尸类型
     */
    [[nodiscard]] bool _isZombieType(Entity& entity) const;

    std::array<CollisionShape, 4> m_shapesByEggCount;
};

} // namespace blocks
} // namespace mc
