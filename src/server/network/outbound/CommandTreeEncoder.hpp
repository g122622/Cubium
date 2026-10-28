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

#include "common/command/CommandNode.hpp"
#include "common/command/CommandTreeSnapshot.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/buffer/ByteBuf.hpp"

#include <climits>
#include <cstdint>
#include <limits>
#include <map>
#include <queue>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::server::net {

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
///
/// 1.21.11 里只有下列类型带非空 properties，其余（bool/block_pos/color/component/style/
/// message/nbt_path/objective/gamemode/item……）都是 SingletonArgumentInfo，网络上只写数值 id。
enum : i32 {
    kArgFloat = 1,
    kArgDouble = 2,
    kArgInteger = 3,
    kArgString = 5,
    kArgEntity = 6,
    kArgNbtCompound = 21,
    kArgScoreHolder = 31,
    kArgTime = 43,
    kArgResourceOrTag = 44,
    kArgResourceOrTagKey = 45,
    kArgResource = 46,
    kArgResourceKey = 47,
    kArgResourceSelector = 48,
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

/// 读 JSON 字段为浮点（f64），缺失返回 defaultValue。
[[nodiscard]] inline f64 jsonDouble(const nlohmann::json& j, const char* key, f64 defaultValue)
{
    if (!j.is_object() || !j.contains(key) || !j.at(key).is_number()) {
        return defaultValue;
    }
    return j.at(key).get<f64>();
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

/// 读 JSON 字段为字符串，缺失/类型不符返回 defaultValue。
[[nodiscard]] inline std::string jsonString(const nlohmann::json& j, const char* key, const char* defaultValue)
{
    if (!j.is_object() || !j.contains(key) || !j.at(key).is_string()) {
        return defaultValue;
    }
    return j.at(key).get<std::string>();
}

/// 参数所属注册表键（资源类参数的必填 metadata），缺失时回落到通用占位。
[[nodiscard]] inline std::string registryKeyOf(const nlohmann::json& props)
{
    return jsonString(props, "registry", "minecraft:resource");
}

/// 写 ArgumentType 的类型特定 properties（serializeCap 的 properties 部分，数值 id 已由调用方写出）。
/// 返回 Error 表示 properties 缺失/非法（如 typeName 未映射到数值 id）。
[[nodiscard]] inline mc::Result<void> writeArgumentProperties(
    mc::network::buffer::ByteBuf& buf, i32 networkId, const nlohmann::json& props, const std::string& typeName)
{
    // 判据一律照抄原版 1.21.11 对应的 ArgumentTypeInfo.serializeToNetwork：
    //   IntegerArgumentInfo            → byte numFlags + [i32 min] + [i32 max]
    //   FloatArgumentInfo / Double…    → byte numFlags + [f32/f64 min] + [f32/f64 max]
    //   StringArgumentType.Info        → VarInt(StringType.id)
    //   NbtArgumentInfo                → ResourceLocation 类型的自定义名（writeIdentifier），没有则空
    //   EntityArgument.Info            → byte flags + 无 NBT 时多一个 has_nbt 字节
    //   ScoreHolderArgument.Info       → byte flags(multiple)
    //   TimeArgumentInfo               → i32 min
    //   ResourceArgument / KeyArgument / ResourceOrTag(*) / ResourceOrTagKey / ResourceOrId
    //                                  → writeResourceKey(registryKey)
    //   ResourceSelectorArgument       → writeResourceKey(registryKey)
    //   其余（SingletonArgumentInfo）  → 空载荷
    switch (networkId) {
        case kArgFloat: {
            // 原版 FloatArgumentInfo 的 "has" 判据是 min != -Float.MAX_VALUE / max != Float.MAX_VALUE；
            // 项目 FloatArgumentType::serializeMetadata 无条件写 min/max，故按原版默认哨兵 gating。
            constexpr f32 kDefaultMin = -std::numeric_limits<f32>::max();
            constexpr f32 kDefaultMax = std::numeric_limits<f32>::max();
            const f32 minVal = jsonFloat(props, "min", kDefaultMin);
            const f32 maxVal = jsonFloat(props, "max", kDefaultMax);
            const bool hasMin = minVal != kDefaultMin;
            const bool hasMax = maxVal != kDefaultMax;
            buf.writeU8(static_cast<u8>((hasMin ? 0x01 : 0) | (hasMax ? 0x02 : 0)));
            if (hasMin) {
                buf.writeF32(minVal);
            }
            if (hasMax) {
                buf.writeF32(maxVal);
            }
            return {};
        }

        case kArgDouble: {
            constexpr f64 kDefaultMin = -std::numeric_limits<f64>::max();
            constexpr f64 kDefaultMax = std::numeric_limits<f64>::max();
            const f64 minVal = jsonDouble(props, "min", kDefaultMin);
            const f64 maxVal = jsonDouble(props, "max", kDefaultMax);
            const bool hasMin = minVal != kDefaultMin;
            const bool hasMax = maxVal != kDefaultMax;
            buf.writeU8(static_cast<u8>((hasMin ? 0x01 : 0) | (hasMax ? 0x02 : 0)));
            if (hasMin) {
                buf.writeF64(minVal);
            }
            if (hasMax) {
                buf.writeF64(maxVal);
            }
            return {};
        }

        case kArgInteger: {
            // 原版 IntegerArgumentInfo 的判据是 min != Integer.MIN_VALUE / max != Integer.MAX_VALUE。
            constexpr i32 kDefaultMin = INT32_MIN;
            constexpr i32 kDefaultMax = INT32_MAX;
            const i32 minVal = jsonInt(props, "min", kDefaultMin);
            const i32 maxVal = jsonInt(props, "max", kDefaultMax);
            const bool hasMin = minVal != kDefaultMin;
            const bool hasMax = maxVal != kDefaultMax;
            buf.writeU8(static_cast<u8>((hasMin ? 0x01 : 0) | (hasMax ? 0x02 : 0)));
            if (hasMin) {
                buf.writeI32(minVal);
            }
            if (hasMax) {
                buf.writeI32(maxVal);
            }
            return {};
        }

        case kArgString: {
            // 原版 brigadier:string：VarInt StringType 序数（0=SingleWord,1=QuotablePhrase,2=GreedyPhrase）。
            i32 mode = 1;
            if (props.is_object()) {
                if (jsonBool(props, "greedy", false)) {
                    mode = 2;
                } else if (!jsonBool(props, "quote", true)) {
                    mode = 0;
                }
            }
            buf.writeVarInt(mode);
            return {};
        }

        case kArgEntity: {
            // minecraft:entity：byte flags(bit0=single, bit1=playersOnly)；带 NBT 的实体参数
            // （EntityArgument.entity(CommandBuildContext)）紧接着多一个 has_nbt 字节。
            const u8 flags = static_cast<u8>(
                (jsonBool(props, "single", true) ? 0x01 : 0) | (jsonBool(props, "playersOnly", false) ? 0x02 : 0));
            buf.writeU8(flags);
            if (jsonBool(props, "withNbt", false)) {
                buf.writeU8(1);
            }
            return {};
        }

        case kArgScoreHolder: {
            // minecraft:score_holder：byte flags(bit0=multiple)。
            buf.writeU8(static_cast<u8>(jsonBool(props, "multiple", false) ? 0x01 : 0));
            return {};
        }

        case kArgTime: {
            // minecraft:time：i32 min（即使 min=0 也写）。
            buf.writeI32(jsonInt(props, "min", 0));
            return {};
        }

        case kArgResource:
        case kArgResourceKey:
        case kArgResourceOrTag:
        case kArgResourceOrTagKey:
        case kArgResourceSelector: {
            // 一律 writeResourceKey(registryKey)：先试会话注册表序号、否则写完整 ResourceLocation。
            // 客户端据此在正确的注册表里补全；写错注册表会让补全内容张冠李戴。
            buf.writeString(registryKeyOf(props));
            return {};
        }

        default:
            // 其余类型（bool/block_pos/color/component/style/message/nbt_path/objective/
            // resource_location/gamemode/item_slot/heightmap/loot_table/dialog……）
            // 在 1.21.11 都是 SingletonArgumentInfo：没有 properties。
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

namespace command_tree_detail {

/**
 * @brief 只编码参数节点的 properties 部分（不含类型数值 id，也不含建议提供器）
 *
 * 差分测试用它与原版 `ArgumentTypeInfo.serializeToNetwork` 的输出逐字节比对：
 * 类型 id 单独比对，负载单独比对，任何一侧写多/写少字节都能立刻定位到具体是
 * id 错还是 payload 错。
 *
 * @param node 参数节点快照（type 必须是 Argument）
 * @return properties 字节；类型名没有数值 id 时返回 Error
 */
[[nodiscard]] inline mc::Result<std::vector<u8>> encodeArgumentPropertiesOnly(
    const mc::command::CommandTreeNodeSnapshot& node)
{
    if (!node.argumentNetworkId.has_value()) {
        return mc::Error(mc::ErrorCode::InvalidData,
            "Command tree argument node has unmapped typeName '" + node.typeName + "'",
            "CommandTreeEncoder.encodeArgumentPropertiesOnly");
    }
    mc::network::buffer::ByteBuf buf;
    auto propsResult = writeArgumentProperties(buf, *node.argumentNetworkId, node.argumentProperties, node.typeName);
    if (!propsResult.success()) {
        return propsResult.error();
    }
    return buf.takeBytes();
}

} // namespace command_tree_detail

/// 把 CommandTreeSnapshot 编码为 1.21.11 ClientboundCommandsPacket 包体字节
/// （VarInt(nodeCount) + nodes + VarInt(rootIndex)）。失败返回 Error（如遇未映射的 typeName）。
[[nodiscard]] inline mc::Result<std::vector<u8>> encodeCommandTree(const mc::command::CommandTreeSnapshot& snapshot)
{
    mc::network::buffer::ByteBuf buf;
    if (snapshot.nodes.empty()) {
        buf.writeVarInt(0);
        buf.writeVarInt(0);
        return buf.takeBytes();
    }

    // 无执行回调的字面量及 Brigadier 基础参数按值相等。先合并等价子树，再按 BFS 编号；
    // 保留原快照中的重定向修改器等执行信息，合并仅作用于下发给客户端的节点条目。
    constexpr u32 UNASSIGNED = std::numeric_limits<u32>::max();
    std::vector<u32> groups(snapshot.nodes.size(), UNASSIGNED);
    std::vector<mc::command::CommandTreeNodeSnapshot> unique;
    std::map<std::vector<u8>, u32> signatures;
    const auto collect = [&](const auto& self, u32 index) -> mc::Result<u32> {
        if (groups[index] != UNASSIGNED) {
            return groups[index];
        }
        auto node = snapshot.nodes[index];
        for (auto& child : node.children) {
            auto group = self(self, child);
            if (!group.success()) {
                return group.error();
            }
            child = group.value();
        }
        const bool valueArgument = node.type == mc::command::NodeType::Argument && node.argumentNetworkId.has_value() &&
            *node.argumentNetworkId >= 0 && *node.argumentNetworkId <= 5;
        const u32 group = static_cast<u32>(unique.size());
        if (!node.executable && (node.type == mc::command::NodeType::Literal || valueArgument)) {
            mc::network::buffer::ByteBuf signature;
            auto result = command_tree_detail::writeNode(signature, node);
            if (!result.success()) {
                return result.error();
            }
            const auto [it, inserted] = signatures.emplace(signature.takeBytes(), group);
            if (!inserted) {
                groups[index] = it->second;
                return it->second;
            }
        }
        groups[index] = group;
        unique.push_back(std::move(node));
        return group;
    };
    for (u32 index = 0; index < snapshot.nodes.size(); ++index) {
        auto result = collect(collect, index);
        if (!result.success()) {
            return result.error();
        }
    }

    std::vector<u32> indices(unique.size(), UNASSIGNED);
    std::vector<u32> order;
    std::queue<u32> pending;
    pending.push(groups[0]);
    while (!pending.empty()) {
        const u32 group = pending.front();
        pending.pop();
        if (indices[group] != UNASSIGNED) {
            continue;
        }
        indices[group] = static_cast<u32>(order.size());
        order.push_back(group);
        for (const u32 child : unique[group].children) {
            pending.push(child);
        }
        if (unique[group].redirect.has_value()) {
            pending.push(groups[*unique[group].redirect]);
        }
    }

    buf.writeVarInt(static_cast<i32>(order.size()));
    for (const u32 group : order) {
        auto& node = unique[group];
        for (auto& child : node.children) {
            child = indices[child];
        }
        if (node.redirect.has_value()) {
            node.redirect = indices[groups[*node.redirect]];
        }
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

} // namespace mc::server::net
