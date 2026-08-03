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
#include "common/network/ir/IrPacketBase.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc::network::ir::configuration {

/**
 * @brief 已知数据包标识（KnownPack，服务端/客户端协商用）
 *
 * 对应 Java KnownPack(namespace, id, version)。服务端在 RegistryData 之前发
 * SelectKnownPacks 告知客户端已知的原版数据包，客户端回 SelectKnownPacks(C→S)
 * 确认其命中集合，未命中的注册表项才在 RegistryData 里带完整 NBT。
 */
struct KnownPack {
    std::string ns;      // 命名空间，如 "minecraft"
    std::string id;      // 标识，如 "core"
    std::string version; // 版本字符串
    [[nodiscard]] friend bool operator==(const KnownPack&, const KnownPack&) noexcept = default;
};

/**
 * @brief 注册表条目（PackedRegistryEntry）
 *
 * 对应 Java RegistrySynchronization.PackedRegistryEntry(id, Optional<Tag> data)。
 * id 是注册表项资源位置（如 "minecraft:stone"）；data 为 nullopt 表示该项客户端
 * 已知（命中 KnownPacks），无 NBT；非 nullopt 则带完整 NBT。
 */
struct RegistryEntry {
    std::string id;                      // 资源位置
    std::optional<std::vector<u8>> data; // NBT（已序列化的任意 tag 字节）；nullopt=已知无 NBT
    [[nodiscard]] friend bool operator==(const RegistryEntry&, const RegistryEntry&) noexcept = default;
};

/**
 * @brief RegistryData（S→C，id=7，注册表数据同步）
 *
 * 1.21.11 服务端在 Configuration 阶段按 registryKey 推送维度/生物群系/聊天类型等注册表。
 * 线格式：Utf8(registryKey) + VarInt(count) + count×PackedRegistryEntry。
 */
struct RegistryData {
    std::string registryKey;            // 如 "minecraft:dimension_type"
    std::vector<RegistryEntry> entries; // 条目列表
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const RegistryData&, const RegistryData&) noexcept = default;
};

/**
 * @brief UpdateTags（S→C，id=13，标签同步）
 *
 * 线格式：VarInt(registry 数) × { Utf8(registryKey) + VarInt(tag 数) ×
 *   { Utf8(tagName) + VarInt(id 数) × VarInt(elementId) } }。
 * 纯结构透传，elementId 为各 registry 里的数值 id。
 */
struct TagListEntry {
    std::string tagName;
    std::vector<i32> elementIds;
    [[nodiscard]] friend bool operator==(const TagListEntry&, const TagListEntry&) noexcept = default;
};
struct TagRegistry {
    std::string registryKey;
    std::vector<TagListEntry> tags;
    [[nodiscard]] friend bool operator==(const TagRegistry&, const TagRegistry&) noexcept = default;
};
struct UpdateTags {
    std::vector<TagRegistry> registries;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const UpdateTags&, const UpdateTags&) noexcept = default;
};

/**
 * @brief UpdateEnabledFeatures（S→C，id=12，启用特性集）
 *
 * 线格式：VarInt(count) + count×Utf8(feature 资源位置)。
 */
struct UpdateEnabledFeatures {
    std::vector<std::string> features;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const UpdateEnabledFeatures&, const UpdateEnabledFeatures&) noexcept = default;
};

/**
 * @brief SelectKnownPacks（S→C id=14 / C→S id=7，已知数据包协商）
 *
 * 双向：S→C 列出服务端已知的原版数据包；C→S 回客户端命中的子集（上限 64）。
 * 线格式：VarInt(count) + count×KnownPack{Utf8(ns)+Utf8(id)+Utf8(version)}。
 */
struct SelectKnownPacks {
    std::vector<KnownPack> knownPacks;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const SelectKnownPacks&, const SelectKnownPacks&) noexcept = default;
};

/**
 * @brief ClientInformation（C→S，id=0，客户端设置）
 *
 * 对应 Java ServerboundClientInformationPacket。线格式：
 * Utf8(16, language) + U8(viewDistance) + VarInt(chatVisibility) + Bool(chatColors) +
 * U8(modelCustomisation) + VarInt(mainHand) + Bool(textFilteringEnabled) +
 * Bool(allowsListing) + VarInt(particleStatus)。
 */
struct ClientInformation {
    std::string language; // 如 "en_us"
    u8 viewDistance;      // 渲染距离（chunk）
    i32 chatVisibility;   // 0=FULL 1=SYSTEM 2=HIDDEN
    bool chatColors;
    u8 modelCustomisation; // 位掩码
    i32 mainHand;          // 0=LEFT 1=RIGHT
    bool textFilteringEnabled;
    bool allowsListing;
    i32 particleStatus; // 0=ALL 1=MINIMAL 2=DECREASED
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ClientInformation&, const ClientInformation&) noexcept = default;
};

/**
 * @brief CustomPayload（双向，S→C id=1 / C→S id=2，自定义数据通道）
 *
 * 线格式：Utf8(identifier) + VarInt(len) + bytes(payload)。未识别 identifier 时
 * payload 按原始字节透传。BrandPayload(minecraft:brand) 是常见子类型。
 */
struct CustomPayload {
    std::string identifier;  // 资源位置，如 "minecraft:brand"
    std::vector<u8> payload; // 原始 payload 字节
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const CustomPayload&, const CustomPayload&) noexcept = default;
};

/**
 * @brief KeepAlive（Configuration 阶段双向，S→C id=4 / C→S id=4）
 *
 * 与 Play 阶段 KeepAlive 同构：I64(id)。配置阶段心跳。
 */
struct KeepAlive {
    i64 id;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const KeepAlive&, const KeepAlive&) noexcept = default;
};

/**
 * @brief Disconnect（Configuration 阶段 S→C，id=2）
 *
 * reason 为文本组件（此处用 JSON 字符串占位，组件 NBT 对齐留 Phase6）。
 */
struct Disconnect {
    std::string reason; // JSON 文本组件
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const Disconnect&, const Disconnect&) noexcept = default;
};

/**
 * @brief Ping（S→C id=5 / C→S id=5，配置阶段 ping）
 *
 * 线格式：I32(parameter)。对端回 Pong(parameter)（Pong 复用本结构，id=4/C→S pong）。
 */
struct Ping {
    i32 parameter;
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const Ping&, const Ping&) noexcept = default;
};

/**
 * @brief FinishConfiguration（S→C id=3 / C→S id=3，配置完成，terminal）
 *
 * 双向 terminal 包：服务端发完所有配置数据后发 S→C 版，客户端回 C→S 版确认，
 * 双方处理后切到 Play 阶段。
 */
struct FinishConfiguration {
    static constexpr bool kTerminal = true;

    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const FinishConfiguration&, const FinishConfiguration&) noexcept = default;
};

} // namespace mc::network::ir::configuration
