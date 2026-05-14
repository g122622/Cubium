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

#include "ClimateLayers.hpp"

namespace mc {
namespace layer {

// ============================================================================
// AddIslandLayer 实现
// ============================================================================

i32 AddIslandLayer::apply(IAreaContext& ctx, i32 x, i32 sw, i32 se, i32 ne, i32 nw, i32 center)
{
    // 参考 MC AddIslandLayer.apply:
    // 参数顺序在 MC 中是: x(south), southEast, east(northEast), northEast(northEast被重用), center
    // 实际传入的是: x坐标, south, southEast, east, northEast, center
    // 对应我们的参数: x, sw, se, ne, nw, center

    // 注意：MC 源码中的参数命名有些混乱，实际对应关系：
    // p_202792_6_ (center) = 中心值
    // p_202792_5_ (nw) = 西北 (MC 命名为 northEast)
    // southEast (se) = 东南
    // p_202792_4_ (ne) = 东北 (MC 命名为 east)
    // x (sw) = 西南 (MC 命名为 south)

    // 参考 MC AddIslandLayer:
    // if (!LayerUtil.isShallowOcean(center) || isShallowOcean(nw) && isShallowOcean(ne) && isShallowOcean(sw) &&
    // isShallowOcean(se)) {
    //     // 陆地或周围都是海洋，保持不变或向海洋扩展
    //     ...
    // } else {
    //     // 海洋周围有陆地，有机会变成陆地
    //     ...
    // }

    // 如果中心不是浅海，或者四角都是浅海
    if (!BiomeValues::isShallowOcean(center) ||
        (BiomeValues::isShallowOcean(nw) && BiomeValues::isShallowOcean(ne) && BiomeValues::isShallowOcean(sw) &&
            BiomeValues::isShallowOcean(se))) {

        // 如果中心不是浅海且周围有浅海，有概率向海洋扩展
        if (!BiomeValues::isShallowOcean(center)) {
            bool hasShallowOceanNeighbor = BiomeValues::isShallowOcean(nw) || BiomeValues::isShallowOcean(ne) ||
                BiomeValues::isShallowOcean(sw) || BiomeValues::isShallowOcean(se);

            if (hasShallowOceanNeighbor && ctx.nextInt(5) == 0) {
                // 选择一个陆地方向
                if (BiomeValues::isShallowOcean(nw)) {
                    return (center == BiomeValues::Climate::Icy) ? BiomeValues::Climate::Icy : nw;
                }
                if (BiomeValues::isShallowOcean(ne)) {
                    return (center == BiomeValues::Climate::Icy) ? BiomeValues::Climate::Icy : ne;
                }
                if (BiomeValues::isShallowOcean(sw)) {
                    return (center == BiomeValues::Climate::Icy) ? BiomeValues::Climate::Icy : sw;
                }
                if (BiomeValues::isShallowOcean(se)) {
                    return (center == BiomeValues::Climate::Icy) ? BiomeValues::Climate::Icy : se;
                }
            }
        }
        return center;
    }

    // 中心是浅海，周围有陆地
    // 随机选择一个陆地方向
    i32 candidates = 0;
    i32 selectedLand = center;

    if (!BiomeValues::isShallowOcean(nw) && ctx.nextInt(++candidates) == 0) {
        selectedLand = nw;
    }
    if (!BiomeValues::isShallowOcean(ne) && ctx.nextInt(++candidates) == 0) {
        selectedLand = ne;
    }
    if (!BiomeValues::isShallowOcean(sw) && ctx.nextInt(++candidates) == 0) {
        selectedLand = sw;
    }
    if (!BiomeValues::isShallowOcean(se) && ctx.nextInt(++candidates) == 0) {
        selectedLand = se;
    }

    // 有 1/3 概率保持海洋
    if (ctx.nextInt(3) == 0) {
        return selectedLand;
    }

    // 否则保持冰冻状态或返回选择
    return (selectedLand == BiomeValues::Climate::Icy) ? BiomeValues::Climate::Icy : center;
}

// ============================================================================
// AddSnowLayer 实现
// ============================================================================

i32 AddSnowLayer::apply(IAreaContext& ctx, i32 value)
{
    // 参考 MC AddSnowLayer.apply:
    // if (LayerUtil.isShallowOcean(value)) {
    //     return value;
    // } else {
    //     int i = context.random(6);
    //     if (i == 0) {
    //         return 4;  // Icy
    //     } else {
    //         return i == 1 ? 3 : 1;  // Cool or Warm
    //     }
    // }

    // 浅海保持不变
    if (BiomeValues::isShallowOcean(value)) {
        return value;
    }

    // 陆地：分配温度区域
    i32 rnd = ctx.nextInt(6);
    if (rnd == 0) {
        return BiomeValues::Climate::Icy; // 冰冻区域 (4)
    } else if (rnd == 1) {
        return BiomeValues::Climate::Cool; // 凉爽区域 (3)
    } else {
        return BiomeValues::Climate::Warm; // 温暖区域 (1)
        // 注意：MC 默认返回 1（温暖），实际上中等温度 (2) 由后续层处理
    }
}

// ============================================================================
// RemoveTooMuchOceanLayer 实现
// ============================================================================

i32 RemoveTooMuchOceanLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    // 参考 MC RemoveTooMuchOceanLayer.apply:
    // return LayerUtil.isShallowOcean(center) && LayerUtil.isShallowOcean(north) &&
    //        LayerUtil.isShallowOcean(west) && LayerUtil.isShallowOcean(east) &&
    //        LayerUtil.isShallowOcean(south) && context.random(2) == 0 ? 1 : center;

