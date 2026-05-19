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

#include "../Layer.hpp"

// 前向声明
namespace mc {
class LayerContext;
class TransformFactory;
class MergeFactory;
} // namespace mc

namespace mc {
namespace layer {

// ============================================================================
// 邻域采样模式特征类
//
// 这些类提供了不同的邻域采样模式，参考 MC 1.16.5 的实现：
// - IC0Transformer: 无偏移，采样单个点 (x, z)
// - IC1Transformer: 偏移 +1，采样单个点 (x+1, z+1)
// - ICastleTransformer: 四方向采样 (N/E/S/W + 中心)
// - IBishopTransformer: 四对角采样 (SW/SE/NE/NW + 中心)
// ============================================================================

/**
 * @brief 邻域采样模式 - 无偏移
 *
 * 采样单个点 (x, z)，无偏移。
 * 参考 MC IC0Transformer
 *
 * 用法示例：
 * @code
 * class MyTransformer : public IC0Transformer {
 * public:
 *     i32 apply(IAreaContext& ctx, i32 value) override {
 *         // value 是 (x, z) 处的值
 *         return transform(value);
 *     }
 * };
 * @endcode
 */
class IC0Transformer : public ITransformer1 {
public:
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& area, i32 x, i32 z) override
    {
        return apply(ctx, area.getValue(x, z));
    }

    /**
     * @brief 处理单个值
     * @param ctx 区域上下文
     * @param value 采样值
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 apply(IAreaContext& ctx, i32 value) = 0;

    [[nodiscard]] i32 getOffsetX(i32 x) const override final { return x; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override final { return z; }

    // ITransformer1 工厂方法
    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(
        IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input) override;
};

/**
 * @brief 邻域采样模式 - 中心点偏移+1
 *
 * 采样点 (x+1, z+1)，用于需要周围上下文的变换器。
 * 参考 MC IC1Transformer
 *
 * 用法示例：
 * @code
 * class AddSnowLayer : public IC1Transformer {
 * public:
 *     i32 apply(IAreaContext& ctx, i32 value) override {
 *         // value 是 (x+1, z+1) 处的值
 *         if (isOcean(value)) return value;
 *         int rnd = ctx.nextInt(6);
 *         return (rnd == 0) ? ICY : (rnd == 1) ? COOL : WARM;
 *     }
 * };
 * @endcode
 */
class IC1Transformer : public ITransformer1 {
public:
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& area, i32 x, i32 z) override
    {
        return apply(ctx, area.getValue(x + 1, z + 1));
    }

    /**
     * @brief 处理单个值
     * @param ctx 区域上下文
     * @param value 采样值
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 apply(IAreaContext& ctx, i32 value) = 0;

    [[nodiscard]] i32 getOffsetX(i32 x) const override final { return x + 1; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override final { return z + 1; }

    // ITransformer1 工厂方法
    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(
        IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input) override;
};

/**
 * @brief 邻域采样模式 - 四方向（N/E/S/W + 中心）
 *
 * 采样五个点：北、东、南、西和中心。
 * 用于边缘检测和平滑变换。
 * 参考 MC ICastleTransformer
 *
 * 坐标映射（MC 坐标系，Z轴向上为北）：
 * - north: (x+1, z)   - 北
 * - east:  (x+2, z+1) - 东
 * - south: (x+1, z+2) - 南
 * - west:  (x, z+1)   - 西
 * - center:(x+1, z+1) - 中心
 *
 * 用法示例：
 * @code
 * class SmoothLayer : public ICastleTransformer {
 * public:
 *     i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override {
 *         bool ewEqual = (east == west);
 *         bool nsEqual = (north == south);
 *         if (ewEqual == nsEqual) {
 *             return ewEqual ? ctx.pickRandom(east, north) : center;
 *         }
 *         return ewEqual ? east : north;
 *     }
 * };
 * @endcode
 */
class ICastleTransformer : public ITransformer1 {
public:
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& area, i32 x, i32 z) override
    {
        return apply(ctx,
            area.getValue(x + 1, z),     // north: (x+1, z)
            area.getValue(x + 2, z + 1), // east:  (x+2, z+1)
            area.getValue(x + 1, z + 2), // south: (x+1, z+2)
            area.getValue(x, z + 1),     // west:  (x, z+1)
            area.getValue(x + 1, z + 1)  // center:(x+1, z+1)
        );
    }

    /**
     * @brief 处理五邻域值
     * @param ctx 区域上下文
     * @param north 北值
     * @param east 东值
     * @param south 南值
     * @param west 西值
     * @param center 中心值
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) = 0;

    [[nodiscard]] i32 getOffsetX(i32 x) const override final { return x + 1; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override final { return z + 1; }

    // ITransformer1 工厂方法
    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(
        IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input) override;
};

/**
 * @brief 邻域采样模式 - 四对角（SW/SE/NE/NW + 中心）
 *
 * 采样五个点：西南、东南、东北、西北和中心。
 * 用于岛屿扩展等操作。
 * 参考 MC IBishopTransformer
 *
 * 坐标映射（视觉坐标系，假设 X 向右，Z 向下）：
 * - nw: (x, z)         - 西北（左上）
 * - ne: (x+2, z)       - 东北（右上）
 * - sw: (x, z+2)       - 西南（左下）
 * - se: (x+2, z+2)     - 东南（右下）
 * - center: (x+1, z+1) - 中心
 *
 * 注意：MC 源码中的参数命名为 south, southEast, east, northEast，这是基于 MC 的坐标约定
 *
 * 用法示例：
 * @code
 * class AddIslandLayer : public IBishopTransformer {
 * public:
 *     i32 apply(IAreaContext& ctx, i32 x, i32 sw, i32 se, i32 ne, i32 nw, i32 center) override {
 *         // center 是中心点，sw/se/ne/nw 是四个对角
 *         if (isOcean(center) && !allOcean(sw, se, ne, nw)) {
 *             return pickLand(ctx, sw, se, ne, nw);
 *         }
 *         return center;
 *     }
 * };
 * @endcode
 */
class IBishopTransformer : public ITransformer1 {
public:
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& area, i32 x, i32 z) override
    {
        return apply(ctx,
            x,
            area.getValue(x, z + 2),     // south (MC name, 实际是左下 sw)
            area.getValue(x + 2, z + 2), // southEast (右下 se)
            area.getValue(x + 2, z),     // east (MC name, 实际是右上 ne)
            area.getValue(x, z),         // northEast (MC name, 实际是左上 nw)
            area.getValue(x + 1, z + 1)  // center
        );
    }

    /**
     * @brief 处理五对角值
     * @param ctx 区域上下文
     * @param x X 坐标（用于随机）
     * @param sw 西南值
     * @param se 东南值
     * @param ne 东北值
     * @param nw 西北值
     * @param center 中心值
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 apply(IAreaContext& ctx, i32 x, i32 sw, i32 se, i32 ne, i32 nw, i32 center) = 0;

    [[nodiscard]] i32 getOffsetX(i32 x) const override final { return x + 1; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override final { return z + 1; }

    // ITransformer1 工厂方法
    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(
        IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input) override;
};

} // namespace layer
} // namespace mc
