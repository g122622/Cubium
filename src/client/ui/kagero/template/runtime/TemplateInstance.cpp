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

#include "TemplateInstance.hpp"
#include "../../event/WidgetEvents.hpp"
#include "../../widget/CheckboxWidget.hpp"
#include "../../widget/ListWidget.hpp"
#include "../../widget/SliderWidget.hpp"
#include "../../widget/TextFieldWidget.hpp"
#include "../../widget/TextWidget.hpp"
#include "../bindings/BuiltinEvents.hpp"
#include "../bindings/BuiltinWidgets.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/event/InputEvents.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/template/compiler/TemplateCompiler.hpp"
#include "client/ui/kagero/template/parser/Ast.hpp"
#include "client/ui/kagero/template/runtime/UpdateScheduler.hpp"
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/widget/IWidgetContainer.hpp"
#include "client/ui/kagero/widget/ScrollableWidget.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace mc::client::ui::kagero::tpl::runtime {

// ========== TemplateInstance实现 ==========

TemplateInstance::TemplateInstance(std::unique_ptr<compiler::CompiledTemplate> compiled, binder::BindingContext& ctx)
    : m_ownedCompiled(std::move(compiled))
    , m_compiled(m_ownedCompiled.get())
    , m_context(&ctx)
{
    registerDefaultFactories();
    registerDefaultAttributeSetters();
    registerDefaultEventBinders();
}

TemplateInstance::TemplateInstance(const compiler::CompiledTemplate* compiled, binder::BindingContext& ctx)
    : m_compiled(compiled)
    , m_context(&ctx)
{
    registerDefaultFactories();
    registerDefaultAttributeSetters();
    registerDefaultEventBinders();
}

TemplateInstance::~TemplateInstance()
{
    // 取消所有订阅
    for (u64 id : m_subscriptionIds) {
        if (m_context) {
            m_context->unsubscribe(id);
        }
    }
}

TemplateInstance::TemplateInstance(TemplateInstance&& other) noexcept
    : m_ownedCompiled(std::move(other.m_ownedCompiled))
    , m_compiled(other.m_ownedCompiled ? m_ownedCompiled.get() : other.m_compiled)
    , m_context(other.m_context)
    , m_rootWidget(std::move(other.m_rootWidget))
    , m_widgetById(std::move(other.m_widgetById))
    , m_widgetByPath(std::move(other.m_widgetByPath))
    , m_widgetFactories(std::move(other.m_widgetFactories))
    , m_attributeSetters(std::move(other.m_attributeSetters))
    , m_eventBinders(std::move(other.m_eventBinders))
    , m_defaultFactory(std::move(other.m_defaultFactory))
    , m_subscriptionIds(std::move(other.m_subscriptionIds))
    , m_scheduler(std::move(other.m_scheduler))
{
    other.m_compiled = nullptr;
    other.m_context = nullptr;

    // 重新绑定调度器回调：原回调捕获了 other 的 this 指针，移动后需重新指向 this
    _rebindSchedulerCallback();
}

TemplateInstance& TemplateInstance::operator=(TemplateInstance&& other) noexcept
{
    if (this != &other) {
        // 取消现有订阅
        for (u64 id : m_subscriptionIds) {
            if (m_context) {
                m_context->unsubscribe(id);
            }
        }

        m_ownedCompiled = std::move(other.m_ownedCompiled);
        m_compiled = other.m_ownedCompiled ? m_ownedCompiled.get() : other.m_compiled;
        m_context = other.m_context;
        m_rootWidget = std::move(other.m_rootWidget);
        m_widgetById = std::move(other.m_widgetById);
        m_widgetByPath = std::move(other.m_widgetByPath);
        m_widgetFactories = std::move(other.m_widgetFactories);
        m_attributeSetters = std::move(other.m_attributeSetters);
        m_eventBinders = std::move(other.m_eventBinders);
        m_defaultFactory = std::move(other.m_defaultFactory);
        m_subscriptionIds = std::move(other.m_subscriptionIds);
        m_scheduler = std::move(other.m_scheduler);

        other.m_compiled = nullptr;
        other.m_context = nullptr;

        // 重新绑定调度器回调
        _rebindSchedulerCallback();
    }
    return *this;
}

void TemplateInstance::registerWidgetFactory(const std::string& tagName, WidgetFactory factory)
{
    m_widgetFactories[tagName] = std::move(factory);
}

