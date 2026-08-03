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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::advancement {

// 前向声明
class Advancement;
using AdvancementPtr = std::shared_ptr<const Advancement>;

} // namespace mc::advancement

// 前向声明 PlayerAdvancements（在 mc::server 命名空间）
namespace mc::server {
class PlayerAdvancements;
}

namespace mc::advancement {

/**
 * @brief 触发器实例基类
 *
 * 所有触发器实例的基类，包含触发器ID。
 * 触发器实例包含在成就条件中，定义了触发条件的具体参数。
 */
class ICriterionInstance {
public:
    virtual ~ICriterionInstance() = default;

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] virtual ResourceLocation getId() const = 0;

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] virtual nlohmann::json toJson() const = 0;
};

/**
 * @brief 触发器实例模板
 */
template <typename T>
class CriterionInstance : public ICriterionInstance {
public:
    using InstanceType = T;

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(T::TRIGGER_ID); }

    [[nodiscard]] nlohmann::json toJson() const override
    {
        nlohmann::json json;
        json["trigger"] = getId().toString();

        nlohmann::json conditions = static_cast<const T*>(this)->conditionsToJson();
        if (!conditions.is_null() && !conditions.empty()) {
            json["conditions"] = std::move(conditions);
        }

        return json;
    }
};

// 前向声明
template <typename T>
class CriterionListener;

/**
 * @brief 触发器监听器
 *
 * 关联一个触发器实例、成就和条件名称。
 * 当触发器触发时，调用监听器来授予条件。
 */
template <typename T>
class CriterionListener {
public:
    using InstanceType = T;

    CriterionListener(const T& instance, AdvancementPtr advancement, std::string criterion) noexcept
        : m_instance(instance)
        , m_advancement(std::move(advancement))
        , m_criterion(std::move(criterion))
    {}

    /**
     * @brief 获取触发器实例
     */
    [[nodiscard]] const T& getInstance() const noexcept { return m_instance; }

    /**
     * @brief 获取成就
     */
    [[nodiscard]] AdvancementPtr getAdvancement() const noexcept { return m_advancement; }

    /**
     * @brief 获取条件名称
     */
    [[nodiscard]] const std::string& getCriterion() const noexcept { return m_criterion; }

    /**
     * @brief 授予条件
     */
    void grantCriterion(::mc::server::PlayerAdvancements& advancements) const;

    /**
     * @brief 比较运算符（用于set）
     */
    bool operator<(const CriterionListener& other) const noexcept
    {
        if (m_advancement != other.m_advancement) {
            return m_advancement < other.m_advancement;
        }
        return m_criterion < other.m_criterion;
    }

    bool operator==(const CriterionListener& other) const noexcept
    {
        return m_advancement == other.m_advancement && m_criterion == other.m_criterion;
    }

private:
    T m_instance;
    AdvancementPtr m_advancement;
    std::string m_criterion;
};

/**
 * @brief 触发器基类（无类型参数）
 *
 * 提供类型擦除的监听器管理接口，用于 PlayerAdvancements 在不知道
 * 具体触发器类型的情况下注册/注销监听器。
 */
class ICriterionTriggerBase {
public:
    virtual ~ICriterionTriggerBase() = default;

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] virtual ResourceLocation getId() const = 0;

    /**
     * @brief 从JSON反序列化触发器实例
     */
    [[nodiscard]] virtual Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) = 0;

    /**
     * @brief 添加监听器（类型擦除版本）
     *
     * 由 PlayerAdvancements::registerListeners() 调用。
     * 内部将 ICriterionInstance 向下转型为具体类型 T 并创建 CriterionListener<T>。
     *
     * @param advancements 玩家成就进度
     * @param advancement 成就
     * @param criterion 条件名称
     * @param instance 触发器实例
     */
    virtual void addListenerForCriterion(::mc::server::PlayerAdvancements& advancements,
        AdvancementPtr advancement,
        const std::string& criterion,
        const std::shared_ptr<ICriterionInstance>& instance) = 0;

    /**
     * @brief 移除监听器（类型擦除版本）
     *
     * 由 PlayerAdvancements::unregisterListeners() 调用。
     *
     * @param advancements 玩家成就进度
     * @param advancement 成就
     * @param criterion 条件名称
     */
    virtual void removeListenerForCriterion(
        ::mc::server::PlayerAdvancements& advancements, AdvancementPtr advancement, const std::string& criterion) = 0;

    /**
     * @brief 移除玩家的所有监听器（类型擦除版本）
     *
     * @param advancements 玩家成就进度
     */
    virtual void removeAllListenersForPlayer(::mc::server::PlayerAdvancements& advancements) = 0;
};

/**
 * @brief 触发器接口
 *
 * 定义触发器的行为：
 * - 添加/移除监听器
 * - 反序列化条件
 * - 触发检测
 */
template <typename T>
class ICriterionTrigger : public ICriterionTriggerBase {
public:
    using Listener = CriterionListener<T>;
    using InstancePtr = std::shared_ptr<T>;

