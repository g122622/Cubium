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

#include "LootParameterSet.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootParameter.hpp"
#include "common/util/math/random/Random.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {

// Forward declarations
class IWorld;

namespace loot {

// Forward declaration - LootContext is defined in LootContext.hpp
class LootCondition;
class LootTable;

/**
 * @brief 掉落上下文构建器
 */
class LootContextBuilder {
public:
    using LootTableResolver = std::function<const LootTable*(const std::string&)>;
    using PredicateResolver = std::function<const LootCondition*(const std::string&)>;

    explicit LootContextBuilder(IWorld& world);

    /**
     * @brief 设置随机数生成器
     */
    LootContextBuilder& withRandom(math::Random& random);

    /**
     * @brief 设置随机种子
     */
    LootContextBuilder& withSeed(u64 seed);

    /**
     * @brief 设置幸运值
     */
    LootContextBuilder& withLuck(f32 luck);

    /**
     * @brief 设置掠夺附魔等级
     */
    LootContextBuilder& withLootingModifier(i32 level);

    /**
     * @brief 设置参数
     */
    template <typename T>
    LootContextBuilder& withParameter(const LootParameter<T>& param, T* value)
    {
        m_params[param.getId()] = static_cast<void*>(value);
        return *this;
    }

    /**
     * @brief 设置可空参数
     */
    template <typename T>
    LootContextBuilder& withNullableParameter(const LootParameter<T>& param, T* value)
    {
        if (value) {
            m_params[param.getId()] = static_cast<void*>(value);
        } else {
            m_params.erase(param.getId());
        }
        return *this;
    }

    /**
     * @brief 设置拥有所有权的参数值
     *
     * 存储值的副本到构建器，在构建时传递给 LootContext。
     * 适用于简单的值类型（如 i32, f32 等）。
     *
     * @tparam T 参数值的类型
     * @param param 参数标识符
     * @param value 参数值
     */
    template <typename T>
    LootContextBuilder& withOwnedValue(const LootParameter<T>& param, T value)
    {
        auto ownedPtr = std::make_shared<T>(std::move(value));
        m_ownedValues.push_back(ownedPtr);
        m_params[param.getId()] = static_cast<void*>(ownedPtr.get());
        return *this;
    }

    /**
     * @brief 设置掉落表解析器
     */
    LootContextBuilder& withLootTableResolver(LootTableResolver resolver)
    {
        m_lootTableResolver = std::move(resolver);
        return *this;
    }

    /**
     * @brief 设置谓词解析器
     *
     * 用于 ReferenceCondition 查找命名谓词。
     *
     * @param resolver 谓词解析器函数
     */
    LootContextBuilder& withPredicateResolver(PredicateResolver resolver)
    {
        m_predicateResolver = std::move(resolver);
        return *this;
    }

    /**
     * @brief 构建掉落上下文
     */
    [[nodiscard]] std::unique_ptr<class LootContext> build(const LootParameterSet& paramSet = LootParameterSet());

private:
    IWorld& m_world;
    math::Random* m_random = nullptr;
    u64 m_seed = 0;
    bool m_hasSeed = false;
    f32 m_luck = 0.0f;
    i32 m_lootingModifier = 0;
    std::unordered_map<std::string, void*> m_params;
    std::vector<std::shared_ptr<void>> m_ownedValues; // 拥有所有权的值存储
    LootTableResolver m_lootTableResolver;
    PredicateResolver m_predicateResolver;
};

} // namespace loot
} // namespace mc
