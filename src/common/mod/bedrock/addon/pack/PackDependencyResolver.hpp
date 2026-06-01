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

#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 包依赖解析器
 *
 * 解析行为包之间的UUID依赖关系
 */
class PackDependencyResolver {
public:
    /**
     * @brief 解析结果
     */
    struct ResolveResult {
        bool success = false;
        std::vector<std::string> missingDependencies; // 缺失的依赖UUID
        std::vector<std::string> versionMismatches;   // 版本不匹配的依赖

        /**
         * @brief 转换为可读字符串
         * @return 结果描述
         */
        [[nodiscard]] std::string toString() const;
    };

    /**
     * @brief 解析所有行为包的依赖关系
     *
     * 检查每个行为包声明的依赖是否都能在包列表中找到匹配的UUID，
     * 并验证版本是否兼容
     *
     * @param packs 行为包列表
     * @return 解析结果
     */
    static ResolveResult resolve(const std::vector<std::unique_ptr<BehaviorPack>>& packs);
};

} // namespace mc::mod::bedrock::addon
