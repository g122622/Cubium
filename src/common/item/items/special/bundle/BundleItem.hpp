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

#include "BundleContents.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/color/DyeColor.hpp"

namespace mc {

class Slot;
class Player;
class LivingEntity;
namespace item::items {

/**
 * @brief 收纳袋物品（17 色变体）
 *
 * 对应 MC 1.21.11 的 net.minecraft.world.item.BundleItem。
 *
 * 功能：
 * - 右键使用（onItemRightClick）：触发使用动作，开始按住使用
 * - onUseTick：周期性丢出内容物（首次满时长 + 每 2 tick 一次）
 * - 使用结束：由 LivingEntity 自动停止，无需 onItemUseFinish（与 MC 1.21.11 一致，
 *   MC 原版 BundleItem 也未重写 finishUsingItem；最后一次丢出在 onUseTick 中完成）
 * - overrideStackedOnOther：玩家手持收纳袋点击槽位时插入/取出
 * - overrideOtherStackedOnMe：玩家手持其他物品点击收纳袋槽位时插入/取出
 * - 染色变体（16 色 + 1 无色）：颜色为物品固有属性，与 HarnessItem 模式一致
 *
 * NBT 存储：tag.custom_data.BundleContents（JSON 对象）
 *
 * 权重系统（整数算术）：
 * - 单个物品权重 = 64 / maxStackSize（向上取整）
 * - 总权重上限 = 64（对应 MC 的 Fraction.ONE）
 * - 收纳袋嵌套权重 = 4 + 内袋权重（对应 MC 的 1/16 + inner.weight）
 *
 * 参考: net.minecraft.world.item.BundleItem
 */
class BundleItem : public Item {
public:
    /**
     * @brief 构造收纳袋
     * @param properties 物品属性（maxStackSize=1）
     * @param color 染料颜色（DyeColor::Count 表示无色变体）
     */
    BundleItem(ItemProperties properties, DyeColor color);

    ~BundleItem() override = default;

    /**
     * @brief 获取颜色
     */
    [[nodiscard]] DyeColor getColor() const noexcept { return m_color; }

    // ========== Item 接口实现 ==========

    /**
     * @brief 右键使用：开始按住使用收纳袋
     *
     * 对应 MC 1.21.11 BundleItem#use：调用 player.startUsingItem(hand)。
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 使用过程中每 tick 调用：周期性丢出内容物
     *
     * 对应 MC 1.21.11 BundleItem#onUseTick：
     * - 首次（elapsedTicks == totalDuration）丢出一次
     * - 之后每 2 tick 丢出一次，直到使用时间剩余 < 10
     */
    void onUseTick(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 elapsedTicks) override;

    /**
     * @brief 获取使用时长（200 ticks）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作类型（Bundle）
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 物品被破坏时掉落内容物
     *
     * 对应 MC 1.21.11 BundleItem#onDestroyed(ItemEntity)：
     * 清空 BundleContents 并在世界中生成内容物物品实体。
     * 注意：本项目的 onDestroyed 签名与 MC 不同（ItemEntity vs Entity），
     * 此处采用一致的行为：将内容物作为物品实体生成在原位置。
     */
    void onDestroyed(ItemStack& stack, IWorld& world, Entity& entity) override;

    // ========== 槽位覆写协议（MC 1.20+）==========

    /**
     * @brief 玩家手持收纳袋点击其他槽位时的覆写行为
     *
     * 对应 MC 1.21.11 BundleItem#overrideStackedOnOther。
     * - 左键 + 槽位有物品：尝试将槽位物品转入收纳袋
     * - 右键 + 槽位为空：从收纳袋取出一项放入槽位
     *
     * @param heldStack 玩家手持的收纳袋
     * @param slot 被点击的槽位
     * @param clickAction 点击动作（Primary/Secondary）
     * @param player 玩家
     * @return 是否处理了此次点击（true 阻止默认行为）
     */
    bool overrideStackedOnOther(ItemStack& heldStack, Slot& slot, SlotClickAction clickAction, Player& player) override;

