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

#include <gtest/gtest.h>

#include "common/advancement/Advancement.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/AdvancementVisibilityEvaluator.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/ImpossibleTrigger.hpp"
#include "common/advancement/trigger/impl/TickTrigger.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

// Undef Windows macros that may conflict with method names
#ifdef parent
#undef parent
#endif

using namespace mc;
using namespace mc::advancement;

/**
 * @brief AdvancementVisibilityEvaluator 单元测试
 *
 * 测试 MC Java 版成就可见性递归算法的核心逻辑：
 * - 已完成的成就始终可见
 * - 无 display 的成就始终不可见（技术成就）
 * - 隐藏成就（hidden=true）在完成前不可见
 * - 非隐藏且未完成的成就，向上回溯 VISIBILITY_DEPTH(2) 层祖先判定可见性
 * - 子树中有已完成的成就会使祖先链可见
 * - findRoot 正确查找成就树的根节点
 */
class AdvancementVisibilityEvaluatorTest : public ::testing::Test {
protected:
    void SetUp() override { CriterionTriggers::instance().registerBuiltinTriggers(); }

    void TearDown() override
    {
        CriterionTriggers::instance().clear();
        AdvancementManager::instance().clear();
    }

    /**
     * @brief 创建带显示信息的成就
     * @param id 成就ID
     * @param parentId 父成就ID（可选）
     * @param hidden 是否隐藏
     * @return 成就指针
     */
    Advancement::Ptr createAdvancement(const std::string& id, const std::string& parentId = "", bool hidden = false)
    {
        Advancement::Builder builder{ResourceLocation(id)};

        if (!parentId.empty()) {
            builder.parent(ResourceLocation(parentId));
        }

        AdvancementDisplay display(ItemStack(),
            std::make_unique<mc::text::StringTextComponent>("Test"),
            std::make_unique<mc::text::StringTextComponent>("Test Description"),
            AdvancementFrame::Task,
            true, // showToast
            true, // announceToChat
            hidden);

        builder.display(std::move(display));
        auto trigger = std::make_shared<TickTriggerInstance>();
        builder.criterion("c1", trigger);

        auto result = builder.build();
        EXPECT_TRUE(result.success());
        if (!result.success()) {
            return nullptr;
        }
        return std::make_shared<Advancement>(std::move(result).value());
    }

    /**
     * @brief 创建无显示信息的技术成就
     * @param id 成就ID
     * @param parentId 父成就ID（可选）
     * @return 成就指针
     */
    Advancement::Ptr createNoDisplayAdvancement(const std::string& id, const std::string& parentId = "")
    {
        Advancement::Builder builder{ResourceLocation(id)};

        if (!parentId.empty()) {
            builder.parent(ResourceLocation(parentId));
        }

        // 不添加 display
        auto trigger = std::make_shared<TickTriggerInstance>();
        builder.criterion("c1", trigger);

        auto result = builder.build();
        EXPECT_TRUE(result.success());
        if (!result.success()) {
            return nullptr;
        }
        return std::make_shared<Advancement>(std::move(result).value());
    }

    /**
     * @brief 将成就注册到 AdvancementManager 并建立父子关系
     */
    void registerToManager(const std::vector<Advancement::Ptr>& advancements)
    {
        auto& manager = AdvancementManager::instance();
        for (const auto& adv : advancements) {
            manager.registerAdvancement(adv);
        }
    }
};

// ========== evaluateVisibilityRule 测试 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, CompletedAdvancementAlwaysVisible)
{
    // 已完成的成就始终可见，无论是否有 display
    auto root = createAdvancement("minecraft:test/root");
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return true; }, // 所有成就标记为完成
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);
}

TEST_F(AdvancementVisibilityEvaluatorTest, NoDisplayAdvancementWithChildrenDoneShowsInAlgorithm)
{
    // 无 display 的技术成就：算法层面 anyChildDone 机制会使其可见，
    // 但客户端/UI 层应进一步过滤无 display 的节点不渲染。
    // 单独一个无 display 的根节点（完成）在算法中是可见的（anyChildDone=true）。
    auto root = createNoDisplayAdvancement("minecraft:test/root_nodisplay");
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return true; }, // 完成
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    // 算法层面：anyChildDone=true -> 可见（但 UI 层应过滤不渲染）
    EXPECT_TRUE(visibility[root]);
}

