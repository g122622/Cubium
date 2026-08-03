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

#include "FlexLayout.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/core/LayoutResult.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::layout {

// ============================================================================
// FlexLayout 实现
// ============================================================================

std::vector<LayoutResult> FlexLayout::compute(const Rect& containerBounds,
    const std::vector<WidgetLayoutAdaptor*>& children,
    const LayoutConstraints& containerConstraints)
{
    if (children.empty()) {
        return {};
    }

    // 清除之前的状态
    m_lines.clear();
    m_results.clear();
    m_results.resize(children.size());
    m_measuredSizes.clear();
    m_measuredSizes.resize(children.size());

    const bool isHorizontal = m_config.isHorizontal();
    const i32 containerMainSize = isHorizontal ? containerBounds.width : containerBounds.height;
    const i32 containerCrossSize = isHorizontal ? containerBounds.height : containerBounds.width;

    // 减去容器的内边距
    i32 availableMainSize = containerMainSize - containerConstraints.padding.horizontal();
    i32 availableCrossSize = containerCrossSize - containerConstraints.padding.vertical();

    if (availableMainSize < 0) availableMainSize = 0;
    if (availableCrossSize < 0) availableCrossSize = 0;

    // 创建主轴和交叉轴测量规格
    MeasureSpec mainSpec = MeasureSpec::MakeAtMost(availableMainSize);
    MeasureSpec crossSpec = MeasureSpec::MakeAtMost(availableCrossSize);

    // 步骤1：测量所有子元素
    _measureChildren(children, availableMainSize, isHorizontal);

    // 步骤2：收集行（考虑换行）
    _collectLines(children, availableMainSize, isHorizontal);

    // 步骤3：计算每行的布局
    i32 totalCrossSize = 0;
    for (auto& line : m_lines) {
        // 计算交叉轴尺寸（行高或行宽）
        i32 lineCrossSize = 0;
        for (size_t i = 0; i < line.items.size(); ++i) {
            size_t idx = line.indices[i];
            auto* child = line.items[i];
            i32 childCross = isHorizontal ? m_measuredSizes[idx].height : m_measuredSizes[idx].width;
            childCross += child->constraints().margin.vertical();
            lineCrossSize = std::max(lineCrossSize, childCross);
        }
        line.crossSize = lineCrossSize;
        totalCrossSize += lineCrossSize;
    }

    // 加上间距
    if (!m_lines.empty()) {
        totalCrossSize += static_cast<i32>(m_lines.size() - 1) * m_config.gap;
    }

    // 步骤4：分配剩余空间/缩小溢出空间
    for (auto& line : m_lines) {
        // 计算行的主轴总尺寸（含间距）
        i32 lineMainSize = 0;
        for (size_t i = 0; i < line.items.size(); ++i) {
            size_t idx = line.indices[i];
            auto* child = line.items[i];
            i32 childMain = isHorizontal ? m_measuredSizes[idx].width : m_measuredSizes[idx].height;
            childMain += child->constraints().margin.horizontal();
            lineMainSize += childMain;
        }

        // 加上间距
        if (!line.items.empty()) {
            lineMainSize += static_cast<i32>(line.items.size() - 1) * m_config.gap;
        }

        line.mainSize = lineMainSize;

        // 计算剩余空间或溢出
        i32 freeSpace = availableMainSize - lineMainSize;

        if (freeSpace > 0 && line.totalGrow > 0.0f) {
            // 有剩余空间，分配给可增长的元素
            _distributeFreeSpace(line, freeSpace, isHorizontal);
        } else if (freeSpace < 0 && line.totalShrink > 0.0f) {
            // 空间不足，缩小可缩小的元素
            _shrinkSpace(line, -freeSpace, isHorizontal);
        }

        // 应用主轴对齐
        _applyJustifyContent(line, availableMainSize, isHorizontal);
    }

    // 步骤5：应用交叉轴对齐并定位子元素
    i32 currentCrossOffset = 0;
    if (m_config.wrap == Wrap::WrapReverse && m_lines.size() > 1) {
        // 反向换行：从最后一行开始
        currentCrossOffset = availableCrossSize - totalCrossSize;
        if (currentCrossOffset < 0) currentCrossOffset = 0;
    }

    // 交叉轴整体对齐
    i32 crossAlignmentOffset = 0;
    if (totalCrossSize < availableCrossSize) {
        switch (m_config.alignItems) {
            case Align::Center:
                crossAlignmentOffset = (availableCrossSize - totalCrossSize) / 2;
                break;
            case Align::End:
                crossAlignmentOffset = availableCrossSize - totalCrossSize;
                break;
            default:
                break;
        }
    }

    currentCrossOffset += crossAlignmentOffset;

    for (size_t lineIdx = 0; lineIdx < m_lines.size(); ++lineIdx) {
        auto& line = m_lines[lineIdx];

        // 应用交叉轴对齐
        _applyAlignItems(line, line.crossSize, isHorizontal);

        // 设置子元素位置
        for (size_t i = 0; i < line.items.size(); ++i) {
            size_t idx = line.indices[i];
            auto* child = line.items[i];
            auto& result = m_results[idx];

            // 调整位置（考虑容器内边距）
            if (isHorizontal) {
                result.bounds.x += containerBounds.x + containerConstraints.padding.left;
                result.bounds.y += containerBounds.y + containerConstraints.padding.top + currentCrossOffset;
            } else {
                result.bounds.x += containerBounds.x + containerConstraints.padding.left + currentCrossOffset;
                result.bounds.y += containerBounds.y + containerConstraints.padding.top;
            }

            // 反向排列调整
            if (m_config.isReverse()) {
                if (isHorizontal) {
                    result.bounds.x = containerBounds.right() - containerConstraints.padding.right -
                        (result.bounds.x - containerBounds.x - containerConstraints.padding.left) - result.bounds.width;
                } else {
                    result.bounds.y = containerBounds.bottom() - containerConstraints.padding.bottom -
                        (result.bounds.y - containerBounds.y - containerConstraints.padding.top) - result.bounds.height;
                }
            }
        }

        currentCrossOffset += line.crossSize + m_config.gap;
    }

    return m_results;
}

