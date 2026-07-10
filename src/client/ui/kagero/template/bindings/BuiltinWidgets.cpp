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

#include "BuiltinWidgets.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace mc::client::ui::kagero::tpl::bindings {

// ========== BuiltinWidgets实现 ==========

BuiltinWidgets& BuiltinWidgets::instance()
{
    static BuiltinWidgets instance;
    return instance;
}

BuiltinWidgets::BuiltinWidgets()
{
    // 延迟初始化
}

void BuiltinWidgets::initialize()
{
    if (m_initialized) return;

    _registerScreenWidget();
    _registerContainerWidget();
    _registerButtonWidget();
    _registerTextWidget();
    _registerTextFieldWidget();
    _registerSliderWidget();
    _registerCheckboxWidget();
    _registerImageWidget();
    _registerGridWidget();
    _registerSlotWidget();
    _registerScrollableWidget();
    _registerListWidget();
    _registerViewport3DWidget();

    m_initialized = true;
}

void BuiltinWidgets::registerCreator(const std::string& tagName, WidgetCreator creator)
{
    m_creators[tagName] = std::move(creator);
}

std::unique_ptr<widget::Widget> BuiltinWidgets::create(
    const std::string& tagName, const std::string& id, const std::map<std::string, std::string>& attrs) const
{

    auto it = m_creators.find(tagName);
    if (it == m_creators.end()) {
        return nullptr;
    }

    auto widget = it->second(id, attrs);

    if (widget) {
        auto posIt = attrs.find("pos");
        if (posIt != attrs.end()) {
            if (widget_attrs::hasPercentValue(posIt->second)) {
                // 百分比位置存储到 userData，待父容器尺寸确定后解析
                widget->setUserData("__pos_percent", posIt->second);
            } else {
                widget_attrs::applyPosition(widget.get(), posIt->second);
            }
        }

        auto sizeIt = attrs.find("size");
        if (sizeIt != attrs.end()) {
            if (widget_attrs::hasPercentValue(sizeIt->second)) {
                // 百分比尺寸存储到 userData，待父容器尺寸确定后解析
                widget->setUserData("__size_percent", sizeIt->second);
                // 同时设置像素值为临时值（百分比解析前使用）
                widget_attrs::applySize(widget.get(), sizeIt->second);
            } else {
                widget_attrs::applySize(widget.get(), sizeIt->second);
            }
        }

        auto visibleIt = attrs.find("visible");
        if (visibleIt != attrs.end()) {
            widget->setVisible(widget_attrs::parseBool(visibleIt->second));
        }

        auto activeIt = attrs.find("active");
        if (activeIt != attrs.end()) {
            widget->setActive(widget_attrs::parseBool(activeIt->second));
        }

        auto anchorIt = attrs.find("anchor");
        if (anchorIt != attrs.end()) {
            widget->setAnchor(widget_attrs::parseAnchor(anchorIt->second));
        }

        auto zIndexIt = attrs.find("zIndex");
        if (zIndexIt != attrs.end()) {
            widget->setZIndex(widget_attrs::parseInt(zIndexIt->second));
        }

        auto alphaIt = attrs.find("alpha");
        if (alphaIt != attrs.end()) {
            widget->setAlpha(widget_attrs::parseFloat(alphaIt->second, 1.0f));
        }

        auto layoutIt = attrs.find("layout");
        if (layoutIt != attrs.end()) {
            std::string layout = layoutIt->second;
            std::transform(layout.begin(), layout.end(), layout.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (auto* container = dynamic_cast<widget::ContainerWidget*>(widget.get())) {
                if (layout == "flex" || layout == "flex-row") {
                    container->setLayoutType(widget::ContainerLayoutType::Flex);
                    auto config = container->flexConfig();
                    config.direction = layout::Direction::Row;
                    container->setFlexConfig(config);
                } else if (layout == "flex-column") {
                    container->setLayoutType(widget::ContainerLayoutType::Flex);
                    auto config = container->flexConfig();
                    config.direction = layout::Direction::Column;
                    container->setFlexConfig(config);
                } else if (layout == "flex-row-reverse") {
                    container->setLayoutType(widget::ContainerLayoutType::Flex);
                    auto config = container->flexConfig();
                    config.direction = layout::Direction::RowReverse;
                    container->setFlexConfig(config);
                } else if (layout == "flex-column-reverse") {
                    container->setLayoutType(widget::ContainerLayoutType::Flex);
                    auto config = container->flexConfig();
                    config.direction = layout::Direction::ColumnReverse;
                    container->setFlexConfig(config);
                } else if (layout == "flex-center") {
                    container->setLayoutType(widget::ContainerLayoutType::Flex);
                    auto config = layout::centerColumnFlexConfig();
                    container->setFlexConfig(config);
                } else if (layout == "grid") {
                    container->setLayoutType(widget::ContainerLayoutType::Grid);
                } else if (layout == "anchor") {
                    container->setLayoutType(widget::ContainerLayoutType::Anchor);
                }
            }
        }

        auto gapIt = attrs.find("gap");
        if (gapIt != attrs.end()) {
            if (auto* container = dynamic_cast<widget::ContainerWidget*>(widget.get())) {
                auto config = container->flexConfig();
                config.gap = widget_attrs::parseInt(gapIt->second);
                container->setFlexConfig(config);
            }
        }

        auto alignItemsIt = attrs.find("align-items");
        if (alignItemsIt != attrs.end()) {
            if (auto* container = dynamic_cast<widget::ContainerWidget*>(widget.get())) {
                auto config = container->flexConfig();
                std::string align = alignItemsIt->second;
                std::transform(align.begin(), align.end(), align.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
                if (align == "start") {
                    config.alignItems = layout::Align::Start;
                } else if (align == "center") {
                    config.alignItems = layout::Align::Center;
                } else if (align == "end") {
                    config.alignItems = layout::Align::End;
                } else if (align == "stretch") {
                    config.alignItems = layout::Align::Stretch;
                } else if (align == "baseline") {
                    config.alignItems = layout::Align::Baseline;
                }
                container->setFlexConfig(config);
            }
        }

        auto justifyContentIt = attrs.find("justify-content");
        if (justifyContentIt != attrs.end()) {
            if (auto* container = dynamic_cast<widget::ContainerWidget*>(widget.get())) {
                auto config = container->flexConfig();
                std::string justify = justifyContentIt->second;
                std::transform(justify.begin(), justify.end(), justify.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
                if (justify == "start") {
                    config.justifyContent = layout::JustifyContent::Start;
                } else if (justify == "center") {
                    config.justifyContent = layout::JustifyContent::Center;
                } else if (justify == "end") {
                    config.justifyContent = layout::JustifyContent::End;
                } else if (justify == "space-between") {
                    config.justifyContent = layout::JustifyContent::SpaceBetween;
                } else if (justify == "space-around") {
                    config.justifyContent = layout::JustifyContent::SpaceAround;
                } else if (justify == "space-evenly") {
                    config.justifyContent = layout::JustifyContent::SpaceEvenly;
                }
                container->setFlexConfig(config);
            }
        }
    }

    return widget;
}

bool BuiltinWidgets::hasTag(const std::string& tagName) const
{
    return m_creators.find(tagName) != m_creators.end();
}

std::vector<std::string> BuiltinWidgets::registeredTags() const
{
    std::vector<std::string> tags;
    tags.reserve(m_creators.size());
    for (const auto& [tag, creator] : m_creators) {
        tags.push_back(tag);
    }
    return tags;
}

std::map<std::string, std::string> BuiltinWidgets::getDefaultAttributes(const std::string& tagName) const
{
    auto it = m_defaultAttributes.find(tagName);
    return it != m_defaultAttributes.end() ? it->second : std::map<std::string, std::string>();
}

void BuiltinWidgets::_registerScreenWidget()
{
    m_creators["screen"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto widget = std::make_unique<widget::ContainerWidget>(id.empty() ? "screen" : id);

        // screen 默认使用垂直居中 Flex 布局，使子元素自动居中排列
        widget->setLayoutType(widget::ContainerLayoutType::Flex);
        widget->setFlexConfig(layout::centerColumnFlexConfig());

        // 将 screen 属性存储到 userData，供 TemplateScreen 读取
        auto titleIt = attrs.find("title");
        if (titleIt != attrs.end()) {
            widget->setUserData("title", titleIt->second);
        }
        auto modalIt = attrs.find("modal");
        if (modalIt != attrs.end()) {
            widget->setUserData("modal", modalIt->second);
        }

        // 解析 background-color 属性
        auto bgColorIt = attrs.find("background-color");
        if (bgColorIt != attrs.end()) {
            widget->setBackgroundColor(widget_attrs::parseColor(bgColorIt->second));
        }

        return widget;
    };

    m_defaultAttributes["screen"] = {{"size", "auto,auto"}};
}

void BuiltinWidgets::_registerContainerWidget()
{
    m_creators["container"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto widget = std::make_unique<widget::ContainerWidget>(id.empty() ? "container" : id);

        // 注意：layout/gap/align-items/justify-content 属性的解析由 BuiltinWidgets::create() 统一处理
        // 此处仅处理 container 特有的属性

        return widget;
    };

    m_defaultAttributes["container"] = {{"size", "auto,auto"}};
}

void BuiltinWidgets::_registerButtonWidget()
{
    m_creators["button"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto button = std::make_unique<widget::ButtonWidget>();

        if (!id.empty()) {
            button->setId(id);
        }

        auto textIt = attrs.find("text");
        if (textIt != attrs.end()) {
            button->setText(textIt->second);
        }

        return button;
    };

    m_defaultAttributes["button"] = {{"size", "200,20"}};
}

void BuiltinWidgets::_registerTextWidget()
{
    m_creators["text"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto text = std::make_unique<widget::TextWidget>();

        if (!id.empty()) {
            text->setId(id);
        }

        auto textContentIt = attrs.find("text");
        if (textContentIt != attrs.end()) {
            text->setText(textContentIt->second);
        }

        auto colorIt = attrs.find("color");
        if (colorIt != attrs.end()) {
            text->setColor(widget_attrs::parseColor(colorIt->second));
        }

        auto shadowIt = attrs.find("shadow");
        if (shadowIt != attrs.end()) {
            text->setShadow(widget_attrs::parseBool(shadowIt->second));
        }

        auto alignIt = attrs.find("align");
        if (alignIt != attrs.end()) {
            std::string align = alignIt->second;
            std::transform(align.begin(), align.end(), align.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (align == "left") {
                text->setAlignment(widget::TextAlignment::Left);
            } else if (align == "center") {
                text->setAlignment(widget::TextAlignment::Center);
            } else if (align == "right") {
                text->setAlignment(widget::TextAlignment::Right);
            }
        }

        auto scaleIt = attrs.find("scale");
        if (scaleIt != attrs.end()) {
            text->setScale(widget_attrs::parseFloat(scaleIt->second, 1.0f));
        }

        return text;
    };

    m_defaultAttributes["text"] = {{"color", "#FFFFFF"}, {"shadow", "true"}};
}

void BuiltinWidgets::_registerTextFieldWidget()
{
    m_creators["textfield"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto textField = std::make_unique<widget::TextFieldWidget>();

        if (!id.empty()) {
            textField->setId(id);
        }

        auto placeholderIt = attrs.find("placeholder");
        if (placeholderIt != attrs.end()) {
            textField->setPlaceholder(placeholderIt->second);
        }

        auto maxLengthIt = attrs.find("maxLength");
        if (maxLengthIt == attrs.end()) {
            maxLengthIt = attrs.find("max-length");
        }
        if (maxLengthIt != attrs.end()) {
            textField->setMaxLength(widget_attrs::parseInt(maxLengthIt->second, textField->maxLength()));
        }

        return textField;
    };

    m_defaultAttributes["textfield"] = {{"size", "200,20"}};
}

void BuiltinWidgets::_registerSliderWidget()
{
    m_creators["slider"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto slider = std::make_unique<widget::SliderWidget>();

        if (!id.empty()) {
            slider->setId(id);
        }

        auto rangeIt = attrs.find("range");
        if (rangeIt != attrs.end()) {
            auto [min, max] = widget_attrs::parseRange(rangeIt->second);
            slider->setRange(min, max);
        }

        auto valueIt = attrs.find("value");
        if (valueIt != attrs.end()) {
            slider->setValue(widget_attrs::parseFloat(valueIt->second, static_cast<f32>(slider->value())));
        }

        return slider;
    };

    m_defaultAttributes["slider"] = {{"size", "200,20"}, {"range", "0,100"}};
}

void BuiltinWidgets::_registerCheckboxWidget()
{
    m_creators["checkbox"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto checkbox = std::make_unique<widget::CheckboxWidget>();

        if (!id.empty()) {
            checkbox->setId(id);
        }

        auto checkedIt = attrs.find("checked");
        if (checkedIt != attrs.end()) {
            checkbox->setChecked(widget_attrs::parseBool(checkedIt->second));
        }

        return checkbox;
    };

    m_defaultAttributes["checkbox"] = {{"size", "20,20"}};
}

void BuiltinWidgets::_registerImageWidget()
{
    m_creators["image"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto widget = std::make_unique<widget::ImageWidget>(id.empty() ? "image" : id);

        // ImageWidget 构造函数已默认 setActive(false)（对齐 MC Java ImageWidget.isActive()==false 语义）

        // 解析 src 属性：精灵ID（图集由外部通过 setSpriteSource/setAtlas 注入）
        // 不在此处绑定图集，因为 BuiltinWidgets 工厂无法访问运行期 GuiSpriteAtlas。
        // 外部（如 TemplateScreen / 业务代码）在创建后调用 setSpriteSource 设置来源。
        auto srcIt = attrs.find("src");
        if (srcIt != attrs.end() && !srcIt->second.empty()) {
            widget->setSpriteId(srcIt->second);
        }

        // 解析 tint 属性：着色（ARGB）
        auto tintIt = attrs.find("tint");
        if (tintIt != attrs.end()) {
            widget->setTint(widget_attrs::parseColor(tintIt->second));
        }

        // 解析 size 属性：支持 "auto" 关键字自适应纹理尺寸
        // 注意：通用 create() 随后会再次调用 applySize，将 "auto" 解析为 0；
        // 但 _resolveDrawRect 在绘制时会根据 m_autoWidth/m_autoHeight 重新计算，
        // 因此此处仅需设置自动标志，尺寸回退到 0 不影响最终绘制。
        auto sizeIt = attrs.find("size");
        if (sizeIt != attrs.end()) {
            const std::string& sizeValue = sizeIt->second;
            // 简单子串匹配 "auto"（兼容 "auto,auto"、"auto,100"、"100,auto"）
            const bool widthAuto = sizeValue.find("auto") != std::string::npos;
            // 仅当第一个分量（宽度）为 auto 时设置 autoWidth
            size_t comma = sizeValue.find(',');
            if (comma != std::string::npos) {
                std::string wPart = sizeValue.substr(0, comma);
                std::string hPart = sizeValue.substr(comma + 1);
                // 去除空格
                auto trim = [](std::string& s) {
                    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
                        s.pop_back();
                    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
                        s.erase(s.begin());
                };
                trim(wPart);
                trim(hPart);
                if (wPart == "auto") {
                    widget->setAutoWidth(true);
                }
                if (hPart == "auto") {
                    widget->setAutoHeight(true);
                }
            } else if (widthAuto) {
                // 整个值为 "auto"
                widget->setAutoSize(true);
            }
        }

        return widget;
    };

    m_defaultAttributes["image"] = {{"size", "auto,auto"}};
}

void BuiltinWidgets::_registerGridWidget()
{
    m_creators["grid"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto widget = std::make_unique<widget::ContainerWidget>(id.empty() ? "grid" : id);
        widget->setLayoutType(widget::ContainerLayoutType::Grid);

        // 解析 cols/rows/gap 属性并应用到 GridConfig
        // - cols：列数，最小为 1（GridLayout 内部会再次钳制）
        // - rows：行数，0 表示根据子项数量自动推算（_resolveRows）
        // - gap：列间距与行间距的统一值，同时写入 columnGap 与 rowGap
        layout::GridConfig config = widget->gridConfig();

        auto colsIt = attrs.find("cols");
        if (colsIt != attrs.end()) {
            config.columns = std::max(1, widget_attrs::parseInt(colsIt->second, 1));
        }
        auto rowsIt = attrs.find("rows");
        if (rowsIt != attrs.end()) {
            config.rows = std::max(0, widget_attrs::parseInt(rowsIt->second, 0));
        }
        auto gapIt = attrs.find("gap");
        if (gapIt != attrs.end()) {
            const i32 gap = std::max(0, widget_attrs::parseInt(gapIt->second, 0));
            config.columnGap = gap;
            config.rowGap = gap;
        }

        widget->setGridConfig(config);

        return widget;
    };

    m_defaultAttributes["grid"] = {{"cols", "1"}, {"rows", "1"}};
}

void BuiltinWidgets::_registerSlotWidget()
{
    m_creators["slot"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto slot = std::make_unique<widget::SlotWidget>();

        if (!id.empty()) {
            slot->setId(id);
        }

        // 解析 index 属性并应用到 SlotWidget
        // 默认值取 slot->slotIndex()（-1），保证属性缺失时保持默认行为
        auto indexIt = attrs.find("index");
        if (indexIt != attrs.end()) {
            slot->setSlotIndex(widget_attrs::parseInt(indexIt->second, slot->slotIndex()));
        }

        return slot;
    };

    m_defaultAttributes["slot"] = {{"size", "18,18"}};
}

void BuiltinWidgets::_registerScrollableWidget()
{
    m_creators["scrollable"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto scrollable = std::make_unique<widget::ScrollableWidget>();

        if (!id.empty()) {
            scrollable->setId(id);
        }

        // 解析属性
        for (const auto& [key, value] : attrs) {
            if (key == "content-height") {
                scrollable->setContentHeight(widget_attrs::parseInt(value));
            } else if (key == "content-width") {
                scrollable->setContentWidth(widget_attrs::parseInt(value));
            } else if (key == "scroll-speed") {
                scrollable->setScrollSpeed(static_cast<f64>(widget_attrs::parseFloat(value)));
            } else if (key == "show-scrollbar") {
                scrollable->setShowScrollbar(widget_attrs::parseBool(value));
            } else if (key == "show-horizontal-scrollbar") {
                scrollable->setShowHorizontalScrollbar(widget_attrs::parseBool(value));
            }
        }

        return scrollable;
    };
}

void BuiltinWidgets::_registerListWidget()
{
    m_creators["list"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto list = std::make_unique<widget::ListWidget>();

        if (!id.empty()) {
            list->setId(id);
        }

        return list;
    };
}

void BuiltinWidgets::_registerViewport3DWidget()
{
    m_creators["viewport3d"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto viewport = std::make_unique<widget::Viewport3DWidget>();

        if (!id.empty()) {
            viewport->setId(id);
        }

        return viewport;
    };

    m_defaultAttributes["viewport3d"] = {{"size", "100,100"}};
}

// ========== ParsedValue实现 ==========

ParsedValue::ParsedValue(i32 pixels)
    : pixels(pixels)
    , percent(-1.0f)
    , isPercent(false)
{}

ParsedValue::ParsedValue(f32 pct)
    : pixels(0)
    , percent(pct)
    , isPercent(true)
{}

// ========== widget_attrs实现 ==========

namespace widget_attrs {

std::pair<i32, i32> parsePosition(const std::string& value)
{
    size_t comma = value.find(',');
    if (comma == std::string::npos) {
        return {0, 0};
    }

    try {
        i32 x = std::stoi(value.substr(0, comma));
        i32 y = std::stoi(value.substr(comma + 1));
        return {x, y};
    }
    catch (...) {
        return {0, 0};
    }
}

std::pair<i32, i32> parseSize(const std::string& value)
{
    return parsePosition(value);
}

ParsedSize parseSizeEx(const std::string& value)
{
    size_t comma = value.find(',');
    if (comma == std::string::npos) {
        return {ParsedValue(0), ParsedValue(0)};
    }

    return {parseSingleValue(value.substr(0, comma)), parseSingleValue(value.substr(comma + 1))};
}

ParsedSize parsePositionEx(const std::string& value)
{
    return parseSizeEx(value);
}

ParsedValue parseSingleValue(const std::string& str)
{
    std::string trimmed = str;
    // 去除首尾空格
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
        trimmed.pop_back();
    }
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
        trimmed.erase(trimmed.begin());
    }

    if (!trimmed.empty() && trimmed.back() == '%') {
        try {
            f32 pct = std::stof(trimmed.substr(0, trimmed.size() - 1));
            return ParsedValue(pct);
        }
        catch (...) {
            return ParsedValue(0);
        }
    }

    try {
        return ParsedValue(std::stoi(trimmed));
    }
    catch (...) {
        return ParsedValue(0);
    }
}

void applyPosition(widget::Widget* widget, const std::string& value)
{
    MC_ASSERT_RELEASE(widget != nullptr);
    auto [x, y] = parsePosition(value);
    widget->setPosition(x, y);
}

void applySize(widget::Widget* widget, const std::string& value)
{
    MC_ASSERT_RELEASE(widget != nullptr);
    auto [width, height] = parseSize(value);
    widget->setSize(width, height);
}

void applySizeWithParent(widget::Widget* widget, const std::string& value, i32 parentWidth, i32 parentHeight)
{
    MC_ASSERT_RELEASE(widget != nullptr);
    auto parsed = parseSizeEx(value);
    i32 w = parsed.width.isPercent ? static_cast<i32>(parsed.width.percent / 100.0f * static_cast<f32>(parentWidth))
                                   : parsed.width.pixels;
    i32 h = parsed.height.isPercent ? static_cast<i32>(parsed.height.percent / 100.0f * static_cast<f32>(parentHeight))
                                    : parsed.height.pixels;
    widget->setSize(w, h);
}

void applyPositionWithParent(widget::Widget* widget, const std::string& value, i32 parentWidth, i32 parentHeight)
{
    MC_ASSERT_RELEASE(widget != nullptr);
    auto parsed = parsePositionEx(value);
    i32 x = parsed.width.isPercent ? static_cast<i32>(parsed.width.percent / 100.0f * static_cast<f32>(parentWidth))
                                   : parsed.width.pixels;
    i32 y = parsed.height.isPercent ? static_cast<i32>(parsed.height.percent / 100.0f * static_cast<f32>(parentHeight))
                                    : parsed.height.pixels;
    widget->setPosition(x, y);
}

bool hasPercentValue(const std::string& value)
{
    size_t comma = value.find(',');
    if (comma == std::string::npos) {
        std::string trimmed = value;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
            trimmed.pop_back();
        }
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
            trimmed.erase(trimmed.begin());
        }
        return !trimmed.empty() && trimmed.back() == '%';
    }
    std::string left = value.substr(0, comma);
    std::string right = value.substr(comma + 1);
    while (!left.empty() && std::isspace(static_cast<unsigned char>(left.back()))) {
        left.pop_back();
    }
    while (!right.empty() && std::isspace(static_cast<unsigned char>(right.back()))) {
        right.pop_back();
    }
    return (!left.empty() && left.back() == '%') || (!right.empty() && right.back() == '%');
}

u32 parseColor(const std::string& value)
{
    if (value.empty()) {
        return 0xFFFFFFFF;
    }

    if (value[0] == '#') {
        std::string hex = value.substr(1);

        if (hex.size() == 3) {
            hex = std::string(1, hex[0]) + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
        } else if (hex.size() == 4) {
            hex = std::string(1, hex[0]) + hex[0] + hex[1] + hex[1] + hex[2] + hex[2] + hex[3] + hex[3];
        }

        try {
            u32 color = static_cast<u32>(std::stoul(hex, nullptr, 16));
            if (hex.size() == 6) {
                color |= 0xFF000000;
            }
            return color;
        }
        catch (...) {
            return 0xFFFFFFFF;
        }
    }

    if (value.starts_with("rgb(")) {
        size_t start = 4;
        size_t end = value.find(')');
        if (end != std::string::npos) {
            std::string inner = value.substr(start, end - start);
            std::istringstream iss(inner);
            std::string token;
            std::vector<i32> values;

            while (std::getline(iss, token, ',')) {
                try {
                    values.push_back(std::stoi(token));
                }
                catch (...) {
                }
            }

            if (values.size() >= 3) {
                u8 r = static_cast<u8>(std::clamp(values[0], 0, 255));
                u8 g = static_cast<u8>(std::clamp(values[1], 0, 255));
                u8 b = static_cast<u8>(std::clamp(values[2], 0, 255));
                return 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }

    if (value.starts_with("rgba(")) {
        size_t start = 5;
        size_t end = value.find(')');
        if (end != std::string::npos) {
            std::string inner = value.substr(start, end - start);
            std::istringstream iss(inner);
            std::string token;
            std::vector<f32> values;

            while (std::getline(iss, token, ',')) {
                try {
                    values.push_back(std::stof(token));
                }
                catch (...) {
                }
            }

            if (values.size() >= 4) {
                u8 r = static_cast<u8>(std::clamp(values[0], 0.0f, 255.0f));
                u8 g = static_cast<u8>(std::clamp(values[1], 0.0f, 255.0f));
                u8 b = static_cast<u8>(std::clamp(values[2], 0.0f, 255.0f));
                u8 a = static_cast<u8>(std::clamp(values[3] * 255.0f, 0.0f, 255.0f));
                return (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    static const std::unordered_map<std::string, u32> namedColors = {{"white", 0xFFFFFFFF},
        {"black", 0xFF000000},
        {"red", 0xFFFF0000},
        {"green", 0xFF00FF00},
        {"blue", 0xFF0000FF},
        {"yellow", 0xFFFFFF00},
        {"cyan", 0xFF00FFFF},
        {"magenta", 0xFFFF00FF},
        {"transparent", 0x00000000},
        {"gray", 0xFF808080},
        {"grey", 0xFF808080},
        {"lightgray", 0xFFD3D3D3},
        {"lightgrey", 0xFFD3D3D3},
        {"darkgray", 0xFFA9A9A9},
        {"darkgrey", 0xFFA9A9A9},
        {"orange", 0xFFFFA500},
        {"pink", 0xFFFFC0CB},
        {"purple", 0xFF800080},
        {"brown", 0xFFA52A2A}};

    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    auto it = namedColors.find(lowerValue);
    if (it != namedColors.end()) {
        return it->second;
    }

    return 0xFFFFFFFF;
}

std::string colorToString(u32 color)
{
    std::ostringstream oss;
    oss << "#" << std::hex << std::setfill('0') << std::setw(8) << color;
    return oss.str();
}

Anchor parseAnchor(const std::string& value)
{
    std::string lower = value;
    std::transform(
        lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    static const std::unordered_map<std::string, Anchor> anchors = {{"topleft", Anchor::TopLeft},
        {"topcenter", Anchor::TopCenter},
        {"topright", Anchor::TopRight},
        {"centerleft", Anchor::CenterLeft},
        {"center", Anchor::Center},
        {"centerright", Anchor::CenterRight},
        {"bottomleft", Anchor::BottomLeft},
        {"bottomcenter", Anchor::BottomCenter},
        {"bottomright", Anchor::BottomRight}};

    auto it = anchors.find(lower);
    return it != anchors.end() ? it->second : Anchor::TopLeft;
}

std::string anchorToString(Anchor anchor)
{
    switch (anchor) {
        case Anchor::TopLeft:
            return "topLeft";
        case Anchor::TopCenter:
            return "topCenter";
        case Anchor::TopRight:
            return "topRight";
        case Anchor::CenterLeft:
            return "centerLeft";
        case Anchor::Center:
            return "center";
        case Anchor::CenterRight:
            return "centerRight";
        case Anchor::BottomLeft:
            return "bottomLeft";
        case Anchor::BottomCenter:
            return "bottomCenter";
        case Anchor::BottomRight:
            return "bottomRight";
        default:
            return "topLeft";
    }
}

bool parseBool(const std::string& value)
{
    std::string lower = value;
    std::transform(
        lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
}

i32 parseInt(const std::string& value, i32 defaultValue)
{
    try {
        return std::stoi(value);
    }
    catch (...) {
        return defaultValue;
    }
}

f32 parseFloat(const std::string& value, f32 defaultValue)
{
    try {
        return std::stof(value);
    }
    catch (...) {
        return defaultValue;
    }
}

std::pair<f32, f32> parseRange(const std::string& value)
{
    size_t comma = value.find(',');
    if (comma == std::string::npos) {
        return {0.0f, 1.0f};
    }

    try {
        f32 min = std::stof(value.substr(0, comma));
        f32 max = std::stof(value.substr(comma + 1));
        return {min, max};
    }
    catch (...) {
        return {0.0f, 1.0f};
    }
}

} // namespace widget_attrs

} // namespace mc::client::ui::kagero::tpl::bindings
