#pragma once

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
