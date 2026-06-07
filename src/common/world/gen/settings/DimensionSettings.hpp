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

#include "NoiseSettings.hpp"
#include "common/world/block/Block.hpp"

// 前向声明
namespace mc {
class BlockState;
}

namespace mc {

/**
 * @brief 维度类型标识
 *
 * 明确标识维度类型，不依赖 seaLevel/bedrockFloor 反推。
 * 用于选择正确的 NoiseRouter 和 SurfaceRules。
 */
enum class DimensionKind : u8 {
    Overworld, ///< 主世界
    Nether,    ///< 下界
    End,       ///< 末地
    Flat       ///< 超平坦
};

/**
 * @brief 维度生成设置
 *
 * 包含维度级别的生成配置。
 * 使用 BlockState* 替代固定 BlockId，支持动态方块注册。
 */
struct DimensionSettings {
    NoiseSettings noise;
    const BlockState* defaultBlock = nullptr; ///< 默认方块（石头等）
    const BlockState* defaultFluid = nullptr; ///< 默认流体（水/熔岩）
    i32 seaLevel = world::SEA_LEVEL;
    i32 bedrockRoof = -10;                                  ///< 基岩顶部（下界用）
    i32 bedrockFloor = 0;                                   ///< 基岩底部
    DimensionKind dimensionKind = DimensionKind::Overworld; ///< 维度类型标识
    bool largeBiomes = false;                               ///< 是否使用大型生物群系预设

    // === 预设 ===

    /**
     * @brief 主世界设置
     */
    static DimensionSettings overworld() noexcept;

    /**
     * @brief 下界设置
     */
    static DimensionSettings nether() noexcept;

    /**
     * @brief 末地设置
     */
    static DimensionSettings end() noexcept;

    /**
     * @brief 平坦世界设置
     */
    static DimensionSettings flat() noexcept;
};

} // namespace mc
