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

#include "Ast.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace mc::client::ui::kagero::tpl::ast {

// ========== BindingInfo ==========

BindingInfo BindingInfo::parse(const std::string& value)
{
    BindingInfo info;

    if (value.empty()) {
        return info;
    }

    if (value.size() > 1 && value[0] == '$') {
        info.isLoopVariable = true;

        size_t dotPos = value.find('.');
        if (dotPos != std::string::npos) {
            info.loopVarName = value.substr(1, dotPos - 1);
            info.property = value.substr(dotPos + 1);
            info.path = value;
        } else {
            info.loopVarName = value.substr(1);
            info.property = "";
            info.path = value;
        }
    } else {
        info.path = value;
        info.isLoopVariable = false;
    }

    return info;
}

// ========== Attribute ==========

Attribute Attribute::createStatic(const std::string& name, const std::string& value, const SourceLocation& loc)
{
    Attribute attr;
    attr.name = name;
    attr.rawValue = value;
    attr.location = loc;
    attr.type = AttributeType::Static;

    if (value == "true") {
        attr.value = true;
    } else if (value == "false") {
        attr.value = false;
    } else {
        try {
            size_t pos;
            i32 intVal = std::stoi(value, &pos);
            if (pos == value.size()) {
                attr.value = intVal;
            } else {
                attr.value = value;
            }
        }
        catch (...) {
            try {
                size_t pos;
                f32 floatVal = std::stof(value, &pos);
                if (pos == value.size()) {
                    attr.value = floatVal;
                } else {
                    attr.value = value;
                }
            }
            catch (...) {
                attr.value = value;
            }
        }
    }

    return attr;
}

Attribute Attribute::createBinding(const std::string& name, const std::string& bindingPath, const SourceLocation& loc)
{
    Attribute attr;
    attr.name = name;
    attr.rawValue = bindingPath;
    attr.location = loc;
    attr.type = AttributeType::Binding;
    attr.binding = BindingInfo::parse(bindingPath);
    attr.value = bindingPath;
    return attr;
}

Attribute Attribute::createEvent(const std::string& name, const std::string& callbackName, const SourceLocation& loc)
{
    Attribute attr;
    attr.name = name;
    attr.rawValue = callbackName;
    attr.location = loc;
    attr.type = AttributeType::Event;
    attr.callbackName = callbackName;
    attr.value = callbackName;
    return attr;
}

std::string Attribute::baseName() const
{
    static const std::string bindPrefix = "bind:";
    static const std::string onPrefix = "on:";

    if (name.size() > bindPrefix.size() && name.substr(0, bindPrefix.size()) == bindPrefix) {
        return name.substr(bindPrefix.size());
    }

    if (name.size() > onPrefix.size() && name.substr(0, onPrefix.size()) == onPrefix) {
        return name.substr(onPrefix.size());
    }

    return name;
}

// ========== ElementNode ==========

void ElementNode::addAttribute(const Attribute& attr)
{
    attributes[attr.name] = attr;
}

const Attribute* ElementNode::getAttribute(const std::string& name) const
{
    auto it = attributes.find(name);
    return it != attributes.end() ? &it->second : nullptr;
}

bool ElementNode::hasAttribute(const std::string& name) const
{
    return attributes.find(name) != attributes.end();
}

void ElementNode::categorizeAttributes()
{
    staticAttrs.clear();
    bindingAttrs.clear();
    eventAttrs.clear();

    for (const auto& [name, attr] : attributes) {
        switch (attr.type) {
            case AttributeType::Static:
                staticAttrs.push_back(attr);
                break;
            case AttributeType::Binding:
                bindingAttrs.push_back(attr);
                break;
            case AttributeType::Event:
                eventAttrs.push_back(attr);
                break;
            case AttributeType::Loop:
            case AttributeType::Condition:
                break;
        }
    }
}

std::unique_ptr<Node> ElementNode::clone() const
{
    auto node = std::make_unique<ElementNode>(type);
    node->tagName = tagName;
    node->id = id;
    node->classes = classes;
    node->attributes = attributes;
    node->staticAttrs = staticAttrs;
    node->bindingAttrs = bindingAttrs;
    node->eventAttrs = eventAttrs;
    node->loop = loop;
    node->condition = condition;
    node->range = range;

    for (const auto& child : children) {
        node->children.push_back(child->clone());
    }

    return node;
}

// ========== DocumentNode ==========

ElementNode* DocumentNode::rootElement()
{
    for (auto& child : children) {
        if (auto* elem = dynamic_cast<ElementNode*>(child.get())) {
            return elem;
        }
    }
    return nullptr;
}

const ElementNode* DocumentNode::rootElement() const
{
    for (const auto& child : children) {
        if (const auto* elem = dynamic_cast<const ElementNode*>(child.get())) {
            return elem;
        }
    }
    return nullptr;
}

std::unique_ptr<Node> DocumentNode::clone() const
{
    auto node = std::make_unique<DocumentNode>();
    node->sourcePath = sourcePath;
    node->version = version;
    node->range = range;

    for (const auto& child : children) {
        node->children.push_back(child->clone());
    }

    return node;
}

// ========== 工具函数 ==========

