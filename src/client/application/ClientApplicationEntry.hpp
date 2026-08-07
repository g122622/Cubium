/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "common/application/BaseApplicationEntry.hpp"

#include "client/application/ClientApplication.hpp"

namespace mc::client {

/**
 * @brief 客户端进程入口（BaseApplicationEntry 子类）
 *
 * 把原 client/main.cpp 的过程式启动流程封装为对象：
 * - onFlagsParsed：把 gflags 的 --config / --skip-integrated / --quick-play / --quick-play-new /
 *   --benchmark-exit-after-initialize 填进 ClientLaunchParams。
 * - runApplication：构造 ClientApplication → initialize → 若 benchmarkExitAfterInitialize 则
 *   log + return 0 → 否则 run()（阻塞到窗口关闭）。
 *
 * 注：client 无信号处理（run() 阻塞到窗口关闭，无需 SIGINT 轮询）。
 */
class ClientApplicationEntry : public mc::application::BaseApplicationEntry {
public:
    ClientApplicationEntry() = default;
    ~ClientApplicationEntry() override = default;

    ClientApplicationEntry(const ClientApplicationEntry&) = delete;
    ClientApplicationEntry& operator=(const ClientApplicationEntry&) = delete;

protected:
    [[nodiscard]] std::string_view displayName() const override { return "Client"; }

    void onFlagsParsed() override;
    [[nodiscard]] int runApplication() override;

private:
    /// 启动参数（由 onFlagsParsed 从 gflags 填充）。
    ClientLaunchParams m_params;
};

} // namespace mc::client
