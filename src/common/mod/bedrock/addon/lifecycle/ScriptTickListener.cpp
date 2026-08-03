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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HAVING BEEN CLAIMED FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/mod/bedrock/addon/lifecycle/ScriptTickListener.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"

namespace mc::mod::bedrock::addon {

ScriptTickListener::ScriptTickListener(ScriptManager& manager)
    : m_manager(manager)
{}

void ScriptTickListener::beginTick()
{
    // 记录tick开始时间（看门狗用）
    m_manager.watchdog().beginTick();
}

void ScriptTickListener::tick(u64 currentTick)
{
    // 1. 执行调度回调（system.run/runInterval/runTimeout）
    m_manager.scheduler().tick(currentTick);

    // 2. 驱动插件tick（处理pending jobs、scheduled callbacks等）
    m_manager.tickPlugins();

    // 3. 驱动JS引擎的pending jobs（Promise、setTimeout等）
    m_manager.executePendingJobs();
}

void ScriptTickListener::endTick()
{
    // 刷新afterEvent队列
    m_manager.eventBus().tick();

    // 检查看门狗
    m_manager.watchdog().endTick();
    m_manager.watchdog().tick(m_manager);
}

} // namespace mc::mod::bedrock::addon
