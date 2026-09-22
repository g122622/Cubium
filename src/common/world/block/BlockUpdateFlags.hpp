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
 * @brief 方块更新标志位，控制一次方块写入触发哪些副作用。
 *
 * 这些标志位由 `setBlockState(..., flags)` 的 flags 参数携带。每个标志位对应
 * 一类副作用，置位表示"启用/抑制"该副作用，未置位表示不启用。
 *
 * 位定义（数值与原版 Block 的 UPDATE_* 常量一致，便于调用点对照）：
 * - bit0 (1)   UPDATE_NEIGHBORS                  通知 6 向邻居 neighborChanged 并更新比较器输出
 * - bit1 (2)   UPDATE_CLIENTS                    向客户端发包同步
 * - bit2 (4)   UPDATE_INVISIBLE                  客户端本地写入时无需回发同步包（仅客户端侧有意义）
 * - bit3 (8)   UPDATE_IMMEDIATE                  保留位，当前版本无任何消费点，置位与不置位等价
 * - bit4 (16)  UPDATE_KNOWN_SHAPE                已知形状正确，跳过形状更新递归
 * - bit5 (32)  UPDATE_SUPPRESS_DROPS             抑制本次写入导致的掉落物
 * - bit6 (64)  UPDATE_MOVE_BY_PISTON             本次写入源于活塞推动，作为 movedByPiston 语义下传
 * - bit7 (128) UPDATE_SKIP_SHAPE_UPDATE_ON_WIRE  该位置的方块为红石线时跳过其形状更新
 * - bit8 (256) UPDATE_SKIP_BLOCK_ENTITY_SIDEEFFECTS 跳过方块实体移除副作用（内容物掉落等）
 * - bit9 (512) UPDATE_SKIP_ON_PLACE              跳过本次写入后的 onPlace 回调（对应 setBlockState 的 onBlockAdded）
 *
 * 常用组合（数值与原版一致）：
 * - UPDATE_ALL (3)                     = NEIGHBORS | CLIENTS，玩家放置/破坏方块的标准更新
 * - UPDATE_ALL_IMMEDIATE (11)          = ALL | IMMEDIATE
 * - UPDATE_NONE (260)                  = INVISIBLE | SKIP_BLOCK_ENTITY_SIDEEFFECTS
 * - UPDATE_SKIP_ALL_SIDEEFFECTS (816)  = SKIP_ON_PLACE | SKIP_BLOCK_ENTITY_SIDEEFFECTS
 *                                        | SUPPRESS_DROPS | KNOWN_SHAPE
 *
 * 结构放置惯用 `UPDATE_CLIENTS | UPDATE_KNOWN_SHAPE`（18，jigsaw 预知形状）或
 * `UPDATE_ALL`（3，逐方块放置），并额外清掉 bit0 以避免依附类方块在支撑尚未放置时自毁。
 *
 * 消费点分布：
 * - `ServerWorld::setBlockState` 解析各标志位并分派对应副作用；
 * - `Block::updateOrDestroy` 消费 UPDATE_SUPPRESS_DROPS；
 * - 形状更新出口消费 UPDATE_SKIP_SHAPE_UPDATE_ON_WIRE；
 * - UPDATE_INVISIBLE 与原版一致，仅客户端侧有意义，服务端不消费。
 *
 * TODO: UPDATE_IMMEDIATE 在当前版本无任何消费点（与原版一致，属保留位），
 *       待引入"立即执行、不入队"的更新语义时接入。
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
