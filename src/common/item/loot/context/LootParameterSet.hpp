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
 * 参考: net.minecraft.loot.LootParameterSet
 */
class LootParameterSet {
public:
    /**
     * @brief 参数集合类型
     */
    enum class Type {
        Empty,   // 空集合
        Generic, // 通用
        Entity,  // 实体相关
        Block,   // 方块相关
        Chest,   // 容器
        Fishing, // 钓鱼
        Gift,    // 礼物
        Barter   // 以物易物
    };

    LootParameterSet() = default;
    explicit LootParameterSet(Type type)
        : m_type(type)
    {}

    /**
     * @brief 获取参数集合类型
     */
    [[nodiscard]] Type getType() const { return m_type; }

    /**
     * @brief 获取参数集合名称
     */
    [[nodiscard]] std::string getName() const
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
     */
    template <typename T>
    void addRequired(const LootParameter<T>& param)
    {
        m_requiredParams.push_back(param.getId());
    }

    /**
     * @brief 添加可选参数
     */
    template <typename T>
    void addOptional(const LootParameter<T>& param)
    {
        m_optionalParams.push_back(param.getId());
    }

    /**
     * @brief 检查参数是否在集合中
     */
    [[nodiscard]] bool contains(const std::string& paramId) const;

    /**
     * @brief 验证上下文是否包含所有必需参数
     */
    [[nodiscard]] bool validate(const std::vector<std::string>& providedParams) const;

private:
    Type m_type = Type::Generic;
    std::vector<std::string> m_requiredParams;
    std::vector<std::string> m_optionalParams;
};

} // namespace loot
} // namespace mc