void TemplateInstance::registerDefaultFactories()
{
    // 确保BuiltinWidgets已初始化
    bindings::BuiltinWidgets::instance().initialize();

    // 设置默认工厂，使用BuiltinWidgets作为fallback
    m_defaultFactory =
        [](const std::string& tagName, const std::string& id, const std::map<std::string, std::string>& attrs) {
            return bindings::BuiltinWidgets::instance().create(tagName, id, attrs);
        };
}

void TemplateInstance::setDefaultFactory(WidgetFactory factory)
{
    m_defaultFactory = std::move(factory);
}

void TemplateInstance::registerAttributeSetter(const std::string& attrName, AttributeSetter setter)
{
    m_attributeSetters[attrName] = std::move(setter);
}

void TemplateInstance::registerDefaultAttributeSetters()
{
    // 位置属性
    m_attributeSetters["pos"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        bindings::widget_attrs::applyPosition(widget, value.toString());
    };

    // 尺寸属性
    m_attributeSetters["size"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        bindings::widget_attrs::applySize(widget, value.toString());
    };

    // 可见性
    m_attributeSetters["visible"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            widget->setVisible(value.asBool());
        };

    // 激活状态
    m_attributeSetters["active"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        widget->setActive(value.asBool());
    };

    // 文本内容
    m_attributeSetters["text"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* textWidget = dynamic_cast<widget::TextWidget*>(widget)) {
            textWidget->setText(value.toString());
        }
    };

    // 文本颜色
    m_attributeSetters["color"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* textWidget = dynamic_cast<widget::TextWidget*>(widget)) {
            textWidget->setColor(bindings::widget_attrs::parseColor(value.toString()));
        }
    };

    // X坐标（单独设置）
    m_attributeSetters["x"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        i32 x = widget->x();
        i32 y = widget->y();
        widget->setPosition(value.asInteger(), y);
    };

    // Y坐标（单独设置）
    m_attributeSetters["y"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        i32 x = widget->x();
        i32 y = widget->y();
        widget->setPosition(x, value.asInteger());
    };

    // 宽度（单独设置）
    m_attributeSetters["width"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        i32 w = widget->width();
        i32 h = widget->height();
        widget->setSize(value.asInteger(), h);
    };

    // 高度（单独设置）
    m_attributeSetters["height"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        i32 w = widget->width();
        i32 h = widget->height();
        widget->setSize(w, value.asInteger());
    };

    // 锚点
    m_attributeSetters["anchor"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        widget->setAnchor(bindings::widget_attrs::parseAnchor(value.toString()));
    };

    // Z层级
    m_attributeSetters["zIndex"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        widget->setZIndex(bindings::widget_attrs::parseInt(value.toString()));
    };

    // 透明度
    m_attributeSetters["alpha"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        widget->setAlpha(bindings::widget_attrs::parseFloat(value.toString(), 1.0f));
    };

    // 阴影
    m_attributeSetters["shadow"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* textWidget = dynamic_cast<widget::TextWidget*>(widget)) {
            textWidget->setShadow(value.asBool());
        }
    };

    // 缩放
    m_attributeSetters["scale"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* textWidget = dynamic_cast<widget::TextWidget*>(widget)) {
            textWidget->setScale(bindings::widget_attrs::parseFloat(value.toString(), 1.0f));
        }
    };

    // 对齐方式
    m_attributeSetters["align"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* textWidget = dynamic_cast<widget::TextWidget*>(widget)) {
            std::string align = value.toString();
            std::transform(align.begin(), align.end(), align.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (align == "left") {
                textWidget->setAlignment(widget::TextAlignment::Left);
            } else if (align == "center") {
                textWidget->setAlignment(widget::TextAlignment::Center);
            } else if (align == "right") {
                textWidget->setAlignment(widget::TextAlignment::Right);
            }
        }
    };

    // margin属性（格式：margin="10" / "h,v" / "l,t,r,b"）
    m_attributeSetters["margin"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        const std::string text = value.toString();
        size_t firstComma = text.find(',');
        if (firstComma == std::string::npos) {
            const i32 all = bindings::widget_attrs::parseInt(text);
            widget->setMargin(Margin(all));
            return;
        }
        size_t secondComma = text.find(',', firstComma + 1);
        if (secondComma == std::string::npos) {
            const i32 horizontal = bindings::widget_attrs::parseInt(text.substr(0, firstComma));
            const i32 vertical = bindings::widget_attrs::parseInt(text.substr(firstComma + 1));
            widget->setMargin(Margin(horizontal, vertical));
            return;
        }
        size_t thirdComma = text.find(',', secondComma + 1);
        const i32 left = bindings::widget_attrs::parseInt(text.substr(0, firstComma));
        const i32 top = bindings::widget_attrs::parseInt(text.substr(firstComma + 1, secondComma - firstComma - 1));
        const i32 right = bindings::widget_attrs::parseInt(thirdComma == std::string::npos
                ? text.substr(secondComma + 1)
                : text.substr(secondComma + 1, thirdComma - secondComma - 1));
        const i32 bottom =
            thirdComma == std::string::npos ? right : bindings::widget_attrs::parseInt(text.substr(thirdComma + 1));
        widget->setMargin(Margin(left, top, right, bottom));
    };

    // padding属性（格式：padding="10" / "h,v" / "l,t,r,b"）
    m_attributeSetters["padding"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            const std::string text = value.toString();
            size_t firstComma = text.find(',');
            if (firstComma == std::string::npos) {
                const i32 all = bindings::widget_attrs::parseInt(text);
                widget->setPadding(Padding(all));
                return;
            }
            size_t secondComma = text.find(',', firstComma + 1);
            if (secondComma == std::string::npos) {
                const i32 horizontal = bindings::widget_attrs::parseInt(text.substr(0, firstComma));
                const i32 vertical = bindings::widget_attrs::parseInt(text.substr(firstComma + 1));
                widget->setPadding(Padding(horizontal, vertical));
                return;
            }
            size_t thirdComma = text.find(',', secondComma + 1);
            const i32 left = bindings::widget_attrs::parseInt(text.substr(0, firstComma));
            const i32 top = bindings::widget_attrs::parseInt(text.substr(firstComma + 1, secondComma - firstComma - 1));
            const i32 right = bindings::widget_attrs::parseInt(thirdComma == std::string::npos
                    ? text.substr(secondComma + 1)
                    : text.substr(secondComma + 1, thirdComma - secondComma - 1));
            const i32 bottom =
                thirdComma == std::string::npos ? right : bindings::widget_attrs::parseInt(text.substr(thirdComma + 1));
            widget->setPadding(Padding(left, top, right, bottom));
        };

    // 背景色
    m_attributeSetters["background-color"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            widget->setBackgroundColor(bindings::widget_attrs::parseColor(value.toString()));
        };

    // 边框色
    m_attributeSetters["border-color"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            widget->setBorderColor(bindings::widget_attrs::parseColor(value.toString()));
        };

    // 圆角
    m_attributeSetters["corner-radius"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            widget->setCornerRadius(bindings::widget_attrs::parseInt(value.toString()));
        };

    // disabled属性
    m_attributeSetters["disabled"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            widget->setActive(!value.asBool());
        };

    // checked属性（用于CheckboxWidget）
    m_attributeSetters["checked"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            if (auto* checkbox = dynamic_cast<widget::CheckboxWidget*>(widget)) {
                checkbox->setChecked(value.asBool());
            }
        };

    // value属性（用于SliderWidget）
    m_attributeSetters["value"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* slider = dynamic_cast<widget::SliderWidget*>(widget)) {
            slider->setValue(static_cast<f64>(value.asFloat()));
        }
    };

    // min属性（用于SliderWidget）
    m_attributeSetters["min"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* slider = dynamic_cast<widget::SliderWidget*>(widget)) {
            slider->setMinValue(static_cast<f64>(value.asFloat()));
        }
    };

    // max属性（用于SliderWidget）
    m_attributeSetters["max"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* slider = dynamic_cast<widget::SliderWidget*>(widget)) {
            slider->setMaxValue(static_cast<f64>(value.asFloat()));
        }
    };

    // placeholder属性（用于TextFieldWidget）
    m_attributeSetters["placeholder"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            if (auto* textField = dynamic_cast<widget::TextFieldWidget*>(widget)) {
                textField->setPlaceholder(value.toString());
            }
        };

    // max-length属性（用于TextFieldWidget）
    m_attributeSetters["max-length"] =
        [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
            (void)attrName;
            if (auto* textField = dynamic_cast<widget::TextFieldWidget*>(widget)) {
                textField->setMaxLength(value.asInteger());
            }
        };

    // id属性（特殊处理）
    m_attributeSetters["id"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        widget->setId(value.toString());
    };

    // title属性（用于Screen标题）
    m_attributeSetters["title"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        widget->setUserData("title", value.toString());
    };

    // bind:items属性（用于ListWidget数据绑定）
    m_attributeSetters["items"] = [](widget::Widget* widget, const std::string& attrName, const binder::Value& value) {
        (void)attrName;
        if (auto* list = dynamic_cast<widget::ListWidget*>(widget)) {
            if (value.isArray()) {
                list->setItemsFromValue(value);
            }
        }
    };
}

