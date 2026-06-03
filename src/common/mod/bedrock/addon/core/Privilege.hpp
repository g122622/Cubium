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

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本执行权限级别
 *
 * 定义脚本模块的执行权限级别，用于控制脚本的执行时机和能力。
 * 权限级别越高，脚本可以越早执行，但同时也需要更高的信任度。
 */
enum class Privilege : u8 {
    Default = 0,               ///< 默认权限，标准执行时机
    RestrictedExecAllowed = 1, ///< 允许受限执行，可以在受限环境下运行
    EarlyExecAllowed = 2,      ///< 允许早期执行（PrePackLoad阶段），在资源包加载前执行
};

} // namespace mc::mod::bedrock::addon
