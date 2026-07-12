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

#include "../../IWaterLoggable.hpp"
#include "../FallingBlock.hpp"
#include "../HorizontalBlock.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "world/blockentity/BlockEntityType.hpp"

namespace mc {

class Player;

namespace blocks {

/**
 * @brief 雕纹书架方块
 *
 * 可放置6本书的书架，可被红石比较器检测。
 * 状态属性：FACING, SLOT_0_OCCUPIED ~ SLOT_5_OCCUPIED
 */
class ChiseledBookshelfBlock : public HorizontalBlock {
public:
    explicit ChiseledBookshelfBlock(const BlockProperties& properties);

    ~ChiseledBookshelfBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 饰纹陶罐方块
 *
 * 由陶片合成的装饰性容器方块，可存放一个物品。
 * 状态属性：FACING, CRACKED, WATERLOGGED
 */
class DecoratedPotBlock : public HorizontalBlock, public IWaterLoggable {
public:
    explicit DecoratedPotBlock(const BlockProperties& properties);

    ~DecoratedPotBlock() override = default;

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

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockEntityType getBlockEntityType() const noexcept { return BlockEntityType::DecoratedPot; }

    // ========== 红石比较器 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 玩家右键交互
     *
     * 手持物品时：向陶罐中放入1个物品（如果可以放入），
     * 触发正摇晃(Positive wobble)动画和插入音效。
     * 空手时：触发负摇晃(Negative wobble)动画和插入失败音效。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 玩家破坏前处理
     *
     * 如果玩家手持的物品具有 BREAKS_DECORATED_POTS 标签
     * （如剑、斧等工具），且没有精准采集附魔，
     * 则将陶罐设为 CRACKED 状态。
     * CRACKED 状态的陶罐被破坏时会掉落4个单独的陶片而非陶罐物品。
     */
    void playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player) override;

    /**
     * @brief 投射物命中处理
     *
     * 投射物命中陶罐时，总是将陶罐设为 CRACKED 状态然后破坏。
     * 投射物破坏的陶罐总是碎裂为陶片，不受精准采集保护。
     */
    void onProjectileHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile) override;