Size FlexLayout::measure(
    const MeasureSpec& widthSpec, const MeasureSpec& heightSpec, const std::vector<WidgetLayoutAdaptor*>& children)
{
    if (children.empty()) {
        // 即使没有子元素，也要尊重测量规格
        return Size(widthSpec.resolve(0), heightSpec.resolve(0));
    }

    // 清除之前的状态
    m_lines.clear();
    m_results.clear();
    m_measuredSizes.clear();
    m_measuredSizes.resize(children.size());

    const bool isHorizontal = m_config.isHorizontal();
    const i32 mainSizeLimit = isHorizontal ? widthSpec.size : heightSpec.size;

    // 测量所有子元素
    _measureChildren(children, mainSizeLimit, isHorizontal);

    // 收集行
    _collectLines(children, mainSizeLimit, isHorizontal);

    // 计算总尺寸
    i32 totalMainSize = 0;
    i32 totalCrossSize = 0;
    i32 maxLineMainSize = 0;

    for (const auto& line : m_lines) {
        i32 lineMainSize = 0;
        i32 lineCrossSize = 0;

        for (size_t i = 0; i < line.items.size(); ++i) {
            size_t idx = line.indices[i];
            auto* child = line.items[i];
            i32 childMain = isHorizontal ? m_measuredSizes[idx].width : m_measuredSizes[idx].height;
            i32 childCross = isHorizontal ? m_measuredSizes[idx].height : m_measuredSizes[idx].width;

            childMain += child->constraints().margin.horizontal();
            childCross += child->constraints().margin.vertical();

            lineMainSize += childMain;
            lineCrossSize = std::max(lineCrossSize, childCross);
        }

        // 加上间距
        if (!line.items.empty()) {
            lineMainSize += static_cast<i32>(line.items.size() - 1) * m_config.gap;
        }

        maxLineMainSize = std::max(maxLineMainSize, lineMainSize);
        totalCrossSize += lineCrossSize;
    }

    // 加上换行间距
    if (m_lines.size() > 1) {
        totalCrossSize += static_cast<i32>(m_lines.size() - 1) * m_config.gap;
    }

    totalMainSize = maxLineMainSize;

    Size result;
    if (isHorizontal) {
        result.width = widthSpec.resolve(totalMainSize);
        result.height = heightSpec.resolve(totalCrossSize);
    } else {
        result.width = widthSpec.resolve(totalCrossSize);
        result.height = heightSpec.resolve(totalMainSize);
    }

    return result;
}

