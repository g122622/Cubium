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

#include "../ChestBlock.hpp"
#include "WeatheringCopperBlock.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/copper/IOxidizableBlock.hpp"
#include <memory>

namespace mc {

class IWorld;
class BlockPos;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 铜箱子方块（涂蜡/基础版）
 *
 * 铜箱子是 1.21.11 引入的容器方块，具有以下特性：
 * - 容量 27 格（与普通箱子一致），支持双箱合并（54 格）
 * - 支持 HORIZONTAL_FACING / CHEST_TYPE / WATERLOGGED 三种状态属性
 * - 拥有方块实体（复用 BlockEntityType::Chest 与 ChestEntity）
 * - 可通过蜜脾涂蜡、斧头除蜡/刮削
 * - 双箱合并时允许跨氧化等级与涂蜡状态连接（chestCanConnectTo 检查 COPPER_CHESTS 标签）
 * - 氧化/涂蜡/除蜡/刮削时保留方块实体（物品不丢失）
 *
 * 本类对应未涂蜡氧化等级中的 Unaffected 等级（基础版 copper_chest）
 * 以及所有涂蜡变体（waxed_*_copper_chest）。
 *
 * 类层次结构（与 MC Java 1.21.11 一致）：
 * - CopperChestBlock：基础类（Unaffected 等级 + 涂蜡变种），不实现 IOxidizableBlock
 * - WeatheringCopperChestBlock：继承 CopperChestBlock + IOxidizableBlock
 *   用于 Exposed/Weathered/Oxidized 等级
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向（北南东西）
 * - CHEST_TYPE: 箱子类型（SINGLE/LEFT/RIGHT）
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.world.level.block.CopperChestBlock (MC 1.21.11)
 */
class CopperChestBlock : public ChestBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 当前氧化等级（涂蜡变体也记录对应氧化等级，用于除蜡后恢复）
     * @param openSound 开箱音效事件（不同氧化等级映射不同声音，涂蜡变体复用对应等级声音）
     * @param closeSound 关箱音效事件（不同氧化等级映射不同声音，涂蜡变体复用对应等级声音）
     *
     * 参考 MC Java 1.21.11 CopperChestBlock 构造函数：
     *   CopperChestBlock(WeatherState, SoundEvent openSound, SoundEvent closeSound, Properties)
     * 声音映射表：
     *   - Unaffected/Exposed + 涂蜡变体 -> block.copper_chest.open/close
     *   - Weathered + 涂蜡变体 -> block.copper_chest_weathered.open/close
     *   - Oxidized + 涂蜡变体 -> block.copper_chest_oxidized.open/close
     */
    CopperChestBlock(const BlockProperties& properties,
        BlockStateProperties::OxidationLevel oxidationLevel,
        const ResourceLocation& openSound,
        const ResourceLocation& closeSound);

    ~CopperChestBlock() override = default;

    // ========== 放置 ==========

    /**
     * @brief 获取放置状态
     *
     * 在父类放置逻辑基础上，调用 getLeastOxidizedChestOfConnectedBlocks：
     * 如果与另一个铜箱子合并为双箱，且两者氧化等级或涂蜡状态不同，
     * 取较低氧化等级（且优先未涂蜡）的方块作为合并后的方块类型。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 邻居更新 ==========

    /**
     * @brief 邻居更新
     *
     * 在父类更新逻辑基础上，处理双箱合并时的方块类型同步：
     * 若与相邻铜箱子建立连接，则将当前方块替换为与相邻箱子一致的方块类型
     * （保留当前方块的 FACING/TYPE/WATERLOGGED 属性），确保双箱两侧方块类型一致。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 双箱连接 ==========

    /**
     * @brief 检查相邻方块是否可以与当前铜箱子连接为双箱
     *
     * 只要邻居在 COPPER_CHESTS 标签中且拥有 CHEST_TYPE 属性即可连接，
     * 允许跨氧化等级与涂蜡状态的双箱合并。
     */
    [[nodiscard]] bool chestCanConnectTo(const BlockState& state) const override;

    // ========== 方块实体保留 ==========