TEST_F(AdvancementVisibilityEvaluatorTest, HiddenAdvancementNotVisibleBeforeCompletion)
{
    // 隐藏成就（hidden=true）在完成前不可见
    auto root = createAdvancement("minecraft:test/hidden_root", "", true);
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return false; }, // 未完成
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_FALSE(visibility[root]);
}

TEST_F(AdvancementVisibilityEvaluatorTest, HiddenAdvancementVisibleAfterCompletion)
{
    // 隐藏成就完成后可见
    auto root = createAdvancement("minecraft:test/hidden_done", "", true);
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return true; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);
}

// ========== 祖先回溯可见性测试 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, UnfinishedChildVisibleWhenParentDone)
{
    // 父成就完成 -> 子成就可见（在 VISIBILITY_DEPTH 范围内）
    auto root = createAdvancement("minecraft:test/parent_done_root");
    auto child = createAdvancement("minecraft:test/parent_done_child", "minecraft:test/parent_done_root");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);

    // 注册到 manager 以建立父子关系
    registerToManager({root, child});

    // root 完成，child 未完成
    std::set<std::string> doneSet = {"minecraft:test/parent_done_root"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);  // 父成就完成 -> 可见
    EXPECT_TRUE(visibility[child]); // 父成就完成 -> 子成就可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, UnfinishedChildNotVisibleWhenParentNotDone)
{
    // 父成就和子成就都未完成 -> 都不可见（没有已完成的祖先）
    auto root = createAdvancement("minecraft:test/parent_not_done_root");
    auto child = createAdvancement("minecraft:test/parent_not_done_child", "minecraft:test/parent_not_done_root");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);

    registerToManager({root, child});

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return false; }, // 全部未完成
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_FALSE(visibility[root]);  // 根未完成且无祖先 -> 不可见
    EXPECT_FALSE(visibility[child]); // 子未完成且无已完成祖先 -> 不可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, VisibilityDepthTwoAncestors)
{
    // VISIBILITY_DEPTH = 2，意味着未完成的非隐藏成就最多向上看 2 层祖先
    // root (完成) -> child1 (未完成) -> child2 (未完成) -> child3 (未完成)
    // child2 距离 root 1 层，child3 距离 root 2 层 -> 两者都可见
    auto root = createAdvancement("minecraft:test/depth_root");
    auto child1 = createAdvancement("minecraft:test/depth_child1", "minecraft:test/depth_root");
    auto child2 = createAdvancement("minecraft:test/depth_child2", "minecraft:test/depth_child1");
    auto child3 = createAdvancement("minecraft:test/depth_child3", "minecraft:test/depth_child2");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child1, nullptr);
    ASSERT_NE(child2, nullptr);
    ASSERT_NE(child3, nullptr);

    registerToManager({root, child1, child2, child3});

    std::set<std::string> doneSet = {"minecraft:test/depth_root"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);    // 完成 -> 可见
    EXPECT_TRUE(visibility[child1]);  // 父 root 完成 -> 可见
    EXPECT_TRUE(visibility[child2]);  // 祖先 root 在 2 层内 -> 可见
    EXPECT_FALSE(visibility[child3]); // 祖先 root 在 3 层外 -> 不可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, VisibilityDepthHiddenParentBlocksPropagation)
{
    // 隐藏的未完成父成就阻断了可见性传播
    // root (完成) -> hidden_child (隐藏, 未完成) -> grandchild (未完成)
    // hidden_child 阻断，grandchild 不可见
    auto root = createAdvancement("minecraft:test/hidden_block_root");
    auto hiddenChild = createAdvancement("minecraft:test/hidden_block_child", "minecraft:test/hidden_block_root", true);
    auto grandchild = createAdvancement("minecraft:test/hidden_block_grandchild", "minecraft:test/hidden_block_child");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(hiddenChild, nullptr);
    ASSERT_NE(grandchild, nullptr);

    registerToManager({root, hiddenChild, grandchild});

    std::set<std::string> doneSet = {"minecraft:test/hidden_block_root"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);         // 完成 -> 可见
    EXPECT_FALSE(visibility[hiddenChild]); // 隐藏且未完成 -> 不可见
    EXPECT_FALSE(visibility[grandchild]);  // 祖先链被隐藏成就阻断 -> 不可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, NoDisplayParentBlocksPropagation)
{
    // 无 display 的父成就阻断了可见性传播
    // root (完成) -> no_display_child (无 display, 未完成) -> grandchild (未完成)
    // no_display_child 阻断，grandchild 不可见
    auto root = createAdvancement("minecraft:test/nodisplay_block_root");
    auto noDisplayChild =
        createNoDisplayAdvancement("minecraft:test/nodisplay_block_child", "minecraft:test/nodisplay_block_root");
    auto grandchild =
        createAdvancement("minecraft:test/nodisplay_block_grandchild", "minecraft:test/nodisplay_block_child");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(noDisplayChild, nullptr);
    ASSERT_NE(grandchild, nullptr);

    registerToManager({root, noDisplayChild, grandchild});

    std::set<std::string> doneSet = {"minecraft:test/nodisplay_block_root"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);            // 完成 -> 可见
    EXPECT_FALSE(visibility[noDisplayChild]); // 无 display -> 不可见
    EXPECT_FALSE(visibility[grandchild]);     // 祖先链被无 display 成就阻断 -> 不可见
}

// ========== 子树完成使祖先可见 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, CompletedDescendantMakesAncestorsVisible)
{
    // 子成就完成 -> 祖先链上的非隐藏成就变为可见
    // root (未完成) -> child (未完成) -> grandchild (完成)
    auto root = createAdvancement("minecraft:test/descendant_root");
    auto child = createAdvancement("minecraft:test/descendant_child", "minecraft:test/descendant_root");
    auto grandchild = createAdvancement("minecraft:test/descendant_grandchild", "minecraft:test/descendant_child");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);
    ASSERT_NE(grandchild, nullptr);

    registerToManager({root, child, grandchild});

    std::set<std::string> doneSet = {"minecraft:test/descendant_grandchild"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);       // 子树有完成 -> 可见
    EXPECT_TRUE(visibility[child]);      // 子树有完成 -> 可见
    EXPECT_TRUE(visibility[grandchild]); // 自身完成 -> 可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, CompletedDescendantMakesNoDisplayAncestorVisibleInAlgorithm)
{
    // 子成就完成 -> anyChildDone 机制使无 display 的祖先在算法层面可见，
    // 但客户端/UI 层应进一步过滤无 display 的节点不渲染。
    // no_display_root (未完成, 无 display) -> child (完成)
    auto root = createNoDisplayAdvancement("minecraft:test/nodisplay_descendant_root");
    auto child =
        createAdvancement("minecraft:test/nodisplay_descendant_child", "minecraft:test/nodisplay_descendant_root");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);

    registerToManager({root, child});

    std::set<std::string> doneSet = {"minecraft:test/nodisplay_descendant_child"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    // 算法层面：root 的 anyChildDone=true -> 可见（但 UI 层应过滤不渲染）
    EXPECT_TRUE(visibility[root]);
    EXPECT_TRUE(visibility[child]); // 自身完成 -> 可见
}

// ========== 多分支测试 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, MultipleBranchesVisibility)
{
    // root (完成) -> childA (未完成) -> grandchildA (未完成)
    //             -> childB (未完成) -> grandchildB (未完成)
    // VISIBILITY_DEPTH=2，grandchild 距离 root 2 层：
    // depth 0 = self (NoChange), depth 1 = child (NoChange), depth 2 = root (Show) -> 可见
    auto root = createAdvancement("minecraft:test/multi_root");
    auto childA = createAdvancement("minecraft:test/multi_childA", "minecraft:test/multi_root");
    auto childB = createAdvancement("minecraft:test/multi_childB", "minecraft:test/multi_root");
    auto grandchildA = createAdvancement("minecraft:test/multi_grandchildA", "minecraft:test/multi_childA");
    auto grandchildB = createAdvancement("minecraft:test/multi_grandchildB", "minecraft:test/multi_childB");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(childA, nullptr);
    ASSERT_NE(childB, nullptr);
    ASSERT_NE(grandchildA, nullptr);
    ASSERT_NE(grandchildB, nullptr);

    registerToManager({root, childA, childB, grandchildA, grandchildB});

    std::set<std::string> doneSet = {"minecraft:test/multi_root"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);        // 完成 -> 可见
    EXPECT_TRUE(visibility[childA]);      // 父 root 完成 -> 可见
    EXPECT_TRUE(visibility[childB]);      // 父 root 完成 -> 可见
    EXPECT_TRUE(visibility[grandchildA]); // depth 2 处 root Show -> 可见
    EXPECT_TRUE(visibility[grandchildB]); // depth 2 处 root Show -> 可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, MultipleBranchesWithCompletedDescendant)
{
    // root (未完成) -> childA (完成) -> grandchildA (未完成)
    //             -> childB (未完成) -> grandchildB (未完成)
    // childA 完成使 root 的 anyChildDone=true -> root 可见
    // 但 root 的 rule 仍然是 NoChange（未完成、非隐藏、有 display）
    // childB 回溯祖先时看到 root 的 rule 是 NoChange，不是 Show
    // 所以 childB 不可见（NoChange 不传播可见性）
    auto root = createAdvancement("minecraft:test/multi_desc_root");
    auto childA = createAdvancement("minecraft:test/multi_desc_childA", "minecraft:test/multi_desc_root");
    auto childB = createAdvancement("minecraft:test/multi_desc_childB", "minecraft:test/multi_desc_root");
    auto grandchildA = createAdvancement("minecraft:test/multi_desc_grandchildA", "minecraft:test/multi_desc_childA");
    auto grandchildB = createAdvancement("minecraft:test/multi_desc_grandchildB", "minecraft:test/multi_desc_childB");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(childA, nullptr);
    ASSERT_NE(childB, nullptr);
    ASSERT_NE(grandchildA, nullptr);
    ASSERT_NE(grandchildB, nullptr);

    registerToManager({root, childA, childB, grandchildA, grandchildB});

    std::set<std::string> doneSet = {"minecraft:test/multi_desc_childA"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);         // anyChildDone=true -> 可见
    EXPECT_TRUE(visibility[childA]);       // 自身完成 -> 可见
    EXPECT_TRUE(visibility[grandchildA]);  // 父 childA 完成(Show) -> 可见
    EXPECT_FALSE(visibility[childB]);      // 祖先链：root 的 rule 是 NoChange -> 不可见
    EXPECT_FALSE(visibility[grandchildB]); // 祖先链：childB(NoChange) -> root(NoChange) -> 不可见
}

// ========== findRoot 测试 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, FindRootOfRootNode)
{
    // 根节点没有父成就，findRoot 应返回自身
    auto root = createAdvancement("minecraft:test/findroot_root");
    ASSERT_NE(root, nullptr);

    registerToManager({root});

    auto& manager = AdvancementManager::instance();
    Advancement::Ptr foundRoot = AdvancementVisibilityEvaluator::findRoot(root, manager);
    EXPECT_EQ(foundRoot, root);
}

TEST_F(AdvancementVisibilityEvaluatorTest, FindRootOfChildNode)
{
    // 子节点应该通过 parent 链找到根节点
    auto root = createAdvancement("minecraft:test/findroot_root2");
    auto child = createAdvancement("minecraft:test/findroot_child", "minecraft:test/findroot_root2");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);

    registerToManager({root, child});

    auto& manager = AdvancementManager::instance();
    Advancement::Ptr foundRoot = AdvancementVisibilityEvaluator::findRoot(child, manager);
    EXPECT_EQ(foundRoot, root);
}

TEST_F(AdvancementVisibilityEvaluatorTest, FindRootOfDeepChain)
{
    // 深层嵌套链：root -> child -> grandchild -> great_grandchild
    auto root = createAdvancement("minecraft:test/findroot_deep_root");
    auto child = createAdvancement("minecraft:test/findroot_deep_child", "minecraft:test/findroot_deep_root");
    auto grandchild =
        createAdvancement("minecraft:test/findroot_deep_grandchild", "minecraft:test/findroot_deep_child");
    auto greatGrandchild =
        createAdvancement("minecraft:test/findroot_deep_great", "minecraft:test/findroot_deep_grandchild");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);
    ASSERT_NE(grandchild, nullptr);
    ASSERT_NE(greatGrandchild, nullptr);

    registerToManager({root, child, grandchild, greatGrandchild});

    auto& manager = AdvancementManager::instance();

    EXPECT_EQ(AdvancementVisibilityEvaluator::findRoot(greatGrandchild, manager), root);
    EXPECT_EQ(AdvancementVisibilityEvaluator::findRoot(grandchild, manager), root);
    EXPECT_EQ(AdvancementVisibilityEvaluator::findRoot(child, manager), root);
    EXPECT_EQ(AdvancementVisibilityEvaluator::findRoot(root, manager), root);
}

