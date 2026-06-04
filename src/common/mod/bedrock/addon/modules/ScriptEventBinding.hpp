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

#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"

#include <string>
#include <typeindex>
#include <vector>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 事件信号信息
 *
 * 描述一个可订阅的事件信号，用于注册JS绑定。
 */
struct EventSignalInfo {
    std::string name;        ///< JS属性名 (e.g. "blockBreak")
    std::type_index typeIdx; ///< C++类型索引
    bool isBefore;           ///< true=beforeEvent(可取消), false=afterEvent
};

/**
 * @brief 注册事件绑定到world对象
 *
 * 在world对象的beforeEvents和afterEvents属性上注册事件信号对象。
 *
 * @param ctx 绑定上下文
 * @param worldObj world对象句柄
 * @param eventBus 事件总线
 * @param signals 事件信号列表
 */
void registerEventBindings(
    IScriptBindingContext& ctx, void* worldObj, ScriptEventBus& eventBus, const std::vector<EventSignalInfo>& signals);

} // namespace mc::mod::bedrock::addon
