#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <set>
#include <typeindex>
#include <unordered_map>
#include <nlohmann/json.hpp>

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

    CriterionListener(const T& instance, AdvancementPtr advancement, std::string criterion)
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
    bool operator<(const CriterionListener& other) const
    {
        if (m_advancement != other.m_advancement) {
            return m_advancement < other.m_advancement;
        }
        return m_criterion < other.m_criterion;
    }

    bool operator==(const CriterionListener& other) const
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
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.AbstractCriterionTrigger
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
    [[nodiscard]] const std::set<Listener>& getListeners(::mc::server::PlayerAdvancements& advancements) const
    {
        static const std::set<Listener> empty;
        auto it = m_listeners.find(&advancements);
        return it != m_listeners.end() ? it->second : empty;
    }

    /**
     * @brief 检查是否有玩家的监听器
     */
    [[nodiscard]] bool hasListeners(::mc::server::PlayerAdvancements& advancements) const
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
