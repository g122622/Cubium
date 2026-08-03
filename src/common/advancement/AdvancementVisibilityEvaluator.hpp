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

#include "Advancement.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <vector>

namespace mc::advancement {

class AdvancementManager;

/**
 * @brief 成就可见性评估器
 *
 * 实现与 MC Java 版一致的成就可见性递归算法。
 * 核心规则：
 * - 已完成的成就始终可见
 * - 无 display 的成就始终不可见（技术成就）
 * - 隐藏成就（hidden=true）在完成前不可见
 * - 非隐藏且未完成的成就，如果在祖先链 VISIBILITY_DEPTH 层内有已完成的祖先，则可见
 * - 子树中有已完成的成就会使祖先链可见
 *
 * VISIBILITY_DEPTH = 2 意味着未完成的非隐藏成就最多向上看 2 层祖先。
 */
class AdvancementVisibilityEvaluator {
public:
    /// 向上回溯的深度限制（未完成非隐藏成就最多向上看几层祖先）
    static constexpr i32 VISIBILITY_DEPTH = 2;

    /**
     * @brief 可见性规则
     *
     * 每个节点根据自身状态计算出的可见性规则，用于祖先回溯判断。
     */
    enum class VisibilityRule : u8 {
        Show,    ///< 强制可见（已完成）
        Hide,    ///< 强制隐藏（无 display 或 hidden 且未完成）
        NoChange ///< 不改变（非隐藏且未完成，由上下文决定）
    };

    /**
     * @brief 可见性输出回调
     *
     * @param advancement 成就
     * @param visible 是否可见
     */
    using Output = std::function<void(Advancement::Ptr advancement, bool visible)>;

    /**
     * @brief 评估成就的可见性（从根节点开始）
     *
     * 从指定成就的根节点开始，递归遍历整棵成就树，
     * 根据 MC Java 版的规则计算每个成就的可见性。
     * 调用者应确保传入根节点（通过 getRoots() 或 findRoot() 获取）。
     *
     * @param startNode 起始成就（最好是根节点）
     * @param isDone 判断成就是否完成的谓词
     * @param output 接受可见性结果的回调
     */
    static void evaluateVisibility(
        Advancement::Ptr startNode, const std::function<bool(Advancement::Ptr)>& isDone, const Output& output);

    /**
     * @brief 评估成就的可见性（从任意节点开始，通过 manager 查找根节点）
     *
     * 通过 AdvancementManager 向上遍历 parent 链找到根节点，
     * 然后从根开始递归计算可见性。
     *
     * @param node 任意成就节点
     * @param manager 成就管理器（用于查找父成就）
     * @param isDone 判断成就是否完成的谓词
     * @param output 接受可见性结果的回调
     */
    static void evaluateVisibilityFromNode(Advancement::Ptr node,
        AdvancementManager& manager,
        const std::function<bool(Advancement::Ptr)>& isDone,
        const Output& output);

    /**
     * @brief 通过 manager 向上查找根节点
     *
     * @param node 起始成就
     * @param manager 成就管理器
     * @return 根成就（如果找不到父成就，返回当前节点）
     */
    static Advancement::Ptr findRoot(Advancement::Ptr node, AdvancementManager& manager);

private:
    /**
     * @brief 评估单个成就节点的可见性规则
     *
     * @param advancement 成就
     * @param done 是否已完成
     * @return 可见性规则
     */
    static VisibilityRule _evaluateVisibilityRule(Advancement::Ptr advancement, bool done);

    /**
     * @brief 评估未完成节点的祖先回溯可见性
     *
     * 从栈中向上回溯最多 VISIBILITY_DEPTH 层祖先，
     * 遇到 Show 则可见，遇到 Hide 则不可见，全部 NoChange 则不可见。
     *
     * @param ruleStack 规则栈（从根到当前节点的路径）
     * @return 是否可见
     */
    static bool _evaluateVisibilityForUnfinishedNode(const std::vector<VisibilityRule>& ruleStack);

    /**
     * @brief 递归遍历成就树，计算可见性
     *
     * @param node 当前成就
     * @param ruleStack 规则栈
     * @param isDone 判断成就是否完成的谓词
     * @param output 接受可见性结果的回调
     * @return 当前子树中是否有已完成的节点
     */
    static bool _evaluateVisibility(Advancement::Ptr node,
        std::vector<VisibilityRule>& ruleStack,
        const std::function<bool(Advancement::Ptr)>& isDone,
        const Output& output);
};

} // namespace mc::advancement
