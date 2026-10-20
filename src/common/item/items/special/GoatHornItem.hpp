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
#include "common/item/instrument/InstrumentDefinition.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 山羊角物品（对应 vanilla `InstrumentItem`）
 *
 * 右键吹奏：按物品 `instrument` 数据组件解析乐器定义（音效、吹奏时长、可听距离），
 * 播放音效后进入 `floor(use_duration * 20)` tick 冷却，冷却期间不可再次吹奏。
 * 未携带 `instrument` 组件的山羊角（创造模式物品栏直接取出的裸物品）无法吹奏。
 *
 * 乐器定义由数据包 `instrument/` 目录提供（`InstrumentLoader`），组件值即乐器资源名。
 *
 * 参考: net.minecraft.world.item.InstrumentItem
 */
class GoatHornItem : public Item {
public:
    /**
     * @brief 构造山羊角物品
     * @param properties 物品属性
     */
    explicit GoatHornItem(ItemProperties properties);
    ~GoatHornItem() override = default;

    /**
     * @brief 解析物品携带的乐器定义
     *
     * @param stack 山羊角物品堆
     * @return 乐器定义；未携带组件或资源名未注册时返回 nullptr
     */
    [[nodiscard]] static const instrument::InstrumentDefinition* getInstrument(const ItemStack& stack);

    /**
     * @brief 创建携带指定乐器的山羊角物品堆（对应 vanilla `InstrumentItem.create`）
     *
     * @param instrument 乐器定义
     * @return 已挂载 `instrument` 组件的物品堆
     */
    [[nodiscard]] static ItemStack createStack(const instrument::InstrumentDefinition& instrument);

    /**
     * @brief 获取使用时长（吹奏时长，tick）；无乐器定义时为 0（不可使用）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作（吹号角动画）
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用：开始吹奏、播放音色并进入冷却
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;
};

} // namespace item::items
} // namespace mc
