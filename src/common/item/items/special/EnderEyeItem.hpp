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

#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include <memory>

namespace mc {

class ItemUseContext;

namespace blockpattern {
class BlockPattern;
}

namespace item::items {

/**
 * @brief 末影之眼物品
 *
 * 用于激活末地传送门。对末地传送门框架（EYE=false）右键使用时：
 * 1. 将末影之眼放入框架（EYE: false → true）
 * 2. 检测是否构成完整传送门图案（12 个带眼框架，朝向正确）
 * 3. 若完整，在框架内部 3×3 区域生成末地传送门方块（end_portal）
 *
 * 参考: net.minecraft.world.item.EnderEyeItem#useOn
 * 参考: net.minecraft.world.level.block.EndPortalFrameBlock#getOrCreatePortalShape
 */
class EnderEyeItem : public Item {
public:
    explicit EnderEyeItem(ItemProperties properties);
    ~EnderEyeItem() override = default;

    /**
     * @brief 在方块上使用末影之眼
     *
     * 仅当点击的是末地传送门框架且 EYE=false 时生效：
     * - 设 EYE=true，pushEntitiesUp（框架升高 13/16→1.0，把上方实体顶起）
     * - 消耗 1 个末影之眼
     * - 播放末影之眼放入框架的音效/粒子事件（levelEvent 1503）
     * - 用 BlockPattern 检测完整传送门图案，若匹配则在内部 3×3 区域生成 end_portal 方块
     *
     * @param context 物品使用上下文
     * @return 动作结果类型（Success 表示成功放入并消耗物品）
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

private:
    /**
     * @brief 获取或创建末地传送门形状检测器（懒加载单例）
     *
     * 对应 vanilla EndPortalFrameBlock.getOrCreatePortalShape()。
     * 图案为 5×5 单层：
     *   ? v v v ?
     *   > ? ? ? <
     *   > ? ? ? <
     *   > ? ? ? <
     *   ? ^ ^ ^ ?
     * 其中 v=框架(FACING=NORTH,HAS_EYE=true)，^=框架(FACING=SOUTH,HAS_EYE=true)，
     *   >=框架(FACING=WEST,HAS_EYE=true)，<=框架(FACING=EAST,HAS_EYE=true)，
     *   ?=任意方块（角落），内部 ? 为空气或可替换方块。
     *
     * @return BlockPattern 引用
     */
    static blockpattern::BlockPattern& getOrCreatePortalShape();

    /// 末地传送门形状检测器（懒加载）
    static std::unique_ptr<blockpattern::BlockPattern> s_portalShape;
};

} // namespace item::items
} // namespace mc
