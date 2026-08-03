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
#include "../../IWaterLoggable.hpp"
#include "../MultifaceBlock.hpp"
#include "../MultifaceSpreader.hpp"
#include "SculkBehaviour.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

class Entity;
class BlockEntity;
enum class BlockEntityType : u16;

namespace blocks {

/**
 * @brief 幽匿块
 *
 * 深暗之域的主要构成方块，可通过幽匿催化体蔓延。
 * 当被精确采集时掉落经验。
 *
 * 实现 SculkBehaviour：在 SculkSpreader 电荷作用下生成生长物（sensor/shrieker）。
 */
class SculkBlock : public Block, public SculkBehaviour {
public:
    explicit SculkBlock(const BlockProperties& properties)
        : Block(properties)
    {}

    ~SculkBlock() override = default;

    // ========== SculkBehaviour ==========

    /// MC SculkBlock.canChangeBlockStateOnSpread = false。
    [[nodiscard]] bool canChangeBlockStateOnSpread() const override { return false; }

    /// MC SculkBehaviour.updateDecayDelay = 1。
    [[nodiscard]] i32 updateDecayDelay(i32 decayDelay) const override { return 1; }

    /**
     * @brief MC SculkBehaviour.attemptSpreadVein：sculk 块不主动扩散脉络。
     *
     * SculkPatchFeature 的电荷游标在 sculk 块上时仅消耗电荷生成生长物（attemptUseCharge），
     * 不调用 vein 的脉络扩散。返回 false 与 MC SculkBlock（未覆写 attemptSpreadVein，
     * 走接口默认的 vein.spreadAll）行为不同——但 MC 中 sculk 块上 facings 为 nullopt，
     * 接口默认分支 attemptSpreadVein(facings==null) 才会触发 vein spreadAll。
     * 此处 worldgen 路径游标 facings 恒为 availableFaces（非空），故不扩散。
     */
    [[nodiscard]] bool attemptSpreadVein(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        std::optional<std::vector<Direction>> facings,
        bool worldGen) override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        MC_UNUSED(facings);
        MC_UNUSED(worldGen);
        return false;
    }

    /// MC SculkBlock.attemptUseCharge：消耗电荷、概率生成 sensor/shrieker 生长物。
    [[nodiscard]] i32 attemptUseCharge(ChargeCursor& cursor,
        IWorld& world,
        const BlockPos& origin,
        math::IRandom& random,
        SculkSpreader& spreader,
        bool shouldUpdateBlocks) override;

private:
    /// MC SculkBlock.getRandomGrowthState：1/11 概率 shrieker，否则 sensor；按流体水化。
    [[nodiscard]] static const BlockState* getRandomGrowthState(
        IWorld& world, const BlockPos& pos, math::IRandom& random, bool worldGen);

    /// MC SculkBlock.canPlaceGrowth：上方为空气/水，且 4×3×4 范围内 sensor+shrieker ≤ 2。
    [[nodiscard]] static bool canPlaceGrowth(IWorld& world, const BlockPos& pos);

    /// MC SculkBlock.getDecayPenalty：距中心越远衰减越多。
    [[nodiscard]] static i32 getDecayPenalty(
        const SculkSpreader& spreader, const BlockPos& pos, const BlockPos& origin, i32 charge);
};

/**
 * @brief 幽匿脉络
 *
 * 多面方块（继承 MultifaceBlock），可附着在方块六面，支持含水。
 * 可被骨粉催生（spreadFromRandomFaceTowardRandomDirection）。
 *
 * 实现 SculkBehaviour：worldgen 电荷作用下把相邻可替换方块转化为 sculk，并在其周围蔓延脉络。
 */
class SculkVeinBlock : public MultifaceBlock, public SculkBehaviour {
public:
    explicit SculkVeinBlock(const BlockProperties& properties);

    ~SculkVeinBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /// MC MultifaceSpreadeableBlock.getSpreader：DEFAULT_SPREAD_ORDER 的脉络扩散器。
    [[nodiscard]] const MultifaceSpreader& getSpreader() const { return m_veinSpreader; }

    /// MC SculkVeinBlock.getSameSpaceSpreader：仅 SAME_POSITION 的扩散器。
    [[nodiscard]] const MultifaceSpreader& getSameSpaceSpreader() const { return m_sameSpaceSpreader; }

