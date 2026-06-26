/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

/**
 * @file BedrockConstants.hpp
 * @brief 基岩版存储格式相关常量
 *
 * 定义基岩版 LevelDB 存档格式中使用的子区块索引范围、调色板参数、
 * Data2D 格式常量和 level.dat 文件头大小等常量。
 *
 * 注意：这些常量是基岩版格式特有的，与 WorldConstants.hpp 中定义的
 * 引擎世界模型常量（Java 版高度范围等）不同。
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc::world::storage::reader::bedrock {

// ============================================================================
// 子区块 Y 坐标范围（Bedrock LevelDB 格式）
// ============================================================================

/// 基岩版子区块 Y 坐标下界（对应世界 Y = -1024）
constexpr i8 BEDROCK_MIN_SUB_CHUNK_Y = -64;

/// 基岩版子区块 Y 坐标上界（不含），对应世界 Y = 1024
constexpr i8 BEDROCK_MAX_SUB_CHUNK_Y = 64;

// ============================================================================
// 子区块方块数量
// ============================================================================

/// 每个子区块的方块数量（16 x 16 x 16 = 4096）
constexpr i32 BLOCKS_PER_SUB_CHUNK = 4096;

// ============================================================================
// 调色板参数
// ============================================================================

/// 基岩版调色板位打包使用的字位宽（int[]，32 位；Java 版为 long[]，64 位）
constexpr i32 BEDROCK_PALETTE_WORD_BITS = 32;

// ============================================================================
// Data2D 格式常量
// ============================================================================

/// Data2D 高度图字节数（16 x 16 = 256）
constexpr i32 BEDROCK_DATA2D_HEIGHTMAP_SIZE = 256;

/// Data2D 最小字节数（高度图 256 + 生物群系 256 = 512）
constexpr i32 BEDROCK_DATA2D_MIN_SIZE = BEDROCK_DATA2D_HEIGHTMAP_SIZE * 2;

// ============================================================================
// level.dat 文件头
// ============================================================================

/// 基岩版 level.dat 文件头字节数
constexpr i32 BEDROCK_LEVEL_DAT_HEADER_SIZE = 8;

} // namespace mc::world::storage::reader::bedrock
