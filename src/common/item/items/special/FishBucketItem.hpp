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
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"

#include <memory>
#include <string>

namespace mc {
namespace entity {
class EntityType;
}

namespace item {

/**
 * @brief 鱼桶物品
 *
 * 右键使用时放置水并生成鱼实体。
 */
class FishBucketItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param fishTypeName 鱼实体类型名称（如 "minecraft:cod"）
     * @param properties 物品属性
     */
    FishBucketItem(const char* fishTypeName, const ItemProperties& properties);

    ~FishBucketItem() noexcept override = default;

    FishBucketItem(const FishBucketItem&) = default;
    FishBucketItem(FishBucketItem&&) noexcept = default;
    FishBucketItem& operator=(const FishBucketItem&) = default;
    FishBucketItem& operator=(FishBucketItem&&) noexcept = default;

    /**
     * @brief 获取鱼类型名称
     */
    [[nodiscard]] const std::string& getFishTypeName() const { return m_fishTypeName; }

    /**
     * @brief 方块交互 - 放置水并生成鱼
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 右键使用 - 在水中生成鱼
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

private:
    /**
     * @brief 在指定位置生成鱼
     * @param world 世界
     * @param pos 位置
     * @return 是否成功生成
     */
    bool _spawnFish(IWorld& world, const BlockPos& pos) const;

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

    std::string m_fishTypeName;
};

} // namespace item
} // namespace mc
