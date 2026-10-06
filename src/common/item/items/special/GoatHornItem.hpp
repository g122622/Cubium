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

#include "common/item/core/Item.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 山羊角物品
 *
 * 右键吹奏：播放一种山羊角音色（8 种变体），并进入约 7 秒（140 tick）冷却，
 * 冷却期间不可再次吹奏。吹奏时长与范围取决于所选音色。
 *
 * 参考: net.minecraft.world.item.InstrumentItem
 *
 * TODO: 对齐 vanilla 的 Instrument 数据驱动体系——8 种音色（item.goat_horn.sound.0-7）
 *   应作为 Instrument 注册表项挂到物品的 INSTRUMENT 数据组件上，由数据包定义
 *   sound_event/use_duration/range。当前项目尚无 Instrument 注册表与组件，
 *   故以固定的音色索引（构造函数传入）与硬编码时长近似实现，待组件体系接入后迁移。
 */
class GoatHornItem : public Item {
public:
    /// 吹奏时长（tick），对齐 vanilla Instrument 默认 use_duration=7.0 秒
    static constexpr i32 USE_DURATION_TICKS = 140;

    /**
     * @brief 构造山羊角物品
     * @param variantIndex 音色变体索引（0-7，对应 item.goat_horn.sound.N）
     * @param properties 物品属性
     */
    GoatHornItem(i32 variantIndex, ItemProperties properties);
    ~GoatHornItem() override = default;

    /**
     * @brief 获取使用时长（吹奏时长）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作（吹号角动画）
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用：吹奏并进入冷却
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 获取音色变体索引
     */
    [[nodiscard]] i32 variantIndex() const noexcept { return m_variantIndex; }

private:
    i32 m_variantIndex; ///< 音色变体索引（0-7）
};

} // namespace item::items
} // namespace mc