TEST_F(AdvancementVisibilityEvaluatorTest, FindRootNullNode)
{
    auto& manager = AdvancementManager::instance();
    Advancement::Ptr foundRoot = AdvancementVisibilityEvaluator::findRoot(nullptr, manager);
    EXPECT_EQ(foundRoot, nullptr);
}

// ========== evaluateVisibilityFromNode 测试 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, EvaluateVisibilityFromNode)
{
    // 从非根节点开始评估，应自动找到根并评估整棵树
    auto root = createAdvancement("minecraft:test/fromnode_root");
    auto child = createAdvancement("minecraft:test/fromnode_child", "minecraft:test/fromnode_root");
    auto grandchild = createAdvancement("minecraft:test/fromnode_grandchild", "minecraft:test/fromnode_child");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);
    ASSERT_NE(grandchild, nullptr);

    registerToManager({root, child, grandchild});

    auto& manager = AdvancementManager::instance();

    std::set<std::string> doneSet = {"minecraft:test/fromnode_root"};
    std::map<Advancement::Ptr, bool> visibility;

    // 从 grandchild 开始评估，应该自动找到 root 并评估整棵树
    AdvancementVisibilityEvaluator::evaluateVisibilityFromNode(
        grandchild,
        manager,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);       // 完成 -> 可见
    EXPECT_TRUE(visibility[child]);      // 父完成 -> 可见
    EXPECT_TRUE(visibility[grandchild]); // 祖先在 depth 2 内 -> 可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, EvaluateVisibilityFromNullNode)
{
    auto& manager = AdvancementManager::instance();

    // 从空节点评估不应崩溃
    AdvancementVisibilityEvaluator::evaluateVisibilityFromNode(
        nullptr, manager, [](Advancement::Ptr) { return false; }, [](Advancement::Ptr, bool) {});
}