void FlexLayout::_measureChildren(
    const std::vector<WidgetLayoutAdaptor*>& children, i32 mainAxisSize, bool isHorizontal)
{
    MeasureSpec mainSpec = MeasureSpec::MakeAtMost(mainAxisSize);
    MeasureSpec crossSpec = MeasureSpec::MakeUnspecified();

    for (size_t i = 0; i < children.size(); ++i) {
        auto* child = children[i];
        if (!child || !child->constraints().isLayoutEnabled()) {
            m_measuredSizes[i] = Size(0, 0);
            continue;
        }

        // 根据主轴方向确定测量规格
        MeasureSpec childMainSpec, childCrossSpec;
        if (isHorizontal) {
            childMainSpec = MeasureSpec::MakeAtMost(mainAxisSize - child->constraints().margin.horizontal());
            childCrossSpec = MeasureSpec::MakeUnspecified();
        } else {
            childMainSpec = MeasureSpec::MakeUnspecified();
            childCrossSpec = MeasureSpec::MakeAtMost(mainAxisSize - child->constraints().margin.vertical());
        }

        // 考虑子元素的最小/最大尺寸约束
        const auto& constraints = child->constraints();
        if (constraints.preferredWidth >= 0 || constraints.preferredHeight >= 0) {
            // 有明确尺寸偏好
            if (isHorizontal) {
                if (constraints.preferredWidth >= 0) {
                    childMainSpec = MeasureSpec::MakeExactly(constraints.preferredWidth);
                }
                if (constraints.preferredHeight >= 0) {
                    childCrossSpec = MeasureSpec::MakeExactly(constraints.preferredHeight);
                }
            } else {
                if (constraints.preferredHeight >= 0) {
                    childMainSpec = MeasureSpec::MakeExactly(constraints.preferredHeight);
                }
                if (constraints.preferredWidth >= 0) {
                    childCrossSpec = MeasureSpec::MakeExactly(constraints.preferredWidth);
                }
            }
        }

        Size size = child->measure(
            isHorizontal ? childMainSpec : childCrossSpec, isHorizontal ? childCrossSpec : childMainSpec);

        // 应用最小/最大约束
        size.width = constraints.clampWidth(size.width);
        size.height = constraints.clampHeight(size.height);

        m_measuredSizes[i] = size;
    }
}

void FlexLayout::_collectLines(const std::vector<WidgetLayoutAdaptor*>& children, i32 mainAxisSize, bool isHorizontal)
{
    m_lines.clear();

    if (m_config.wrap == Wrap::NoWrap || mainAxisSize <= 0) {
        // 不换行，所有元素放在一行
        FlexLine line;
        for (size_t i = 0; i < children.size(); ++i) {
            auto* child = children[i];
            if (!child || !child->constraints().isLayoutEnabled()) continue;

            i32 mainSize = isHorizontal ? m_measuredSizes[i].width : m_measuredSizes[i].height;
            mainSize += child->constraints().margin.horizontal();
            line.addItem(i, child, mainSize);
        }
        m_lines.push_back(std::move(line));
        return;
    }

    // 换行模式
    FlexLine currentLine;
    i32 currentMainSize = 0;
    i32 gap = m_config.gap;

    for (size_t i = 0; i < children.size(); ++i) {
        auto* child = children[i];
        if (!child || !child->constraints().isLayoutEnabled()) continue;

        i32 itemMainSize = isHorizontal ? m_measuredSizes[i].width : m_measuredSizes[i].height;
        itemMainSize +=
            isHorizontal ? child->constraints().margin.horizontal() : child->constraints().margin.vertical();

        // 检查是否需要换行
        i32 newSize = currentMainSize + itemMainSize;
        if (!currentLine.items.empty()) {
            newSize += gap;
        }

        if (newSize > mainAxisSize && !currentLine.items.empty()) {
            // 当前行放不下，开始新行
            m_lines.push_back(std::move(currentLine));
            currentLine = FlexLine();
            currentMainSize = 0;
            newSize = itemMainSize;
        }

        currentLine.addItem(i, child, itemMainSize);
        currentMainSize = newSize;
    }

    // 添加最后一行
    if (!currentLine.items.empty()) {
        m_lines.push_back(std::move(currentLine));
    }
}

void FlexLayout::_distributeFreeSpace(FlexLine& line, i32 freeSpace, bool isHorizontal)
{
    if (freeSpace <= 0 || line.totalGrow <= 0.0f) return;

    // 按grow比例分配剩余空间
    for (size_t i = 0; i < line.items.size(); ++i) {
        auto* child = line.items[i];
        size_t idx = line.indices[i];

        if (child->flexItem().grow <= 0.0f) continue;

        f32 ratio = child->flexItem().grow / line.totalGrow;
        i32 extra = static_cast<i32>(freeSpace * ratio);

        if (isHorizontal) {
            m_measuredSizes[idx].width += extra;
        } else {
            m_measuredSizes[idx].height += extra;
        }
    }
}

