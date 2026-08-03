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

#include "client/ui/kagero/template/core/TemplateConfig.hpp"
#include "client/ui/kagero/template/core/TemplateError.hpp"
#include "common/core/Types.hpp"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mc::client::ui::kagero::tpl::ast {

// 引入core命名空间的类型
using core::SourceLocation;
using core::SourceRange;
using core::TemplateVersion;

/**
 * @brief AST节点类型枚举
 *
 * 定义所有可能的模板节点类型
 */
enum class NodeType : u8 {
    // 根节点
    Document, ///< 文档根节点

    // 组件节点
    Screen,     ///< 屏幕节点
    Widget,     ///< 通用组件节点
    Button,     ///< 按钮节点
    Text,       ///< 文本节点
    TextField,  ///< 文本输入框节点
    Slider,     ///< 滑块节点
    Checkbox,   ///< 复选框节点
    Image,      ///< 图片节点
    Grid,       ///< 网格节点
    Slot,       ///< 物品槽节点
    Viewport3D, ///< 3D视口节点
    Scrollable, ///< 可滚动容器节点
    List,       ///< 列表节点
    Container,  ///< 容器节点
    Style,      ///< 样式节点

    // 属性节点
    StaticAttr,  ///< 静态属性
    BindingAttr, ///< 绑定属性 (bind:xxx)
    EventAttr,   ///< 事件属性 (on:xxx)

    // 指令节点
    LoopDirective,      ///< 循环指令
    ConditionDirective, ///< 条件指令

    // 文本节点
    TextContent, ///< 纯文本内容
    Comment      ///< 注释
};

/**
 * @brief 获取节点类型名称
 */
[[nodiscard]] inline const char* nodeTypeName(NodeType type)
{
    switch (type) {
        case NodeType::Document:
            return "Document";
        case NodeType::Screen:
            return "Screen";
        case NodeType::Widget:
            return "Widget";
        case NodeType::Button:
            return "Button";
        case NodeType::Text:
            return "Text";
        case NodeType::TextField:
            return "TextField";
        case NodeType::Slider:
            return "Slider";
        case NodeType::Checkbox:
            return "Checkbox";
        case NodeType::Image:
            return "Image";
        case NodeType::Grid:
            return "Grid";
        case NodeType::Slot:
            return "Slot";
        case NodeType::Viewport3D:
            return "Viewport3D";
        case NodeType::Scrollable:
            return "Scrollable";
        case NodeType::List:
            return "List";
        case NodeType::Container:
            return "Container";
        case NodeType::Style:
            return "Style";
        case NodeType::StaticAttr:
            return "StaticAttr";
        case NodeType::BindingAttr:
            return "BindingAttr";
        case NodeType::EventAttr:
            return "EventAttr";
        case NodeType::LoopDirective:
            return "LoopDirective";
        case NodeType::ConditionDirective:
            return "ConditionDirective";
        case NodeType::TextContent:
            return "TextContent";
        case NodeType::Comment:
            return "Comment";
        default:
            return "Unknown";
    }
}

/**
 * @brief 属性值类型
 */
using AttrValue = std::variant<std::string, ///< 字符串值
    i32,                                    ///< 整数值
    f32,                                    ///< 浮点值
    bool                                    ///< 布尔值
    >;

/**
 * @brief 属性类型枚举
 */
enum class AttributeType : u8 {
    Static,   ///< 静态属性（直接值）
    Binding,  ///< 绑定属性 (bind:xxx)
    Event,    ///< 事件属性 (on:xxx)
    Loop,     ///< 循环属性 (for:xxx)
    Condition ///< 条件属性 (if:xxx)
};

/**
 * @brief 绑定信息
 *
 * 存储绑定属性的解析信息
 */
struct BindingInfo {
    std::string path;            ///< 绑定路径 (e.g., "player.inventory.main")
    bool isLoopVariable = false; ///< 是否是循环变量 ($item)
    std::string loopVarName;     ///< 循环变量名 (e.g., "item" for $item)
    std::string property;        ///< 属性名 (e.g., "item" for $slot.item)

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return !path.empty() || isLoopVariable; }

    /**
     * @brief 解析绑定路径
     * @param value 原始绑定值 (e.g., "$slot.item" 或 "player.name")
     * @return 解析后的绑定信息
     */
    static BindingInfo parse(const std::string& value);
};

/**
 * @brief 属性节点
 *
 * 表示元素的属性，区分静态属性、绑定属性和事件属性
 */
struct Attribute {
    std::string name;                           ///< 属性名 (e.g., "pos", "bind:text", "on:click")
    std::string rawValue;                       ///< 原始属性值字符串
    AttrValue value;                            ///< 解析后的值（静态属性）
    AttributeType type = AttributeType::Static; ///< 属性类型
    SourceLocation location;                    ///< 源码位置

    /// 绑定信息（仅当 type == Binding 时有效）
    std::optional<BindingInfo> binding;

    /// 事件回调名（仅当 type == Event 时有效）
    std::string callbackName;

    /**
     * @brief 创建静态属性
     */
    static Attribute createStatic(
        const std::string& name, const std::string& value, const SourceLocation& loc = SourceLocation());

    /**
     * @brief 创建绑定属性
     */
    static Attribute createBinding(
        const std::string& name, const std::string& bindingPath, const SourceLocation& loc = SourceLocation());

    /**
     * @brief 创建事件属性
     */
    static Attribute createEvent(
        const std::string& name, const std::string& callbackName, const SourceLocation& loc = SourceLocation());

    /**
     * @brief 获取属性的基础名称（去除前缀）
     */
    [[nodiscard]] std::string baseName() const;

    /**
     * @brief 检查是否是绑定属性
     */
    [[nodiscard]] bool isBinding() const { return type == AttributeType::Binding; }

