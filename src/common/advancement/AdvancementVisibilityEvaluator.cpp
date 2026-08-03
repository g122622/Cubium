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

#include "AdvancementVisibilityEvaluator.hpp"
#include "AdvancementManager.hpp"
#include "common/advancement/Advancement.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <vector>

namespace mc::advancement {

Advancement::Ptr AdvancementVisibilityEvaluator::findRoot(Advancement::Ptr node, AdvancementManager& manager)
{
    if (!node) {
        return nullptr;
    }

    // 通过 parent ID 链向上查找根节点
    Advancement::Ptr current = node;
    while (current->getParent().has_value()) {
        Advancement::Ptr parent = manager.get(current->getParent().value());
        if (!parent) {
            // 父成就不存在（引用错误），以当前节点作为根
            break;
        }
        current = parent;
    }
    return current;
}

AdvancementVisibilityEvaluator::VisibilityRule AdvancementVisibilityEvaluator::_evaluateVisibilityRule(
    Advancement::Ptr advancement, bool done)
{
    const auto& display = advancement->getDisplay();

    // 无 display 的成就（如配方解锁等技术成就）始终不可见
    if (!display.has_value()) {
        return VisibilityRule::Hide;
    }

    // 已完成的成就始终可见
    if (done) {
        return VisibilityRule::Show;
    }

    // 隐藏成就（hidden=true）在完成前不可见
    if (display->isHidden()) {
        return VisibilityRule::Hide;
    }

    // 非隐藏且未完成的成就，由上下文（祖先链）决定
    return VisibilityRule::NoChange;
}

bool AdvancementVisibilityEvaluator::_evaluateVisibilityForUnfinishedNode(const std::vector<VisibilityRule>& ruleStack)
{
    // 从栈顶（当前节点）向上回溯最多 VISIBILITY_DEPTH 层
    i32 stackSize = static_cast<i32>(ruleStack.size());
    for (i32 i = 0; i <= VISIBILITY_DEPTH; ++i) {
        i32 index = stackSize - 1 - i;
        if (index < 0) {
            break;
        }
        VisibilityRule rule = ruleStack[static_cast<size_t>(index)];
        if (rule == VisibilityRule::Show) {
            return true; // 祖先链上有 SHOW -> 可见
        }
        if (rule == VisibilityRule::Hide) {
            return false; // 祖先链上遇到 HIDE -> 不可见（阻断查找）
        }
        // NO_CHANGE -> 继续向上看
    }
    return false; // 全部都是 NO_CHANGE -> 不可见
}

bool AdvancementVisibilityEvaluator::_evaluateVisibility(Advancement::Ptr node,
    std::vector<VisibilityRule>& ruleStack,
    const std::function<bool(Advancement::Ptr)>& isDone,
    const Output& output)
{
    bool done = isDone(node);
    VisibilityRule rule = _evaluateVisibilityRule(node, done);
    bool anyChildDone = done; // 跟踪子树中是否有已完成的节点

    ruleStack.push_back(rule);

    // 递归处理所有子节点
    for (const auto& child : node->getChildren()) {
        anyChildDone |= _evaluateVisibility(child, ruleStack, isDone, output);
    }

    // 综合判定可见性：
    // 如果子树中有已完成的节点 -> 可见
    // 否则通过祖先回溯判定
    bool visible = anyChildDone || _evaluateVisibilityForUnfinishedNode(ruleStack);

    ruleStack.pop_back();
    output(node, visible);
    return anyChildDone;
}

void AdvancementVisibilityEvaluator::evaluateVisibility(
    Advancement::Ptr startNode, const std::function<bool(Advancement::Ptr)>& isDone, const Output& output)
{
    if (!startNode) {
        return;
    }

    // 初始化规则栈，压入 VISIBILITY_DEPTH + 1 个哨兵 NO_CHANGE
    // 确保根节点在向上回溯时不会越界
    std::vector<VisibilityRule> ruleStack;
    ruleStack.reserve(static_cast<size_t>(VISIBILITY_DEPTH) + 4);
    for (i32 i = 0; i <= VISIBILITY_DEPTH; ++i) {
        ruleStack.push_back(VisibilityRule::NoChange);
    }

    _evaluateVisibility(startNode, ruleStack, isDone, output);
}

void AdvancementVisibilityEvaluator::evaluateVisibilityFromNode(Advancement::Ptr node,
    AdvancementManager& manager,
    const std::function<bool(Advancement::Ptr)>& isDone,
    const Output& output)
{
    if (!node) {
        return;
    }

    // 通过 manager 向上查找根节点
    Advancement::Ptr root = findRoot(node, manager);
    evaluateVisibility(root, isDone, output);
}

} // namespace mc::advancement