void FlexLayout::_shrinkSpace(FlexLine& line, i32 overflow, bool isHorizontal)
{
    if (overflow <= 0 || line.totalShrink <= 0.0f) return;

    // 按shrink比例缩小
    for (size_t i = 0; i < line.items.size(); ++i) {
        auto* child = line.items[i];
        size_t idx = line.indices[i];

        if (child->flexItem().shrink <= 0.0f) continue;

        f32 ratio = child->flexItem().shrink / line.totalShrink;
        i32 reduction = static_cast<i32>(overflow * ratio);

        if (isHorizontal) {
            m_measuredSizes[idx].width =
                std::max(child->constraints().minWidth, m_measuredSizes[idx].width - reduction);
        } else {
            m_measuredSizes[idx].height =
                std::max(child->constraints().minHeight, m_measuredSizes[idx].height - reduction);
        }
    }
}

void FlexLayout::_applyJustifyContent(FlexLine& line, i32 mainAxisSize, bool isHorizontal)
{
    if (line.items.empty()) return;

    // 计算当前行的主轴总尺寸
    i32 lineMainSize = 0;
    for (size_t i = 0; i < line.items.size(); ++i) {
        size_t idx = line.indices[i];
        auto* child = line.items[i];
        i32 main = isHorizontal ? m_measuredSizes[idx].width : m_measuredSizes[idx].height;
        main += child->constraints().margin.horizontal();
        lineMainSize += main;
    }
    lineMainSize += static_cast<i32>(line.items.size() - 1) * m_config.gap;

    i32 freeSpace = mainAxisSize - lineMainSize;
    if (freeSpace < 0) freeSpace = 0;

    i32 offset = 0;
    i32 extraGap = 0;

    switch (m_config.justifyContent) {
        case JustifyContent::Start:
            // 默认行为，不需要调整
            break;

        case JustifyContent::End:
            offset = freeSpace;
            break;

        case JustifyContent::Center:
            offset = freeSpace / 2;
            break;

        case JustifyContent::SpaceBetween:
            if (line.items.size() > 1) {
                extraGap = freeSpace / static_cast<i32>(line.items.size() - 1);
            }
            break;

        case JustifyContent::SpaceAround:
            if (!line.items.empty()) {
                extraGap = freeSpace / static_cast<i32>(line.items.size());
                offset = extraGap / 2;
            }
            break;

        case JustifyContent::SpaceEvenly:
            if (!line.items.empty()) {
                extraGap = freeSpace / static_cast<i32>(line.items.size() + 1);
                offset = extraGap;
            }
            break;
    }

    // 应用偏移
    i32 currentPos = offset;
    for (size_t i = 0; i < line.items.size(); ++i) {
        size_t idx = line.indices[i];
        auto* child = line.items[i];

        // 设置位置和尺寸
        if (isHorizontal) {
            m_results[idx].bounds.x = currentPos + child->constraints().margin.left;
            m_results[idx].bounds.width = m_measuredSizes[idx].width;
        } else {
            m_results[idx].bounds.y = currentPos + child->constraints().margin.top;
            m_results[idx].bounds.height = m_measuredSizes[idx].height;
        }

        i32 main = isHorizontal ? m_measuredSizes[idx].width : m_measuredSizes[idx].height;
        main += child->constraints().margin.horizontal();

        currentPos += main + m_config.gap + extraGap;
    }
}

