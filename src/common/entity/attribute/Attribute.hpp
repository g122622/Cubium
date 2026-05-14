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

#include "../../core/Types.hpp"
#include <functional>
#include <memory>
#include <string>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性基类
 *
 * 定义一个可修改的属性，如最大生命值、移动速度等。
 * 每个属性有唯一的注册名称、默认值和范围限制。
 *
 * 参考 MC 1.16.5 Attribute
 */
class Attribute {
public:
    /**
     * @brief 构造属性
     * @param registryName 注册名称（如 "generic.max_health"）
     * @param defaultValue 默认值
     * @param minValue 最小值
     * @param maxValue 最大值
     */
    Attribute(const std::string& registryName, f64 defaultValue, f64 minValue, f64 maxValue)
        : m_registryName(registryName)
        , m_defaultValue(defaultValue)
        , m_minValue(minValue)
        , m_maxValue(maxValue)
    {}

    virtual ~Attribute() = default;

    /**
     * @brief 获取注册名称
     */
    [[nodiscard]] const std::string& registryName() const { return m_registryName; }

    /**
     * @brief 获取默认值
     */
    [[nodiscard]] f64 defaultValue() const { return m_defaultValue; }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] f64 minValue() const { return m_minValue; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] f64 maxValue() const { return m_maxValue; }

    /**
     * @brief 是否应该同步到客户端
     */
    [[nodiscard]] virtual bool shouldSync() const { return true; }

    /**
     * @brief 克隆属性
     */
    [[nodiscard]] virtual std::unique_ptr<Attribute> clone() const
    {
        return std::make_unique<Attribute>(m_registryName, m_defaultValue, m_minValue, m_maxValue);
    }

    /**
     * @brief 比较操作符
     */
    bool operator==(const Attribute& other) const { return m_registryName == other.m_registryName; }

    bool operator!=(const Attribute& other) const { return m_registryName != other.m_registryName; }

private:
    std::string m_registryName;
    f64 m_defaultValue;
    f64 m_minValue;
    f64 m_maxValue;
};

} // namespace attribute
} // namespace entity
} // namespace mc
