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

namespace mc {
namespace entity {

/**
 * @brief 飞行动物接口 - 用于可以飞行的动物
 *
 * 实现此接口的动物可以在空中飞行。
 * 例如：蜜蜂、鹦鹉等。
 */
class IFlyingAnimal {
public:
    virtual ~IFlyingAnimal() noexcept = default;

    /**
     * @brief 检查是否正在飞行
     * @return 如果正在飞行返回true
     */
    virtual bool isFlying() const = 0;

    /**
     * @brief 设置飞行状态
     * @param flying 是否飞行
     */
    virtual void setFlying(bool flying) = 0;

    /**
     * @brief 获取飞行高度限制
     * @return 最大飞行高度（相对地面）
     */
    virtual f32 getMaxFlightHeight() const { return 32.0f; }

    /**
     * @brief 检查是否可以降落
     * @return 如果可以降落返回true
     */
    virtual bool canLand() const { return true; }

    /**
     * @brief 检查是否可以在当前位置悬停
     * @return 如果可以悬停返回true
     */
    virtual bool canHover() const { return false; }
};

} // namespace entity
} // namespace mc
