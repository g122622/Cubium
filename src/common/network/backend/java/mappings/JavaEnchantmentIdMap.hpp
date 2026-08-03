/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
#include <string>
#include <string_view>
#include <unordered_map>

namespace mc::network::backend::java {

/**
 * @brief 附魔资源位置 ↔ Java enchantment 注册表 wire id 双向映射（Java 协议对齐层）
 *
 * ItemEnchantments wire（1.21.11 ItemEnchantments.STREAM_CODEC）的每个条目 key 是 Java
 * `minecraft:enchantment` 注册表（Registries.ENCHANTMENT）的 holder id（wire 上是纯
 * VarInt(registryId)）。enchantment 是动态注册表，由服务端经 RegistryData 同步给客户端，
 * 同步顺序 = vanilla `Enchantments.bootstrap()` 内 register 调用顺序（0-based，共 43 项）。
 *
 * 本项目 EnchantmentRegistry 按相同 vanilla 顺序注册内置附魔，故 wire id 与 vanilla 一致。
 * 本表以资源位置字符串（"minecraft:protection"）为 key 建立双向映射，供 ItemEnchantments
 * wire codec（DataComponentPatch 的 ENCHANTMENTS 分支）在发侧把字符串 id 转 wire id、
 * 收侧把 wire id 还原字符串 id。
 *
 * 数据源：vanilla `Enchantments.java` bootstrap() 的 register 调用顺序（手写固定 43 项表，
 * 0-based）。注意 LUNGE 在 bootstrap 第 33 位（非常量声明第 41 位），表须按 bootstrap 序。
 *
 * enchantment 是 holderRegistry（纯 VarInt id），与 MobEffect/DimensionType 同模式。
 * 边界收口于 ItemEnchantments wire codec，附魔业务子系统零感知。
 */
class JavaEnchantmentIdMap {
public:
    static JavaEnchantmentIdMap& instance();

    JavaEnchantmentIdMap() = default;
    ~JavaEnchantmentIdMap() = default;
    JavaEnchantmentIdMap(const JavaEnchantmentIdMap&) = delete;
    JavaEnchantmentIdMap& operator=(const JavaEnchantmentIdMap&) = delete;

    /// 构建双向映射。可重复调用（幂等，先清空再重建）。
    [[nodiscard]] Result<void> initialize();

    /// 附魔资源位置 → Java enchantment registry id（发侧）。查不到返回 0（protection）并 warn。
    [[nodiscard]] u32 toJavaRegistryId(std::string_view enchantmentId) const;

    /// Java enchantment registry id → 附魔资源位置（收侧）。查不到返回空串并 warn。
    [[nodiscard]] std::string fromJavaRegistryId(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /// 已映射的附魔数（诊断用）。
    [[nodiscard]] size_t matchedCount() const noexcept { return m_nameToJava.size(); }

private:
    bool m_initialized = false;
    /// "minecraft:protection" → vanilla registry id
    std::unordered_map<std::string, u32> m_nameToJava;
    /// vanilla registry id → "minecraft:protection"
    std::unordered_map<u32, std::string> m_javaToName;
};

} // namespace mc::network::backend::java