    /**
     * @brief 检查是否是事件属性
     */
    [[nodiscard]] bool isEvent() const { return type == AttributeType::Event; }

    /**
     * @brief 检查是否是静态属性
     */
    [[nodiscard]] bool isStatic() const { return type == AttributeType::Static; }
};

/**
 * @brief 循环信息
 *
 * 存储循环渲染指令的信息
 *
 * 支持语法：
 * - for:item="item in collection" - 基本循环
 * - for:(item, index) in collection" - 带索引循环
 */
struct LoopInfo {
    std::string collectionPath; ///< 集合路径 (e.g., "player.inventory.slots")
    std::string itemVarName;    ///< 循环变量名 (e.g., "slot" for $slot)
    std::string indexVarName;   ///< 索引变量名 (可选，如 "$index")
    SourceLocation location;    ///< 源码位置
    bool hasIndex = false;      ///< 是否有索引变量

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return !collectionPath.empty() && !itemVarName.empty(); }
};

/**
 * @brief 条件信息
 *
 * 存储条件渲染指令的信息
 */
struct ConditionInfo {
    std::string booleanPath; ///< 布尔路径 (e.g., "player.isSneaking")
    bool negate = false;     ///< 是否取反 (bind:visible="!player.hidden")
    SourceLocation location; ///< 源码位置

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return !booleanPath.empty(); }
};

/**
 * @brief AST节点基类
 */
struct Node {
    NodeType type;                               ///< 节点类型
    SourceRange range;                           ///< 源码范围
    std::vector<std::unique_ptr<Node>> children; ///< 子节点

    explicit Node(NodeType t)
        : type(t)
    {}
    virtual ~Node() = default;

    // 禁止拷贝
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    // 允许移动
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;

    /**
     * @brief 克隆节点
     */
    [[nodiscard]] virtual std::unique_ptr<Node> clone() const = 0;
};

/**
 * @brief 元素节点
 *
 * 表示XML元素（标签）
 */
struct ElementNode : public Node {
    std::string tagName;                         ///< 标签名
    std::string id;                              ///< ID属性（如果有）
    std::vector<std::string> classes;            ///< 类名列表
    std::map<std::string, Attribute> attributes; ///< 属性映射（按属性名索引）

    // 显式分离的属性集合（方便后续处理）
    std::vector<Attribute> staticAttrs;  ///< 静态属性列表
    std::vector<Attribute> bindingAttrs; ///< 绑定属性列表
    std::vector<Attribute> eventAttrs;   ///< 事件属性列表

    // 指令（如果有）
    std::optional<LoopInfo> loop;           ///< 循环指令
    std::optional<ConditionInfo> condition; ///< 条件指令

    explicit ElementNode(NodeType t)
        : Node(t)
    {}

    /**
     * @brief 添加属性
     */
    void addAttribute(const Attribute& attr);

    /**
     * @brief 获取属性
     */
    [[nodiscard]] const Attribute* getAttribute(const std::string& name) const;

    /**
     * @brief 检查是否有属性
     */
    [[nodiscard]] bool hasAttribute(const std::string& name) const;

    /**
     * @brief 分类属性
     *
     * 将属性分类到staticAttrs、bindingAttrs、eventAttrs
     */
    void categorizeAttributes();

    /**
     * @brief 克隆节点
     */
    [[nodiscard]] std::unique_ptr<Node> clone() const override;
};

/**
 * @brief 文本内容节点
 */
struct TextNode : public Node {
    std::string text;          ///< 文本内容
    bool isWhitespace = false; ///< 是否全是空白字符

    TextNode()
        : Node(NodeType::TextContent)
    {}

    [[nodiscard]] std::unique_ptr<Node> clone() const override
    {
        auto node = std::make_unique<TextNode>();
        node->text = text;
        node->isWhitespace = isWhitespace;
        node->range = range;
        return node;
    }
};

/**
 * @brief 注释节点
 */
struct CommentNode : public Node {
    std::string text; ///< 注释内容

    CommentNode()
        : Node(NodeType::Comment)
    {}

    [[nodiscard]] std::unique_ptr<Node> clone() const override
    {
        auto node = std::make_unique<CommentNode>();
        node->text = text;
        node->range = range;
        return node;
    }
};

/**
 * @brief 文档节点（根节点）
 */
struct DocumentNode : public Node {
    std::string sourcePath;  ///< 源文件路径
    TemplateVersion version; ///< 模板版本

    DocumentNode()
        : Node(NodeType::Document)
        , version(TemplateVersion::LATEST)
    {}

    /**
     * @brief 获取根元素
     */
    [[nodiscard]] ElementNode* rootElement();

    /**
     * @brief 获取根元素（const版本）
     */
    [[nodiscard]] const ElementNode* rootElement() const;

    [[nodiscard]] std::unique_ptr<Node> clone() const override;
};

/**
 * @brief 检查标签名对应的节点类型
 */
[[nodiscard]] NodeType getNodeTypeFromTagName(const std::string& tagName);

/**
 * @brief 检查标签名是否是有效的组件标签
 */
[[nodiscard]] bool isValidWidgetTag(const std::string& tagName);

/**
 * @brief 检查属性名是否是有效的属性
 */
[[nodiscard]] bool isValidAttributeName(const std::string& name);

/**
 * @brief 检查绑定路径是否有效
 *
 * 在严格模式下验证绑定路径格式
 */
[[nodiscard]] bool isValidBindingPath(const std::string& path);

/**
 * @brief 检查回调名称是否有效
 *
 * 回调名称必须是有效的C++标识符
 */
[[nodiscard]] bool isValidCallbackName(const std::string& name);

} // namespace mc::client::ui::kagero::tpl::ast
