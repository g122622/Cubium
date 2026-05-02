#pragma once

#include "Screen.hpp"
#include "../../kagero/template/compiler/TemplateCompiler.hpp"
#include "../../kagero/template/runtime/TemplateInstance.hpp"
#include "../../kagero/state/ReactiveState.hpp"
#include "../../kagero/template/binder/BindingContext.hpp"
#include <functional>
#include <memory>
#include <utility>

namespace mc::client::ui::minecraft {

/**
 * @brief 模板驱动的屏幕基类
 *
 * 负责加载模板、维护绑定上下文，并把模板实例化为屏幕子树。
 */
class TemplateScreen : public Screen {
public:
    TemplateScreen(const String& templateSource,
                   kagero::tpl::binder::BindingContext& context,
                   const String& screenId = "template-screen");
    TemplateScreen(std::unique_ptr<kagero::tpl::binder::BindingContext> context,
                   const String& screenId = "template-screen");
    TemplateScreen(kagero::tpl::binder::BindingContext& context,
                   const String& screenId = "template-screen");

    static std::unique_ptr<TemplateScreen> fromFile(
        const String& templatePath,
        kagero::tpl::binder::BindingContext& context,
        const String& screenId = "template-screen");

    ~TemplateScreen() override;

    TemplateScreen(const TemplateScreen&) = delete;
    TemplateScreen& operator=(const TemplateScreen&) = delete;
    TemplateScreen(TemplateScreen&&) noexcept;
    TemplateScreen& operator=(TemplateScreen&&) noexcept;

    void onOpen() override;
    void onClose() override;
    void tick(f32 dt) override;
    void onResize(i32 width, i32 height) override;

    template<typename T>
    void expose(const String& path, const T* value) {
        if (m_context) {
            m_context->expose(path, value);
        }
    }

    template<typename T>
    void exposeWritable(const String& path, T* value) {
        if (m_context) {
            m_context->exposeWritable(path, value);
        }
    }

    template<typename T>
    void exposeReactive(const String& path, kagero::state::Reactive<T>& reactive) {
        if (m_context) {
            m_context->exposeReactive(path, reactive);
        }
    }

    void exposeSimpleCallback(const String& name, std::function<void()> callback) {
        if (m_context) {
            m_context->exposeSimpleCallback(name, std::move(callback));
        }
    }

    void exposeCallback(const String& name,
                        kagero::tpl::binder::BindingContext::Callback callback) {
        if (m_context) {
            m_context->exposeCallback(name, std::move(callback));
        }
    }

    void refresh();
    void refreshBinding(const String& path);

    [[nodiscard]] kagero::widget::Widget* findWidget(const String& id);
    [[nodiscard]] const kagero::widget::Widget* findWidget(const String& id) const;

    [[nodiscard]] bool isValid() const { return m_instance != nullptr; }
    [[nodiscard]] kagero::tpl::runtime::TemplateInstance* instance() { return m_instance.get(); }
    [[nodiscard]] const kagero::tpl::runtime::TemplateInstance* instance() const { return m_instance.get(); }

    [[nodiscard]] kagero::tpl::binder::BindingContext* bindingContext() { return m_context; }
    [[nodiscard]] const kagero::tpl::binder::BindingContext* bindingContext() const { return m_context; }

protected:
    bool loadTemplate(const String& source);
    bool loadTemplateFile(const String& path);

private:
    void syncRootWidgetBounds();

    std::unique_ptr<kagero::tpl::binder::BindingContext> m_ownedContext;
    kagero::tpl::binder::BindingContext* m_context = nullptr;
    std::unique_ptr<kagero::tpl::runtime::TemplateInstance> m_instance;
    bool m_templateLoaded = false;
};

} // namespace mc::client::ui::minecraft
