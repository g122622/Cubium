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

#include "LootFunction.hpp"

#include <memory>

#include <string>

#include <unordered_map>

namespace mc {

namespace loot {

/**
 * @brief 命名战利品函数注册表（对应原版 LootItemFunctions 注册表）
 *
 * minecraft:reference 通过资源位置引用注册在此处的函数。注册表持有函数所有权，
 * 供 FunctionReferenceFunction 以稳定指针引用（同 vanilla：加载时解析引用，缺失即报错）。
 */

class LootFunctionManager {

public:
    /// 获取单例

    static LootFunctionManager& instance();

    /**
     * @brief 注册命名函数
     * @param id 函数资源位置
     * @param function 函数（转移所有权）
     * @return 是否注册成功（同 ID 已注册时返回 false）
     */

    [[nodiscard]] bool registerFunction(const std::string& id, std::unique_ptr<LootFunction> function);

    /**
     * @brief 按资源位置查找函数
     * @return 函数指针，不存在返回 nullptr
     */

    [[nodiscard]] const LootFunction* getFunction(const std::string& id) const;

    /**
     * @brief 清空已注册函数（测试/重载数据包使用）
     */

    void clear();

private:
    LootFunctionManager() = default;

    std::unordered_map<std::string, std::unique_ptr<LootFunction>> m_functions;
};

} // namespace loot

} // namespace mc
