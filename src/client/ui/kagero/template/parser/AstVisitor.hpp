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

#include "Ast.hpp"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace mc::client::ui::kagero::tpl::ast {

/**
 * @brief AST访问者接口
 *
 * 使用访问者模式遍历AST节点。
 * 支持前序遍历、后序遍历和自定义遍历逻辑。
 *
 * 使用示例：
 * @code
 * class MyVisitor : public AstVisitor {
 * public:
 *     void visitElement(ElementNode& node) override {
 *         // 处理元素节点
 *         traverseChildren(node); // 继续遍历子节点
 *     }
 *
 *     void visitText(TextNode& node) override {
 *         // 处理文本节点
 *     }
 * };
 * @endcode
 */
class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    // ========== 节点访问方法 ==========

    /**
     * @brief 访问通用节点
     *
     * 默认实现会根据节点类型分发到具体的visit方法
     */
    virtual void visit(Node& node)
    {
        switch (node.type) {
            case NodeType::Document:
                visitDocument(static_cast<DocumentNode&>(node));
                break;
            case NodeType::TextContent:
                visitText(static_cast<TextNode&>(node));
                break;
            case NodeType::Comment:
                visitComment(static_cast<CommentNode&>(node));
                break;
            default:
                // 元素类型节点
                visitElement(static_cast<ElementNode&>(node));
                break;
        }
    }

    /**
     * @brief 访问文档节点
     */
    virtual void visitDocument(DocumentNode& node) { traverseChildren(node); }

    /**
     * @brief 访问元素节点
     *
     * 重写此方法处理元素节点，记得调用traverseChildren或手动遍历子节点
     */
    virtual void visitElement(ElementNode& node) { traverseChildren(node); }

    /**
     * @brief 访问文本节点
     */
    virtual void visitText(TextNode& node) { (void)node; }

    /**
     * @brief 访问注释节点
     */
    virtual void visitComment(CommentNode& node) { (void)node; }

    // ========== 遍历控制 ==========

    /**
     * @brief 遍历子节点
     */
    virtual void traverseChildren(Node& node)
    {
        for (auto& child : node.children) {
            visit(*child);
            if (m_shouldStop) break;
        }
    }

    /**
     * @brief 停止遍历
     *
     * 设置此标志将停止后续遍历
     */
    void stop() { m_shouldStop = true; }

    /**
     * @brief 检查是否应该停止
     */
    [[nodiscard]] bool shouldStop() const { return m_shouldStop; }

private:
    bool m_shouldStop = false;
};

/**
 * @brief 常量AST访问者接口
 *
 * 用于只读遍历AST
 */
class ConstAstVisitor {
public:
    virtual ~ConstAstVisitor() = default;

    virtual void visit(const Node& node)
    {
        switch (node.type) {
            case NodeType::Document:
                visitDocument(static_cast<const DocumentNode&>(node));
                break;
            case NodeType::TextContent:
                visitText(static_cast<const TextNode&>(node));
                break;
            case NodeType::Comment:
                visitComment(static_cast<const CommentNode&>(node));
                break;
            default:
                visitElement(static_cast<const ElementNode&>(node));
                break;
        }
    }

    virtual void visitDocument(const DocumentNode& node) { traverseChildren(node); }

    virtual void visitElement(const ElementNode& node) { traverseChildren(node); }

    virtual void visitText(const TextNode& node) { (void)node; }

    virtual void visitComment(const CommentNode& node) { (void)node; }

    virtual void traverseChildren(const Node& node)
    {
        for (const auto& child : node.children) {
            visit(*child);
            if (m_shouldStop) break;
        }
    }

    void stop() { m_shouldStop = true; }
    [[nodiscard]] bool shouldStop() const { return m_shouldStop; }

private:
    bool m_shouldStop = false;
};

/**
 * @brief AST遍历工具函数
 */
namespace traversal {

/**
 * @brief 前序遍历AST
 *
 * @param root 根节点
 * @param callback 节点回调函数，返回true继续遍历，返回false停止
 */
void preorder(Node& root, const std::function<bool(Node&)>& callback);

/**
 * @brief 后序遍历AST
 */
void postorder(Node& root, const std::function<bool(Node&)>& callback);

/**
 * @brief 层序遍历AST（广度优先）
 */
void levelOrder(Node& root, const std::function<bool(Node&)>& callback);

/**
 * @brief 常量版本的前序遍历
 */
void preorderConst(const Node& root, const std::function<bool(const Node&)>& callback);

/**
 * @brief 常量版本的后序遍历
 */
void postorderConst(const Node& root, const std::function<bool(const Node&)>& callback);

/**
 * @brief 查找第一个匹配的节点
 *
 * @param root 根节点
 * @param predicate 谓词函数
 * @return 匹配的节点指针，未找到返回nullptr
 */
Node* findFirst(Node& root, const std::function<bool(const Node&)>& predicate);
const Node* findFirst(const Node& root, const std::function<bool(const Node&)>& predicate);

/**
 * @brief 查找所有匹配的节点
 */
std::vector<Node*> findAll(Node& root, const std::function<bool(const Node&)>& predicate);
std::vector<const Node*> findAll(const Node& root, const std::function<bool(const Node&)>& predicate);

/**
 * @brief 通过ID查找元素
 */
ElementNode* findById(Node& root, const std::string& id);
const ElementNode* findById(const Node& root, const std::string& id);

/**
 * @brief 通过标签名查找所有元素
 */
std::vector<ElementNode*> findByTagName(Node& root, const std::string& tagName);
std::vector<const ElementNode*> findByTagName(const Node& root, const std::string& tagName);

/**
 * @brief 获取节点深度
 */
size_t getDepth(const Node& node);

/**
 * @brief 统计节点数量
 */
size_t countNodes(const Node& root);

/**
 * @brief 统计特定类型节点数量
 */
size_t countNodes(const Node& root, NodeType type);

} // namespace traversal

} // namespace mc::client::ui::kagero::tpl::ast
