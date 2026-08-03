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
#include "WeatheringCopperBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/blocks/copper/IOxidizableBlock.hpp"
#include <memory>

namespace mc {

class IWorld;
class BlockPos;
class Player;
class BlockRaycastResult;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 铜傀儡雕像方块（涂蜡/基础版）
 *
 * 铜傀儡雕像是一种装饰性方块，具有以下特性：
 * - 支持 4 种姿态（Standing/Sitting/Running/Star），玩家右键点击循环切换
 * - 红石比较器模拟输出 = POSE.ordinal() + 1 (1-4)
 * - 支持水平朝向（FACING 属性）
 * - 支持含水（WATERLOGGED 属性）
 * - 拥有方块实体，用于保存 CUSTOM_NAME 组件和 POSE 状态
 *
 * 本类对应未涂蜡氧化等级中的 Unaffected 等级（基础版 copper_golem_statue）
 * 以及涂蜡变体（waxed_*_copper_golem_statue）。基础版使用斧头时返回 PASS
 * 以便让 WeatheringCopperGolemStatueBlock 处理氧化与铜傀儡生成逻辑。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向（北南东西）
 * - COPPER_GOLEM_POSE: 姿态
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.world.level.block.CopperGolemStatueBlock (MC 1.21.11)
 */
class CopperGolemStatueBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CopperGolemStatueBlock(const BlockProperties& properties);

    ~CopperGolemStatueBlock() override = default;

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     *
     * 根据玩家水平朝向设置 FACING（与玩家朝向相反），
     * 并根据位置流体状态设置 WATERLOGGED。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转/镜像 ==========

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 邻居更新 ==========

    /**
     * @brief 邻居更新
     *
     * 含水时调度水流体 tick。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     *
     * 铜傀儡雕像使用固定的圆柱形碰撞形状：
     * 底面 3x3 到 13x13，高度 0 到 14（直径 10，高 14）。
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 玩家交互 ==========

    /**
     * @brief 玩家右键点击
     *
     * 持有斧头时返回 PASS（委托给 WeatheringCopperGolemStatueBlock 处理刮削/生成逻辑）；
     * 否则循环切换 POSE 并播放铜傀儡变雕像音效。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 红石 ==========

    /**
     * @brief 是否有比较器输入覆盖
     *
     * 铜傀儡雕像支持比较器模拟输出。
     */
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 获取比较器输入覆盖值
     *
     * 返回 POSE.ordinal() + 1（范围 1-4）。
     */
    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 方块实体 ==========

    /**
     * @brief 是否拥有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    /**
     * @brief 是否在状态变更时保留方块实体
     *
     * 铜傀儡雕像在氧化/涂蜡/除蜡/刮削导致方块类型变化时需要保留方块实体
     * （CUSTOM_NAME 自定义名称等不丢失）。返回 true 让 ServerWorld::setBlockState
     * 迁移旧 BlockEntity 至新方块而非创建空实体。
     *
     * 注意：基础 CopperGolemStatueBlock（Unaffected 等级）实际不会发生氧化导致的
     * 方块类型变化（不实现 IOxidizableBlock），但涂蜡变体被斧头除蜡时会变为
     * 基础变体，此时需要保留 CUSTOM_NAME。返回 true 覆盖所有铜傀儡雕像变体。
     */
    [[nodiscard]] bool shouldChangedStateKeepBlockEntity(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 姿态工具 ==========

    /**
     * @brief 循环切换到下一个姿态
     *
     * Standing -> Sitting -> Running -> Star -> Standing
     */
    [[nodiscard]] static BlockStateProperties::CopperGolemPose getNextPose(
        BlockStateProperties::CopperGolemPose current) noexcept;

protected:
    /**
     * @brief 填充状态容器
     *
     * 状态在构造函数中通过 Builder 创建，此处为空实现。
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    /**
     * @brief 播放变雕像音效并触发 BLOCK_CHANGE 游戏事件
     *
     * 对应 MC Java CopperGolemStatueBlock.updatePose()。
     */
    void updatePose(IWorld& world, const BlockState& state, const BlockPos& pos, Player& player) const;

    /// 雕像的固定圆柱形碰撞形状
    CollisionShape m_shape;
};

/**
 * @brief 可风化铜傀儡雕像方块
 *
 * 继承 CopperGolemStatueBlock，额外实现 IOxidizableBlock 接口。
 * 在随机 tick 中尝试氧化到下一等级，并通过 withPropertiesOf() 保留
 * HORIZONTAL_FACING/COPPER_GOLEM_POSE/WATERLOGGED 等共有属性。
 *
 * 氧化链：copper_golem_statue -> exposed_copper_golem_statue ->
 *         weathered_copper_golem_statue -> oxidized_copper_golem_statue
 *
 * 注意：MC 原版中基础 copper_golem_statue 是 CopperGolemStatueBlock
 * （不实现 WeatheringCopper，不参与氧化 tick），但处于氧化链的 Unaffected 位置。
 * 暴露/锈蚀/氧化等级使用本类，可氧化到下一等级。
 *
 * 参考: net.minecraft.world.level.block.WeatheringCopperGolemStatueBlock (MC 1.21.11)
 */
class WeatheringCopperGolemStatueBlock : public CopperGolemStatueBlock, public IOxidizableBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 初始氧化等级
     */
    WeatheringCopperGolemStatueBlock(
        const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel);

    ~WeatheringCopperGolemStatueBlock() override = default;

    // ========== 氧化接口实现 ==========

    /**
     * @brief 获取当前氧化等级
     */
    [[nodiscard]] BlockStateProperties::OxidationLevel getOxidationLevel() const override { return m_oxidationLevel; }

    /**
     * @brief 设置下一氧化等级对应的方块
     */
    void setNextOxidationBlock(Block* nextBlock) { m_nextOxidationBlock = nextBlock; }

    /**
     * @brief 获取下一氧化等级对应的方块
     */
    [[nodiscard]] Block* getNextOxidationBlock() const override { return m_nextOxidationBlock; }

    /**
     * @brief 设置上一氧化等级对应的方块（用于斧头刮削）
     */
    void setPreviousOxidationBlock(Block* prevBlock) { m_previousOxidationBlock = prevBlock; }

    /**
     * @brief 获取上一氧化等级对应的方块
     */
    [[nodiscard]] Block* getPreviousOxidationBlock() const override { return m_previousOxidationBlock; }

    // ========== 随机 Tick ==========

    /**
     * @brief 随机 Tick - 尝试氧化
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     *
     * 只有未达到最高氧化等级 (Oxidized) 的方块才响应随机刻。
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override
    {
        return m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized;
    }

protected:
    /**
     * @brief 填充状态容器
     *
     * 与父类属性集合一致（HORIZONTAL_FACING + COPPER_GOLEM_POSE + WATERLOGGED），
     * 氧化等级不进入 block state。
     */
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    /// 当前氧化等级
    BlockStateProperties::OxidationLevel m_oxidationLevel;

    /// 下一氧化等级对应的方块（nullptr 表示已是最高等级）
    Block* m_nextOxidationBlock = nullptr;

    /// 上一氧化等级对应的方块（nullptr 表示已是最低等级，用于斧头刮削）
    Block* m_previousOxidationBlock = nullptr;
};

} // namespace blocks
} // namespace mc
