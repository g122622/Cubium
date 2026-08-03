/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/sound/SoundEvents.hpp"

namespace mc {

class Player;
class IWorld;

namespace item {

/**
 * @brief 细雪桶物品
 *
 * 细雪桶是一种特殊的桶类物品，继承自 Item（非 BucketItem），
 * 因为细雪不是流体，而是固体方块。
 *
 * 功能：
 * - 右键方块放置细雪方块（PowderSnowBlock）
 * - 水下使用时返回 Consume（不允许水下放置，对应 MC Java 涌现行为）
 * - 放置后替换为空桶（非创造模式）
 * - 支持发射器调用 emptyContents() 放置细雪
 * - 与炼药锅的交互由炼药锅自身的 onBlockActivated 处理
 *
 * 与 BucketItem 的区别：
 * - BucketItem 处理流体（水、岩浆），通过 Fluid 系统放置
 * - PowderSnowBucketItem 放置固体方块，不走流体路径
 * - 细雪桶不实现流体相关接口（ILiquidContainer 等）
 */
class PowderSnowBucketItem : public Item {
public:
    /**
     * @brief 构造细雪桶
     * @param properties 物品属性（应设置 maxStackSize(1).containerItem(BUCKET)）
     */
    explicit PowderSnowBucketItem(const ItemProperties& properties);

    ~PowderSnowBucketItem() noexcept override = default;

    // 禁止拷贝和移动（Item 基类不可拷贝）
    PowderSnowBucketItem(const PowderSnowBucketItem&) = delete;
    PowderSnowBucketItem(PowderSnowBucketItem&&) noexcept = delete;
    PowderSnowBucketItem& operator=(const PowderSnowBucketItem&) = delete;
    PowderSnowBucketItem& operator=(PowderSnowBucketItem&&) noexcept = delete;

    /**
     * @brief 方块交互 - 放置细雪方块
     *
     * 在目标位置放置 PowderSnowBlock，成功后替换为空桶。
     * 仅在服务端执行实际逻辑，客户端返回 Success 预测。
     *
     * @param context 物品使用上下文
     * @return 交互结果
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 倒空容器 - 放置细雪方块（发射器/代码调用）
     *
     * 检查目标位置是否为空方块，如果是则放置细雪。
     *
     * @param player 使用者（可为 nullptr，如发射器调用）
     * @param world 世界引用
     * @param pos 目标位置
     * @return 是否成功放置
     */
    [[nodiscard]] bool emptyContents(Player* player, IWorld& world, const BlockPos& pos) const;

private:
    /**
     * @brief 返回空桶给玩家
     *
     * 如果当前物品堆已空，直接替换为空桶；
     * 否则尝试添加到背包，背包满则掉落。
     *
     * @param player 玩家
     * @param stack 当前物品堆（可能被修改）
     */
    void _returnEmptyBucket(Player& player, ItemStack& stack) const;
};

} // namespace item
} // namespace mc
