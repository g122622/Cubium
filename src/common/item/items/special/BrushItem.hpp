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
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {

class IWorld;
class Player;
class LivingEntity;
class BlockRaycastResult;

namespace item {

/**
 * @brief 刷子物品
 *
 * 刷子用于考古挖掘可疑方块（可疑沙、可疑沙砾）中的物品，
 * 也可以刷犰狳获取犰狳鳞甲。
 *
 * 属性：
 * - 最大耐久：64
 * - 附魔能力：1（仅支持耐久、经验修补、消失诅咒）
 * - 使用时长：200 ticks（10秒）
 * - 动画周期：10 ticks
 *
 * 使用机制：
 * - 玩家右键方块开始持续使用刷子（onItemUse 检查视线是否对准方块）
 * - 每10 ticks触发一次刷扫（动画周期的第5 tick）
 * - 每次成功刷扫消耗1耐久
 * - 刷扫犰狳时消耗16耐久
 * - 非玩家或未对准方块时取消使用
 *
 * 参考: net.minecraft.world.item.BrushItem
 */
class BrushItem final : public Item {
public:
    /// 最大耐久度
    static constexpr i32 MAX_DURABILITY = 64;

    /// 使用持续时长（ticks）
    static constexpr i32 USE_DURATION = 200;

    /// 动画周期（ticks），每10 ticks触发一次刷扫
    static constexpr i32 ANIMATION_DURATION = 10;

    /// 刷扫触发时机（动画周期内的第几个tick，0-based 为第4 tick，即第5 tick触发）
    static constexpr i32 BRUSH_TICK_IN_CYCLE = 4;

    /// 刷犰狳时的耐久消耗量
    static constexpr i32 ARMADILLO_DURABILITY_COST = 16;

    /**
     * @brief 刷扫粒子方向偏移记录
     *
     * 对应 MC 1.21.11 BrushItem.DustParticlesDelta。
     * 根据玩家视线方向和命中面方向计算粒子在 X/Z 轴上的方向偏移，
     * 用于让粒子沿方块表面散射而非垂直飞出。
     */
    struct DustParticlesDelta {
        /// X 轴方向偏移
        f64 xd;
        /// Y 轴方向偏移（恒为0，刷子粒子不向上飞）
        f64 yd;
        /// Z 轴方向偏移
        f64 zd;

        /**
         * @brief 根据视线方向和命中面方向计算粒子方向偏移
         *
         * 算法对齐 MC 1.21.11 BrushItem.DustParticlesDelta.fromDirection：
         * - Down/Up：偏移取视线向量的 (z, -x) 分量
         * - North：(-1, 0, -0.1)
         * - South：(1, 0, 0.1)
         * - West：(-0.1, 0, -1)
         * - East：(0.1, 0, 1)
         *
         * @param viewVector 玩家视线方向（归一化）
         * @param direction 命中面方向
         * @return 粒子方向偏移
         */
        [[nodiscard]] static DustParticlesDelta fromDirection(const Vector3& viewVector, Direction direction) noexcept;
    };

    /**
     * @brief 构造刷子
     * @param properties 物品属性
     */
    explicit BrushItem(ItemProperties properties);

    ~BrushItem() override = default;

    // ========== 物品使用 ==========

    /**
     * @brief 在方块上使用物品
     *
     * 当玩家对准一个方块右键时调用。
     * 检查玩家视线是否对准方块，如果是则开始持续使用。
     *
     * @param context 物品使用上下文
     * @return Consume（开始使用）或 Pass（不对准方块时不使用）
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 右键使用物品
     *
     * 当玩家右键（不对准方块）时调用。
     * 开始持续使用刷子。
     *
     * @param world 世界引用
     * @param player 玩家引用
     * @param hand 使用的手
     * @return 动作结果
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 物品使用过程中每tick调用
     *
     * 每 ANIMATION_DURATION (10) ticks 的第 BRUSH_TICK_IN_CYCLE+1 (5th) tick 触发刷扫逻辑：
     * 1. 通过 calculateHitResult 检查玩家视线是否仍对准方块，未对准则 stopActiveHand
     * 2. 命中方块时：
     *    - 播放方块对应的刷扫音效（BrushableBlock::getBrushSound 或 BRUSH_GENERIC）
     *    - 生成方块碎屑粒子（spawnDustParticles，仅当 shouldSpawnTerrainParticles 且渲染类型非 INVISIBLE）
     *    - 调用 BrushableBlockEntity.brush()（若存在），返回 true 时消耗1耐久
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用实体
     * @param elapsedTicks 已使用的tick数（从1开始）
     */
    void onUseTick(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 elapsedTicks) override;

    /**
     * @brief 与实体交互
     *
     * 刷犰狳时掉落犰狳鳞甲并消耗16耐久。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    bool itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand) override;

    // ========== 属性 ==========

    /**
     * @brief 获取使用时长
     * @return USE_DURATION (200 ticks)
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作类型
     * @return UseAction::Brush
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& /*stack*/) const override { return UseAction::Brush; }

    /**
     * @brief 获取附魔能力值
     *
     * 刷子的附魔能力为1，仅支持耐久、经验修补和消失诅咒。
     *
     * @return 1
     */
    [[nodiscard]] i32 getItemEnchantability() const override { return 1; }

private:
    /**
     * @brief 计算玩家视线射线检测结果
     *
     * 对齐 MC 1.21.11 BrushItem.calculateHitResult。
     * 从玩家眼睛位置沿视线方向进行方块射线检测，
     * 最大距离为玩家方块交互距离（Player::blockInteractionRange()）。
     *
     * @param player 玩家
     * @return 方块射线检测结果（miss 或 hit）
     */
    [[nodiscard]] static BlockRaycastResult calculateHitResult(const Player& player);

    /**
     * @brief 生成刷扫方块碎屑粒子
     *
     * 对齐 MC 1.21.11 BrushItem.spawnDustParticles。
     * 在命中点附近生成 7~11 个 Block 粒子（携带方块状态），
     * 粒子速度沿命中面表面散射（由 DustParticlesDelta 决定），
     * 并根据主手/副手镜像方向。
     *
     * @param world 世界引用
     * @param hitResult 方块射线检测结果
     * @param blockState 命中方块状态（用于粒子纹理）
     * @param viewVector 玩家视线方向（归一化）
     * @param arm 使用刷子的手臂侧（Left/Right，决定粒子方向镜像）
     */
    static void spawnDustParticles(IWorld& world,
        const BlockRaycastResult& hitResult,
        const BlockState& blockState,
        const Vector3& viewVector,
        HandSide arm);
};

} // namespace item
} // namespace mc
