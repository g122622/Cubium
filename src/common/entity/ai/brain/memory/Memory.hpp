#pragma once

#include "../../../../core/Types.hpp"
#include <limits>
#include <optional>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief 内存模块 - 存储带TTL的记忆
 *
 * @tparam T 记忆的数据类型
 *
 * 参考 MC 1.16.5 Memory
 */
template <typename T>
class Memory {
public:
    /**
     * @brief 构造一个带TTL的记忆
     * @param value 记忆值
     * @param ttl 存活时间(ticks)，Long.MAX_VALUE表示永不过期
     */
    Memory(const T& value, i64 ttl = std::numeric_limits<i64>::max())
        : m_value(value)
        , m_ttl(ttl)
    {}

    /**
     * @brief 创建一个永不过期的记忆
     */
    static Memory<T> permanent(const T& value) { return Memory<T>(value, std::numeric_limits<i64>::max()); }

    /**
     * @brief 创建一个带TTL的记忆
     */
    static Memory<T> timed(const T& value, i64 ttl) { return Memory<T>(value, ttl); }

    /**
     * @brief 获取记忆值
     */
    [[nodiscard]] const T& getValue() const { return m_value; }

    /**
     * @brief 获取可变记忆值
     */
    T& getValue() { return m_value; }

    /**
     * @brief 每tick调用，减少TTL
     */
    void tick()
    {
        if (hasTTL()) {
            m_ttl--;
        }
    }

    /**
     * @brief 检查记忆是否已过期
     */
    [[nodiscard]] bool isExpired() const { return m_ttl <= 0; }

    /**
     * @brief 检查是否有TTL限制
     */
    [[nodiscard]] bool hasTTL() const { return m_ttl != std::numeric_limits<i64>::max(); }

    /**
     * @brief 获取剩余TTL
     */
    [[nodiscard]] i64 getTTL() const { return m_ttl; }

private:
    T m_value;
    i64 m_ttl;
};

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