void FlexLayout::_applyAlignItems(FlexLine& line, i32 crossAxisSize, bool isHorizontal)
{
    // 先收集行内最大基线（用于Baseline对齐）
    i32 maxBaseline = 0;
    bool hasBaseline = false;
    for (size_t i = 0; i < line.items.size(); ++i) {
        Align align = line.items[i]->flexItem().alignSelf;
        if (align == Align::Stretch) {
            align = m_config.alignItems;
        }
        if (align == Align::Baseline) {
            i32 baseline = line.items[i]->getBaseline();
            if (baseline > maxBaseline) {
                maxBaseline = baseline;
            }
            hasBaseline = true;
        }
    }

    for (size_t i = 0; i < line.items.size(); ++i) {
        auto* child = line.items[i];
        size_t idx = line.indices[i];

        Align align = child->flexItem().alignSelf;
        if (align == Align::Stretch) {
            align = m_config.alignItems;
        }

        i32 childCross = isHorizontal ? m_measuredSizes[idx].height : m_measuredSizes[idx].width;
        i32 childMarginCross =
            isHorizontal ? child->constraints().margin.vertical() : child->constraints().margin.horizontal();

        i32 totalChildCross = childCross + childMarginCross;
        i32 freeCross = crossAxisSize - totalChildCross;

        // 先设置交叉轴尺寸
        if (isHorizontal) {
            m_results[idx].bounds.height = m_measuredSizes[idx].height;
        } else {
            m_results[idx].bounds.width = m_measuredSizes[idx].width;
        }

        if (freeCross <= 0) {
            // 交叉轴空间不足，不调整位置，使用默认起始位置
            if (isHorizontal) {
                m_results[idx].bounds.y = child->constraints().margin.top;
            } else {
                m_results[idx].bounds.x = child->constraints().margin.left;
            }
            continue;
        }

        i32 crossOffset = child->constraints().margin.top; // 默认从上/左开始

        switch (align) {
            case Align::Start:
                crossOffset = isHorizontal ? child->constraints().margin.top : child->constraints().margin.left;
                break;

            case Align::End:
                crossOffset = crossAxisSize - childCross -
                    (isHorizontal ? child->constraints().margin.bottom : child->constraints().margin.right);
                break;

            case Align::Center:
                crossOffset = (crossAxisSize - totalChildCross) / 2 +
                    (isHorizontal ? child->constraints().margin.top : child->constraints().margin.left);
                break;

            case Align::Stretch:
                // 拉伸到交叉轴尺寸
                if (isHorizontal) {
                    m_measuredSizes[idx].height = crossAxisSize - childMarginCross;
                    m_measuredSizes[idx].height = child->flexItem().clampHeight(m_measuredSizes[idx].height);
                    m_results[idx].bounds.height = m_measuredSizes[idx].height;
                } else {
                    m_measuredSizes[idx].width = crossAxisSize - childMarginCross;
                    m_measuredSizes[idx].width = child->flexItem().clampWidth(m_measuredSizes[idx].width);
                    m_results[idx].bounds.width = m_measuredSizes[idx].width;
                }
                crossOffset = isHorizontal ? child->constraints().margin.top : child->constraints().margin.left;
                break;

            case Align::Baseline:
                if (hasBaseline && isHorizontal) {
                    // 基线对齐：子元素的基线与行内最大基线对齐
                    i32 childBaseline = child->getBaseline();
                    crossOffset = maxBaseline - childBaseline + child->constraints().margin.top;
                } else {
                    // 非水平布局时退化为Start
                    crossOffset = isHorizontal ? child->constraints().margin.top : child->constraints().margin.left;
                }
                break;
        }

        if (isHorizontal) {
            m_results[idx].bounds.y = crossOffset;
        } else {
            m_results[idx].bounds.x = crossOffset;
        }
    }
}

void FlexLayout::_layoutLine(FlexLine& line, i32 mainAxisSize, i32 crossAxisSize, i32 lineOffset, bool isHorizontal)
{
    // TODO: 当前布局逻辑已整合到compute()中，此方法暂未使用，待重构时启用
    (void)line;
    (void)mainAxisSize;
    (void)crossAxisSize;
    (void)lineOffset;
    (void)isHorizontal;
}

void FlexLayout::_positionChildren(const Rect& containerBounds, bool isHorizontal)
{
    // TODO: 当前布局逻辑已整合到compute()中，此方法暂未使用，待重构时启用
    (void)containerBounds;
    (void)isHorizontal;
}

i32 FlexLayout::_calculateMainAxisSize(WidgetLayoutAdaptor* child, const MeasureSpec& mainSpec, bool isHorizontal)
{
    const auto& constraints = child->constraints();

    if (isHorizontal) {
        if (constraints.preferredWidth >= 0) {
            return constraints.preferredWidth;
        }
        return mainSpec.resolve(constraints.minWidth);
    } else {
        if (constraints.preferredHeight >= 0) {
            return constraints.preferredHeight;
        }
        return mainSpec.resolve(constraints.minHeight);
    }
}

i32 FlexLayout::_calculateCrossAxisSize(
    WidgetLayoutAdaptor* child, const MeasureSpec& crossSpec, bool isHorizontal, i32 baseline)
{
    const auto& constraints = child->constraints();

    if (isHorizontal) {
        if (constraints.preferredHeight >= 0) {
            return constraints.preferredHeight;
        }
        return crossSpec.resolve(constraints.minHeight);
    } else {
        if (constraints.preferredWidth >= 0) {
            return constraints.preferredWidth;
        }
        return crossSpec.resolve(constraints.minWidth);
    }
}

} // namespace mc::client::ui::kagero::layout
