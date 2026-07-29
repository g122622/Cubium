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

#include "common/command/CommandTreeSnapshot.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/buffer/ByteBuf.hpp"

#include <climits>
#include <limits>
#include <string>
#include <vector>

namespace mc::network::java::codecs {

// ============================================================================
// clientbound/minecraft:commands 二进制 CommandNode 树编码器
//
// 对齐 Java 1.21.11 ClientboundCommandsPacket（protocol 774）。替换旧占位实现
// （把命令树 JSON 文本当 opaque payload 透传，真 Java 客户端按二进制线格式解码必崩，
// 报 "Non [a-z0-9_.-] character in namespace" —— disconnect-2026-07-29_17.24.34）。
//
// 包体线格式（ClientboundCommandsPacket.java:52-61）：
//   VarInt(nodeCount) + nodeCount×Entry + VarInt(rootIndex)
// 包尾是单个 VarInt(rootIndex)（非 rootCount 列表）。root 是 nodes[rootIndex] 的普通条目，
// snapshot.nodes[0] 即 root（BFS 从 root 起），故 rootIndex=0。
//
// 单个 Entry 字段顺序（Entry.write:232-242，严格）：
//   1. byte flags
//   2. VarInt(childCount) + childCount×VarInt(childIndex)   ← children 在 name 之前
//   3. 若 flags&0x08：VarInt(redirectIndex)
//   4. 类型 stub（依 flags&0x03）：
//        ROOT(0)：无 stub（不写 name）
//        LITERAL(1)：writeUtf(name)
//        ARGUMENT(2)：writeUtf(name) + serializeCap + 若 flags&0x10：writeIdentifier(suggestionId)
//
// flags 位（ClientboundCommandsPacket.java:35-42）：
//   bits 0-1 (0x03)：type（0=root,1=literal,2=argument）
//   bit 2 (0x04)：executable
//   bit 3 (0x08)：has_redirect
//   bit 4 (0x10)：has_custom_suggestions（仅 argument）
//   bit 5 (0x20)：restricted（项目无此信息，保守 0）
//
// serializeCap：VarInt(numericRegistryId) + properties。数值 id 非 Identifier 字符串。
// ByteBuf 大端，与 vanilla FriendlyByteBuf 一致。
// ============================================================================

namespace command_tree_detail {

/// ArgumentType 数值注册表 id（ArgumentTypeInfos.bootstrap 顺序）。
/// 与 CommandTreeSnapshot.hpp detail::argumentNetworkIdForTypeName 一致，此处供编码器内联判定。
enum : i32 {
    kArgBool = 0,
    kArgFloat = 1,
    kArgInteger = 3,
    kArgString = 5,
    kArgEntity = 6,
    kArgTime = 43,
};

/// flags 类型位（bits 0-1）。
[[nodiscard]] inline u8 typeBits(mc::command::NodeType type)
{
    switch (type) {
        case mc::command::NodeType::Root:
            return 0x00;
        case mc::command::NodeType::Literal:
            return 0x01;
        case mc::command::NodeType::Argument:
            return 0x02;
    }
    return 0x00;
}

/// 组装单个节点的 flags 字节。
[[nodiscard]] inline u8 assembleFlags(const mc::command::CommandTreeNodeSnapshot& node)
{
    u8 flags = typeBits(node.type);
    if (node.executable) {
        flags |= 0x04;
    }
    if (node.redirect.has_value()) {
        flags |= 0x08;
    }
    // has_custom_suggestions（bit4）仅对 argument 节点有意义；suggestionProviderId 有值即置位。
    if (node.type == mc::command::NodeType::Argument && node.suggestionProviderId.has_value()) {
        flags |= 0x10;
    }
    // bit5 restricted：保守 0（项目无 NodeInspector 等价信息）。
    return flags;
}

/// 读 JSON 字段为 bool，缺失返回 defaultValue。
[[nodiscard]] inline bool jsonBool(const nlohmann::json& j, const char* key, bool defaultValue)
{
    if (!j.is_object() || !j.contains(key) || !j.at(key).is_boolean()) {
        return defaultValue;
    }
    return j.at(key).get<bool>();
}

/// 读 JSON 字段为整数（i32），缺失返回 defaultValue。
[[nodiscard]] inline i32 jsonInt(const nlohmann::json& j, const char* key, i32 defaultValue)
{
    if (!j.is_object() || !j.contains(key) || !j.at(key).is_number_integer()) {
        return defaultValue;
    }
    return j.at(key).get<i32>();
}

/// 读 JSON 字段为浮点（f32），缺失返回 defaultValue。
[[nodiscard]] inline f32 jsonFloat(const nlohmann::json& j, const char* key, f32 defaultValue)
{
    if (!j.is_object() || !j.contains(key) || !j.at(key).is_number()) {
        return defaultValue;
    }
    return j.at(key).get<f32>();
}

/// 写 ArgumentType 的类型特定 properties（serializeCap 的 properties 部分，数值 id 已由调用方写出）。
/// 返回 Error 表示 properties 缺失/非法（如 typeName 未映射到数值 id）。
[[nodiscard]] inline mc::Result<void> writeArgumentProperties(
    mc::network::buffer::ByteBuf& buf, i32 networkId, const nlohmann::json& props, const std::string& typeName)
{
    switch (networkId) {
        case kArgBool:
            // brigadier:bool：无 properties。
            return {};

        case kArgFloat: {
            // brigadier:float：byte numFlags(bit0=hasMin,bit1=hasMax); 有 min→f32; 有 max→f32。
            // vanilla FloatArgumentInfo.serializeToNetwork 的 "has" 判据是 min != -Float.MAX_VALUE /
            // max != Float.MAX_VALUE（即非默认无界），而非 JSON 是否含字段。项目 FloatArgumentType::
            // serializeMetadata 无条件写 min/max（即使 -FLT_MAX/FLT_MAX），故此处须按 vanilla 默认
            // 哨兵 gating：min==(-FLT_MAX) 视为无下界，max==FLT_MAX 视为无上界，不写对应字节。
            // 否则每个无界 float 多写 4~8 字节，致客户端游标错位 → IndexOutOfBoundsException。
            const f32 kDefaultMin = -std::numeric_limits<f32>::max();
            const f32 kDefaultMax = std::numeric_limits<f32>::max();
            const f32 minVal = jsonFloat(props, "min", kDefaultMin);
            const f32 maxVal = jsonFloat(props, "max", kDefaultMax);
            const bool hasMin = minVal != kDefaultMin;
            const bool hasMax = maxVal != kDefaultMax;
            const u8 numFlags = static_cast<u8>((hasMin ? 0x01 : 0) | (hasMax ? 0x02 : 0));
            buf.writeU8(numFlags);
            if (hasMin) {
                buf.writeF32(minVal);
            }
            if (hasMax) {
                buf.writeF32(maxVal);
            }
            return {};
        }

        case kArgInteger: {
            // brigadier:integer：byte numFlags(bit0=hasMin,bit1=hasMax); 有 min→i32; 有 max→i32。
            // vanilla IntegerArgumentInfo.serializeToNetwork 的 "has" 判据是 min != Integer.MIN_VALUE /
            // max != Integer.MAX_VALUE（即非默认无界），而非 JSON 是否含字段。项目 IntegerArgumentType::
            // serializeMetadata 无条件写 min/max（即使 INT32_MIN/INT32_MAX），故此处须按 vanilla 默认
            // 哨兵 gating：min==INT32_MIN 视为无下界，max==INT32_MAX 视为无上界，不写对应字节。
            // 否则每个无界 integer 多写 4~8 字节，致客户端游标错位 → IndexOutOfBoundsException。
            constexpr i32 kDefaultMin = INT32_MIN;
            constexpr i32 kDefaultMax = INT32_MAX;
            const i32 minVal = jsonInt(props, "min", kDefaultMin);
            const i32 maxVal = jsonInt(props, "max", kDefaultMax);
            const bool hasMin = minVal != kDefaultMin;
            const bool hasMax = maxVal != kDefaultMax;
            const u8 numFlags = static_cast<u8>((hasMin ? 0x01 : 0) | (hasMax ? 0x02 : 0));
            buf.writeU8(numFlags);
            if (hasMin) {
                buf.writeI32(minVal);
            }
            if (hasMax) {
                buf.writeI32(maxVal);
            }
            return {};
        }

        case kArgString: {
            // brigadier:string：VarInt StringType 序数（0=SingleWord,1=QuotablePhrase,2=GreedyPhrase）。
            // typeName 直接对应：word→0, phrase→1, greedy_string→2。
            i32 mode = 1; // 默认 QuotablePhrase
            if (typeName == "word") {
                mode = 0;
            } else if (typeName == "phrase") {
                mode = 1;
            } else if (typeName == "greedy_string") {
                mode = 2;
            } else if (props.is_object() && props.contains("mode") && props.at("mode").is_string()) {
                const auto& modeStr = props.at("mode").get<std::string>();
                if (modeStr == "word" || modeStr == "single_word") {
                    mode = 0;
                } else if (modeStr == "phrase" || modeStr == "quotable_phrase") {
                    mode = 1;
                } else if (modeStr == "greedy_string" || modeStr == "greedy_phrase") {
                    mode = 2;
                }
            }
            buf.writeVarInt(mode);
            return {};
        }

        case kArgEntity: {
            // minecraft:entity：byte flags(bit0=single, bit1=playersOnly)。
            const u8 flags = static_cast<u8>(
                (jsonBool(props, "single", true) ? 0x01 : 0) | (jsonBool(props, "playersOnly", false) ? 0x02 : 0));
            buf.writeU8(flags);
            return {};
        }

        case kArgTime: {
            // minecraft:time：i32 min（即使 min=0 也写）。
            buf.writeI32(jsonInt(props, "min", 0));
            return {};
        }

        default:
            // block_pos/vec3/vec2/block_state/item/item_predicate/nbt_*/item_slot/
            // resource_location/function/dimension/gamemode：1.21.11 SingletonArgumentInfo 空 payload。
            return {};
    }
}

/// 写单个 Entry（CommandNode）。
[[nodiscard]] inline mc::Result<void> writeNode(
    mc::network::buffer::ByteBuf& buf, const mc::command::CommandTreeNodeSnapshot& node)
{
    const u8 flags = assembleFlags(node);
    buf.writeU8(flags);

    // 2. children（VarInt count + indices）—— 在 name 之前。
    buf.writeVarInt(static_cast<i32>(node.children.size()));
    for (const u32 child : node.children) {
        buf.writeVarInt(static_cast<i32>(child));
    }

    // 3. redirect（仅 has_redirect）。
    if (node.redirect.has_value()) {
        buf.writeVarInt(static_cast<i32>(*node.redirect));
    }

    // 4. 类型 stub。
    switch (node.type) {
        case mc::command::NodeType::Root:
            // ROOT：无 stub。
            return {};

        case mc::command::NodeType::Literal:
            // LITERAL：writeUtf(name)。
            buf.writeString(node.name);
            return {};

        case mc::command::NodeType::Argument: {
            // ARGUMENT：writeUtf(name) + serializeCap + [suggestionId]。
            buf.writeString(node.name);

            if (!node.argumentNetworkId.has_value()) {
                return mc::Error(mc::ErrorCode::InvalidData,
                    "Command tree argument node has unmapped typeName '" + node.typeName +
                        "' (no COMMAND_ARGUMENT_TYPE numeric id; enum is unsupported)",
                    "CommandTreeEncoder.writeNode");
            }
            const i32 networkId = *node.argumentNetworkId;
            buf.writeVarInt(networkId);

            auto propsResult = writeArgumentProperties(buf, networkId, node.argumentProperties, node.typeName);
            if (!propsResult.success()) {
                return propsResult.error();
            }

            // suggestionId（仅 flags&0x10，即 argument + hasCustomSuggestions）。
            if (flags & 0x10) {
                buf.writeString(node.suggestionProviderId.value_or("minecraft:ask_server"));
            }
            return {};
        }
    }
    return {};
}

} // namespace command_tree_detail

/// 把 CommandTreeSnapshot 编码为 1.21.11 ClientboundCommandsPacket 包体字节
/// （VarInt(nodeCount) + nodes + VarInt(rootIndex)）。失败返回 Error（如遇未映射的 typeName）。
[[nodiscard]] inline mc::Result<std::vector<u8>> encodeCommandTree(const mc::command::CommandTreeSnapshot& snapshot)
{
    mc::network::buffer::ByteBuf buf;

    // 空树：vanilla 不会发空 commands 包，但防御性处理——写 0 节点 + rootIndex=0。
    // （真服务端命令树恒有 root，故正常路径不触发。）
    const auto nodeCount = static_cast<i32>(snapshot.nodes.size());
    buf.writeVarInt(nodeCount);

    for (const auto& node : snapshot.nodes) {
        auto nodeResult = command_tree_detail::writeNode(buf, node);
        if (!nodeResult.success()) {
            return nodeResult.error();
        }
    }

    // 包尾 rootIndex：snapshot.nodes[0] 即 root（BFS 从 root 起）。
    // 空树 rootIndex=0。
    buf.writeVarInt(0);

    return buf.takeBytes();
}

} // namespace mc::network::java::codecs
