#include "LayoutEngine.hpp"
#include "LayoutEngineAdapters.hpp"
#include <algorithm>
#include <chrono>

namespace mc::client::ui::kagero::layout {

// ============================================================================
// LayoutEngine 实现
// ============================================================================

LayoutEngine::LayoutEngine()
    : m_flexLayout(std::make_unique<FlexLayout>()) {
    // 注册内置算法
    registerAlgorithm("flex", std::make_unique<FlexLayoutAlgorithm>());
    registerAlgorithm("flex-row", std::make_unique<FlexLayoutAlgorithm>([](){
        FlexConfig config;
        config.direction = Direction::Row;
        return config;
    }()));
    registerAlgorithm("flex-column", std::make_unique<FlexLayoutAlgorithm>([](){
        FlexConfig config;
        config.direction = Direction::Column;
        return config;
    }()));
    registerAlgorithm("flex-center", std::make_unique<FlexLayoutAlgorithm>(centerRowFlexConfig()));
    registerAlgorithm("grid", std::make_unique<detail::GridLayoutAlgorithm>());
    registerAlgorithm("anchor", std::make_unique<detail::AnchorLayoutAlgorithm>());
    registerAlgorithm("stack", std::make_unique<detail::StackLayoutAlgorithm>());
}

LayoutEngine& LayoutEngine::instance() {
    static LayoutEngine instance;
    return instance;
}

void LayoutEngine::registerAlgorithm(
    const String& name,
    std::unique_ptr<ILayoutAlgorithm> algorithm
) {
    m_algorithms[name] = std::move(algorithm);
}

ILayoutAlgorithm* LayoutEngine::getAlgorithm(const String& name) const {
    auto it = m_algorithms.find(name);
    return (it != m_algorithms.end()) ? it->second.get() : nullptr;
}

bool LayoutEngine::hasAlgorithm(const String& name) const {
    return m_algorithms.find(name) != m_algorithms.end();
}

void LayoutEngine::layout(WidgetLayoutAdaptor* root, const Rect& availableSpace) {
    if (!root || !root->isValid()) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    m_stats.reset();

    // 创建测量规格
    MeasureSpec widthSpec = MeasureSpec::MakeExactly(availableSpace.width);
    MeasureSpec heightSpec = MeasureSpec::MakeExactly(availableSpace.height);

    // 执行布局
    layoutNode(root, widthSpec, heightSpec, 0);

    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs = std::chrono::duration<f64, std::milli>(endTime - startTime).count();
}

void LayoutEngine::layoutDirty(WidgetLayoutAdaptor* root) {
    if (!root || !root->isValid()) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    // 收集dirty节点
    std::vector<WidgetLayoutAdaptor*> dirtyNodes;
    collectDirtyNodes(root, dirtyNodes);

    if (dirtyNodes.empty()) {
        return;  // 没有需要重新布局的节点
    }

    // 按深度排序（浅层优先）
    std::sort(dirtyNodes.begin(), dirtyNodes.end(),
        [](WidgetLayoutAdaptor* a, WidgetLayoutAdaptor* b) {
            return a->depth() < b->depth();
        });

    // 重新布局dirty节点
    m_stats.relayoutedWidgets = static_cast<i32>(dirtyNodes.size());

    for (auto* node : dirtyNodes) {
        if (!node->isLayoutDirty()) continue;

        // 获取父节点的约束（简化实现：使用节点的当前尺寸）
        Rect bounds = node->currentBounds();
        MeasureSpec widthSpec = MeasureSpec::MakeExactly(bounds.width);
        MeasureSpec heightSpec = MeasureSpec::MakeExactly(bounds.height);

        layoutNode(node, widthSpec, heightSpec, node->depth());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs = std::chrono::duration<f64, std::milli>(endTime - startTime).count();
}

void LayoutEngine::layoutWith(
    const String& algorithmName,
    WidgetLayoutAdaptor* container,
    const Rect& availableSpace
) {
    ILayoutAlgorithm* algorithm = getAlgorithm(algorithmName);
    if (!algorithm) {
        // 默认使用flex布局
        algorithm = getAlgorithm("flex");
    }

    if (!algorithm || !container || !container->isValid()) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    // 获取子元素
    auto children = container->getChildren();

    // 执行布局计算
    auto results = algorithm->compute(availableSpace, children, container->constraints());

    // 应用结果
    for (size_t i = 0; i < children.size() && i < results.size(); ++i) {
        if (children[i] && results[i].isValid()) {
            children[i]->applyLayout(results[i]);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs += std::chrono::duration<f64, std::milli>(endTime - startTime).count();
    m_stats.layoutCount++;
}

void LayoutEngine::layoutFlex(
    WidgetLayoutAdaptor* container,
    const Rect& availableSpace,
    const FlexConfig& config
) {
    if (!container || !container->isValid()) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    m_flexLayout->setConfig(config);

    // 获取子元素
    auto children = container->getChildren();

    // 执行布局计算
    auto results = m_flexLayout->compute(availableSpace, children, container->constraints());

    // 应用结果
    for (size_t i = 0; i < children.size() && i < results.size(); ++i) {
        if (children[i] && results[i].isValid()) {
            children[i]->applyLayout(results[i]);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalTimeMs += std::chrono::duration<f64, std::milli>(endTime - startTime).count();
    m_stats.layoutCount++;
}

LayoutResult LayoutEngine::layoutNode(
    WidgetLayoutAdaptor* node,
    const MeasureSpec& widthSpec,
    const MeasureSpec& heightSpec,
    i32 depth
) {
    if (!node || !node->isValid()) {
        return LayoutResult();
    }

    m_stats.totalWidgets++;
    m_stats.maxDepth = std::max(m_stats.maxDepth, depth);
    m_stats.measureCount++;

    // 测量节点
    Size measuredSize = node->measure(widthSpec, heightSpec);

    // 解析最终尺寸
    i32 finalWidth = widthSpec.resolve(measuredSize.width);
    i32 finalHeight = heightSpec.resolve(measuredSize.height);

    // 应用约束
    const auto& constraints = node->constraints();
    finalWidth = constraints.clampWidth(finalWidth);
    finalHeight = constraints.clampHeight(finalHeight);

    // 创建布局结果
    LayoutResult result(0, 0, finalWidth, finalHeight);

    // 如果是容器，布局子元素
    if (node->isContainer()) {
        auto children = node->getChildren();
        m_stats.totalWidgets += static_cast<i32>(children.size());

        if (!children.empty()) {
            // 默认使用flex布局
            m_flexLayout->setConfig(FlexConfig{});

            Rect contentRect(
                constraints.padding.left,
                constraints.padding.top,
                finalWidth - constraints.padding.horizontal(),
                finalHeight - constraints.padding.vertical()
            );

            auto childResults = m_flexLayout->compute(contentRect, children, constraints);

            for (size_t i = 0; i < children.size() && i < childResults.size(); ++i) {
                if (children[i] && childResults[i].isValid()) {
                    // 递归布局子元素
                    MeasureSpec childWidthSpec = MeasureSpec::MakeExactly(childResults[i].bounds.width);
                    MeasureSpec childHeightSpec = MeasureSpec::MakeExactly(childResults[i].bounds.height);

                    layoutNode(children[i], childWidthSpec, childHeightSpec, depth + 1);

                    // 应用位置
                    childResults[i].bounds.x += result.bounds.x;
                    childResults[i].bounds.y += result.bounds.y;
                    children[i]->applyLayout(childResults[i]);
                }
            }
        }
    }

    // 清除脏标记
    node->clearLayoutDirty();

    return result;
}

void LayoutEngine::collectDirtyNodes(
    WidgetLayoutAdaptor* node,
    std::vector<WidgetLayoutAdaptor*>& out
) {
    if (!node) return;

    if (node->isLayoutDirty()) {
        out.push_back(node);

        // 如果父节点dirty，子节点会被父节点的布局自动更新，不需要单独处理
        return;
    }

    // 递归检查子节点
    auto children = node->getChildren();
    for (auto* child : children) {
        collectDirtyNodes(child, out);
    }
}

ILayoutAlgorithm* LayoutEngine::selectAlgorithm(LayoutType type, const String& name) {
    // 首先尝试按名称查找
    if (!name.empty()) {
        ILayoutAlgorithm* algo = getAlgorithm(name);
        if (algo) return algo;
    }

    // 按类型选择
    switch (type) {
        case LayoutType::Flex:
            return getAlgorithm("flex");
        case LayoutType::Grid:
            // TODO: 实现Grid布局
            return getAlgorithm("flex");  // 临时使用flex
        case LayoutType::Anchor:
            // TODO: 实现Anchor布局
            return nullptr;
        case LayoutType::Stack:
            // TODO: 实现Stack布局
            return nullptr;
        case LayoutType::None:
        default:
            return nullptr;
    }
}

} // namespace mc::client::ui::kagero::layout
