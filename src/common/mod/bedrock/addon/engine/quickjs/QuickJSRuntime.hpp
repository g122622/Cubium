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
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include <memory>

struct JSRuntime;

namespace mc::mod::bedrock::addon {

class QuickJSContext;

/**
 * @brief QuickJS运行时实现
 *
 * 封装QuickJS的JSRuntime，管理内存和上下文。
 * 每个QuickJSEngine拥有一个QuickJSRuntime。
 */
class QuickJSRuntime : public IScriptRuntime {
public:
    QuickJSRuntime();
    ~QuickJSRuntime() override;

    // 禁止拷贝
    QuickJSRuntime(const QuickJSRuntime&) = delete;
    QuickJSRuntime& operator=(const QuickJSRuntime&) = delete;

    /**
     * @brief 初始化QuickJS运行时
     * @return 是否成功
     */
    bool initialize();

    // IScriptRuntime接口实现
    [[nodiscard]] std::unique_ptr<IScriptContext> createContext(const ContextConfig& config) override;
    void destroyContext(IScriptContext* context) override;
    void executePendingJobs() override;
    [[nodiscard]] bool hasPendingJobs() const override;
    [[nodiscard]] RuntimeStats computeStats() const override;
    void setMemoryLimit(u64 limitBytes) override;

    /**
     * @brief 获取原生JSRuntime指针
     */
    [[nodiscard]] JSRuntime* nativeRuntime() const { return m_runtime; }

private:
    JSRuntime* m_runtime = nullptr;
    u32 m_contextCount = 0;
    u64 m_memoryLimit = 0;
};

} // namespace mc::mod::bedrock::addon
