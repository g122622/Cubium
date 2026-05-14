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

#include "ZoomLayers.hpp"
#include <memory>

namespace mc {
namespace layer {

// ============================================================================
// ZoomLayer 实现
// ============================================================================

ZoomLayer::ZoomLayer(Mode mode)
    : m_mode(mode)
{}

i32 ZoomLayer::apply(IAreaContext& ctx, const IArea& area, i32 x, i32 z)
{
    // 参考 MC ZoomLayer.apply:
    // int i = area.getValue(this.getOffsetX(x), this.getOffsetZ(z));
    // context.setPosition((long)(x >> 1 << 1), (long)(z >> 1 << 1));
    // int j = x & 1;
    // int k = z & 1;
    //
    // if (j == 0 && k == 0) {
    //     return i;
    // } else {
    //     int l = area.getValue(this.getOffsetX(x), this.getOffsetZ(z + 1));
    //     int i1 = context.pickRandom(i, l);
    //     if (j == 0 && k == 1) {
    //         return i1;
    //     } else {
    //         int j1 = area.getValue(this.getOffsetX(x + 1), this.getOffsetZ(z));
    //         int k1 = context.pickRandom(i, j1);
    //         if (j == 1 && k == 0) {
    //             return k1;
    //         } else {
    //             int l1 = area.getValue(this.getOffsetX(x + 1), this.getOffsetZ(z + 1));
    //             return this.pickZoomed(context, i, j1, l, l1);
    //         }
    //     }
    // }

    // 获取基础坐标
    i32 baseX = getOffsetX(x);
    i32 baseZ = getOffsetZ(z);

    // 获取四个角落的值
    i32 v00 = area.getValue(baseX, baseZ);         // 左上
    i32 v10 = area.getValue(baseX + 1, baseZ);     // 右上
    i32 v01 = area.getValue(baseX, baseZ + 1);     // 左下
    i32 v11 = area.getValue(baseX + 1, baseZ + 1); // 右下

    // 设置位置种子（用于模糊模式的随机）
    ctx.setPosition(static_cast<i64>(x >> 1 << 1), static_cast<i64>(z >> 1 << 1));

    // 计算局部坐标
    i32 localX = x & 1;
    i32 localZ = z & 1;

    if (m_mode == Mode::Fuzzy) {
        // 模糊模式：随机选择四个值之一
        return ctx.pickRandom(v00, v10, v01, v11);
    }

    // 普通模式
    if (localX == 0 && localZ == 0) {
        // 偶数坐标：直接返回左上
        return v00;
    } else if (localX == 0) {
        // 左边缘：从左上和左下中选择
        return ctx.pickRandom(v00, v01);
    } else if (localZ == 0) {
        // 上边缘：从左上和右上中选择
        return ctx.pickRandom(v00, v10);
    } else {
        // 角落：使用众数算法
        return pickZoomed(ctx, v00, v10, v01, v11);
    }
}

i32 ZoomLayer::pickZoomed(IAreaContext& ctx, i32 a, i32 b, i32 c, i32 d)
{
    // 参考 MC ZoomLayer.pickZoomed:
    // if (second == third && third == fourth) {
    //     return second;
    // } else if (first == second && first == third) {
    //     return first;
    // } else if (first == second && first == fourth) {
    //     return first;
    // } else if (first == third && first == fourth) {
    //     return first;
    // } else if (first == second && third != fourth) {
    //     return first;
    // } else if (first == third && second != fourth) {
    //     return first;
    // } else if (first == fourth && second != third) {
    //     return first;
    // } else if (second == third && first != fourth) {
    //     return second;
    // } else if (second == fourth && first != third) {
    //     return second;
    // } else {
    //     return third == fourth && first != second ? third : context.pickRandom(first, second, third, fourth);
    // }

    // a = 左上, b = 右上, c = 左下, d = 右下

    // 检查三值相同
    if (b == c && c == d) return b;
    if (a == b && a == c) return a;
    if (a == b && a == d) return a;
    if (a == c && a == d) return a;

    // 检查两值相同且另外两个不同
    if (a == b && c != d) return a;
    if (a == c && b != d) return a;
    if (a == d && b != c) return a;
    if (b == c && a != d) return b;
    if (b == d && a != c) return b;

    // 检查下边两值相同
    if (c == d && a != b) return c;

    // 全部不同，随机选择
    return ctx.pickRandom(a, b, c, d);
}

std::unique_ptr<IAreaFactory> ZoomLayer::apply(IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<TransformFactory>(this, sharedContext, std::move(input));
}

} // namespace layer
} // namespace mc