// ========== 边界条件测试 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, NullStartNode)
{
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        nullptr,
        [](Advancement::Ptr) { return false; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility.empty());
}

TEST_F(AdvancementVisibilityEvaluatorTest, SingleRootNodeAllDone)
{
    auto root = createAdvancement("minecraft:test/single_root_done");
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return true; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);
}

TEST_F(AdvancementVisibilityEvaluatorTest, SingleRootNodeNoneDone)
{
    auto root = createAdvancement("minecraft:test/single_root_notdone");
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return false; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    // 根成就未完成、非隐藏 -> 没有已完成祖先 -> 不可见
    EXPECT_FALSE(visibility[root]);
}

TEST_F(AdvancementVisibilityEvaluatorTest, MixedTreeWithNoDisplayAndHidden)
{
    // 复杂混合树：
    // root (完成, 有 display)
    //   -> tech_node (未完成, 无 display) -> tech_child (未完成, 有 display)
    //   -> visible_child (未完成, 有 display)
    //   -> hidden_child (未完成, 隐藏)
    auto root = createAdvancement("minecraft:test/mixed_root");
    auto techNode = createNoDisplayAdvancement("minecraft:test/mixed_tech", "minecraft:test/mixed_root");
    auto techChild = createAdvancement("minecraft:test/mixed_tech_child", "minecraft:test/mixed_tech");
    auto visibleChild = createAdvancement("minecraft:test/mixed_visible", "minecraft:test/mixed_root");
    auto hiddenChild = createAdvancement("minecraft:test/mixed_hidden", "minecraft:test/mixed_root", true);
    ASSERT_NE(root, nullptr);
    ASSERT_NE(techNode, nullptr);
    ASSERT_NE(techChild, nullptr);
    ASSERT_NE(visibleChild, nullptr);
    ASSERT_NE(hiddenChild, nullptr);

    registerToManager({root, techNode, techChild, visibleChild, hiddenChild});

    std::set<std::string> doneSet = {"minecraft:test/mixed_root"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);         // 完成 -> 可见
    EXPECT_FALSE(visibility[techNode]);    // 无 display -> 不可见
    EXPECT_FALSE(visibility[techChild]);   // 父 tech 无 display (Hide) -> 阻断 -> 不可见
    EXPECT_TRUE(visibility[visibleChild]); // 父 root 完成(Show) -> 可见
    EXPECT_FALSE(visibility[hiddenChild]); // 隐藏且未完成 -> 不可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, VisibilityDepthExact)
{
    // 测试 VISIBILITY_DEPTH 边界：
    // root (完成) -> L1 (未完成) -> L2 (未完成) -> L3 (未完成)
    // L1 距离 root 0 层祖先内（自身）-> root Show 在 depth 1 -> 可见
    // L2 距离 root 1 层祖先 -> root Show 在 depth 2 -> 可见
    // L3 距离 root 2 层祖先 -> root Show 在 depth 3，超出 VISIBILITY_DEPTH=2 -> 不可见

    // 但注意：anyChildDone 机制可能影响结果。
    // root 的 anyChildDone=true (因为 L1 等子树存在且 L1 有 root Show 作为祖先，虽然 L1 本身 NoChange)
    // 实际上 anyChildDone 只看 isDone，不关心可见性。isDone 回调只标记 root 为完成。

    auto root = createAdvancement("minecraft:test/exact_root");
    auto L1 = createAdvancement("minecraft:test/exact_L1", "minecraft:test/exact_root");
    auto L2 = createAdvancement("minecraft:test/exact_L2", "minecraft:test/exact_L1");
    auto L3 = createAdvancement("minecraft:test/exact_L3", "minecraft:test/exact_L2");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(L1, nullptr);
    ASSERT_NE(L2, nullptr);
    ASSERT_NE(L3, nullptr);

    registerToManager({root, L1, L2, L3});

    std::set<std::string> doneSet = {"minecraft:test/exact_root"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]); // 完成 -> 可见
    EXPECT_TRUE(visibility[L1]);   // depth 1 处 root Show -> 可见
    EXPECT_TRUE(visibility[L2]);   // depth 2 处 root Show -> 可见
    EXPECT_FALSE(visibility[L3]);  // depth 3 处 root Show 超出 VISIBILITY_DEPTH=2 -> 不可见
}

