// Copyright (c) 2024 Cubium Project
// SPDX-License-Identifier: MIT

#pragma once

#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

#include <utility>

namespace mc::mod::bedrock::addon {

/**
 * @brief 引擎无关的脚本回调持有器
 *
 * 持有JS函数引用的生命周期管理包装。
 * 构造时retainValue，析构时releaseValue。
 * 替代QuickJS特定的JSCallbackHolder。
 *
 * 使用模式：
 *   void* funcHandle = ctx.getProperty(obj, "onTick");
 *   if (ctx.isFunction(funcHandle)) {
 *       ScriptCallbackHolder holder(ctx, funcHandle);
 *       // holder在生命周期内持有函数引用
 *       ctx.releaseValue(funcHandle); // 释放getProperty的引用
 *   }
 */
class ScriptCallbackHolder {
public:
    ScriptCallbackHolder() = default;

    /**
     * @brief 从函数句柄构造持有器
     *
     * 调用retainValue增加引用计数。
     *
     * @param ctx 绑定上下文
     * @param func 函数句柄（必须是有效的函数值）
     */
    ScriptCallbackHolder(IScriptBindingContext& ctx, void* func)
        : m_ctx(&ctx)
    {
        if (func && ctx.isFunction(func)) {
            ctx.retainValue(func);
            m_func = func;
        }
    }

    ~ScriptCallbackHolder()
    {
        if (m_ctx && m_func) {
            m_ctx->releaseValue(m_func);
        }
    }

    // 不可复制
    ScriptCallbackHolder(const ScriptCallbackHolder&) = delete;
    ScriptCallbackHolder& operator=(const ScriptCallbackHolder&) = delete;

    // 可移动
    ScriptCallbackHolder(ScriptCallbackHolder&& other) noexcept
        : m_ctx(other.m_ctx)
        , m_func(other.m_func)
    {
        other.m_ctx = nullptr;
        other.m_func = nullptr;
    }

    ScriptCallbackHolder& operator=(ScriptCallbackHolder&& other) noexcept
    {
        if (this != &other) {
            if (m_ctx && m_func) {
                m_ctx->releaseValue(m_func);
            }
            m_ctx = other.m_ctx;
            m_func = other.m_func;
            other.m_ctx = nullptr;
            other.m_func = nullptr;
        }
        return *this;
    }

    /**
     * @brief 调用回调，传入一个参数
     *
     * @param arg0 第一个参数句柄
     * @return 返回值句柄（调用者拥有所有权），异常时返回异常句柄
     */
    [[nodiscard]] void* call(void* arg0)
    {
        if (!m_ctx || !m_func) {
            return nullptr;
        }
        void* undef = m_ctx->createUndefined();
        void* result = m_ctx->callFunction1(m_func, undef, arg0);
        m_ctx->releaseValue(undef);
        return result;
    }

    /**
     * @brief 调用回调，无参数
     *
     * @return 返回值句柄（调用者拥有所有权），异常时返回异常句柄
     */
    [[nodiscard]] void* call0()
    {
        if (!m_ctx || !m_func) {
            return nullptr;
        }
        void* undef = m_ctx->createUndefined();
        void* result = m_ctx->callFunction0(m_func, undef);
        m_ctx->releaseValue(undef);
        return result;
    }

    /**
     * @brief 检查回调是否有效
     */
    [[nodiscard]] bool isValid() const { return m_ctx != nullptr && m_func != nullptr; }

    /**
     * @brief 获取函数句柄（不转移所有权）
     */
    [[nodiscard]] void* func() const { return m_func; }

    /**
     * @brief 获取绑定上下文
     */
    [[nodiscard]] IScriptBindingContext* context() const { return m_ctx; }

private:
    IScriptBindingContext* m_ctx = nullptr;
    void* m_func = nullptr;
};

} // namespace mc::mod::bedrock::addon
