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
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;
class Player;
class BlockRaycastResult;

namespace blocks {

/**
 * @brief 甜浆果丛方块
 *
 * 生长在草地、泥土等上的灌木，有4个生长阶段（AGE 0-3）。
 * 玩家穿过时会造成伤害和减速，右键可采摘浆果。
 *
 * 状态属性：
 * - AGE_0_3: 生长阶段（0-3）
 */
class SweetBerryBushBlock : public BushBlock, public IGrowable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit SweetBerryBushBlock(const BlockProperties& properties);

    ~SweetBerryBushBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取 AGE 属性
     */
    static const IntegerProperty& AGE() { return BlockStateProperties::AGE_0_3(); }

    /**
     * @brief 获取最大年龄
     */
    [[nodiscard]] static constexpr i32 getMaxAge() noexcept { return 3; }

    /**
     * @brief 获取当前年龄
     */
    [[nodiscard]] i32 getAge(const BlockState& state) const noexcept;

    /**
     * @brief 创建指定年龄的状态
     */
    [[nodiscard]] const BlockState& withAge(const BlockState& state, i32 age) const;

    /**
     * @brief 是否为最大年龄
     */
    [[nodiscard]] bool isMaxAge(const BlockState& state) const noexcept;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== IGrowable 接口 ==========

    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 实体碰撞 ==========

    /**
     * @brief 实体碰撞处理
     *
     * 减速效果和伤害（AGE > 0 时）。
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 交互 ==========

    /**
     * @brief 右键交互
     *
     * AGE > 1 时可以采摘浆果。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

protected:
    /**
     * @brief 检查下方是否可支撑
     *
     * 甜浆果丛可以种在草地、泥土、砂土、灰化土、耕地上。
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

private:
    /// 各年龄阶段的形状缓存
    std::array<CollisionShape, 4> m_shapesByAge;

    /// 各年龄阶段的碰撞形状缓存
    std::array<CollisionShape, 4> m_collisionShapesByAge;

    /**
     * @brief 初始化形状缓存
     */
    void initShapes();
};

} // namespace blocks
} // namespace mc
