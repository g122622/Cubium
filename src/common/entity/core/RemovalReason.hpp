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

/**
 * @brief 实体移除原因
 *
 * 对齐 vanilla 1.21.11 Entity.RemovalReason（Entity.java:4083-4103）。每个取值携带两个
 * 正交语义标志（shouldDestroy / shouldSave），是移除副作用与存档决策的唯一依据：
 *
 * | 原因               | shouldDestroy | shouldSave |
 * |--------------------|---------------|------------|
 * | Killed             | 是            | 否         |
 * | Discarded          | 是            | 否         |
 * | UnloadedToChunk    | 否            | 是         |
 * | UnloadedWithPlayer | 否            | 否         |
 * | ChangedDimension   | 否            | 否         |
 *
 * shouldDestroy 决定移除时是否执行"摧毁类"副作用：自己下骑（Entity::remove 的 stopRiding 门控）、
 * 容器实体掉落内容（箱子船/漏斗矿车等）、死亡掉落。卸载类原因（区块卸载/玩家退出/切维度）下实体
 * 只是离开当前世界视图，内容物必须随实体保留，故不执行。
 *
 * shouldSave 决定实体是否需在移除前写盘。当前项目的区块卸载走"先保存、后移除"（调用点已保证
 * 落盘顺序），此标志暂由调用点显式决定，保留供未来"移除后再决定是否保存"的路径消费。
 *
 * 本枚举原可内联于 Entity.hpp，提取为独立头以避免 EntityManager / IWorld / 事件层循环依赖
 * Entity.hpp（参照 EntityFlags.hpp 的提取先例）。
 */
enum class RemovalReason : u8 {
    Killed,             ///< 死亡流程结束（tickDeath 收尾、/kill）；触发掉落物与经验
    Discarded,          ///< 静默丢弃（消失、爆炸后自毁、中间产物清理）；无任何副作用
    UnloadedToChunk,    ///< 区块卸载；需保存，保留骑乘关系与内容物
    UnloadedWithPlayer, ///< 玩家退出/掉线
    ChangedDimension,   ///< 跨维度迁移（实体所有权移交另一维度，非真正销毁）
};

/**
 * @brief 该移除原因是否应执行"摧毁类"副作用
 *
 * 对齐 vanilla RemovalReason.shouldDestroy()：仅 Killed / Discarded 为真。移除时据此决定
 * 是否下骑（Entity::remove）与是否掉落容器内容物（箱子船/矿车容器等）。
 */
[[nodiscard]] constexpr bool shouldDestroy(RemovalReason reason) noexcept
{
    return reason == RemovalReason::Killed || reason == RemovalReason::Discarded;
}

/**
 * @brief 该移除原因下实体是否应写盘
 *
 * 对齐 vanilla RemovalReason.shouldSave()：仅 UnloadedToChunk 为真。
 */
[[nodiscard]] constexpr bool shouldSave(RemovalReason reason) noexcept
{
    return reason == RemovalReason::UnloadedToChunk;
}

} // namespace mc
