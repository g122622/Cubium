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
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
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
    if (m_instance) {
        m_instance->updateBindings();
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
        m_instance->updateBindings();
    }
}

void TemplateScreen::onResize(i32 width, i32 height)
{
    Screen::onResize(width, height);
    _syncRootWidgetBounds();
    if (m_instance) {
        m_instance->updateBindings();
    }
}

void TemplateScreen::refresh()
{
    if (m_instance) {
        m_instance->updateBindings();
    }
}

void TemplateScreen::refreshBinding(const std::string& path)
{
    if (m_instance) {
        m_instance->notifyStateChange(path);
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

} // namespace mc::client::ui::minecraft