    /**
     * @brief 玩家手持其他物品点击收纳袋槽位时的覆写行为
     *
     * 对应 MC 1.21.11 BundleItem#overrideOtherStackedOnMe。
     * - 左键 + 手持物品非空：尝试将手持物品插入收纳袋
     * - 右键 + 手持物品为空：从收纳袋取出一项到光标
     *
     * @param bundleStack 收纳袋槽位中的物品堆
     * @param cursorStack 玩家光标上的物品堆
     * @param slot 收纳袋所在槽位
     * @param clickAction 点击动作
     * @param player 玩家
     * @return 是否处理了此次点击
     */
    bool overrideOtherStackedOnMe(ItemStack& bundleStack,
        ItemStack& cursorStack,
        Slot& slot,
        SlotClickAction clickAction,
        Player& player) override;

    // ========== 静态工具方法 ==========

    /**
     * @brief 切换选中项
     *
     * 对应 MC 1.21.11 BundleItem#toggleSelectedItem。
     * 用于客户端标记选中项以高亮显示。
     */
    static void toggleSelectedItem(ItemStack& stack, i32 index);

    /**
     * @brief 是否有选中项
     */
    [[nodiscard]] static bool hasSelectedItem(const ItemStack& stack);

    /**
     * @brief 获取选中项索引
     */
    [[nodiscard]] static i32 getSelectedItem(const ItemStack& stack);

    /**
     * @brief 获取选中项物品堆
     */
    [[nodiscard]] static ItemStack getSelectedItemStack(const ItemStack& stack);

    /**
     * @brief 获取要显示的物品数量
     */
    [[nodiscard]] static i32 getNumberOfItemsToShow(const ItemStack& stack);

    /**
     * @brief 获取满度（0.0~1.0，用于 GUI 进度条）
     */
    [[nodiscard]] static f32 getFullnessDisplay(const ItemStack& stack);

    // BundleTooltip 渲染：kagero 体系暂未实现收纳袋图像 tooltip（ItemTooltipBuilder
    // 仅构建文本 tooltip）。上述 getNumberOfItemsToShow / getFullnessDisplay 仍保留为
    // 公共 API，供其他模块（如 HUD、第三方插件）查询收纳袋状态使用。

    /**
     * @brief 判断物品堆是否为收纳袋（任意颜色变体）
     *
     * 通过物品 ID 后缀 "_bundle" 或完全匹配 "bundle" 识别。
     */
    [[nodiscard]] static bool isBundleItem(const ItemStack& stack);

    // ========== 颜色变体工具 ==========

    /**
     * @brief 根据颜色获取对应收纳袋物品
     *
     * 对应 MC 1.21.11 BundleItem#getByColor。
     * 必须在 Items::initialize() 之后调用。
     *
     * @param color 染料颜色（DyeColor::Count 表示无色）
     * @return 物品指针
     */
    [[nodiscard]] static Item* getByColor(DyeColor color);

private:
    DyeColor m_color;

    /**
     * @brief 丢出单个内容物
     *
     * 对应 MC 1.21.11 BundleItem#dropContent(ItemStack, Player)：
     * 从收纳袋取出一项并让玩家丢弃。
     *
     * @param bundleStack 收纳袋物品堆
     * @param player 玩家
     * @return 是否成功丢出
     */
    static bool dropContent(ItemStack& bundleStack, Player& player);

    // ========== 音效 ==========

    static void playRemoveOneSound(Entity& entity);
    static void playInsertSound(Entity& entity);
    static void playInsertFailSound(Entity& entity);
    static void playDropContentsSound(Entity& entity);

    // ========== NBT 存取辅助 ==========

    /**
     * @brief 从物品堆读取 BundleContents
     */
    [[nodiscard]] static BundleContents getContents(const ItemStack& stack);

    /**
     * @brief 将 BundleContents 写入物品堆 NBT
     */
    static void setContents(ItemStack& stack, const BundleContents& contents);
};

} // namespace item::items
} // namespace mc
