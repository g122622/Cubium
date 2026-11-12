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

#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/blocks/container/CopperChestBlock.hpp"
#include "common/world/block/capability/oxidation/IOxidizableBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"

#include <memory>

namespace mc {

class IWorld;
class BlockPos;
class BlockItemUseContext;

namespace blocks {

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
    [[nodiscard]] BlockStateProperties::OxidationLevel getOxidationLevel() const override
    {
        return m_oxidationLevel;
    }

    /**
     * @brief 设置下一氧化等级对应的方块
     */
    void setNextOxidationBlock(Block* nextBlock)
    {
        m_nextOxidationBlock = nextBlock;
    }

    /**
     * @brief 获取下一氧化等级对应的方块
     */
    [[nodiscard]] Block* getNextOxidationBlock() const override
    {
        return m_nextOxidationBlock;
    }

    /**
     * @brief 设置上一氧化等级对应的方块（用于斧头刮削）
     */
    void setPreviousOxidationBlock(Block* prevBlock)
    {
        m_previousOxidationBlock = prevBlock;
    }

    /**
     * @brief 获取上一氧化等级对应的方块
     */
    [[nodiscard]] Block* getPreviousOxidationBlock() const override
    {
        return m_previousOxidationBlock;
    }

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
        // vanilla WeatheringCopperChestBlock#isRandomlyTicking：getNext(block).isPresent()。
        // WeatheringCopper 的 NEXT_BY_BLOCK 链条是 Unaffected → Exposed → Weathered → Oxidized，
        // 「Unaffected」只是链条的起点（名字有误导性），它同样会被随机刻氧化。
        // 因此这里只排除已达顶点的 Oxidized。
        return m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized;
    }

private:
    /// 下一氧化等级对应的方块（nullptr 表示已是最高等级）
    Block* m_nextOxidationBlock = nullptr;

    /// 上一氧化等级对应的方块（nullptr 表示已是最低等级，用于斧头刮削）
    Block* m_previousOxidationBlock = nullptr;
};

} // namespace blocks
} // namespace mc
