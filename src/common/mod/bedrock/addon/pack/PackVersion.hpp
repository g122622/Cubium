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

#include "common/core/Types.hpp"

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 包版本号
 *
 * 兼容基岩版 [major, minor, patch] 格式的版本号
 */
struct PackVersion {
    i32 major = 0;
    i32 minor = 0;
    i32 patch = 0;

    /**
     * @brief 从整数数组解析版本号
     * @param v 版本数组，至少包含一个元素
     * @return 解析后的版本号
     */
    static PackVersion fromVector(const std::vector<i32>& v);

    /**
     * @brief 转换为整数数组
     * @return [major, minor, patch] 格式的数组
     */
    [[nodiscard]] std::vector<i32> toVector() const;

    /**
     * @brief 转换为字符串表示
     * @return "major.minor.patch" 格式的字符串
     */
    [[nodiscard]] std::string toString() const;

    // 比较运算符
    [[nodiscard]] bool operator==(const PackVersion& o) const;
    [[nodiscard]] bool operator!=(const PackVersion& o) const;
    [[nodiscard]] bool operator<(const PackVersion& o) const;
    [[nodiscard]] bool operator<=(const PackVersion& o) const;
    [[nodiscard]] bool operator>(const PackVersion& o) const;
    [[nodiscard]] bool operator>=(const PackVersion& o) const;

    /**
     * @brief 检查版本兼容性
     *
     * 主版本号必须相同，当前版本的次版本号和补丁号必须大于等于要求的版本
     *
     * @param required 要求的最低版本
     * @return 是否兼容
     */
    [[nodiscard]] bool isCompatibleWith(const PackVersion& required) const;
};

} // namespace mc::mod::bedrock::addon
