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
 */

#pragma once

#include "common/core/Types.hpp"
#include <any>
#include <string>
#include <utility>

namespace mc::mod::bedrock::addon {

/**
 * @brief 自定义组件参数
 *
 * 从JSON定义传入的参数数据，传递给组件事件回调。
 * 脚本注册自定义组件时，可以在行为包的block.json/item.json中
 * 定义参数，这些参数会通过CustomComponentParameters传递给回调。
 *
 * 该类使用std::any作为参数存储容器，支持任意类型的参数数据。
 * 调用者需要通过params()获取std::any后，使用std::any_cast转换为具体类型。
 */
class CustomComponentParameters {
public:
    CustomComponentParameters() noexcept = default;

    explicit CustomComponentParameters(std::any params) noexcept
        : m_params(std::move(params))
    {}

    // 移动构造和赋值
    CustomComponentParameters(CustomComponentParameters&& other) noexcept = default;
    CustomComponentParameters& operator=(CustomComponentParameters&& other) noexcept = default;

    // 禁止拷贝（std::any的拷贝可能昂贵）
    CustomComponentParameters(const CustomComponentParameters&) = delete;
    CustomComponentParameters& operator=(const CustomComponentParameters&) = delete;

    /**
     * @brief 获取参数数据
     *
     * @return 参数的std::any常量引用，调用者需使用std::any_cast转换
     */
    [[nodiscard]] const std::any& params() const noexcept { return m_params; }

    /**
     * @brief 是否有参数
     *
     * @return 如果参数有值则返回true
     */
    [[nodiscard]] bool hasParams() const noexcept { return m_params.has_value(); }

private:
    std::any m_params;
};

} // namespace mc::mod::bedrock::addon
