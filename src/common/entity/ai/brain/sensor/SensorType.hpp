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

#include "Sensor.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace sensor {

/**
 * @brief 传感器类型
 *
 * 用于注册和创建传感器实例
 */
template <typename E>
class SensorType {
public:
    using Factory = std::function<std::unique_ptr<Sensor<E>>()>;

    explicit SensorType(const std::string& name, Factory factory)
        : m_name(name)
        , m_factory(std::move(factory))
    {}

    [[nodiscard]] const std::string& getName() const { return m_name; }

    [[nodiscard]] std::unique_ptr<Sensor<E>> create() const { return m_factory(); }

private:
    std::string m_name;
    Factory m_factory;
};

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
