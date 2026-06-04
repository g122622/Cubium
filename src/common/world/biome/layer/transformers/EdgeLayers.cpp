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

#include "EdgeLayers.hpp"

namespace mc {
namespace layer {

// ============================================================================
// 辅助函数
// ============================================================================

namespace {

bool allNeighborsAre(i32 north, i32 east, i32 south, i32 west, i32 biome)
{
    return north == biome && east == biome && south == biome && west == biome;
}

} // anonymous namespace

// ============================================================================
// CoolWarmEdgeLayer 实现
// ============================================================================

i32 CoolWarmEdgeLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    (void)ctx; // 未使用

    // 如果中心不是温暖区域(1)，保持不变
    if (center != BiomeValues::Climate::Warm) {
        return center;
    }

    // 如果温暖区域相邻有凉爽(3)或冰冻(4)区域，变成中等温度(2)
    if (north == BiomeValues::Climate::Cool || north == BiomeValues::Climate::Icy ||
        east == BiomeValues::Climate::Cool || east == BiomeValues::Climate::Icy ||
        south == BiomeValues::Climate::Cool || south == BiomeValues::Climate::Icy ||
        west == BiomeValues::Climate::Cool || west == BiomeValues::Climate::Icy) {
        return BiomeValues::Climate::Medium;
    }

    return center;
}

// ============================================================================
// HeatIceEdgeLayer 实现
// ============================================================================

i32 HeatIceEdgeLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    (void)ctx; // 未使用

    // 如果中心不是冰冻区域(4)，保持不变
    if (center != BiomeValues::Climate::Icy) {
        return center;
    }

    // 如果冰冻区域相邻有温暖(1)或中等(2)区域，变成凉爽(3)
    if (north == BiomeValues::Climate::Warm || north == BiomeValues::Climate::Medium ||
        east == BiomeValues::Climate::Warm || east == BiomeValues::Climate::Medium ||
        south == BiomeValues::Climate::Warm || south == BiomeValues::Climate::Medium ||
        west == BiomeValues::Climate::Warm || west == BiomeValues::Climate::Medium) {
        return BiomeValues::Climate::Cool;
    }

    return center;
}

// ============================================================================
// SpecialEdgeLayer 实现
// ============================================================================

i32 SpecialEdgeLayer::apply(IAreaContext& ctx, i32 value)
{
    // 浅海不变
    if (BiomeValues::isShallowOcean(value)) {
        return value;
    }

    // 非海洋有 1/13 概率添加特殊变体位
    if (ctx.nextInt(13) == 0) {
        // 在 bits 8-11 添加随机特殊值
        i32 special = 1 + ctx.nextInt(15);
        return (value & ~0xF00) | ((special << 8) & 0xF00);
    }

    return value;
}

// ============================================================================
// BiomeEdgeLayer 实现
// ============================================================================

i32 BiomeEdgeLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    (void)ctx; // 未使用

    // 处理山地边缘 (mountain_edge)
    if (BiomeValues::areBiomesSimilar(center, BiomeValues::Mountains)) {
        return center;
    }

    // 处理恶地边缘
    if (center == BiomeValues::WoodedBadlandsPlateau) {
        if (allNeighborsAre(north, east, south, west, BiomeValues::WoodedBadlandsPlateau)) {
            return center;
        }
        return BiomeValues::Badlands;
    }

    if (center == BiomeValues::BadlandsPlateau) {
        if (allNeighborsAre(north, east, south, west, BiomeValues::BadlandsPlateau)) {
            return center;
        }
        return BiomeValues::Badlands;
    }

    // 处理针叶林边缘
    if (center == BiomeValues::GiantTreeTaiga) {
        if (allNeighborsAre(north, east, south, west, BiomeValues::GiantTreeTaiga)) {
            return center;
        }
        return BiomeValues::Taiga;
    }

    // 处理沙漠与雪地相邻
    if (center == BiomeValues::Desert) {
        if (BiomeValues::isSnowy(north) || BiomeValues::isSnowy(east) || BiomeValues::isSnowy(south) ||
            BiomeValues::isSnowy(west)) {
            return BiomeValues::SnowyTaiga;
        }
    }

    // 处理沼泽边缘
    if (center == BiomeValues::Swamp) {
        // 沼泽与沙漠/山脉/雪地相邻变成平原
        if (north == BiomeValues::Desert || east == BiomeValues::Desert || south == BiomeValues::Desert ||
            west == BiomeValues::Desert || BiomeValues::isSnowy(north) || BiomeValues::isSnowy(east) ||
            BiomeValues::isSnowy(south) || BiomeValues::isSnowy(west)) {
            return BiomeValues::Plains;
        }
        // 沼泽与丛林相邻变成丛林边缘
        if (BiomeValues::isJungle(north) || BiomeValues::isJungle(east) || BiomeValues::isJungle(south) ||
            BiomeValues::isJungle(west)) {
            return BiomeValues::JungleEdge;
        }
    }

    return center;
}

} // namespace layer
} // namespace mc
