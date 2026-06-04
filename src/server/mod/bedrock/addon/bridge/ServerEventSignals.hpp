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

#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"

#include <vector>

namespace mc::server {

/**
 * @brief 获取所有beforeEvent信号定义
 *
 * 返回可取消的事件信号列表，包含事件名称和C++类型索引。
 * 这些信号注册到world.beforeEvents上。
 */
std::vector<mc::mod::bedrock::addon::EventSignalInfo> getBeforeEventSignals();

/**
 * @brief 获取所有afterEvent信号定义
 *
 * 返回不可取消的事件信号列表，包含事件名称和C++类型索引。
 * 这些信号注册到world.afterEvents上。
 */
std::vector<mc::mod::bedrock::addon::EventSignalInfo> getAfterEventSignals();

} // namespace mc::server