void TemplateInstance::registerEventBinder(const std::string& eventName, EventBinder binder)
{
    m_eventBinders[eventName] = std::move(binder);
}

void TemplateInstance::registerDefaultEventBinders()
{
    // 确保BuiltinEvents已初始化
    bindings::BuiltinEvents::instance().initialize();

    // 点击事件
    m_eventBinders["click"] = [](widget::Widget* widget,
                                  const std::string& eventName,
                                  const std::string& callbackName,
                                  binder::BindingContext& ctx) {
        (void)eventName;
        if (auto* button = dynamic_cast<widget::ButtonWidget*>(widget)) {
            button->setOnPress([&ctx, callbackName](widget::ButtonWidget& btn) {
                ctx.invokeCallback(callbackName, &btn, event::ButtonClickEvent(&btn));
            });
        }
    };

    // 双击事件
    m_eventBinders["doubleClick"] = [](widget::Widget* widget,
                                        const std::string& eventName,
                                        const std::string& callbackName,
                                        binder::BindingContext& ctx) {
        (void)eventName;
        if (widget && !callbackName.empty()) {
            widget->setOnDoubleClickCallback([&ctx, callbackName, widget](widget::Widget& w) {
                ctx.invokeCallback(callbackName, &w, event::MouseClickEvent(0, 0, 0, 2));
            });
        }
    };

    // 右键点击事件
    m_eventBinders["rightClick"] = [](widget::Widget* widget,
                                       const std::string& eventName,
                                       const std::string& callbackName,
                                       binder::BindingContext& ctx) {
        (void)eventName;
        if (widget && !callbackName.empty()) {
            widget->setOnRightClickCallback([&ctx, callbackName, widget](widget::Widget& w) {
                ctx.invokeCallback(callbackName, &w, event::MouseClickEvent(0, 0, 1, 1));
            });
        }
    };

    // 鼠标进入事件：只注册回调，状态修改由Widget自身的事件处理管理
    m_eventBinders["mouseEnter"] = [](widget::Widget* widget,
                                       const std::string& eventName,
                                       const std::string& callbackName,
                                       binder::BindingContext& ctx) {
        (void)eventName;
        if (widget && !callbackName.empty()) {
            // 仅注册用户回调，hover状态由ContainerWidget::updateHover()管理
            widget->setUserData("onMouseEnterCallback", callbackName);
        }
    };

    // 鼠标离开事件：只注册回调，状态修改由Widget自身的事件处理管理
    m_eventBinders["mouseLeave"] = [](widget::Widget* widget,
                                       const std::string& eventName,
                                       const std::string& callbackName,
                                       binder::BindingContext& ctx) {
        (void)eventName;
        if (widget && !callbackName.empty()) {
            // 仅注册用户回调，hover状态由ContainerWidget::updateHover()管理
            widget->setUserData("onMouseLeaveCallback", callbackName);
        }
    };

    // 滚动事件
    m_eventBinders["scroll"] = [](widget::Widget* widget,
                                   const std::string& eventName,
                                   const std::string& callbackName,
                                   binder::BindingContext& ctx) {
        (void)eventName;
        if (auto* scrollable = dynamic_cast<widget::ScrollableWidget*>(widget)) {
            scrollable->setOnScroll([&ctx, callbackName, widget](i32 x, i32 y, f64 deltaX, f64 deltaY) {
                if (!callbackName.empty()) {
                    ctx.invokeCallback(callbackName, widget, event::MouseScrollEvent(x, y, deltaX, deltaY));
                }
            });
        }
    };

    // 键盘按下事件
    // TODO: 实现键盘事件绑定，当前仅标记Widget有回调，实际分发由屏幕/输入系统处理
    m_eventBinders["keyDown"] = [](widget::Widget* widget,
                                    const std::string& eventName,
                                    const std::string& callbackName,
                                    binder::BindingContext& ctx) {
        (void)eventName;
        (void)widget;
        (void)ctx;
        (void)callbackName;
    };

    // 键盘释放事件
    // TODO: 实现键盘事件绑定，当前暂未实现
    m_eventBinders["keyUp"] = [](widget::Widget* widget,
                                  const std::string& eventName,
                                  const std::string& callbackName,
                                  binder::BindingContext& ctx) {
        (void)eventName;
        (void)widget;
        (void)ctx;
        (void)callbackName;
    };

    // 焦点获得事件：只注册回调，焦点状态由ContainerWidget的焦点管理管理
    m_eventBinders["focus"] = [](widget::Widget* widget,
                                  const std::string& eventName,
                                  const std::string& callbackName,
                                  binder::BindingContext& ctx) {
        (void)eventName;
        if (widget && !callbackName.empty()) {
            widget->setUserData("onFocusGainedCallback", callbackName);
        }
    };

    // 失去焦点事件：只注册回调，焦点状态由ContainerWidget的焦点管理管理
    m_eventBinders["blur"] = [](widget::Widget* widget,
                                 const std::string& eventName,
                                 const std::string& callbackName,
                                 binder::BindingContext& ctx) {
        (void)eventName;
        if (widget && !callbackName.empty()) {
            widget->setUserData("onFocusLostCallback", callbackName);
        }
    };

    // 值变化事件
    m_eventBinders["change"] = [](widget::Widget* widget,
                                   const std::string& eventName,
                                   const std::string& callbackName,
                                   binder::BindingContext& ctx) {
        (void)eventName;
        if (callbackName.empty()) {
            return;
        }

        if (auto* checkbox = dynamic_cast<widget::CheckboxWidget*>(widget)) {
            checkbox->setOnChanged([widget, &ctx, callbackName](bool oldOrNewChecked) {
                (void)oldOrNewChecked;
                ctx.invokeCallback(callbackName, widget, event::CheckboxChangeEvent(false, oldOrNewChecked));
            });
            return;
        }

        if (auto* slider = dynamic_cast<widget::SliderWidget*>(widget)) {
            slider->setOnValueChanged([widget, &ctx, callbackName](f64 value) {
                ctx.invokeCallback(callbackName, widget, event::SliderValueChangeEvent(value, value));
            });
            return;
        }

        if (auto* textField = dynamic_cast<widget::TextFieldWidget*>(widget)) {
            textField->setTextChangedCallback([widget, &ctx, callbackName](const std::string& text) {
                ctx.invokeCallback(callbackName, widget, event::TextChangeEvent(text, text));
            });
        }
    };

    // 输入事件
    m_eventBinders["input"] = [](widget::Widget* widget,
                                  const std::string& eventName,
                                  const std::string& callbackName,
                                  binder::BindingContext& ctx) {
        (void)eventName;
        if (callbackName.empty()) {
            return;
        }
        if (auto* textField = dynamic_cast<widget::TextFieldWidget*>(widget)) {
            textField->setTextChangedCallback([widget, &ctx, callbackName](const std::string& text) {
                ctx.invokeCallback(callbackName, widget, event::CharInputEvent(0));
            });
        }
    };
}

