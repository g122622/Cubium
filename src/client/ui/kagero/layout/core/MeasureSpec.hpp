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

#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <limits>

namespace mc::client::ui::kagero::layout {

/**
 * @brief 测量模式
 *
 * 定义父容器给子元素的尺寸约束类型。
 * 参考 Android View.MeasureSpec 设计。
 *
 * 使用场景：
 * - Unspecified: 父容器对子元素尺寸无限制，子元素可以取任意大小
 * - Exactly: 父容器指定了精确尺寸，子元素必须使用该尺寸
 * - AtMost: 父容器指定了最大尺寸，子元素不能超过该值
 */
enum class MeasureMode : u8 {
    Unspecified = 0, ///< 无约束，子元素想要多大就多大
    Exactly = 1,     ///< 精确值，子元素必须用这个尺寸
    AtMost = 2       ///< 最大值，子元素不能超过这个尺寸
};

/**
 * @brief 测量规格
 *
 * 封装尺寸和测量模式，用于布局测量阶段。
 * 类似 Android 的 MeasureSpec，将模式和尺寸打包在一起。
 *
 * 使用示例：
 * @code
 * // 创建精确尺寸规格
 * MeasureSpec exact = MeasureSpec::MakeExactly(100);
 *
 * // 创建最大值规格
 * MeasureSpec atMost = MeasureSpec::MakeAtMost(200);
 *
 * // 创建无限制规格
 * MeasureSpec unspecified = MeasureSpec::MakeUnspecified();
 *
 * // 根据规格测量子元素
 * Size childSize = child->measure(widthSpec, heightSpec);
 * @endcode
 */
struct MeasureSpec {
    i32 size = 0;                                ///< 尺寸值（像素）
    MeasureMode mode = MeasureMode::Unspecified; ///< 测量模式

    MeasureSpec() = default;
    MeasureSpec(i32 s, MeasureMode m)
        : size(s)
        , mode(m)
    {}

    /**
     * @brief 创建精确尺寸规格
     * @param size 精确尺寸（必须 >= 0）
     * @return 测量规格
     */
    [[nodiscard]] static MeasureSpec MakeExactly(i32 size)
    {
        return MeasureSpec(std::max(0, size), MeasureMode::Exactly);
    }

    /**
     * @brief 创建最大值规格
     * @param maxSize 最大尺寸（必须 >= 0）
     * @return 测量规格
     */
    [[nodiscard]] static MeasureSpec MakeAtMost(i32 maxSize)
    {
        return MeasureSpec(std::max(0, maxSize), MeasureMode::AtMost);
    }

    /**
     * @brief 创建无限制规格
     * @return 测量规格
     */
    [[nodiscard]] static MeasureSpec MakeUnspecified() { return MeasureSpec(0, MeasureMode::Unspecified); }

    /**
     * @brief 检查是否是精确模式
     */
    [[nodiscard]] bool isExactly() const { return mode == MeasureMode::Exactly; }

    /**
     * @brief 检查是否是最大值模式
     */
    [[nodiscard]] bool isAtMost() const { return mode == MeasureMode::AtMost; }

    /**
     * @brief 检查是否是无限制模式
     */
    [[nodiscard]] bool isUnspecified() const { return mode == MeasureMode::Unspecified; }

    /**
     * @brief 根据测量结果解析最终尺寸
     *
     * 当子元素测量出自己的期望尺寸后，使用此方法得到最终尺寸。
     *
     * @param measuredSize 子元素测量的期望尺寸
     * @return 最终尺寸
     *
     * 规则：
     * - Exactly: 返回规格尺寸（忽略测量结果）
     * - AtMost: 返回 min(规格尺寸, 测量尺寸)
     * - Unspecified: 返回测量尺寸
     */
    [[nodiscard]] i32 resolve(i32 measuredSize) const
    {
        switch (mode) {
            case MeasureMode::Exactly:
                return size;
            case MeasureMode::AtMost:
                return std::min(size, measuredSize);
            case MeasureMode::Unspecified:
            default:
                return measuredSize;
        }
    }

    /**
     * @brief 调整尺寸以符合规格约束
     *
     * 与 resolve 类似，但用于已知尺寸的情况。
     *
     * @param desiredSize 期望尺寸
     * @return 符合规格的尺寸
     */
    [[nodiscard]] i32 adjust(i32 desiredSize) const
    {
        switch (mode) {
            case MeasureMode::Exactly:
                return size;
            case MeasureMode::AtMost:
                return std::min(size, std::max(0, desiredSize));
            case MeasureMode::Unspecified:
            default:
                return std::max(0, desiredSize);
        }
    }

    /**
     * @brief 比较两个测量规格是否相等
     */
    bool operator==(const MeasureSpec& other) const { return size == other.size && mode == other.mode; }

    bool operator!=(const MeasureSpec& other) const { return !(*this == other); }
};

/**
 * @brief 尺寸结构
 *
 * 用于表示宽度和高度。
 */
struct Size {
    i32 width = 0;
    i32 height = 0;

    Size() = default;
    Size(i32 w, i32 h)
        : width(w)
        , height(h)
    {}

    /**
     * @brief 检查尺寸是否有效（非负）
     */
    [[nodiscard]] bool isValid() const { return width >= 0 && height >= 0; }

    /**
     * @brief 创建无限尺寸
     */
    [[nodiscard]] static Size unlimited()
    {
        return Size(std::numeric_limits<i32>::max(), std::numeric_limits<i32>::max());
    }

    bool operator==(const Size& other) const { return width == other.width && height == other.height; }

    bool operator!=(const Size& other) const { return !(*this == other); }
};

} // namespace mc::client::ui::kagero::layout