// ========== _evaluateVisibilityRule 间接测试 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, VisibilityRuleShowForDone)
{
    // 已完成成就 -> Show 规则
    auto root = createAdvancement("minecraft:test/rule_show");
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return true; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]); // Show -> 可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, VisibilityRuleHideForNoDisplay)
{
    // 无 display 成就 -> Hide 规则，但 anyChildDone 机制会覆盖
    // 算法层面：单个已完成的 no-display 节点，anyChildDone=true -> 可见
    // 客户端/UI 层应进一步过滤无 display 的节点不渲染
    auto root = createNoDisplayAdvancement("minecraft:test/rule_hide_nodisplay");
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return true; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    // 算法层面：anyChildDone=true -> 可见（但 UI 层应过滤）
    EXPECT_TRUE(visibility[root]);
}

TEST_F(AdvancementVisibilityEvaluatorTest, VisibilityRuleHideForHiddenNotDone)
{
    // 隐藏且未完成 -> Hide 规则
    auto root = createAdvancement("minecraft:test/rule_hide_hidden", "", true);
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return false; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_FALSE(visibility[root]); // Hide -> 不可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, VisibilityRuleNoChangeForUnfinishedVisible)
{
    // 非隐藏、未完成、有 display -> NoChange 规则
    // 作为根节点，没有已完成祖先 -> 不可见
    auto root = createAdvancement("minecraft:test/rule_nochange");
    ASSERT_NE(root, nullptr);

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return false; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_FALSE(visibility[root]); // NoChange + 没有已完成祖先 -> 不可见
}

// ========== 复杂级联测试 ==========

TEST_F(AdvancementVisibilityEvaluatorTest, CompletingMiddleNodeAffectsSubtree)
{
    // 完成中间节点影响子树可见性
    // root (未完成) -> middle (完成) -> leaf (未完成)
    // middle 完成 -> middle 可见，root 因 anyChildDone 可见，leaf 因 middle(Show) 可见
    auto root = createAdvancement("minecraft:test/cascade_root");
    auto middle = createAdvancement("minecraft:test/cascade_middle", "minecraft:test/cascade_root");
    auto leaf = createAdvancement("minecraft:test/cascade_leaf", "minecraft:test/cascade_middle");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(middle, nullptr);
    ASSERT_NE(leaf, nullptr);

    registerToManager({root, middle, leaf});

    std::set<std::string> doneSet = {"minecraft:test/cascade_middle"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);   // 子树 middle 完成(anyChildDone) -> 可见
    EXPECT_TRUE(visibility[middle]); // 自身完成 -> 可见
    EXPECT_TRUE(visibility[leaf]);   // 父 middle 完成(Show) -> 可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, AllNodesComplete)
{
    // 所有成就完成 -> 全部可见
    auto root = createAdvancement("minecraft:test/all_done_root");
    auto child = createAdvancement("minecraft:test/all_done_child", "minecraft:test/all_done_root");
    auto grandchild = createAdvancement("minecraft:test/all_done_grandchild", "minecraft:test/all_done_child");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);
    ASSERT_NE(grandchild, nullptr);

    registerToManager({root, child, grandchild});

    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [](Advancement::Ptr) { return true; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);
    EXPECT_TRUE(visibility[child]);
    EXPECT_TRUE(visibility[grandchild]);
}

TEST_F(AdvancementVisibilityEvaluatorTest, HiddenMiddleCompletedShowsChildren)
{
    // 隐藏的中间节点完成后，其子节点可见
    // root (完成) -> hidden_middle (隐藏, 完成) -> visible_child (未完成)
    auto root = createAdvancement("minecraft:test/hidden_middle_root");
    auto hiddenMiddle = createAdvancement("minecraft:test/hidden_middle", "minecraft:test/hidden_middle_root", true);
    auto visibleChild = createAdvancement("minecraft:test/hidden_middle_child", "minecraft:test/hidden_middle");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(hiddenMiddle, nullptr);
    ASSERT_NE(visibleChild, nullptr);

    registerToManager({root, hiddenMiddle, visibleChild});

    std::set<std::string> doneSet = {"minecraft:test/hidden_middle_root", "minecraft:test/hidden_middle"};
    std::map<Advancement::Ptr, bool> visibility;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { visibility[adv] = visible; });

    EXPECT_TRUE(visibility[root]);         // 完成 -> 可见
    EXPECT_TRUE(visibility[hiddenMiddle]); // 隐藏但已完成 -> 可见
    EXPECT_TRUE(visibility[visibleChild]); // 父 hiddenMiddle 完成(Show) -> 可见
}

