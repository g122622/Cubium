#pragma once

#include "../../core/Types.hpp"
#include <string>
#include <memory>
#include <vector>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性修改器操作类型
 *
 * 定义属性修改器如何影响基础值
 *
 * 参考 MC 1.16.5 Operation
 */
enum class Operation : u8 {
    /**
     * @brief 加法
     * 将修改器值加到基础值上
     * 例如：基础值 10 + 修改器 5 = 15
     */
    Addition = 0,

    /**
     * @brief 乘法基础
     * 基于基础值进行乘法，然后加到结果上
     * 例如：基础值 10，修改器 0.5 -> 10 + (10 * 0.5) = 15
     */
    MultiplyBase = 1,

    /**
     * @brief 乘法总计
     * 基于当前总计值进行乘法
     * 例如：当前值 15，修改器 0.1 -> 15 * 1.1 = 16.5
     */
    MultiplyTotal = 2
};

/**
 * @brief 属性修改器
 *
 * 用于临时或永久修改属性值。
 * 每个修改器有唯一的ID、操作类型和值。
 *
 * 参考 MC 1.16.5 AttributeModifier
 */
class AttributeModifier {
public:
    /**
     * @brief 构造属性修改器
     * @param id 修改器唯一ID
     * @param name 修改器名称
     * @param amount 修改量
     * @param operation 操作类型
     */
    AttributeModifier(const std::string& id, const std::string& name, f64 amount, Operation operation)
        : m_id(id)
        , m_name(name)
        , m_amount(amount)
        , m_operation(operation)
    {}

    /**
     * @brief 获取修改器ID
     */
    [[nodiscard]] const std::string& id() const { return m_id; }

    /**
     * @brief 获取修改器名称
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief 获取修改量
     */
    [[nodiscard]] f64 amount() const { return m_amount; }

    /**
     * @brief 获取操作类型
     */
    [[nodiscard]] Operation operation() const { return m_operation; }

    /**
     * @brief 设置修改量
     */
    void setAmount(f64 amount) { m_amount = amount; }

    /**
     * @brief 比较操作符
     */
    bool operator==(const AttributeModifier& other) const {
        return m_id == other.m_id;
    }

    bool operator!=(const AttributeModifier& other) const {
        return m_id != other.m_id;
    }

private:
    std::string m_id;
    std::string m_name;
    f64 m_amount;
    Operation m_operation;
};

} // namespace attribute
} // namespace entity
} // namespace mc
