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

#include "Attribute.hpp"
#include "AttributeInstance.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性映射表
 *
 * 管理实体的所有属性实例。
 * 提供属性的注册、获取和修改功能。
 */
class AttributeMap {
public:
    AttributeMap() = default;

    /**
     * @brief 注册属性
     * @param attribute 属性定义
     * @return 是否成功注册
     */
    bool registerAttribute(const Attribute& attribute)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const std::string& name = attribute.registryName();
        if (m_instances.find(name) != m_instances.end()) {
            return false;
        }
        m_instances.emplace(name, std::make_unique<AttributeInstance>(attribute));
        return true;
    }

    /**
     * @brief 获取属性实例
     * @param name 属性名称
     * @return 属性实例指针，不存在返回nullptr
     */
    AttributeInstance* getInstance(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_instances.find(name);
        if (it != m_instances.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    /**
     * @brief 获取属性实例（const版本）
     * @param name 属性名称
     * @return 属性实例指针，不存在返回nullptr
     */
    [[nodiscard]] const AttributeInstance* getInstance(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_instances.find(name);
        if (it != m_instances.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    /**
     * @brief 获取属性值
     * @param name 属性名称
     * @param defaultValue 默认值（属性不存在时返回）
     * @return 属性值
     */
    [[nodiscard]] f64 getValue(const std::string& name, f64 defaultValue = 0.0) const
    {
        const AttributeInstance* instance = getInstance(name);
        return instance ? instance->getValue() : defaultValue;
    }

    /**
     * @brief 获取属性基础值
     * @param name 属性名称
     * @param defaultValue 默认值（属性不存在时返回）
     * @return 基础值
     */
    [[nodiscard]] f64 getBaseValue(const std::string& name, f64 defaultValue = 0.0) const
    {
        const AttributeInstance* instance = getInstance(name);
        return instance ? instance->baseValue() : defaultValue;
    }

    /**
     * @brief 设置属性基础值
     * @param name 属性名称
     * @param value 新的基础值
     * @return 是否成功设置
     */
    bool setBaseValue(const std::string& name, f64 value)
    {
        AttributeInstance* instance = getInstance(name);
        if (instance) {
            instance->setBaseValue(value);
            return true;
        }
        return false;
    }

    /**
     * @brief 添加修改器到属性
     * @param attributeName 属性名称
     * @param modifier 修改器
     * @return 是否成功添加
     */
    bool addModifier(const std::string& attributeName, const AttributeModifier& modifier)
    {
        AttributeInstance* instance = getInstance(attributeName);
        if (instance) {
            instance->addModifier(modifier);
            return true;
        }
        return false;
    }

    /**
     * @brief 从属性移除修改器
     * @param attributeName 属性名称
     * @param modifierId 修改器ID
     * @return 是否成功移除
     */
    bool removeModifier(const std::string& attributeName, const std::string& modifierId)
    {
        AttributeInstance* instance = getInstance(attributeName);
        if (instance) {
            return instance->removeModifier(modifierId);
        }
        return false;
    }

    /**
     * @brief 重置属性基础值为默认值
     * @param name 属性名称
     * @return 是否成功重置（属性不存在返回false）
     */
    bool resetBaseValue(const std::string& name)
    {
        AttributeInstance* instance = getInstance(name);
        if (instance) {
            instance->setBaseValue(instance->attribute().defaultValue());
            return true;
        }
        return false;
    }

    /**
     * @brief 检查属性是否有指定修饰器
     * @param attributeName 属性名称
     * @param modifierId 修饰器ID
     * @return 是否存在该修饰器
     */
    [[nodiscard]] bool hasModifier(const std::string& attributeName, const std::string& modifierId) const
    {
        const AttributeInstance* instance = getInstance(attributeName);
        return instance ? instance->hasModifier(modifierId) : false;
    }

    /**
     * @brief 获取修饰器的值
     * @param attributeName 属性名称
     * @param modifierId 修饰器ID
     * @param defaultValue 默认值（修饰器不存在时返回）
     * @return 修饰器的amount值
     */
    [[nodiscard]] f64 getModifierValue(
        const std::string& attributeName, const std::string& modifierId, f64 defaultValue = 0.0) const
    {
        const AttributeInstance* instance = getInstance(attributeName);
        if (instance) {
            const AttributeModifier* modifier = instance->getModifier(modifierId);
            return modifier ? modifier->amount() : defaultValue;
        }
        return defaultValue;
    }

    /**
     * @brief 检查是否有属性
     * @param name 属性名称
     */
    [[nodiscard]] bool hasAttribute(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_instances.find(name) != m_instances.end();
    }

    /**
     * @brief 获取所有属性实例
     */
    [[nodiscard]] const std::unordered_map<std::string, std::unique_ptr<AttributeInstance>>& allInstances() const
    {
        return m_instances;
    }

    /**
     * @brief 清除所有属性
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_instances.clear();
    }

    /**
     * @brief 从另一个属性映射表复制属性值
     * @param other 源属性映射表
     */
    void copyFrom(const AttributeMap& other)
    {
        // 使用 scoped_lock 避免死锁（C++20 死锁避免算法）
        std::scoped_lock lock(m_mutex, other.m_mutex);

        for (const auto& [name, instance] : other.m_instances) {
            auto it = m_instances.find(name);
            if (it != m_instances.end()) {
                it->second->setBaseValue(instance->baseValue());
                // 清除现有修改器
                it->second->clearModifiers();
                // 复制修改器
                for (const auto& modifier : instance->modifiers()) {
                    it->second->addModifier(modifier);
                }
            }
        }
    }

private:
    std::unordered_map<std::string, std::unique_ptr<AttributeInstance>> m_instances;
    mutable std::mutex m_mutex;
};

} // namespace attribute
} // namespace entity
} // namespace mc