    // 如果中心和四个方向都是浅海，有 50% 概率变成陆地
    if (BiomeValues::isShallowOcean(center) && BiomeValues::isShallowOcean(north) &&
        BiomeValues::isShallowOcean(east) && BiomeValues::isShallowOcean(south) && BiomeValues::isShallowOcean(west)) {
        return ctx.nextInt(2) == 0 ? 1 : center;
    }

    return center;
}

// ============================================================================
// DeepOceanLayer 实现
// ============================================================================

i32 DeepOceanLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    // 参考 MC DeepOceanLayer.apply:
    // if (LayerUtil.isShallowOcean(center)) {
    //     int i = 0;
    //     if (LayerUtil.isShallowOcean(north)) ++i;
    //     if (LayerUtil.isShallowOcean(west)) ++i;
    //     if (LayerUtil.isShallowOcean(east)) ++i;
    //     if (LayerUtil.isShallowOcean(south)) ++i;
    //     if (i > 3) {
    //         if (center == 44) return 47;  // warm -> deep_warm
    //         if (center == 45) return 48;  // lukewarm -> deep_lukewarm
    //         if (center == 0) return 24;   // ocean -> deep_ocean
    //         if (center == 46) return 49;  // cold -> deep_cold
    //         if (center == 10) return 50;  // frozen -> deep_frozen
    //         return 24;  // 默认深海
    //     }
    // }
    // return center;

    if (BiomeValues::isShallowOcean(center)) {
        i32 count = 0;
        if (BiomeValues::isShallowOcean(north)) count++;
        if (BiomeValues::isShallowOcean(east)) count++;
        if (BiomeValues::isShallowOcean(south)) count++;
        if (BiomeValues::isShallowOcean(west)) count++;

        // 如果四个方向中有超过 3 个是浅海，变成深海
        if (count > 3) {
            switch (center) {
                case BiomeValues::WarmOcean:
                    return BiomeValues::DeepWarmOcean; // 44 -> 47
                case BiomeValues::LukewarmOcean:
                    return BiomeValues::DeepLukewarmOcean; // 45 -> 48
                case BiomeValues::Ocean:
                    return BiomeValues::DeepOcean; // 0 -> 24
                case BiomeValues::ColdOcean:
                    return BiomeValues::DeepColdOcean; // 46 -> 49
                case BiomeValues::FrozenOcean:
                    return BiomeValues::DeepFrozenOcean; // 10 -> 50
                default:
                    return BiomeValues::DeepOcean; // 默认深海
            }
        }
    }

    return center;
}

} // namespace layer
} // namespace mc
