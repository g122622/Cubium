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
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "item/items/block/WallOrFloorItem.hpp"
#include "util/color/DyeColor.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {

namespace text {
class ITextComponent;
}

namespace item {

class ItemGroup;

/**
 * @brief 旗帜物品
 *
 * 可以放置为站立旗帜或墙壁旗帜的物品。
 * 继承自 WallOrFloorItem，根据放置位置自动选择放置形态。
 * 每种颜色的旗帜是独立的物品实例。
 *
 * 参考: net.minecraft.item.BannerItem
 */
class BannerItem : public WallOrFloorItem {
public:
    /**
     * @brief 构造函数
     * @param floorBlock 站立旗帜方块
     * @param wallBlock 墙壁旗帜方块
     * @param properties 物品属性（最大堆叠数16，装饰标签页）
     */
    BannerItem(const Block& floorBlock, const Block& wallBlock, ItemProperties properties);

    ~BannerItem() override = default;

    /**
     * @brief 获取旗帜底色
     *
     * 颜色从关联的站立旗帜方块获取。
     */
    [[nodiscard]] DyeColor getColor() const;

    /**
     * @brief 添加物品提示信息
     *
     * 在物品提示中显示最多6层图案信息。
     */
    void addInformation(
        const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const override;

    /**
     * @brief 从ItemStack的BlockEntityTag获取图案列表（静态方法）
     * @param stack 旗帜物品
     * @return 图案列表
     */
    [[nodiscard]] static std::vector<std::pair<std::string, i32>> getPatternListFromStack(const ItemStack& stack);
};

} // namespace item
} // namespace mc