bool TemplateInstance::instantiate()
{
    if (!m_compiled || !m_compiled->isValid()) {
        return false;
    }

    // 清理旧实例
    m_rootWidget.reset();
    m_widgetById.clear();
    m_widgetByPath.clear();

    // 清理旧订阅
    for (u64 id : m_subscriptionIds) {
        if (m_context) {
            m_context->unsubscribe(id);
        }
    }
    m_subscriptionIds.clear();

    // 清理调度器中遗留的待处理任务（避免重新实例化后执行到旧路径）
    m_scheduler.cancelAll();

    // 实例化根节点
    const ast::DocumentNode* doc = m_compiled->astRoot();
    if (!doc) return false;

    const ast::ElementNode* rootElement = doc->rootElement();
    if (!rootElement) return false;

    m_rootWidget = _instantiateElement(rootElement, nullptr);

    if (!m_rootWidget) {
        return false;
    }

    // 设置状态变更订阅
    for (const auto& path : m_compiled->watchedPaths()) {
        u64 subId =
            m_context->subscribe(path, [this](const std::string& p, const binder::Value&) { notifyStateChange(p); });
        m_subscriptionIds.push_back(subId);
    }

    // 绑定调度器回调：将路径入队转交给 updateBinding
    _rebindSchedulerCallback();

    return true;
}

