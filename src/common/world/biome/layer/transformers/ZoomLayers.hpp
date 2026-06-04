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

#include "../LayerContext.hpp"
#include <memory>

namespace mc {
namespace layer {

/**
 * @brief 缩放层变换器
 *
 * 将区域放大 2 倍。有两种模式：普通和模糊。
 *
 * 采样模式：
 * - 偶数坐标 (x, z)：直接返回父级值
 * - 边缘坐标：从相邻值中选择
 * - 角落坐标：使用众数算法
 */
class ZoomLayer : public ITransformer1 {
public:
    /**
     * @brief 缩放模式
     */
    enum class Mode {
        Normal, // 普通缩放：使用众数算法
        Fuzzy   // 模糊缩放：随机选择
    };

    explicit ZoomLayer(Mode mode = Mode::Normal);

    using ITransformer1::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& area, i32 x, i32 z) override;

    [[nodiscard]] i32 getOffsetX(i32 x) const override { return x >> 1; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override { return z >> 1; }

    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(
        IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input) override;

private:
    Mode m_mode;

    /**
     * @brief 选择缩放后的值（众数算法）
     */
    [[nodiscard]] i32 _pickZoomed(IAreaContext& ctx, i32 a, i32 b, i32 c, i32 d);
};

} // namespace layer
} // namespace mc
