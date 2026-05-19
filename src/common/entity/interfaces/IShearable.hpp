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
#include <vector>

namespace mc {

// 前向声明
class Player;
class ItemStack;

namespace entity {

/**
 * @brief 可剪毛接口 - 用于可以被剪刀剪毛的实体
 *
 * 实现此接口的实体可以使用剪刀进行剪毛操作。
 * 例如：羊、雪傀儡、哞菇等。
 *
 * 参考 MC 1.16.5 IShearable
 */
class IShearable {
public:
    virtual ~IShearable() = default;

    /**
     * @brief 检查当前是否可以被剪毛
     * @return 如果可以被剪毛返回true
     *
     * 注意：羊需要先长出羊毛才能剪，雪傀儡需要有南瓜头才能剪
     */
    virtual bool isShearable() const = 0;

    /**
     * @brief 执行剪毛操作
     * @param player 执行剪毛的玩家（可能为nullptr）
     * @return 剪下的物品列表
     *
     * 此方法会修改实体状态（如羊失去羊毛）
     */
    virtual std::vector<ItemStack> shear(Player* player = nullptr) = 0;

    /**
     * @brief 获取剪毛后的冷却时间（ticks）
     * @return 冷却时间，0表示无冷却
     */
    virtual i32 getShearCooldown() const { return 0; }
};

} // namespace entity
} // namespace mc