bool TemplateInstance::instantiateInto(widget::IWidgetContainer* container)
{
    if (!instantiate()) {
        return false;
    }

    if (!container) {
        return false;
    }

    container->addWidget(std::move(m_rootWidget));
    return true;
}

void TemplateInstance::updateBindings()
{
    if (!m_compiled || !m_context) return;

    for (const auto& plan : m_compiled->bindingPlans()) {
        auto it = m_widgetByPath.find(plan.widgetPath);
        if (it == m_widgetByPath.end()) {
            it = m_widgetById.find(plan.widgetPath);
            if (it == m_widgetById.end()) continue;
        }

        widget::Widget* widget = it->second;
        if (!widget) continue;

        binder::Value value = m_context->resolveBinding(plan.statePath, plan.loopVarName, binder::Value());

        auto setterIt = m_attributeSetters.find(plan.attributeName);
        if (setterIt != m_attributeSetters.end()) {
            setterIt->second(widget, plan.attributeName, value);
        }
    }
}

bool TemplateInstance::updateBinding(const std::string& path)
{
    if (!m_compiled || !m_context) return false;

    bool updated = false;
    for (const auto& plan : m_compiled->bindingPlans()) {
        if (plan.statePath != path) continue;

        auto it = m_widgetByPath.find(plan.widgetPath);
        if (it == m_widgetByPath.end()) {
            it = m_widgetById.find(plan.widgetPath);
            if (it == m_widgetById.end()) continue;
        }

        widget::Widget* widget = it->second;
        if (!widget) continue;

        binder::Value value = m_context->resolveBinding(plan.statePath, plan.loopVarName, binder::Value());

        auto setterIt = m_attributeSetters.find(plan.attributeName);
        if (setterIt != m_attributeSetters.end()) {
            setterIt->second(widget, plan.attributeName, value);
            updated = true;
        }
    }

    return updated;
}

