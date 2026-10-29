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

#include <mutex>
#include <string>
#include <vector>

namespace mc::server::console {

/**
 * @brief 面板「自定义指标行」的写入与读取
 *
 * 右侧面板的指标来自 tick 线程定期采集的快照，但世界状态摘要（方块/群系/区块状态）
 * 无法在 post-tick 回调里廉价地算出来——它是按需查询（`/panel` 命令触发，由玩家在
 * 终端输入后经命令队列回到 tick 线程）。故单独用一个小容器传递：
 *
 * - tick 线程：持锁改写行列表；
 * - UI 线程：持锁整体拷贝一份。
 *
 * 行列表为空时面板只显示一行 "not refreshed yet" 提示。
 */
class PanelDetailStore {
public:
    /**
     * @brief 替换全部自定义行（tick 线程调用）
     *
     * 每行是完整文本（含左侧缩进），由调用方决定排版。
     *
     * @param lines 新的行列表（整体替换）
     */
    void update(std::vector<std::string> lines)
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_lines = std::move(lines);
    }

    /**
     * @brief 拷贝当前行列表（UI 线程调用）
     *
     * @return 行列表副本
     */
    [[nodiscard]] std::vector<std::string> get() const
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        return m_lines;
    }

private:
    mutable std::mutex m_mutex;
    std::vector<std::string> m_lines;
};

} // namespace mc::server::console
