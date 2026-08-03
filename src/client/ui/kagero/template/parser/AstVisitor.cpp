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

#include "AstVisitor.hpp"
#include "client/ui/kagero/template/parser/Ast.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <queue>
#include <string>
#include <vector>

namespace mc::client::ui::kagero::tpl::ast::traversal {

void preorder(Node& root, const std::function<bool(Node&)>& callback)
{
    if (!callback(root)) return;

    for (auto& child : root.children) {
        preorder(*child, callback);
    }
}

void postorder(Node& root, const std::function<bool(Node&)>& callback)
{
    for (auto& child : root.children) {
        postorder(*child, callback);
    }

    callback(root);
}

void levelOrder(Node& root, const std::function<bool(Node&)>& callback)
{
    std::queue<Node*> queue;
    queue.push(&root);

    while (!queue.empty()) {
        Node* current = queue.front();
        queue.pop();

        if (!callback(*current)) break;

        for (auto& child : current->children) {
            queue.push(child.get());
        }
    }
}

void preorderConst(const Node& root, const std::function<bool(const Node&)>& callback)
{
    if (!callback(root)) return;

    for (const auto& child : root.children) {
        preorderConst(*child, callback);
    }
}

void postorderConst(const Node& root, const std::function<bool(const Node&)>& callback)
{
    for (const auto& child : root.children) {
        postorderConst(*child, callback);
    }

    callback(root);
}

Node* findFirst(Node& root, const std::function<bool(const Node&)>& predicate)
{
    if (predicate(root)) {
        return &root;
    }

    for (auto& child : root.children) {
        Node* found = findFirst(*child, predicate);
        if (found) return found;
    }

    return nullptr;
}

const Node* findFirst(const Node& root, const std::function<bool(const Node&)>& predicate)
{
    if (predicate(root)) {
        return &root;
    }

    for (const auto& child : root.children) {
        const Node* found = findFirst(*child, predicate);
        if (found) return found;
    }

    return nullptr;
}

std::vector<Node*> findAll(Node& root, const std::function<bool(const Node&)>& predicate)
{
    std::vector<Node*> results;

    if (predicate(root)) {
        results.push_back(&root);
    }

    for (auto& child : root.children) {
        auto childResults = findAll(*child, predicate);
        results.insert(results.end(), childResults.begin(), childResults.end());
    }

    return results;
}

std::vector<const Node*> findAll(const Node& root, const std::function<bool(const Node&)>& predicate)
{
    std::vector<const Node*> results;

    if (predicate(root)) {
        results.push_back(&root);
    }

    for (const auto& child : root.children) {
        auto childResults = findAll(*child, predicate);
        results.insert(results.end(), childResults.begin(), childResults.end());
    }

    return results;
}

ElementNode* findById(Node& root, const std::string& id)
{
    Node* found = findFirst(root, [&id](const Node& node) {
        if (auto* elem = dynamic_cast<const ElementNode*>(&node)) {
            return elem->id == id;
        }
        return false;
    });

    return dynamic_cast<ElementNode*>(found);
}

const ElementNode* findById(const Node& root, const std::string& id)
{
    const Node* found = findFirst(root, [&id](const Node& node) {
        if (auto* elem = dynamic_cast<const ElementNode*>(&node)) {
            return elem->id == id;
        }
        return false;
    });

    return dynamic_cast<const ElementNode*>(found);
}

std::vector<ElementNode*> findByTagName(Node& root, const std::string& tagName)
{
    auto nodes = findAll(root, [&tagName](const Node& node) {
        if (auto* elem = dynamic_cast<const ElementNode*>(&node)) {
            return elem->tagName == tagName;
        }
        return false;
    });

    std::vector<ElementNode*> results;
    for (auto* node : nodes) {
        if (auto* elem = dynamic_cast<ElementNode*>(node)) {
            results.push_back(elem);
        }
    }
    return results;
}

std::vector<const ElementNode*> findByTagName(const Node& root, const std::string& tagName)
{
    auto nodes = findAll(root, [&tagName](const Node& node) {
        if (auto* elem = dynamic_cast<const ElementNode*>(&node)) {
            return elem->tagName == tagName;
        }
        return false;
    });

    std::vector<const ElementNode*> results;
    for (const auto* node : nodes) {
        if (auto* elem = dynamic_cast<const ElementNode*>(node)) {
            results.push_back(elem);
        }
    }
    return results;
}

size_t getDepth(const Node& node)
{
    if (node.children.empty()) {
        return 0;
    }

    size_t maxChildDepth = 0;
    for (const auto& child : node.children) {
        maxChildDepth = std::max(maxChildDepth, getDepth(*child));
    }

    return maxChildDepth + 1;
}

size_t countNodes(const Node& root)
{
    size_t count = 1;
    for (const auto& child : root.children) {
        count += countNodes(*child);
    }
    return count;
}

size_t countNodes(const Node& root, NodeType type)
{
    size_t count = (root.type == type) ? 1 : 0;
    for (const auto& child : root.children) {
        count += countNodes(*child, type);
    }
    return count;
}

} // namespace mc::client::ui::kagero::tpl::ast::traversal
