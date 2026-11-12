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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/blocks/base/FallingBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "world/blockentity/BlockEntityType.hpp"

#include <memory>

namespace mc {
namespace blocks {

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
    [[nodiscard]] const ResourceLocation& getBrushSound() const noexcept
    {
        return m_brushSound;
    }

    /**
     * @brief 获取刷扫完成音效
     *
     * 当 BrushableBlockEntity.brush() 返回 true（即刷扫完成，刷出物品）时播放。
     * 可疑沙返回 BRUSH_SAND_COMPLETED，可疑沙砾返回 BRUSH_GRAVEL_COMPLETED。
     *
     * @return 刷扫完成音效资源位置
     */
    /**
     * @brief 坠落落地碎开（原版 `BrushableBlock#onBrokenAfterFall`）
     *
     * 可疑沙/沙砾在下落时被摔碎：先放方块破碎粒子，再发 `block_destroy` 事件
     * （幽匿感测体据此听到「方块被毁」）。原版这两个动作都在这个方法里，
     * 所以必须一起做，只发粒子不发事件会让坠落破碎在幽匿电路里隐形。
     *
     * @param world 世界引用
     * @param pos 落地方块位置
     * @param entity 碎开的掉落方块实体
     */
    void onBroken(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity) override;

    [[nodiscard]] const ResourceLocation& getBrushCompletedSound() const noexcept
    {
        return m_brushCompletedSound;
    }

    /**
     * @brief 获取刷扫完成后转换成的目标方块
     *
     * 对应 MC 1.21.11 BrushableBlock.getTurnsInto()。
     * 可疑沙返回沙方块，可疑沙砾返回沙砾方块。
     *
     * @return 目标方块指针（可能为 nullptr，调用方需检查）
     */
    [[nodiscard]] const Block* getTurnsInto() const noexcept
    {
        return m_turnsInto;
    }

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockEntityType getBlockEntityType() const noexcept
    {
        return BlockEntityType::BrushableBlock;
    }

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

} // namespace blocks
} // namespace mc
