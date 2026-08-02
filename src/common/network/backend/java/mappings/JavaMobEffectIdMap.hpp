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
#include "common/entity/effect/EffectType.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace mc::network::backend::java {

/**
 * @brief 项目 EffectType ↔ Java mob_effect registry id 双向映射（Java 协议对齐层）
 *
 * MobEffectInstance wire（1.21.11 MobEffectInstance.STREAM_CODEC）的 effect 字段是 Java
 * `minecraft:mob_effect` 注册表（`BuiltInRegistries.MOB_EFFECT`）的 holder id（wire 上是纯
 * VarInt(registryId)）。该注册表未由本项目 RegistryDataBuilder 同步（不在 23 个
 * SYNCHRONIZED_REGISTRIES，mob_effect 是静态注册表，由客户端 MobEffects.bootstrap 在启动期
 * 注册），真 Java 客户端使用其内置 vanilla core 包注册表，id 为 vanilla 1.21.11
 * `MobEffects.java` 静态字段声明顺序（speed=0/slowness=1/…/breath_of_the_nautilus=39，共 40 项）。
 *
 * 项目 `EffectType` 是业务枚举（Speed=1…BreathOfTheNautilus=40），其数字值与 vanilla registry
 * id 顺序存在错位（darkness 在项目末尾、vanilla 中部；wind_charged↔raid_omen 互换），不能简单
 * 用 `EffectType-1` 换算。故本表按资源名（"minecraft:speed"）建立双向映射，与 JavaItemIdMap
 * 用 itemLocation().toString() 作 key 同构。
 *
 * 数据源：vanilla `MobEffects.java` 静态字段声明顺序（手写固定表，39→40 项，0-based）。mob_effect
 * 条目少且固定，无需 items 那样的预烘焙脚本，直接编码进 .cpp 静态表。
 *
 * mob_effect 是 holderRegistry（纯 VarInt id），与 DimensionType/MapDecoration 同模式；
 * 客户端按 vanilla bootstrap 顺序分配 id，服务端发 id 必须与该顺序一致——本表即保证此一致。
 *
 * 边界收口于 MobEffectInstance codec（PotionContents.customEffects 内的 List<MobEffectInstance>），
 * effect 业务子系统零感知。
 */
class JavaMobEffectIdMap {
public:
    static JavaMobEffectIdMap& instance();

    JavaMobEffectIdMap() = default;
    ~JavaMobEffectIdMap() = default;
    JavaMobEffectIdMap(const JavaMobEffectIdMap&) = delete;
    JavaMobEffectIdMap& operator=(const JavaMobEffectIdMap&) = delete;

    /// 构建双向映射。可重复调用（幂等，先清空再重建）。
    [[nodiscard]] Result<void> initialize();

    /// 项目 EffectType → Java mob_effect registry id（发侧）。查不到返回 0（speed）并 warn。
    [[nodiscard]] u32 toJavaRegistryId(::mc::entity::effect::EffectType type) const;

    /// Java mob_effect registry id → 项目 EffectType（收侧）。查不到返回 Speed 并 warn。
    [[nodiscard]] ::mc::entity::effect::EffectType fromJavaRegistryId(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /// 已映射的效果类型数（诊断用）。
    [[nodiscard]] size_t matchedCount() const noexcept { return m_javaToInternal.size(); }

private:
    bool m_initialized = false;
    /// "minecraft:speed" → vanilla registry id
    std::unordered_map<std::string, u32> m_nameToJava;
    /// vanilla registry id → 项目 EffectType
    std::unordered_map<u32, ::mc::entity::effect::EffectType> m_javaToInternal;
};

} // namespace mc::network::backend::java
