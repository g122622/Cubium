#pragma once

#include "Property.hpp"
#include "../../util/Direction.hpp"
#include <functional>

namespace mc {

/**
 * @brief 枚举属性
 *
 * 表示一个枚举类型的方块状态属性。
 * 枚举类型必须提供 toString() 和 fromName() 方法。
 *
 * 参考: net.minecraft.state.EnumProperty
 *
 * 注意:
 * - 枚举类型需要特化 EnumProperty<E>::Traits 或提供 toString/fromName 方法
 * - 属性名称应该遵循MC命名约定
 */
template<typename E>
class EnumProperty : public Property<E> {
public:
    /**
     * @brief 枚举值序列化特征
     *
     * 特化此模板为枚举类型提供字符串转换
     */
    struct Traits {
        static String toString(const E& value);
        static std::optional<E> fromName(StringView name);
    };

    /**
     * @brief 创建枚举属性
     * @param name 属性名称
     * @param values 允许的枚举值列表
     * @return 属性实例
     */
    [[nodiscard]] static std::unique_ptr<EnumProperty<E>> create(const String& name, const std::vector<E>& values) {
        return std::unique_ptr<EnumProperty<E>>(new EnumProperty<E>(name, values));
    }

    /**
     * @brief 将枚举值转换为字符串
     */
    [[nodiscard]] String valueToString(const E& value) const override {
        return Traits::toString(value);
    }

    /**
     * @brief 解析字符串为枚举值
     */
    [[nodiscard]] std::optional<E> parse(StringView str) const override {
        auto value = Traits::fromName(str);
        if (value && this->indexOf(*value)) {
            return value;
        }
        return std::nullopt;
    }

    /**
     * @brief 计算哈希值
     */
    [[nodiscard]] size_t hashCode() const override {
        size_t h = std::hash<String>{}(this->m_name);
        h ^= (std::hash<String>{}("EnumProperty") << 1);
        for (const auto& value : this->m_values) {
            h ^= std::hash<size_t>{}(static_cast<size_t>(value));
        }
        return h;
    }

    /**
     * @brief 获取类型名称
     */
    [[nodiscard]] const char* typeName() const override {
        return "EnumProperty";
    }

protected:
    EnumProperty(const String& name, const std::vector<E>& values)
        : Property<E>(name, values) {
    }
};

// ============================================================================
// 枚举特征特化 - Direction
// ============================================================================

template<>
struct EnumProperty<Direction>::Traits {
    static String toString(const Direction& value) {
        return Directions::toString(value);
    }
    static std::optional<Direction> fromName(StringView name) {
        return Directions::fromName(name);
    }
};

// ============================================================================
// 枚举特征特化 - Axis
// ============================================================================

template<>
struct EnumProperty<Axis>::Traits {
    static String toString(const Axis& value) {
        return Axes::toString(value);
    }
    static std::optional<Axis> fromName(StringView name) {
        return Axes::fromName(name);
    }
};

} // namespace mc
