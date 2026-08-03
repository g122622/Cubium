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

#include "LayoutEngine.hpp"
#include "LayoutEngineAdapters.hpp"

#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/algorithms/FlexLayout.hpp"
#include "client/ui/kagero/layout/algorithms/GridLayout.hpp"
#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/core/LayoutResult.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "common/core/Types.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <ratio>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::layout {

namespace {

/**
 * @brief 将算法计算结果应用到子 Widget
 *
 * 仅在结果有效时才写回目标 Widget，避免无效布局污染现有状态。
 */
void applyLayoutResults(const std::vector<WidgetLayoutAdaptor*>& children, const std::vector<LayoutResult>& results)
{
    const size_t count = std::min(children.size(), results.size());
    for (size_t i = 0; i < count; ++i) {
        if (children[i] != nullptr && results[i].isValid()) {
            children[i]->applyLayout(results[i]);
        }
    }
}

} // namespace

// ============================================================================
// LayoutEngine 实现
// ============================================================================

LayoutEngine::LayoutEngine()
    : m_flexLayout(std::make_unique<FlexLayout>())
    , m_gridLayout(std::make_unique<GridLayout>())
{
    registerAlgorithm("flex", std::make_unique<FlexLayoutAlgorithm>(FlexConfig{}));
    registerAlgorithm("flex-row", std::make_unique<FlexLayoutAlgorithm>([]() {
        FlexConfig config;
        config.direction = Direction::Row;
        return config;
    }()));
    registerAlgorithm("flex-column", std::make_unique<FlexLayoutAlgorithm>([]() {
        FlexConfig config;
        config.direction = Direction::Column;
        return config;
    }()));
    registerAlgorithm("flex-center", std::make_unique<FlexLayoutAlgorithm>(centerRowFlexConfig()));
    registerAlgorithm("grid", std::make_unique<detail::GridLayoutAlgorithm>());
    registerAlgorithm("anchor", std::make_unique<detail::AnchorLayoutAlgorithm>());
    registerAlgorithm("stack", std::make_unique<detail::StackLayoutAlgorithm>());
}

LayoutEngine& LayoutEngine::instance()
{
    static LayoutEngine instance;
    return instance;
}

void LayoutEngine::registerAlgorithm(const std::string& name, std::unique_ptr<ILayoutAlgorithm> algorithm)
{
    m_algorithms[name] = std::move(algorithm);
}

ILayoutAlgorithm* LayoutEngine::getAlgorithm(const std::string& name) const
{
    const auto it = m_algorithms.find(name);
    return it != m_algorithms.end() ? it->second.get() : nullptr;
}

bool LayoutEngine::hasAlgorithm(const std::string& name) const
{
    return m_algorithms.find(name) != m_algorithms.end();
}

void LayoutEngine::layout(WidgetLayoutAdaptor* root, const Rect& availableSpace)
{
    if (root == nullptr || !root->isValid()) {
        return;
    }

    m_stats.reset();
    const auto startTime = std::chrono::high_resolution_clock::now();

    const Rect rootBounds(
        availableSpace.x, availableSpace.y, std::max(0, availableSpace.width), std::max(0, availableSpace.height));
    root->applyLayout(LayoutResult(rootBounds));

    const MeasureSpec widthSpec = MeasureSpec::MakeExactly(rootBounds.width);
    const MeasureSpec heightSpec = MeasureSpec::MakeExactly(rootBounds.height);
    _layoutNode(root, widthSpec, heightSpec, 0);

    const auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs = std::chrono::duration<f64, std::milli>(endTime - startTime).count();
    m_stats.layoutCount = 1;
}

