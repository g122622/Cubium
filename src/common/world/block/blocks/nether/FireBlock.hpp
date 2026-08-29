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
#include "../../BlockTags.hpp"
#include "../../Material.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

// 测试 fixture 前向声明（全局命名空间，tests/common/world/block/blocks/FireBlockTest.cpp），
// 供 FireBlock 经 friend 授权访问 protected getIncreasedFireBurnout 做偏差 #4 回退验证。
class FireBlockBiomeTest;

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 火方块基类
 *
 * 火的基类，包括普通火和灵魂火。
 * 火可以蔓延到可燃方块上。
 *
 * 状态属性：
 * - AGE_0_15: 火的年龄（影响蔓延）
 * - NORTH/SOUTH/EAST/WEST/UP: 各方向的连接
 */
class FireBlock : public Block {
public:
    explicit FireBlock(const BlockProperties& properties, i32 fireDamage = 1);
    ~FireBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] BlockState withAge(i32 age) const;

    // ========== 放置逻辑 ==========

    /**
     * @brief 根据环境选择正确的火焰状态
     *
     * 如果目标位置下方是灵魂沙或灵魂土，返回灵魂火状态；否则返回普通火状态。
     * 对应 MC 原版 BaseFireBlock.getState()。
     *
     * @param world 世界引用
     * @param pos 要放置火焰的位置
     * @return 应该放置的火焰方块状态
     */
    [[nodiscard]] static const BlockState& getFireState(IWorld& world, const BlockPos& pos);

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== Tick ==========

    // 对齐 vanilla FireBlock.tick（FireBlock.java:143-218）：火焰由计划刻驱动
    // （onBlockAdded 调度 + tick 首行自我续期），不响应随机刻。
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // 火焰不响应随机刻（对齐 vanilla FireBlock 无 randomTick）。此前 Cubium 用
    // randomTick 驱动火焰蔓延，与 vanilla 计划刻范式不一致（节奏受 randomTickSpeed
    // 影响而非 vanilla 的 30~39 tick 周期），且导致 tick() 内的雨天熄灭等逻辑成为死代码。
    [[nodiscard]] bool ticksRandomly() const noexcept override { return false; }

    // ========== 放置回调 ==========

    /**
     * @brief 方块被添加到世界时调用
     *
     * 对齐 vanilla FireBlock.onPlace（FireBlock.java:294-297）：
     *   1. 检测并点燃下界传送门（立即，而非 tick 时）；
     *   2. 调度计划刻 scheduleBlockTick(pos, this, 30 + nextInt(10))，驱动火焰 tick 链路。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 新方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

protected:
    // 测试 fixture（tests/common/world/block/blocks/FireBlockTest.cpp），经 friend 访问
    // protected getIncreasedFireBurnout，验证偏差 #4 修复（群系标志替代 isRaining 近似）。
    friend class ::FireBlockBiomeTest;

    /**
     * @brief 火焰计划刻延迟（对齐 vanilla FireBlock.getFireTickDelay）
     *
     * 返回 30 + nextInt(10)，即 30~39 tick。onBlockAdded 首次调度与 tick 自我续期
     * 均用此延迟，形成 vanilla 的火焰 tick 周期。
     *
     * @param random 随机数生成器
     * @return 计划刻延迟（tick）
     */
    [[nodiscard]] static i32 getFireTickDelay(math::IRandom& random) { return 30 + random.nextInt(10); }

    /**
     * @brief 按维度获取 infiniburn 标签（对齐 vanilla dimensionType().infiniburn()）
     *
     * 主世界→INFINIBURN_OVERWORLD，下界→INFINIBURN_NETHER，末地→INFINIBURN_END，
     * 其他维度默认 INFINIBURN_OVERWORLD。FireBlock::tick 用此标签查 belowState 判定无限火源。
     *
     * @param dimension 维度 ID（0=主世界, -1=下界, 1=末地）
     * @return 对应维度的 infiniburn 标签引用
     */
    [[nodiscard]] static const BlockTag& getInfiniburnTag(DimensionId dimension);

    /**
     * @brief 检查位置是否可以燃烧（有可燃方块）
     *
     * 检查指定位置周围是否有可燃方块。
     *
     * @param world 世界读取器
     * @param pos 要检查的位置
     * @return 如果位置可以燃烧返回 true
     */
    [[nodiscard]] virtual bool canBurn(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 尝试火焰蔓延
     *
     * 在火焰 tick 时调用，尝试将火焰蔓延到周围方块。
     *
     * @param world 世界引用
     * @param pos 火焰位置
     * @param age 火焰年龄 (0-15)
     * @param random 随机数生成器
     */
    void trySpread(IWorld& world, const BlockPos& pos, i32 age, math::IRandom& random);

    /**
     * @brief 尝试点燃下界传送门
     *
     * 检查火焰周围是否形成有效的下界传送门框架，
     * 如果有效则点燃传送门。
     *
     * @param world 世界引用
     * @param pos 火焰位置
     * @return 是否成功点燃传送门
     */
    bool tryLightNetherPortal(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块是否可燃
     */
    [[nodiscard]] bool isFlammable(const BlockState& state) const;

    /**
     * @brief 检查火焰是否会被雨淋灭
     *
     * @param world 世界引用
     * @param pos 火焰位置
     * @return 如果火焰会被雨淋灭返回 true
     */
    [[nodiscard]] bool canDie(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查指定位置是否会被雨淋灭
     *
     * 同 canDie，用于远距离蔓延检查。
     *
     * @param world 世界引用
     * @param pos 位置
     * @return 如果位置会被雨淋灭返回 true
     */
    [[nodiscard]] bool canDieAt(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 查询当前位置的群系是否为"火焰加速熄灭"群系
     *
     * 对齐 vanilla 1.21.11 FireBlock.checkBurnOut（FireBlock.java:178-179）的 flag1：
     * 读群系级 EnvironmentAttributes.INCREASED_FIRE_BURNOUT 环境属性。Cubium 经
     * ChunkData::getBiomeAtBlock（运行时查已生成的群系，O(1) 无 Voronoi 重算，等价
     * vanilla Level.getBiome 运行时查 chunk biome palette）+ BiomeRegistry 取群系标志。
     *
     * 潮湿/特殊群系（vanilla 8 个：swamp/mangrove_swamp/jungle/bamboo_jungle/mushroom_fields/
     * frozen_peaks/jagged_peaks/snowy_slopes）置 true，恒定生效（与是否下雨无关）。
     *
     * TODO: 完整 EnvironmentAttributes 系统实现后，迁移到
     * world.environmentAttributes().getValue(INCREASED_FIRE_BURNOUT, pos)。
     *
     * @param world 世界引用
     * @param pos 火焰位置
     * @return 是否为火焰加速熄灭群系
     */
    [[nodiscard]] bool getIncreasedFireBurnout(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 获取目标位置周围的火焰蔓延鼓励值
     *
     * @param world 世界引用
     * @param pos 目标位置
     * @return 最大火焰蔓延速度
     */
    [[nodiscard]] i32 getNeighborEncouragement(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 尝试点燃指定位置
     *
     * @param world 世界引用
     * @param pos 目标位置
     * @param chance 基础点燃概率分母
     * @param random 随机数生成器
     * @param age 火焰年龄
     * @param face 点燃面
     */
    void tryCatchFire(IWorld& world, const BlockPos& pos, i32 chance, math::IRandom& random, i32 age, Direction face);

    /**
     * @brief 检查周围是否有可燃方块
     *
     * @param world 世界读取器
     * @param pos 位置
     * @return 如果周围有可燃方块返回 true
     */
    [[nodiscard]] bool areNeighborsFlammable(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 检查位置是否可以被点燃
     *
     * @param world 世界引用
     * @param pos 目标位置
     * @param face 点燃面
     * @return 如果位置可以被点燃返回 true
     */
    [[nodiscard]] bool canCatchFire(IWorld& world, const BlockPos& pos, Direction face) const;

    /// 火焰伤害
    i32 m_fireDamage;

    /// 火焰形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
