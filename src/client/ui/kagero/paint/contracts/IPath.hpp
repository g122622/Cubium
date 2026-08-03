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

#include "client/ui/kagero/paint/Geometry.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::kagero::paint {

/**
 * @brief 路径命令类型
 */
enum class PathCommand : u8 {
    MoveTo,  ///< 移动到新起点
    LineTo,  ///< 直线连接
    QuadTo,  ///< 二次贝塞尔曲线
    CubicTo, ///< 三次贝塞尔曲线
    Close    ///< 闭合路径
};

/**
 * @brief 路径控制点
 */
struct PathPoint {
    f32 x = 0.0f; ///< X坐标
    f32 y = 0.0f; ///< Y坐标
};

/**
 * @brief 路径接口
 *
 * 定义2D矢量路径的构建能力，支持直线、二次/三次贝塞尔曲线等基本图元，
 * 以及矩形、圆角矩形、圆形等几何形状的路径构建。
 */
class IPath {
public:
    virtual ~IPath() = default;

    /// 重置路径，清除所有命令
    virtual void reset() = 0;

    /// 移动画笔到指定位置（起点）
    virtual void moveTo(f32 x, f32 y) = 0;

    /// 从当前位置画直线到指定点
    virtual void lineTo(f32 x, f32 y) = 0;

    /// 从当前位置画二次贝塞尔曲线到(x2,y2)，控制点为(x1,y1)
    virtual void quadTo(f32 x1, f32 y1, f32 x2, f32 y2) = 0;

    /// 从当前位置画三次贝塞尔曲线到(x3,y3)，控制点为(x1,y1)和(x2,y2)
    virtual void cubicTo(f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3) = 0;

    /// 闭合当前子路径，连接回起点
    virtual void close() = 0;

    /// 添加矩形路径
    virtual void addRect(const Rect& rect) = 0;

    /// 添加圆角矩形路径
    virtual void addRRect(const RRect& roundRect) = 0;

    /// 添加圆形路径
    virtual void addCircle(f32 cx, f32 cy, f32 radius) = 0;

    /// 判断路径是否为空
    [[nodiscard]] virtual bool isEmpty() const = 0;
};

} // namespace mc::client::ui::kagero::paint
