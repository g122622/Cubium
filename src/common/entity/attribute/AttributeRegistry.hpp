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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "Attribute.hpp"
#include "Attributes.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性注册表
 *
 * 集中管理所有已知属性定义，提供按名称查询属性信息的能力。
 * 消除了在各处硬编码属性名和范围的重复数据问题。
 *
 * 注册表在首次访问时通过 Attributes 命名空间的工厂函数自动初始化，
 * 确保属性定义数据只存在一份（Single Source of Truth）。
 *
 * 参考 MC Java 版 BuiltInRegistries.ATTRIBUTE 注册表。
 */
class AttributeRegistry {
public:
    /**
     * @brief 获取注册表单例
     * @return 注册表引用
     */
    [[nodiscard]] static AttributeRegistry& instance()
    {
        static AttributeRegistry registry;
        return registry;
    }

    /**
     * @brief 注册一个属性定义
     * @param attribute 属性定义
     * @return 是否成功注册（已存在则返回 false）
     */
    bool registerAttribute(const Attribute& attribute)
    {
        const std::string& name = attribute.registryName();
        if (m_attributes.find(name) != m_attributes.end()) {
            return false;
        }
        m_attributes.emplace(name, attribute.clone());
        return true;
    }

    /**
     * @brief 检查属性是否已知（已注册）
     * @param name 属性注册名称
     * @return 是否已知
     */
    [[nodiscard]] bool isKnown(const std::string& name) const { return m_attributes.find(name) != m_attributes.end(); }

    /**
     * @brief 获取属性定义
     * @param name 属性注册名称
     * @return 属性定义指针，不存在返回 nullptr
     */
    [[nodiscard]] const Attribute* getAttribute(const std::string& name) const
    {
        auto it = m_attributes.find(name);
        return it != m_attributes.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 获取属性的默认值
     * @param name 属性注册名称
     * @param fallback 找不到时返回的默认值
     * @return 默认值
     */
    [[nodiscard]] f64 getDefaultValue(const std::string& name, f64 fallback = 0.0) const
    {
        auto* attr = getAttribute(name);
        return attr ? attr->defaultValue() : fallback;
    }

    /**
     * @brief 获取属性的值范围
     * @param name 属性注册名称
     * @return {最小值, 最大值}，不存在返回 {0.0, 1024.0}
     */
    [[nodiscard]] std::pair<f64, f64> getRange(const std::string& name) const
    {
        auto* attr = getAttribute(name);
        if (attr != nullptr) {
            return {attr->minValue(), attr->maxValue()};
        }
        return {0.0, 1024.0};
    }

    /**
     * @brief 获取所有已注册属性的名称列表
     * @return 属性名称列表
     */
    [[nodiscard]] std::vector<std::string> getAllNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_attributes.size());
        for (const auto& [name, _] : m_attributes) {
            names.push_back(name);
        }
        // 按字母序排序以保证确定性
        std::sort(names.begin(), names.end());
        return names;
    }

    /**
     * @brief 获取已注册属性数量
     */
    [[nodiscard]] size_t size() const { return m_attributes.size(); }

    /**
     * @brief 规范化属性名称
     *
     * 自动为已知短名称添加 "generic." 前缀。
     * 例如：输入 "max_health" 返回 "generic.max_health"。
     * 如果名称已包含命名空间前缀（如 "generic."、"horse."、"zombie."、"forge."）则不做修改。
     * 如果名称不是已知属性，也原样返回。
     *
     * @param name 原始属性名称
     * @return 规范化后的属性名称
     */
    [[nodiscard]] std::string normalizeName(const std::string& name) const
    {
        // 移除 minecraft: 前缀
        constexpr std::string_view minecraftPrefix = "minecraft:";
        std::string normalized = name;
        if (normalized.starts_with(minecraftPrefix)) {
            normalized = normalized.substr(minecraftPrefix.size());
        }

        // 已包含已知命名空间前缀，直接返回
        if (normalized.starts_with("generic.") || normalized.starts_with("horse.") ||
            normalized.starts_with("zombie.") || normalized.starts_with("forge.")) {
            return normalized;
        }

        // 尝试添加 generic. 前缀看是否匹配已知属性
        std::string withGeneric = "generic." + normalized;
        if (isKnown(withGeneric)) {
            return withGeneric;
        }

        // 尝试添加 horse. 前缀
        std::string withHorse = "horse." + normalized;
        if (isKnown(withHorse)) {
            return withHorse;
        }

        // 尝试添加 zombie. 前缀
        std::string withZombie = "zombie." + normalized;
        if (isKnown(withZombie)) {
            return withZombie;
        }

        // 尝试添加 forge. 前缀
        std::string withForge = "forge." + normalized;
        if (isKnown(withForge)) {
            return withForge;
        }

        // 无法匹配，原样返回
        return normalized;
    }

private:
    AttributeRegistry() { _registerBuiltinAttributes(); }

    /**
     * @brief 注册所有内置属性定义
     *
     * 通过 Attributes 命名空间的工厂函数创建属性定义并注册，
     * 确保属性信息只存在一份，避免硬编码数据不同步。
     */
    void _registerBuiltinAttributes()
    {
        // MC 原版属性
        registerAttribute(*Attributes::maxHealth());
        registerAttribute(*Attributes::followRange());
        registerAttribute(*Attributes::knockbackResistance());
        registerAttribute(*Attributes::movementSpeed());
        registerAttribute(*Attributes::flyingSpeed());
        registerAttribute(*Attributes::attackDamage());
        registerAttribute(*Attributes::attackKnockback());
        registerAttribute(*Attributes::attackSpeed());
        registerAttribute(*Attributes::armor());
        registerAttribute(*Attributes::armorToughness());
        registerAttribute(*Attributes::luck());
        registerAttribute(*Attributes::maxAbsorption());
        registerAttribute(*Attributes::breathMax());
        registerAttribute(*Attributes::jumpBoost());
        registerAttribute(*Attributes::horseJumpStrength());
        registerAttribute(*Attributes::zombieSpawnReinforcements());

        // Forge 扩展属性
        registerAttribute(*Attributes::entityGravity());
        registerAttribute(*Attributes::swimSpeed());

        // MC 1.21+ 新增属性
        registerAttribute(*Attributes::movementEfficiency());
        registerAttribute(*Attributes::blockInteractionRange());
        registerAttribute(*Attributes::entityInteractionRange());
        registerAttribute(*Attributes::safeFallDistance());
        registerAttribute(*Attributes::fallDamageMultiplier());
        registerAttribute(*Attributes::oxygenBonus());
    }

    std::unordered_map<std::string, std::unique_ptr<Attribute>> m_attributes;
};

} // namespace attribute
} // namespace entity
} // namespace mc