    /**
     * @brief 添加监听器
     * @param advancements 玩家成就进度
     * @param listener 监听器
     */
    virtual void addListener(::mc::server::PlayerAdvancements& advancements, const Listener& listener) = 0;

    /**
     * @brief 移除监听器
     * @param advancements 玩家成就进度
     * @param listener 监听器
     */
    virtual void removeListener(::mc::server::PlayerAdvancements& advancements, const Listener& listener) = 0;

    /**
     * @brief 移除玩家的所有监听器
     * @param advancements 玩家成就进度
     */
    virtual void removeAllListeners(::mc::server::PlayerAdvancements& advancements) = 0;

    /**
     * @brief 从JSON反序列化触发器实例
     */
    Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override
    {
        auto instance = std::make_shared<T>();
        auto result = instance->fromJson(json);
        if (result.failed()) {
            return result.error();
        }
        return instance;
    }
};

/**
 * @brief 触发器抽象基类
 *
 * 提供监听器管理的通用实现。
 */
template <typename T>
class AbstractCriterionTrigger : public ICriterionTrigger<T> {
public:
    using Listener = typename ICriterionTrigger<T>::Listener;

    void addListener(::mc::server::PlayerAdvancements& advancements, const Listener& listener) override
    {
        m_listeners[&advancements].insert(listener);
    }

    void removeListener(::mc::server::PlayerAdvancements& advancements, const Listener& listener) override
    {
        auto it = m_listeners.find(&advancements);
        if (it != m_listeners.end()) {
            it->second.erase(listener);
            if (it->second.empty()) {
                m_listeners.erase(it);
            }
        }
    }

    void removeAllListeners(::mc::server::PlayerAdvancements& advancements) override
    {
        m_listeners.erase(&advancements);
    }

    // ========== 类型擦除的监听器管理（ICriterionTriggerBase 接口实现）==========

    void addListenerForCriterion(::mc::server::PlayerAdvancements& advancements,
        AdvancementPtr advancement,
        const std::string& criterion,
        const std::shared_ptr<ICriterionInstance>& instance) override
    {
        // 将 ICriterionInstance 向下转型为具体类型 T
        auto typedInstance = std::dynamic_pointer_cast<T>(instance);
        if (typedInstance == nullptr) {
            spdlog::warn("Failed to cast criterion instance for trigger {}: type mismatch", this->getId().toString());
            return;
        }

        // 创建监听器并注册
        Listener listener(*typedInstance, std::move(advancement), criterion);
        addListener(advancements, listener);
    }

    void removeListenerForCriterion(::mc::server::PlayerAdvancements& advancements,
        AdvancementPtr advancement,
        const std::string& criterion) override
    {
        // 需要找到匹配的监听器并移除
        // 由于 CriterionListener 的比较基于 advancement 和 criterion，
        // 我们构造一个临时监听器来查找
        auto it = m_listeners.find(&advancements);
        if (it == m_listeners.end()) {
            return;
        }

        // 查找匹配的监听器
        for (auto listenerIt = it->second.begin(); listenerIt != it->second.end(); ++listenerIt) {
            if (listenerIt->getAdvancement() == advancement && listenerIt->getCriterion() == criterion) {
                it->second.erase(listenerIt);
                break;
            }
        }

        // 如果该玩家已无监听器，移除整个条目
        if (it->second.empty()) {
            m_listeners.erase(it);
        }
    }

    void removeAllListenersForPlayer(::mc::server::PlayerAdvancements& advancements) override
    {
        removeAllListeners(advancements);
    }

    /**
     * @brief 触发检测
     *
     * 遍历玩家的所有监听器，对满足条件的监听器授予进度。
     *
     * @tparam PredicateT 谓词类型
     * @param advancements 玩家成就进度
     * @param predicate 检测谓词，返回true表示条件满足
     */
    template <typename PredicateT>
    void trigger(::mc::server::PlayerAdvancements& advancements, PredicateT&& predicate);

    /**
     * @brief 获取玩家的所有监听器
     */
    [[nodiscard]] const std::set<Listener>& getListeners(::mc::server::PlayerAdvancements& advancements) const noexcept
    {
        static const std::set<Listener> empty;
        auto it = m_listeners.find(&advancements);
        return it != m_listeners.end() ? it->second : empty;
    }

    /**
     * @brief 检查是否有玩家的监听器
     */
    [[nodiscard]] bool hasListeners(::mc::server::PlayerAdvancements& advancements) const noexcept
    {
        auto it = m_listeners.find(&advancements);
        return it != m_listeners.end() && !it->second.empty();
    }

private:
    std::unordered_map<::mc::server::PlayerAdvancements*, std::set<Listener>> m_listeners;
};

} // namespace mc::advancement

// 哈希特化
namespace std {
template <typename T>
struct hash<mc::advancement::CriterionListener<T>> {
    size_t operator()(const mc::advancement::CriterionListener<T>& listener) const
    {
        size_t h1 = hash<mc::advancement::AdvancementPtr>()(listener.getAdvancement());
        size_t h2 = hash<string>()(listener.getCriterion());
        return h1 ^ (h2 << 1);
    }
};
} // namespace std
