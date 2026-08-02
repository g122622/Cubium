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

#include "common/core/Result.hpp"
#include "common/item/component/DataComponentMap.hpp"

#include <utility>
#include <vector>

namespace mc {
namespace network {
namespace buffer {
class ByteBuf;
} // namespace buffer
} // namespace network

namespace item {
namespace component {

/**
 * @brief 将 DataComponentPatch 写入 Java wire 缓冲
 *
 * 线格式（1.21.11 DataComponentPatch.STREAM_CODEC）：
 *   VarInt(addedCount) + VarInt(removedCount)   // 两 count 在前
 *   + [ VarInt(typeId) + per-component value ]* // added 段（每组件专属 codec，无统一外壳）
 *   + [ VarInt(typeId) ]*                       // removed 段
 *
 * 每组件 value 用其专属 streamCodec（Damage/RepairCost=裸VarInt、CustomName=ComponentNBT、
 * Lore=VarInt+NBT序列、Enchantments=ItemEnchantments、PotionContents=PotionContentsPayload、
 * CanPlaceOn/CanBreak=空list降级、CustomData=根NBT），无统一 compound{v:NBT} 外壳。
 * typeId 是 Java DATA_COMPONENT_TYPE 注册表 id（DataComponentType 枚举值）。
 *
 * 未落地 typeId 不写出（调用方保证）。
 *
 * @param buf 目标缓冲
 * @param patch 组件补丁
 * @return 成功或错误
 */
[[nodiscard]] Result<void> writePatchToWire(network::buffer::ByteBuf& buf, const DataComponentPatch& patch);

/**
 * @brief 从 Java wire 缓冲读取 DataComponentPatch
 *
 * 线格式见 writePatchToWire。未知 typeId 报错（vanilla 标准 patch 无长度前缀，无法安全跳过）。
 *
 * @param buf 源缓冲
 * @return 解析出的 patch 或错误
 */
[[nodiscard]] Result<DataComponentPatch> readPatchFromWire(network::buffer::ByteBuf& buf);

/**
 * @brief 按 1.21.11 wire 规则消费 buf 中的 DataComponentPatch 区段，原样返回其字节
 *
 * 与 readPatchFromWire 同结构（两 count 在前 + added 段 + removed 段），但只推进游标、
 * 不构造 DataComponentPatch 对象，把消费到的字节副本返回。供 ItemStack metadata / 容器
 * 物品的读侧把 patch 作为透传字节存入 ItemStackView.componentsPatch（由 ItemStackBridge
 * 后续用 readPatchFromWire 还原为业务侧 patch）。
 *
 * 每组件 value 按 typeId 分派到专属 codec 跳过（与 readPatchFromWire 同分派）；未知 typeId
 * 报错（vanilla 标准 patch 无长度前缀，无法跳过未知组件）。
 *
 * @param buf 源缓冲（游标须位于 patch 区段起点）
 * @return patch 区段的原始 wire 字节或错误
 */
[[nodiscard]] Result<std::vector<u8>> readPatchBytesFromWire(network::buffer::ByteBuf& buf);

/**
 * @brief 计算 DataComponentPatch 各组件的 CRC-32C 哈希（HashedStack 用）
 *
 * 遍历 patch.added()，每个组件用其专属 codec（与 writePatchToWire 同分派）编码到临时
 * ByteBuf，对 wire 字节算 CRC-32C，产出 (typeId→hash) 列表；patch.removed() 直接收集 typeId。
 *
 * 仅落地组件参与（componentTypeById 命中），未落地组件跳过。哈希算法为轻量 CRC-32C，
 * 与 vanilla HashOps.CRC32C_INSTANCE（DataFixers 体系）不保证字节相等，仅供我方 IR
 * 自洽承载 HashedPatchMap 结构化字段（见 PlayPackets.hpp HashedStack 文档）。
 *
 * @param patch 组件补丁
 * @return {addedHashes(typeId,hash), removedTypes(typeId)}；编解码失败项跳过（不中断）
 */
[[nodiscard]] std::pair<std::vector<std::pair<i32, i32>>, std::vector<i32>> computeComponentHashes(
    const DataComponentPatch& patch);

} // namespace component
} // namespace item
} // namespace mc
