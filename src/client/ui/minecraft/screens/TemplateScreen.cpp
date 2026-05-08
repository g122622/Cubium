#include "TemplateScreen.hpp"
#include "../../kagero/state/StateStore.hpp"
#include "../../kagero/event/EventBus.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace mc::client::ui::minecraft {
namespace {

std::filesystem::path resolveTemplatePath(const std::string& path) {
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

TemplateScreen::TemplateScreen(const std::string& templateSource,
                               kagero::tpl::binder::BindingContext& context,
                               const std::string& screenId)
    : Screen(screenId)
    , m_context(&context) {
    loadTemplate(templateSource);
}

TemplateScreen::TemplateScreen(std::unique_ptr<kagero::tpl::binder::BindingContext> context,
                               const std::string& screenId)
    : Screen(screenId)
    , m_ownedContext(std::move(context))
    , m_context(m_ownedContext.get()) {
}

TemplateScreen::TemplateScreen(kagero::tpl::binder::BindingContext& context,
                               const std::string& screenId)
    : Screen(screenId)
    , m_context(&context) {
}

TemplateScreen::~TemplateScreen() = default;

TemplateScreen::TemplateScreen(TemplateScreen&& other) noexcept
    : Screen(std::move(other))
    , m_ownedContext(std::move(other.m_ownedContext))
    , m_context(m_ownedContext ? m_ownedContext.get() : other.m_context)
    , m_instance(std::move(other.m_instance))
    , m_templateLoaded(other.m_templateLoaded) {
    other.m_context = nullptr;
    other.m_templateLoaded = false;
}

TemplateScreen& TemplateScreen::operator=(TemplateScreen&& other) noexcept {
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
    const std::string& templatePath,
    kagero::tpl::binder::BindingContext& context,
    const std::string& screenId) {
    auto screen = std::make_unique<TemplateScreen>(context, screenId);
    if (!screen->loadTemplateFile(templatePath)) {
        spdlog::error("[TemplateScreen] Failed to load template from: {}", templatePath);
        return nullptr;
    }

    return screen;
}

void TemplateScreen::onOpen() {
    Screen::onOpen();
    syncRootWidgetBounds();
    if (m_instance) {
        m_instance->updateBindings();
    }
}

void TemplateScreen::onClose() {
    Screen::onClose();
}

void TemplateScreen::tick(f32 dt) {
    Screen::tick(dt);
    if (m_instance) {
        m_instance->updateBindings();
    }
}

void TemplateScreen::onResize(i32 width, i32 height) {
    Screen::onResize(width, height);
    syncRootWidgetBounds();
    if (m_instance) {
        m_instance->updateBindings();
    }
}

void TemplateScreen::refresh() {
    if (m_instance) {
        m_instance->updateBindings();
    }
}

void TemplateScreen::refreshBinding(const std::string& path) {
    if (m_instance) {
        m_instance->notifyStateChange(path);
    }
}

kagero::widget::Widget* TemplateScreen::findWidget(const std::string& id) {
    if (m_instance) {
        return m_instance->findWidgetById(id);
    }
    return nullptr;
}

const kagero::widget::Widget* TemplateScreen::findWidget(const std::string& id) const {
    if (m_instance) {
        return m_instance->findWidgetById(id);
    }
    return nullptr;
}

bool TemplateScreen::loadTemplate(const std::string& source) {
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

    m_instance = std::make_unique<kagero::tpl::runtime::TemplateInstance>(
        std::move(compiled), *m_context);

    if (!m_instance->instantiateInto(this)) {
        spdlog::error("[TemplateScreen] Failed to instantiate template for screen: {}", id());
        m_instance.reset();
        m_templateLoaded = false;
        return false;
    }

    m_templateLoaded = true;
    syncRootWidgetBounds();
    return true;
}

bool TemplateScreen::loadTemplateFile(const std::string& path) {
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

void TemplateScreen::syncRootWidgetBounds() {
    if (!m_instance || m_children.empty()) {
        return;
    }

    auto* root = m_children.front().get();
    if (root != nullptr) {
        root->setBounds(bounds());
    }
}

} // namespace mc::client::ui::minecraft