void TemplateInstance::notifyStateChange(const std::string& path)
{
    // 将路径入队到调度器，由 tick() / flush() 统一调度执行
    // 调度器会根据 deferredUpdate 和 batchDelayMs 决定执行时机
    // 同路径多次入队会被去重（只保留最新）
    m_scheduler.schedule(path, UpdateScheduler::Priority::Normal);
}

void TemplateInstance::refresh()
{
    // 立即执行所有待处理任务（无视延迟）
    flushPending();
    // 全量刷新绑定
    updateBindings();
}

u32 TemplateInstance::tick(u64 currentMs)
{
    return m_scheduler.tick(currentMs);
}

u32 TemplateInstance::flushPending()
{
    return m_scheduler.flush();
}

void TemplateInstance::_rebindSchedulerCallback()
{
    // 调度器回调捕获 this 指针，移动构造/赋值后需重新绑定
    m_scheduler.setUpdateCallback([this](const std::string& path) -> bool {
        if (!m_compiled || !m_context) return false;
        return updateBinding(path);
    });
}

widget::Widget* TemplateInstance::findWidgetById(const std::string& id)
{
    auto it = m_widgetById.find(id);
    return it != m_widgetById.end() ? it->second : nullptr;
}

widget::Widget* TemplateInstance::findWidgetByPath(const std::string& path)
{
    auto it = m_widgetByPath.find(path);
    return it != m_widgetByPath.end() ? it->second : nullptr;
}

std::string TemplateInstance::debugInfo() const
{
    std::ostringstream oss;
    oss << "TemplateInstance:\n";
    oss << "  Valid: " << (m_compiled && m_compiled->isValid() ? "Yes" : "No") << "\n";
    oss << "  Root Widget: " << (m_rootWidget ? "Yes" : "No") << "\n";
    oss << "  Widgets by ID: " << m_widgetById.size() << "\n";
    oss << "  Widgets by Path: " << m_widgetByPath.size() << "\n";
    oss << "  Subscriptions: " << m_subscriptionIds.size() << "\n";
    oss << "  Scheduler pending: " << m_scheduler.pendingCount() << "\n";
    oss << "  Scheduler deferred: " << (m_scheduler.deferredUpdate() ? "Yes" : "No") << "\n";
    oss << "  Scheduler batchDelay: " << m_scheduler.batchDelay() << "ms\n";
    return oss.str();
}

std::unique_ptr<widget::Widget> TemplateInstance::_instantiateNode(const ast::Node* node, widget::Widget* parent)
{
    if (!node) return nullptr;

    switch (node->type) {
        case ast::NodeType::TextContent:
            return _instantiateText(static_cast<const ast::TextNode*>(node), parent);

        case ast::NodeType::Comment:
            // 跳过注释
            return nullptr;

        default:
            if (auto* element = dynamic_cast<const ast::ElementNode*>(node)) {
                return _instantiateElement(element, parent);
            }
            break;
    }

    return nullptr;
}

