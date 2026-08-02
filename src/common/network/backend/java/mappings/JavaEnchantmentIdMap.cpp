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

#include "common/network/backend/java/mappings/JavaEnchantmentIdMap.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace mc::network::backend::java {

namespace {

/// vanilla 1.21.11 Enchantments.bootstrap() 内 register 调用顺序（wire id 权威源，0-based）。
/// 注意 LUNGE 在 bootstrap 第 33 位（非常量声明第 41 位），表须按 bootstrap 序。
/// 每项 = {vanilla registry id, vanilla name}。
struct VanillaEnchantmentEntry {
    u32 vanillaId;
    const char* vanillaName;
};

constexpr VanillaEnchantmentEntry kVanillaEnchantments[] = {
    {0, "minecraft:protection"},
    {1, "minecraft:fire_protection"},
    {2, "minecraft:feather_falling"},
    {3, "minecraft:blast_protection"},
    {4, "minecraft:projectile_protection"},
    {5, "minecraft:respiration"},
    {6, "minecraft:aqua_affinity"},
    {7, "minecraft:thorns"},
    {8, "minecraft:depth_strider"},
    {9, "minecraft:frost_walker"},
    {10, "minecraft:binding_curse"},
    {11, "minecraft:soul_speed"},
    {12, "minecraft:swift_sneak"},
    {13, "minecraft:sharpness"},
    {14, "minecraft:smite"},
    {15, "minecraft:bane_of_arthropods"},
    {16, "minecraft:knockback"},
    {17, "minecraft:fire_aspect"},
    {18, "minecraft:looting"},
    {19, "minecraft:sweeping_edge"},
    {20, "minecraft:efficiency"},
    {21, "minecraft:silk_touch"},
    {22, "minecraft:unbreaking"},
    {23, "minecraft:fortune"},
    {24, "minecraft:power"},
    {25, "minecraft:punch"},
    {26, "minecraft:flame"},
    {27, "minecraft:infinity"},
    {28, "minecraft:luck_of_the_sea"},
    {29, "minecraft:lure"},
    {30, "minecraft:loyalty"},
    {31, "minecraft:impaling"},
    {32, "minecraft:riptide"},
    {33, "minecraft:lunge"},
    {34, "minecraft:channeling"},
    {35, "minecraft:multishot"},
    {36, "minecraft:quick_charge"},
    {37, "minecraft:piercing"},
    {38, "minecraft:density"},
    {39, "minecraft:breach"},
    {40, "minecraft:wind_burst"},
    {41, "minecraft:mending"},
    {42, "minecraft:vanishing_curse"},
};

constexpr size_t kVanillaEnchantmentCount = std::size(kVanillaEnchantments);

} // namespace

// ============================================================================
// 单例
// ============================================================================

JavaEnchantmentIdMap& JavaEnchantmentIdMap::instance()
{
    static JavaEnchantmentIdMap s_instance;
    return s_instance;
}

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaEnchantmentIdMap::initialize()
{
    m_initialized = false;
    m_nameToJava.clear();
    m_javaToName.clear();

    for (size_t i = 0; i < kVanillaEnchantmentCount; ++i) {
        const auto& entry = kVanillaEnchantments[i];
        m_nameToJava[entry.vanillaName] = entry.vanillaId;
        m_javaToName[entry.vanillaId] = entry.vanillaName;
    }

    spdlog::info("JavaEnchantmentIdMap: matched {} enchantments", m_javaToName.size());

    m_initialized = true;
    return {};
}

u32 JavaEnchantmentIdMap::toJavaRegistryId(std::string_view enchantmentId) const
{
    if (!m_initialized) {
        // 防御：漏初始化时自动建表（幂等），避免全发 id 0。
        (void)const_cast<JavaEnchantmentIdMap*>(this)->initialize();
    }
    if (const auto it = m_nameToJava.find(std::string(enchantmentId)); it != m_nameToJava.end()) {
        return it->second;
    }
    spdlog::warn("JavaEnchantmentIdMap: toJavaRegistryId miss for enchantment {}", enchantmentId);
    return 0; // protection 兜底
}

std::string JavaEnchantmentIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    if (!m_initialized) {
        return std::string{};
    }
    if (const auto it = m_javaToName.find(javaRegistryId); it != m_javaToName.end()) {
        return it->second;
    }
    spdlog::warn("JavaEnchantmentIdMap: fromJavaRegistryId miss for javaRegistryId={}", javaRegistryId);
    return std::string{}; // 空串兜底
}

} // namespace mc::network::backend::java
