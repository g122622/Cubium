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
#include <spdlog/spdlog.h>

namespace mc::advancement {

Advancement::Ptr AdvancementVisibilityEvaluator::_findRoot(Advancement::Ptr node)
{
    // Advancement 只存储 parent ID（ResourceLocation），不存储直接父指针。
    // 但 children 是运行时填充的，我们需要从整棵树的角度找到根。
    // 由于成就树的结构是：根成就 isRoot() == true，我们只能从已知的根开始遍历。
    // 这里使用简单策略：沿着 parent 链向上查找。
    // 但 Advancement 只有 getParent() 返回 ResourceLocation，没有直接的父指针。
    // 所以我们需要换一种方式：调用者应该直接传入根节点。

    // 事实上，在 MC 原版中 AdvancementNode 有 parent 指针可以向上遍历。
    // 在我们的实现中，Advancement 没有 parent 指针，只有 parent ID。
    // 因此 _findRoot 的正确做法是：对于根节点，isRoot() == true。
    // 如果传入的不是根节点，我们无法向上查找（需要 manager）。
    // 解决方案：evaluateVisibility 的调用者应该确保传入根节点，
    // 或者我们遍历整棵树从根开始。

    // 对于根节点直接返回
    if (node->isRoot()) {
        return node;
    }

    // 非根节点无法向上查找，返回自身
    // 调用者应确保传入根节点
    return node;
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
    // 栈的布局：[..., 哨兵NO_CHANGE, 哨兵NO_CHANGE, 哨兵NO_CHANGE, 根规则, ..., 当前节点规则]
    // 栈顶是当前节点

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

    // 找到根节点
    Advancement::Ptr root = _findRoot(startNode);

    // 初始化规则栈，压入 VISIBILITY_DEPTH + 1 个哨兵 NO_CHANGE
    // 确保根节点在向上回溯时不会越界
    std::vector<VisibilityRule> ruleStack;
    ruleStack.reserve(static_cast<size_t>(VISIBILITY_DEPTH) + 4);
    for (i32 i = 0; i <= VISIBILITY_DEPTH; ++i) {
        ruleStack.push_back(VisibilityRule::NoChange);
    }

    _evaluateVisibility(root, ruleStack, isDone, output);
}

} // namespace mc::advancement