std::unique_ptr<widget::Widget> TemplateInstance::_instantiateElement(
    const ast::ElementNode* element, widget::Widget* parent)
{

    if (!element) return nullptr;

    // 1. 条件渲染检查（优先）
    if (element->condition.has_value()) {
        if (!_evaluateCondition(element->condition.value())) {
            return nullptr; // 条件不满足，跳过创建
        }
    }

    // 2. 循环渲染检查
    if (element->loop.has_value()) {
        // 循环元素：为集合中每个项创建子元素
        _instantiateLoopChildren(
            element, parent, element->loop->collectionPath, element->loop->itemVarName, element->loop->indexVarName);
        return nullptr; // 循环容器本身不返回Widget
    }

    // 3. 收集属性
    std::map<std::string, std::string> staticAttrs;
    for (const auto& attr : element->staticAttrs) {
        staticAttrs[attr.name] = attr.rawValue;
    }

    // 4. 创建Widget
    std::string id = element->id.empty() ? "" : element->id;
    auto widget = _createWidget(element->tagName, id, staticAttrs);
    if (!widget) return nullptr;

    // 5. 设置父Widget容器
    if (auto* containerParent = dynamic_cast<widget::IWidgetContainer*>(parent)) {
        widget->setParent(containerParent);
    }

    // 6. 注册Widget
    std::string widgetPath = _buildWidgetPath(element, parent ? parent->id() : "");
    _registerWidgetPath(widgetPath, widget.get());
    if (!id.empty()) {
        _registerWidgetId(id, widget.get());
    }

    // 7. 应用静态属性
    _applyStaticAttributes(widget.get(), element->staticAttrs);

    // 8. 应用绑定属性（初始值）
    _applyBindingAttributes(widget.get(), element->bindingAttrs, widgetPath);

    // 9. 应用事件绑定
    _applyEventBindings(widget.get(), element->eventAttrs, widgetPath);

    // 10. 实例化子节点
    for (const auto& child : element->children) {
        auto childWidget = _instantiateNode(child.get(), widget.get());
        if (childWidget) {
            // 如果Widget是容器，添加子Widget
            if (auto* container = dynamic_cast<widget::IWidgetContainer*>(widget.get())) {
                container->addWidget(std::move(childWidget));
            }
        }
    }

    return widget;
}

std::unique_ptr<widget::Widget> TemplateInstance::_instantiateText(
    const ast::TextNode* textNode, widget::Widget* parent)
{

    if (!textNode || textNode->isWhitespace) return nullptr;

    auto widget = std::make_unique<widget::TextWidget>();
    widget->setText(textNode->text);
    if (auto* containerParent = dynamic_cast<widget::IWidgetContainer*>(parent)) {
        widget->setParent(containerParent);
    }

    return widget;
}

std::unique_ptr<widget::Widget> TemplateInstance::_createWidget(
    const std::string& tagName, const std::string& id, const std::map<std::string, std::string>& attrs)
{

    // 1. 首先使用BuiltinWidgets单例
    auto widget = bindings::BuiltinWidgets::instance().create(tagName, id, attrs);
    if (widget) {
        return widget;
    }

    // 2. 然后使用自定义工厂
    auto it = m_widgetFactories.find(tagName);
    if (it != m_widgetFactories.end()) {
        return it->second(tagName, id, attrs);
    }

    // 3. 最后使用默认工厂
    if (m_defaultFactory) {
        return m_defaultFactory(tagName, id, attrs);
    }

    return nullptr;
}

void TemplateInstance::_applyStaticAttributes(widget::Widget* widget, const std::vector<ast::Attribute>& attrs)
{
    if (!widget) return;

    for (const auto& attr : attrs) {
        auto setterIt = m_attributeSetters.find(attr.name);
        if (setterIt != m_attributeSetters.end()) {
            binder::Value value = _parseStaticValue(attr);
            setterIt->second(widget, attr.name, value);
        }
    }
}

