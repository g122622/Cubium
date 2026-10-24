#pragma once

#include "common/util/assert/AssertAll.hpp"

#include <cstddef>
#include <memory>
#include <utility>

namespace mc {

/**
 * @brief 可跨 tick 保存的非拥有实体引用
 *
 * 实体逻辑移除或析构时生命周期标记失效，get() 随后返回空指针，
 * 无需访问实体本身，也不依赖世界或 ECS registry 继续存活。
 * 与实体行为一样，只在所属游戏线程上读写；不延长实体的生命周期。
 */
template <class T>
class EntityReference {
public:
    /** @brief 构造空引用 */
    EntityReference() noexcept = default;

    /** @brief 从当前有效的实体指针构造引用 */
    EntityReference(T* entity)
    {
        *this = entity;
    }

    /** @brief 绑定当前有效的实体指针，空指针表示清除引用 */
    EntityReference& operator=(T* entity)
    {
        auto lifetime = entity != nullptr ? entity->referenceLifetime() : std::weak_ptr<const void>{};
        m_lifetime = std::move(lifetime);
        m_entity = entity;
        return *this;
    }

    /** @brief 取得尚未移除或析构的实体，失效引用返回空指针 */
    [[nodiscard]] T* get() const noexcept
    {
        return m_lifetime.expired() ? nullptr : m_entity;
    }

    /** @brief 判断引用是否仍有效 */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return get() != nullptr;
    }

    /** @brief 判断引用是否为空或已经失效 */
    [[nodiscard]] bool operator==(std::nullptr_t) const noexcept
    {
        return get() == nullptr;
    }

    /** @brief 访问有效引用，调用方必须先检查引用 */
    [[nodiscard]] T* operator->() const
    {
        T* entity = get();
        MC_ASSERT_RELEASE(entity != nullptr);
        return entity;
    }

    /** @brief 解引用有效引用，调用方必须先检查引用 */
    [[nodiscard]] T& operator*() const
    {
        return *operator->();
    }

private:
    T* m_entity = nullptr;
    std::weak_ptr<const void> m_lifetime;
};

} // namespace mc
