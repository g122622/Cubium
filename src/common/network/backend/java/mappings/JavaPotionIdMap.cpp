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

#include "common/network/backend/java/mappings/JavaPotionIdMap.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace mc::network::backend::java {

namespace {

/// vanilla 1.21.11 Potions.java 静态字段声明顺序（wire id 权威源，0-based，共 47 项）。
/// vanilla 不含 "empty"（empty 是 PotionContents 语义非注册项），项目 PotionRegistry 多注册
/// 的 empty 在本表 miss，调用方按"无药水"处理。
struct VanillaPotionEntry {
    u32 vanillaId;
    const char* vanillaName;
};

constexpr VanillaPotionEntry kVanillaPotions[] = {
    {0, "minecraft:water"},
    {1, "minecraft:mundane"},
    {2, "minecraft:thick"},
    {3, "minecraft:awkward"},
    {4, "minecraft:night_vision"},
    {5, "minecraft:long_night_vision"},
    {6, "minecraft:invisibility"},
    {7, "minecraft:long_invisibility"},
    {8, "minecraft:leaping"},
    {9, "minecraft:long_leaping"},
    {10, "minecraft:strong_leaping"},
    {11, "minecraft:fire_resistance"},
    {12, "minecraft:long_fire_resistance"},
    {13, "minecraft:swiftness"},
    {14, "minecraft:long_swiftness"},
    {15, "minecraft:strong_swiftness"},
    {16, "minecraft:slowness"},
    {17, "minecraft:long_slowness"},
    {18, "minecraft:strong_slowness"},
    {19, "minecraft:turtle_master"},
    {20, "minecraft:long_turtle_master"},
    {21, "minecraft:strong_turtle_master"},
    {22, "minecraft:water_breathing"},
    {23, "minecraft:long_water_breathing"},
    {24, "minecraft:healing"},
    {25, "minecraft:strong_healing"},
    {26, "minecraft:harming"},
    {27, "minecraft:strong_harming"},
    {28, "minecraft:poison"},
    {29, "minecraft:long_poison"},
    {30, "minecraft:strong_poison"},
    {31, "minecraft:regeneration"},
    {32, "minecraft:long_regeneration"},
    {33, "minecraft:strong_regeneration"},
    {34, "minecraft:strength"},
    {35, "minecraft:long_strength"},
    {36, "minecraft:strong_strength"},
    {37, "minecraft:weakness"},
    {38, "minecraft:long_weakness"},
    {39, "minecraft:luck"},
    {40, "minecraft:slow_falling"},
    {41, "minecraft:long_slow_falling"},
    {42, "minecraft:wind_charged"},
    {43, "minecraft:weaving"},
    {44, "minecraft:oozing"},
    {45, "minecraft:infested"},
};

constexpr size_t kVanillaPotionCount = std::size(kVanillaPotions);

} // namespace

// ============================================================================
// 单例
// ============================================================================

JavaPotionIdMap& JavaPotionIdMap::instance()
{
    static JavaPotionIdMap s_instance;
    return s_instance;
}

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaPotionIdMap::initialize()
{
    m_initialized = false;
    m_nameToJava.clear();
    m_javaToName.clear();

    for (size_t i = 0; i < kVanillaPotionCount; ++i) {
        const auto& entry = kVanillaPotions[i];
        m_nameToJava[entry.vanillaName] = entry.vanillaId;
        m_javaToName[entry.vanillaId] = entry.vanillaName;
    }

    spdlog::info("JavaPotionIdMap: matched {} potions", m_javaToName.size());

    m_initialized = true;
    return {};
}

u32 JavaPotionIdMap::toJavaRegistryId(std::string_view potionLocation) const
{
    if (!m_initialized) {
        // 防御：漏初始化时自动建表（幂等），避免全发 id 0。
        (void)const_cast<JavaPotionIdMap*>(this)->initialize();
    }
    if (const auto it = m_nameToJava.find(std::string(potionLocation)); it != m_nameToJava.end()) {
        return it->second;
    }
    spdlog::warn("JavaPotionIdMap: toJavaRegistryId miss for potion {}", potionLocation);
    return 0; // water 兜底
}

std::string JavaPotionIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    if (!m_initialized) {
        return {};
    }
    if (const auto it = m_javaToName.find(javaRegistryId); it != m_javaToName.end()) {
        return it->second;
    }
    spdlog::warn("JavaPotionIdMap: fromJavaRegistryId miss for javaRegistryId={}", javaRegistryId);
    return {}; // 空串兜底（调用方按"无药水"处理）
}

} // namespace mc::network::backend::java
