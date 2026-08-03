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

#include "TemplateScreen.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/template/bindings/BuiltinWidgets.hpp"
#include "client/ui/kagero/template/compiler/TemplateCompiler.hpp"
#include "client/ui/kagero/template/runtime/TemplateInstance.hpp"
#include "client/ui/kagero/widget/IWidgetContainer.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "common/core/Types.hpp"
#include "common/util/TimeUtils.hpp"
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::client::ui::minecraft {
namespace {

std::filesystem::path resolveTemplatePath(const std::string& path)
{
    const std::filesystem::path requested(path);
    if (std::filesystem::exists(requested)) {
        return requested;
    }

    const std::filesystem::path fileDir = std::filesystem::path(__FILE__).parent_path();
    const std::filesystem::path sourceTemplatesDir = fileDir.parent_path() / "templates";
    const std::filesystem::path filename = requested.filename();

    const std::filesystem::path sourceCandidate = sourceTemplatesDir / filename;
    if (std::filesystem::exists(sourceCandidate)) {
        return sourceCandidate;
    }

    const std::filesystem::path directSourceCandidate = sourceTemplatesDir / requested;
    if (std::filesystem::exists(directSourceCandidate)) {
        return directSourceCandidate;
    }

    return requested;
}

} // namespace

TemplateScreen::TemplateScreen(
    const std::string& templateSource, kagero::tpl::binder::BindingContext& context, const std::string& screenId)
    : Screen(screenId)
    , m_context(&context)
{
    loadTemplate(templateSource);
}

TemplateScreen::TemplateScreen(
    std::unique_ptr<kagero::tpl::binder::BindingContext> context, const std::string& screenId)
    : Screen(screenId)
    , m_ownedContext(std::move(context))
    , m_context(m_ownedContext.get())
{}

TemplateScreen::TemplateScreen(kagero::tpl::binder::BindingContext& context, const std::string& screenId)
    : Screen(screenId)
    , m_context(&context)
{}

TemplateScreen::~TemplateScreen() = default;

TemplateScreen::TemplateScreen(TemplateScreen&& other) noexcept
    : Screen(std::move(other))
    , m_ownedContext(std::move(other.m_ownedContext))
    , m_context(m_ownedContext ? m_ownedContext.get() : other.m_context)
    , m_instance(std::move(other.m_instance))
    , m_templateLoaded(other.m_templateLoaded)
{
    other.m_context = nullptr;
    other.m_templateLoaded = false;
}

TemplateScreen& TemplateScreen::operator=(TemplateScreen&& other) noexcept
{
    if (this != &other) {
        Screen::operator=(std::move(other));
        m_ownedContext = std::move(other.m_ownedContext);
        m_context = m_ownedContext ? m_ownedContext.get() : other.m_context;
        m_instance = std::move(other.m_instance);
        m_templateLoaded = other.m_templateLoaded;
        other.m_context = nullptr;
        other.m_templateLoaded = false;
    }
    return *this;
}

std::unique_ptr<TemplateScreen> TemplateScreen::fromFile(
    const std::string& templatePath, kagero::tpl::binder::BindingContext& context, const std::string& screenId)
{
    auto screen = std::make_unique<TemplateScreen>(context, screenId);
    if (!screen->loadTemplateFile(templatePath)) {
        spdlog::error("[TemplateScreen] Failed to load template from: {}", templatePath);
        return nullptr;
    }

    return screen;
}

void TemplateScreen::onOpen()
{
    Screen::onOpen();
    _syncRootWidgetBounds();
    _resolvePercentSizes();

    // 从模板根节点读取 modal 属性并设置到 Screen
    if (!m_children.empty()) {
        auto* root = m_children.front().get();
        if (root != nullptr) {
            const auto* modalValue = root->getUserData("modal");
            if (modalValue != nullptr) {
                bool isModal = (*modalValue == "true" || *modalValue == "1");
                setModal(isModal);
            }
        }
    }

    if (m_instance) {
        // 立即刷新所有待处理任务并全量更新绑定
        m_instance->refresh();
    }
}

void TemplateScreen::onClose()
{
    Screen::onClose();
}

void TemplateScreen::tick(f32 dt)
{
    Screen::tick(dt);
    if (m_instance) {
        // 推进调度器：执行到期的延迟更新任务
        // 注意：updateBindings() 不再每帧全量调用，由调度器按需增量更新
        // 仅在调度器禁用延迟更新且无待处理任务时，才做兜底全量刷新
        const u64 currentMs = util::TimeUtils::getCurrentTimeMs();
        const u32 executed = m_instance->tick(currentMs);
        (void)executed;
    }
}

void TemplateScreen::onResize(i32 width, i32 height)
{
    Screen::onResize(width, height);
    _syncRootWidgetBounds();
    _resolvePercentSizes();
    if (m_instance) {
        // 立即刷新所有待处理任务并全量更新绑定
        m_instance->refresh();
    }
}

void TemplateScreen::refresh()
{
    if (m_instance) {
        m_instance->refresh();
    }
}

void TemplateScreen::refreshBinding(const std::string& path)
{
    if (m_instance) {
        // 入队该路径并立即刷新（绕过调度器延迟）
        m_instance->notifyStateChange(path);
        m_instance->flushPending();
    }
}

kagero::widget::Widget* TemplateScreen::findWidget(const std::string& id)
{
    if (m_instance) {
        return m_instance->findWidgetById(id);
    }
    return nullptr;
}

const kagero::widget::Widget* TemplateScreen::findWidget(const std::string& id) const
{
    if (m_instance) {
        return m_instance->findWidgetById(id);
    }
    return nullptr;
}

bool TemplateScreen::loadTemplate(const std::string& source)
{
    if (m_context == nullptr) {
        spdlog::error("[TemplateScreen] Missing binding context for screen: {}", id());
        m_templateLoaded = false;
        return false;
    }

    clearWidgets();
    m_instance.reset();

    kagero::tpl::compiler::TemplateCompiler compiler;
    auto compiled = compiler.compile(source, id());
    if (!compiled) {
        spdlog::error("[TemplateScreen] Failed to compile template for screen: {}", id());
        m_templateLoaded = false;
        return false;
    }

    m_instance = std::make_unique<kagero::tpl::runtime::TemplateInstance>(std::move(compiled), *m_context);

    if (!m_instance->instantiateInto(this)) {
        spdlog::error("[TemplateScreen] Failed to instantiate template for screen: {}", id());
        m_instance.reset();
        m_templateLoaded = false;
        return false;
    }

    m_templateLoaded = true;
    _syncRootWidgetBounds();
    _resolvePercentSizes();
    return true;
}

bool TemplateScreen::loadTemplateFile(const std::string& path)
{
    const auto resolvedPath = resolveTemplatePath(path);
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        spdlog::error("[TemplateScreen] Failed to open template file: {}", resolvedPath.string());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadTemplate(buffer.str());
}

void TemplateScreen::_syncRootWidgetBounds()
{
    if (!m_instance || m_children.empty()) {
        return;
    }

    auto* root = m_children.front().get();
    if (root != nullptr) {
        root->setBounds(bounds());
    }
}

void TemplateScreen::_resolvePercentSizes()
{
    if (m_children.empty()) {
        return;
    }

    // 递归遍历控件树，解析百分比值
    std::function<void(kagero::widget::Widget*, i32, i32)> resolveRecursive;
    resolveRecursive = [&resolveRecursive](kagero::widget::Widget* widget, i32 parentWidth, i32 parentHeight) {
        if (widget == nullptr || parentWidth <= 0 || parentHeight <= 0) {
            return;
        }

        // 解析百分比尺寸
        const auto* sizePercent = widget->getUserData("__size_percent");
        if (sizePercent != nullptr) {
            kagero::tpl::bindings::widget_attrs::applySizeWithParent(widget, *sizePercent, parentWidth, parentHeight);
        }

        // 解析百分比位置
        const auto* posPercent = widget->getUserData("__pos_percent");
        if (posPercent != nullptr) {
            kagero::tpl::bindings::widget_attrs::applyPositionWithParent(
                widget, *posPercent, parentWidth, parentHeight);
        }

        // 递归子控件
        if (auto* container = dynamic_cast<kagero::widget::IWidgetContainer*>(widget)) {
            for (auto& child : container->widgets()) {
                resolveRecursive(child.get(), widget->width(), widget->height());
            }
        }
    };

    auto* root = m_children.front().get();
    if (root != nullptr) {
        // 根控件的父容器是 TemplateScreen 自身
        resolveRecursive(root, bounds().width, bounds().height);
    }
}

} // namespace mc::client::ui::minecraft
