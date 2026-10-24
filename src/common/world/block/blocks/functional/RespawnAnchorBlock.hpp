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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 重生锚方块
 *
 * 在下界设置重生点的功能方块。
 * 使用萤石充能，有4个充能等级。
 *
 * 状态属性：
 * - CHARGES: 充能等级 (0-4)
 */
class RespawnAnchorBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RespawnAnchorBlock(const BlockProperties& properties);
    ~RespawnAnchorBlock() noexcept override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override
    {
        return true;
    }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 红石 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 工具方法 ==========

    /**
     * @brief 获取充能等级
     */
    [[nodiscard]] static i32 getCharges(const BlockState& state)
    {
        return state.get(BlockStateProperties::CHARGES_0_4());
    }

    /**
     * @brief 是否已充满
     */
    [[nodiscard]] static bool isFullyCharged(const BlockState& state)
    {
        return getCharges(state) >= 4;
    }

    /**
     * @brief 充能
     * @return 新的方块状态
     */
    static BlockState charge(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 消耗一次充能
     */
    static void discharge(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 该维度是否允许在重生锚上设置重生点
     *
     * 等价 EnvironmentAttributes.RESPAWN_ANCHOR_WORKS：仅下界为 true。
     */
    [[nodiscard]] static bool canSetSpawn(const IWorld& world);

    /**
     * @brief 物品是否可用作重生锚燃料
     */
    [[nodiscard]] static bool isRespawnFuel(const ItemStack& stack);

    /**
     * @brief 重生锚是否仍可被充能（充能等级 < 4）
     */
    [[nodiscard]] static bool canBeCharged(const BlockState& state)
    {
        return getCharges(state) < MAX_CHARGES;
    }

    /**
     * @brief 在重生锚周围寻找实体的安全落点
     *
     * 候选为 8 个水平相邻位置及其上下两格，外加正上方一格，共 25 个；先安全模式，
     * 全部失败后再允许不安全位置。
     *
     * @param world 世界
     * @param anchorPos 重生锚位置
     * @param width 实体碰撞箱宽度
     * @param height 实体碰撞箱高度
     * @param isPlayer 是否按玩家规则检查
     * @param fireImmune 实体是否免疫火焰
     * @return 安全落点（实体脚底位置）；找不到时返回 nullopt
     */
    [[nodiscard]] static std::optional<Vector3> findStandUpPosition(
        const IWorld& world, const BlockPos& anchorPos, f32 width, f32 height, bool isPlayer, bool fireImmune);

    /// 最小充能等级
    static constexpr i32 MIN_CHARGES = 0;
    /// 最大充能等级
    static constexpr i32 MAX_CHARGES = 4;

    // ========== 动态光照 ==========

    /**
     * @brief 获取动态光照等级
     *
     * 重生锚的光照等级根据充能等级变化：
     * - 0级: 0
     * - 1级: 3
     * - 2级: 7
     * - 3级: 11
     * - 4级: 15
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    /**
     * @brief 右键交互（手持物品） - 仅处理萤石充能
     *
     * 非萤石物品返回 TryEmptyHandInteraction，交由空手交互处理设置重生点/爆炸。
     */
    [[nodiscard]] BlockActionResult useItemOn(ItemStack& stack,
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 右键交互（空手） - 设置重生点或爆炸
     *
     * 0 充能直接 Pass；维度不允许时爆炸；否则把玩家重生点设置为该锚。
     * 玩家重生点已是该锚时返回 Consume（不重新设置、不播音效）。
     */
    [[nodiscard]] BlockActionResult useWithoutItem(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        const BlockRaycastResult& hit) override;

protected:
    /// 重生锚形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
