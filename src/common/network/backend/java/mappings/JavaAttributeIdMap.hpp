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
#include "common/core/Types.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

namespace mc::network::backend::java {

/**
 * @brief 项目属性名 ↔ Java attribute registry id 双向映射（Java 协议对齐层）
 *
 * update_attributes 包中每条属性快照的 attribute 字段是 Java `minecraft:attribute` 注册表
 * （`BuiltInRegistries.ATTRIBUTE`）的 holder id，wire 上是纯 `VarInt(registryId)`，0-based
 * 无偏移。该注册表不由 RegistryDataBuilder 同步（不在 SYNCHRONIZED_REGISTRIES 内，attribute
 * 是静态注册表，客户端启动期自行 bootstrap），真 Java 客户端使用其内置数据包注册的注册表，
 * id 即 vanilla 1.21.11 `Attributes.java` 的静态字段声明顺序（共 35 项）。
 *
 * 项目的属性名沿用 1.16.5 形式（`generic.max_health` 等），与 vanilla 的资源名
 * （`minecraft:max_health`）不同，故按名建立双向映射。项目自有的非 vanilla 属性
 * （`horse.jump_strength`、`generic.breath_max`）不在此表内——它们没有对应的 registry id。
 *
 * 另有两条与 vanilla 的差异需要调用方知晓：
 *   - 项目属性集合小于 vanilla（缺 `block_break_speed`、`scale`、`sneaking_speed` 等 12 条），
 *     这些 id 仍占位但 `isImplemented` 为假，不可下发；
 *   - vanilla 有 8 条属性是 `setSyncable(false)`（attack_damage 等），本表按 `isClientSyncable`
 *     标出，发送方需据此过滤。
 */
class JavaAttributeIdMap {
public:
    static JavaAttributeIdMap& instance();

    JavaAttributeIdMap() = default;
    ~JavaAttributeIdMap() = default;
    JavaAttributeIdMap(const JavaAttributeIdMap&) = delete;
    JavaAttributeIdMap& operator=(const JavaAttributeIdMap&) = delete;

    /// 构建双向映射。可重复调用（幂等，先清空再重建）。
    [[nodiscard]] Result<void> initialize();

    /// 项目属性名 → Java attribute registry id（发侧）。未知返回 nullopt。
    [[nodiscard]] std::optional<u32> toJavaRegistryId(const std::string& attributeName) const;

    /// Java attribute registry id → 项目属性名（收侧）。未知返回空串。
    [[nodiscard]] const std::string& fromJavaRegistryId(u32 javaRegistryId) const;

    /// 该项目属性是否有对应的 registry id（项目未实现的 vanilla 属性返回 false）。
    [[nodiscard]] bool isImplemented(const std::string& attributeName) const;

    /// 该 id 是否需要同步给客户端（对齐 vanilla 的 setSyncable 语义）。
    /// 未映射到项目实现的 id 恒返回 false。
    [[nodiscard]] bool isClientSyncable(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /// 已映射到项目实现的属性数（诊断用）。
    [[nodiscard]] size_t matchedCount() const noexcept { return m_internalToJava.size(); }

private:
    bool m_initialized = false;
    /// 项目属性名 → vanilla registry id
    std::unordered_map<std::string, u32> m_internalToJava;
    /// vanilla registry id → 项目属性名
    std::unordered_map<u32, std::string> m_javaToInternal;
    /// vanilla registry id → 是否同步给客户端
    std::unordered_map<u32, bool> m_javaSyncable;
};

} // namespace mc::network::backend::java