NodeType getNodeTypeFromTagName(const std::string& tagName)
{
    static const std::map<std::string, NodeType> tagMap = {{"screen", NodeType::Screen},
        {"widget", NodeType::Widget},
        {"button", NodeType::Button},
        {"text", NodeType::Text},
        {"textfield", NodeType::TextField},
        {"slider", NodeType::Slider},
        {"checkbox", NodeType::Checkbox},
        {"image", NodeType::Image},
        {"grid", NodeType::Grid},
        {"slot", NodeType::Slot},
        {"viewport3d", NodeType::Viewport3D},
        {"scrollable", NodeType::Scrollable},
        {"list", NodeType::List},
        {"container", NodeType::Container},
        {"style", NodeType::Style}};

    std::string lowerName = tagName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    auto it = tagMap.find(lowerName);
    return it != tagMap.end() ? it->second : NodeType::Widget;
}

bool isValidWidgetTag(const std::string& tagName)
{
    static const std::set<std::string> validTags = {"screen",
        "widget",
        "button",
        "text",
        "textfield",
        "slider",
        "checkbox",
        "image",
        "grid",
        "slot",
        "viewport3d",
        "scrollable",
        "list",
        "style",
        "container"};

    std::string lowerName = tagName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return validTags.find(lowerName) != validTags.end();
}

bool isValidAttributeName(const std::string& name)
{
    if (name.empty()) {
        return false;
    }

    // 允许的属性前缀
    static const std::vector<std::string> allowedPrefixes = {"bind:", "on:", "for:", "if:"};

    // 检查是否有前缀
    for (const auto& prefix : allowedPrefixes) {
        if (name.size() > prefix.size() && name.substr(0, prefix.size()) == prefix) {
            // 前缀后面的部分必须是有效的标识符
            std::string suffix = name.substr(prefix.size());
            return isValidCallbackName(suffix);
        }
    }

    // 静态属性名必须是有效的标识符：首字符为字母或下划线，
    // 其后可为字母数字、下划线、连字符或冒号（冒号用于命名空间风格的属性名）。
    // 不允许以数字开头。
    if (std::isdigit(static_cast<unsigned char>(name[0]))) {
        return false;
    }
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != ':') {
            return false;
        }
    }

    return true;
}

bool isValidBindingPath(const std::string& path)
{
    if (path.empty()) {
        return false;
    }

    // 绑定路径可以是：
    // 1. 简单标识符：player, game
    // 2. 点分隔路径：player.health, game.version
    // 3. 数组索引：slots[0], grid[1][2], slots[0].item，亦支持命名索引 array[index]
    // 4. 循环变量：$item, $slot
    // 5. 循环变量属性：$item.name, $slot.count
    //
    // 规则：每个路径段必须是合法标识符（首字符为字母或下划线，其后为字母数字或下划线），
    //      不能以数字开头。段后可跟随若干 [index] 索引片段，index 为非负整数或标识符，
    //      但不允许负号（[-1] 无效）或空索引（[] 无效）。

    size_t i = 0;
    const size_t len = path.size();

    // 检查循环变量前缀
    if (path[0] == '$') {
        i = 1;
        if (i >= len) {
            return false; // 只有 "$" 是无效的
        }
    }

    // 至少需要一个路径段
    bool hasSegment = false;

    while (i < len) {
        char c = path[i];

        if (c == '.') {
            // 点不能出现在开头，也不能紧跟另一个点或索引片段之后
            if (!hasSegment) {
                return false;
            }
            hasSegment = false;
            ++i;
            continue;
        }

        if (c == '[') {
            // 索引片段必须紧跟一个有效段
            if (!hasSegment) {
                return false;
            }
            ++i;
            // 索引内容可为非负整数（如 [0]）或标识符（如 [index]），但不能为空，
            // 也不能以负号开头（[-1] 无效）。
            if (i >= len) {
                return false; // 未闭合
            }
            char first = path[i];
            bool isDigitIndex = std::isdigit(static_cast<unsigned char>(first));
            bool isIdentIndex = std::isalpha(static_cast<unsigned char>(first)) || first == '_';
            if (!isDigitIndex && !isIdentIndex) {
                return false; // 空索引或非法首字符（含 '-'）
            }
            ++i;
            if (isDigitIndex) {
                while (i < len && std::isdigit(static_cast<unsigned char>(path[i]))) {
                    ++i;
                }
            } else {
                while (i < len && (std::isalnum(static_cast<unsigned char>(path[i])) || path[i] == '_')) {
                    ++i;
                }
            }
            if (i >= len || path[i] != ']') {
                return false; // 缺少右括号
            }
            ++i; // 消费 ']'
            // 索引后可以接 '.' 或 '[' 或结束，但不能直接接字母
            continue;
        }

        // 标识符段字符
        if (!hasSegment) {
            // 段首字符：必须是字母或下划线（不允许数字开头）
            if (!std::isalpha(static_cast<unsigned char>(c)) && c != '_') {
                return false;
            }
            hasSegment = true;
        } else {
            // 段内字符：字母、数字或下划线
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
                return false;
            }
        }
        ++i;
    }

    // 必须以有效段或索引片段结尾（不能以点结尾）
    return hasSegment;
}

bool isValidCallbackName(const std::string& name)
{
    if (name.empty()) {
        return false;
    }

    // 必须是有效的 C++ 标识符
    // 首字符必须是字母或下划线
    if (!std::isalpha(static_cast<unsigned char>(name[0])) && name[0] != '_') {
        return false;
    }

    // 其余字符必须是字母、数字或下划线
    for (size_t i = 1; i < name.size(); ++i) {
        char c = name[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }

    return true;
}

} // namespace mc::client::ui::kagero::tpl::ast
