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

#include "Screen.hpp"
#include "client/ui/kagero/state/ReactiveState.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/template/compiler/TemplateCompiler.hpp"
#include "client/ui/kagero/template/runtime/TemplateInstance.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace mc::client::ui::minecraft {

/**
 * @brief 模板驱动的屏幕基类
 *
 * 负责加载模板、维护绑定上下文，并把模板实例化为屏幕子树。
 */
class TemplateScreen : public Screen {
public:
    TemplateScreen(const std::string& templateSource,
        kagero::tpl::binder::BindingContext& context,
        const std::string& screenId = "template-screen");
    TemplateScreen(
        std::unique_ptr<kagero::tpl::binder::BindingContext> context, const std::string& screenId = "template-screen");
    TemplateScreen(kagero::tpl::binder::BindingContext& context, const std::string& screenId = "template-screen");

    static std::unique_ptr<TemplateScreen> fromFile(const std::string& templatePath,
        kagero::tpl::binder::BindingContext& context,
        const std::string& screenId = "template-screen");

    ~TemplateScreen() override;

    TemplateScreen(const TemplateScreen&) = delete;
    TemplateScreen& operator=(const TemplateScreen&) = delete;
    TemplateScreen(TemplateScreen&&) noexcept;
    TemplateScreen& operator=(TemplateScreen&&) noexcept;

    void onOpen() override;
    void onClose() override;
    void tick(f32 dt) override;
    void onResize(i32 width, i32 height) override;

    template <typename T>
    void expose(const std::string& path, const T* value)
    {
        m_context->expose(path, value);
    }

    template <typename T>
    void exposeWritable(const std::string& path, T* value)
    {
        m_context->exposeWritable(path, value);
    }

    template <typename T>
    void exposeReactive(const std::string& path, kagero::state::Reactive<T>& reactive)
    {
        m_context->exposeReactive(path, reactive);
    }

    void exposeSimpleCallback(const std::string& name, std::function<void()> callback)
    {
        m_context->exposeSimpleCallback(name, std::move(callback));
    }

    void exposeCallback(const std::string& name, kagero::tpl::binder::BindingContext::Callback callback)
    {
        m_context->exposeCallback(name, std::move(callback));
    }

    void refresh();
    void refreshBinding(const std::string& path);

    [[nodiscard]] kagero::widget::Widget* findWidget(const std::string& id);
    [[nodiscard]] const kagero::widget::Widget* findWidget(const std::string& id) const;

    [[nodiscard]] bool isValid() const { return m_instance != nullptr; }
    [[nodiscard]] kagero::tpl::runtime::TemplateInstance* instance() { return m_instance.get(); }
    [[nodiscard]] const kagero::tpl::runtime::TemplateInstance* instance() const { return m_instance.get(); }

    [[nodiscard]] kagero::tpl::binder::BindingContext* bindingContext() { return m_context; }
    [[nodiscard]] const kagero::tpl::binder::BindingContext* bindingContext() const { return m_context; }

protected:
    bool loadTemplate(const std::string& source);
    bool loadTemplateFile(const std::string& path);

private:
    void _syncRootWidgetBounds();
    void _resolvePercentSizes();

    std::unique_ptr<kagero::tpl::binder::BindingContext> m_ownedContext;
    kagero::tpl::binder::BindingContext* m_context = nullptr;
    std::unique_ptr<kagero::tpl::runtime::TemplateInstance> m_instance;
    bool m_templateLoaded = false;
};

} // namespace mc::client::ui::minecraft
