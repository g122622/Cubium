#pragma once

#include "../../core/Types.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace mc {

/**
 * @brief 属性接口基类 - 提供类型擦除的接口
 *
 * 所有属性类型（BooleanProperty、IntegerProperty、EnumProperty等）
 * 都通过此接口提供统一的访问方式。
 *
 * 参考: net.minecraft.state.Property
 */
class IProperty {
public:
    virtual ~IProperty() = default;

    /**
     * @brief 获取属性名称
     * @return 属性名称字符串
     */
    [[nodiscard]] virtual const String& name() const = 0;

    /**
     * @brief 获取允许的值的数量
     * @return 值的数量
     */
    [[nodiscard]] virtual size_t valueCount() const = 0;

    /**
     * @brief 将值索引转换为字符串表示
     * @param index 值索引（0到valueCount()-1）
     * @return 字符串表示
     */
    [[nodiscard]] virtual String valueToString(size_t index) const = 0;

    /**
     * @brief 解析字符串为值索引
     * @param str 字符串表示
     * @return 值索引，如果解析失败返回nullopt
     */
    [[nodiscard]] virtual std::optional<size_t> parseValue(StringView str) const = 0;

    /**
     * @brief 计算哈希值
     * @return 属性的哈希值
     */
    [[nodiscard]] virtual size_t hashCode() const = 0;

    /**
     * @brief 比较两个属性是否相等
     * @param other 另一个属性
     * @return 是否相等
     */
    [[nodiscard]] virtual bool equals(const IProperty& other) const = 0;

    /**
     * @brief 获取属性的类型信息名称（用于调试）
     * @return 类型名称
     */
    [[nodiscard]] virtual const char* typeName() const = 0;
};

} // namespace mc
