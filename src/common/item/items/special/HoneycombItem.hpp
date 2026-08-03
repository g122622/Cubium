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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../core/Item.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/world/block/BlockState.hpp"

#include <optional>
#include <unordered_map>

namespace mc {

class Block;

namespace item::items {

/**
 * @brief 蜜脾物品
 *
 * 蜜脾可从蜂巢（满蜂蜜等级时使用剪刀）获取，主要用途：
 * 1. 右键铜方块：给铜方块涂蜡，阻止氧化（已实现）
 * 2. 右键告示牌：给告示牌上蜡，防止文字被修改（已实现）
 *
 * 涂蜡映射表 WAXABLES 将未涂蜡的铜方块映射到对应的涂蜡变种。
 * 反向映射 WAX_OFF_MAP 供 AxeItem 除蜡使用。
 *
 * 参考: net.minecraft.world.item.HoneycombItem
 */
class HoneycombItem : public Item {
public:
    /**
     * @brief 构造蜜脾物品
     * @param properties 物品属性
     */
    explicit HoneycombItem(ItemProperties properties);

    ~HoneycombItem() override = default;

    /**
     * @brief 在方块上使用物品
     *
     * 对未涂蜡的铜方块使用蜜脾，将其转换为涂蜡变种。
     * 涂蜡后方块不再继续氧化，并播放涂蜡音效和粒子效果。
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 获取涂蜡后的方块状态
     *
     * 查找 WAXABLES 映射表，如果给定方块有涂蜡变种，
     * 返回涂蜡方块的默认状态并复制原方块状态中兼容的属性。
     *
     * @param state 原始方块状态
     * @return 涂蜡后方块状态，如果不可涂蜡则为 std::nullopt
     */
    [[nodiscard]] static std::optional<BlockState> getWaxed(const BlockState& state);

    /**
     * @brief 获取除蜡后的方块状态
     *
     * 查找 WAX_OFF_BY_BLOCK 映射表，如果给定方块是涂蜡变种，
     * 返回未涂蜡方块的默认状态并复制原方块状态中兼容的属性。
     *
     * @param state 涂蜡方块状态
     * @return 除蜡后方块状态，如果不是涂蜡方块则为 std::nullopt
     */
    [[nodiscard]] static std::optional<BlockState> getWaxedOff(const BlockState& state);

    /**
     * @brief 获取涂蜡映射表（延迟初始化）
     *
     * 使用"construct on first use"模式，确保静态方法调用前映射表已初始化。
     * 映射表键为未涂蜡铜方块指针，值为对应的涂蜡变种方块指针。
     *
     * @return 涂蜡映射表的引用
     */
    static std::unordered_map<const Block*, const Block*>& getWaxablesMap();

    /**
     * @brief 获取除蜡映射表（延迟初始化）
     *
     * 除蜡映射表是涂蜡映射表的反向映射。
     * 键为涂蜡铜方块指针，值为对应的未涂蜡变种方块指针。
     *
     * @return 除蜡映射表的引用
     */
    static std::unordered_map<const Block*, const Block*>& getWaxOffMap();

private:
    /**
     * @brief 初始化涂蜡映射表
     * @return 涂蜡映射表
     */
    static std::unordered_map<const Block*, const Block*> _buildWaxablesMap();

    /**
     * @brief 初始化除蜡映射表（涂蜡映射表的反向）
     * @return 除蜡映射表
     */
    static std::unordered_map<const Block*, const Block*> _buildWaxOffMap();
};

} // namespace item::items
} // namespace mc
