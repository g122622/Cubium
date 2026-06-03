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

#include "LootParameter.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 掉落参数集合
 *
 * 定义生成掉落物所需的参数类型集合。
 * 用于验证掉落表执行时上下文是否包含所需的参数。
 */
class LootParameterSet {
public:
    /**
     * @brief 参数集合类型枚举
     *
     * 定义不同场景下使用的参数集合类型。
     */
    enum class Type {
        Empty,   ///< 空集合，不需要任何参数
        Generic, ///< 通用集合
        Entity,  ///< 实体相关，如击杀掉落
        Block,   ///< 方块相关，如方块破坏掉落
        Chest,   ///< 容器，如宝箱战利品
        Fishing, ///< 钓鱼
        Gift,    ///< 礼物，如猫的礼物
        Barter   ///< 以物易物，如猪灵交易
    };

    LootParameterSet() = default;

    /**
     * @brief 构造指定类型的参数集合
     * @param type 参数集合类型
     */
    explicit LootParameterSet(Type type) noexcept
        : m_type(type)
    {}

    /**
     * @brief 获取参数集合类型
     * @return 参数集合类型
     */
    [[nodiscard]] Type getType() const noexcept { return m_type; }

    /**
     * @brief 获取参数集合的资源路径名称
     * @return 参数集合名称，格式为 "minecraft:xxx"
     */
    [[nodiscard]] std::string_view getName() const noexcept
    {
        switch (m_type) {
            case Type::Empty:
                return "minecraft:empty";
            case Type::Generic:
                return "minecraft:generic";
            case Type::Entity:
                return "minecraft:entity";
            case Type::Block:
                return "minecraft:block";
            case Type::Fishing:
                return "minecraft:fishing";
            case Type::Chest:
                return "minecraft:chest";
            case Type::Gift:
                return "minecraft:gift";
            case Type::Barter:
                return "minecraft:barter";
            default:
                return "minecraft:generic";
        }
    }

    /**
     * @brief 添加必需参数
     * @tparam T 参数类型
     * @param param 掉落参数对象
     */
    template <typename T>
    void addRequired(const LootParameter<T>& param)
    {
        m_requiredParams.push_back(param.getId());
    }

    /**
     * @brief 添加可选参数
     * @tparam T 参数类型
     * @param param 掉落参数对象
     */
    template <typename T>
    void addOptional(const LootParameter<T>& param)
    {
        m_optionalParams.push_back(param.getId());
    }

    /**
     * @brief 检查参数是否在集合中（必需或可选）
     * @param paramId 参数ID
     * @return 如果参数存在于集合中返回true
     */
    [[nodiscard]] bool contains(std::string_view paramId) const noexcept;

    /**
     * @brief 验证上下文是否包含所有必需参数
     * @param providedParams 提供的参数ID列表
     * @return 如果所有必需参数都存在返回true
     */
    [[nodiscard]] bool validate(const std::vector<std::string>& providedParams) const noexcept;

private:
    Type m_type = Type::Generic;               ///< 参数集合类型
    std::vector<std::string> m_requiredParams; ///< 必需参数ID列表
    std::vector<std::string> m_optionalParams; ///< 可选参数ID列表
};

} // namespace loot
} // namespace mc
