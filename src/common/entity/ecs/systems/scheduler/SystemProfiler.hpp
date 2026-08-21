#pragma once

#include <functional>

namespace mc::ecs {

/**
 * @brief System 执行期 profiling 扩展点
 *
 * 提供可注册的 pre/post invoke 钩子，执行器在每个 system 执行前后调用。默认禁用
 * （m_enabled=false），执行器在禁用时不调用钩子，零开销。注册钩子并 enable 后生效。
 *
 * 首批仅提供钩子框架，MC_TRACE_SCOPED_EVENT 由执行器直接发（逐 system 作用域事件）。
 * 阶段 H 将扩展为逐 system 耗时采集（累计耗时表 + 前后回调触发），届时补实现。
 */
class SystemProfiler {
public:
    using InvokeHook = std::function<void(const char* systemName)>;

    /** 启用/禁用 profiling（禁用时 preInvoke/postInvoke 立即返回，零开销） */
    void enable(bool enabled) noexcept { m_enabled = enabled; }

    /** profiling 是否启用 */
    [[nodiscard]] bool enabled() const noexcept { return m_enabled; }

    /** 注册 system 执行前回调（如调试标记、计数器复位） */
    void setPreInvokeHook(InvokeHook hook) { m_preInvoke = std::move(hook); }

    /** 注册 system 执行后回调（如耗时记录、状态快照） */
    void setPostInvokeHook(InvokeHook hook) { m_postInvoke = std::move(hook); }

    /** 执行器在 system 执行前调用（禁用时零开销返回） */
    void preInvoke(const char* systemName) const
    {
        if (m_enabled && m_preInvoke) {
            m_preInvoke(systemName);
        }
    }

    /** 执行器在 system 执行后调用（禁用时零开销返回） */
    void postInvoke(const char* systemName) const
    {
        if (m_enabled && m_postInvoke) {
            m_postInvoke(systemName);
        }
    }

private:
    bool m_enabled{false};
    InvokeHook m_preInvoke;
    InvokeHook m_postInvoke;

    // TODO(阶段H): 扩展为逐 system 耗时采集——累计耗时表（system 名 → 纳秒）+
    // preInvoke 记录开始时间戳、postInvoke 累加耗时，提供 gatherSystemTimings() 查询接口。
};

} // namespace mc::ecs
