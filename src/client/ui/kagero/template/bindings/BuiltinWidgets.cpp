#include "BuiltinWidgets.hpp"
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

    registerScreenWidget();
    registerContainerWidget();
    registerButtonWidget();
    registerTextWidget();
    registerTextFieldWidget();
    registerSliderWidget();
    registerCheckboxWidget();
    registerImageWidget();
    registerGridWidget();
    registerSlotWidget();
    registerScrollableWidget();
    registerListWidget();
    registerViewport3DWidget();

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
            widget_attrs::applyPosition(widget.get(), posIt->second);
        }

        auto sizeIt = attrs.find("size");
        if (sizeIt != attrs.end()) {
            widget_attrs::applySize(widget.get(), sizeIt->second);
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
                if (layout == "flex") {
                    container->setLayoutType(widget::ContainerLayoutType::Flex);
                } else if (layout == "grid") {
                    container->setLayoutType(widget::ContainerLayoutType::Grid);
                } else if (layout == "anchor") {
                    container->setLayoutType(widget::ContainerLayoutType::Anchor);
                }
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

void BuiltinWidgets::registerScreenWidget()
{
    m_creators["screen"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto widget = std::make_unique<widget::ContainerWidget>(id.empty() ? "screen" : id);
        auto titleIt = attrs.find("title");
        if (titleIt != attrs.end()) {
            (void)titleIt;
        }
        auto modalIt = attrs.find("modal");
        if (modalIt != attrs.end()) {
            (void)modalIt;
        }
        return widget;
    };

    m_defaultAttributes["screen"] = {{"size", "auto,auto"}};
}

void BuiltinWidgets::registerContainerWidget()
{
    m_creators["container"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto widget = std::make_unique<widget::ContainerWidget>(id.empty() ? "container" : id);

        auto layoutIt = attrs.find("layout");
        if (layoutIt != attrs.end()) {
            std::string layout = layoutIt->second;
            std::transform(layout.begin(), layout.end(), layout.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (layout == "flex") {
                widget->setLayoutType(widget::ContainerLayoutType::Flex);
            } else if (layout == "grid") {
                widget->setLayoutType(widget::ContainerLayoutType::Grid);
            } else if (layout == "anchor") {
                widget->setLayoutType(widget::ContainerLayoutType::Anchor);
            }
        }

        return widget;
    };

    m_defaultAttributes["container"] = {{"size", "auto,auto"}};
}

void BuiltinWidgets::registerButtonWidget()
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

void BuiltinWidgets::registerTextWidget()
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

void BuiltinWidgets::registerTextFieldWidget()
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

void BuiltinWidgets::registerSliderWidget()
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

void BuiltinWidgets::registerCheckboxWidget()
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

void BuiltinWidgets::registerImageWidget()
{
    m_creators["image"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto widget = std::make_unique<widget::ContainerWidget>(id.empty() ? "image" : id);

        auto srcIt = attrs.find("src");
        if (srcIt != attrs.end()) {
            (void)srcIt;
        }

        return widget;
    };

    m_defaultAttributes["image"] = {{"size", "auto,auto"}};
}

void BuiltinWidgets::registerGridWidget()
{
    m_creators["grid"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto widget = std::make_unique<widget::ContainerWidget>(id.empty() ? "grid" : id);
        widget->setLayoutType(widget::ContainerLayoutType::Grid);

        auto colsIt = attrs.find("cols");
        if (colsIt != attrs.end()) {
            (void)colsIt;
        }
        auto rowsIt = attrs.find("rows");
        if (rowsIt != attrs.end()) {
            (void)rowsIt;
        }

        return widget;
    };

    m_defaultAttributes["grid"] = {{"cols", "1"}, {"rows", "1"}};
}

void BuiltinWidgets::registerSlotWidget()
{
    m_creators["slot"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto slot = std::make_unique<widget::SlotWidget>();

        if (!id.empty()) {
            slot->setId(id);
        }

        auto indexIt = attrs.find("index");
        if (indexIt != attrs.end()) {
            (void)indexIt;
        }

        return slot;
    };

    m_defaultAttributes["slot"] = {{"size", "18,18"}};
}

void BuiltinWidgets::registerScrollableWidget()
{
    m_creators["scrollable"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto scrollable = std::make_unique<widget::ScrollableWidget>();

        if (!id.empty()) {
            scrollable->setId(id);
        }

        // 解析属性
        for (const auto& [key, value] : attrs) {
            if (key == "content-height") {
                try {
                    scrollable->setContentHeight(std::stoi(value));
                }
                catch (...) {
                }
            } else if (key == "content-width") {
                try {
                    scrollable->setContentWidth(std::stoi(value));
                }
                catch (...) {
                }
            } else if (key == "scroll-speed") {
                try {
                    scrollable->setScrollSpeed(std::stod(value));
                }
                catch (...) {
                }
            } else if (key == "show-scrollbar") {
                scrollable->setShowScrollbar(value == "true");
            }
        }

        return scrollable;
    };
}

void BuiltinWidgets::registerListWidget()
{
    m_creators["list"] = [](const std::string& id, const std::map<std::string, std::string>& attrs) {
        auto list = std::make_unique<widget::ListWidget>();

        if (!id.empty()) {
            list->setId(id);
        }

        return list;
    };
}

void BuiltinWidgets::registerViewport3DWidget()
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

void applyPosition(widget::Widget* widget, const std::string& value)
{
    if (!widget) return;
    auto [x, y] = parsePosition(value);
    widget->setPosition(x, y);
}

void applySize(widget::Widget* widget, const std::string& value)
{
    if (!widget) return;
    auto [width, height] = parseSize(value);
    widget->setSize(width, height);
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

    if (value.substr(0, 4) == "rgb(") {
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

    if (value.substr(0, 5) == "rgba(") {
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