TEST_F(AdvancementVisibilityEvaluatorTest, MultipleTreesEvaluation)
{
    // 多棵独立的成就树
    auto tree1Root = createAdvancement("minecraft:test/tree1_root");
    auto tree2Root = createAdvancement("minecraft:test/tree2_root");
    ASSERT_NE(tree1Root, nullptr);
    ASSERT_NE(tree2Root, nullptr);

    registerToManager({tree1Root, tree2Root});

    auto& manager = AdvancementManager::instance();

    // tree1 完成，tree2 未完成
    std::set<std::string> doneSet = {"minecraft:test/tree1_root"};

    // 分别评估每棵树
    std::map<Advancement::Ptr, bool> vis1;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        tree1Root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { vis1[adv] = visible; });

    std::map<Advancement::Ptr, bool> vis2;
    AdvancementVisibilityEvaluator::evaluateVisibility(
        tree2Root,
        [&doneSet](Advancement::Ptr adv) { return doneSet.count(adv->getId().toString()) > 0; },
        [&](Advancement::Ptr adv, bool visible) { vis2[adv] = visible; });

    EXPECT_TRUE(vis1[tree1Root]);  // tree1 完成 -> 可见
    EXPECT_FALSE(vis2[tree2Root]); // tree2 未完成 -> 不可见
}
