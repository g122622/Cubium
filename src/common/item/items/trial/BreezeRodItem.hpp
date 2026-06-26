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
namespace item {

/**
 * @brief 狂风杖物品
 *
 * 旋风人掉落的材料物品。可以作为酿造材料制作风充药水，
 * 也可以用于合成重锤和涡流盔甲纹饰锻造模板。
 *
 * MC 原版中 BreezeRod 继承自 Item，没有特殊交互行为。
 * 后续酿造系统和合成系统集成时可扩展此类的用途。
 *
 * 获取方式：
 * - 击杀旋风人掉落 1-2 个（仅被玩家击杀时，受抢夺附魔影响）
 *
 * 命名空间ID: minecraft:breeze_rod
 */
class BreezeRodItem final : public Item {
public:
    /**
     * @brief 构造狂风杖物品
     * @param properties 物品属性
     */
    explicit BreezeRodItem(const ItemProperties& properties);
};

} // namespace item
} // namespace mc
