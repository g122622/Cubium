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

#include <string>
#include <string_view>
#include <unordered_map>

namespace mc::network::backend::java {

/**
 * @brief 项目 Potion 资源位置 ↔ Java potion registry id 双向映射（Java 协议对齐层）
 *
 * PotionContents 组件的 potion 字段是 Java `minecraft:potion` 注册表
 * （`BuiltInRegistries.POTION`）的 holder id（wire 上是纯 VarInt(registryId)）。该注册表
 * 未由本项目 RegistryDataBuilder 同步（不在 23 个 SYNCHRONIZED_REGISTRIES，potion 是静态
 * 注册表，由客户端 Potions.bootstrap 在启动期注册），真 Java 客户端使用其内置 vanilla core
 * 包注册表，id 为 vanilla 1.21.11 `Potions.java` 静态字段声明顺序（water=0/…/infested=46，
 * 共 47 项；注意 vanilla 不含 "empty"，empty 是 PotionContents 语义非注册项）。
 *
 * 项目 `PotionRegistry` 按资源位置（"minecraft:water"）索引，注册名与 vanilla 一致，但内部
 * 无 vanilla registry id。本表按资源名建立 id 映射，与 JavaMobEffectIdMap 同构。
 *
 * 数据源：vanilla `Potions.java` 静态字段声明顺序（手写固定 47 项，0-based）。条目少且固定，
 * 无需预烘焙脚本，直接编码进 .cpp 静态表。
 *
 * 边界收口于 PotionContents 专属 codec（DataComponentPatch 内的 POTION_CONTENTS 组件），
 * potion 业务子系统零感知。
 */
class JavaPotionIdMap {
public:
    static JavaPotionIdMap& instance();

    JavaPotionIdMap() = default;
    ~JavaPotionIdMap() = default;
    JavaPotionIdMap(const JavaPotionIdMap&) = delete;
    JavaPotionIdMap& operator=(const JavaPotionIdMap&) = delete;

    /// 构建双向映射。可重复调用（幂等，先清空再重建）。
    [[nodiscard]] Result<void> initialize();

    /// 项目 Potion 资源位置字符串（如 "minecraft:water"）→ Java registry id（发侧）。
    /// 查不到返回 0（water）并 warn。空串（无药水）调用方应先判 present，不进此函数。
    [[nodiscard]] u32 toJavaRegistryId(std::string_view potionLocation) const;

    /// Java registry id → 项目 Potion 资源位置字符串（收侧）。查不到返回空串并 warn。
    [[nodiscard]] std::string fromJavaRegistryId(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /// 已映射的药水数（诊断用）。
    [[nodiscard]] size_t matchedCount() const noexcept { return m_javaToName.size(); }

private:
    bool m_initialized = false;
    /// "minecraft:water" → vanilla registry id
    std::unordered_map<std::string, u32> m_nameToJava;
    /// vanilla registry id → "minecraft:water"
    std::unordered_map<u32, std::string> m_javaToName;
};

} // namespace mc::network::backend::java
