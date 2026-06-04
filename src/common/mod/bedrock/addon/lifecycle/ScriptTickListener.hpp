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

class ScriptManager;

/**
 * @brief 脚本Tick监听器
 *
 * 每个服务器tick调用，驱动脚本系统的执行。
 * 执行顺序：
 * 1. beginTick() - 记录tick开始
 * 2. tick(currentTick) - 执行调度回调、插件tick、pending jobs
 * 3. endTick() - 刷新afterEvent队列、检查看门狗
 */
class ScriptTickListener {
public:
    explicit ScriptTickListener(ScriptManager& manager);
    ~ScriptTickListener() = default;

    /**
     * @brief tick开始时调用
     *
     * 记录tick开始时间，用于看门狗超时检测。
     */
    void beginTick();

    /**
     * @brief 执行脚本tick
     *
     * 处理调度回调（system.run/runInterval/runTimeout）、
     * JS引擎的pending jobs（Promise回调等）、驱动各插件的tick。
     *
     * @param currentTick 当前服务器tick计数
     */
    void tick(u64 currentTick);

    /**
     * @brief tick结束时调用
     *
     * 刷新afterEvent队列，检查看门狗。
     */
    void endTick();

private:
    ScriptManager& m_manager;
};

} // namespace mc::mod::bedrock::addon
