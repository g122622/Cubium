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
namespace world {

/**
 * @brief 方块更新标志位（对齐 vanilla net.minecraft.world.level.block.Block）。
 *
 * 这些标志位由 `setBlockState(..., flags)` 的 flags 参数携带，控制一次方块写入
 * 触发哪些副作用。项目历史上 `ServerWorld::setBlockState` 忽略 flags（无条件全做），
 * 导致结构放置等需要静默写入的场景误触发 neighborChanged/updatePostPlacement，
 * 使依附类方块（按钮/拉杆/火把等）在支撑尚未放置时自毁。
 *
 * 位定义（与 vanilla 1.21.11 Block.java 完全一致）：
 * - bit0 (1)  UPDATE_NEIGHBORS       通知 6 向邻居 neighborChanged + updatePostPlacement
 * - bit1 (2)  UPDATE_CLIENTS         向客户端发包同步
 * - bit2 (4)  UPDATE_INVISIBLE       静默（不触发视觉/动画副作用）
 * - bit3 (8)  UPDATE_IMMEDIATE       立即执行（不入队延迟）
 * - bit4 (16) UPDATE_KNOWN_SHAPE     跳过形状更新（已知形状正确）
 * - bit5 (32) UPDATE_SUPPRESS_DROPS  抑制掉落
 * - bit6 (64) UPDATE_MOVE_BY_PISTON  活塞推动
 *
 * 常用组合：
 * - UPDATE_ALL (3)            = NEIGHBORS | CLIENTS，玩家放置/破坏方块的标准更新
 * - UPDATE_NONE (260)         = CLIENTS | IMMEDIATE | KNOWN_SHAPE，无邻居更新
 * - 结构放置默认 flags=18     = CLIENTS | KNOWN_SHAPE，无邻居更新（按钮不自毁）
 */
namespace BlockUpdateFlags {
inline constexpr i32 UPDATE_NEIGHBORS = 1;
inline constexpr i32 UPDATE_CLIENTS = 2;
inline constexpr i32 UPDATE_INVISIBLE = 4;
inline constexpr i32 UPDATE_IMMEDIATE = 8;
inline constexpr i32 UPDATE_KNOWN_SHAPE = 16;
inline constexpr i32 UPDATE_SUPPRESS_DROPS = 32;
inline constexpr i32 UPDATE_MOVE_BY_PISTON = 64;
inline constexpr i32 UPDATE_SKIP_SHAPE_UPDATE_ON_WIRE = 128;
inline constexpr i32 UPDATE_SKIP_BLOCK_ENTITY_SIDEEFFECTS = 256;
inline constexpr i32 UPDATE_SKIP_ON_PLACE = 512;

inline constexpr i32 UPDATE_ALL = 3;
inline constexpr i32 UPDATE_ALL_IMMEDIATE = 11;
inline constexpr i32 UPDATE_NONE = 260;
inline constexpr i32 UPDATE_SKIP_ALL_SIDEEFFECTS = 816;
} // namespace BlockUpdateFlags

} // namespace world
} // namespace mc
