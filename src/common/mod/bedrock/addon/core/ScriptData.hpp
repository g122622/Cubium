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

/**
 * @file ScriptData.hpp
 * @brief 脚本数据结构定义
 *
 * 定义从行为包加载的脚本源码和元信息的数据结构。
 */

#include <string>
#include <utility>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本数据
 *
 * 包含从行为包加载的脚本源码和元信息。
 * 用于在脚本引擎中加载和执行脚本代码。
 */
struct ScriptData {
    std::string name;      ///< 脚本模块名或路径
    std::string source;    ///< 脚本源码
    std::string filePath;  ///< 脚本文件路径（用于错误报告）
    bool isModule = false; ///< 是否为ES6模块

    /**
     * @brief 默认构造函数
     */
    ScriptData() = default;

    /**
     * @brief 参数构造函数
     * @param scriptName 脚本模块名或路径
     * @param scriptSource 脚本源码
     * @param scriptFilePath 脚本文件路径
     * @param moduleFlag 是否为ES6模块
     */
    ScriptData(std::string scriptName, std::string scriptSource, std::string scriptFilePath, bool moduleFlag = false)
        : name(std::move(scriptName))
        , source(std::move(scriptSource))
        , filePath(std::move(scriptFilePath))
        , isModule(moduleFlag)
    {}

    /**
     * @brief 移动构造函数
     */
    ScriptData(ScriptData&& other) noexcept = default;

    /**
     * @brief 拷贝构造函数
     */
    ScriptData(const ScriptData& other) = default;

    /**
     * @brief 移动赋值运算符
     */
    ScriptData& operator=(ScriptData&& other) noexcept = default;

    /**
     * @brief 拷贝赋值运算符
     */
    ScriptData& operator=(const ScriptData& other) = default;

    /**
     * @brief 析构函数
     */
    ~ScriptData() = default;
};

} // namespace mc::mod::bedrock::addon