    // ========== SculkBehaviour ==========

    /// MC SculkVeinBlock.attemptUseCharge：worldgen 时尝试把相邻可替换方块转化为 sculk。
    [[nodiscard]] i32 attemptUseCharge(ChargeCursor& cursor,
        IWorld& world,
        const BlockPos& origin,
        math::IRandom& random,
        SculkSpreader& spreader,
        bool shouldUpdateBlocks) override;

    /// MC SculkBehaviour.attemptSpreadVein（接口默认分支）：facings 为空时走 sameSpaceSpreader。
    [[nodiscard]] bool attemptSpreadVein(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        std::optional<std::vector<Direction>> facings,
        bool worldGen) override;

    /// MC SculkVeinBlock.onDischarged：移除贴在 sculk 上的面，无面则变空气/水。
    void onDischarged(IWorld& world, const BlockState& state, const BlockPos& pos, math::IRandom& random) override;

    /// MC SculkBehaviour.canChangeBlockStateOnSpread = true（接口默认）。
    [[nodiscard]] bool canChangeBlockStateOnSpread() const override { return true; }

    // ========== 静态方法 ==========

    /// MC SculkVeinBlock.hasSubstrateAccess：vein 某面朝向 SCULK_REPLACEABLE 方块。
    [[nodiscard]] static bool hasSubstrateAccess(IWorld& world, const BlockState& state, const BlockPos& pos);

    /// MC SculkVeinBlock.regrow：从一组方向重建 vein（仅可 attach 的方向加面）。
    [[nodiscard]] static bool regrow(
        IWorld& world, const BlockPos& pos, const BlockState& current, const std::vector<Direction>& directions);

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override { MC_UNUSED(container); }

private:
    /// MC SculkVeinBlock.attemptPlaceSculk：随机方向遍历，把相邻可替换方块转化为 sculk 并蔓延脉络。
    [[nodiscard]] bool attemptPlaceSculk(
        SculkSpreader& spreader, IWorld& world, const BlockPos& pos, math::IRandom& random);

    /// 预计算 64 种面组合形状（2^6）。
    std::array<CollisionShape, 64> m_shapes;
    static size_t shapeIndex(const BlockState& state);

    MultifaceSpreader m_veinSpreader;
    MultifaceSpreader m_sameSpaceSpreader;
};

/**
 * @brief 幽匿感测体
 *
 * 检测振动并输出红石信号的方块。
 * 状态属性：SCULK_SENSOR_PHASE, POWER, WATERLOGGED
 */
class SculkSensorBlock : public Block, public IWaterLoggable {
public:
    explicit SculkSensorBlock(const BlockProperties& properties);

    ~SculkSensorBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override { return true; }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 方块 tick 回调
     *
     * 处理 Active→Cooldown 和 Cooldown→Inactive 的状态转换。
     * 由 scheduleBlockTick 调度触发。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 方块移除时通知邻居红石信号变化
     *
     * 如果移除时处于 Active 状态，需要通知邻居更新红石信号，
     * 否则邻居可能仍认为有信号输入。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /// 方块实体支持
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;
    [[nodiscard]] BlockEntityType getBlockEntityType() const;

    /// 活跃阶段持续tick数
    static constexpr i32 ACTIVE_TICKS = 30;
    /// 冷却阶段持续tick数
    static constexpr i32 COOLDOWN_TICKS = 10;

    // ========== 激活/失活/状态查询静态方法 ==========

    /**
     * @brief 检查幽匿感测体是否可以被激活
     *
     * 只有当前 Phase 为 Inactive 时才能被激活。
     */
    [[nodiscard]] static bool canActivate(const BlockState& state);

    /**
     * @brief 激活幽匿感测体
     *
     * 设置方块状态为 Active，设置红石信号强度，调度 tick，通知邻居红石更新，
     * 触发共振事件，发出 SCULK_SENSOR_TENDRILS_CLICKING 游戏事件和声音。
     *
     * @param sourceEntity 触发振动的源实体（可为nullptr）
     * @param world 世界引用
     * @pos 方块位置
     * @param state 当前方块状态
     * @param redstoneStrength 红石信号强度 (1-15)，基于振动距离计算
     * @param frequency 振动频率 (1-15)，用于共振和比较器输出
     */
    static void activate(const Entity* sourceEntity,
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        i32 redstoneStrength,
        i32 frequency);