    /**
     * @brief 是否在状态变更时保留方块实体
     *
     * 铜箱子在氧化/涂蜡/除蜡/刮削导致方块类型变化时，需要保留方块实体
     * （物品内容、自定义名称等不丢失）。返回 true 表示当前方块是铜箱子，
     * 应当保留方块实体。
     */
    [[nodiscard]] bool shouldChangedStateKeepBlockEntity(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 移除处理 ==========

    /**
     * @brief 方块被移除时的处理
     *
     * 重写父类行为：若新方块仍然是铜箱子（氧化/涂蜡/除蜡/刮削导致的方块类型变化），
     * 则不掉落物品（方块实体将由 createBlockEntity 克隆保留）；
     * 否则调用父类逻辑掉落物品内容物。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 方块实体 ==========

    /**
     * @brief 创建方块实体
     *
     * 若当前位置已存在 ChestEntity（来自氧化/涂蜡/除蜡/刮削前的旧方块），
     * 则克隆旧实体（保留物品内容物、自定义名称等）；否则创建新的空箱子实体。
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 氧化等级访问 ==========

    /**
     * @brief 获取当前氧化等级
     *
     * 用于双箱合并时比较两侧氧化等级，决定合并后方块类型。
     */
    [[nodiscard]] BlockStateProperties::OxidationLevel getOxidationLevel() const noexcept { return m_oxidationLevel; }

    /**
     * @brief 是否为涂蜡变体
     *
     * 用于双箱合并时判断涂蜡状态，以及 CopperChestBlock::getFromCopperBlock
     * 将铜块转换为铜箱子时确定氧化等级。
     * 基础（未涂蜡）CopperChestBlock 返回 false，涂蜡变体由子类重写返回 true。
     */
    [[nodiscard]] virtual bool isWaxed() const noexcept { return false; }

    // ========== 铜块 → 铜箱子转换 ==========

    /**
     * @brief 将铜块转换为对应氧化等级的铜箱子状态
     *
     * 用于铜傀儡生成时替换铜块底座：CarvedPumpkinBlock 在铜傀儡生成后，
     * 用铜箱子替换原铜块位置（保留方块实体内容，对应 MC 1.21.11
     * CarvedPumpkinBlock.replaceCopperBlockWithChest + CopperChestBlock.getFromCopperBlock）。
     *
     * 转换规则（与 MC 1.21.11 COPPER_TO_COPPER_CHEST_MAPPING 一致）：
     * | 铜块                       | 铜箱子                     |
     * |---------------------------|---------------------------|
     * | copper_block              | copper_chest              |
     * | exposed_copper            | exposed_copper_chest      |
     * | weathered_copper          | weathered_copper_chest    |
     * | oxidized_copper           | oxidized_copper_chest     |
     * | waxed_copper_block        | copper_chest              |
     * | waxed_exposed_copper      | exposed_copper_chest      |
     * | waxed_weathered_copper    | weathered_copper_chest    |
     * | waxed_oxidized_copper     | oxidized_copper_chest     |
     *
     * 涂蜡铜块映射到对应氧化等级的"未涂蜡"铜箱子（与 MC 一致：涂蜡状态不传递到箱子）。
     * 未识别的方块（非铜块）回退到 copper_chest（与 MC 默认行为一致）。
     *
     * 返回的 BlockState 包含：
     * - HORIZONTAL_FACING = 传入的 direction
     * - CHEST_TYPE = 根据相邻方块自动判定（Single/Left/Right）
     * - WATERLOGGED = false（生成环境不会在水下）
     *
     * @param copperBlock 源铜块指针（可为 nullptr，回退到 copper_chest）
     * @param facing 朝向
     * @param world 世界引用
     * @param pos 放置位置
     * @return 铜箱子方块状态
     *
     * 参考: net.minecraft.world.level.block.CopperChestBlock.getFromCopperBlock (MC 1.21.11)
     */
    [[nodiscard]] static BlockState getFromCopperBlock(
        const Block* copperBlock, Direction facing, IWorld& world, const BlockPos& pos);

    // ========== 开合音效 ==========

    /**
     * @brief 获取开箱音效
     *
     * 铜箱子根据氧化等级返回不同的开箱音效事件：
     * - Unaffected/Exposed: BLOCK_COPPER_CHEST_OPEN
     * - Weathered: BLOCK_COPPER_CHEST_WEATHERED_OPEN
     * - Oxidized: BLOCK_COPPER_CHEST_OXIDIZED_OPEN
     * 涂蜡变体复用对应氧化等级的声音（在构造时传入相同的声音引用）。
     */
    [[nodiscard]] const ResourceLocation& getOpenSound() const override { return m_openSound; }

    /**
     * @brief 获取关箱音效
     *
     * 铜箱子根据氧化等级返回不同的关箱音效事件：
     * - Unaffected/Exposed: BLOCK_COPPER_CHEST_CLOSE
     * - Weathered: BLOCK_COPPER_CHEST_WEATHERED_CLOSE
     * - Oxidized: BLOCK_COPPER_CHEST_OXIDIZED_CLOSE
     * 涂蜡变体复用对应氧化等级的声音（在构造时传入相同的声音引用）。
     */
    [[nodiscard]] const ResourceLocation& getCloseSound() const override { return m_closeSound; }

protected:
    /// 当前氧化等级（涂蜡变体也记录对应氧化等级）
    BlockStateProperties::OxidationLevel m_oxidationLevel;

    /// 开箱音效（不同氧化等级映射不同声音，涂蜡变体复用对应等级声音）
    const ResourceLocation& m_openSound;

    /// 关箱音效（不同氧化等级映射不同声音，涂蜡变体复用对应等级声音）
    const ResourceLocation& m_closeSound;
};

/**
 * @brief 可风化铜箱子方块
 *
 * 继承 CopperChestBlock + IOxidizableBlock，用于 Exposed/Weathered/Oxidized 等级。
 * 在随机 tick 中尝试氧化到下一等级，但满足以下条件之一时不氧化：
 * - 当前箱子是双箱的 RIGHT 部分（避免双箱两侧同时氧化导致不同步）
 * - 箱子正在被玩家打开（m_openCount > 0）
 *
 * 氧化链：copper_chest -> exposed_copper_chest ->
 *         weathered_copper_chest -> oxidized_copper_chest
 *
 * 参考: net.minecraft.world.level.block.WeatheringCopperChestBlock (MC 1.21.11)
 */
class WeatheringCopperChestBlock : public CopperChestBlock, public IOxidizableBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 初始氧化等级（Exposed/Weathered/Oxidized）
     * @param openSound 开箱音效事件（按氧化等级映射）
     * @param closeSound 关箱音效事件（按氧化等级映射）
     */
    WeatheringCopperChestBlock(const BlockProperties& properties,
        BlockStateProperties::OxidationLevel oxidationLevel,
        const ResourceLocation& openSound,
        const ResourceLocation& closeSound);