    /**
     * @brief 方块移除时处理
     *
     * 掉落陶罐内存储的物品，并触发容器邻居更新。
     * 陶罐物品本身由战利品表系统处理掉落。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 中键选取物品（创造模式）
     *
     * 返回带有陶罐图案数据的物品堆，保留 sherds 信息。
     * 不包含罐内存储的物品。
     */
    [[nodiscard]] ItemStack getCloneItemStack(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

/**
 * @brief 可刷方块（可疑沙/可疑沙砾）
 *
 * 可被刷子刷出考古物品的方块。受重力影响。
 * 状态属性：DUSTED (0-3)
 *
 * 每个 BrushableBlock 实例绑定一个刷扫音效（brushSound）和刷扫完成音效
 * （brushCompletedSound），由构造函数传入。MC 1.21.11 中 BrushableBlock 通过
 * 数据驱动字段 brush_sound / brush_completed_sound / turns_into 配置，
 * 本项目以构造参数形式提供等价能力。
 *
 * 刷扫完成时（BrushableBlockEntity.brush() 返回 true），方块会被替换为
 * `turnsInto` 指向的普通方块（可疑沙→沙，可疑沙砾→沙砾）。
 *
 * 方块 tick（计划刻）会先调用方块实体上的 checkReset() 递减刷扫计数，
 * 再执行 FallingBlock 的下落检测逻辑（对齐 MC 1.21.11 BrushableBlock.tick）。
 */
class BrushableBlock : public FallingBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param turnsInto 刷扫完成后转换成的目标方块（如沙、沙砾）
     * @param brushSound 刷扫过程中循环播放的音效（对应 MC 的 brush_sound）
     * @param brushCompletedSound 刷扫完成时播放的音效（对应 MC 的 brush_completed_sound）
     */
    BrushableBlock(const BlockProperties& properties,
        const Block* turnsInto,
        ResourceLocation brushSound,
        ResourceLocation brushCompletedSound);

    ~BrushableBlock() override = default;

    /**
     * @brief 获取刷扫音效
     *
     * BrushItem::onUseTick 在每次刷扫触发 tick 调用此方法获取音效并播放。
     * 可疑沙返回 BRUSH_SAND，可疑沙砾返回 BRUSH_GRAVEL。
     *
     * @return 刷扫音效资源位置
     */
    [[nodiscard]] const ResourceLocation& getBrushSound() const noexcept { return m_brushSound; }

    /**
     * @brief 获取刷扫完成音效
     *
     * 当 BrushableBlockEntity.brush() 返回 true（即刷扫完成，刷出物品）时播放。
     * 可疑沙返回 BRUSH_SAND_COMPLETED，可疑沙砾返回 BRUSH_GRAVEL_COMPLETED。
     *
     * @return 刷扫完成音效资源位置
     */
    [[nodiscard]] const ResourceLocation& getBrushCompletedSound() const noexcept { return m_brushCompletedSound; }

    /**
     * @brief 获取刷扫完成后转换成的目标方块
     *
     * 对应 MC 1.21.11 BrushableBlock.getTurnsInto()。
     * 可疑沙返回沙方块，可疑沙砾返回沙砾方块。
     *
     * @return 目标方块指针（可能为 nullptr，调用方需检查）
     */
    [[nodiscard]] const Block* getTurnsInto() const noexcept { return m_turnsInto; }

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockEntityType getBlockEntityType() const noexcept { return BlockEntityType::BrushableBlock; }

    // ========== 方块 tick（计划刻）==========

    /**
     * @brief 计划刻回调
     *
     * 对齐 MC 1.21.11 BrushableBlock.tick：
     * 1. 获取 BrushableBlockEntity 并调用 checkReset() 递减刷扫计数
     * 2. 执行 FallingBlock 的下落检测逻辑
     *
     * 注意：FallingBlock::tick 已经实现了下落检测，此处先调用 checkReset
     * 再委托给基类执行下落逻辑。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    const Block* m_turnsInto;
    ResourceLocation m_brushSound;
    ResourceLocation m_brushCompletedSound;
};

/**
 * @brief 嗅探兽蛋方块
 *
 * 可孵化出嗅探兽生物的蛋方块。
 * 状态属性：HATCH (0-2)
 *
 * 孵化机制（对齐 MC 1.21.11 SnifferEggBlock）：
 * - 放置时（onBlockAdded）通过 scheduleTick 调度孵化 tick。
 * - 调度延迟：常规 24000/3 + [0, 300) tick；加速 12000/3 + [0, 300) tick。
 * - 加速条件：下方方块在 BlockTags::SNIFFER_EGG_HATCH_BOOST 标签中（如苔藓块）。
 * - tick 回调推进 HATCH 等级：0→1→2，到 2 时孵化完成生成嗅探兽幼体。
 * - 加速放置时广播 WorldEvents::EGG_CRACK (3009) 粒子事件。
 * - 不再依赖 randomTick，因此 ticksRandomly() 返回 false。
 */
class SnifferEggBlock : public Block {
public:
    explicit SnifferEggBlock(const BlockProperties& properties);

    ~SnifferEggBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 方块放置时调度孵化 tick
     *
     * 检测下方方块是否在 SNIFFER_EGG_HATCH_BOOST 标签中，决定孵化总时长
     * （12000 或 24000 tick），分三阶段调度（i / 3 + [0, 300) tick）。
     * 加速时广播 EGG_CRACK 粒子事件，并发出 BLOCK_PLACE 游戏事件。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 计划刻回调 - 推进孵化进度
     *
     * HATCH < 2：播放 SNIFFER_EGG_CRACK 音效并 +1 等级。
     * HATCH = 2：播放 SNIFFER_EGG_HATCH 音效，销毁蛋方块，生成嗅探兽幼体。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 检查下方方块是否为孵化加速方块
     *
     * @param world 方块查询接口
     * @param pos 蛋方块位置
     * @return 下方方块是否在 SNIFFER_EGG_HATCH_BOOST 标签中
     */
    [[nodiscard]] static bool hatchBoost(IWorld& world, const BlockPos& pos);

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_noCrackShape;
    CollisionShape m_crackedShape;
    CollisionShape m_hatchingShape;
};

} // namespace blocks
} // namespace mc
