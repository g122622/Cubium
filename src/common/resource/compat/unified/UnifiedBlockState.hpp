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

#include "../../../core/Types.hpp"
#include "UnifiedResource.hpp"
#include <functional>
#include <optional>
#include <vector>

namespace mc {
namespace resource {
namespace compat {
namespace unified {

/**
 * @brief 模型变体定义
 *
 * 表示具有旋转和 UV 锁定的单个模型变体。
 */
struct ModelVariant {
    ResourceLocation model; ///< 使用的模型
    i32 x = 0;              ///< X 轴旋转 (0, 90, 180, 270)
    i32 y = 0;              ///< Y 轴旋转 (0, 90, 180, 270)
    bool uvLock = false;    ///< 旋转时锁定 UV
    i32 weight = 1;         ///< 随机变体选择权重
};

/**
 * @brief 单个状态的模型变体列表
 *
 * 一个状态可以有多个加权变体用于随机选择。
 */
struct VariantList {
    std::vector<ModelVariant> variants;

    bool empty() const noexcept { return variants.empty(); }

    size_t size() const noexcept { return variants.size(); }

    /**
     * @brief 获取所有变体的总权重
     */
    i32 totalWeight() const noexcept
    {
        i32 total = 0;
        for (const auto& v : variants) {
            total += v.weight;
        }
        return total;
    }

    /**
     * @brief 按权重选择变体
     * @param random 范围 [0, totalWeight) 内的随机值
     * @return 选中的变体
     */
    const ModelVariant& selectByWeight(i32 random) const
    {
        i32 accumulated = 0;
        for (const auto& v : variants) {
            accumulated += v.weight;
            if (random < accumulated) {
                return v;
            }
        }
        return variants.back();
    }
};

/**
 * @brief 多部分条件
 *
 * 多部分模型部件应用的条件。
 */
struct MultipartCondition {
    // 属性条件（属性 -> 值）
    // 空表示"始终应用"
    std::map<std::string, std::string> properties;

    // OR 条件
    std::vector<std::map<std::string, std::string>> orConditions;

    /**
     * @brief 检查条件是否匹配给定的属性值
     *
     * @param properties 方块状态属性
     * @return 如果匹配则返回 true
     */
    bool matches(const std::map<std::string, std::string>& props) const
    {
        // 如果有 OR 条件，检查是否有任何一个匹配
        if (!orConditions.empty()) {
            for (const auto& orCond : orConditions) {
                if (matchesProperties(orCond, props)) {
                    return true;
                }
            }
            return false;
        }

        // 检查直接属性
        return matchesProperties(properties, props);
    }

private:
    static bool matchesProperties(
        const std::map<std::string, std::string>& conditions, const std::map<std::string, std::string>& properties)
    {
        for (const auto& [key, value] : conditions) {
            auto it = properties.find(key);
            if (it == properties.end() || it->second != value) {
                return false;
            }
        }
        return true;
    }
};

/**
 * @brief 多部分选择器
 *
 * 将条件与模型变体组合在一起。
 */
struct MultipartSelector {
    MultipartCondition condition;
    VariantList variants;

    /**
     * @brief 检查此选择器是否适用于给定属性
     */
    bool appliesTo(const std::map<std::string, std::string>& properties) const { return condition.matches(properties); }
};

/**
 * @brief 统一方块状态表示
 *
 * 方块状态定义方块的不同属性组合使用哪个模型。
 *
 * 两种格式:
 * 1. Variants: 简单属性 -> 模型映射
 * 2. Multipart: 复杂的基于条件的模型组合
 */
struct UnifiedBlockState : public UnifiedResource {
    /// 变体格式: 属性字符串 -> 变体列表
    /// 属性字符串格式: "property1=value1,property2=value2"
    std::map<std::string, VariantList> variants;

    /// 多部分格式: 条件模型部件
    std::optional<std::vector<MultipartSelector>> multipart;

    UnifiedBlockState() { type = ResourceType::BlockState; }

    /**
     * @brief 检查方块状态是否使用变体格式
     */
    bool isVariantFormat() const noexcept { return !variants.empty(); }

    /**
     * @brief 检查方块状态是否使用多部分格式
     */
    bool isMultipartFormat() const noexcept { return multipart.has_value() && !multipart->empty(); }

    /**
     * @brief 获取属性字符串的变体
     *
     * @param propString 属性字符串（例如 "facing=north,half=top"）
     * @return 变体列表，如果未找到则返回 nullptr
     */
    const VariantList* getVariants(const std::string& propString) const
    {
        auto it = variants.find(propString);
        return it != variants.end() ? &it->second : nullptr;
    }

    /**
     * @brief 获取适用的多部分选择器
     *
     * @param properties 方块状态属性
     * @return 适用的变体列表向量
     */
    std::vector<const VariantList*> getApplicableMultipart(const std::map<std::string, std::string>& properties) const
    {
        std::vector<const VariantList*> result;

        if (!multipart.has_value()) {
            return result;
        }

        for (const auto& selector : *multipart) {
            if (selector.appliesTo(properties)) {
                result.push_back(&selector.variants);
            }
        }

        return result;
    }

    /**
     * @brief 检查方块状态是否有任何变体
     */
    bool empty() const noexcept { return variants.empty() && (!multipart.has_value() || multipart->empty()); }
};

} // namespace unified
} // namespace compat
} // namespace resource
} // namespace mc