    ~WeatheringCopperChestBlock() override = default;

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
     *
     * 满足以下条件之一时不氧化：
     * - 当前箱子是双箱的 RIGHT 部分（CHEST_TYPE == RIGHT）
     * - 箱子正在被玩家打开（ChestEntity::getOpenCount() > 0）
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

private:
    /// 下一氧化等级对应的方块（nullptr 表示已是最高等级）
    Block* m_nextOxidationBlock = nullptr;

    /// 上一氧化等级对应的方块（nullptr 表示已是最低等级，用于斧头刮削）
    Block* m_previousOxidationBlock = nullptr;
};

/**
 * @brief 涂蜡铜箱子方块
 *
 * 涂蜡后的铜箱子不会氧化。重写 isWaxed() 返回 true，
 * 用于双箱合并时判断涂蜡状态。
 *
 * 参考: net.minecraft.world.level.block.CopperChestBlock (MC 1.21.11, 涂蜡变体)
 */
class WaxedCopperChestBlock : public CopperChestBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 对应的氧化等级（涂蜡变体记录氧化等级，用于除蜡后恢复）
     * @param openSound 开箱音效事件（涂蜡变体复用对应氧化等级的声音）
     * @param closeSound 关箱音效事件（涂蜡变体复用对应氧化等级的声音）
     */
    WaxedCopperChestBlock(const BlockProperties& properties,
        BlockStateProperties::OxidationLevel oxidationLevel,
        const ResourceLocation& openSound,
        const ResourceLocation& closeSound)
        : CopperChestBlock(properties, oxidationLevel, openSound, closeSound)
    {}

    ~WaxedCopperChestBlock() override = default;

    /**
     * @brief 是否为涂蜡变体
     * @return 始终返回 true
     */
    [[nodiscard]] bool isWaxed() const noexcept override { return true; }
};

} // namespace blocks
} // namespace mc
