#include "TemplateSystem.hpp"
#include "bindings/BuiltinEvents.hpp"
#include "bindings/BuiltinWidgets.hpp"
#include "runtime/TemplateInstance.hpp"

namespace mc::client::ui::kagero::tpl {

static bool s_initialized = false;

void initializeTemplateSystem()
{
    if (s_initialized) return;

    // 初始化内置组件工厂
    bindings::BuiltinWidgets::instance().initialize();

    // 初始化内置事件处理器
    bindings::BuiltinEvents::instance().initialize();

    s_initialized = true;
}

void shutdownTemplateSystem()
{
    s_initialized = false;
}

bool isTemplateSystemInitialized()
{
    return s_initialized;
}

} // namespace mc::client::ui::kagero::tpl