    /**
     * @brief 停用幽匿感测体（Active→Cooldown）
     *
     * 设置方块状态为 Cooldown，红石信号归零，调度 tick，通知邻居红石更新。
     */
    static void deactivate(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 获取当前相位
     */
    [[nodiscard]] static BlockStateProperties::SculkSensorPhase getPhase(const BlockState& state);

    /**
     * @brief 获取活跃阶段的tick数（子类可覆盖）
     *
     * 普通感测体为30tick，校准感测体为10tick。
     */
    [[nodiscard]] virtual i32 getActiveTicks() const { return ACTIVE_TICKS; }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

/**
 * @brief 校准幽匿感测体
 *
 * 可通过红石信号过滤振动频率的高级感测体。
 * 状态属性：FACING, SCULK_SENSOR_PHASE, POWER, WATERLOGGED
 */
class CalibratedSculkSensorBlock : public SculkSensorBlock {
public:
    explicit CalibratedSculkSensorBlock(const BlockProperties& properties);

    ~CalibratedSculkSensorBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 校准幽匿感测体仅在非输入面方向输出红石信号
     *
     * FACING 方向是输入面（从该方向读取红石信号频率过滤），
     * 红石信号只在非 FACING 方向输出。
     */
    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    /// 校准感测体活跃阶段更短（10 tick）
    static constexpr i32 CALIBRATED_ACTIVE_TICKS = 10;
    [[nodiscard]] i32 getActiveTicks() const override { return CALIBRATED_ACTIVE_TICKS; }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 幽匿催化体
 *
 * 生物在此附近死亡时生成幽匿块。
 * 状态属性：BLOOM
 */
class SculkCatalystBlock : public Block {
public:
    explicit SculkCatalystBlock(const BlockProperties& properties);

    ~SculkCatalystBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

/**
 * @brief 幽匿尖啸体
 *
 * 被激活多次后可召唤监守者。状态属性：SHRIEKING, CAN_SUMMON, WATERLOGGED
 *
 * 激活流程：
 * 1. 实体踩上方块或接收到 SHRIEK 振动事件时触发 tryShriek()
 * 2. tryShriek() 检查 CAN_SUMMON、难度、游戏规则、附近监守者、玩家冷却等条件
 * 3. 条件通过后调用 shriek() 设置 SHRIEKING=true、播放声音、发射 SHRIEK 事件
 * 4. 90 tick 后 tick() 将 SHRIEKING 设回 false 并调用 tryRespond()
 * 5. tryRespond() 播放警告声音、应用黑暗效果、在警告等级 >= 4 时尝试召唤监守者
 *
 * 服务端逻辑（tryShriek, tryRespond, _trySummonWarden 等）位于
 * SculkShriekerHelper（server/world/blockentity/sculk/），因为它们依赖 ServerWorld。
 */
class SculkShriekerBlock : public Block, public IWaterLoggable {
public:
    explicit SculkShriekerBlock(const BlockProperties& properties);

    ~SculkShriekerBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /// 方块实体支持
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;
    [[nodiscard]] BlockEntityType getBlockEntityType() const;

    /**
     * @brief 实体踩上方块时触发
     *
     * 非潜行实体踩上时发出 SHRIEK 游戏事件，触发附近的幽匿尖啸体。
     */
    void onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    /**
     * @brief 方块 tick 回调
     *
     * 处理 SHRIEKING→!SHRIEKING 的状态转换，并通知服务端执行响应逻辑。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 方块移除时的清理
     *
     * 如果方块正在 SHRIEKING 状态时被移除，仍需执行 tryRespond()
     * 以确保警告效果（黑暗效果、监守者召唤检查）被触发。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 静态方法 ==========

    /**
     * @brief 播放尖啸声音和粒子效果，设置 SHRIEKING 状态
     *
     * 此方法仅处理视觉效果和状态转换，不涉及警告等级或召唤逻辑。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param sourceEntity 触发实体（用于游戏事件上下文，可为nullptr）
     */
    static void shriek(IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* sourceEntity);

    /// SHRIEKING 状态持续tick数（90 ticks = 4.5秒）
    static constexpr i32 SHRIEKING_TICKS = 90;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