void TemplateInstance::_applyBindingAttributes(
    widget::Widget* widget, const std::vector<ast::Attribute>& attrs, const std::string& widgetPath)
{
    if (!widget || !m_context) return;

    for (const auto& attr : attrs) {
        if (!attr.binding.has_value()) continue;

        // 解析绑定值，支持循环变量
        binder::Value value;

        // 检查绑定路径是否是循环变量引用
        const std::string& bindingPath = attr.binding->path;
        if (!bindingPath.empty() && bindingPath[0] == '$') {
            // 循环变量引用
            std::string varName;
            std::string property;
            size_t dotPos = bindingPath.find('.');
            if (dotPos != std::string::npos) {
                varName = bindingPath.substr(1, dotPos - 1);
                property = bindingPath.substr(dotPos + 1);
            } else {
                varName = bindingPath.substr(1);
            }

            // 获取循环变量值
            binder::Value loopValue = m_context->getLoopVariable(varName);
            if (!loopValue.isNull()) {
                if (property.empty()) {
                    value = loopValue;
                } else {
                    value = loopValue.getProperty(property);
                }
            }
        } else {
            // 普通绑定路径
            value = m_context->resolveBinding(bindingPath);
        }

        // 应用属性
        auto setterIt = m_attributeSetters.find(attr.baseName());
        if (setterIt != m_attributeSetters.end()) {
            setterIt->second(widget, attr.baseName(), value);
        }
    }
}

void TemplateInstance::_applyEventBindings(
    widget::Widget* widget, const std::vector<ast::Attribute>& attrs, const std::string& widgetPath)
{
    if (!widget || !m_context) return;

    for (const auto& attr : attrs) {
        std::string eventName = attr.baseName();
        std::string callbackName = attr.callbackName;

        auto binderIt = m_eventBinders.find(eventName);
        if (binderIt != m_eventBinders.end()) {
            binderIt->second(widget, eventName, callbackName, *m_context);
        }
    }
}

binder::Value TemplateInstance::_parseStaticValue(const ast::Attribute& attr) const
{
    // 根据属性值类型创建Value
    if (std::holds_alternative<std::string>(attr.value)) {
        return binder::Value(std::get<std::string>(attr.value));
    } else if (std::holds_alternative<i32>(attr.value)) {
        return binder::Value(std::get<i32>(attr.value));
    } else if (std::holds_alternative<f32>(attr.value)) {
        return binder::Value(std::get<f32>(attr.value));
    } else if (std::holds_alternative<bool>(attr.value)) {
        return binder::Value(std::get<bool>(attr.value));
    }
    return binder::Value();
}

std::string TemplateInstance::_buildWidgetPath(const ast::ElementNode* element, const std::string& parentPath) const
{
    if (!element) return parentPath;

    if (!element->id.empty()) {
        return parentPath.empty() ? element->id : parentPath + "." + element->id;
    }

    return parentPath.empty() ? element->tagName : parentPath + "." + element->tagName;
}

void TemplateInstance::_registerWidgetPath(const std::string& path, widget::Widget* widget)
{
    m_widgetByPath[path] = widget;
}

void TemplateInstance::_registerWidgetId(const std::string& id, widget::Widget* widget)
{
    m_widgetById[id] = widget;
}

void TemplateInstance::_instantiateLoopChildren(const ast::ElementNode* element,
    widget::Widget* parent,
    const std::string& collectionPath,
    const std::string& itemVarName,
    const std::string& indexVarName)
{

    if (!element || !m_context) return;

    // 解析集合
    auto collection = _resolveCollection(collectionPath);

    // 获取父容器
    auto* container = dynamic_cast<widget::IWidgetContainer*>(parent);
    if (!container) return;

    // 为每个元素创建子节点
    for (size_t i = 0; i < collection.size(); ++i) {
        // 设置循环变量
        m_context->setLoopVariable(itemVarName, collection[i]);
        if (!indexVarName.empty()) {
            m_context->setLoopVariable(indexVarName, binder::Value(static_cast<i32>(i)));
        }

        // 实例化子节点
        for (const auto& child : element->children) {
            auto childWidget = _instantiateNode(child.get(), parent);
            if (childWidget) {
                container->addWidget(std::move(childWidget));
            }
        }

        // 清除循环变量
        m_context->clearLoopVariable(itemVarName);
        if (!indexVarName.empty()) {
            m_context->clearLoopVariable(indexVarName);
        }
    }
}

std::vector<binder::Value> TemplateInstance::_resolveCollection(const std::string& path) const
{
    if (!m_context) return {};

    return m_context->resolveCollection(path);
}

bool TemplateInstance::_evaluateCondition(const ast::ConditionInfo& condition) const
{
    if (!m_context) return false;

    auto value = m_context->resolveBinding(condition.booleanPath);
    bool visible = value.asBool();

    if (condition.negate) {
        visible = !visible;
    }

    return visible;
}

} // namespace mc::client::ui::kagero::tpl::runtime
