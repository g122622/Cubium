#pragma once

#include "Sensor.hpp"
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

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
        : m_name(name), m_factory(std::move(factory)) {}

    [[nodiscard]] const std::string& getName() const { return m_name; }

    [[nodiscard]] std::unique_ptr<Sensor<E>> create() const {
        return m_factory();
    }

private:
    std::string m_name;
    Factory m_factory;
};

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