void LayoutEngine::layoutDirty(WidgetLayoutAdaptor* root)
{
    if (root == nullptr || !root->isValid()) {
        return;
    }

    m_stats.reset();
    const auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<WidgetLayoutAdaptor*> dirtyNodes;
    _collectDirtyNodes(root, dirtyNodes);
    if (dirtyNodes.empty()) {
        return;
    }

    std::sort(dirtyNodes.begin(), dirtyNodes.end(), [](WidgetLayoutAdaptor* lhs, WidgetLayoutAdaptor* rhs) {
        return lhs->depth() < rhs->depth();
    });

    m_stats.relayoutedWidgets = static_cast<i32>(dirtyNodes.size());

    for (auto* node : dirtyNodes) {
        if (node == nullptr || !node->isLayoutDirty()) {
            continue;
        }

        const Rect bounds = node->currentBounds();
        const MeasureSpec widthSpec = MeasureSpec::MakeExactly(std::max(0, bounds.width));
        const MeasureSpec heightSpec = MeasureSpec::MakeExactly(std::max(0, bounds.height));
        _layoutNode(node, widthSpec, heightSpec, node->depth());
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs = std::chrono::duration<f64, std::milli>(endTime - startTime).count();
    m_stats.layoutCount = 1;
}

void LayoutEngine::layoutWith(
    const std::string& algorithmName, WidgetLayoutAdaptor* container, const Rect& availableSpace)
{
    if (container == nullptr || !container->isValid()) {
        return;
    }

    ILayoutAlgorithm* algorithm = _selectAlgorithm(LayoutType::Flex, algorithmName);
    if (algorithm == nullptr) {
        return;
    }

    m_stats.reset();
    const auto startTime = std::chrono::high_resolution_clock::now();

    container->applyLayout(LayoutResult(availableSpace));
    auto children = container->getChildren();
    const auto results = algorithm->compute(availableSpace, children, container->constraints());
    applyLayoutResults(children, results);

    const auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs = std::chrono::duration<f64, std::milli>(endTime - startTime).count();
    m_stats.layoutCount = 1;
}

void LayoutEngine::layoutFlex(WidgetLayoutAdaptor* container, const Rect& availableSpace, const FlexConfig& config)
{
    if (container == nullptr || !container->isValid()) {
        return;
    }

    m_stats.reset();
    const auto startTime = std::chrono::high_resolution_clock::now();

    container->applyLayout(LayoutResult(availableSpace));
    m_flexLayout->setConfig(config);

    auto children = container->getChildren();
    const auto results = m_flexLayout->compute(availableSpace, children, container->constraints());
    applyLayoutResults(children, results);

    const auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs = std::chrono::duration<f64, std::milli>(endTime - startTime).count();
    m_stats.layoutCount = 1;
}

void LayoutEngine::layoutGrid(WidgetLayoutAdaptor* container, const Rect& availableSpace, const GridConfig& config)
{
    if (container == nullptr || !container->isValid()) {
        return;
    }

    m_stats.reset();
    const auto startTime = std::chrono::high_resolution_clock::now();

    container->applyLayout(LayoutResult(availableSpace));
    m_gridLayout->setConfig(config);

    auto children = container->getChildren();
    const auto results = m_gridLayout->compute(availableSpace, children);
    applyLayoutResults(children, results);

    const auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs = std::chrono::duration<f64, std::milli>(endTime - startTime).count();
    m_stats.layoutCount = 1;
}

LayoutResult LayoutEngine::_layoutNode(
    WidgetLayoutAdaptor* node, const MeasureSpec& widthSpec, const MeasureSpec& heightSpec, i32 depth)
{
    if (node == nullptr || !node->isValid()) {
        return LayoutResult();
    }

    m_stats.totalWidgets++;
    m_stats.maxDepth = std::max(m_stats.maxDepth, depth);
    m_stats.measureCount++;

    const Size measuredSize = node->measure(widthSpec, heightSpec);
    i32 finalWidth = widthSpec.resolve(measuredSize.width);
    i32 finalHeight = heightSpec.resolve(measuredSize.height);

    const auto& constraints = node->constraints();
    finalWidth = constraints.clampWidth(finalWidth);
    finalHeight = constraints.clampHeight(finalHeight);

    LayoutResult result(0, 0, finalWidth, finalHeight);

    if (node->isContainer()) {
        auto children = node->getChildren();
        m_stats.totalWidgets += static_cast<i32>(children.size());

        if (!children.empty()) {
            // 从ContainerWidget获取FlexConfig，而非使用默认空配置
            auto* containerWidget = dynamic_cast<widget::ContainerWidget*>(node->getWidget());
            if (containerWidget != nullptr && containerWidget->layoutType() == widget::ContainerLayoutType::Flex) {
                m_flexLayout->setConfig(containerWidget->flexConfig());
            } else {
                m_flexLayout->setConfig(FlexConfig{});
            }

            const i32 contentWidth = std::max(0, finalWidth - constraints.padding.horizontal());
            const i32 contentHeight = std::max(0, finalHeight - constraints.padding.vertical());
            const Rect contentRect(constraints.padding.left, constraints.padding.top, contentWidth, contentHeight);

            const auto childResults = m_flexLayout->compute(contentRect, children, constraints);
            for (size_t i = 0; i < children.size() && i < childResults.size(); ++i) {
                if (children[i] == nullptr || !childResults[i].isValid()) {
                    continue;
                }

                const MeasureSpec childWidthSpec = MeasureSpec::MakeExactly(std::max(0, childResults[i].bounds.width));
                const MeasureSpec childHeightSpec =
                    MeasureSpec::MakeExactly(std::max(0, childResults[i].bounds.height));

                _layoutNode(children[i], childWidthSpec, childHeightSpec, depth + 1);

                LayoutResult appliedResult = childResults[i];
                appliedResult.bounds.x += result.bounds.x;
                appliedResult.bounds.y += result.bounds.y;
                children[i]->applyLayout(appliedResult);
            }
        }
    }

    node->clearLayoutDirty();
    return result;
}

void LayoutEngine::_collectDirtyNodes(WidgetLayoutAdaptor* node, std::vector<WidgetLayoutAdaptor*>& out)
{
    if (node == nullptr) {
        return;
    }

    if (node->isLayoutDirty()) {
        out.push_back(node);
        return;
    }

    auto children = node->getChildren();
    for (auto* child : children) {
        _collectDirtyNodes(child, out);
    }
}

ILayoutAlgorithm* LayoutEngine::_selectAlgorithm(LayoutType type, const std::string& name)
{
    if (!name.empty()) {
        if (auto* algorithm = getAlgorithm(name); algorithm != nullptr) {
            return algorithm;
        }
    }

    switch (type) {
        case LayoutType::Flex:
            return getAlgorithm("flex");
        case LayoutType::Grid:
            return getAlgorithm("grid");
        case LayoutType::Anchor:
            return getAlgorithm("anchor");
        case LayoutType::Stack:
            return getAlgorithm("stack");
        case LayoutType::None:
        default:
            return nullptr;
    }
}

} // namespace mc::client::ui::kagero::layout
